/* tests/native/testarguments.java - tests argument passing

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Christian Thalinger

   $Id: testarguments.java 1756 2004-12-13 10:09:35Z twisti $

*/


import java.util.*;

public class testarguments {
    static Random r;

    public static native void nisub(int a, int b, int c, int d, int e,
                                    int f, int g, int h, int i, int j);
    public static native void nlsub(long a, long b, long c, long d, long e,
                                    long f, long g, long h, long i, long j);
    public static native void nfsub(float a, float b, float c, float d, float e,
                                    float f, float g, float h, float i, float j);
    public static native void ndsub(double a, double b, double c, double d, double e,
                                    double f, double g, double h, double i, double j);

    public static void main(String[] argv) {
        r = new Random(0);

        System.loadLibrary("testarguments");

        pln("testing int --------------------------------------------------");
        isub(i(), i(), i(), i(), i(), i(), i(), i(), i(), i());
        pln();

        pln("testing long -------------------------------------------------");
        lsub(l(), l(), l(), l(), l(), l(), l(), l(), l(), l());
        pln();

        pln("testing float ------------------------------------------------");
        fsub(f(), f(), f(), f(), f(), f(), f(), f(), f(), f());
        pln();

        pln("testing double -----------------------------------------------");
        dsub(d(), d(), d(), d(), d(), d(), d(), d(), d(), d());

//          try {
//              ndsub(d(), d(), d(), d(), d(), d(), d(), d(), d(), d());
//          } catch (LinkageError e) {
//              System.out.println("catched");
//          }
    }


    // test java-java argument passing

    public static void isub(int a, int b, int c, int d, int e,
                            int f, int g, int h, int i, int j) {
        p("java-java  :");
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
        pln();

        nisub(a, b, c, d, e, f, g, h, i, j);
    }

    public static void lsub(long a, long b, long c, long d, long e,
                            long f, long g, long h, long i, long j) {
        p("java-java  :");
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
        pln();

        nlsub(a, b, c, d, e, f, g, h, i, j);
    }

    public static void fsub(float a, float b, float c, float d, float e,
                            float f, float g, float h, float i, float j) {
        p("java-java  :");
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
        pln();

        nfsub(a, b, c, d, e, f, g, h, i, j);
    }

    public static void dsub(double a, double b, double c, double d, double e,
                            double f, double g, double h, double i, double j) {
        p("java-java  :");
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
        pln();

        ndsub(a, b, c, d, e, f, g, h, i, j);
    }


    // test native-java argument passing

    public static void jisub(int a, int b, int c, int d, int e,
                             int f, int g, int h, int i, int j) {
        p("native-java:");
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
        pln();
    }

    public static void jlsub(long a, long b, long c, long d, long e,
                             long f, long g, long h, long i, long j) {
        p("native-java:");
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
        pln();
    }

    public static void jfsub(float a, float b, float c, float d, float e,
                             float f, float g, float h, float i, float j) {
        p("native-java:");
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
        pln();
    }

    public static void jdsub(double a, double b, double c, double d, double e,
                             double f, double g, double h, double i, double j) {
        p("native-java:");
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
        pln();
    }


    static int i() { return r.nextInt(); }
    static long l() { return r.nextLong(); }
    static float f() { return r.nextFloat(); }
    static double d() { return r.nextDouble(); }

    static void p(String s) { System.out.print(s); }

    static void p(int i) { System.out.print(" " + i); }
    static void p(long l) { System.out.print(" " + l); }
    static void p(float f) { System.out.print(" " + f); }
    static void p(double d) { System.out.print(" " + d); }

    static void pln() { System.out.println(); }
    static void pln(String s) { System.out.println(s); }
}
