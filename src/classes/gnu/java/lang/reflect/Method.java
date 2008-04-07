/* java.lang.reflect.Method - reflection of Java methods
   Copyright (C) 1998, 2001, 2002, 2005, 2007, 2008 Free Software Foundation, Inc.

This file is part of GNU Classpath.

GNU Classpath is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.
 
GNU Classpath is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Classpath; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301 USA.

Linking this library statically or dynamically with other modules is
making a combined work based on this library.  Thus, the terms and
conditions of the GNU General Public License cover the whole
combination.

As a special exception, the copyright holders of this library give you
permission to link this library with independent modules to produce an
executable, regardless of the license terms of these independent
modules, and to copy and distribute the resulting executable under
terms of your choice, provided that you also meet, for each linked
independent module, the terms and conditions of the license of that
module.  An independent module is a module which is not derived from
or based on this library.  If you modify this library, you may extend
this exception to your version of the library, but you are not
obligated to do so.  If you do not wish to do so, delete this
exception statement from your version. */


package java.lang.reflect;

import gnu.java.lang.ClassHelper;
import gnu.java.lang.CPStringBuilder;

import gnu.java.lang.reflect.MethodSignatureParser;

import java.lang.annotation.Annotation;

/**
 * The Method class represents a member method of a class. It also allows
 * dynamic invocation, via reflection. This works for both static and
 * instance methods. Invocation on Method objects knows how to do
 * widening conversions, but throws {@link IllegalArgumentException} if
 * a narrowing conversion would be necessary. You can query for information
 * on this Method regardless of location, but invocation access may be limited
 * by Java language access controls. If you can't do it in the compiler, you
 * can't normally do it here either.<p>
 *
 * <B>Note:</B> This class returns and accepts types as Classes, even
 * primitive types; there are Class types defined that represent each
 * different primitive type.  They are <code>java.lang.Boolean.TYPE,
 * java.lang.Byte.TYPE,</code>, also available as <code>boolean.class,
 * byte.class</code>, etc.  These are not to be confused with the
 * classes <code>java.lang.Boolean, java.lang.Byte</code>, etc., which are
 * real classes.<p>
 *
 * Also note that this is not a serializable class.  It is entirely feasible
 * to make it serializable using the Externalizable interface, but this is
 * on Sun, not me.
 *
 * @author John Keiser
 * @author Eric Blake <ebb9@email.byu.edu>
 * @see Member
 * @see Class
 * @see java.lang.Class#getMethod(String,Class[])
 * @see java.lang.Class#getDeclaredMethod(String,Class[])
 * @see java.lang.Class#getMethods()
 * @see java.lang.Class#getDeclaredMethods()
 * @since 1.1
 * @status updated to 1.4
 */
public final class Method
extends AccessibleObject implements Member, GenericDeclaration
{
  private static final int METHOD_MODIFIERS
    = Modifier.ABSTRACT | Modifier.FINAL | Modifier.NATIVE
      | Modifier.PRIVATE | Modifier.PROTECTED | Modifier.PUBLIC
      | Modifier.STATIC | Modifier.STRICT | Modifier.SYNCHRONIZED;

  private MethodSignatureParser p;

  VMMethod m;

  /**
   * This class is uninstantiable outside this package.
   */
  Method(VMMethod m)
  {
    this.m = m;
    m.m = this;
  }

  /**
   * Gets the class that declared this method, or the class where this method
   * is a non-inherited member.
   * @return the class that declared this member
   */
  @SuppressWarnings("unchecked")
  public Class<?> getDeclaringClass()
  {
    return (Class<?>) m.getDeclaringClass();
  }

  /**
   * Gets the name of this method.
   * @return the name of this method
   */
  public String getName()
  {
    return m.getName();
  }

  /**
   * Gets the modifiers this method uses.  Use the <code>Modifier</code>
   * class to interpret the values.  A method can only have a subset of the
   * following modifiers: public, private, protected, abstract, static,
   * final, synchronized, native, and strictfp.
   *
   * @return an integer representing the modifiers to this Member
   * @see Modifier
   */
  public int getModifiers()
  {
    return m.getModifiersInternal() & METHOD_MODIFIERS;
  }

  /**
   * Return true if this method is a bridge method.  A bridge method
   * is generated by the compiler in some situations involving
   * generics and inheritance.
   * @since 1.5
   */
  public boolean isBridge()
  {
    return (m.getModifiersInternal() & Modifier.BRIDGE) != 0;
  }

  /**
   * Return true if this method is synthetic, false otherwise.
   * @since 1.5
   */
  public boolean isSynthetic()
  {
    return (m.getModifiersInternal() & Modifier.SYNTHETIC) != 0;
  }

  /**
   * Return true if this is a varargs method, that is if
   * the method takes a variable number of arguments.
   * @since 1.5
   */
  public boolean isVarArgs()
  {
    return (m.getModifiersInternal() & Modifier.VARARGS) != 0;
  }

  /**
   * Gets the return type of this method.
   * @return the type of this method
   */
  @SuppressWarnings("unchecked")
  public Class<?> getReturnType()
  {
    return (Class<?>) m.getReturnType();
  }

  /**
   * Get the parameter list for this method, in declaration order. If the
   * method takes no parameters, returns a 0-length array (not null).
   *
   * @return a list of the types of the method's parameters
   */
  @SuppressWarnings("unchecked")
  public Class<?>[] getParameterTypes()
  {
    return (Class<?>[]) m.getParameterTypes();
  }

  /**
   * Get the exception types this method says it throws, in no particular
   * order. If the method has no throws clause, returns a 0-length array
   * (not null).
   *
   * @return a list of the types in the method's throws clause
   */
  @SuppressWarnings("unchecked")
  public Class<?>[] getExceptionTypes()
  {
    return (Class<?>[]) m.getExceptionTypes();
  }

  /**
   * Compare two objects to see if they are semantically equivalent.
   * Two Methods are semantically equivalent if they have the same declaring
   * class, name, parameter list, and return type.
   *
   * @param o the object to compare to
   * @return <code>true</code> if they are equal; <code>false</code> if not
   */
  public boolean equals(Object o)
  {
    return m.equals(o);
  }

  /**
   * Get the hash code for the Method. The Method hash code is the hash code
   * of its name XOR'd with the hash code of its class name.
   *
   * @return the hash code for the object
   */
  public int hashCode()
  {
    return m.getDeclaringClass().getName().hashCode() ^ m.getName().hashCode();
  }

  /**
   * Get a String representation of the Method. A Method's String
   * representation is "&lt;modifiers&gt; &lt;returntype&gt;
   * &lt;methodname&gt;(&lt;paramtypes&gt;) throws &lt;exceptions&gt;", where
   * everything after ')' is omitted if there are no exceptions.<br> Example:
   * <code>public static int run(java.lang.Runnable,int)</code>
   *
   * @return the String representation of the Method
   */
  public String toString()
  {
    // 128 is a reasonable buffer initial size for constructor
    CPStringBuilder sb = new CPStringBuilder(128);
    Modifier.toString(getModifiers(), sb).append(' ');
    sb.append(ClassHelper.getUserName(getReturnType())).append(' ');
    sb.append(getDeclaringClass().getName()).append('.');
    sb.append(getName()).append('(');
    Class[] c = getParameterTypes();
    if (c.length > 0)
      {
        sb.append(ClassHelper.getUserName(c[0]));
        for (int i = 1; i < c.length; i++)
          sb.append(',').append(ClassHelper.getUserName(c[i]));
      }
    sb.append(')');
    c = getExceptionTypes();
    if (c.length > 0)
      {
        sb.append(" throws ").append(c[0].getName());
        for (int i = 1; i < c.length; i++)
          sb.append(',').append(c[i].getName());
      }
    return sb.toString();
  }

  public String toGenericString()
  {
    // 128 is a reasonable buffer initial size for constructor
    CPStringBuilder sb = new CPStringBuilder(128);
    Modifier.toString(getModifiers(), sb).append(' ');
    Constructor.addTypeParameters(sb, getTypeParameters());
    sb.append(getGenericReturnType()).append(' ');
    sb.append(getDeclaringClass().getName()).append('.');
    sb.append(getName()).append('(');
    Type[] types = getGenericParameterTypes();
    if (types.length > 0)
      {
        sb.append(types[0]);
        for (int i = 1; i < types.length; i++)
          sb.append(',').append(types[i]);
      }
    sb.append(')');
    types = getGenericExceptionTypes();
    if (types.length > 0)
      {
        sb.append(" throws ").append(types[0]);
        for (int i = 1; i < types.length; i++)
          sb.append(',').append(types[i]);
      }
    return sb.toString();
  }

  /**
   * Invoke the method. Arguments are automatically unwrapped and widened,
   * and the result is automatically wrapped, if needed.<p>
   *
   * If the method is static, <code>o</code> will be ignored. Otherwise,
   * the method uses dynamic lookup as described in JLS 15.12.4.4. You cannot
   * mimic the behavior of nonvirtual lookup (as in super.foo()). This means
   * you will get a <code>NullPointerException</code> if <code>o</code> is
   * null, and an <code>IllegalArgumentException</code> if it is incompatible
   * with the declaring class of the method. If the method takes 0 arguments,
   * you may use null or a 0-length array for <code>args</code>.<p>
   *
   * Next, if this Method enforces access control, your runtime context is
   * evaluated, and you may have an <code>IllegalAccessException</code> if
   * you could not acces this method in similar compiled code. If the method
   * is static, and its class is uninitialized, you trigger class
   * initialization, which may end in a
   * <code>ExceptionInInitializerError</code>.<p>
   *
   * Finally, the method is invoked. If it completes normally, the return value
   * will be null for a void method, a wrapped object for a primitive return
   * method, or the actual return of an Object method. If it completes
   * abruptly, the exception is wrapped in an
   * <code>InvocationTargetException</code>.
   *
   * @param o the object to invoke the method on
   * @param args the arguments to the method
   * @return the return value of the method, wrapped in the appropriate
   *         wrapper if it is primitive
   * @throws IllegalAccessException if the method could not normally be called
   *         by the Java code (i.e. it is not public)
   * @throws IllegalArgumentException if the number of arguments is incorrect;
   *         if the arguments types are wrong even with a widening conversion;
   *         or if <code>o</code> is not an instance of the class or interface
   *         declaring this method
   * @throws InvocationTargetException if the method throws an exception
   * @throws NullPointerException if <code>o</code> is null and this field
   *         requires an instance
   * @throws ExceptionInInitializerError if accessing a static method triggered
   *         class initialization, which then failed
   */
  public Object invoke(Object o, Object... args)
    throws IllegalAccessException, InvocationTargetException
  {
    return m.invoke(o, args);
  }

  /**
   * Returns an array of <code>TypeVariable</code> objects that represents
   * the type variables declared by this constructor, in declaration order.
   * An array of size zero is returned if this class has no type
   * variables.
   *
   * @return the type variables associated with this class. 
   * @throws GenericSignatureFormatError if the generic signature does
   *         not conform to the format specified in the Virtual Machine
   *         specification, version 3.
   * @since 1.5
   */
  public TypeVariable<Method>[] getTypeParameters()
  {
    if (p == null)
      {
	String sig = m.getSignature();
	if (sig == null)
	  return (TypeVariable<Method>[]) new TypeVariable[0];
	p = new MethodSignatureParser(this, sig);
      }
    return p.getTypeParameters();
  }

  /**
   * Returns an array of <code>Type</code> objects that represents
   * the exception types declared by this method, in declaration order.
   * An array of size zero is returned if this method declares no
   * exceptions.
   *
   * @return the exception types declared by this method. 
   * @throws GenericSignatureFormatError if the generic signature does
   *         not conform to the format specified in the Virtual Machine
   *         specification, version 3.
   * @since 1.5
   */
  public Type[] getGenericExceptionTypes()
  {
    if (p == null)
      {
	String sig = m.getSignature();
	if (sig == null)
	  return getExceptionTypes();
	p = new MethodSignatureParser(this, sig);
      }
    return p.getGenericExceptionTypes();
  }

  /**
   * Returns an array of <code>Type</code> objects that represents
   * the parameter list for this method, in declaration order.
   * An array of size zero is returned if this method takes no
   * parameters.
   *
   * @return a list of the types of the method's parameters
   * @throws GenericSignatureFormatError if the generic signature does
   *         not conform to the format specified in the Virtual Machine
   *         specification, version 3.
   * @since 1.5
   */
  public Type[] getGenericParameterTypes()
  {
    if (p == null)
      {
	String sig = m.getSignature();
	if (sig == null)
	  return getParameterTypes();
	p = new MethodSignatureParser(this, sig);
      }
    return p.getGenericParameterTypes();
  }

  /**
   * Returns the return type of this method.
   *
   * @return the return type of this method
   * @throws GenericSignatureFormatError if the generic signature does
   *         not conform to the format specified in the Virtual Machine
   *         specification, version 3.
   * @since 1.5
   */
  public Type getGenericReturnType()
  {
    if (p == null)
      {
	String sig = m.getSignature();
	if (sig == null)
	  return getReturnType();
	p = new MethodSignatureParser(this, sig);
      }
    return p.getGenericReturnType();
  }

  /**
   * If this method is an annotation method, returns the default
   * value for the method.  If there is no default value, or if the
   * method is not a member of an annotation type, returns null.
   * Primitive types are wrapped.
   *
   * @throws TypeNotPresentException if the method returns a Class,
   * and the class cannot be found
   *
   * @since 1.5
   */
  public Object getDefaultValue()
  {
    return m.getDefaultValue();
  }

  /**
   * <p>
   * Return an array of arrays representing the annotations on each
   * of the method's parameters.  The outer array is aligned against
   * the parameters of the method and is thus equal in length to
   * the number of parameters (thus having a length zero if there are none).
   * Each array element in the outer array contains an inner array which
   * holds the annotations.  This array has a length of zero if the parameter
   * has no annotations.
   * </p>
   * <p>
   * The returned annotations are serialized.  Changing the annotations has
   * no affect on the return value of future calls to this method.
   * </p>
   * 
   * @return an array of arrays which represents the annotations used on the
   *         parameters of this method.  The order of the array elements
   *         matches the declaration order of the parameters.
   * @since 1.5
   */
  public Annotation[][] getParameterAnnotations()
  {
    return m.getParameterAnnotations();
  }

  /**
   * Returns the element's annotation for the specified annotation type,
   * or <code>null</code> if no such annotation exists.
   *
   * @param annotationClass the type of annotation to look for.
   * @return this element's annotation for the specified type, or
   *         <code>null</code> if no such annotation exists.
   * @throws NullPointerException if the annotation class is <code>null</code>.
   */
  @SuppressWarnings("unchecked")
  public <T extends Annotation> T getAnnotation(Class<T> annotationClass)
  {
    return (T) m.getAnnotation(annotationClass);
  }

  /**
   * Returns all annotations directly defined by the element.  If there are
   * no annotations directly associated with the element, then a zero-length
   * array will be returned.  The returned array may be modified by the client
   * code, but this will have no effect on the annotation content of this
   * class, and hence no effect on the return value of this method for
   * future callers.
   *
   * @return the annotations directly defined by the element.
   * @since 1.5
   */
  public Annotation[] getDeclaredAnnotations()
  {
    return m.getDeclaredAnnotations();
  }

}
