.class public test_no_store_store_conflict
.super java/lang/Object

; ======================================================================

.method public <init>()V
   aload_0
   invokenonvirtual java/lang/Object/<init>()V
   return
.end method

; ======================================================================

.method public static checkI(I)V
	.limit locals 1
	.limit stack 10
	getstatic java/lang/System/out Ljava/io/PrintStream;
	iload_0
	invokevirtual java/io/PrintStream/println(I)V
	return
.end method

; ======================================================================

.method public static main([Ljava/lang/String;)V
	.limit stack 2
	.limit locals 3

	ldc 35
	istore 1
	ldc 36
	istore 2

	aload 0
	ifnull force_basic_block_boundary

	; --------------------------------------------------

	ldc 42
	ldc 7
	istore 2
	istore 1

	; --------------------------------------------------

force_basic_block_boundary:

	iload 1
	invokestatic test_no_store_store_conflict/checkI(I)V
	; OUTPUT: 42

	iload 2
	invokestatic test_no_store_store_conflict/checkI(I)V
	; OUTPUT: 7

	return
.end method
