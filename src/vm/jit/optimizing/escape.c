/* src/vm/optimizing/escape.c

   Copyright (C) 2008
   CACAOVM - Verein zu Foerderung der freien virtuellen Machine CACAO

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.
*/

#include "vm/jit/jit.h"
#include "vmcore/class.h"
#include "vm/jit/optimizing/escape.h"

/*** escape_state *************************************************************/

const char *escape_state_to_string(escape_state_t escape_state) {
#	define str(x) case x: return #x;
	switch (escape_state) {
		str(ESCAPE_UNKNOWN)
		str(ESCAPE_NONE)
		str(ESCAPE_METHOD)
		str(ESCAPE_GLOBAL_THROUGH_METHOD)
		str(ESCAPE_GLOBAL)
		default: return "???";
	}
#	undef str
}

/*** instruction **************************************************************/

static inline s2 instruction_get_opcode(const instruction *iptr) {
	if (iptr->opc == ICMD_BUILTIN) {
		return iptr->sx.s23.s3.bte->opcode;
	} else {
		return iptr->opc;
	}
}

static inline bool instruction_is_unresolved(const instruction *iptr) {
	return iptr->flags.bits & INS_FLAG_UNRESOLVED;
}

static inline s4 instruction_field_type(const instruction *iptr) {
	if (instruction_is_unresolved(iptr)) {
		return iptr->sx.s23.s3.uf->fieldref->parseddesc.fd->type;
	} else {
		return iptr->sx.s23.s3.fmiref->p.field->type;
	}
}

static inline s4 instruction_s1(const instruction *iptr) {
	return iptr->s1.varindex;
}

static inline s4 instruction_s2(const instruction *iptr) {
	return iptr->sx.s23.s2.varindex;
}

static inline s4 instruction_s3(const instruction *iptr) {
	return iptr->sx.s23.s3.varindex;
}

static inline s4 instruction_dst(const instruction *iptr) {
	return iptr->dst.varindex;
}

static inline s4 instruction_arg(const instruction *iptr, int arg) {
	return iptr->sx.s23.s2.args[arg];
}

static inline bool instruction_is_class_constant(const instruction *iptr) {
	return iptr->flags.bits & INS_FLAG_CLASS;
}

static inline classinfo *instruction_classinfo(const instruction *iptr) {
	return iptr->sx.val.c.cls;
}

static inline methodinfo *instruction_local_methodinfo(const instruction *iptr) {
	if (instruction_is_unresolved(iptr)) {
		return NULL;
	} else {
		return iptr->sx.s23.s3.fmiref->p.method;
	}
}

static inline int instruction_dst_type(const instruction *iptr, jitdata *jd) {
	return VAROP(iptr->dst)->type;
}

static inline int instruction_return_type(const instruction *iptr) {
	return instruction_call_site(iptr)->returntype.type;
}

static inline s4 instruction_arg_type(const instruction *iptr, int arg) {
	methoddesc *md = instruction_call_site(iptr);
	assert(0 <= arg && arg < md->paramcount);
	return md->paramtypes[arg].type;
}

static inline int instruction_arg_count(const instruction *iptr) {
	return instruction_call_site(iptr)->paramcount;
}

/*** instruction_list ********************************************************/

typedef struct instruction_list_item {
	instruction *instr;
	struct instruction_list_item *next;
} instruction_list_item_t;

typedef struct {
	instruction_list_item_t *first;
} instruction_list_t;

void instruction_list_init(instruction_list_t *list) {
	list->first = NULL;
}

void instruction_list_add(instruction_list_t *list, instruction *instr) {
	instruction_list_item_t *item = DNEW(instruction_list_item_t);
	item->instr = instr;
	item->next = list->first;
	list->first = item;
}

#define FOR_EACH_INSTRUCTION_LIST(list, it) \
	for ((it) = (list)->first; (it) != NULL; (it) = (it)->next)

/*** escape_analysis *********************************************************/

struct var_extra;

typedef struct {
	jitdata *jd;

	instruction_list_t *allocations;
	instruction_list_t *getfields;

	struct var_extra **var;

	unsigned adr_args_count;

	bool verbose;

} escape_analysis_t;

/*** dependency_list_item ****************************************************/

typedef struct dependency_list_item {
	instruction *store;
	struct dependency_list_item *next;
} dependency_list_item_t;

bool dependency_list_item_compare(const dependency_list_item_t *item, const instruction *load) {

	instruction *store = item->store;
	utf *storen;
	const utf *loadn;

	if (load->opc == ICMD_AALOAD) {

		if (store->opc != ICMD_AASTORE) {
			return false;
		}

		return true;

	} else {

		if (store->opc != ICMD_PUTFIELD) {
			return false;
		}

		if (
			instruction_is_unresolved(store) !=
			instruction_is_unresolved(load)
		) {
			return false;
		}

		if (instruction_is_unresolved(store)) {
			storen = store->sx.s23.s3.uf->fieldref->name;
			loadn = load->sx.s23.s3.uf->fieldref->name;
		} else {
			storen = store->sx.s23.s3.fmiref->name;
			loadn = load->sx.s23.s3.fmiref->name;
		}

		/* TODO pointer equality ? */	

		if (storen->blength != loadn->blength) {
			return false;
		}

		return (strcmp(storen->text, loadn->text) == 0);
	}
}

/* TODO rename */
s4 dependency_list_item_get_dependency(const dependency_list_item_t *item) {
	switch (item->store->opc) {
		case ICMD_AASTORE:
			return instruction_s3(item->store);
		case ICMD_PUTFIELD:
			return instruction_s2(item->store);
		default:
			assert(0);
			return 0;
	}
}

/*** dependency_list *********************************************************/

typedef struct {
	dependency_list_item_t *first;
	dependency_list_item_t *last;
} dependency_list_t;

void dependency_list_init(dependency_list_t *dl) {
	dl->first = NULL;
	dl->last = NULL;
}

void dependency_list_add(dependency_list_t *dl, instruction *store) {
	dependency_list_item_t *item = DNEW(dependency_list_item_t);

	item->store = store;
	item->next = NULL;
	
	if (dl->first == NULL) {
		dl->first = item;
		dl->last = item;
	} else {
		dl->last->next = item;
		dl->last = item;
	}
}

void dependenCy_list_import(dependency_list_t *dl, dependency_list_t *other) {

	if (other == NULL) {
		return;
	}

	if (dl->first == NULL) {
		*dl = *other;
	} else {
		dl->last->next = other->first;
		dl->last = other->last;
	}

	other->first = NULL;
	other->last = NULL;
	
}

#define FOR_EACH_DEPENDENCY_LIST(dl, it) \
	for ((it) = (dl)->first; (it) != NULL; (it) = (it)->next)

/*** var_extra ***************************************************************/

typedef struct var_extra {
	instruction *allocation;
	escape_state_t escape_state;
	s4 representant;
	dependency_list_t *dependency_list;
	bool is_arg; /* TODO optimize */
} var_extra_t;

static void var_extra_init(var_extra_t *ve) {
	ve->allocation = NULL;
	ve->escape_state = ESCAPE_NONE;
	ve->representant = -1;
	ve->dependency_list = NULL;
	ve->is_arg = false;
}

static inline var_extra_t *var_extra_get_no_alloc(const escape_analysis_t *e, s4 var) {
	return e->var[var];
}

static var_extra_t* var_extra_get(escape_analysis_t *e, s4 var) {
	var_extra_t *ve;

	assert(0 <= var && var <= e->jd->vartop);

	ve = var_extra_get_no_alloc(e, var);

	if (ve == NULL) {
		ve = DNEW(var_extra_t);
		var_extra_init(ve);
		e->var[var] = ve;
	}

	return ve;
}

static s4 var_extra_get_representant(escape_analysis_t *e, s4 var) {
	var_extra_t *ve;
#if !defined(NDEBUG)
	int ctr = 0;
#endif

	ve = var_extra_get(e, var);

	while (ve->representant != -1) {
		assert(ctr++ < 10000);
		var = ve->representant;
		ve = var_extra_get_no_alloc(e, var);
		assert(ve);
	}

	return var;
}

static escape_state_t var_extra_get_escape_state(escape_analysis_t *e, s4 var) {
	var_extra_t *ve;
	
	var = var_extra_get_representant(e, var);
	ve = var_extra_get(e, var);

	return ve->escape_state;
}

static void var_extra_set_escape_state(escape_analysis_t *e, s4 var, escape_state_t escape_state) {
	var_extra_t *ve;
	
	var = var_extra_get_representant(e, var);
	ve = var_extra_get(e, var);

	ve->escape_state = escape_state;
}

static dependency_list_t *var_extra_get_dependency_list(escape_analysis_t *e, s4 var) {
	var_extra_t *ve;
	
	var = var_extra_get_representant(e, var);
	ve = var_extra_get(e, var);

	if (ve->dependency_list == NULL) {
		ve->dependency_list = DNEW(dependency_list_t);
		dependency_list_init(ve->dependency_list);
	}

	return ve->dependency_list;
}

/*** escape_analysis *********************************************************/

static void escape_analysis_init(escape_analysis_t *e, jitdata *jd) {
	e->jd = jd;

	e->allocations = DNEW(instruction_list_t);
	instruction_list_init(e->allocations);

	e->getfields = DNEW(instruction_list_t);
	instruction_list_init(e->getfields);

	e->var = DMNEW(var_extra_t *, jd->vartop);
	MZERO(e->var, var_extra_t *, jd->vartop);

	e->adr_args_count = 0;

	e->verbose = (
		strcmp(e->jd->m->clazz->name->text, "gnu/java/util/regex/RESyntax") == 0 
		&& strcmp(e->jd->m->name->text, "<clinit>") == 0
	);
	e->verbose = 1;
}

static void escape_analysis_set_allocation(escape_analysis_t *e, s4 var, instruction *iptr) {
	var_extra_get(e, var)->allocation = iptr;
}

static instruction *escape_analysis_get_allocation(const escape_analysis_t *e, s4 var) {
	var_extra_t *ve = var_extra_get_no_alloc(e, var);

	assert(ve != NULL);
	assert(ve->allocation != NULL);

	return ve->allocation;
}

static void escape_analysis_set_is_argument(escape_analysis_t *e, s4 var) {
	var_extra_get(e, var)->is_arg = true;
}

static bool escape_analysis_get_is_argument(escape_analysis_t *e, s4 var) {
	return var_extra_get(e, var)->is_arg;
}

static void escape_analysis_ensure_state(escape_analysis_t *e, s4 var, escape_state_t escape_state) {
	
	var_extra_t *ve;
	dependency_list_item_t *it;

	var = var_extra_get_representant(e, var);
	ve = var_extra_get(e, var);

	if (ve->escape_state < escape_state) {
		if (e->verbose) {
			printf(
				"escape state of %d %s => %s\n", 
				var, 
				escape_state_to_string(ve->escape_state), 
				escape_state_to_string(escape_state)
			);
		}
		ve->escape_state = escape_state;
		if (ve->dependency_list != NULL) {
			FOR_EACH_DEPENDENCY_LIST(ve->dependency_list, it) {
				if (e->verbose) {
					printf("propagating to %s@%d\n", icmd_table[it->store->opc].name, it->store->line);
				}
				escape_analysis_ensure_state(
					e, 
					dependency_list_item_get_dependency(it),
					escape_state
				);
			}
		}
	}
}

static escape_state_t escape_analysis_get_state(escape_analysis_t *e, s4 var) {
	return var_extra_get_escape_state(e, var);
}

#define escape_analysis_assert_has_escape(e, var) \
	assert( \
		var_extra_get_no_alloc(e, var) && \
		(var_extra_get_no_alloc(e, var)->escape_state > ESCAPE_UNKNOWN) \
	)

static classinfo *escape_analysis_classinfo_in_var(escape_analysis_t *e, s4 var) {
	instruction *iptr = escape_analysis_get_allocation(e, var);

	if (iptr == NULL) {
		return NULL;
	}

	if (! instruction_is_class_constant(iptr)) {
		return NULL;
	}

	if (instruction_dst(iptr) != var) {
		return NULL;
	}

	if (instruction_is_unresolved(iptr)) {
		return NULL;
	}

	return instruction_classinfo(iptr);
}

static void escape_analysis_merge(escape_analysis_t *e, s4 var1, s4 var2) {

	var_extra_t *ve1, *ve2;
	dependency_list_item_t *itd;
	bool has_become_arg;

	var1 = var_extra_get_representant(e, var1);
	var2 = var_extra_get_representant(e, var2);

	/* Don't merge the same escape sets. */

	if (var1 == var2) {
		return;
	}

	ve1 = var_extra_get(e, var1);
	ve2 = var_extra_get(e, var2);

	/* Adjust escape state to maximal escape state. */

	escape_analysis_ensure_state(e, var1, ve2->escape_state);
	escape_analysis_ensure_state(e, var2, ve1->escape_state);

	/* Representant of var1 becomes the representant of var2. */

	ve2->representant = var1;

	/* Adjust is_argument to logical or. */
	
	has_become_arg = ve1->is_arg != ve2->is_arg;
	ve1->is_arg = ve1->is_arg || ve2->is_arg;

	if (e->verbose && has_become_arg) printf("(%d,%d) has become arg.\n", var1, var2);

	/* Merge list of dependencies. */

	if (ve1->dependency_list == NULL) {
		ve1->dependency_list = ve2->dependency_list;
	} else {
		dependenCy_list_import(ve1->dependency_list, ve2->dependency_list);
	}

	/* If one of the merged values is an argument but the other not,
	   all dependencies of the newly created value escape globally. */

	if (has_become_arg && ve1->dependency_list != NULL) {
		FOR_EACH_DEPENDENCY_LIST(ve1->dependency_list, itd) {
			escape_analysis_ensure_state(
				e,
				dependency_list_item_get_dependency(itd),
				ESCAPE_GLOBAL
			);
		}
	}
}

static void escape_analysis_add_dependency(escape_analysis_t *e, instruction *store) {
	s4 obj = instruction_s1(store);
	dependency_list_t *dl = var_extra_get_dependency_list(e, obj);

	assert(store->opc == ICMD_PUTFIELD || store->opc == ICMD_AASTORE);

	dependency_list_add(dl, store);

	if (e->verbose) {
		printf("dependency_list_add\n");
	}
}

static void escape_analysis_process_instruction(escape_analysis_t *e, instruction *iptr) {
	jitdata *jd = e->jd;
	classinfo *c;
	s4 count;
	u1 *paramescape;
	unsigned i;
	instruction **iarg;
	constant_FMIref *fmi;
	methodinfo *mi;
	resolve_result_t result;

	if (e->verbose) {
		printf("processing %s@%d\n", icmd_table[iptr->opc].name, iptr->line);
	}

	switch (instruction_get_opcode(iptr)) {
		case ICMD_ACONST:

			escape_analysis_set_allocation(e, instruction_dst(iptr), iptr);
			escape_analysis_ensure_state(e, instruction_dst(iptr), ESCAPE_NONE);

			break;

		case ICMD_NEW:
			c = escape_analysis_classinfo_in_var(e, instruction_arg(iptr, 0));

			escape_analysis_set_allocation(e, instruction_dst(iptr), iptr);

			if (c == NULL) {
				escape_analysis_ensure_state(e, instruction_dst(iptr), ESCAPE_GLOBAL);
				if (e->verbose) printf("1\n");
			} else if (c->finalizer != NULL) {
				escape_analysis_ensure_state(e, instruction_dst(iptr), ESCAPE_GLOBAL);
				if (e->verbose) printf("3\n");
			} else {
				escape_analysis_ensure_state(e, instruction_dst(iptr), ESCAPE_NONE);
				if (e->verbose) printf("2\n");
			}

			instruction_list_add(e->allocations, iptr);

			break;

		case ICMD_NEWARRAY:
		case ICMD_ANEWARRAY:
			
			escape_analysis_ensure_state(e, instruction_dst(iptr), ESCAPE_GLOBAL);
			escape_analysis_set_allocation(e, instruction_dst(iptr), iptr);
			instruction_list_add(e->allocations, iptr);

			break;

		case ICMD_PUTSTATIC:
			if (instruction_field_type(iptr) == TYPE_ADR) {
				/*
				escape_analysis_assert_has_escape(e, instruction_s1(iptr));
				*/
				escape_analysis_ensure_state(e, instruction_s1(iptr), ESCAPE_GLOBAL);
			}
			break;

		case ICMD_PUTFIELD:
			if (instruction_field_type(iptr) == TYPE_ADR) {
				/*
				escape_analysis_assert_has_escape(e, instruction_s1(iptr));
				escape_analysis_assert_has_escape(e, instruction_s2(iptr));
				*/
				if (escape_analysis_get_is_argument(e, instruction_s1(iptr))) {
					escape_analysis_ensure_state(e, instruction_s2(iptr), ESCAPE_GLOBAL);
				} else {
					escape_analysis_ensure_state(e, instruction_s2(iptr), escape_analysis_get_state(e, instruction_s1(iptr)));
					escape_analysis_add_dependency(e, iptr);
				}
			}
			break;

		case ICMD_AASTORE:
			/*
			escape_analysis_assert_has_escape(e, instruction_s1(iptr));
			escape_analysis_assert_has_escape(e, instruction_s3(iptr));
			*/
			if (escape_analysis_get_is_argument(e, instruction_s1(iptr))) {
				escape_analysis_ensure_state(e, instruction_s3(iptr), ESCAPE_GLOBAL);
			} else {
				escape_analysis_ensure_state(e, instruction_s3(iptr), escape_analysis_get_state(e, instruction_s1(iptr)));
				escape_analysis_add_dependency(e, iptr);
			}
			break;

		case ICMD_GETSTATIC:
			if (instruction_field_type(iptr) == TYPE_ADR) {
				escape_analysis_ensure_state(e, instruction_dst(iptr), ESCAPE_GLOBAL);
				escape_analysis_set_allocation(e, instruction_dst(iptr), iptr);
			}
			break;

		case ICMD_GETFIELD:
			if (instruction_field_type(iptr) == TYPE_ADR) {
				/*
				escape_analysis_assert_has_escape(e, instruction_s1(iptr));
				*/

				if (escape_analysis_get_is_argument(e, instruction_s1(iptr))) {
					/* Fields loaded from arguments escape globally.
					   x = arg.foo;
					   x.bar = y;
					   => y escapes globally. */
					escape_analysis_ensure_state(e, instruction_dst(iptr), ESCAPE_GLOBAL);
				} else {
					escape_analysis_ensure_state(e, instruction_dst(iptr), ESCAPE_NONE);
				}

				escape_analysis_set_allocation(e, instruction_dst(iptr), iptr);

				instruction_list_add(e->getfields, iptr);
			}
			break;

		case ICMD_ARRAYLENGTH:
			/*
			escape_analysis_assert_has_escape(e, instruction_s1(iptr));
			*/
			break;

		case ICMD_AALOAD:
			/*
			escape_analysis_assert_has_escape(e, instruction_s1(iptr));
			*/

			if (escape_analysis_get_is_argument(e, instruction_s1(iptr))) {
				/* If store into argument, escapes globally. See ICMD_GETFIELD. */
				escape_analysis_ensure_state(e, instruction_dst(iptr), ESCAPE_GLOBAL);
			} else {
				escape_analysis_ensure_state(e, instruction_dst(iptr), ESCAPE_NONE);
			}

			escape_analysis_set_allocation(e, instruction_dst(iptr), iptr);

			instruction_list_add(e->getfields, iptr);
			break;

		case ICMD_IF_ACMPEQ:
		case ICMD_IF_ACMPNE:
			/*
			escape_analysis_assert_has_escape(e, instruction_s1(iptr));
			escape_analysis_assert_has_escape(e, instruction_s2(iptr));
			*/
			escape_analysis_ensure_state(e, instruction_s1(iptr), ESCAPE_METHOD);
			escape_analysis_ensure_state(e, instruction_s2(iptr), ESCAPE_METHOD);
			break;

		case ICMD_IFNULL:
		case ICMD_IFNONNULL:
		case ICMD_CHECKNULL:
			/*
			escape_analysis_assert_has_escape(e, instruction_s1(iptr));
			*/
			escape_analysis_ensure_state(e, instruction_s1(iptr), ESCAPE_METHOD);
			break;

		case ICMD_CHECKCAST:
			/*
			escape_analysis_assert_has_escape(e, instruction_s1(iptr));
			*/
			escape_analysis_merge(e, instruction_s1(iptr), instruction_dst(iptr));
			escape_analysis_ensure_state(e, instruction_s1(iptr), ESCAPE_METHOD);
			escape_analysis_set_allocation(e, instruction_dst(iptr), iptr);
			break;

		case ICMD_INSTANCEOF:
			/*
			escape_analysis_assert_has_escape(e, instruction_s1(iptr));
			*/
			escape_analysis_ensure_state(e, instruction_s1(iptr), ESCAPE_METHOD);
			break;

		case ICMD_INVOKESPECIAL:
		case ICMD_INVOKEVIRTUAL:
		case ICMD_INVOKEINTERFACE:
		case ICMD_INVOKESTATIC:
			count = instruction_arg_count(iptr);
			mi = instruction_local_methodinfo(iptr);
			paramescape = NULL;

			if (mi != NULL) {
				/* If the method could be resolved, it already is. */
				paramescape = mi->paramescape;

				if (paramescape == NULL) {
					if (e->verbose) {
						printf("BC escape analyzing callee.\n");
					}
					bc_escape_analysis_perform(mi);
					paramescape = mi->paramescape;
				}
			} else {
				if (e->verbose) {
					printf("Unresolved callee.\n");
				}
			}

			for (i = 0; i < count; ++i) {
				if (instruction_arg_type(iptr, i) == TYPE_ADR) {

					/*
					escape_analysis_assert_has_escape(e, instruction_arg(iptr, i));
					*/
					if (paramescape == NULL) {
						escape_analysis_ensure_state(
							e, 
							instruction_arg(iptr, i), 
							instruction_local_methodinfo(iptr) && instruction_local_methodinfo(iptr)->jcode ?
								ESCAPE_GLOBAL_THROUGH_METHOD :
								ESCAPE_GLOBAL
						);
					} else if ((escape_state_t)*paramescape < ESCAPE_METHOD) {
						escape_analysis_ensure_state(e, instruction_arg(iptr, i), ESCAPE_METHOD);
					} else {
						escape_analysis_ensure_state(e, instruction_arg(iptr, i), (escape_state_t)*paramescape);
					}

					if (paramescape != NULL) {
						++paramescape;
					}
				}
			}

			if (instruction_return_type(iptr) == TYPE_ADR) {
				escape_analysis_ensure_state(e, instruction_dst(iptr), ESCAPE_NONE);
				escape_analysis_set_allocation(e, instruction_dst(iptr), iptr);
			}

			break;

		case ICMD_ATHROW:
			/*
			escape_analysis_assert_has_escape(e, instruction_s1(iptr));
			*/
			escape_analysis_ensure_state(e, instruction_s1(iptr), ESCAPE_GLOBAL);
			break;

		case ICMD_ARETURN:
			/*
			escape_analysis_assert_has_escape(e, instruction_s1(iptr));
			*/
			escape_analysis_ensure_state(e, instruction_s1(iptr), ESCAPE_GLOBAL);
			break;

		case ICMD_ALOAD:
		case ICMD_ASTORE:
		case ICMD_MOVE:
		case ICMD_COPY:
			if (instruction_dst_type(iptr, jd) == TYPE_ADR) {
				/*
				escape_analysis_assert_has_escape(e, instruction_s1(iptr));
				*/
				escape_analysis_merge(e, instruction_s1(iptr), instruction_dst(iptr));
				escape_analysis_set_allocation(e, instruction_dst(iptr), iptr);
			}
			break;

		case ICMD_PHI:
			for (
				iarg = iptr->sx.s23.s2.iargs;
				iarg != iptr->sx.s23.s2.iargs + iptr->s1.argcount;
				++iarg
			) {
				escape_analysis_merge(e, instruction_dst(iptr), instruction_dst(*iarg));
			}
			break;

		case ICMD_GETEXCEPTION:
			escape_analysis_ensure_state(e, instruction_dst(iptr), ESCAPE_NONE);
			break;
	}
}

static void escape_analysis_process_instructions(escape_analysis_t *e) {
	basicblock *bptr;
	instruction *iptr;

	FOR_EACH_BASICBLOCK(e->jd, bptr) {

		for (iptr = bptr->phis; iptr != bptr->phis + bptr->phicount; ++iptr) {
			escape_analysis_process_instruction(e, iptr);
		}

		FOR_EACH_INSTRUCTION(bptr, iptr) {
			escape_analysis_process_instruction(e, iptr);
		}

	}
}

static void escape_analysis_post_process_getfields(escape_analysis_t *e) {
	instruction_list_item_t *iti;
	dependency_list_item_t *itd;
	instruction *iptr;
	dependency_list_t *dl;

	FOR_EACH_INSTRUCTION_LIST(e->getfields, iti) {

		iptr = iti->instr;

		/* Get the object the field/element is loaded from. */

		dl = var_extra_get_dependency_list(e, instruction_s1(iptr));

		/* Adjust escape state of all objects in the dependency list,
		   referenced via the field of this getfield/arraystore. */

		if (dl != NULL) {
			FOR_EACH_DEPENDENCY_LIST(dl, itd) {
				if (dependency_list_item_compare(itd, iptr)) {

					/* Fields match. Adjust escape state. */

					escape_analysis_ensure_state(
						e, 
						dependency_list_item_get_dependency(itd),
						escape_analysis_get_state(e, instruction_dst(iptr))
					);
				}
			}
		}

	}
}

static void display_allocation(escape_analysis_t *e, const char *prefix, const instruction *iptr, escape_state_t es) {
	const char *cl = "WTF";
	classinfo *c;

	if (instruction_get_opcode(iptr) == ICMD_NEW) {
		c = escape_analysis_classinfo_in_var(e, instruction_arg(iptr, 0));
		if (c) {
			cl = c->name->text;
		}
	}
	

	printf(
		" %s %s %s: %s %s @%d %s\n", 
		prefix,
		e->jd->m->clazz->name->text,
		e->jd->m->name->text,
		icmd_table[iptr->opc].name,
		cl,
		iptr->line,
		escape_state_to_string(es)
	);
}

static void escape_analysis_mark_allocations(escape_analysis_t *e) {
	instruction_list_item_t *iti;
	escape_state_t es;
/*
	FOR_EACH_INSTRUCTION_LIST(e->allocations, iti) {
		es = escape_analysis_get_state(e, instruction_dst(iti->instr));
		if (es < ESCAPE_GLOBAL_THROUGH_METHOD) {
			display_allocation(e, "****", iti->instr, es);
		}
		if (es == ESCAPE_GLOBAL_THROUGH_METHOD) {
			display_allocation(e, "!!!!", iti->instr, es);
		}
	}
*/
}

static void escape_analysis_process_arguments(escape_analysis_t *e) {
	s4 p;
	s4 t;
	s4 l;
	s4 varindex;
	methoddesc *md;
	
	md = e->jd->m->parseddesc;

	for (p = 0, l = 0; p < md->paramcount; ++p) {
		t = md->paramtypes[p].type;
		varindex = e->jd->local_map[l * 5 + t];
		if (t == TYPE_ADR) {
			if (varindex != UNUSED) {
				escape_analysis_ensure_state(e, varindex, ESCAPE_NONE);
				escape_analysis_set_is_argument(e, varindex);
			}
			e->adr_args_count += 1;
		}
		l += IS_2_WORD_TYPE(t) ? 2 : 1;
	}
}

static void escape_analysis_export_arguments(escape_analysis_t *e) {
	s4 p;
	s4 t;
	s4 l;
	s4 varindex;
	methoddesc *md;
	u1 *paramescape;
	
	md = e->jd->m->parseddesc;

	paramescape = MNEW(u1, e->adr_args_count);
	e->jd->m->paramescape = paramescape;

	for (p = 0, l = 0; p < md->paramcount; ++p) {
		t = md->paramtypes[p].type;
		varindex = e->jd->local_map[l * 5 + t];
		if (t == TYPE_ADR) {
			if (varindex == UNUSED) {
				*paramescape = (u1)ESCAPE_NONE;
			} else {
				*paramescape = (u1)escape_analysis_get_state(e, varindex);
			}
			if (e->verbose) {
				printf("adr parameter %d: %s\n", p, escape_state_to_string((escape_state_t)*paramescape));
			}
			paramescape += 1;
		}
		l += IS_2_WORD_TYPE(t) ? 2 : 1;
	}
}

static void escape_analysis_display(escape_analysis_t *e) {
	instruction_list_item_t *iti;
	var_extra_t *ve;
	instruction *iptr;

	FOR_EACH_INSTRUCTION_LIST(e->allocations, iti) {
		iptr = iti->instr;
		ve = var_extra_get(e, instruction_dst(iptr));
		printf(
			"%s@%d: %s\n", 
			icmd_table[iptr->opc].name, 
			iptr->line, 
			escape_state_to_string(ve->escape_state)
		);
	}
}

void escape_analysis_perform(jitdata *jd) {
	escape_analysis_t *e;

	jd->m->flags |= ACC_METHOD_EA;

	/*bc_escape_analysis_perform(jd->m);*/

	e = DNEW(escape_analysis_t);
	escape_analysis_init(e, jd);

	if (e->verbose) 
		printf("==== %s/%s ====\n", e->jd->m->clazz->name->text, e->jd->m->name->text);
		
	/*fprintf(stderr, ".");*/

	escape_analysis_process_arguments(e);
	escape_analysis_process_instructions(e);
	escape_analysis_post_process_getfields(e);
	escape_analysis_export_arguments(e);
	if (e->verbose) escape_analysis_display(e);
	escape_analysis_mark_allocations(e);

	jd->m->flags &= ~ACC_METHOD_EA;
}

void escape_analysis_escape_check(void *vp) {
}