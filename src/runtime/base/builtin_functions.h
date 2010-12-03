/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010 Facebook, Inc. (http://www.facebook.com)          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#ifndef __HPHP_BUILTIN_FUNCTIONS_H__
#define __HPHP_BUILTIN_FUNCTIONS_H__

#include <runtime/base/execution_context.h>
#include <runtime/base/types.h>
#include <runtime/base/complex_types.h>
#include <runtime/base/binary_operations.h>
#include <runtime/base/string_offset.h>
#include <runtime/base/frame_injection.h>
#include <runtime/base/intercept.h>
#include <runtime/base/runtime_error.h>
#include <runtime/base/runtime_option.h>
#include <runtime/base/variable_unserializer.h>
#include <util/case_insensitive.h>
#ifdef TAINTED
#include <runtime/base/propagate_tainting.h>
#endif

#ifdef __APPLE__
# ifdef isset
#  undef isset
# endif
#endif

/**
 * This file contains a list of functions that HPHP generates to wrap around
 * different expressions to maintain semantics. If we read through all types of
 * expressions in compiler/expression/expression.h, we can find most of them can be
 * directly transformed into C/C++ counterpart without too much syntactical
 * changes. The functions in this file happen to be the ones that are somewhat
 * special.
 *
 * Another way to think about this file is that this file has a list of C-style
 * functions, and the rest of run-time has object/classes for other tasks,
 * although we do have some global functions defined in other files as well,
 * when they are closer to the classes/objects in the same files.
 */

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////
// empty

inline bool empty(bool    v) { return !v;}
inline bool empty(char    v) { return !v;}
inline bool empty(short   v) { return !v;}
inline bool empty(int     v) { return !v;}
inline bool empty(int64   v) { return !v;}
inline bool empty(double  v) { return !v;}
inline bool empty(litstr  v) { return !v || !*v;}
inline bool empty(const StringData *v) { return v ? v->toBoolean() : false;}
inline bool empty(CStrRef v) { return !v.toBoolean();}
inline bool empty(CArrRef v) { return !v.toBoolean();}
inline bool empty(CObjRef v) { return !v.toBoolean();}
inline bool empty(CVarRef v) { return !v.toBoolean();}

bool empty(CVarRef v, bool    offset);
bool empty(CVarRef v, char    offset);
bool empty(CVarRef v, short   offset);
bool empty(CVarRef v, int     offset);
bool empty(CVarRef v, int64   offset);
bool empty(CVarRef v, double  offset);
bool empty(CVarRef v, CArrRef offset);
bool empty(CVarRef v, CObjRef offset);
bool empty(CVarRef v, CVarRef offset);
bool empty(CVarRef v, litstr  offset, bool isString = false);
bool empty(CVarRef v, CStrRef offset, bool isString = false);

///////////////////////////////////////////////////////////////////////////////
// operators

/**
 * These functions are rarely performance bottlenecks, so we are not fully
 * type-specialized, although we could.
 */

inline bool logical_xor(bool v1, bool v2) { return (v1 ? 1:0) ^ (v2 ? 1:0);}
inline Variant bitwise_or (CVarRef v1, CVarRef v2) { return v1 | v2;}
inline Variant bitwise_and(CVarRef v1, CVarRef v2) { return v1 & v2;}
inline Variant bitwise_xor(CVarRef v1, CVarRef v2) { return v1 ^ v2;}
inline Numeric multiply(CVarRef v1, CVarRef v2)    { return v1 * v2;}
inline Variant plus(CVarRef v1, CVarRef v2)        { return v1 + v2;}
inline Numeric minus(CVarRef v1, CVarRef v2)       { return v1 - v2;}
inline Numeric divide(CVarRef v1, CVarRef v2)      { return v1 / v2; }
inline Numeric modulo(int64 v1, int64 v2) {
  if (abs(v2) == 1) {
    return 0;
  }
  if (v2 == 0) {
    raise_warning("Division by zero");
    return false;
  }
  return v1 % v2;
}
inline int64 shift_left(int64 v1, int64 v2)        { return v1 << v2; }
inline int64 shift_right(int64 v1, int64 v2)       { return v1 >> v2; }

inline char    negate(char v)    { return -v; }
inline short   negate(short v)   { return -v; }
inline int     negate(int v)     { return -v; }
inline int64   negate(int64 v)   { return -v; }
inline double  negate(double v)  { return -v; }
inline Variant negate(CVarRef v) { return -(Variant)v; }

inline String concat(CStrRef s1, CStrRef s2)         {
  #ifndef TAINTED
  return s1 + s2;
  #else
  String res = s1 + s2;
  propagate_tainting2(s1, s2, res);
  return res;
  #endif
}
inline String &concat_assign(String &s1, litstr s2)  {
  return s1 += s2;
  // nothing to be done for tainting
}
inline String &concat_assign(String &s1, CStrRef s2) {
  #ifndef TAINTED
  return s1 += s2;
  #else
  propagate_tainting2(s1, s2, s1 += s2);
  return s1;
  #endif
}

#define MAX_CONCAT_ARGS 6
String concat3(CStrRef s1, CStrRef s2, CStrRef s3);
String concat4(CStrRef s1, CStrRef s2, CStrRef s3, CStrRef s4);
String concat5(CStrRef s1, CStrRef s2, CStrRef s3, CStrRef s4, CStrRef s5);
String concat6(CStrRef s1, CStrRef s2, CStrRef s3, CStrRef s4, CStrRef s5,
               CStrRef s6);

inline Variant &concat_assign(Variant &v1, litstr s2) {
  if (v1.getType() == KindOfString) {
    StringData *data = v1.getStringData();
    if (data->getCount() == 1) {
      data->append(s2, strlen(s2));
      // nothing to be done for tainting
      return v1;
    }
  }
  String s1 = v1.toString();
  s1 += s2;
  v1 = s1;
  // nothing to be done for tainting
  return v1;
}

inline Variant &concat_assign(Variant &v1, CStrRef s2) {
  if (v1.getType() == KindOfString) {
    StringData *data = v1.getStringData();
    if (data->getCount() == 1) {
      data->append(s2.data(), s2.size());
      #ifndef TAINTED
      return v1;
      #else
      String s1 = v1.toString();
      propagate_tainting2(s1, s2, s1);
      return v1;
      #endif
    }
  }
  String s1 = v1.toString();
  s1 += s2;
  #ifdef TAINTED
  propagate_tainting2(s1, s2, s1);
  #endif
  v1 = s1;
  return v1;
}

inline String &concat_assign(const StringOffset &s1, litstr s2) {
  return concat_assign(s1.lval(), s2);
}

inline String &concat_assign(const StringOffset &s1, CStrRef s2) {
  return concat_assign(s1.lval(), s2);
}

inline bool instanceOf(bool    v, CStrRef s) { return false;}
inline bool instanceOf(char    v, CStrRef s) { return false;}
inline bool instanceOf(short   v, CStrRef s) { return false;}
inline bool instanceOf(int     v, CStrRef s) { return false;}
inline bool instanceOf(int64   v, CStrRef s) { return false;}
inline bool instanceOf(double  v, CStrRef s) { return false;}
inline bool instanceOf(litstr  v, CStrRef s) { return false;}
inline bool instanceOf(CStrRef v, CStrRef s) { return false;}
inline bool instanceOf(CArrRef v, CStrRef s) { return false;}
inline bool instanceOf(CObjRef v, CStrRef s) { return v.instanceof(s);}
inline bool instanceOf(CVarRef v, CStrRef s) {
  return v.is(KindOfObject) &&
    v.toObject().instanceof(s);
}
inline bool instanceOf(ObjectData *v, CStrRef s) {
  return v && v->o_instanceof(s);
}

template <class K, class V>
const V &String::set(K key, const V &value) {
  (m_px = StringData::escalate(m_px))->setChar(toInt32(key), toString(value));
  return value;
}

///////////////////////////////////////////////////////////////////////////////
// output functions

inline int print(const char *s) {
  g_context->write(s);
  return 1;
}
inline int print(CStrRef s) {
  g_context->write(s);
  return 1;
}
inline void echo(const char *s) {
  g_context->write(s);
}
inline void echo(CStrRef s) {
  g_context->write(s);
}

String get_source_filename(litstr path);

void throw_exception(CObjRef e);
bool set_line(int line0, int char0 = 0, int line1 = 0, int char1 = 0);

///////////////////////////////////////////////////////////////////////////////
// isset/unset

inline bool isInitialized(CVarRef v) { return v.isInitialized();}
Variant getDynamicConstant(CVarRef v, CStrRef name);
String getUndefinedConstant(CStrRef name);

inline bool isset(CVarRef v) { return !v.isNull();}
inline bool isset(CObjRef v) { return !v.isNull();}
bool isset(CVarRef v, bool    offset);
bool isset(CVarRef v, char    offset);
bool isset(CVarRef v, short   offset);
bool isset(CVarRef v, int     offset);
bool isset(CVarRef v, int64   offset);
bool isset(CVarRef v, double  offset);
bool isset(CVarRef v, CArrRef offset);
bool isset(CVarRef v, CObjRef offset);
bool isset(CVarRef v, CVarRef offset);
bool isset(CVarRef v, litstr  offset, bool isString = false);
bool isset(CVarRef v, CStrRef offset, bool isString = false);

inline Variant unset(Variant &v)               { v.unset();   return null;}
inline Variant unset(CVarRef v)                {              return null;}
inline Variant setNull(Variant &v)             { v.setNull(); return null;}
inline Variant unset(Object &v)                { v.reset();   return null;}

///////////////////////////////////////////////////////////////////////////////
// special variable contexts

/**
 * lval() is mainly to make this work,
 *
 *   $arr['a']['b'] = $value;
 *
 * where $arr['a'] is in an l-value context. Note that lval() cannot replace
 * those offset classes, because calling these lval() functions will actually
 * insert a null value into an array/object, whereas an offset class can be
 * more powerful by not inserting a dummy value beforehand. For example,
 *
 *   isset($arr['a']); // we have to use offset's exists() function
 *   $obj['a'] = $value; // ArrayAccess's offset is completely customized
 *
 */
template<class T>
T &lval(T &v) { return v; }
inline Variant &lval(Variant &v) { return v;}
inline Array   &lval(Array   &v) { return v;}
inline Variant &lval(CVarRef  v) { // in case generating lval(1)
  throw FatalErrorException("taking reference from an r-value");
}
inline String  &lval(const StringOffset  &v) { return v.lval();}

template<class T>
Variant &unsetLval(Variant &v, const T &key) {
  if (v.isNull()) {
    return v;
  }
  if (v.is(KindOfArray)) {
    if (v.toArray().exists(key)) {
      return v.lvalAt(key);
    }
    return Variant::lvalBlackHole();
  }
  return Variant::lvalInvalid();
}

template<class T>
Variant &unsetLval(Array &v, const T &key) {
  if (!v.isNull() && v.exists(key)) {
    return v.lvalAt(key);
  }
  return Variant::lvalBlackHole();
}

/**
 * ref() sets contagious flag, so that next assignment will make both sides
 * strongly bind to the same underlying variant data. For example,
 *
 *   a = ref(b); // strong binding: now both a and b point to the same data
 *   a = b;      // weak binding: a will copy or copy-on-write
 *
 * The case of VarNR is only supposed to show up in ifa_ calls where it
 * it should be made no effect.
 */
inline CVarRef ref(CVarRef v) {
  if (!v.isVarNR()) {
    v.setContagious();
  } else {
    ASSERT(false);
  }
  return v;
}

///////////////////////////////////////////////////////////////////////////////
// misc functions

bool class_exists(CStrRef class_name, bool autoload = true);
String get_static_class_name(CVarRef objOrClassName);

Variant f_call_user_func_array(CVarRef function, CArrRef params,
                               bool bound = false);

Variant invoke(CStrRef function, CArrRef params, int64 hash = -1,
               bool tryInterp = true, bool fatal = true);
/**
 * Invoking an arbitrary static method.
 */
Variant invoke_static_method(CStrRef s, CStrRef method,
                             CArrRef params, bool fatal = true);
/**
 * For "static::" resolution
 */
Variant invoke_static_method_bind(CStrRef s, CStrRef method,
                                  CArrRef params, bool fatal = true);

/**
 * Fallback when a dynamic function call fails to find a user function
 * matching the name.  If no handlers are able to
 * invoke the function, throw an InvalidFunctionCallException.
 */
Variant invoke_failed(const char *func, CArrRef params, int64 hash,
                      bool fatal = true);

Variant o_invoke_failed(const char *cls, const char *meth,
                        bool fatal = true);

Array collect_few_args(int count, INVOKE_FEW_ARGS_IMPL_ARGS);
Array collect_few_args_ref(int count, INVOKE_FEW_ARGS_IMPL_ARGS);

void get_call_info_or_fail(const CallInfo *&ci, void *&extra, CStrRef name);

/**
 * When fatal coding errors are transformed to this function call.
 */
inline Variant throw_fatal(const char *msg, void *dummy = NULL) {
  throw FatalErrorException(msg);
}
inline Variant throw_missing_class(const char *cls) {
  throw ClassNotFoundException((std::string("unknown class ") + cls).c_str());
}

inline Variant throw_missing_file(const char *cls) {
  throw PhpFileDoesNotExistException(cls);
}
void throw_instance_method_fatal(const char *name);

/**
 * Argument count handling.
 *   - When level is 2, it's from constructors that turn these into fatals
 *   - When level is 1, it's from system funcs that turn both into warnings
 *   - When level is 0, it's from user funcs that turn missing arg in warnings
 */
Variant throw_missing_arguments(const char *fn, int num, int level = 0);
Variant throw_toomany_arguments(const char *fn, int num, int level = 0);
Variant throw_wrong_arguments(const char *fn, int count, int cmin, int cmax,
                              int level = 0);
Variant throw_missing_typed_argument(const char *fn, const char *type, int arg);

/**
 * When fatal coding errors are transformed to this function call.
 */
inline Object throw_fatal_object(const char *msg, void *dummy = NULL) {
  throw FatalErrorException(msg);
}

void throw_unexpected_argument_type(int argNum, const char *fnName,
                                    const char *expected, CVarRef val);

/**
 * Handler for exceptions thrown from object destructors. Implemented in
 * program_functions.cpp.
 */
void handle_destructor_exception();

/**
 * If RuntimeOption::ThrowBadTypeExceptions is on, we are running in
 * a restrictive mode that's not compatible with PHP, and this will throw.
 * If RuntimeOption::ThrowBadTypeExceptions is off, we will log a
 * warning and swallow the error.
 */
void throw_bad_type_exception(const char *fmt, ...);

/**
 * If RuntimeOption::ThrowInvalidArguments is on, we are running in
 * a restrictive mode that's not compatible with PHP, and this will throw.
 * If RuntimeOption::ThrowInvalidArguments is off, we will log a
 * warning and swallow the error.
 */
void throw_invalid_argument(const char *fmt, ...);

/**
 * Unsetting ClassName::StaticProperty.
 */
Variant throw_fatal_unset_static_property(const char *s, const char *prop);

/**
 * Exceptions injected code throws
 */
void throw_infinite_loop_exception() ATTRIBUTE_COLD;
void throw_infinite_recursion_exception() ATTRIBUTE_COLD;
void throw_request_timeout_exception() ATTRIBUTE_COLD;
void throw_memory_exceeded_exception() ATTRIBUTE_COLD __attribute__((noreturn));
void throw_call_non_object() ATTRIBUTE_COLD __attribute__((noreturn));

/**
 * Cloning an object.
 */
Object f_clone(CVarRef v);

/**
 * Serialize/unserialize a variant into/from a string. We need these two
 * functions in runtime/base, as there are functions in runtime/base that depend on
 * these two functions.
 */
String f_serialize(CVarRef value);
Variant unserialize_ex(CStrRef str, VariableUnserializer::Type type);
inline Variant f_unserialize(CStrRef str) {
  return unserialize_ex(str, VariableUnserializer::Serialize);
}

class LVariableTable;
Variant include(CStrRef file, bool once = false,
                LVariableTable* variables = NULL,
                const char *currentDir = "");
Variant require(CStrRef file, bool once = false,
                LVariableTable* variables = NULL,
                const char *currentDir = "");
Variant include_impl_invoke(CStrRef file, bool once = false,
                            LVariableTable* variables = NULL,
                            const char *currentDir = "");

inline void assignCallTemp(Variant& temp, CVarRef val) {
  temp.unset();
  temp.clearContagious();
  temp = val;
  if (temp.isReferenced()) {
    temp.setContagious();
  }
}

/**
 * For wrapping expressions that have no effect, so to make gcc happy.
 */
inline bool    id(bool    v) { return v; }
inline char    id(char    v) { return v; }
inline short   id(short   v) { return v; }
inline int     id(int     v) { return v; }
inline int64   id(int64   v) { return v; }
inline uint64  id(uint64  v) { return v; }
inline double  id(double  v) { return v; }
inline litstr  id(litstr  v) { return v; }
inline CStrRef id(CStrRef v) { return v; }
inline CArrRef id(CArrRef v) { return v; }
inline CObjRef id(CObjRef v) { return v; }
inline CVarRef id(CVarRef v) { return v; }
template <class T>
inline const SmartObject<T> &id(const SmartObject<T> &v) { return v; }

/**
 * For wrapping return values to prevent elision of copy
 * constructors (which can incorrectly pass through
 * the contagious bit).
 */
inline Variant wrap_variant(CVarRef x) { return x; }

inline LVariableTable *lvar_ptr(const LVariableTable &vt) {
  return const_cast<LVariableTable*>(&vt);
}

bool function_exists(CStrRef function_name);

/**
 * For autoload support
 */
class Globals;
void checkClassExists(CStrRef name, Globals *g, bool nothrow = false);
bool checkClassExists(CStrRef name, const bool *declared, bool autoloadExists,
                      bool nothrow = false);
bool checkInterfaceExists(CStrRef name, const bool *declared,
                          bool autoloadExists, bool nothrow = false);

class CallInfo;

class MethodCallPackage {
public:
  MethodCallPackage() : ci(NULL), extra(NULL), m_fatal(true), obj(NULL) {}

  // e->n() style method call
  void methodCall(CObjRef self, CStrRef method, int64 prehash = -1) {
    methodCall(self.get(), method, prehash);
  }
  void methodCall(ObjectData *self, CStrRef method, int64 prehash = -1) {
    rootObj = self;
    name = method;
    self->o_get_call_info(*this, prehash);
  }
  void methodCallWithIndex(CObjRef self, CStrRef method, MethodIndex mi,
                           int64 prehash = -1) {
    methodCallWithIndex(self.get(), method, mi, prehash);
  }
  void methodCallWithIndex(ObjectData *self, CStrRef method, MethodIndex mi,
                           int64 prehash = -1) {
    rootObj = self;
    name = method;
    self->o_get_call_info_with_index(*this, mi, prehash);
  }
  // K::n() style call, where K is a parent and n is not static and in an
  // instance method. Lookup is done outside since K is known.
  void methodCallEx(CObjRef self, CStrRef method) {
    rootObj = self;
    name = method;
  }
  // K::n() style call where K::n() is a static method. Lookup is done outside
  void staticMethodCall(litstr cname, CStrRef method) {
    rootObj = cname;
    name = method;
  }
  // e::n() call. e could evaluate to be either a string or object.
  void dynamicNamedCall(CVarRef self, CStrRef method, int64 prehash = -1);
  void dynamicNamedCallWithIndex(CVarRef self, CStrRef method,
      MethodIndex mi, int64 prehash = -1);
  // e::n() call where e is definitely a string
  void dynamicNamedCall(CStrRef self, CStrRef method, int64 prehash = -1);
  void dynamicNamedCallWithIndex(CStrRef self, CStrRef method,
      MethodIndex mi, int64 prehash = -1);
  void dynamicNamedCall(const char *self, CStrRef method, int64 prehash = -1);
  void dynamicNamedCallWithIndex(const char *self, CStrRef method,
      MethodIndex mi, int64 prehash = -1);
  // Get constructor
  void construct(CObjRef self);

  void noFatal() { m_fatal = false; }
  void fail();
  void lateStaticBind(ThreadInfo *ti);
  const CallInfo *bindClass(ThreadInfo *ti);
  String getClassName();
  const CallInfo *ci;
  void *extra;
  Variant rootObj; // object or class name
  bool m_fatal;
  ObjectData *obj;
  String name;
};

class CallInfo {
public:
  enum Flags {
    VarArgs = 0x1,
    RefVarArgs = 0x2,
    Method = 0x4,
    StaticMethod = 0x8,
    CallMagicMethod = 0x10 // Special flag for __call handler
  };
  CallInfo(void *inv, void *invFa, int ac, int flags, int64 refs)
    : m_invoker(inv), m_invokerFewArgs(invFa), m_argCount(ac), m_flags(flags),
    m_refFlags(refs) {}
  void *m_invoker;
  void *m_invokerFewArgs; // remove in time
  int m_argCount;
  int m_flags;
  int64 m_refFlags;
  bool isRef(int n) const {
    return n <= m_argCount ? (m_refFlags & (1 << n)) : (m_flags & RefVarArgs);
  }
  typedef Variant (*FuncInvoker)(void*, CArrRef);
  typedef Variant (*FuncInvokerFewArgs)(void*, int,
      INVOKE_FEW_ARGS_IMPL_ARGS);
  FuncInvoker getFunc() const { return (FuncInvoker)m_invoker; }
  FuncInvokerFewArgs getFuncFewArgs() const {
    return (FuncInvokerFewArgs)m_invokerFewArgs;
  }
  typedef Variant (*MethInvoker)(MethodCallPackage &mcp, CArrRef);
  typedef Variant (*MethInvokerFewArgs)(MethodCallPackage &mcp, int,
      INVOKE_FEW_ARGS_IMPL_ARGS);
  MethInvoker getMeth() const {
    return (MethInvoker)m_invoker;
  }
  MethInvokerFewArgs getMethFewArgs() const {
    return (MethInvokerFewArgs)m_invokerFewArgs;
  }
};

///////////////////////////////////////////////////////////////////////////////
}

#endif // __HPHP_BUILTIN_FUNCTIONS_H__
