/************************* test/jctest.java ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Eine Klasse zum Testen (fast) aller JavaVM-Instruktionen
	in verschiedenen Konstellationen.

	Die Ausgabe, die dieses Programm generiert, sollte bei allen
	verschiedenen Java-Interpreter/Compiler-Systemen gleich
	sein. Wenn nicht, dann ist wahrscheinlich eines der Systeme
	fehlerhaft.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/12/03

*******************************************************************************/


public class jctest implements jcinterface {
	static int i1=77;
	static long l1=123456789,l2=987654321,l3=-99999999999999L;
	static int i2,i3=-100;
	static float f1=0.1F,f2=0.2F,f3=0.3F;
	static double d1=0.001,d2=0.002,d3=0.003;
	static String s1,s2="der konstante String";

	int n_i1,n_i2,n_i3;
	long n_l1,n_l2,n_l3,n_l4;
	float n_f1,n_f2,n_f3,n_f4;
	int n_i4;
	double n_d1,n_d2,n_d3,n_d4;

	
	public static void main(String[] s) {
		p ("=================== JavaVM - Tester ========================");

		p ("------------------- test arguments");
		int i;
		for (i=0; i<s.length; i++) p(s[i]);

		testgeneral();
		testtables();
		testcasts();
		testspecialnullpointers();
		testarrays();
		
		p ("------------------- test consts");
		testconst ( 1,          1);
		testconst (-1,         -1);
		testconst (-24123,     -4918923241323L);
		testconst (-243511,    -4423423234231423L);
		testconst (0x7fffffff, 0x7fffffffffffffffL);
		testconst (0x80000000, 0x8000000000000000L);

		p ("------------------- test rem consts");
		testremconst(17);
		testremconst(12347);
		testremconst(8893427);
		testremconst(1005234562);
		testremconst(-17);
		testremconst(-12347);
		testremconst(-8893427);
		testremconst(-1005234562);

		testremconst(17L);
		testremconst(12347L);
		testremconst(8893427L);
		testremconst(1005234562L);
		testremconst(12135005234562L);
		testremconst(2343552355623464626L);
		testremconst(-17L);
		testremconst(-12347L);
		testremconst(-8893427L);
		testremconst(-1005234562L);
		testremconst(-12135005234562L);
		testremconst(-2343552355623464626L);

		p ("------------------- test rem 0x10001");
		testrem0x10001(0);
		testrem0x10001(1);
		testrem0x10001(17);
		testrem0x10001(123);
		testrem0x10001(1234);
		testrem0x10001(13247);
		testrem0x10001(563467);
		testrem0x10001(7456774);
		testrem0x10001(34523525);
		testrem0x10001(234523456);
		testrem0x10001(1005234562);
		testrem0x10001(2147483647);
		testrem0x10001(-1);
		testrem0x10001(-17);
		testrem0x10001(-123);
		testrem0x10001(-1234);
		testrem0x10001(-13247);
		testrem0x10001(-563467);
		testrem0x10001(-7456774);
		testrem0x10001(-34523525);
		testrem0x10001(-234523456);
		testrem0x10001(-1005234562);
		testrem0x10001(-2147483647);
		testrem0x10001(-2147483648);

		testrem0x10001(0L);
		testrem0x10001(1L);
		testrem0x10001(17L);
		testrem0x10001(123L);
		testrem0x10001(1234L);
		testrem0x10001(13247L);
		testrem0x10001(563467L);
		testrem0x10001(7456774L);
		testrem0x10001(34523525L);
		testrem0x10001(234523456L);
		testrem0x10001(1005234562L);
		testrem0x10001(1983223864L);
		testrem0x10001(2147483646L);
		testrem0x10001(2147483647L);
		testrem0x10001(2147483648L);
		testrem0x10001(10052234562L);
		testrem0x10001(561005234562L);
		testrem0x10001(2100527345962L);
		testrem0x10001(34600523456287L);
		testrem0x10001(546805234456882L);
		testrem0x10001(1360052334734722L);
		testrem0x10001(23456705569626354L);
		testrem0x10001(9223372036854775807L);
		testrem0x10001(-1L);
		testrem0x10001(-17L);
		testrem0x10001(-123L);
		testrem0x10001(-1234L);
		testrem0x10001(-13247L);
		testrem0x10001(-563467L);
		testrem0x10001(-7456774L);
		testrem0x10001(-34523525L);
		testrem0x10001(-234523456L);
		testrem0x10001(-1005234562L);
		testrem0x10001(-1983223864L);
		testrem0x10001(-2147483647L);
		testrem0x10001(-2147483648L);
		testrem0x10001(-10052234562L);
		testrem0x10001(-561005234562L);
		testrem0x10001(-2100527345962L);
		testrem0x10001(-34600523456287L);
		testrem0x10001(-546805234456882L);
		testrem0x10001(-1360052334734722L);
		testrem0x10001(-23456705569626354L);
		testrem0x10001(-9223372036854775807L);
		testrem0x10001(-9223372036854775808L);

		p ("------------------- test ints");
		testint (1,2);
		testint (-1,17);
		testint (-24351,24123);
		testint (4918923,-441423);
		testint (0,0);
		testint (-1,-1);
		testint (1423487,123444444);
		testint (0x7fffffff,1);
		testint (0,0x7fffffff);
		testint (0x3333,143444);
		testint (4444441,12342);

		p ("------------------- test longs");
		testlong (1,2);
		testlong (-1,17);
		testlong (-24351,24123);
		testlong (4918923241323L,-4423423234231423L);
		testlong (0,0);
		testlong (-1,-1);
		testlong (1423487,123444442344L);
		testlong (0x7fffffffffffffffL,1);
		testlong (0,0x7fffffffffffffffL);
		testlong (0x3333,143444);
		testlong (4444441,12342);

		p ("------------------- test floats");
		testfloat ((float) 1,(float) 2.042);
		testfloat ((float) -1.234,(float) 17.44);
		testfloat ((float) -24351,(float) 24123);
		testfloat ((float) 0.1,(float) 1243);
		testfloat ((float) 0.0,(float) -555.4132);
		testfloat ((float) 77.0,(float) -555);
		testfloat ((float) 2147483000.0,(float) -555234);

		p ("------------------- test doubles");
		testdouble (1,2.042);
		testdouble (-1.234,17.44);
		testdouble (-24351,24123);
		testdouble (0.1,1243);
		testdouble (0.0,-555.4132);
		testdouble (77.0,-555);
		testdouble (2147483000.0,-555234);
		

		p ("=================== end of test =========================");			
		}


	static /* init */ {
		p ("successful initialisation");
		}


		
	public static void testgeneral() {
		
		int i;
		// ******************** basic data types *******************************
		
		p ("------------------- test int-PUSH-STORE-LOAD");
		int j = -1;
		p(j); p (0); p(2); p(17); p(-100); 
		p (500); p(-32768); p(-32769); p(32767); p(32768);
		p (90000); p(-1000000000);
		
		p ("------------------- test long-PUSH-STORE-LOAD");
		long l = 3L;
		p ( l ); p ( 0L ); p ( 99L );
		p (500L); p(-32768L); p(-32769L); p(32767L); p(32768L);
		p ( 6900000000000L ); p ( 349827389478173274L );

		p ("------------------- test float-PUSH-STORE-LOAD");
		float f = 99.444F;
		p ( f ); p (0.0F); p (1.0F); p (342323423478.2223434234232334F);
	
		p ("------------------- test double-PUSH-STORE-LOAD");
		double d = 99234.42D;
		p ( d ); p (0.0D); p (1.0D); p (342323423478.2223434234232334D);


		// ******************** static variables *******************************

		p ("------------------- test static variables");	
		i1 = i1+i2+i3;
		l2 = l1+l2+l3;
		f1 = f1*f2;
		p (i1); p(i2); p(i3); 
		p (l1); p(l2); p(l3);
		p (f1); p(f2); p(f3);
		p (d1); p(d2); p(d3);

		// ******************** arithmetic test ********************************
		
		p ("------------------- test arithmetic");
		i1 = 17;
		i2 = 0x7fffffff;
		p (i2);
		p (i2+1);
		p (i1-i2);
		l1 = 0x7fffffffffffffffL;
		p (l1);
		p (l1+1);
		p (l1+0x7fffffffffffffffL);
		
		
		// ******************** test method calls ******************************
		
		p ("statische methode");
		jctest ttt = new jctest ();
		ttt.p_manyparam (19,18,17,16, 88,77,66,55, 
		                 0.1F,0.2F,0.3F,0.4F, -2.0D,-3.0D,-4.0D,-5.0D );
		jcinterface ttt2 = ttt;
		ttt2.p_nonstatic ("interface method");

		}



	// ************************ test tables ************************************
		
	public static void testtables() {
		int i;

		p ("------------------- test tableswitch");

		for (i = -5; i < 15; i++) {
			switch (i) {
				case  2:  p ("->  2"); break;	
				case  3:  p ("->  3"); break;	
				case  5:  p ("->  5"); break;	
				case  6:  p ("->  6"); break;
				case  7:  p ("->  7"); break;
				case  8:  p ("->  8"); break;
				case 10:  p ("-> 10"); break;
				default:  p ("default"); break;
				}
			}

		p ("------------------- test lookupswitch");

		for (i = -5; i < 15; i++) {
			switch (i) {
				case  2:  p ("->  2"); break;
				case  8:  p ("->  8"); break;
				case 14:  p ("-> 14"); break;
				case -4:  p ("-> -4"); break;
				default:  p ("default"); break;
				}
			}	
		}


	// ****************** test type casts and array stores *********************		

	public static void testcasts() {
		Object    o   = new Object();
		Object    oi  = new Integer(0);
		Object[]  oa  = new Object [1];
		Object[]  oia = new Integer[1];
		Integer   i   = new Integer(0);
		Integer[] ia;

		p ("------------------- test casts");

		try {
			p ("type cast check: Integer = Object(Integer)");
			i = (Integer) oi;
			p ("type cast check: Integer = Object");
			i = (Integer) o;
			p ("error: class cast exception not thrown");
			}	
		catch (ClassCastException c) {
			p ("exception: class cast");
			}

		try {
			p ("type cast check: Integer[] = Object(Integer)[]");
			ia = (Integer[]) oia;
			p ("type cast check: Integer[] = Object[]");
			ia = (Integer[]) oa;
			p ("error: class cast exception not thrown");
			}	
		catch (ClassCastException c) {
			p ("exception: class cast");
			}

		try {
			p ("array store check: Object(Integer)[0] = Integer");
			oia[0] = i;
			p ("array store check: Object(Integer)[0] = Object");
			oia[0] = o;
			p ("error: array store exception not thrown");
			}	
		catch (ArrayStoreException c) {
			p ("exception: array store");
			}
		}


	// ****************** test special null pointers ***************************		

	public static void testspecialnullpointers() {
		int i = 0;
		jctest c = null;
		jcinterface f = null;

		p ("------------------- test special null pointers");

		try {
			p ("null pointer check: put field");
			c.n_i1 = 0;
			p ("error: put field null pointer exception not thrown");
			}	
		catch (NullPointerException x) {
			p ("exception: null pointer");
			}

		try {
			p ("null pointer check: get field");
			i = c.n_i1;
			p ("error: get field null pointer exception not thrown");
			}	
		catch (NullPointerException x) {
			p ("exception: null pointer");
			}

		try {
			p ("null pointer check: invokevirtual");
			c.p_nonstatic("invokevirtual");
			p ("error: invokevirtual null pointer exception not thrown");
			}	
		catch (NullPointerException x) {
			p ("exception: null pointer");
			}

		try {
			p ("null pointer check: invokeinterface");
			f.p_nonstatic("invokeinterface");
			p ("error: invokeinterface null pointer exception not thrown");
			}	
		catch (NullPointerException x) {
			p ("exception: null pointer");
			}

		try {
			p ("null pointer check: monitorenter");
			synchronized (c) {
				p ("error: monitorenter null pointer exception not thrown");
				}
			}	
		catch (NullPointerException x) {
			p ("exception: null pointer");
			}
		}


	// ************************ test array bounds ******************************		

	public static void testarraybounds(byte[] ba, int i) {
		
			p ("testarraybounds: " + (i - 10));
			ba[i-10] = 0;
			p ("testarraybounds: " + (i - 5));
			ba[i-5]  = 0;
			p ("testarraybounds: " + (i));
			ba[i]    = 0;
			p ("testarraybounds: " + (i + 5));
			ba[i+5]  = 0;
			p ("testarraybounds: " + (i + 10));
			ba[i+10] = 0;
		}


	// ************************ test arrays ************************************		

	public static void testarrays() {
		int    i;
		long   l;
		float  f;
		double d;
		String s;
		
		p ("------------------- test byte arrays");

		byte[] ba = null;

		try {
			p ("null pointer check: byte array store");
			ba[1] = 0;
			p ("error: null pointer exception not thrown");
			}	
		catch (NullPointerException c) {
			p ("exception: null pointer");
			}
		try {
			p ("null pointer check: byte array load");
			i = ba[1];
			p ("error: null pointer exception not thrown");
			}	
		catch (NullPointerException c) {
			p ("exception: null pointer");
			}

		try {
			p ("negative array size check: byte array");
			ba = new byte [-2];
			p ("error: negative array size exception not thrown");
			}	
		catch (NegativeArraySizeException c) {
			p ("exception: negative array size");
			}

		ba = new byte [100];


		try {
			p ("array bound check: byte array store");
			ba[-1] = 0;
			p ("error: exception not thrown");
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-1));
			}
		try {
			p ("array bound check: byte array load");
			i = ba[-1];
			p ("error: exception not thrown");
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-1));
			}

		try {
			testarraybounds(ba, 5);
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+5);
			}
		try {
			testarraybounds(ba, 50);
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+50);
			}
		try {
			testarraybounds(ba, 100);
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+100);
			}

		try {
			ba[-4] = 0;
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-4));
			}
		try {
			ba[-3] = 0;
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-3));
			}
		for (i=-2; i<102; i++) { 
			try {
				ba[i] = (byte) (i-50);
				}	
			catch (ArrayIndexOutOfBoundsException c) {
				p ("exception: out of bounds: "+i);
				}
			}
		try {
			ba[102] = 0;
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+102);
			}
		try {
			ba[103] = 0;
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+103);
			}
		for (i=0; i<100; i++) p (ba[i]);		


		p ("-------- test short arrays");		

		short[] sa = null;

		try {
			p ("null pointer check: short array store");
			sa[1] = 0;
			p ("error: null pointer exception not thrown");
			}	
		catch (NullPointerException c) {
			p ("exception: null pointer");
			}
		try {
			p ("null pointer check: short array load");
			i = sa[1];
			p ("error: null pointer exception not thrown");
			}	
		catch (NullPointerException c) {
			p ("exception: null pointer");
			}

		sa = new short [100];

		try {
			p ("array bound check: short array store");
			sa[-1] = 0;
			p ("error: exception not thrown");
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-1));
			}
		try {
			p ("array bound check: short array load");
			i = sa[-1];
			p ("error: exception not thrown");
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-1));
			}

		for (i=0; i<100; i++) sa[i] = (short) (i-50);
		for (i=0; i<100; i++) p (sa[i]);
		

		p ("-------- test int arrays");		

		int[] ia = null;

		try {
			p ("null pointer check: int array store");
			ia[1] = 0;
			p ("error: null pointer exception not thrown");
			}	
		catch (NullPointerException c) {
			p ("exception: null pointer");
			}
		try {
			p ("null pointer check: int array load");
			i = ia[1];
			p ("error: null pointer exception not thrown");
			}	
		catch (NullPointerException c) {
			p ("exception: null pointer");
			}

		ia = new int [50];

		try {
			p ("array bound check: int array store");
			ia[-1] = 0;
			p ("error: exception not thrown");
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-1));
			}
		try {
			p ("array bound check: int array load");
			i = ia[-1];
			p ("error: exception not thrown");
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-1));
			}

		for (i=0; i<10; i++) ia[i] = (123456 + i);
		for (i=0; i<10; i++) p (ia[i]);

		

		p ("-------- test long arrays");		

		long[] la = null;

		try {
			p ("null pointer check: long array store");
			la[1] = 0;
			p ("error: null pointer exception not thrown");
			}	
		catch (NullPointerException c) {
			p ("exception: null pointer");
			}
		try {
			p ("null pointer check: long array load");
			l = la[1];
			p ("error: null pointer exception not thrown");
			}	
		catch (NullPointerException c) {
			p ("exception: null pointer");
			}

		la = new long [10];

		try {
			p ("array bound check: long array store");
			la[-1] = 0;
			p ("error: exception not thrown");
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-1));
			}
		try {
			p ("array bound check: long array load");
			l = la[-1];
			p ("error: exception not thrown");
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-1));
			}

		for (i=0; i<10; i++) la[i] = (1234567890123L + i);
		for (i=0; i<10; i++) p (la[i]);

		
		p ("-------- test char arrays");		

		char[] ca = null;

		try {
			p ("null pointer check: char array store");
			ca[1] = 0;
			p ("error: null pointer exception not thrown");
			}	
		catch (NullPointerException c) {
			p ("exception: null pointer");
			}
		try {
			p ("null pointer check: char array load");
			i = ca[1];
			p ("error: null pointer exception not thrown");
			}	
		catch (NullPointerException c) {
			p ("exception: null pointer");
			}

		ca = new char [50];

		try {
			p ("array bound check: char array store");
			ca[-1] = 0;
			p ("error: exception not thrown");
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-1));
			}
		try {
			p ("array bound check: char array load");
			i = ca[-1];
			p ("error: exception not thrown");
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-1));
			}

		for (i=0; i<50; i++) ca[i] = (char) ('A' + i);
		for (i=0; i<50; i++) p (ca[i]);

		p ("-------- test address arrays");

		String[] sta = null;

		try {
			p ("null pointer check: address array store");
			sta[1] = null;
			p ("error: null pointer exception not thrown");
			}	
		catch (NullPointerException c) {
			p ("exception: null pointer");
			}
		try {
			p ("null pointer check: address array load");
			s = sta[1];
			p ("error: null pointer exception not thrown");
			}	
		catch (NullPointerException c) {
			p ("exception: null pointer");
			}

		try {
			p ("negative array size check: address array");
			sta = new String[-3];
			p ("error: negative array size exception not thrown");
			}	
		catch (NegativeArraySizeException c) {
			p ("exception: negative array size");
			}

		sta = new String[5];

		try {
			p ("array bound check: address array store");
			sta[-1] = null;
			p ("error: exception not thrown");
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-1));
			}
		try {
			p ("array bound check: address array load");
			s = sta[-1];
			p ("error: exception not thrown");
			}	
		catch (ArrayIndexOutOfBoundsException c) {
			p ("exception: out of bounds: "+(-1));
			}

		for (i=0; i<5; i++) sta[i] = Integer.toString(i) + ". Zeile";
		for (i=0; i<5; i++) p (sta[i]);
		
		p ("-------- test multi dimensional arrays");

		int [][][] iaaa = null;

		try {
			p ("negative array size check: multi dimensional array");
			iaaa = new int[2][3][-4];
			p ("error: negative array size exception not thrown");
			}	
		catch (NegativeArraySizeException c) {
			p ("exception: negative array size");
			}

		iaaa = new int[2][3][4];
		long [][][] laaa = new long[2][3][6];
		float [][][] faaa = new float[2][3][4];
		double [][][] daaa = new double[3][4][7];
		for (i=0; i<2; i++) {
			int i2; for (i2=0; i2<3; i2++) {
				int i3; for (i3=0; i3<4; i3++) {
					iaaa[i][i2][i3] = i*i*i + i2*i2 + i3;
					laaa[i][i2][i3] = i*i*i + i2*i2 + i3 + 7;
					faaa[i][i2][i3] = i*i*i + i2*i2 + i3 + 0.4F;
					daaa[i][i2][i3] = i*i*i + i2*i2 + i3 - 47.0001;
					p (iaaa[i][i2][i3]);
					p (laaa[i][i2][i3]);
					p (faaa[i][i2][i3]);
					p (daaa[i][i2][i3]);
					}
				}
			}


					
		}


	static public void testconst(int i, long l) {
		p ("TESTCONST CALLED WITH ", i);
		p ("                  AND ", l);
		
		if (!(i == (0))) p("not IF_ICMPEQ", (0));
		if (!(i != (0))) p("not IF_ICMPNE", (0));
		if (!(i <  (0))) p("not IF_ICMPLT", (0));
		if (!(i <= (0))) p("not IF_ICMPLE", (0));
		if (!(i >  (0))) p("not IF_ICMPGT", (0));
		if (!(i >= (0))) p("not IF_ICMPGE", (0));

		if (!(l == (0))) p("not IF_LCMPEQ", (0));
		if (!(l != (0))) p("not IF_LCMPNE", (0));
		if (!(l <  (0))) p("not IF_LCMPLT", (0));
		if (!(l <= (0))) p("not IF_LCMPLE", (0));
		if (!(l >  (0))) p("not IF_LCMPGT", (0));
		if (!(l >= (0))) p("not IF_LCMPGE", (0));

		p("IADDCONST:  ", i  +  (-1));
		p("ISUBCONST:  ", i  -  (-1));
		p("IMULCONST:  ", i  *  (-1));
		p("ISHLCONST:  ", i <<  (-1));
		p("ISHRCONST:  ", i >>  (-1));
		p("IUSHRCONST: ", i >>> (-1));
		p("IANDCONST:  ", i  &  (-1));
		p("IORCONST:   ", i  |  (-1));
		p("IXORCONST:  ", i  ^  (-1));

		if (!(i == (-1))) p("not IF_ICMPEQ", (-1));
		if (!(i != (-1))) p("not IF_ICMPNE", (-1));
		if (!(i <  (-1))) p("not IF_ICMPLT", (-1));
		if (!(i <= (-1))) p("not IF_ICMPLE", (-1));
		if (!(i >  (-1))) p("not IF_ICMPGT", (-1));
		if (!(i >= (-1))) p("not IF_ICMPGE", (-1));

		p("LADDCONST:  ", l  +  (-1));
		p("LSUBCONST:  ", l  -  (-1));
		p("LMULCONST:  ", l  *  (-1));
		p("LSHLCONST:  ", l <<  (-1));
		p("LSHRCONST:  ", l >>  (-1));
		p("LUSHRCONST: ", l >>> (-1));
		p("LANDCONST:  ", l  &  (-1));
		p("LORCONST:   ", l  |  (-1));
		p("LXORCONST:  ", l  ^  (-1));

		if (!(l == (-1))) p("not IF_LCMPEQ", (-1));
		if (!(l != (-1))) p("not IF_LCMPNE", (-1));
		if (!(l <  (-1))) p("not IF_LCMPLT", (-1));
		if (!(l <= (-1))) p("not IF_LCMPLE", (-1));
		if (!(l >  (-1))) p("not IF_LCMPGT", (-1));
		if (!(l >= (-1))) p("not IF_LCMPGE", (-1));

		p("IADDCONST:  ", i  +  (1));
		p("ISUBCONST:  ", i  -  (1));
		p("IMULCONST:  ", i  *  (1));
		p("ISHLCONST:  ", i <<  (1));
		p("ISHRCONST:  ", i >>  (1));
		p("IUSHRCONST: ", i >>> (1));
		p("IANDCONST:  ", i  &  (1));
		p("IORCONST:   ", i  |  (1));
		p("IXORCONST:  ", i  ^  (1));

		if (!(i == (1))) p("not IF_ICMPEQ", (1));
		if (!(i != (1))) p("not IF_ICMPNE", (1));
		if (!(i <  (1))) p("not IF_ICMPLT", (1));
		if (!(i <= (1))) p("not IF_ICMPLE", (1));
		if (!(i >  (1))) p("not IF_ICMPGT", (1));
		if (!(i >= (1))) p("not IF_ICMPGE", (1));

		p("LADDCONST:  ", l  +  (1));
		p("LSUBCONST:  ", l  -  (1));
		p("LMULCONST:  ", l  *  (1));
		p("LSHLCONST:  ", l <<  (1));
		p("LSHRCONST:  ", l >>  (1));
		p("LUSHRCONST: ", l >>> (1));
		p("LANDCONST:  ", l  &  (1));
		p("LORCONST:   ", l  |  (1));
		p("LXORCONST:  ", l  ^  (1));

		if (!(l == (1))) p("not IF_LCMPEQ", (1));
		if (!(l != (1))) p("not IF_LCMPNE", (1));
		if (!(l <  (1))) p("not IF_LCMPLT", (1));
		if (!(l <= (1))) p("not IF_LCMPLE", (1));
		if (!(l >  (1))) p("not IF_LCMPGT", (1));
		if (!(l >= (1))) p("not IF_LCMPGE", (1));

		p("IADDCONST:  ", i  +  (255));
		p("ISUBCONST:  ", i  -  (255));
		p("IMULCONST:  ", i  *  (255));
		p("ISHLCONST:  ", i <<  (255));
		p("ISHRCONST:  ", i >>  (255));
		p("IUSHRCONST: ", i >>> (255));
		p("IANDCONST:  ", i  &  (255));
		p("IORCONST:   ", i  |  (255));
		p("IXORCONST:  ", i  ^  (255));

		if (!(i == (255))) p("not IF_ICMPEQ", (255));
		if (!(i != (255))) p("not IF_ICMPNE", (255));
		if (!(i <  (255))) p("not IF_ICMPLT", (255));
		if (!(i <= (255))) p("not IF_ICMPLE", (255));
		if (!(i >  (255))) p("not IF_ICMPGT", (255));
		if (!(i >= (255))) p("not IF_ICMPGE", (255));

		p("LADDCONST:  ", l  +  (255));
		p("LSUBCONST:  ", l  -  (255));
		p("LMULCONST:  ", l  *  (255));
		p("LSHLCONST:  ", l <<  (255));
		p("LSHRCONST:  ", l >>  (255));
		p("LUSHRCONST: ", l >>> (255));
		p("LANDCONST:  ", l  &  (255));
		p("LORCONST:   ", l  |  (255));
		p("LXORCONST:  ", l  ^  (255));

		if (!(l == (255))) p("not IF_LCMPEQ", (255));
		if (!(l != (255))) p("not IF_LCMPNE", (255));
		if (!(l <  (255))) p("not IF_LCMPLT", (255));
		if (!(l <= (255))) p("not IF_LCMPLE", (255));
		if (!(l >  (255))) p("not IF_LCMPGT", (255));
		if (!(l >= (255))) p("not IF_LCMPGE", (255));

		p("IADDCONST:  ", i  +  (256));
		p("ISUBCONST:  ", i  -  (256));
		p("IMULCONST:  ", i  *  (256));
		p("ISHLCONST:  ", i <<  (256));
		p("ISHRCONST:  ", i >>  (256));
		p("IUSHRCONST: ", i >>> (256));
		p("IANDCONST:  ", i  &  (256));
		p("IORCONST:   ", i  |  (256));
		p("IXORCONST:  ", i  ^  (256));

		if (!(i == (256))) p("not IF_ICMPEQ", (256));
		if (!(i != (256))) p("not IF_ICMPNE", (256));
		if (!(i <  (256))) p("not IF_ICMPLT", (256));
		if (!(i <= (256))) p("not IF_ICMPLE", (256));
		if (!(i >  (256))) p("not IF_ICMPGT", (256));
		if (!(i >= (256))) p("not IF_ICMPGE", (256));

		p("LADDCONST:  ", l  +  (256));
		p("LSUBCONST:  ", l  -  (256));
		p("LMULCONST:  ", l  *  (256));
		p("LSHLCONST:  ", l <<  (256));
		p("LSHRCONST:  ", l >>  (256));
		p("LUSHRCONST: ", l >>> (256));
		p("LANDCONST:  ", l  &  (256));
		p("LORCONST:   ", l  |  (256));
		p("LXORCONST:  ", l  ^  (256));

		if (!(l == (256))) p("not IF_LCMPEQ", (256));
		if (!(l != (256))) p("not IF_LCMPNE", (256));
		if (!(l <  (256))) p("not IF_LCMPLT", (256));
		if (!(l <= (256))) p("not IF_LCMPLE", (256));
		if (!(l >  (256))) p("not IF_LCMPGT", (256));
		if (!(l >= (256))) p("not IF_LCMPGE", (256));

		p("IADDCONST:  ", i  +  (32767));
		p("ISUBCONST:  ", i  -  (32767));
		p("IMULCONST:  ", i  *  (32767));
		p("ISHLCONST:  ", i <<  (32767));
		p("ISHRCONST:  ", i >>  (32767));
		p("IUSHRCONST: ", i >>> (32767));
		p("IANDCONST:  ", i  &  (32767));
		p("IORCONST:   ", i  |  (32767));
		p("IXORCONST:  ", i  ^  (32767));

		if (!(i == (32767))) p("not IF_ICMPEQ", (32767));
		if (!(i != (32767))) p("not IF_ICMPNE", (32767));
		if (!(i <  (32767))) p("not IF_ICMPLT", (32767));
		if (!(i <= (32767))) p("not IF_ICMPLE", (32767));
		if (!(i >  (32767))) p("not IF_ICMPGT", (32767));
		if (!(i >= (32767))) p("not IF_ICMPGE", (32767));

		p("LADDCONST:  ", l  +  (32767));
		p("LSUBCONST:  ", l  -  (32767));
		p("LMULCONST:  ", l  *  (32767));
		p("LSHLCONST:  ", l <<  (32767));
		p("LSHRCONST:  ", l >>  (32767));
		p("LUSHRCONST: ", l >>> (32767));
		p("LANDCONST:  ", l  &  (32767));
		p("LORCONST:   ", l  |  (32767));
		p("LXORCONST:  ", l  ^  (32767));

		if (!(l == (32767))) p("not IF_LCMPEQ", (32767));
		if (!(l != (32767))) p("not IF_LCMPNE", (32767));
		if (!(l <  (32767))) p("not IF_LCMPLT", (32767));
		if (!(l <= (32767))) p("not IF_LCMPLE", (32767));
		if (!(l >  (32767))) p("not IF_LCMPGT", (32767));
		if (!(l >= (32767))) p("not IF_LCMPGE", (32767));

		p("IADDCONST:  ", i  +  (32768));
		p("ISUBCONST:  ", i  -  (32768));
		p("IMULCONST:  ", i  *  (32768));
		p("ISHLCONST:  ", i <<  (32768));
		p("ISHRCONST:  ", i >>  (32768));
		p("IUSHRCONST: ", i >>> (32768));
		p("IANDCONST:  ", i  &  (32768));
		p("IORCONST:   ", i  |  (32768));
		p("IXORCONST:  ", i  ^  (32768));

		if (!(i == (32768))) p("not IF_ICMPEQ", (32768));
		if (!(i != (32768))) p("not IF_ICMPNE", (32768));
		if (!(i <  (32768))) p("not IF_ICMPLT", (32768));
		if (!(i <= (32768))) p("not IF_ICMPLE", (32768));
		if (!(i >  (32768))) p("not IF_ICMPGT", (32768));
		if (!(i >= (32768))) p("not IF_ICMPGE", (32768));

		p("LADDCONST:  ", l  +  (32768));
		p("LSUBCONST:  ", l  -  (32768));
		p("LMULCONST:  ", l  *  (32768));
		p("LSHLCONST:  ", l <<  (32768));
		p("LSHRCONST:  ", l >>  (32768));
		p("LUSHRCONST: ", l >>> (32768));
		p("LANDCONST:  ", l  &  (32768));
		p("LORCONST:   ", l  |  (32768));
		p("LXORCONST:  ", l  ^  (32768));

		if (!(l == (32768))) p("not IF_LCMPEQ", (32768));
		if (!(l != (32768))) p("not IF_LCMPNE", (32768));
		if (!(l <  (32768))) p("not IF_LCMPLT", (32768));
		if (!(l <= (32768))) p("not IF_LCMPLE", (32768));
		if (!(l >  (32768))) p("not IF_LCMPGT", (32768));
		if (!(l >= (32768))) p("not IF_LCMPGE", (32768));

		p("IADDCONST:  ", i  +  (-32768));
		p("ISUBCONST:  ", i  -  (-32768));
		p("IMULCONST:  ", i  *  (-32768));
		p("ISHLCONST:  ", i <<  (-32768));
		p("ISHRCONST:  ", i >>  (-32768));
		p("IUSHRCONST: ", i >>> (-32768));
		p("IANDCONST:  ", i  &  (-32768));
		p("IORCONST:   ", i  |  (-32768));
		p("IXORCONST:  ", i  ^  (-32768));

		if (!(i == (-32768))) p("not IF_ICMPEQ", (-32768));
		if (!(i != (-32768))) p("not IF_ICMPNE", (-32768));
		if (!(i <  (-32768))) p("not IF_ICMPLT", (-32768));
		if (!(i <= (-32768))) p("not IF_ICMPLE", (-32768));
		if (!(i >  (-32768))) p("not IF_ICMPGT", (-32768));
		if (!(i >= (-32768))) p("not IF_ICMPGE", (-32768));

		p("LADDCONST:  ", l  +  (-32768));
		p("LSUBCONST:  ", l  -  (-32768));
		p("LMULCONST:  ", l  *  (-32768));
		p("LSHLCONST:  ", l <<  (-32768));
		p("LSHRCONST:  ", l >>  (-32768));
		p("LUSHRCONST: ", l >>> (-32768));
		p("LANDCONST:  ", l  &  (-32768));
		p("LORCONST:   ", l  |  (-32768));
		p("LXORCONST:  ", l  ^  (-32768));

		if (!(l == (-32768))) p("not IF_LCMPEQ", (-32768));
		if (!(l != (-32768))) p("not IF_LCMPNE", (-32768));
		if (!(l <  (-32768))) p("not IF_LCMPLT", (-32768));
		if (!(l <= (-32768))) p("not IF_LCMPLE", (-32768));
		if (!(l >  (-32768))) p("not IF_LCMPGT", (-32768));
		if (!(l >= (-32768))) p("not IF_LCMPGE", (-32768));

		p("IADDCONST:  ", i  +  (-32769));
		p("ISUBCONST:  ", i  -  (-32769));
		p("IMULCONST:  ", i  *  (-32769));
		p("ISHLCONST:  ", i <<  (-32769));
		p("ISHRCONST:  ", i >>  (-32769));
		p("IUSHRCONST: ", i >>> (-32769));
		p("IANDCONST:  ", i  &  (-32769));
		p("IORCONST:   ", i  |  (-32769));
		p("IXORCONST:  ", i  ^  (-32769));

		if (!(i == (-32769))) p("not IF_ICMPEQ", (-32769));
		if (!(i != (-32769))) p("not IF_ICMPNE", (-32769));
		if (!(i <  (-32769))) p("not IF_ICMPLT", (-32769));
		if (!(i <= (-32769))) p("not IF_ICMPLE", (-32769));
		if (!(i >  (-32769))) p("not IF_ICMPGT", (-32769));
		if (!(i >= (-32769))) p("not IF_ICMPGE", (-32769));

		p("LADDCONST:  ", l  +  (-32769));
		p("LSUBCONST:  ", l  -  (-32769));
		p("LMULCONST:  ", l  *  (-32769));
		p("LSHLCONST:  ", l <<  (-32769));
		p("LSHRCONST:  ", l >>  (-32769));
		p("LUSHRCONST: ", l >>> (-32769));
		p("LANDCONST:  ", l  &  (-32769));
		p("LORCONST:   ", l  |  (-32769));
		p("LXORCONST:  ", l  ^  (-32769));

		if (!(l == (-32769))) p("not IF_LCMPEQ", (-32769));
		if (!(l != (-32769))) p("not IF_LCMPNE", (-32769));
		if (!(l <  (-32769))) p("not IF_LCMPLT", (-32769));
		if (!(l <= (-32769))) p("not IF_LCMPLE", (-32769));
		if (!(l >  (-32769))) p("not IF_LCMPGT", (-32769));
		if (!(l >= (-32769))) p("not IF_LCMPGE", (-32769));

	}

	static public void testrem0x10001(int a) {
		p("IREM0X10001 (" + a + " % 0x10001):  ", a % 0x10001);
		}

	static public void testrem0x10001(long a) {
		p("LREM0X10001 (" + a + " % 0x10001):  ", a % 0x10001L);
		}

	static public void testremconst(int a) {
		p("IREMPOW2 (" + a + " % 0x00000002):  ", a % 0x00000002);
		p("IREMPOW2 (" + a + " % 0x00000004):  ", a % 0x00000004);
		p("IREMPOW2 (" + a + " % 0x00000008):  ", a % 0x00000008);
		p("IREMPOW2 (" + a + " % 0x00000010):  ", a % 0x00000010);
		p("IREMPOW2 (" + a + " % 0x00000020):  ", a % 0x00000020);
		p("IREMPOW2 (" + a + " % 0x00000040):  ", a % 0x00000040);
		p("IREMPOW2 (" + a + " % 0x00000080):  ", a % 0x00000080);
		p("IREMPOW2 (" + a + " % 0x00000100):  ", a % 0x00000100);
		p("IREMPOW2 (" + a + " % 0x00000200):  ", a % 0x00000200);
		p("IREMPOW2 (" + a + " % 0x00000400):  ", a % 0x00000400);
		p("IREMPOW2 (" + a + " % 0x00000800):  ", a % 0x00000800);
		p("IREMPOW2 (" + a + " % 0x00001000):  ", a % 0x00001000);
		p("IREMPOW2 (" + a + " % 0x00002000):  ", a % 0x00002000);
		p("IREMPOW2 (" + a + " % 0x00004000):  ", a % 0x00004000);
		p("IREMPOW2 (" + a + " % 0x00008000):  ", a % 0x00008000);
		p("IREMPOW2 (" + a + " % 0x00010000):  ", a % 0x00010000);
		p("IREMPOW2 (" + a + " % 0x00020000):  ", a % 0x00020000);
		p("IREMPOW2 (" + a + " % 0x00040000):  ", a % 0x00040000);
		p("IREMPOW2 (" + a + " % 0x00080000):  ", a % 0x00080000);
		p("IREMPOW2 (" + a + " % 0x00100000):  ", a % 0x00100000);
		p("IREMPOW2 (" + a + " % 0x00200000):  ", a % 0x00200000);
		p("IREMPOW2 (" + a + " % 0x00400000):  ", a % 0x00400000);
		p("IREMPOW2 (" + a + " % 0x00800000):  ", a % 0x00800000);
		p("IREMPOW2 (" + a + " % 0x01000000):  ", a % 0x01000000);
		p("IREMPOW2 (" + a + " % 0x02000000):  ", a % 0x02000000);
		p("IREMPOW2 (" + a + " % 0x04000000):  ", a % 0x04000000);
		p("IREMPOW2 (" + a + " % 0x08000000):  ", a % 0x08000000);
		p("IREMPOW2 (" + a + " % 0x10000000):  ", a % 0x10000000);
		p("IREMPOW2 (" + a + " % 0x20000000):  ", a % 0x20000000);
		p("IREMPOW2 (" + a + " % 0x40000000):  ", a % 0x40000000);
		p("IREMPOW2 (" + a + " % 0x80000000):  ", a % 0x80000000);
		}

	static public void testremconst(long a) {
		p("LREMPOW2 (" + a + " % 0x00000002):  ", a % 0x00000002L);
		p("LREMPOW2 (" + a + " % 0x00000004):  ", a % 0x00000004L);
		p("LREMPOW2 (" + a + " % 0x00000008):  ", a % 0x00000008L);
		p("LREMPOW2 (" + a + " % 0x00000010):  ", a % 0x00000010L);
		p("LREMPOW2 (" + a + " % 0x00000020):  ", a % 0x00000020L);
		p("LREMPOW2 (" + a + " % 0x00000040):  ", a % 0x00000040L);
		p("LREMPOW2 (" + a + " % 0x00000080):  ", a % 0x00000080L);
		p("LREMPOW2 (" + a + " % 0x00000100):  ", a % 0x00000100L);
		p("LREMPOW2 (" + a + " % 0x00000200):  ", a % 0x00000200L);
		p("LREMPOW2 (" + a + " % 0x00000400):  ", a % 0x00000400L);
		p("LREMPOW2 (" + a + " % 0x00000800):  ", a % 0x00000800L);
		p("LREMPOW2 (" + a + " % 0x00001000):  ", a % 0x00001000L);
		p("LREMPOW2 (" + a + " % 0x00002000):  ", a % 0x00002000L);
		p("LREMPOW2 (" + a + " % 0x00004000):  ", a % 0x00004000L);
		p("LREMPOW2 (" + a + " % 0x00008000):  ", a % 0x00008000L);
		p("LREMPOW2 (" + a + " % 0x00010000):  ", a % 0x00010000L);
		p("LREMPOW2 (" + a + " % 0x00020000):  ", a % 0x00020000L);
		p("LREMPOW2 (" + a + " % 0x00040000):  ", a % 0x00040000L);
		p("LREMPOW2 (" + a + " % 0x00080000):  ", a % 0x00080000L);
		p("LREMPOW2 (" + a + " % 0x00100000):  ", a % 0x00100000L);
		p("LREMPOW2 (" + a + " % 0x00200000):  ", a % 0x00200000L);
		p("LREMPOW2 (" + a + " % 0x00400000):  ", a % 0x00400000L);
		p("LREMPOW2 (" + a + " % 0x00800000):  ", a % 0x00800000L);
		p("LREMPOW2 (" + a + " % 0x01000000):  ", a % 0x01000000L);
		p("LREMPOW2 (" + a + " % 0x02000000):  ", a % 0x02000000L);
		p("LREMPOW2 (" + a + " % 0x04000000):  ", a % 0x04000000L);
		p("LREMPOW2 (" + a + " % 0x08000000):  ", a % 0x08000000L);
		p("LREMPOW2 (" + a + " % 0x10000000):  ", a % 0x10000000L);
		p("LREMPOW2 (" + a + " % 0x20000000):  ", a % 0x20000000L);
		p("LREMPOW2 (" + a + " % 0x40000000):  ", a % 0x40000000L);
		p("LREMPOW2 (" + a + " % 0x80000000):  ", a % 0x80000000L);
		}


	static public void testint(int a, int b) {
		p ("TESTINT CALLED WITH ", a);
		p ("                AND ", b);
		
		p("IADD:  ", a+b);
		p("ISUB:  ", a-b);
		p("IMUL:  ", a*b);
		try { p("IDIV:  ", a/b); } 
		catch (ArithmeticException e) { p("divison by zero"); }
		try { p("IREM:  ", a%b); } 
		catch (ArithmeticException e) { p("divison by zero"); }
		p("INEG:  ", -a);
		p("ISHL:  ", a<<b);
		p("ISHR:  ", a>>b);
		p("IUSHR: ", a>>>b);
		p("IAND:  ", a & b);
		p("IOR:   ", a | b);
		p("IXOR:  ", a ^ b);

		p("I2L:   ", (long) a);
		p("I2F:   ", (float) a);
		p("I2D:   ", (double) a);
		p("INT2BYTE: ", (byte) a);	
		p("INT2CHAR: ", (char) a);	
		p("INT2SHORT: ", (short) a);	

		if (!(a==0)) p("not IFEQ");
		if (!(a!=0)) p("not IFNE");
		if (!(a<0))  p("not IFLT");
		if (!(a<=0)) p("not IFLE");
		if (!(a>0))  p("not IFGT");
		if (!(a>=0)) p("not IFGE");

		if (!(a==b)) p("not IF_ICMPEQ");
		if (!(a!=b)) p("not IF_ICMPNE");
		if (!(a<b))  p("not IF_ICMPLT");
		if (!(a<=b)) p("not IF_ICMPLE");
		if (!(a>b))  p("not IF_ICMPGT");
		if (!(a>=b)) p("not IF_ICMPGE");
		
	}

	static public void testlong(long a, long b) {
		p ("TESTLONG called with ", a);
		p ("                 AND ", b);
		
		p("LADD:  ", a + b);
		p("LSUB:  ", a - b);
		p("LMUL:  ", a * b);
		try { p("LDIV:  ", a / b); } 
		catch (ArithmeticException e) { p("divison by zero"); }
		try { p("LREM:  ", a % b); } 
		catch (ArithmeticException e) { p("divison by zero"); }
		p("LNEG:  ", -a);
		p("LSHL:  ", a << b);
		p("LSHR:  ", a >> b);
		p("LUSHR: ", a >>>b);
		p("LAND:  ", a &  b);
		p("LOR:   ", a |  b);
		p("LXOR:  ", a ^  b);

		p("L2I:   ", (int) a);
		p("L2F:   ", (float) a);
		p("L2D:   ", (double) a);

		p("LCMP a == b : ", a == b);
		p("LCMP a != b : ", a != b);
		p("LCMP a <  b : ", a <  b);
		p("LCMP a <= b : ", a <= b);
		p("LCMP a >  b : ", a >  b);
		p("LCMP a >= b : ", a >= b);

		if ((a==b)) p("not IF_LCMPEQ");
		if ((a!=b)) p("not IF_LCMPNE");
		if ((a<b))  p("not IF_LCMPLT");
		if ((a<=b)) p("not IF_LCMPLE");
		if ((a>b))  p("not IF_LCMPGT");
		if ((a>=b)) p("not IF_LCMPGE");
	}

	static public void testfloat(float a, float b) {
		p ("TESTFLOAT called with ", a);
		p ("                  AND ", b);
		
		p("FADD:  ", a+b);
		p("FSUB:  ", a-b);
		p("FMUL:  ", a*b);
		p("FDIV:  ", a/b); 
		p("FREM:  ", a%b);
		
		p("F2I:   ", (int) a);
		p("F2L:   ", (long) a);
		p("F2D:   ", (double) a);

		if ((a==b)) p("FCMP a=b");
		if ((a!=b)) p("FCMP a!=b");
		if ((a<b))  p("FCMP a<b");
		if ((a<=b)) p("FCMP a<=b");
		if ((a>b))  p("FCMP a>b");
		if ((a>=b)) p("FCMP a>=b");
	}

	static public void testdouble(double a, double b) {
		p ("TESTDOUBLE called with ", a);
		p ("                   AND ", b);
		
		p("DADD:  ", a+b);
		p("DSUB:  ", a-b);
		p("DMUL:  ", a*b);
		p("DDIV:  ", a/b); 
		p("DREM:  ", a%b);
		
		p("D2I:   ", (int) a);
		p("D2L:   ", (long) a);
		p("D2F:   ", (float) a);

		if ((a==b)) p("DCMP a=b");
		if ((a!=b)) p("DCMP a!=b");
		if ((a<b))  p("DCMP a<b");
		if ((a<=b)) p("DCMP a<=b");
		if ((a>b))  p("DCMP a>b");
		if ((a>=b)) p("DCMP a>=b");
	}


	// ********************* output methods ****************************
	
	public static int linenum = 0;
	public static void pnl() {
		System.out.println ();
		System.out.print (linenum);
		System.out.print (".    ");
		linenum++;
		}

	public static void p(String a) { System.out.print(a); pnl(); }
	public static void p(boolean a) {System.out.print(a); 
	                              pnl();  }
	public static void p(int a) { System.out.print ("int: ");
	                              System.out.print(a); 
	                              pnl();  }
	public static void p(long a) { System.out.print ("long: ");
	                               System.out.print(a); 
	                               pnl(); }
	public static void p(short a) { System.out.print ("short: ");
	                               System.out.print(a); 
	                               pnl(); }
	public static void p(byte a) { System.out.print ("byte: ");
	                               System.out.print(a); 
	                               pnl(); }
	public static void p(char a) { System.out.print ("char: ");
	                               System.out.print((int)a); 
	                               pnl(); }
	public static void p(float a) { System.out.print ("float: ");
	                                System.out.print ( java.lang.Float.floatToIntBits(a) ); 
	                                pnl(); }
	public static void p(double a) { System.out.print ("double: ");
	                                 System.out.print( java.lang.Double.doubleToLongBits(a) ); 
	                                 pnl(); }

	public static void p(String s,boolean i) { 
		System.out.print(s); p(i);
		}
	public static void p(String s,int i) { 
		System.out.print(s); p(i);
		}
	public static void p(String s,byte i) { 
		System.out.print(s); p(i);
		}
	public static void p(String s,char i) { 
		System.out.print(s); p(i);
		}
	public static void p(String s,short i) { 
		System.out.print(s); p(i);
		}
	public static void p(String s,long l) { 
		System.out.print(s); p(l);
		}
	public static void p(String s,float f) { 
		System.out.print(s); p(f);
		}
	public static void p(String s,double d) {
		System.out.print(s); p(d);
		}




	// methods for testing interface and method calls

	public void jctest() { p (" <init> wird aktiviert"); };
	public void p_manyparam (int p_i1,int p_i2,
	                         int p_i3, int p_i4, 
	                         long p_l1,long p_l2,
	                         long p_l3,long p_l4, 
	                         float p_f1, float p_f2, 
	                         float p_f3, float p_f4,
	                         double p_d1, double p_d2,
	                         double p_d3, double p_d4) {
		n_i1 = p_i1;
		n_i2 = p_i2;
		n_i3 = p_i3;
		n_i4 = p_i4;
		n_l1 = p_l1;
		n_l2 = p_l2;
		n_l3 = p_l3;
		n_l4 = p_l4;
		n_f1 = p_f1;
		n_f2 = p_f2;
		n_f3 = p_f3;
		n_f4 = p_f4;
		n_d1 = p_d1;
		n_d2 = p_d2;
		n_d3 = p_d3;
		n_d4 = p_d4;
		}
		
	public void p_nonstatic (String a) { 
		p(a); 
		p(n_i1);
		p(n_i2);
		p(n_i3);
		p(n_i4);
		p(n_l1);
		p(n_l2);
		p(n_l3);
		p(n_l4);
		p(n_f1);
		p(n_f2);
		p(n_f3);
		p(n_f4);
		p(n_d1);
		p(n_d2);
		p(n_d3);
		p(n_d4);
		
		}

	}
	
	
interface jcinterface {	
	public void p_nonstatic (String a);
	}
	
	
