/***************************** alpha/ngen.c ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the codegenerator for an Alpha processor.
	This module generates Alpha machine code for a sequence of
	pseudo commands (ICMDs).

	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1998/08/10

*******************************************************************************/

public class fptest {
	public static void main(String [] s) {

		float  fnan  = Float.NaN;
		float  fpinf = Float.POSITIVE_INFINITY;
		float  fninf = Float.NEGATIVE_INFINITY;
		float  fmax  = Float.MAX_VALUE;
		float  fmin  = Float.MIN_VALUE;
		float  f1    = 0F;
		float  f2    = 0F;

		double dnan  = Double.NaN;
		double dpinf = Double.POSITIVE_INFINITY;
		double dninf = Double.NEGATIVE_INFINITY;
		double dmax  = Double.MAX_VALUE;
		double dmin  = Double.MIN_VALUE;
		double d1    = 0D;
		double d2    = 0D;
		
		p("---------------------------- tests NaNs and Infs -------------------");
		p("------------------- print NaNs and Infs");

		p("NaNQ ", fnan);
		p("+INF ", fpinf);
		p("-INF ", fninf);

		p("NaNQ ", dnan);
		p("+INF ", dpinf);
		p("-INF ", dninf);

		p("------------------- test zero division");

		zerodiv("0 / 0 = NaNQ ",  0F, f1);
		zerodiv("+ / 0 = +INF ",  5F, f1);
		zerodiv("- / 0 = -INF ", -5F, f1);

		zerodiv("0 / 0 = NaNQ ",  0D, d1);
		zerodiv("+ / 0 = +INF ",  5D, d1);
		zerodiv("- / 0 = -INF ", -5D, d1);

		p("------------------- test conversions");
		testfcvt("NaNQ", fnan, dnan);
		testfcvt("+INF", fpinf, dpinf);
		testfcvt("-INF", fninf, dninf);
		testfcvt(" MAX",  fmax, dmax);
		testfcvt(" MIN",  fmin, dmin);
		testfcvt("MAXINT-1",  2147483646.0F, 2147483646.0D);
		testfcvt("MAXINT+0",  2147483647.0F, 2147483647.0D);
		testfcvt("MAXINT+1",  2147483648.0F, 2147483648.0D);
		testfcvt("-MAXINT+1",  -2147483647.0F, -2147483647.0D);
		testfcvt("-MAXINT+0",  -2147483648.0F, -2147483648.0D);
		testfcvt("-MAXINT-1",  -2147483649.0F, -2147483649.0D);
		testfcvt("MAXLNG-1",  9223372036854775806.0F, 9223372036854775806.0D);
		testfcvt("MAXLNG+0",  9223372036854775807.0F, 9223372036854775807.0D);
		testfcvt("MAXLNG+1",  9223372036854775808.0F, 9223372036854775808.0D);
		testfcvt("-MAXLNG+1",  -9223372036854775807.0F, -9223372036854775807.0D);
		testfcvt("-MAXLNG+0",  -9223372036854775808.0F, -9223372036854775808.0D);
		testfcvt("-MAXLNG-1",  -9223372036854775809.0F, -9223372036854775809.0D);

		p("------------------- test NaNQ op value");
		testfops("NaNQ", "5.0", fnan, 5F, dnan, 5D);
		testfcmp("NaNQ", "5.0", fnan, 5F, dnan, 5D);

		p("------------------- test value op NaNQ");
		testfops("5.0", "NaNQ", 5F, fnan, 5D, dnan);
		testfcmp("5.0", "NaNQ", 5F, fnan, 5D, dnan);

		p("------------------- test +INF op value");
		testfops("+INF", "5.0", fpinf, 5F, dpinf, 5D);
		testfcmp("+INF", "5.0", fpinf, 5F, dpinf, 5D);

		p("------------------- test +INF op value");
		testfops("5.0", "+INF", 5F, fpinf, 5D, dpinf);
		testfcmp("5.0", "+INF", 5F, fpinf, 5D, dpinf);

		p("------------------- test -INF op value");
		testfops("-INF", "5.0", fninf, 5F, dninf, 5D);
		testfcmp("-INF", "5.0", fninf, 5F, dninf, 5D);

		p("------------------- test -INF op value");
		testfops("5.0", "-INF", 5F, fninf, 5D, dninf);
		testfcmp("5.0", "-INF", 5F, fninf, 5D, dninf);

		p("------------------- test MAX op value");
		testfops("MAX", "5.0", fmax, 5F, dmax, 5D);

		p("------------------- test value op MAX");
		testfops("5.0", "MAX", 5F, fmax, 5D, dmax);

		p("------------------- test MIN op value");
		testfops("MIN", "5.0", fmin, 5F, dmin, 5D);

		p("------------------- test value op MIN");
		testfops("5.0", "MIN", 5F, fmin, 5D, dmin);

		}
		
	public static void zerodiv(String s, float f1, float f2) {
		p(s, f1 / f2);
		}

	public static void zerodiv(String s, double d1, double d2) {
		p(s, d1 / d2);
		}

	public static void testfcvt(String s1, float f1, double d1) {
		p("convert " + s1 + " (" + f1 + "," + d1 + ") to ", (int)  f1);
		p("convert " + s1 + " (" + f1 + "," + d1 + ") to ", (int)  d1);
		p("convert " + s1 + " (" + f1 + "," + d1 + ") to ", (long) f1);
		p("convert " + s1 + " (" + f1 + "," + d1 + ") to ", (long) d1);
		}

	public static void testfops(String s1, String s2, float f1, float f2,
	                           double d1, double d2) {
		p(s1 + " + " + s2 + " = ", f1 + f2);
		p(s1 + " - " + s2 + " = ", f1 - f2);
		p(s1 + " * " + s2 + " = ", f1 * f2);
		p(s1 + " / " + s2 + " = ", f1 / f2);
		p(s1 + " % " + s2 + " = ", f1 % f2);
		p(s1 + " + " + s2 + " = ", d1 + d2);
		p(s1 + " - " + s2 + " = ", d1 - d2);
		p(s1 + " * " + s2 + " = ", d1 * d2);
		p(s1 + " / " + s2 + " = ", d1 / d2);
		p(s1 + " % " + s2 + " = ", d1 % d2);
		}

	public static void testfcmp(String s1, String s2, float f1, float f2,
	                           double d1, double d2) {

		if ( (f1 == f2)) p(" (" + s1 + " == " + s2 + ") = true");
		else             p(" (" + s1 + " == " + s2 + ") = false");
		if ( (f1 != f2)) p(" (" + s1 + " != " + s2 + ") = true");
		else             p(" (" + s1 + " != " + s2 + ") = false");
		if ( (f1 <  f2)) p(" (" + s1 + " <  " + s2 + ") = true");
		else             p(" (" + s1 + " <  " + s2 + ") = false");
		if ( (f1 <= f2)) p(" (" + s1 + " <= " + s2 + ") = true");
		else             p(" (" + s1 + " <= " + s2 + ") = false");
		if ( (f1 >  f2)) p(" (" + s1 + " >  " + s2 + ") = true");
		else             p(" (" + s1 + " >  " + s2 + ") = false");
		if ( (f1 >= f2)) p(" (" + s1 + " >= " + s2 + ") = true");
		else             p(" (" + s1 + " >= " + s2 + ") = false");

		if (!(f1 == f2)) p("!(" + s1 + " == " + s2 + ") = true");
		else             p("!(" + s1 + " == " + s2 + ") = false");
		if (!(f1 != f2)) p("!(" + s1 + " != " + s2 + ") = true");
		else             p("!(" + s1 + " != " + s2 + ") = false");
		if (!(f1 <  f2)) p("!(" + s1 + " <  " + s2 + ") = true");
		else             p("!(" + s1 + " <  " + s2 + ") = false");
		if (!(f1 <= f2)) p("!(" + s1 + " <= " + s2 + ") = true");
		else             p("!(" + s1 + " <= " + s2 + ") = false");
		if (!(f1 >  f2)) p("!(" + s1 + " >  " + s2 + ") = true");
		else             p("!(" + s1 + " >  " + s2 + ") = false");
		if (!(f1 >= f2)) p("!(" + s1 + " >= " + s2 + ") = true");
		else             p("!(" + s1 + " >= " + s2 + ") = false");

		if ( (d1 == d2)) p(" (" + s1 + " == " + s2 + ") = true");
		else             p(" (" + s1 + " == " + s2 + ") = false");
		if ( (d1 != d2)) p(" (" + s1 + " != " + s2 + ") = true");
		else             p(" (" + s1 + " != " + s2 + ") = false");
		if ( (d1 <  d2)) p(" (" + s1 + " <  " + s2 + ") = true");
		else             p(" (" + s1 + " <  " + s2 + ") = false");
		if ( (d1 <= d2)) p(" (" + s1 + " <= " + s2 + ") = true");
		else             p(" (" + s1 + " <= " + s2 + ") = false");
		if ( (d1 >  d2)) p(" (" + s1 + " >  " + s2 + ") = true");
		else             p(" (" + s1 + " >  " + s2 + ") = false");
		if ( (d1 >= d2)) p(" (" + s1 + " >= " + s2 + ") = true");
		else             p(" (" + s1 + " >= " + s2 + ") = false");

		if (!(d1 == d2)) p("!(" + s1 + " == " + s2 + ") = true");
		else             p("!(" + s1 + " == " + s2 + ") = false");
		if (!(d1 != d2)) p("!(" + s1 + " != " + s2 + ") = true");
		else             p("!(" + s1 + " != " + s2 + ") = false");
		if (!(d1 <  d2)) p("!(" + s1 + " <  " + s2 + ") = true");
		else             p("!(" + s1 + " <  " + s2 + ") = false");
		if (!(d1 <= d2)) p("!(" + s1 + " <= " + s2 + ") = true");
		else             p("!(" + s1 + " <= " + s2 + ") = false");
		if (!(d1 >  d2)) p("!(" + s1 + " >  " + s2 + ") = true");
		else             p("!(" + s1 + " >  " + s2 + ") = false");
		if (!(d1 >= d2)) p("!(" + s1 + " >= " + s2 + ") = true");
		else             p("!(" + s1 + " >= " + s2 + ") = false");
		}

	// ********************* output methods ****************************

	public static int linenum = 0;

	public static void pnl() {
		int i;

		System.out.println();
		for (i = 4 - Integer.toString(linenum).length(); i > 0; i--)
			System.out.print(' ');
		System.out.print(linenum);
		System.out.print(".    ");
		linenum++;
		}

	public static void p(String a) {
		System.out.print(a); pnl();
		}
	public static void p(boolean a) {
		System.out.print(a); pnl();
		}
	public static void p(int a) {
		System.out.print("int:    "); System.out.print(a); pnl();
		}
	public static void p(long a) {
		System.out.print("long:   "); System.out.print(a); pnl();
		}
	public static void p(short a) {
		System.out.print("short:  "); System.out.print(a); pnl();
		}
	public static void p(byte a) {
		System.out.print("byte:   "); System.out.print(a); pnl();
		}
	public static void p(char a) {
		System.out.print("char:   "); System.out.print((int)a); pnl();
		}
	public static void p(float a) {
		System.out.print("float:  "); System.out.print(a); pnl();
		}
	public static void p(double a) {
		System.out.print("double: "); System.out.print(a); pnl();
		}

	public static void p(String s, boolean i) { 
		System.out.print(s); p(i);
		}
	public static void p(String s, int i) { 
		System.out.print(s); p(i);
		}
	public static void p(String s, byte i) { 
		System.out.print(s); p(i);
		}
	public static void p(String s, char i) { 
		System.out.print(s); p(i);
		}
	public static void p(String s, short i) { 
		System.out.print(s); p(i);
		}
	public static void p(String s, long l) { 
		System.out.print(s); p(l);
		}
	public static void p(String s, float f) { 
		System.out.print(s); p(f);
		}
	public static void p(String s, double d) {
		System.out.print(s); p(d);
		}

	}
