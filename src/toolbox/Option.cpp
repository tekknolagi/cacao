/* src/toolbox/Option.cpp - Command line option parsing library

   Copyright (C) 1996-2014
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

#include "toolbox/Option.hpp"
#include "toolbox/Debug.hpp"
#include "toolbox/OStream.hpp"

namespace cacao {

namespace option {

OptionPrefix& root(){
	static OptionPrefix prefix("-");
	return prefix;
}

OptionPrefix& xx_root(){
	static OptionPrefix prefix("-XX:");
	return prefix;
}

} // end namespace option

OptionPrefix::OptionPrefix(const char* name) : name(name), s(std::strlen(name)) {
	err() << "OptionPrefix: " << name << nl;
}
void OptionPrefix::insert(OptionEntry* oe) {
	err() << "OptionPrefix::insert: " << oe->get_name() << nl;
	children.insert(oe);
}

void OptionParser::print_usage(OptionPrefix& root, FILE *fp) {
	static const char* blank29 = "                             "; // 29 spaces
	static const char* blank25 = blank29 + 4;                     // 25 spaces
	OStream OS(fp);

	std::set<OptionEntry*> sorted(root.begin(),root.end());

	for(std::set<OptionEntry*>::iterator i = sorted.begin(), e = sorted.end();
			i != e; ++i) {
		OptionEntry& oe = **i;
		std::size_t name_len = oe.size() + root.size();
		OS << "    " << root.get_name() << oe.get_name();
		if (name_len < (25-1)) {
			OS << (blank25 + name_len);
		} else {
			OS << nl << blank29;
		}
		// TODO description line break
		OS << oe.get_desc() << nl;
	}

}

namespace {

bool option_matcher(const char* a, std::size_t a_len,
		const char* b, std::size_t b_len) {
	return (a_len == b_len) && std::strncmp(a, b, a_len) == 0;
}

} // end anonymous namespace

bool OptionParser::parse_option(OptionPrefix& root, const char* name, size_t name_len,
		const char* value, size_t value_len) {
	assert(std::strncmp(root.get_name(), name, std::strlen(root.get_name())) == 0);
		//"root name: " << root.get_name() << " name: " << name );
	name += root.size();
	name_len -= root.size();
	err() << "name: " << name << nl;
	err() << "name_len: " << name_len << nl;
	err() << "value: " << value << nl;
	err() << "value_len: " << value_len << nl;
	for(OptionPrefix::iterator i = root.begin(), e = root.end();
			i != e; ++i) {
		OptionEntry& oe = **i;
		err() << "OptionEntry: " << oe.get_name() << nl;
		if (option_matcher(oe.get_name(), oe.size(), name, name_len)) {
			if (oe.parse(value))
				return true;
			return false;
		}
	}
	return false;
}

template<>
bool Option<const char*>::parse(const char* value) {
	err() << "set_value " << get_name() << " := " << value << nl;
	set_value(value);
	err() << "DEBUG: " << DEBUG_COND_WITH_NAME("properties") << nl;
	return true;
}

} // end namespace cacao

/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
