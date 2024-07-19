// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*-----------------------------------------------------------------------------------------------------------
SafeInt.hpp
Version 3.0.25p

This header implements an integer handling class designed to catch
unsafe integer operations

This header compiles properly at Wall on Visual Studio, -Wall on gcc, and -Weverything on clang.
Tested most recently on clang 3.8.0, gcc 7.3.1, and both Visual Studio 2015 and 2017.

Please read the leading comments before using the class.
---------------------------------------------------------------*/
#ifndef SAFEINT_HPP
#define SAFEINT_HPP

// It is a bit tricky to sort out what compiler we are actually using,
// do this once here, and avoid cluttering the code
#define VISUAL_STUDIO_COMPILER 0
#define CLANG_COMPILER 1
#define GCC_COMPILER 2
#define UNKNOWN_COMPILER -1

// Clang will sometimes pretend to be Visual Studio
// and does pretend to be gcc. Check it first, as nothing else pretends to be clang
#if defined __clang__
#define SAFEINT_COMPILER CLANG_COMPILER
#elif defined __GNUC__
#define SAFEINT_COMPILER GCC_COMPILER
#elif defined _MSC_VER
#define SAFEINT_COMPILER VISUAL_STUDIO_COMPILER
#else
#define SAFEINT_COMPILER UNKNOWN_COMPILER
#endif

#define CPLUSPLUS_98 0
#define CPLUSPLUS_11 1
#define CPLUSPLUS_14 2
#define CPLUSPLUS_17 3 // Future use

// Determine C++ support level
#if SAFEINT_COMPILER == CLANG_COMPILER || SAFEINT_COMPILER == GCC_COMPILER

#if __cplusplus < 201103L
#define CPLUSPLUS_STD CPLUSPLUS_98
#elif __cplusplus < 201402L
#define CPLUSPLUS_STD CPLUSPLUS_11
#else 
#define CPLUSPLUS_STD CPLUSPLUS_14
#endif

#elif SAFEINT_COMPILER == VISUAL_STUDIO_COMPILER

// This needs additional testing to get more versions of _MSCVER
#if _MSC_VER < 1900 // Prior to VS 2015, need more testing to determine support
#define CPLUSPLUS_STD CPLUSPLUS_98

#elif _MSC_VER < 1910 // VS 2015
#define CPLUSPLUS_STD CPLUSPLUS_11

#else // VS 2017 or later
// Note - there is a __cpp_constexpr test now, but everything prior to VS 2017 reports incorrect values
// and this version always supports at least the CPLUSPLUS_14 approach
#define CPLUSPLUS_STD CPLUSPLUS_14

#endif 

#else
// Unknown compiler, assume C++ 98
#define CPLUSPLUS_STD CPLUSPLUS_98
#endif // Determine C++ support level

#if (SAFEINT_COMPILER == CLANG_COMPILER || SAFEINT_COMPILER == GCC_COMPILER) && CPLUSPLUS_STD < CPLUSPLUS_11
#error Must compile with --std=c++11, preferably --std=c++14 to use constexpr improvements
#endif

#define CONSTEXPR_NONE 0
#define CONSTEXPR_CPP11 1
#define CONSTEXPR_CPP14 2

// Let's try to use the new standard to determine feature compliance
// If the user has an unknown compiler, or just for testing, allow forcing this setting
#if !defined CONSTEXPR_SUPPORT

#if defined __cpp_constexpr
// If it is gcc or clang, at least recent versions, then we have -std=c++11 or -std=c++14
// This won't be set otherwise, but the headers won't compile, either
#if __cpp_constexpr >= 201304L
#define CONSTEXPR_SUPPORT CONSTEXPR_CPP14 // Clang, gcc, Visual Studio 2017 or later
#elif __cpp_constexpr >= 200704L 
#define CONSTEXPR_SUPPORT CONSTEXPR_CPP11 // Clang, gcc with -std=c++11, Visual Studio 2015
#else
#define CONSTEXPR_SUPPORT CONSTEXPR_NONE
#endif

#else // !defined __cpp_constexpr
// Visual Studio is somehow not playing nice. shows __cpp_constexpr visually as defined, but won't compile
#if SAFEINT_COMPILER == VISUAL_STUDIO_COMPILER
#if CPLUSPLUS_STD == CPLUSPLUS_14
#define CONSTEXPR_SUPPORT CONSTEXPR_CPP14
#elif CPLUSPLUS_STD == CPLUSPLUS_11
#define CONSTEXPR_SUPPORT CONSTEXPR_CPP11
#else
#define CONSTEXPR_SUPPORT CONSTEXPR_NONE
#endif
#else
#define CONSTEXPR_SUPPORT CONSTEXPR_NONE
#endif

#endif // defined __cpp_constexpr

#endif // !defined CONSTEXPR_SUPPORT


#if CONSTEXPR_SUPPORT == CONSTEXPR_NONE
#define _CONSTEXPR11
#define _CONSTEXPR14
#elif CONSTEXPR_SUPPORT == CONSTEXPR_CPP11
#define _CONSTEXPR11 constexpr
#define _CONSTEXPR14
#elif CPLUSPLUS_STD >= CPLUSPLUS_14
#define _CONSTEXPR11 constexpr
#define _CONSTEXPR14 constexpr
#else
#error "Unexpected value of CPLUSPLUS_STD"
#endif

// Determine whether exceptions are enabled by the compiler
// Also, allow the user to force this, in case the compiler
// doesn't support the __cpp_exceptions feature
#if !defined SAFE_INT_HAS_EXCEPTIONS
    #if __cpp_exceptions >= 199711L
        #define SAFE_INT_HAS_EXCEPTIONS 1
    #else
        #define SAFE_INT_HAS_EXCEPTIONS 0
    #endif
#endif

// Enable compiling with /Wall under VC
#if SAFEINT_COMPILER == VISUAL_STUDIO_COMPILER
// Off by default - unreferenced inline function has been removed
// Note - this intentionally leaks from the header, doesn't quench the warnings otherwise
// Also disable Spectre mitigation warning
#pragma warning( disable: 4514 5045 )

#pragma warning( push )
// Disable warnings coming from headers
#pragma warning( disable:4987 4820 4987 4820 )
#endif

// More defines to accomodate compiler differences
#if SAFEINT_COMPILER == GCC_COMPILER || SAFEINT_COMPILER == CLANG_COMPILER
#define SAFEINT_NORETURN __attribute__((noreturn))
#define SAFEINT_STDCALL
#define SAFEINT_VISIBLE __attribute__ ((__visibility__("default")))
#define SAFEINT_WEAK __attribute__ ((weak))
#else
#define SAFEINT_NORETURN __declspec(noreturn)
#define SAFEINT_STDCALL __stdcall
#define SAFEINT_VISIBLE
#define SAFEINT_WEAK
#endif

// On the Microsoft compiler, violating a throw() annotation is a silent error.
// Other compilers might turn these into exceptions, and some users may want to not have throw() enabled.
// In addition, some error handlers may not throw C++ exceptions, which makes everything no throw.
#if defined SAFEINT_REMOVE_NOTHROW
#define SAFEINT_NOTHROW
#else
#define SAFEINT_NOTHROW noexcept
#endif

#include <cstdint>
#include <limits>
#include <type_traits> // This is now required
// Need this for ptrdiff_t on some compilers
#include <cstddef>
#include <cmath> // Needed for floating point implementation

// Note - intrinsics and constexpr are mutually exclusive
// If it is important to get constexpr for multiplication, then define SAFEINT_USE_INTRINSICS 0
// However, intrinsics will result in much smaller code, and should have better perf
#if SAFEINT_COMPILER == VISUAL_STUDIO_COMPILER && defined _M_AMD64 && !defined SAFEINT_USE_INTRINSICS
    #include <intrin.h>
    #define SAFEINT_USE_INTRINSICS 1
    #define _CONSTEXPR14_MULTIPLY 
#else
    #define SAFEINT_USE_INTRINSICS 0
    #define _CONSTEXPR14_MULTIPLY _CONSTEXPR14
#endif

// If you would like to use your own custom assert
// Define SAFEINT_ASSERT
#if !defined SAFEINT_ASSERT
#include <assert.h>
#define SAFEINT_ASSERT(x) assert(x)
#endif

#if SAFEINT_COMPILER == VISUAL_STUDIO_COMPILER
#pragma warning( pop )
#endif

#if !defined _CRT_SECURE_INVALID_PARAMETER
// Calling fail fast is somewhat more robust than calling abort, 
// but abort is the closest we can manage without Visual Studio support
// Need the header for abort()
#include <stdlib.h>
#define _CRT_SECURE_INVALID_PARAMETER(msg) abort()
#endif

// Let's test some assumptions
// We're assuming two's complement negative numbers
static_assert( -1 == static_cast<int>(0xffffffff), "Two's complement signed numbers are required" );

/************* Compiler Options *****************************************************************************************************

SafeInt supports several compile-time options that can change the behavior of the class.

Compiler options:
SAFEINT_ASSERT_ON_EXCEPTION        - it is often easier to stop on an assert and figure out a problem than to try and figure out
                                     how you landed in the catch block.

SafeIntDefaultExceptionHandler     - if you'd like to replace the exception handlers SafeInt provides, define your replacement and
                                     define this. There are three current options:

                                     SAFEINT_RAISE_EXCEPTION - Windows specific, throws a structured exception. This is legacy.
                                     SAFEINT_FAILFAST        - On Windows, calls __failfast, else calls abort()
                                     Default                 - Uses the SafeIntExceptionHandler class, throws a SafeIntException.

                                     If you do replace the exception handler, then make sure you define:
                                     
                                     SafeIntOnOverflow
                                     SafeIntOnDivZero

SAFEINT_DISALLOW_UNSIGNED_NEGATION - Invoking the unary negation operator may create warnings, but if you'd like it to completely fail
                                     to compile, define this.

SAFEINT_DISABLE_BINARY_ASSERT      - binary AND, OR or XOR operations on mixed size types can produce unexpected results. If you do
                                     this, the default is to assert. Set this if you prefer not to assert under these conditions.

SIZE_T_CAST_NEEDED                 - some compilers complain if there is not a cast to size_t, others complain if there is one.
                                     This lets you not have your compiler complain. Default is not to have an overload, and it 
                                     appears that no recent compilers need this.

SAFEINT_DISABLE_ADDRESS_OPERATOR   - Disables the overload of the & operator, which results in a raw pointer to the underlying type. This has
                                     been a debated feature for the entire life of the project - the benefit is that it makes it easier to just
                                     change a declaration from uint32_t to SafeInt<uint32_t>, and the downstream code is less likely to need
                                     modification, which is especially handy in legacy code bases. The drawback is that it breaks good C++
                                     practice, and breaks some libraries that auto-generate code. In the future, I expect to make disabling this the 
                                     default.

************************************************************************************************************************************/

/*
*  The SafeInt class is designed to have as low an overhead as possible
*  while still ensuring that all integer operations are conducted safely.
*  Nearly every operator has been overloaded, with a very few exceptions.
*
*  A usability-safety trade-off has been made to help ensure safety. This
*  requires that every operation return either a SafeInt or a bool. If we
*  allowed an operator to return a base integer type T, then the following
*  can happen:
*
*  char i = SafeInt<char>(32) * 2 + SafeInt<char>(16) * 4;
*
*  The * operators take precedence, get overloaded, return a char, and then
*  you have:
*
*  char i = (char)64 + (char)64; //overflow!
*
*  This situation would mean that safety would depend on usage, which isn't
*  acceptable.
*
*  One key operator that is missing is an implicit cast to type T. The reason for
*  this is that if there is an implicit cast operator, then we end up with
*  an ambiguous compile-time precedence. Because of this amiguity, there
*  are two methods that are provided:
*
*  Casting operators for every native integer type
*  Version 3 note - it now compiles correctly for size_t without warnings
*
*  SafeInt::Ptr()   - returns the address of the internal integer
*  Note - the '&' (address of) operator has been overloaded and returns
*         the address of the internal integer.
*
*  The SafeInt class should be used in any circumstances where ensuring
*  integrity of the calculations is more important than performance. See Performance
*  Notes below for additional information.
*
*  Many of the conditionals will optimize out or be inlined for a release
*  build (especially with /Ox), but it does have significantly more overhead,
*  especially for signed numbers. If you do not _require_ negative numbers, use
*  unsigned integer types - certain types of problems cannot occur, and this class
*  performs most efficiently.
*
*  Here's an example of when the class should ideally be used -
*
*  void* AllocateMemForStructs(int StructSize, int HowMany)
*  {
*     SafeInt<unsigned long> s(StructSize);
*
*     s *= HowMany;
*
*     return malloc(s);
*
*  }
*
*  Here's when it should NOT be used:
*
*  void foo()
*  {
*    int i;
*
*    for(i = 0; i < 0xffff; i++)
*      ....
*  }
*
*  Error handling - a SafeInt class will throw exceptions if something
*  objectionable happens. The exceptions are SafeIntException classes,
*  which contain an enum as a code.
*
*  Typical usage might be:
*
*  bool foo()
*  {
*    SafeInt<unsigned long> s; //note that s == 0 unless set
*
*    try{
*      s *= 23;
*      ....
*    }
*    catch(SafeIntException err)
*    {
*       //handle errors here
*    }
*  }
*
*  Update for 3.0 - the exception class is now a template parameter.
*  You can replace the exception class with any exception class you like. This is accomplished by:
*  1) Create a class that has the following interface:
*
    template <> class YourSafeIntExceptionHandler < YourException >
    {
    public:
        static __declspec(noreturn) void __stdcall SafeIntOnOverflow()
        {
            throw YourException( YourSafeIntArithmeticOverflowError );
        }

        static __declspec(noreturn) void __stdcall SafeIntOnDivZero()
        {
            throw YourException( YourSafeIntDivideByZeroError );
        }
    };
*
*  Note that you don't have to throw C++ exceptions, you can throw Win32 exceptions, or do
*  anything you like, just don't return from the call back into the code.
*
*  2) Either explicitly declare SafeInts like so:
*     SafeInt< int, YourSafeIntExceptionHandler > si;
*  or
*     #define SafeIntDefaultExceptionHandler YourSafeIntExceptionHandler
*
*  Performance:
*
*  Due to the highly nested nature of this class, you can expect relatively poor
*  performance in unoptimized code. In tests of optimized code vs. correct inline checks
*  in native code, this class has been found to take approximately 8% more CPU time (this varies),
*  most of which is due to exception handling. Solutions:
*
*  1) Compile optimized code - /Ox is best, /O2 also performs well. Interestingly, /O1
*     (optimize for size) does not work as well.
*  2) If that 8% hit is really a serious problem, walk through the code and inline the
*     exact same checks as the class uses.
*  3) Some operations are more difficult than others - avoid using signed integers, and if
*     possible keep them all the same size. 64-bit integers are also expensive. Mixing
*     different integer sizes and types may prove expensive. Be aware that literals are
*     actually ints. For best performance, cast literals to the type desired.
*
*
*  Performance update
*  The current version of SafeInt uses template specialization to force the compiler to invoke only the
*  operator implementation needed for any given pair of types. This will dramatically improve the perf
*  of debug builds.
*
*  3.0 update - not only have we maintained the specialization, there were some cases that were overly complex,
*  and using some additional cases (e.g. std::int64_t and std::uint64_t) resulted in some simplification.
*  Additionally, there was a lot of work done to better optimize the 64-bit multiplication.
*
*  Binary Operators
*
*  All of the binary operators have certain assumptions built into the class design.
*  This is to ensure correctness. Notes on each class of operator follow:
*
*  Arithmetic Operators (*,/,+,-,%)
*  There are three possible variants:
*  SafeInt< T, E > op SafeInt< T, E >
*  SafeInt< T, E > op U
*  U op SafeInt< T, E >
*
*  The SafeInt< T, E > op SafeInt< U, E > variant is explicitly not supported, and if you try to do
*  this the compiler with throw the following error:
*
*  error C2593: 'operator *' is ambiguous
*
*  This is because the arithmetic operators are required to return a SafeInt of some type.
*  The compiler cannot know whether you'd prefer to get a type T or a type U returned. If
*  you need to do this, you need to extract the value contained within one of the two using
*  the casting operator. For example:
*
*  SafeInt< T, E > t, result;
*  SafeInt< U, E > u;
*
*  result = t * (U)u;
*
*  Comparison Operators
*  Because each of these operators return type bool, mixing SafeInts of differing types is
*  allowed.
*
*  Shift Operators
*  Shift operators always return the type on the left hand side of the operator. Mixed type
*  operations are allowed because the return type is always known.
*
*  Boolean Operators
*  Like comparison operators, these overloads always return type bool, and mixed-type SafeInts
*  are allowed. Additionally, specific overloads exist for type bool on both sides of the
*  operator.
*
*  Binary Operators
*  Mixed-type operations are discouraged, however some provision has been made in order to
*  enable things like:
*
*  SafeInt<char> c = 2;
*
*  if(c & 0x02)
*    ...
*
*  The "0x02" is actually an int, and it needs to work.
*  In the case of binary operations on integers smaller than 32-bit, or of mixed type, corner
*  cases do exist where you could get unexpected results. In any case where SafeInt returns a different
*  result than the underlying operator, it will call assert(). You should examine your code and cast things
*  properly so that you are not programming with side effects.
*
*  Documented issues:
*
*  This header compiles correctly at /W4 using VC++ 8 (Version 14.00.50727.42) and later.
*  As of this writing, I believe it will also work for VC 7.1, but not for VC 7.0 or below.
*  If you need a version that will work with lower level compilers, try version 1.0.7. None
*  of them work with Visual C++ 6, and gcc didn't work very well, either, though this hasn't
*  been tried recently.
*
*  It is strongly recommended that any code doing integer manipulation be compiled at /W4
*  - there are a number of warnings which pertain to integer manipulation enabled that are
*  not enabled at /W3 (default for VC++)
*
*  Perf note - postfix operators are slightly more costly than prefix operators.
*  Unless you're actually assigning it to something, ++SafeInt is less expensive than SafeInt++
*
*  The comparison operator behavior in this class varies from the ANSI definition, which is
*  arguably broken. As an example, consider the following:
*
*  unsigned int l = 0xffffffff;
*  char c = -1;
*
*  if(c == l)
*    printf("Why is -1 equal to 4 billion???\n");
*
*  The problem here is that c gets cast to an int, now has a value of 0xffffffff, and then gets
*  cast again to an unsigned int, losing the true value. This behavior is despite the fact that
*  an std::int64_t exists, and the following code will yield a different (and intuitively correct)
*  answer:
*
*  if((std::int64_t)c == (std::int64_t)l))
*    printf("Why is -1 equal to 4 billion???\n");
*  else
*    printf("Why doesn't the compiler upcast to 64-bits when needed?\n");
*
*  Note that combinations with smaller integers won't display the problem - if you
*  changed "unsigned int" above to "unsigned short", you'd get the right answer.
*
*  Revision history:
*
*  Oct 12, 2003 - Created
*  Author - David LeBlanc - dleblanc@microsoft.com
*
*  Oct 27, 2003 - fixed numerous items pointed out by michmarc and bdawson
*  Dec 28, 2003 - 1.0
*                 added support for mixed-type operations
*                 thanks to vikramh
*                 also fixed broken std::int64_t multiplication section
*                 added extended support for mixed-type operations where possible
*  Jan 28, 2004 - 1.0.1
*                 changed WCHAR to wchar_t
*                 fixed a construct in two mixed-type assignment overloads that was
*                 not compiling on some compilers
*                 Also changed name of private method to comply with standards on
*                 reserved names
*                 Thanks to Niels Dekker for the input
*  Feb 12, 2004 - 1.0.2
*                 Minor changes to remove dependency on Windows headers
*                 Consistently used std::int16_t, std::int32_t and std::int64_t to ensure
*                 portability
*  May 10, 2004 - 1.0.3
*                 Corrected bug in one case of GreaterThan
*  July 22, 2004 - 1.0.4
*                 Tightened logic in addition check (saving 2 instructions)
*                 Pulled error handler out into function to enable user-defined replacement
*                 Made internal type of SafeIntException an enum (as per Niels' suggestion)
*                 Added casts for base integer types (as per Scott Meyers' suggestion)
*                 Updated usage information - see important new perf notes.
*                 Cleaned up several const issues (more thanks to Niels)
*
*  Oct 1, 2004 - 1.0.5
*                 Added support for SEH exceptions instead of C++ exceptions - Win32 only
*                 Made handlers for DIV0 and overflows individually overridable
*                 Commented out the destructor - major perf gains here
*                 Added cast operator for type long, since long != std::int32_t
*                  Corrected a couple of missing const modifiers
*                 Fixed broken >= and <= operators for type U op SafeInt< T, E >
*  Nov 5, 2004 - 1.0.6
*                 Implemented new logic in binary operators to resolve issues with
*                 implicit casts
*                 Fixed casting operator because char != signed char
*                 Defined std::int32_t as int instead of long
*                 Removed unsafe SafeInt::Value method
*                 Re-implemented casting operator as a result of removing Value method
*  Dec 1, 2004 - 1.0.7
*                 Implemented specialized operators for pointer arithmetic
*                 Created overloads for cases of U op= SafeInt. What you do with U
*                 after that may be dangerous.
*                 Fixed bug in corner case of MixedSizeModulus
*                 Fixed bug in MixedSizeMultiply and MixedSizeDivision with input of 0
*                 Added throw() decorations
*
*  Apr 12, 2005 - 2.0
*                 Extensive revisions to leverage template specialization.
*  April, 2007    Extensive revisions for version 3.0
*  Nov 22, 2009   Forked from MS internal code
*                 Changes needed to support gcc compiler - many thanks to Niels Dekker
*                 for determining not just the issues, but also suggesting fixes.
*                 Also updating some of the header internals to be the same as the upcoming Visual Studio version.
*
*  Jan 16, 2010   64-bit gcc has long == std::int64_t, which means that many of the existing 64-bit
*                 templates are over-specialized. This forces a redefinition of all the 64-bit
*                 multiplication routines to use pointers instead of references for return
*                 values. Also, let's use some intrinsics for x64 Microsoft compiler to
*                 reduce code size, and hopefully improve efficiency.
*
*  June 21, 2014  Better support for clang, higher warning levels supported for all 3 primary supported
                  compilers (Visual Studio, clang, gcc).
                  Also started to converge the code base such that the public CodePlex version will
                  be a drop-in replacement for the Visual Studio version.

* Feb 12, 2018    Fixed floating point bug
*                 Fix to allow initialization by an enum
*                 Add support for static_assert, make it default to fix compiler warnings from C_ASSERT on gcc, clang
*                 Changed throw() to noexcept

* March, 2018     Introduced support for constexpr, both the C++11 and C++14 flavors work. The C++14 standard
                  allows for much more thorough usage, and should be preferred.

*  Note about code style - throughout this class, casts will be written using C-style (T),
*  not C++ style static_cast< T >. This is because the class is nearly always dealing with integer
*  types, and in this case static_cast and a C cast are equivalent. Given the large number of casts,
*  the code is a little more readable this way. In the event a cast is needed where static_cast couldn't
*  be substituted, we'll use the new templatized cast to make it explicit what the operation is doing.
*
************************************************************************************************************
* Version 3.0 changes:
*
* 1) The exception type thrown is now replacable, and you can throw your own exception types. This should help
*    those using well-developed exception classes.
* 2) The 64-bit multiplication code has had a lot of perf work done, and should be faster than 2.0.
* 3) There is now limited floating point support. You can initialize a SafeInt with a floating point type,
*    and you can cast it out (or assign) to a float as well.
* 4) There is now an Align method. I noticed people use this a lot, and rarely check errors, so now you have one.
*
* Another major improvement is the addition of external functions - if you just want to check an operation, this can now happen:
* All of the following can be invoked without dealing with creating a class, or managing exceptions. This is especially handy
* for 64-bit porting, since SafeCast compiles away for a 32-bit cast from size_t to unsigned long, but checks it for 64-bit.
*
* inline bool SafeCast( const T From, U& To ) throw()
* inline bool SafeEquals( const T t, const U u ) throw()
* inline bool SafeNotEquals( const T t, const U u ) throw()
* inline bool SafeGreaterThan( const T t, const U u ) throw()
* inline bool SafeGreaterThanEquals( const T t, const U u ) throw()
* inline bool SafeLessThan( const T t, const U u ) throw()
* inline bool SafeLessThanEquals( const T t, const U u ) throw()
* inline bool SafeModulus( const T& t, const U& u, T& result ) throw()
* inline bool SafeMultiply( T t, U u, T& result ) throw()
* inline bool SafeDivide( T t, U u, T& result ) throw()
* inline bool SafeAdd( T t, U u, T& result ) throw()
* inline bool SafeSubtract( T t, U u, T& result ) throw()
*
*/

// Warning - this very old work-around will be deprecated in future releases. 
#if defined VISUAL_STUDIO_SAFEINT_COMPAT
namespace msl
{

namespace utilities
{
#endif

// catch these to handle errors
// Currently implemented code values:
// ERROR_ARITHMETIC_OVERFLOW
// EXCEPTION_INT_DIVIDE_BY_ZERO
enum class SafeIntError
{
    SafeIntNoError = 0,
    SafeIntArithmeticOverflow,
    SafeIntDivideByZero
};

#if defined VISUAL_STUDIO_SAFEINT_COMPAT
} // utilities
} // msl
#endif


/*
* Error handler classes
* Using classes to deal with exceptions is going to allow the most
* flexibility, and we can mix different error handlers in the same project
* or even the same file. It isn't advisable to do this in the same function
* because a SafeInt< int, MyExceptionHandler > isn't the same thing as
* SafeInt< int, YourExceptionHander >.
* If for some reason you have to translate between the two, cast one of them back to its
* native type.
*
* To use your own exception class with SafeInt, first create your exception class,
* which may look something like the SafeIntException class below. The second step is to
* create a template specialization that implements SafeIntOnOverflow and SafeIntOnDivZero.
* For example:
*
* template <> class SafeIntExceptionHandler < YourExceptionClass >
* {
*     static __declspec(noreturn) void __stdcall SafeIntOnOverflow()
*     {
*         throw YourExceptionClass( EXCEPTION_INT_OVERFLOW );
*     }
*
*     static __declspec(noreturn) void __stdcall SafeIntOnDivZero()
*     {
*         throw YourExceptionClass( EXCEPTION_INT_DIVIDE_BY_ZERO );
*     }
* };
*
* typedef SafeIntExceptionHandler < YourExceptionClass > YourSafeIntExceptionHandler
* You'd then declare your SafeInt objects like this:
* SafeInt< int, YourSafeIntExceptionHandler >
*
* Unfortunately, there is no such thing as partial template specialization in typedef
* statements, so you have three options if you find this cumbersome:
*
* 1) Create a holder class:
*
* template < typename T >
* class MySafeInt
* {
*   public:
*   SafeInt< T, MyExceptionClass> si;
* };
*
* You'd then declare an instance like so:
* MySafeInt< int > i;
*
* You'd lose handy things like initialization - it would have to be initialized as:
*
* i.si = 0;
*
* 2) You could create a typedef for every int type you deal with:
*
* typedef SafeInt< int, MyExceptionClass > MySafeInt;
* typedef SafeInt< char, MyExceptionClass > MySafeChar;
*
* and so on. The second approach is probably more usable, and will just drop into code
* better, which is the original intent of the SafeInt class.
*
* 3) If you're going to consistently use a different class to handle your exceptions,
*    you can override the default typedef like so:
*
*    #define SafeIntDefaultExceptionHandler YourSafeIntExceptionHandler
*
*    Overall, this is probably the best approach.
* */

#if defined VISUAL_STUDIO_SAFEINT_COMPAT
namespace msl
{

namespace utilities
{
#endif

#if defined SAFEINT_ASSERT_ON_EXCEPTION
    inline void SafeIntExceptionAssert() SAFEINT_NOTHROW { SAFEINT_ASSERT(false); }
#else
    inline void SafeIntExceptionAssert() SAFEINT_NOTHROW {}
#endif

// Note - removed weak annotation on class due to gcc complaints
// This was the only place in the file that used it, need to better understand 
// whether it was put there correctly in the first place

class SAFEINT_VISIBLE SafeIntException
{
public:
    _CONSTEXPR11 SafeIntException( SafeIntError code = SafeIntError::SafeIntNoError) SAFEINT_NOTHROW  : m_code(code)
    {
    }
    SafeIntError m_code;
};

namespace SafeIntInternal
{
    // Visual Studio version of SafeInt provides for two possible error
    // handlers:
    // SafeIntErrorPolicy_SafeIntException - C++ exception, default if not otherwise defined
    // SafeIntErrorPolicy_InvalidParameter - Calls fail fast (Windows-specific), bypasses any exception handlers, 
    //                                       exits the app with a crash
    template < typename E > class SafeIntExceptionHandler;

#if SAFE_INT_HAS_EXCEPTIONS

    // Some users may have applications that do not use C++ exceptions
    // and cannot compile the following class. If that is the case,
    // either SafeInt_InvalidParameter must be defined as the default,
    // or a custom, user-supplied exception handler must be provided.

    template <> class SafeIntExceptionHandler < SafeIntException >
    {
    public:

        static SAFEINT_NORETURN void SAFEINT_STDCALL SafeIntOnOverflow()
        {
            SafeIntExceptionAssert();
            throw SafeIntException( SafeIntError::SafeIntArithmeticOverflow );
        }

        static SAFEINT_NORETURN void SAFEINT_STDCALL SafeIntOnDivZero()
        {
            SafeIntExceptionAssert();
            throw SafeIntException( SafeIntError::SafeIntDivideByZero );
        }
    };

#endif

   class SafeInt_InvalidParameter
   {
   public:
       static SAFEINT_NORETURN void SafeIntOnOverflow() SAFEINT_NOTHROW
       {
           SafeIntExceptionAssert();
           _CRT_SECURE_INVALID_PARAMETER("SafeInt Arithmetic Overflow");
       }

       static SAFEINT_NORETURN void SafeIntOnDivZero() SAFEINT_NOTHROW
       {
           SafeIntExceptionAssert();
           _CRT_SECURE_INVALID_PARAMETER("SafeInt Divide By Zero");
       }
   };

#if defined _WINDOWS_ 

    class SafeIntWin32ExceptionHandler 
    {
    public:
        static SAFEINT_NORETURN void SAFEINT_STDCALL SafeIntOnOverflow() SAFEINT_NOTHROW
        {
            SafeIntExceptionAssert();
            RaiseException( static_cast<DWORD>(EXCEPTION_INT_OVERFLOW), EXCEPTION_NONCONTINUABLE, 0, 0);
        }

        static SAFEINT_NORETURN void SAFEINT_STDCALL SafeIntOnDivZero() SAFEINT_NOTHROW
        {
            SafeIntExceptionAssert();
            RaiseException( static_cast<DWORD>(EXCEPTION_INT_DIVIDE_BY_ZERO), EXCEPTION_NONCONTINUABLE, 0, 0);
        }
    };

#endif

} // namespace SafeIntInternal

// both of these have cross-platform support
#if SAFE_INT_HAS_EXCEPTIONS
typedef SafeIntInternal::SafeIntExceptionHandler < SafeIntException > CPlusPlusExceptionHandler;
#endif

typedef SafeIntInternal::SafeInt_InvalidParameter InvalidParameterExceptionHandler;

// This exception handler is no longer recommended, but is left here in order not to break existing users
#if defined _WINDOWS_ 
typedef SafeIntInternal::SafeIntWin32ExceptionHandler Win32ExceptionHandler;
#endif

// For Visual Studio compatibility
#if defined VISUAL_STUDIO_SAFEINT_COMPAT 
    typedef CPlusPlusExceptionHandler  SafeIntErrorPolicy_SafeIntException;
    typedef InvalidParameterExceptionHandler SafeIntErrorPolicy_InvalidParameter;
#endif

// If the user hasn't defined a default exception handler,
// define one now, depending on whether they would like Win32 or C++ exceptions

// This library will use conditional noexcept soon, but not in this release
// Some users might mix exception handlers, which is not advised, but is supported
#if !defined SafeIntDefaultExceptionHandler
    #if defined SAFEINT_RAISE_EXCEPTION
        #if !defined _WINDOWS_
        #error Include windows.h in order to use Win32 exceptions
        #endif

        #define SafeIntDefaultExceptionHandler Win32ExceptionHandler
    #elif defined SAFEINT_FAILFAST
        #define SafeIntDefaultExceptionHandler InvalidParameterExceptionHandler
    #else
        #if SAFE_INT_HAS_EXCEPTIONS
            #define SafeIntDefaultExceptionHandler CPlusPlusExceptionHandler
        #else
            #define SafeIntDefaultExceptionHandler InvalidParameterExceptionHandler
        #endif

        #if !defined SAFEINT_EXCEPTION_HANDLER_CPP
        #define SAFEINT_EXCEPTION_HANDLER_CPP 1
        #endif
    #endif
#endif

#if !defined SAFEINT_EXCEPTION_HANDLER_CPP
#define SAFEINT_EXCEPTION_HANDLER_CPP 0
#endif

// If an error handler is chosen other than C++ exceptions, such as Win32 exceptions, fail fast, 
// or abort, then all methods become no throw. Some teams track throw() annotations closely,
// and the following option provides for this.
#if SAFEINT_EXCEPTION_HANDLER_CPP
#define SAFEINT_CPP_THROW
#else
#define SAFEINT_CPP_THROW SAFEINT_NOTHROW
#endif

namespace safeint_internal
{
    // If we have support for std<typetraits>, then we can do this easily, and detect enums as well
    template < typename T > class numeric_type;

    // Continue to special case bool
    template <> class numeric_type<bool> { public: enum { isBool = true, isInt = false }; };
    template < typename T > class numeric_type
    {
    public:
        enum
        {
            isBool = false, // We specialized out a bool  
            // If it is an enum, then consider it an int type
            // This does allow someone to make a SafeInt from an enum type, which is not recommended,
            // but it also allows someone to add an enum value to a SafeInt, which is handy.
            isInt = std::is_integral<T>::value || std::is_enum<T>::value,
            isEnum = std::is_enum<T>::value
        };
    };

    template < typename T > class int_traits
    {
    public:
        static_assert(safeint_internal::numeric_type< T >::isInt, "Integer type required");

        enum
        {
            is64Bit = (sizeof(T) == 8),
            is32Bit = (sizeof(T) == 4),
            is16Bit = (sizeof(T) == 2),
            is8Bit = (sizeof(T) == 1),
            isLT32Bit = (sizeof(T) < 4),
            isLT64Bit = (sizeof(T) < 8),
            isInt8 = (sizeof(T) == 1 && std::numeric_limits<T>::is_signed),
            isUint8 = (sizeof(T) == 1 && !std::numeric_limits<T>::is_signed),
            isInt16 = (sizeof(T) == 2 && std::numeric_limits<T>::is_signed),
            isUint16 = (sizeof(T) == 2 && !std::numeric_limits<T>::is_signed),
            isInt32 = (sizeof(T) == 4 && std::numeric_limits<T>::is_signed),
            isUint32 = (sizeof(T) == 4 && !std::numeric_limits<T>::is_signed),
            isInt64 = (sizeof(T) == 8 && std::numeric_limits<T>::is_signed),
            isUint64 = (sizeof(T) == 8 && !std::numeric_limits<T>::is_signed),
            bitCount = (sizeof(T) * 8),
            isBool = ((T)2 == (T)1)
        };
    };

    template < typename T, typename U > class type_compare
    {
    public:
        enum
        {
            isBothSigned = (std::numeric_limits< T >::is_signed && std::numeric_limits< U >::is_signed),
            isBothUnsigned = (!std::numeric_limits< T >::is_signed && !std::numeric_limits< U >::is_signed),
            isLikeSigned = ((bool)(std::numeric_limits< T >::is_signed) == (bool)(std::numeric_limits< U >::is_signed)),
            isCastOK = ((isLikeSigned && sizeof(T) >= sizeof(U)) ||
            (std::numeric_limits< T >::is_signed && sizeof(T) > sizeof(U))),
            isBothLT32Bit = (safeint_internal::int_traits< T >::isLT32Bit && safeint_internal::int_traits< U >::isLT32Bit),
            isBothLT64Bit = (safeint_internal::int_traits< T >::isLT64Bit && safeint_internal::int_traits< U >::isLT64Bit)
        };
    };

}
//all of the arithmetic operators can be solved by the same code within
//each of these regions without resorting to compile-time constant conditionals
//most operators collapse the problem into less than the 22 zones, but this is used
//as the first cut
//using this also helps ensure that we handle all of the possible cases correctly

template < typename T, typename U > class IntRegion
{
public:
    enum
    {
        //unsigned-unsigned zone
        IntZone_UintLT32_UintLT32 = safeint_internal::type_compare< T,U >::isBothUnsigned && safeint_internal::type_compare< T,U >::isBothLT32Bit,
        IntZone_Uint32_UintLT64   = safeint_internal::type_compare< T,U >::isBothUnsigned && safeint_internal::int_traits< T >::is32Bit && safeint_internal::int_traits< U >::isLT64Bit,
        IntZone_UintLT32_Uint32   = safeint_internal::type_compare< T,U >::isBothUnsigned && safeint_internal::int_traits< T >::isLT32Bit && safeint_internal::int_traits< U >::is32Bit,
        IntZone_Uint64_Uint       = safeint_internal::type_compare< T,U >::isBothUnsigned && safeint_internal::int_traits< T >::is64Bit,
        IntZone_UintLT64_Uint64    = safeint_internal::type_compare< T,U >::isBothUnsigned && safeint_internal::int_traits< T >::isLT64Bit && safeint_internal::int_traits< U >::is64Bit,
        //unsigned-signed
        IntZone_UintLT32_IntLT32  = !std::numeric_limits< T >::is_signed && std::numeric_limits< U >::is_signed && safeint_internal::type_compare< T,U >::isBothLT32Bit,
        IntZone_Uint32_IntLT64    = safeint_internal::int_traits< T >::isUint32 && std::numeric_limits< U >::is_signed && safeint_internal::int_traits< U >::isLT64Bit,
        IntZone_UintLT32_Int32    = !std::numeric_limits< T >::is_signed && safeint_internal::int_traits< T >::isLT32Bit && safeint_internal::int_traits< U >::isInt32,
        IntZone_Uint64_Int        = safeint_internal::int_traits< T >::isUint64 && std::numeric_limits< U >::is_signed && safeint_internal::int_traits< U >::isLT64Bit,
        IntZone_UintLT64_Int64    = !std::numeric_limits< T >::is_signed && safeint_internal::int_traits< T >::isLT64Bit && safeint_internal::int_traits< U >::isInt64,
        IntZone_Uint64_Int64      = safeint_internal::int_traits< T >::isUint64 && safeint_internal::int_traits< U >::isInt64,
        //signed-signed
        IntZone_IntLT32_IntLT32   = safeint_internal::type_compare< T,U >::isBothSigned && safeint_internal::type_compare< T, U >::isBothLT32Bit,
        IntZone_Int32_IntLT64     = safeint_internal::type_compare< T,U >::isBothSigned && safeint_internal::int_traits< T >::is32Bit && safeint_internal::int_traits< U >::isLT64Bit,
        IntZone_IntLT32_Int32     = safeint_internal::type_compare< T,U >::isBothSigned && safeint_internal::int_traits< T >::isLT32Bit && safeint_internal::int_traits< U >::is32Bit,
        IntZone_Int64_Int64       = safeint_internal::type_compare< T,U >::isBothSigned && safeint_internal::int_traits< T >::isInt64 && safeint_internal::int_traits< U >::isInt64,
        IntZone_Int64_Int         = safeint_internal::type_compare< T,U >::isBothSigned && safeint_internal::int_traits< T >::is64Bit && safeint_internal::int_traits< U >::isLT64Bit,
        IntZone_IntLT64_Int64     = safeint_internal::type_compare< T,U >::isBothSigned && safeint_internal::int_traits< T >::isLT64Bit && safeint_internal::int_traits< U >::is64Bit,
        //signed-unsigned
        IntZone_IntLT32_UintLT32  = std::numeric_limits< T >::is_signed && !std::numeric_limits< U >::is_signed && safeint_internal::type_compare< T,U >::isBothLT32Bit,
        IntZone_Int32_UintLT32    = safeint_internal::int_traits< T >::isInt32 && !std::numeric_limits< U >::is_signed && safeint_internal::int_traits< U >::isLT32Bit,
        IntZone_IntLT64_Uint32    = std::numeric_limits< T >::is_signed && safeint_internal::int_traits< T >::isLT64Bit && safeint_internal::int_traits< U >::isUint32,
        IntZone_Int64_UintLT64    = safeint_internal::int_traits< T >::isInt64 && !std::numeric_limits< U >::is_signed && safeint_internal::int_traits< U >::isLT64Bit,
        IntZone_Int_Uint64        = std::numeric_limits< T >::is_signed && safeint_internal::int_traits< U >::isUint64 && safeint_internal::int_traits< T >::isLT64Bit,
        IntZone_Int64_Uint64      = safeint_internal::int_traits< T >::isInt64 && safeint_internal::int_traits< U >::isUint64
    };
};

// In all of the following functions, we have two versions
// One for SafeInt, which throws C++ (or possibly SEH) exceptions
// The non-throwing versions are for use by the helper functions that return success and failure.
// Some of the non-throwing functions are not used, but are maintained for completeness.

// There's no real alternative to duplicating logic, but keeping the two versions
// immediately next to one another will help reduce problems

// useful function to help with getting the magnitude of a negative number
enum AbsMethod
{
    AbsMethodInt,
    AbsMethodInt64,
    AbsMethodNoop
};

template < typename T >
class GetAbsMethod
{
public:
    enum
    {
        method = safeint_internal::int_traits< T >::isLT64Bit && std::numeric_limits< T >::is_signed ? AbsMethodInt :
                 safeint_internal::int_traits< T >::isInt64 ? AbsMethodInt64 : AbsMethodNoop
    };
};

// let's go ahead and hard-code a dependency on the
// representation of negative numbers to keep compilers from getting overly
// happy with optimizing away things like -MIN_INT.
template < typename T, int > class AbsValueHelper;

template < typename T > class AbsValueHelper < T, AbsMethodInt>
{
public:
    _CONSTEXPR14 static std::uint32_t Abs( T t ) SAFEINT_NOTHROW
    {
        SAFEINT_ASSERT( t < 0 );
        return ~(std::uint32_t)t + 1;
    }
};

template < typename T > class AbsValueHelper < T, AbsMethodInt64 >
{
public:
    _CONSTEXPR14 static std::uint64_t Abs( T t ) SAFEINT_NOTHROW
    {
        SAFEINT_ASSERT( t < 0 );
        return ~(std::uint64_t)t + 1;
    }
};

template < typename T > class AbsValueHelper < T, AbsMethodNoop >
{
public:
    _CONSTEXPR14 static T Abs( T t ) SAFEINT_NOTHROW
    {
        // Why are you calling Abs on an unsigned number ???
        SAFEINT_ASSERT( false );
        return t;
    }
};

// Helper classes to work keep compilers from
// optimizing away negation
template < typename T > class SignedNegation;

template <>
class SignedNegation <std::int32_t>
{
public:
    _CONSTEXPR11 static std::int32_t Value(std::uint64_t in) SAFEINT_NOTHROW
    {
        return (std::int32_t)(~(std::uint32_t)in + 1);
    }

    _CONSTEXPR11 static std::int32_t Value(std::uint32_t in) SAFEINT_NOTHROW
    {
        return (std::int32_t)(~in + 1);
    }
};

template <>
class SignedNegation <std::int64_t>
{
public:
    _CONSTEXPR11 static std::int64_t Value(std::uint64_t in) SAFEINT_NOTHROW
    {
        return (std::int64_t)(~in + 1);
    }
};

template < typename T, bool > class NegationHelper;
// Previous versions had an assert that the type being negated was 32-bit or higher
// In retrospect, this seems like something to just document
// Negation will normally upcast to int
// For example -(unsigned short)0xffff == (int)0xffff0001
// This class will retain the type, and will truncate, which may not be what
// you wanted
// If you want normal operator casting behavior, do this:
// SafeInt<unsigned short> ss = 0xffff;
// then:
// -(SafeInt<int>(ss))
// will then emit a signed int with the correct value and bitfield

// Note, unlike all of the other helper classes, the non-throwing negation
// doesn't make sense, isn't exposed or tested, so omit it

template < typename T > class NegationHelper <T, true> // Signed
{
public:
    template <typename E>
    _CONSTEXPR14 static T NegativeThrow( T t ) SAFEINT_CPP_THROW
    {
        // corner case
        if( t != std::numeric_limits<T>::min() )
        {
            // cast prevents unneeded checks in the case of small ints
            return -t;
        }
        E::SafeIntOnOverflow();
    }

    _CONSTEXPR14 static bool Negative(T t, T& out)
    {
        // corner case
        if (t != std::numeric_limits<T>::min())
        {
            out = -t;
            return true;
        }
        return false;
    }
};

template < typename T > class NegationHelper <T, false> // unsigned
{
public:
    template <typename E>
    _CONSTEXPR14 static T NegativeThrow( T t ) SAFEINT_CPP_THROW
    {
#if defined SAFEINT_DISALLOW_UNSIGNED_NEGATION
        static_assert( sizeof(T) == 0, "Unsigned negation is unsupported" );
#endif
        // This may not be the most efficient approach, but you shouldn't be doing this
        return (T)SignedNegation<std::int64_t>::Value(t);
    }

    _CONSTEXPR14 static bool Negative(T , T& /*out*/)
    {
        // This will only be used by the SafeNegation function
        return false;
    }
};

//core logic to determine casting behavior
enum CastMethod
{
    CastOK = 0,
    CastCheckLTZero,
    CastCheckGTMax,
    CastCheckSafeIntMinMaxUnsigned,
    CastCheckSafeIntMinMaxSigned,
    CastToFloat,
    CastFromFloat,
    CastToBool,
    CastFromBool,
    CastFromEnum
};


template < typename ToType, typename FromType >
class GetCastMethod
{
public:
    enum
    {
        method = ( safeint_internal::numeric_type<FromType>::isEnum )                     ? CastFromEnum :
                 ( safeint_internal::int_traits< FromType >::isBool &&
                     !safeint_internal::int_traits< ToType >::isBool )                    ? CastFromBool :

                 ( !safeint_internal::int_traits< FromType >::isBool &&
                     safeint_internal::int_traits< ToType >::isBool )                     ? CastToBool :

                 ( safeint_internal::type_compare< ToType, FromType >::isCastOK )      ? CastOK :

                 ( ( std::numeric_limits< ToType >::is_signed &&
                        !std::numeric_limits< FromType >::is_signed &&
                        sizeof( FromType ) >= sizeof( ToType ) ) ||
                   ( safeint_internal::type_compare< ToType, FromType >::isBothUnsigned &&
                        sizeof( FromType ) > sizeof( ToType ) ) )      ? CastCheckGTMax :

                 ( !std::numeric_limits< ToType >::is_signed &&
                     std::numeric_limits< FromType >::is_signed &&
                     sizeof( ToType ) >= sizeof( FromType ) )          ? CastCheckLTZero :

                 ( !std::numeric_limits< ToType >::is_signed )                    ? CastCheckSafeIntMinMaxUnsigned
                                                                       : CastCheckSafeIntMinMaxSigned
    };
};

template < typename FromType > class GetCastMethod < float, FromType >
{
public:
    enum{ method = CastOK };
};

template < typename FromType > class GetCastMethod < double, FromType >
{
public:
    enum{ method = CastOK };
};

template < typename FromType > class GetCastMethod < long double, FromType >
{
public:
    enum{ method = CastOK };
};

template < typename ToType > class GetCastMethod < ToType, float >
{
public:
    enum{ method = CastFromFloat };
};

template < typename ToType > class GetCastMethod < ToType, double >
{
public:
    enum{ method = CastFromFloat };
};

template < typename ToType > class GetCastMethod < ToType, long double >
{
public:
    enum{ method = CastFromFloat };
};

template < typename T, typename U, int > class SafeCastHelper;

template < typename T, typename U > class SafeCastHelper < T, U, CastOK >
{
public:
    _CONSTEXPR14 static bool Cast( U u, T& t ) SAFEINT_NOTHROW
    {
        t = (T)u;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void CastThrow( U u, T& t ) SAFEINT_CPP_THROW
    {
        t = (T)u;
    }
};

template <typename T, bool> class float_cast_helper;

template <typename T> class float_cast_helper <T, true> // Unsigned case
{
public:
    _CONSTEXPR14 static bool Test(double d)
    {
        const std::uint64_t signifDouble = 0x1fffffffffffff;

        // Anything larger than this either is larger than 2^64-1, or cannot be represented by a double
        const std::uint64_t maxUnsignedDouble = signifDouble << 11;

        // There is the possibility of both negative and positive zero,
        // but we'll allow either, since (-0.0 < 0) == false
        // if we wanted to change that, then use the signbit() macro
        if (d < 0 || d > static_cast<double>(maxUnsignedDouble))
            return false;

        // The input can now safely be cast to an unsigned long long
        if (static_cast<std::uint64_t>(d) > std::numeric_limits<T>::max())
            return false;

        return true;
    }
};

template <typename T> class float_cast_helper <T, false> // Signed case
{
public:
    _CONSTEXPR14 static bool Test(double d)
    {
        const std::uint64_t signifDouble = 0x1fffffffffffff;
        // This has to fit in 2^63-1
        const std::uint64_t maxSignedDouble = signifDouble << 10;
        // The smallest signed long long is easier
        const std::int64_t minSignedDouble = static_cast<std::int64_t>(0x8000000000000000);

        if (d < static_cast<double>(minSignedDouble) || d > static_cast<double>(maxSignedDouble))
            return false;

        // And now cast to long long, and check against min and max for this type
        std::int64_t test = static_cast<std::int64_t>(d);
        if ((std::int64_t)test < (std::int64_t)std::numeric_limits<T>::min() || (std::int64_t)test >(std::int64_t)std::numeric_limits<T>::max())
            return false;

        return true;
    }
};

// special case floats and doubles
template < typename T, typename U > class SafeCastHelper < T, U, CastFromFloat >
{
public:

    static bool CheckFloatingPointCast(double d)
    {
        // A double can hold at most 53 bits of the value
        // 53 bits is:
        bool fValid = false;

        switch (std::fpclassify(d))
        {
        case FP_NORMAL:    // A positive or negative normalized non - zero value
        case FP_SUBNORMAL: // A positive or negative denormalized value
        case FP_ZERO:      // A positive or negative zero value
            fValid = true;
            break;

        case FP_NAN:       // A quiet, signaling, or indeterminate NaN
        case FP_INFINITE:  // A positive or negative infinity
        default:
            fValid = false;
            break;
        }

        if (!fValid)
            return false;

        return float_cast_helper< T, !std::numeric_limits< T >::is_signed >::Test(d);
    }

    static bool Cast( U u, T& t ) SAFEINT_NOTHROW
    {
        if(CheckFloatingPointCast(u))
        {
            t = (T)u;
            return true;
        }
        return false;
    }

    template < typename E >
    static void CastThrow( U u, T& t ) SAFEINT_CPP_THROW
    {
        if (CheckFloatingPointCast(u))
        {
            t = (T)u;
            return;
        }
        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class SafeCastHelper < T, U, CastFromEnum >
{
public:
    _CONSTEXPR14 static bool Cast(U u, T& t) SAFEINT_NOTHROW
    {
        return SafeCastHelper< T, int, GetCastMethod< T, int >::method >::Cast(static_cast<int>(u), t);
    }

    template < typename E >
    _CONSTEXPR14 static void CastThrow(U u, T& t) SAFEINT_CPP_THROW
    {
        SafeCastHelper< T, int, GetCastMethod< T, int >::method >::template CastThrow< E >(static_cast<int>(u), t);
    }
};

// Match on any method where a bool is cast to type T
template < typename T > class SafeCastHelper < T, bool, CastFromBool >
{
public:
    _CONSTEXPR14 static bool Cast( bool b, T& t ) SAFEINT_NOTHROW
    {
        t = (T)( b ? 1 : 0 );
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void CastThrow( bool b, T& t ) SAFEINT_CPP_THROW
    {
        t = (T)( b ? 1 : 0 );
    }
};

template < typename T > class SafeCastHelper < bool, T, CastToBool >
{
public:
    _CONSTEXPR14 static bool Cast( T t, bool& b ) SAFEINT_NOTHROW
    {
        b = !!t;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void CastThrow( bool b, T& t ) SAFEINT_CPP_THROW
    {
        b = !!t;
    }
};

template < typename T, typename U > class SafeCastHelper < T, U, CastCheckLTZero >
{
public:
    _CONSTEXPR14 static bool Cast( U u, T& t ) SAFEINT_NOTHROW
    {
        if( u < 0 )
            return false;

        t = (T)u;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void CastThrow( U u, T& t ) SAFEINT_CPP_THROW
    {
        if( u < 0 )
            E::SafeIntOnOverflow();

        t = (T)u;
    }
};

template < typename T, typename U > class SafeCastHelper < T, U, CastCheckGTMax >
{
public:
    _CONSTEXPR14 static bool Cast( U u, T& t ) SAFEINT_NOTHROW
    {
        if( u > (U)std::numeric_limits<T>::max() )
            return false;

        t = (T)u;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void CastThrow( U u, T& t ) SAFEINT_CPP_THROW
    {
        if( u > (U)std::numeric_limits<T>::max() )
            E::SafeIntOnOverflow();

        t = (T)u;
    }
};

template < typename T, typename U > class SafeCastHelper < T, U, CastCheckSafeIntMinMaxUnsigned >
{
public:
    _CONSTEXPR14 static bool Cast( U u, T& t ) SAFEINT_NOTHROW
    {
        // U is signed - T could be either signed or unsigned
        if( u > std::numeric_limits<T>::max() || u < 0 )
            return false;

        t = (T)u;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void CastThrow( U u, T& t ) SAFEINT_CPP_THROW
    {
        // U is signed - T could be either signed or unsigned
        if( u > std::numeric_limits<T>::max() || u < 0 )
            E::SafeIntOnOverflow();

        t = (T)u;
    }
};

template < typename T, typename U > class SafeCastHelper < T, U, CastCheckSafeIntMinMaxSigned >
{
public:
    _CONSTEXPR14 static bool Cast( U u, T& t ) SAFEINT_NOTHROW
    {
        // T, U are signed
        if( u > std::numeric_limits<T>::max() || u < std::numeric_limits<T>::min() )
            return false;

        t = (T)u;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void CastThrow( U u, T& t ) SAFEINT_CPP_THROW
    {
        //T, U are signed
        if( u > std::numeric_limits<T>::max() || u < std::numeric_limits<T>::min() )
            E::SafeIntOnOverflow();

        t = (T)u;
    }
};

//core logic to determine whether a comparison is valid, or needs special treatment
enum ComparisonMethod
{
    ComparisonMethod_Ok = 0,
    ComparisonMethod_CastInt,
    ComparisonMethod_CastInt64,
    ComparisonMethod_UnsignedT,
    ComparisonMethod_UnsignedU
};

    // Note - the standard is arguably broken in the case of some integer
    // conversion operations
    // For example, signed char a = -1 = 0xff
    //              unsigned int b = 0xffffffff
    // If you then test if a < b, a value-preserving cast
    // is made, and you're essentially testing
    // (unsigned int)a < b == false
    //
    // I do not think this makes sense - if you perform
    // a cast to an std::int64_t, which can clearly preserve both value and signedness
    // then you get a different and intuitively correct answer
    // IMHO, -1 should be less than 4 billion
    // If you prefer to retain the ANSI standard behavior
    // insert #define ANSI_CONVERSIONS into your source
    // Behavior differences occur in the following cases:
    // 8, 16, and 32-bit signed int, unsigned 32-bit int
    // any signed int, unsigned 64-bit int
    // Note - the signed int must be negative to show the problem

template < typename T, typename U >
class ValidComparison
{
public:
    enum
    {
        method = ( ( safeint_internal::type_compare< T, U >::isLikeSigned )                              ? ComparisonMethod_Ok :
                 ( ( std::numeric_limits< T >::is_signed && sizeof(T) < 8 && sizeof(U) < 4 ) ||
                   ( std::numeric_limits< U >::is_signed && sizeof(T) < 4 && sizeof(U) < 8 ) )  ? ComparisonMethod_CastInt :
                 ( ( std::numeric_limits< T >::is_signed && sizeof(U) < 8 ) ||
                   ( std::numeric_limits< U >::is_signed && sizeof(T) < 8 ) )                   ? ComparisonMethod_CastInt64 :
                 ( !std::numeric_limits< T >::is_signed )                                       ? ComparisonMethod_UnsignedT :
                                                                                       ComparisonMethod_UnsignedU )
    };
};

template <typename T, typename U, int state> class EqualityTest;

template < typename T, typename U > class EqualityTest< T, U, ComparisonMethod_Ok >
{
public:
    _CONSTEXPR11 static bool IsEquals( const T t, const U u ) SAFEINT_NOTHROW { return ( t == u ); }
};

template < typename T, typename U > class EqualityTest< T, U, ComparisonMethod_CastInt >
{
public:
    _CONSTEXPR11 static bool IsEquals( const T t, const U u ) SAFEINT_NOTHROW { return ( (int)t == (int)u ); }
};

template < typename T, typename U > class EqualityTest< T, U, ComparisonMethod_CastInt64 >
{
public:
    _CONSTEXPR11 static bool IsEquals( const T t, const U u ) SAFEINT_NOTHROW { return ( (std::int64_t)t == (std::int64_t)u ); }
};

template < typename T, typename U > class EqualityTest< T, U, ComparisonMethod_UnsignedT >
{
public:
    _CONSTEXPR14 static bool IsEquals( const T t, const U u ) SAFEINT_NOTHROW
    {
        //one operand is 32 or 64-bit unsigned, and the other is signed and the same size or smaller
        if( u < 0 )
            return false;

        //else safe to cast to type T
        return ( t == (T)u );
    }
};

template < typename T, typename U > class EqualityTest< T, U, ComparisonMethod_UnsignedU>
{
public:
    _CONSTEXPR14 static bool IsEquals( const T t, const U u ) SAFEINT_NOTHROW
    {
        //one operand is 32 or 64-bit unsigned, and the other is signed and the same size or smaller
        if( t < 0 )
            return false;

        //else safe to cast to type U
        return ( (U)t == u );
    }
};

template <typename T, typename U, int state> class GreaterThanTest;

template < typename T, typename U > class GreaterThanTest< T, U, ComparisonMethod_Ok >
{
public:
    _CONSTEXPR11 static bool GreaterThan( const T t, const U u ) SAFEINT_NOTHROW { return ( t > u ); }
};

template < typename T, typename U > class GreaterThanTest< T, U, ComparisonMethod_CastInt >
{
public:
    _CONSTEXPR11 static bool GreaterThan( const T t, const U u ) SAFEINT_NOTHROW { return ( (int)t > (int)u ); }
};

template < typename T, typename U > class GreaterThanTest< T, U, ComparisonMethod_CastInt64 >
{
public:
    _CONSTEXPR11 static bool GreaterThan( const T t, const U u ) SAFEINT_NOTHROW { return ( (std::int64_t)t > (std::int64_t)u ); }
};

template < typename T, typename U > class GreaterThanTest< T, U, ComparisonMethod_UnsignedT >
{
public:
    _CONSTEXPR14 static bool GreaterThan( const T t, const U u ) SAFEINT_NOTHROW
    {
        // one operand is 32 or 64-bit unsigned, and the other is signed and the same size or smaller
        if( u < 0 )
            return true;

        // else safe to cast to type T
        return ( t > (T)u );
    }
};

template < typename T, typename U > class GreaterThanTest< T, U, ComparisonMethod_UnsignedU >
{
public:
    _CONSTEXPR14 static bool GreaterThan( const T t, const U u ) SAFEINT_NOTHROW
    {
        // one operand is 32 or 64-bit unsigned, and the other is signed and the same size or smaller
        if( t < 0 )
            return false;

        // else safe to cast to type U
        return ( (U)t > u );
    }
};

// Modulus is simpler than comparison, but follows much the same logic
// using this set of functions, it can't fail except in a div 0 situation
template <typename T, typename U, int method > class ModulusHelper;

template <typename U, bool> class mod_corner_case;

template <typename U> class mod_corner_case <U, true> // signed
{
public:
    _CONSTEXPR14 static bool is_undefined(U u)
    {
        return (u == -1);
    }
};

template <typename U> class mod_corner_case <U, false> // unsigned
{
public:
    _CONSTEXPR14 static bool is_undefined(U)
    {
        return false;
    }
};

template <typename T, typename U> class ModulusHelper <T, U, ComparisonMethod_Ok>
{
public:
    _CONSTEXPR14 static SafeIntError Modulus( const T& t, const U& u, T& result ) SAFEINT_NOTHROW
    {
        if(u == 0)
            return SafeIntError::SafeIntDivideByZero;

        //trap corner case
        if(mod_corner_case<U, std::numeric_limits< U >::is_signed >::is_undefined(u))
        {
            result = 0;
            return SafeIntError::SafeIntNoError;
        }

        result = (T)(t % u);
        return SafeIntError::SafeIntNoError;
    }

    template < typename E >
    _CONSTEXPR14 static void ModulusThrow( const T& t, const U& u, T& result ) SAFEINT_CPP_THROW
    {
        if(u == 0)
            E::SafeIntOnDivZero();

        //trap corner case
        if (mod_corner_case<U, std::numeric_limits< U >::is_signed >::is_undefined(u))
        {
            result = 0;
            return;
        }

        result = (T)(t % u);
    }
};

template <typename T, typename U> class ModulusHelper <T, U, ComparisonMethod_CastInt>
{
public:
    _CONSTEXPR14 static SafeIntError Modulus( const T& t, const U& u, T& result ) SAFEINT_NOTHROW
    {
        if(u == 0)
            return SafeIntError::SafeIntDivideByZero;

        //trap corner case
        if (mod_corner_case<U, std::numeric_limits< U >::is_signed >::is_undefined(u))
        {
            result = 0;
            return SafeIntError::SafeIntNoError;
        }

        result = (T)(t % u);
        return SafeIntError::SafeIntNoError;
    }

    template < typename E >
    _CONSTEXPR14 static void ModulusThrow( const T& t, const U& u, T& result ) SAFEINT_CPP_THROW
    {
        if(u == 0)
            E::SafeIntOnDivZero();

        //trap corner case
        if (mod_corner_case<U, std::numeric_limits< U >::is_signed >::is_undefined(u))
        {
            result = 0;
            return;
        }

        result = (T)(t % u);
    }
};

template < typename T, typename U > class ModulusHelper< T, U, ComparisonMethod_CastInt64>
{
public:
    _CONSTEXPR14 static SafeIntError Modulus( const T& t, const U& u, T& result ) SAFEINT_NOTHROW
    {
        if(u == 0)
            return SafeIntError::SafeIntDivideByZero;

        //trap corner case
        if (mod_corner_case<U, std::numeric_limits< U >::is_signed >::is_undefined(u))
        {
            result = 0;
            return SafeIntError::SafeIntNoError;
        }

        result = (T)((std::int64_t)t % (std::int64_t)u);
        return SafeIntError::SafeIntNoError;
    }

    template < typename E >
    _CONSTEXPR14 static void ModulusThrow( const T& t, const U& u, T& result ) SAFEINT_CPP_THROW
    {
        if(u == 0)
            E::SafeIntOnDivZero();

        if (mod_corner_case<U, std::numeric_limits< U >::is_signed >::is_undefined(u))
        {
            result = 0;
            return;
        }

        result = (T)((std::int64_t)t % (std::int64_t)u);
    }
};

// T is std::uint64_t, U is any signed int
template < typename T, typename U > class ModulusHelper< T, U, ComparisonMethod_UnsignedT>
{
public:
    _CONSTEXPR14 static SafeIntError Modulus( const T& t, const U& u, T& result ) SAFEINT_NOTHROW
    {
        if(u == 0)
            return SafeIntError::SafeIntDivideByZero;

        // u could be negative - if so, need to convert to positive
        // casts below are always safe due to the way modulus works
        if(u < 0)
            result = (T)(t % AbsValueHelper< U, GetAbsMethod< U >::method >::Abs(u));
        else
            result = (T)(t % u);

        return SafeIntError::SafeIntNoError;
    }

    template < typename E >
    _CONSTEXPR14 static void ModulusThrow( const T& t, const U& u, T& result ) SAFEINT_CPP_THROW
    {
        if(u == 0)
            E::SafeIntOnDivZero();

        // u could be negative - if so, need to convert to positive
        if(u < 0)
            result = (T)(t % AbsValueHelper< U, GetAbsMethod< U >::method >::Abs( u ));
        else
            result = (T)(t % u);
    }
};

// U is std::uint64_t, T any signed int
template < typename T, typename U > class ModulusHelper< T, U, ComparisonMethod_UnsignedU>
{
public:
    _CONSTEXPR14 static SafeIntError Modulus( const T& t, const U& u, T& result ) SAFEINT_NOTHROW
    {
        if(u == 0)
            return SafeIntError::SafeIntDivideByZero;

        //t could be negative - if so, need to convert to positive
        if(t < 0)
            result = (T)( ~( AbsValueHelper< T, GetAbsMethod< T >::method >::Abs( t ) % u ) + 1 );
        else
            result = (T)((T)t % u);

        return SafeIntError::SafeIntNoError;
    }

    template < typename E >
    _CONSTEXPR14 static void ModulusThrow( const T& t, const U& u, T& result ) SAFEINT_CPP_THROW
    {
        if(u == 0)
            E::SafeIntOnDivZero();

        //t could be negative - if so, need to convert to positive
        if(t < 0)
            result = (T)( ~( AbsValueHelper< T, GetAbsMethod< T >::method >::Abs( t ) % u ) + 1);
        else
            result = (T)( (T)t % u );
    }
};

//core logic to determine method to check multiplication
enum MultiplicationState
{
    MultiplicationState_CastInt = 0,  // One or both signed, smaller than 32-bit
    MultiplicationState_CastInt64,    // One or both signed, smaller than 64-bit
    MultiplicationState_CastUint,     // Both are unsigned, smaller than 32-bit
    MultiplicationState_CastUint64,   // Both are unsigned, both 32-bit or smaller
    MultiplicationState_Uint64Uint,   // Both are unsigned, lhs 64-bit, rhs 32-bit or smaller
    MultiplicationState_Uint64Uint64, // Both are unsigned int64
    MultiplicationState_Uint64Int,    // lhs is unsigned int64, rhs int32
    MultiplicationState_Uint64Int64,  // lhs is unsigned int64, rhs signed int64
    MultiplicationState_UintUint64,   // Both are unsigned, lhs 32-bit or smaller, rhs 64-bit
    MultiplicationState_UintInt64,    // lhs unsigned 32-bit or less, rhs int64
    MultiplicationState_Int64Uint,    // lhs int64, rhs unsigned int32
    MultiplicationState_Int64Int64,   // lhs int64, rhs int64
    MultiplicationState_Int64Int,     // lhs int64, rhs int32
    MultiplicationState_IntUint64,    // lhs int, rhs unsigned int64
    MultiplicationState_IntInt64,     // lhs int, rhs int64
    MultiplicationState_Int64Uint64,  // lhs int64, rhs uint64
    MultiplicationState_Error
};

template < typename T, typename U >
class MultiplicationMethod
{
public:
    enum
    {
                 // unsigned-unsigned
        method = (IntRegion< T,U >::IntZone_UintLT32_UintLT32  ? MultiplicationState_CastUint :
                 (IntRegion< T,U >::IntZone_Uint32_UintLT64 ||
                  IntRegion< T,U >::IntZone_UintLT32_Uint32)   ? MultiplicationState_CastUint64 :
                  safeint_internal::type_compare< T,U >::isBothUnsigned &&
                  safeint_internal::int_traits< T >::isUint64 && safeint_internal::int_traits< U >::isUint64 ? MultiplicationState_Uint64Uint64 :
                 (IntRegion< T,U >::IntZone_Uint64_Uint)       ? MultiplicationState_Uint64Uint :
                 (IntRegion< T,U >::IntZone_UintLT64_Uint64)   ? MultiplicationState_UintUint64 :
                 // unsigned-signed
                 (IntRegion< T,U >::IntZone_UintLT32_IntLT32)  ? MultiplicationState_CastInt :
                 (IntRegion< T,U >::IntZone_Uint32_IntLT64 ||
                  IntRegion< T,U >::IntZone_UintLT32_Int32)    ? MultiplicationState_CastInt64 :
                 (IntRegion< T,U >::IntZone_Uint64_Int)        ? MultiplicationState_Uint64Int :
                 (IntRegion< T,U >::IntZone_UintLT64_Int64)    ? MultiplicationState_UintInt64 :
                 (IntRegion< T,U >::IntZone_Uint64_Int64)      ? MultiplicationState_Uint64Int64 :
                 // signed-signed
                 (IntRegion< T,U >::IntZone_IntLT32_IntLT32)   ? MultiplicationState_CastInt :
                 (IntRegion< T,U >::IntZone_Int32_IntLT64 ||
                  IntRegion< T,U >::IntZone_IntLT32_Int32)     ? MultiplicationState_CastInt64 :
                 (IntRegion< T,U >::IntZone_Int64_Int64)       ? MultiplicationState_Int64Int64 :
                 (IntRegion< T,U >::IntZone_Int64_Int)         ? MultiplicationState_Int64Int :
                 (IntRegion< T,U >::IntZone_IntLT64_Int64)     ? MultiplicationState_IntInt64 :
                 // signed-unsigned
                 (IntRegion< T,U >::IntZone_IntLT32_UintLT32)  ? MultiplicationState_CastInt :
                 (IntRegion< T,U >::IntZone_Int32_UintLT32 ||
                  IntRegion< T,U >::IntZone_IntLT64_Uint32)    ? MultiplicationState_CastInt64 :
                 (IntRegion< T,U >::IntZone_Int64_UintLT64)    ? MultiplicationState_Int64Uint :
                 (IntRegion< T,U >::IntZone_Int_Uint64)        ? MultiplicationState_IntUint64 :
                 (IntRegion< T,U >::IntZone_Int64_Uint64       ? MultiplicationState_Int64Uint64 :
                  MultiplicationState_Error ) )
    };
};

template <typename T, typename U, int state> class MultiplicationHelper;

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_CastInt>
{
public:
    //accepts signed, both less than 32-bit
    _CONSTEXPR14 static bool Multiply( const T& t, const U& u, T& ret ) SAFEINT_NOTHROW
    {
        int tmp = t * u;

        if( tmp > std::numeric_limits<T>::max() || tmp < std::numeric_limits<T>::min() )
            return false;

        ret = (T)tmp;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void MultiplyThrow( const T& t, const U& u, T& ret ) SAFEINT_CPP_THROW
    {
        int tmp = t * u;

        if( tmp > std::numeric_limits<T>::max() || tmp < std::numeric_limits<T>::min() )
            E::SafeIntOnOverflow();

        ret = (T)tmp;
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_CastUint >
{
public:
    //accepts unsigned, both less than 32-bit
    _CONSTEXPR14 static bool Multiply( const T& t, const U& u, T& ret ) SAFEINT_NOTHROW
    {
        unsigned int tmp = (unsigned int)t * (unsigned int)u;

        if( tmp > std::numeric_limits<T>::max() )
            return false;

        ret = (T)tmp;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void MultiplyThrow( const T& t, const U& u, T& ret ) SAFEINT_CPP_THROW
    {
        unsigned int tmp = (unsigned int)( t * u );

        if( tmp > std::numeric_limits<T>::max() )
            E::SafeIntOnOverflow();

        ret = (T)tmp;
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_CastInt64>
{
public:
    //mixed signed or both signed where at least one argument is 32-bit, and both a 32-bit or less
    _CONSTEXPR14 static bool Multiply( const T& t, const U& u, T& ret ) SAFEINT_NOTHROW
    {
        std::int64_t tmp = (std::int64_t)t * (std::int64_t)u;

        if(tmp > (std::int64_t)std::numeric_limits<T>::max() || tmp < (std::int64_t)std::numeric_limits<T>::min())
            return false;

        ret = (T)tmp;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void MultiplyThrow( const T& t, const U& u, T& ret ) SAFEINT_CPP_THROW
    {
        std::int64_t tmp = (std::int64_t)t * (std::int64_t)u;

        if(tmp > (std::int64_t)std::numeric_limits<T>::max() || tmp < (std::int64_t)std::numeric_limits<T>::min())
            E::SafeIntOnOverflow();

        ret = (T)tmp;
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_CastUint64>
{
public:
    //both unsigned where at least one argument is 32-bit, and both are 32-bit or less
    _CONSTEXPR14 static bool Multiply( const T& t, const U& u, T& ret ) SAFEINT_NOTHROW
    {
        std::uint64_t tmp = (std::uint64_t)t * (std::uint64_t)u;

        if(tmp > (std::uint64_t)std::numeric_limits<T>::max())
            return false;

        ret = (T)tmp;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void MultiplyThrow( const T& t, const U& u, T& ret ) SAFEINT_CPP_THROW
    {
        std::uint64_t tmp = (std::uint64_t)t * (std::uint64_t)u;

        if(tmp > (std::uint64_t)std::numeric_limits<T>::max())
            E::SafeIntOnOverflow();

        ret = (T)tmp;
    }
};

// T = left arg and return type
// U = right arg
template < typename T, typename U > class LargeIntRegMultiply;

#if SAFEINT_USE_INTRINSICS
// As usual, unsigned is easy
inline bool IntrinsicMultiplyUint64( const std::uint64_t& a, const std::uint64_t& b, std::uint64_t* pRet ) SAFEINT_NOTHROW
{
    std::uint64_t ulHigh = 0;
    *pRet = _umul128(a , b, &ulHigh);
    return ulHigh == 0;
}

// Signed, is not so easy
inline bool IntrinsicMultiplyInt64( const std::int64_t& a, const std::int64_t& b, std::int64_t* pRet ) SAFEINT_NOTHROW
{
    std::int64_t llHigh = 0;
    *pRet = _mul128(a , b, &llHigh);

    // Now we need to figure out what we expect
    // If llHigh is 0, then treat *pRet as unsigned
    // If llHigh is < 0, then treat *pRet as signed

    if( (a ^ b) < 0 )
    {
        // Negative result expected
        if( llHigh == -1 && *pRet < 0 ||
            llHigh == 0 && *pRet == 0 )
        {
            // Everything is within range
            return true;
        }
    }
    else
    {
        // Result should be positive
        // Check for overflow
        if( llHigh == 0 && (std::uint64_t)*pRet <= (std::uint64_t)std::numeric_limits<std::int64_t>::max() )
            return true;
    }
    return false;
}

#endif

template<> class LargeIntRegMultiply< std::uint64_t, std::uint64_t >
{
public:
    _CONSTEXPR14_MULTIPLY static bool RegMultiply( const std::uint64_t& a, const std::uint64_t& b, std::uint64_t* pRet ) SAFEINT_NOTHROW
    {
#if SAFEINT_USE_INTRINSICS
        return IntrinsicMultiplyUint64( a, b, pRet );
#else
        std::uint32_t aHigh = 0, aLow = 0, bHigh = 0, bLow = 0;

        // Consider that a*b can be broken up into:
        // (aHigh * 2^32 + aLow) * (bHigh * 2^32 + bLow)
        // => (aHigh * bHigh * 2^64) + (aLow * bHigh * 2^32) + (aHigh * bLow * 2^32) + (aLow * bLow)
        // Note - same approach applies for 128 bit math on a 64-bit system

        aHigh = (std::uint32_t)(a >> 32);
        aLow  = (std::uint32_t)a;
        bHigh = (std::uint32_t)(b >> 32);
        bLow  = (std::uint32_t)b;

        *pRet = 0;

        if(aHigh == 0)
        {
            if(bHigh != 0)
            {
                *pRet = (std::uint64_t)aLow * (std::uint64_t)bHigh;
            }
        }
        else if(bHigh == 0)
        {
            if(aHigh != 0)
            {
                *pRet = (std::uint64_t)aHigh * (std::uint64_t)bLow;
            }
        }
        else
        {
            return false;
        }

        if(*pRet != 0)
        {
            std::uint64_t tmp = 0;

            if((std::uint32_t)(*pRet >> 32) != 0)
                return false;

            *pRet <<= 32;
            tmp = (std::uint64_t)aLow * (std::uint64_t)bLow;
            *pRet += tmp;

            if(*pRet < tmp)
                return false;

            return true;
        }

        *pRet = (std::uint64_t)aLow * (std::uint64_t)bLow;
        return true;
#endif
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void RegMultiplyThrow( const std::uint64_t& a, const std::uint64_t& b, std::uint64_t* pRet ) SAFEINT_CPP_THROW
    {
#if SAFEINT_USE_INTRINSICS
        if( !IntrinsicMultiplyUint64( a, b, pRet ) )
            E::SafeIntOnOverflow();
#else
        std::uint32_t aHigh = 0, aLow = 0, bHigh = 0, bLow = 0;

        // Consider that a*b can be broken up into:
        // (aHigh * 2^32 + aLow) * (bHigh * 2^32 + bLow)
        // => (aHigh * bHigh * 2^64) + (aLow * bHigh * 2^32) + (aHigh * bLow * 2^32) + (aLow * bLow)
        // Note - same approach applies for 128 bit math on a 64-bit system

        aHigh = (std::uint32_t)(a >> 32);
        aLow  = (std::uint32_t)a;
        bHigh = (std::uint32_t)(b >> 32);
        bLow  = (std::uint32_t)b;

        *pRet = 0;

        if(aHigh == 0)
        {
            if(bHigh != 0)
            {
                *pRet = (std::uint64_t)aLow * (std::uint64_t)bHigh;
            }
        }
        else if(bHigh == 0)
        {
            if(aHigh != 0)
            {
                *pRet = (std::uint64_t)aHigh * (std::uint64_t)bLow;
            }
        }
        else
        {
            E::SafeIntOnOverflow();
        }

        if(*pRet != 0)
        {
            std::uint64_t tmp = 0;

            if((std::uint32_t)(*pRet >> 32) != 0)
                E::SafeIntOnOverflow();

            *pRet <<= 32;
            tmp = (std::uint64_t)aLow * (std::uint64_t)bLow;
            *pRet += tmp;

            if(*pRet < tmp)
                E::SafeIntOnOverflow();

            return;
        }

        *pRet = (std::uint64_t)aLow * (std::uint64_t)bLow;
#endif
    }
};

template<> class LargeIntRegMultiply< std::uint64_t, std::uint32_t >
{
public:
    _CONSTEXPR14_MULTIPLY static bool RegMultiply( const std::uint64_t& a, std::uint32_t b, std::uint64_t* pRet ) SAFEINT_NOTHROW
    {
#if SAFEINT_USE_INTRINSICS
        return IntrinsicMultiplyUint64( a, (std::uint64_t)b, pRet );
#else
        std::uint32_t aHigh = 0, aLow = 0;

        // Consider that a*b can be broken up into:
        // (aHigh * 2^32 + aLow) * b
        // => (aHigh * b * 2^32) + (aLow * b)

        aHigh = (std::uint32_t)(a >> 32);
        aLow  = (std::uint32_t)a;

        *pRet = 0;

        if(aHigh != 0)
        {
            *pRet = (std::uint64_t)aHigh * (std::uint64_t)b;

            std::uint64_t tmp = 0;

            if((std::uint32_t)(*pRet >> 32) != 0)
                return false;

            *pRet <<= 32;
            tmp = (std::uint64_t)aLow * (std::uint64_t)b;
            *pRet += tmp;

            if(*pRet < tmp)
                return false;

            return true;
        }

        *pRet = (std::uint64_t)aLow * (std::uint64_t)b;
        return true;
#endif
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void RegMultiplyThrow( const std::uint64_t& a, std::uint32_t b, std::uint64_t* pRet ) SAFEINT_CPP_THROW
    {
#if SAFEINT_USE_INTRINSICS
        if( !IntrinsicMultiplyUint64( a, (std::uint64_t)b, pRet ) )
            E::SafeIntOnOverflow();
#else
        std::uint32_t aHigh = 0, aLow = 0;

        // Consider that a*b can be broken up into:
        // (aHigh * 2^32 + aLow) * b
        // => (aHigh * b * 2^32) + (aLow * b)

        aHigh = (std::uint32_t)(a >> 32);
        aLow  = (std::uint32_t)a;

        *pRet = 0;

        if(aHigh != 0)
        {
            *pRet = (std::uint64_t)aHigh * (std::uint64_t)b;

            std::uint64_t tmp = 0;

            if((std::uint32_t)(*pRet >> 32) != 0)
                E::SafeIntOnOverflow();

            *pRet <<= 32;
            tmp = (std::uint64_t)aLow * (std::uint64_t)b;
            *pRet += tmp;

            if(*pRet < tmp)
                E::SafeIntOnOverflow();

            return;
        }

        *pRet = (std::uint64_t)aLow * (std::uint64_t)b;
        return;
#endif
    }
};

template<> class LargeIntRegMultiply< std::uint64_t, std::int32_t >
{
public:
    // Intrinsic not needed
    _CONSTEXPR14_MULTIPLY static bool RegMultiply( const std::uint64_t& a, std::int32_t b, std::uint64_t* pRet ) SAFEINT_NOTHROW
    {
        if( b < 0 && a != 0 )
            return false;

#if SAFEINT_USE_INTRINSICS
        return IntrinsicMultiplyUint64( a, (std::uint64_t)b, pRet );
#else
        return LargeIntRegMultiply< std::uint64_t, std::uint32_t >::RegMultiply(a, (std::uint32_t)b, pRet);
#endif
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void RegMultiplyThrow( const std::uint64_t& a, std::int32_t b, std::uint64_t* pRet ) SAFEINT_CPP_THROW
    {
        if( b < 0 && a != 0 )
            E::SafeIntOnOverflow();

#if SAFEINT_USE_INTRINSICS
        if( !IntrinsicMultiplyUint64( a, (std::uint64_t)b, pRet ) )
            E::SafeIntOnOverflow();
#else
        LargeIntRegMultiply< std::uint64_t, std::uint32_t >::template RegMultiplyThrow< E >( a, (std::uint32_t)b, pRet );
#endif
    }
};

template<> class LargeIntRegMultiply< std::uint64_t, std::int64_t >
{
public:
    _CONSTEXPR14_MULTIPLY static bool RegMultiply( const std::uint64_t& a, std::int64_t b, std::uint64_t* pRet ) SAFEINT_NOTHROW
    {
        if( b < 0 && a != 0 )
            return false;

#if SAFEINT_USE_INTRINSICS
        return IntrinsicMultiplyUint64( a, (std::uint64_t)b, pRet );
#else
        return LargeIntRegMultiply< std::uint64_t, std::uint64_t >::RegMultiply(a, (std::uint64_t)b, pRet);
#endif
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void RegMultiplyThrow( const std::uint64_t& a, std::int64_t b, std::uint64_t* pRet ) SAFEINT_CPP_THROW
    {
        if( b < 0 && a != 0 )
            E::SafeIntOnOverflow();

#if SAFEINT_USE_INTRINSICS
        if( !IntrinsicMultiplyUint64( a, (std::uint64_t)b, pRet ) )
            E::SafeIntOnOverflow();
#else
        LargeIntRegMultiply< std::uint64_t, std::uint64_t >::template RegMultiplyThrow< E >( a, (std::uint64_t)b, pRet );
#endif
    }
};

template<> class LargeIntRegMultiply< std::int32_t, std::uint64_t >
{
public:
    // Devolves into ordinary 64-bit calculation
    _CONSTEXPR14 static bool RegMultiply( std::int32_t a, const std::uint64_t& b, std::int32_t* pRet ) SAFEINT_NOTHROW
    {
        std::uint32_t bHigh = 0, bLow = 0;
        bool fIsNegative = false;

        // Consider that a*b can be broken up into:
        // (aHigh * 2^32 + aLow) * (bHigh * 2^32 + bLow)
        // => (aHigh * bHigh * 2^64) + (aLow * bHigh * 2^32) + (aHigh * bLow * 2^32) + (aLow * bLow)
        // aHigh == 0 implies:
        // ( aLow * bHigh * 2^32 ) + ( aLow + bLow )
        // If the first part is != 0, fail

        bHigh = (std::uint32_t)(b >> 32);
        bLow  = (std::uint32_t)b;

        *pRet = 0;

        if(bHigh != 0 && a != 0)
            return false;

        if( a < 0 )
        {

            a = (std::int32_t)AbsValueHelper< std::int32_t, GetAbsMethod< std::int32_t >::method >::Abs(a);
            fIsNegative = true;
        }

        std::uint64_t tmp = (std::uint32_t)a * (std::uint64_t)bLow;

        if( !fIsNegative )
        {
            if( tmp <= (std::uint64_t)std::numeric_limits< std::int32_t >::max() )
            {
                *pRet = (std::int32_t)tmp;
                return true;
            }
        }
        else
        {
            if( tmp <= (std::uint64_t)std::numeric_limits< std::int32_t >::max()+1 )
            {
                *pRet = SignedNegation< std::int32_t >::Value( tmp );
                return true;
            }
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void RegMultiplyThrow( std::int32_t a, const std::uint64_t& b, std::int32_t* pRet ) SAFEINT_CPP_THROW
    {
        std::uint32_t bHigh = 0, bLow = 0;
        bool fIsNegative = false;

        // Consider that a*b can be broken up into:
        // (aHigh * 2^32 + aLow) * (bHigh * 2^32 + bLow)
        // => (aHigh * bHigh * 2^64) + (aLow * bHigh * 2^32) + (aHigh * bLow * 2^32) + (aLow * bLow)

        bHigh = (std::uint32_t)(b >> 32);
        bLow  = (std::uint32_t)b;

        *pRet = 0;

        if(bHigh != 0 && a != 0)
            E::SafeIntOnOverflow();

        if( a < 0 )
        {
            a = (std::int32_t)AbsValueHelper< std::int32_t, GetAbsMethod< std::int32_t >::method >::Abs(a);
            fIsNegative = true;
        }

        std::uint64_t tmp = (std::uint32_t)a * (std::uint64_t)bLow;

        if( !fIsNegative )
        {
            if( tmp <= (std::uint64_t)std::numeric_limits< std::int32_t >::max() )
            {
                *pRet = (std::int32_t)tmp;
                return;
            }
        }
        else
        {
            if( tmp <= (std::uint64_t)std::numeric_limits< std::int32_t >::max()+1 )
            {
                *pRet = SignedNegation< std::int32_t >::Value( tmp );
                return;
            }
        }

        E::SafeIntOnOverflow();
    }
};

template<> class LargeIntRegMultiply< std::uint32_t, std::uint64_t >
{
public:
    // Becomes ordinary 64-bit multiplication, intrinsic not needed
    _CONSTEXPR14 static bool RegMultiply( std::uint32_t a, const std::uint64_t& b, std::uint32_t* pRet ) SAFEINT_NOTHROW
    {
        // Consider that a*b can be broken up into:
        // (bHigh * 2^32 + bLow) * a
        // => (bHigh * a * 2^32) + (bLow * a)
        // In this case, the result must fit into 32-bits
        // If bHigh != 0 && a != 0, immediate error.

        if( (std::uint32_t)(b >> 32) != 0 && a != 0 )
            return false;

        std::uint64_t tmp = b * (std::uint64_t)a;

        if( (std::uint32_t)(tmp >> 32) != 0 ) // overflow
            return false;

        *pRet = (std::uint32_t)tmp;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void RegMultiplyThrow( std::uint32_t a, const std::uint64_t& b, std::uint32_t* pRet ) SAFEINT_CPP_THROW
    {
        if( (std::uint32_t)(b >> 32) != 0 && a != 0 )
            E::SafeIntOnOverflow();

        std::uint64_t tmp = b * (std::uint64_t)a;

        if( (std::uint32_t)(tmp >> 32) != 0 ) // overflow
            E::SafeIntOnOverflow();

        *pRet = (std::uint32_t)tmp;
    }
};

template<> class LargeIntRegMultiply< std::uint32_t, std::int64_t >
{
public:
    _CONSTEXPR14 static bool RegMultiply( std::uint32_t a, const std::int64_t& b, std::uint32_t* pRet ) SAFEINT_NOTHROW
    {
        if( b < 0 && a != 0 )
            return false;
        return LargeIntRegMultiply< std::uint32_t, std::uint64_t >::RegMultiply( a, (std::uint64_t)b, pRet );
    }

    template < typename E >
    _CONSTEXPR14 static void RegMultiplyThrow( std::uint32_t a, const std::int64_t& b, std::uint32_t* pRet ) SAFEINT_CPP_THROW
    {
        if( b < 0 && a != 0 )
            E::SafeIntOnOverflow();

        LargeIntRegMultiply< std::uint32_t, std::uint64_t >::template RegMultiplyThrow< E >( a, (std::uint64_t)b, pRet );
    }
};

template<> class LargeIntRegMultiply< std::int64_t, std::int64_t >
{
public:
    _CONSTEXPR14_MULTIPLY static bool RegMultiply( const std::int64_t& a, const std::int64_t& b, std::int64_t* pRet ) SAFEINT_NOTHROW
    {
#if SAFEINT_USE_INTRINSICS
        return IntrinsicMultiplyInt64( a, b, pRet );
#else
        bool aNegative = false;
        bool bNegative = false;

        std::uint64_t tmp = 0;
        std::int64_t a1 = a;
        std::int64_t b1 = b;

        if( a1 < 0 )
        {
            aNegative = true;
            a1 = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(a1);
        }

        if( b1 < 0 )
        {
            bNegative = true;
            b1 = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(b1);
        }

        if( LargeIntRegMultiply< std::uint64_t, std::uint64_t >::RegMultiply( (std::uint64_t)a1, (std::uint64_t)b1, &tmp ) )
        {
            // The unsigned multiplication didn't overflow
            if( aNegative ^ bNegative )
            {
                // Result must be negative
                if( tmp <= (std::uint64_t)std::numeric_limits< std::int64_t >::min() )
                {
                    *pRet = SignedNegation< std::int64_t >::Value( tmp );
                    return true;
                }
            }
            else
            {
                // Result must be positive
                if( tmp <= (std::uint64_t)std::numeric_limits<std::int64_t>::max() )
                {
                    *pRet = (std::int64_t)tmp;
                    return true;
                }
            }
        }

        return false;
#endif
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void RegMultiplyThrow( const std::int64_t& a, const std::int64_t& b, std::int64_t* pRet ) SAFEINT_CPP_THROW
    {
#if SAFEINT_USE_INTRINSICS
        if( !IntrinsicMultiplyInt64( a, b, pRet ) )
            E::SafeIntOnOverflow();
#else
        bool aNegative = false;
        bool bNegative = false;

        std::uint64_t tmp = 0;
        std::int64_t a1 = a;
        std::int64_t b1 = b;

        if( a1 < 0 )
        {
            aNegative = true;
            a1 = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(a1);
        }

        if( b1 < 0 )
        {
            bNegative = true;
            b1 = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(b1);
        }

        LargeIntRegMultiply< std::uint64_t, std::uint64_t >::template RegMultiplyThrow< E >( (std::uint64_t)a1, (std::uint64_t)b1, &tmp );

        // The unsigned multiplication didn't overflow or we'd be in the exception handler
        if( aNegative ^ bNegative )
        {
            // Result must be negative
            if( tmp <= (std::uint64_t)std::numeric_limits< std::int64_t >::min() )
            {
                *pRet = SignedNegation< std::int64_t >::Value( tmp );
                return;
            }
        }
        else
        {
            // Result must be positive
            if( tmp <= (std::uint64_t)std::numeric_limits<std::int64_t>::max() )
            {
                *pRet = (std::int64_t)tmp;
                return;
            }
        }

        E::SafeIntOnOverflow();
#endif
    }
};

template<> class LargeIntRegMultiply< std::int64_t, std::uint32_t >
{
public:
    _CONSTEXPR14_MULTIPLY static bool RegMultiply( const std::int64_t& a, std::uint32_t b, std::int64_t* pRet ) SAFEINT_NOTHROW
    {
#if SAFEINT_USE_INTRINSICS
        return IntrinsicMultiplyInt64( a, (std::int64_t)b, pRet );
#else
        bool aNegative = false;
        std::uint64_t tmp = 0;
        std::int64_t a1 = a;

        if( a1 < 0 )
        {
            aNegative = true;
            a1 = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(a1);
        }

        if( LargeIntRegMultiply< std::uint64_t, std::uint32_t >::RegMultiply( (std::uint64_t)a1, b, &tmp ) )
        {
            // The unsigned multiplication didn't overflow
            if( aNegative )
            {
                // Result must be negative
                if( tmp <= (std::uint64_t)std::numeric_limits< std::int64_t >::min() )
                {
                    *pRet = SignedNegation< std::int64_t >::Value( tmp );
                    return true;
                }
            }
            else
            {
                // Result must be positive
                if( tmp <= (std::uint64_t)std::numeric_limits<std::int64_t>::max() )
                {
                    *pRet = (std::int64_t)tmp;
                    return true;
                }
            }
        }

        return false;
#endif
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void RegMultiplyThrow( const std::int64_t& a, std::uint32_t b, std::int64_t* pRet ) SAFEINT_CPP_THROW
    {
#if SAFEINT_USE_INTRINSICS
        if( !IntrinsicMultiplyInt64( a, (std::int64_t)b, pRet ) )
            E::SafeIntOnOverflow();
#else
        bool aNegative = false;
        std::uint64_t tmp = 0;
        std::int64_t a1 = a;

        if( a1 < 0 )
        {
            aNegative = true;
            a1 = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(a1);
        }

        LargeIntRegMultiply< std::uint64_t, std::uint32_t >::template RegMultiplyThrow< E >( (std::uint64_t)a1, b, &tmp );

        // The unsigned multiplication didn't overflow
        if( aNegative )
        {
            // Result must be negative
            if( tmp <= (std::uint64_t)std::numeric_limits< std::int64_t >::min() )
            {
                *pRet = SignedNegation< std::int64_t >::Value( tmp );
                return;
            }
        }
        else
        {
            // Result must be positive
            if( tmp <= (std::uint64_t)std::numeric_limits<std::int64_t>::max() )
            {
                *pRet = (std::int64_t)tmp;
                return;
            }
        }

        E::SafeIntOnOverflow();
#endif
    }
};

template<> class LargeIntRegMultiply< std::int64_t, std::int32_t >
{
public:
    _CONSTEXPR14_MULTIPLY static bool RegMultiply( const std::int64_t& a, std::int32_t b, std::int64_t* pRet ) SAFEINT_NOTHROW
    {
#if SAFEINT_USE_INTRINSICS
        return IntrinsicMultiplyInt64( a, (std::int64_t)b, pRet );
#else
        bool aNegative = false;
        bool bNegative = false;

        std::uint64_t tmp = 0;
        std::int64_t a1 = a;
        std::int64_t b1 = b;

        if( a1 < 0 )
        {
            aNegative = true;
            a1 = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(a1);
        }

        if( b1 < 0 )
        {
            bNegative = true;
            b1 = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(b1);
        }

        if( LargeIntRegMultiply< std::uint64_t, std::uint32_t >::RegMultiply( (std::uint64_t)a1, (std::uint32_t)b1, &tmp ) )
        {
            // The unsigned multiplication didn't overflow
            if( aNegative ^ bNegative )
            {
                // Result must be negative
                if( tmp <= (std::uint64_t)std::numeric_limits< std::int64_t >::min() )
                {
                    *pRet = SignedNegation< std::int64_t >::Value( tmp );
                    return true;
                }
            }
            else
            {
                // Result must be positive
                if( tmp <= (std::uint64_t)std::numeric_limits<std::int64_t>::max() )
                {
                    *pRet = (std::int64_t)tmp;
                    return true;
                }
            }
        }

        return false;
#endif
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void RegMultiplyThrow( std::int64_t a, std::int32_t b, std::int64_t* pRet ) SAFEINT_CPP_THROW
    {
#if SAFEINT_USE_INTRINSICS
        if( !IntrinsicMultiplyInt64( a, (std::int64_t)b, pRet ) )
            E::SafeIntOnOverflow();
#else
        bool aNegative = false;
        bool bNegative = false;

        std::uint64_t tmp = 0;

        if( a < 0 )
        {
            aNegative = true;
            a = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(a);
        }

        if( b < 0 )
        {
            bNegative = true;
            b = (std::int32_t)AbsValueHelper< std::int32_t, GetAbsMethod< std::int32_t >::method >::Abs(b);
        }

        LargeIntRegMultiply< std::uint64_t, std::uint32_t >::template RegMultiplyThrow< E >( (std::uint64_t)a, (std::uint32_t)b, &tmp );

        // The unsigned multiplication didn't overflow
        if( aNegative ^ bNegative )
        {
            // Result must be negative
            if( tmp <= (std::uint64_t)std::numeric_limits< std::int64_t >::min() )
            {
                *pRet = SignedNegation< std::int64_t >::Value( tmp );
                return;
            }
        }
        else
        {
            // Result must be positive
            if( tmp <= (std::uint64_t)std::numeric_limits<std::int64_t>::max() )
            {
                *pRet = (std::int64_t)tmp;
                return;
            }
        }

        E::SafeIntOnOverflow();
#endif
    }
};

template<> class LargeIntRegMultiply< std::int32_t, std::int64_t >
{
public:
    _CONSTEXPR14_MULTIPLY static bool RegMultiply( std::int32_t a, const std::int64_t& b, std::int32_t* pRet ) SAFEINT_NOTHROW
    {
#if SAFEINT_USE_INTRINSICS
        std::int64_t tmp = 0;

        if( IntrinsicMultiplyInt64( a, b, &tmp ) )
        {
            if( tmp > std::numeric_limits< std::int32_t >::max() ||
                tmp < std::numeric_limits< std::int32_t >::min() )
            {
                return false;
            }

            *pRet = (std::int32_t)tmp;
            return true;
        }
        return false;
#else
        bool aNegative = false;
        bool bNegative = false;

        std::uint32_t tmp = 0;
        std::int64_t b1 = b;

        if( a < 0 )
        {
            aNegative = true;
            a = (std::int32_t)AbsValueHelper< std::int32_t, GetAbsMethod< std::int32_t >::method >::Abs(a);
        }

        if( b1 < 0 )
        {
            bNegative = true;
            b1 = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(b1);
        }

        if( LargeIntRegMultiply< std::uint32_t, std::uint64_t >::RegMultiply( (std::uint32_t)a, (std::uint64_t)b1, &tmp ) )
        {
            // The unsigned multiplication didn't overflow
            if( aNegative ^ bNegative )
            {
                // Result must be negative
                if( tmp <= (std::uint32_t)std::numeric_limits< std::int32_t >::min() )
                {
                    *pRet = SignedNegation< std::int32_t >::Value( tmp );
                    return true;
                }
            }
            else
            {
                // Result must be positive
                if( tmp <= (std::uint32_t)std::numeric_limits< std::int32_t >::max() )
                {
                    *pRet = (std::int32_t)tmp;
                    return true;
                }
            }
        }

        return false;
#endif
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void RegMultiplyThrow( std::int32_t a, const std::int64_t& b, std::int32_t* pRet ) SAFEINT_CPP_THROW
    {
#if SAFEINT_USE_INTRINSICS
        std::int64_t tmp;

        if( IntrinsicMultiplyInt64( a, b, &tmp ) )
        {
            if( tmp > std::numeric_limits< std::int32_t >::max() ||
                tmp < std::numeric_limits< std::int32_t >::min() )
            {
                E::SafeIntOnOverflow();
            }

            *pRet = (std::int32_t)tmp;
            return;
        }
        E::SafeIntOnOverflow();
#else
        bool aNegative = false;
        bool bNegative = false;

        std::uint32_t tmp = 0;
        std::int64_t b2 = b;

        if( a < 0 )
        {
            aNegative = true;
            a = (std::int32_t)AbsValueHelper< std::int32_t, GetAbsMethod< std::int32_t >::method >::Abs(a);
        }

        if( b < 0 )
        {
            bNegative = true;
            b2 = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(b2);
        }

        LargeIntRegMultiply< std::uint32_t, std::uint64_t >::template RegMultiplyThrow< E >( (std::uint32_t)a, (std::uint64_t)b2, &tmp );

        // The unsigned multiplication didn't overflow
        if( aNegative ^ bNegative )
        {
            // Result must be negative
            if( tmp <= (std::uint32_t)std::numeric_limits< std::int32_t >::min() )
            {
                *pRet = SignedNegation< std::int32_t >::Value( tmp );
                return;
            }
        }
        else
        {
            // Result must be positive
            if( tmp <= (std::uint32_t)std::numeric_limits< std::int32_t >::max() )
            {
                *pRet = (std::int32_t)tmp;
                return;
            }
        }

        E::SafeIntOnOverflow();
#endif
    }
};

template<> class LargeIntRegMultiply< std::int64_t, std::uint64_t >
{
public:
    // Leave this one as-is - will call unsigned intrinsic internally
    _CONSTEXPR14_MULTIPLY static bool RegMultiply( const std::int64_t& a, const std::uint64_t& b, std::int64_t* pRet ) SAFEINT_NOTHROW
    {
        bool aNegative = false;

        std::uint64_t tmp = 0;
        std::int64_t a1 = a;

        if( a1 < 0 )
        {
            aNegative = true;
            a1 = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(a1);
        }

        if( LargeIntRegMultiply< std::uint64_t, std::uint64_t >::RegMultiply( (std::uint64_t)a1, (std::uint64_t)b, &tmp ) )
        {
            // The unsigned multiplication didn't overflow
            if( aNegative )
            {
                // Result must be negative
                if( tmp <= (std::uint64_t)std::numeric_limits< std::int64_t >::min() )
                {
                    *pRet = SignedNegation< std::int64_t >::Value( tmp );
                    return true;
                }
            }
            else
            {
                // Result must be positive
                if( tmp <= (std::uint64_t)std::numeric_limits<std::int64_t>::max() )
                {
                    *pRet = (std::int64_t)tmp;
                    return true;
                }
            }
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void RegMultiplyThrow( const std::int64_t& a, const std::uint64_t& b, std::int64_t* pRet ) SAFEINT_CPP_THROW
    {
        bool aNegative = false;
        std::uint64_t tmp = 0;
        std::int64_t a1 = a;

        if( a1 < 0 )
        {
            aNegative = true;
            a1 = (std::int64_t)AbsValueHelper< std::int64_t, GetAbsMethod< std::int64_t >::method >::Abs(a1);
        }

        if( LargeIntRegMultiply< std::uint64_t, std::uint64_t >::RegMultiply( (std::uint64_t)a1, (std::uint64_t)b, &tmp ) )
        {
            // The unsigned multiplication didn't overflow
            if( aNegative )
            {
                // Result must be negative
                if( tmp <= (std::uint64_t)std::numeric_limits< std::int64_t >::min() )
                {
                    *pRet = SignedNegation< std::int64_t >::Value( tmp );
                    return;
                }
            }
            else
            {
                // Result must be positive
                if( tmp <= (std::uint64_t)std::numeric_limits<std::int64_t>::max() )
                {
                    *pRet = (std::int64_t)tmp;
                    return;
                }
            }
        }

        E::SafeIntOnOverflow();
    }
};

// In all of the following functions where LargeIntRegMultiply methods are called,
// we need to properly transition types. The methods need std::int64_t, std::int32_t, etc.
// but the variables being passed to us could be long long, long int, or long, depending on
// the compiler. Microsoft compiler knows that long long is the same type as std::int64_t, but gcc doesn't

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_Uint64Uint64 >
{
public:
    // T, U are std::uint64_t
    _CONSTEXPR14_MULTIPLY static bool Multiply( const T& t, const U& u, T& ret ) SAFEINT_NOTHROW
    {
        static_assert( safeint_internal::int_traits<T>::isUint64 && safeint_internal::int_traits<U>::isUint64, "T, U must be Uint64" );
        std::uint64_t t1 = t;
        std::uint64_t u1 = u;
        std::uint64_t tmp = 0;
        bool f = LargeIntRegMultiply< std::uint64_t, std::uint64_t >::RegMultiply( t1, u1, &tmp );
        ret = tmp;
        return f;
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void MultiplyThrow(const std::uint64_t& t, const std::uint64_t& u, T& ret) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<T>::isUint64 && safeint_internal::int_traits<U>::isUint64, "T, U must be Uint64");
        std::uint64_t t1 = t;
        std::uint64_t u1 = u;
        std::uint64_t tmp = 0;
        LargeIntRegMultiply< std::uint64_t, std::uint64_t >::template RegMultiplyThrow< E >( t1, u1, &tmp );
        ret = tmp;
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_Uint64Uint >
{
public:
    // T is std::uint64_t
    // U is any unsigned int 32-bit or less
    _CONSTEXPR14_MULTIPLY static bool Multiply( const T& t, const U& u, T& ret ) SAFEINT_NOTHROW
    {
        static_assert( safeint_internal::int_traits<T>::isUint64, "T must be Uint64" );
        std::uint64_t t1 = t;
        std::uint64_t tmp = 0;
        bool f = LargeIntRegMultiply< std::uint64_t, std::uint32_t >::RegMultiply( t1, (std::uint32_t)u, &tmp );
        ret = tmp;
        return f;
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void MultiplyThrow( const T& t, const U& u, T& ret ) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<T>::isUint64, "T must be Uint64");
        std::uint64_t t1 = t;
        std::uint64_t tmp = 0;
        LargeIntRegMultiply< std::uint64_t, std::uint32_t >::template RegMultiplyThrow< E >( t1, (std::uint32_t)u, &tmp );
        ret = tmp;
    }
};

// converse of the previous function
template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_UintUint64 >
{
public:
    // T is any unsigned int up to 32-bit
    // U is std::uint64_t
    _CONSTEXPR14 static bool Multiply(const T& t, const U& u, T& ret) SAFEINT_NOTHROW
    {
        static_assert(safeint_internal::int_traits<U>::isUint64, "U must be Uint64");
        std::uint64_t u1 = u;
        std::uint32_t tmp = 0;

        if( LargeIntRegMultiply< std::uint32_t, std::uint64_t >::RegMultiply( t, u1, &tmp ) &&
            SafeCastHelper< T, std::uint32_t, GetCastMethod< T, std::uint32_t >::method >::Cast(tmp, ret) )
        {
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void MultiplyThrow(const T& t, const U& u, T& ret) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<U>::isUint64, "U must be Uint64");
        std::uint64_t u1 = u;
        std::uint32_t tmp = 0;

        LargeIntRegMultiply< std::uint32_t, std::uint64_t >::template RegMultiplyThrow< E >( t, u1, &tmp );
        SafeCastHelper< T, std::uint32_t, GetCastMethod< T, std::uint32_t >::method >::template CastThrow< E >(tmp, ret);
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_Uint64Int >
{
public:
    // T is std::uint64_t
    // U is any signed int, up to 64-bit
    _CONSTEXPR14_MULTIPLY static bool Multiply(const T& t, const U& u, T& ret) SAFEINT_NOTHROW
    {
        static_assert(safeint_internal::int_traits<T>::isUint64, "T must be Uint64");
        std::uint64_t t1 = t;
        std::uint64_t tmp = 0;
        bool f = LargeIntRegMultiply< std::uint64_t, std::int32_t >::RegMultiply(t1, (std::int32_t)u, &tmp);
        ret = tmp;
        return f;
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void MultiplyThrow(const T& t, const U& u, T& ret) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<T>::isUint64, "T must be Uint64");
        std::uint64_t t1 = t;
        std::uint64_t tmp = 0;
        LargeIntRegMultiply< std::uint64_t, std::int32_t >::template RegMultiplyThrow< E >(t1, (std::int32_t)u, &tmp);
        ret = tmp;
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_Uint64Int64 >
{
public:
    // T is std::uint64_t
    // U is std::int64_t
    _CONSTEXPR14_MULTIPLY static bool Multiply(const T& t, const U& u, T& ret) SAFEINT_NOTHROW
    {
        static_assert( safeint_internal::int_traits<T>::isUint64 && safeint_internal::int_traits<U>::isInt64, "T must be Uint64, U Int64" );
        std::uint64_t t1 = t;
        std::int64_t          u1 = u;
        std::uint64_t tmp = 0;
        bool f = LargeIntRegMultiply< std::uint64_t, std::int64_t >::RegMultiply(t1, u1, &tmp);
        ret = tmp;
        return f;
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void MultiplyThrow(const T& t, const U& u, T& ret) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<T>::isUint64 && safeint_internal::int_traits<U>::isInt64, "T must be Uint64, U Int64");
        std::uint64_t t1 = t;
        std::int64_t          u1 = u;
        std::uint64_t tmp = 0;
        LargeIntRegMultiply< std::uint64_t, std::int64_t >::template RegMultiplyThrow< E >(t1, u1, &tmp);
        ret = tmp;
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_UintInt64 >
{
public:
    // T is unsigned up to 32-bit
    // U is std::int64_t
    _CONSTEXPR14 static bool Multiply(const T& t, const U& u, T& ret) SAFEINT_NOTHROW
    {
        static_assert(safeint_internal::int_traits<U>::isInt64, "U must be Int64");
        std::int64_t          u1 = u;
        std::uint32_t tmp = 0;

        if( LargeIntRegMultiply< std::uint32_t, std::int64_t >::RegMultiply( (std::uint32_t)t, u1, &tmp ) &&
            SafeCastHelper< T, std::uint32_t, GetCastMethod< T, std::uint32_t >::method >::Cast(tmp, ret) )
        {
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void MultiplyThrow(const T& t, const U& u, T& ret) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<U>::isInt64, "U must be Int64");
        std::int64_t          u1 = u;
        std::uint32_t tmp = 0;

        LargeIntRegMultiply< std::uint32_t, std::int64_t >::template RegMultiplyThrow< E >( (std::uint32_t)t, u1, &tmp );
        SafeCastHelper< T, std::uint32_t, GetCastMethod< T, std::uint32_t >::method >::template CastThrow< E >(tmp, ret);
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_Int64Uint >
{
public:
    // T is std::int64_t
    // U is unsigned up to 32-bit
    _CONSTEXPR14_MULTIPLY static bool Multiply( const T& t, const U& u, T& ret ) SAFEINT_NOTHROW
    {
        static_assert(safeint_internal::int_traits<T>::isInt64, "T must be Int64");
        std::int64_t t1 = t;
        std::int64_t tmp = 0;
        bool f = LargeIntRegMultiply< std::int64_t, std::uint32_t >::RegMultiply( t1, (std::uint32_t)u, &tmp );
        ret = tmp;
        return f;
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void MultiplyThrow( const T& t, const U& u, T& ret ) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<T>::isInt64, "T must be Int64");
        std::int64_t          t1 = t;
        std::int64_t tmp = 0;
        LargeIntRegMultiply< std::int64_t, std::uint32_t >::template RegMultiplyThrow< E >( t1, (std::uint32_t)u, &tmp );
        ret = tmp;
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_Int64Int64 >
{
public:
    // T, U are std::int64_t
    _CONSTEXPR14_MULTIPLY static bool Multiply( const T& t, const U& u, T& ret ) SAFEINT_NOTHROW
    {
        static_assert( safeint_internal::int_traits<T>::isInt64 && safeint_internal::int_traits<U>::isInt64, "T, U must be Int64" );
        std::int64_t  t1 = t;
        std::int64_t u1 = u;
        std::int64_t tmp = 0;
        bool f = LargeIntRegMultiply< std::int64_t, std::int64_t >::RegMultiply( t1, u1, &tmp );
        ret = tmp;
        return f;
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void MultiplyThrow( const T& t, const U& u, T& ret ) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<T>::isInt64 && safeint_internal::int_traits<U>::isInt64, "T, U must be Int64");
        std::int64_t t1 = t;
        std::int64_t u1 = u;
        std::int64_t tmp = 0;
        LargeIntRegMultiply< std::int64_t, std::int64_t >::template RegMultiplyThrow< E >( t1, u1, &tmp);
        ret = tmp;
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_Int64Int >
{
public:
    // T is std::int64_t
    // U is signed up to 32-bit
    _CONSTEXPR14_MULTIPLY static bool Multiply( const T& t, U u, T& ret ) SAFEINT_NOTHROW
    {
        static_assert(safeint_internal::int_traits<T>::isInt64, "T must be Int64");
        std::int64_t t1 = t;
        std::int64_t tmp = 0;
        bool f = LargeIntRegMultiply< std::int64_t, std::int32_t >::RegMultiply( t1, (std::int32_t)u, &tmp);
        ret = tmp;
        return f;
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void MultiplyThrow( const std::int64_t& t, U u, T& ret ) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<T>::isInt64, "T must be Int64");
        std::int64_t t1 = t;
        std::int64_t tmp = 0;
        LargeIntRegMultiply< std::int64_t, std::int32_t >::template RegMultiplyThrow< E >(t1, (std::int32_t)u, &tmp);
        ret = tmp;
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_IntUint64 >
{
public:
    // T is signed up to 32-bit
    // U is std::uint64_t
    _CONSTEXPR14 static bool Multiply(T t, const U& u, T& ret) SAFEINT_NOTHROW
    {
        static_assert(safeint_internal::int_traits<U>::isUint64, "U must be Uint64");
        std::uint64_t u1 = u;
        std::int32_t tmp = 0;

        if( LargeIntRegMultiply< std::int32_t, std::uint64_t >::RegMultiply( (std::int32_t)t, u1, &tmp ) &&
            SafeCastHelper< T, std::int32_t, GetCastMethod< T, std::int32_t >::method >::Cast( tmp, ret ) )
        {
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void MultiplyThrow(T t, const std::uint64_t& u, T& ret) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<U>::isUint64, "U must be Uint64");
        std::uint64_t u1 = u;
        std::int32_t tmp = 0;

        LargeIntRegMultiply< std::int32_t, std::uint64_t >::template RegMultiplyThrow< E >( (std::int32_t)t, u1, &tmp );
        SafeCastHelper< T, std::int32_t, GetCastMethod< T, std::int32_t >::method >::template CastThrow< E >( tmp, ret );
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_Int64Uint64>
{
public:
    // T is std::int64_t
    // U is std::uint64_t
    _CONSTEXPR14_MULTIPLY static bool Multiply( const T& t, const U& u, T& ret ) SAFEINT_NOTHROW
    {
        static_assert( safeint_internal::int_traits<T>::isInt64 && safeint_internal::int_traits<U>::isUint64, "T must be Int64, U Uint64" );
        std::int64_t t1 = t;
        std::uint64_t u1 = u;
        std::int64_t tmp = 0;
        bool f = LargeIntRegMultiply< std::int64_t, std::uint64_t >::RegMultiply( t1, u1, &tmp );
        ret = tmp;
        return f;
    }

    template < typename E >
    _CONSTEXPR14_MULTIPLY static void MultiplyThrow( const std::int64_t& t, const std::uint64_t& u, T& ret ) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<T>::isInt64 && safeint_internal::int_traits<U>::isUint64, "T must be Int64, U Uint64");
        std::int64_t t1 = t;
        std::uint64_t u1 = u;
        std::int64_t tmp = 0;
        LargeIntRegMultiply< std::int64_t, std::uint64_t >::template RegMultiplyThrow< E >( t1, u1, &tmp );
        ret = tmp;
    }
};

template < typename T, typename U > class MultiplicationHelper< T, U, MultiplicationState_IntInt64>
{
public:
    // T is signed, up to 32-bit
    // U is std::int64_t
    _CONSTEXPR14 static bool Multiply( T t, const U& u, T& ret ) SAFEINT_NOTHROW
    {
        static_assert( safeint_internal::int_traits<U>::isInt64, "U must be Int64" );
        std::int64_t u1 = u;
        std::int32_t tmp = 0;

        if( LargeIntRegMultiply< std::int32_t, std::int64_t >::RegMultiply( (std::int32_t)t, u1, &tmp ) &&
            SafeCastHelper< T, std::int32_t, GetCastMethod< T, std::int32_t >::method >::Cast( tmp, ret ) )
        {
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void MultiplyThrow(T t, const U& u, T& ret) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<U>::isInt64, "U must be Int64");
        std::int64_t u1 = u;
        std::int32_t tmp = 0;

        LargeIntRegMultiply< std::int32_t, std::int64_t >::template RegMultiplyThrow< E >( (std::int32_t)t, u1, &tmp );
        SafeCastHelper< T, std::int32_t, GetCastMethod< T, std::int32_t >::method >::template CastThrow< E >( tmp, ret );
    }
};

enum DivisionState
{
    DivisionState_OK,
    DivisionState_UnsignedSigned,
    DivisionState_SignedUnsigned32,
    DivisionState_SignedUnsigned64,
    DivisionState_SignedUnsigned,
    DivisionState_SignedSigned
};

template < typename T, typename U > class DivisionMethod
{
public:
    enum
    {
        method = (safeint_internal::type_compare< T, U >::isBothUnsigned                     ? DivisionState_OK :
                 (!std::numeric_limits< T >::is_signed && std::numeric_limits< U >::is_signed) ? DivisionState_UnsignedSigned :
                 (std::numeric_limits< T >::is_signed &&
                    safeint_internal::int_traits< U >::isUint32 &&
                    safeint_internal::int_traits< T >::isLT64Bit)                           ? DivisionState_SignedUnsigned32 :
                 (std::numeric_limits< T >::is_signed && safeint_internal::int_traits< U >::isUint64)  ? DivisionState_SignedUnsigned64 :
                 (std::numeric_limits< T >::is_signed && !std::numeric_limits< U >::is_signed) ? DivisionState_SignedUnsigned :
                                                                           DivisionState_SignedSigned)
    };
};

template < typename T, typename U, int state > class DivisionHelper;

template < typename T, typename U > class DivisionHelper< T, U, DivisionState_OK >
{
public:
    _CONSTEXPR14 static SafeIntError Divide( const T& t, const U& u, T& result ) SAFEINT_NOTHROW
    {
        if( u == 0 )
            return SafeIntError::SafeIntDivideByZero;

        if( t == 0 )
        {
            result = 0;
            return SafeIntError::SafeIntNoError;
        }

        result = (T)( t/u );
        return SafeIntError::SafeIntNoError;
    }

    template < typename E >
    _CONSTEXPR14 static void DivideThrow( const T& t, const U& u, T& result ) SAFEINT_CPP_THROW
    {
        if( u == 0 )
            E::SafeIntOnDivZero();

        if( t == 0 )
        {
            result = 0;
            return;
        }

        result = (T)( t/u );
    }
};

template < typename T, typename U > class DivisionHelper< T, U, DivisionState_UnsignedSigned>
{
public:
    _CONSTEXPR14 static SafeIntError Divide( const T& t, const U& u, T& result ) SAFEINT_NOTHROW
    {

        if( u == 0 )
            return SafeIntError::SafeIntDivideByZero;

        if( t == 0 )
        {
            result = 0;
            return SafeIntError::SafeIntNoError;
        }

        if( u > 0 )
        {
            result = (T)( t/u );
            return SafeIntError::SafeIntNoError;
        }

        // it is always an error to try and divide an unsigned number by a negative signed number
        // unless u is bigger than t
        if( AbsValueHelper< U, GetAbsMethod< U >::method >::Abs( u ) > t )
        {
            result = 0;
            return SafeIntError::SafeIntNoError;
        }

        return SafeIntError::SafeIntArithmeticOverflow;
    }

    template < typename E >
    _CONSTEXPR14 static void DivideThrow( const T& t, const U& u, T& result ) SAFEINT_CPP_THROW
    {

        if( u == 0 )
            E::SafeIntOnDivZero();

        if( t == 0 )
        {
            result = 0;
            return;
        }

        if( u > 0 )
        {
            result = (T)( t/u );
            return;
        }

        // it is always an error to try and divide an unsigned number by a negative signed number
        // unless u is bigger than t
        if( AbsValueHelper< U, GetAbsMethod< U >::method >::Abs( u ) > t )
        {
            result = 0;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class DivisionHelper< T, U, DivisionState_SignedUnsigned32 >
{
public:
    _CONSTEXPR14 static SafeIntError Divide( const T& t, const U& u, T& result ) SAFEINT_NOTHROW
    {
        if( u == 0 )
            return SafeIntError::SafeIntDivideByZero;

        if( t == 0 )
        {
            result = 0;
            return SafeIntError::SafeIntNoError;
        }

        // Test for t > 0
        // If t < 0, must explicitly upcast, or implicit upcast to ulong will cause errors
        // As it turns out, 32-bit division is about twice as fast, which justifies the extra conditional

        if( t > 0 )
            result = (T)( t/u );
        else
            result = (T)( (std::int64_t)t/(std::int64_t)u );

        return SafeIntError::SafeIntNoError;
    }

    template < typename E >
    _CONSTEXPR14 static void DivideThrow( const T& t, const U& u, T& result ) SAFEINT_CPP_THROW
    {
        if( u == 0 )
        {
            E::SafeIntOnDivZero();
        }

        if( t == 0 )
        {
            result = 0;
            return;
        }

        // Test for t > 0
        // If t < 0, must explicitly upcast, or implicit upcast to ulong will cause errors
        // As it turns out, 32-bit division is about twice as fast, which justifies the extra conditional

        if( t > 0 )
            result = (T)( t/u );
        else
            result = (T)( (std::int64_t)t/(std::int64_t)u );
    }
};

template < typename T, typename U, bool > class div_signed_uint64;
template < typename T, typename U> class div_signed_uint64 <T, U, true> // Value of u fits into an int32
{
public:
    _CONSTEXPR14 static T divide(T t, U u) { return (T)((std::int32_t)t / (std::int32_t)u); }
};

template < typename T, typename U> class div_signed_uint64 <T, U, false>
{
public:
    _CONSTEXPR14 static T divide(T t, U u) { return (T)((std::int64_t)t / (std::int64_t)u); }
};

template < typename T, typename U > class DivisionHelper< T, U, DivisionState_SignedUnsigned64 >
{
public:
    _CONSTEXPR14 static SafeIntError Divide( const T& t, const std::uint64_t& u, T& result ) SAFEINT_NOTHROW
    {
        static_assert(safeint_internal::int_traits<U>::isUint64, "U must be Uint64");

        if( u == 0 )
        {
            return SafeIntError::SafeIntDivideByZero;
        }

        if( t == 0 )
        {
            result = 0;
            return SafeIntError::SafeIntNoError;
        }

        if( u <= (std::uint64_t)std::numeric_limits<T>::max() )
        {
            result = div_signed_uint64 < T, U, sizeof(T) < sizeof(std::int64_t) > ::divide(t, u);
        }
        else // Corner case
        if( t == std::numeric_limits<T>::min() && u == (std::uint64_t)std::numeric_limits<T>::min() )
        {
            // Min int divided by it's own magnitude is -1
            result = -1;
        }
        else
        {
            result = 0;
        }
        return SafeIntError::SafeIntNoError;
    }

    template < typename E >
    _CONSTEXPR14 static void DivideThrow( const T& t, const std::uint64_t& u, T& result ) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits<U>::isUint64, "U must be Uint64");

        if( u == 0 )
        {
            E::SafeIntOnDivZero();
        }

        if( t == 0 )
        {
            result = 0;
            return;
        }

        if( u <= (std::uint64_t)std::numeric_limits<T>::max() )
        {
            result = div_signed_uint64 < T, U, sizeof(T) < sizeof(std::int64_t) > ::divide(t, u);
        }
        else // Corner case
        if( t == std::numeric_limits<T>::min() && u == (std::uint64_t)std::numeric_limits<T>::min() )
        {
            // Min int divided by it's own magnitude is -1
            result = -1;
        }
        else
        {
            result = 0;
        }
    }
};

template < typename T, typename U > class DivisionHelper< T, U, DivisionState_SignedUnsigned>
{
public:
    // T is any signed, U is unsigned and smaller than 32-bit
    // In this case, standard operator casting is correct
    _CONSTEXPR14 static SafeIntError Divide( const T& t, const U& u, T& result ) SAFEINT_NOTHROW
    {
        if( u == 0 )
        {
            return SafeIntError::SafeIntDivideByZero;
        }

        if( t == 0 )
        {
            result = 0;
            return SafeIntError::SafeIntNoError;
        }

        result = (T)( t/u );
        return SafeIntError::SafeIntNoError;
    }

    template < typename E >
    _CONSTEXPR14 static void DivideThrow( const T& t, const U& u, T& result ) SAFEINT_CPP_THROW
    {
        if( u == 0 )
        {
            E::SafeIntOnDivZero();
        }

        if( t == 0 )
        {
            result = 0;
            return;
        }

        result = (T)( t/u );
    }
};

template < typename T, typename U > class DivisionHelper< T, U, DivisionState_SignedSigned>
{
public:
    _CONSTEXPR14 static SafeIntError Divide( const T& t, const U& u, T& result ) SAFEINT_NOTHROW
    {
        if( u == 0 )
        {
            return SafeIntError::SafeIntDivideByZero;
        }

        if( t == 0 )
        {
            result = 0;
            return SafeIntError::SafeIntNoError;
        }

        // Must test for corner case
        if( t == std::numeric_limits<T>::min() && u == (U)-1 )
            return SafeIntError::SafeIntArithmeticOverflow;

        result = (T)( t/u );
        return SafeIntError::SafeIntNoError;
    }

    template < typename E >
    _CONSTEXPR14 static void DivideThrow( const T& t, const U& u, T& result ) SAFEINT_CPP_THROW
    {
        if(u == 0)
        {
            E::SafeIntOnDivZero();
        }

        if( t == 0 )
        {
            result = 0;
            return;
        }

        // Must test for corner case
        if( t == std::numeric_limits<T>::min() && u == (U)-1 )
            E::SafeIntOnOverflow();

        result = (T)( t/u );
    }
};

enum AdditionState
{
    AdditionState_CastIntCheckMax,
    AdditionState_CastUintCheckOverflow,
    AdditionState_CastUintCheckOverflowMax,
    AdditionState_CastUint64CheckOverflow,
    AdditionState_CastUint64CheckOverflowMax,
    AdditionState_CastIntCheckSafeIntMinMax,
    AdditionState_CastInt64CheckSafeIntMinMax,
    AdditionState_CastInt64CheckMax,
    AdditionState_CastUint64CheckSafeIntMinMax,
    AdditionState_CastUint64CheckSafeIntMinMax2,
    AdditionState_CastInt64CheckOverflow,
    AdditionState_CastInt64CheckOverflowSafeIntMinMax,
    AdditionState_CastInt64CheckOverflowMax,
    AdditionState_ManualCheckInt64Uint64,
    AdditionState_ManualCheck,
    AdditionState_Error
};

template< typename T, typename U >
class AdditionMethod
{
public:
    enum
    {
                 //unsigned-unsigned
        method = (IntRegion< T,U >::IntZone_UintLT32_UintLT32  ? AdditionState_CastIntCheckMax :
                 (IntRegion< T,U >::IntZone_Uint32_UintLT64)   ? AdditionState_CastUintCheckOverflow :
                 (IntRegion< T,U >::IntZone_UintLT32_Uint32)   ? AdditionState_CastUintCheckOverflowMax :
                 (IntRegion< T,U >::IntZone_Uint64_Uint)       ? AdditionState_CastUint64CheckOverflow :
                 (IntRegion< T,U >::IntZone_UintLT64_Uint64)   ? AdditionState_CastUint64CheckOverflowMax :
                 //unsigned-signed
                 (IntRegion< T,U >::IntZone_UintLT32_IntLT32)  ? AdditionState_CastIntCheckSafeIntMinMax :
                 (IntRegion< T,U >::IntZone_Uint32_IntLT64 ||
                  IntRegion< T,U >::IntZone_UintLT32_Int32)    ? AdditionState_CastInt64CheckSafeIntMinMax :
                 (IntRegion< T,U >::IntZone_Uint64_Int ||
                  IntRegion< T,U >::IntZone_Uint64_Int64)      ? AdditionState_CastUint64CheckSafeIntMinMax :
                 (IntRegion< T,U >::IntZone_UintLT64_Int64)    ? AdditionState_CastUint64CheckSafeIntMinMax2 :
                 //signed-signed
                 (IntRegion< T,U >::IntZone_IntLT32_IntLT32)   ? AdditionState_CastIntCheckSafeIntMinMax :
                 (IntRegion< T,U >::IntZone_Int32_IntLT64 ||
                  IntRegion< T,U >::IntZone_IntLT32_Int32)     ? AdditionState_CastInt64CheckSafeIntMinMax :
                 (IntRegion< T,U >::IntZone_Int64_Int ||
                  IntRegion< T,U >::IntZone_Int64_Int64)       ? AdditionState_CastInt64CheckOverflow :
                 (IntRegion< T,U >::IntZone_IntLT64_Int64)     ? AdditionState_CastInt64CheckOverflowSafeIntMinMax :
                 //signed-unsigned
                 (IntRegion< T,U >::IntZone_IntLT32_UintLT32)  ? AdditionState_CastIntCheckMax :
                 (IntRegion< T,U >::IntZone_Int32_UintLT32 ||
                  IntRegion< T,U >::IntZone_IntLT64_Uint32)    ? AdditionState_CastInt64CheckMax :
                 (IntRegion< T,U >::IntZone_Int64_UintLT64)    ? AdditionState_CastInt64CheckOverflowMax :
                 (IntRegion< T,U >::IntZone_Int64_Uint64)      ? AdditionState_ManualCheckInt64Uint64 :
                 (IntRegion< T,U >::IntZone_Int_Uint64)        ? AdditionState_ManualCheck :
                  AdditionState_Error)
    };
};

template < typename T, typename U, int method > class AdditionHelper;

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastIntCheckMax >
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        //16-bit or less unsigned addition
        std::int32_t tmp = lhs + rhs;

        if( tmp <= (std::int32_t)std::numeric_limits<T>::max() )
        {
            result = (T)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        //16-bit or less unsigned addition
        std::int32_t tmp = lhs + rhs;

        if( tmp <= (std::int32_t)std::numeric_limits<T>::max() )
        {
            result = (T)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastUintCheckOverflow >
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // 32-bit or less - both are unsigned
        std::uint32_t tmp = (std::uint32_t)lhs + (std::uint32_t)rhs;

        //we added didn't get smaller
        if( tmp >= lhs )
        {
            result = (T)tmp;
            return true;
        }
        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // 32-bit or less - both are unsigned
        std::uint32_t tmp = (std::uint32_t)lhs + (std::uint32_t)rhs;

        //we added didn't get smaller
        if( tmp >= lhs )
        {
            result = (T)tmp;
            return;
        }
        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastUintCheckOverflowMax>
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // 32-bit or less - both are unsigned
        std::uint32_t tmp = (std::uint32_t)lhs + (std::uint32_t)rhs;

        // We added and it didn't get smaller or exceed maxInt
        if( tmp >= lhs && tmp <= std::numeric_limits<T>::max() )
        {
            result = (T)tmp;
            return true;
        }
        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        //32-bit or less - both are unsigned
        std::uint32_t tmp = (std::uint32_t)lhs + (std::uint32_t)rhs;

        // We added and it didn't get smaller or exceed maxInt
        if( tmp >= lhs && tmp <= std::numeric_limits<T>::max() )
        {
            result = (T)tmp;
            return;
        }
        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastUint64CheckOverflow>
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // lhs std::uint64_t, rhs unsigned
        std::uint64_t tmp = (std::uint64_t)lhs + (std::uint64_t)rhs;

        // We added and it didn't get smaller
        if(tmp >= lhs)
        {
            result = (T)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs std::uint64_t, rhs unsigned
        std::uint64_t tmp = (std::uint64_t)lhs + (std::uint64_t)rhs;

        // We added and it didn't get smaller
        if(tmp >= lhs)
        {
            result = (T)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastUint64CheckOverflowMax >
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        //lhs std::uint64_t, rhs unsigned
        std::uint64_t tmp = (std::uint64_t)lhs + (std::uint64_t)rhs;

        // We added and it didn't get smaller
        if( tmp >= lhs && tmp <= std::numeric_limits<T>::max() )
        {
            result = (T)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        //lhs std::uint64_t, rhs unsigned
        std::uint64_t tmp = (std::uint64_t)lhs + (std::uint64_t)rhs;

        // We added and it didn't get smaller
        if( tmp >= lhs && tmp <= std::numeric_limits<T>::max() )
        {
            result = (T)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastIntCheckSafeIntMinMax >
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // 16-bit or less - one or both are signed
        std::int32_t tmp = lhs + rhs;

        if( tmp <= (std::int32_t)std::numeric_limits<T>::max() && tmp >= (std::int32_t)std::numeric_limits<T>::min() )
        {
            result = (T)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // 16-bit or less - one or both are signed
        std::int32_t tmp = lhs + rhs;

        if( tmp <= (std::int32_t)std::numeric_limits<T>::max() && tmp >= (std::int32_t)std::numeric_limits<T>::min() )
        {
            result = (T)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastInt64CheckSafeIntMinMax >
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // 32-bit or less - one or both are signed
        std::int64_t tmp = (std::int64_t)lhs + (std::int64_t)rhs;

        if( tmp <= (std::int64_t)std::numeric_limits<T>::max() && tmp >= (std::int64_t)std::numeric_limits<T>::min() )
        {
            result = (T)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // 32-bit or less - one or both are signed
        std::int64_t tmp = (std::int64_t)lhs + (std::int64_t)rhs;

        if( tmp <= (std::int64_t)std::numeric_limits<T>::max() && tmp >= (std::int64_t)std::numeric_limits<T>::min() )
        {
            result = (T)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastInt64CheckMax >
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // 32-bit or less - lhs signed, rhs unsigned
        std::int64_t tmp = (std::int64_t)lhs + (std::int64_t)rhs;

        if( tmp <= std::numeric_limits<T>::max() )
        {
            result = (T)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // 32-bit or less - lhs signed, rhs unsigned
        std::int64_t tmp = (std::int64_t)lhs + (std::int64_t)rhs;

        if( tmp <= std::numeric_limits<T>::max() )
        {
            result = (T)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastUint64CheckSafeIntMinMax >
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // lhs is std::uint64_t, rhs signed
        std::uint64_t tmp = 0;

        if( rhs < 0 )
        {
            // So we're effectively subtracting
            tmp = AbsValueHelper< U, GetAbsMethod< U >::method >::Abs( rhs );

            if( tmp <= lhs )
            {
                result = lhs - tmp;
                return true;
            }
        }
        else
        {
            // now we know that rhs can be safely cast into an std::uint64_t
            tmp = (std::uint64_t)lhs + (std::uint64_t)rhs;

            // We added and it did not become smaller
            if( tmp >= lhs )
            {
                result = (T)tmp;
                return true;
            }
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs is std::uint64_t, rhs signed
        std::uint64_t tmp = 0;

        if( rhs < 0 )
        {
            // So we're effectively subtracting
            tmp = AbsValueHelper< U, GetAbsMethod< U >::method >::Abs( rhs );

            if( tmp <= lhs )
            {
                result = lhs - tmp;
                return;
            }
        }
        else
        {
            // now we know that rhs can be safely cast into an std::uint64_t
            tmp = (std::uint64_t)lhs + (std::uint64_t)rhs;

            // We added and it did not become smaller
            if( tmp >= lhs )
            {
                result = (T)tmp;
                return;
            }
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastUint64CheckSafeIntMinMax2>
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // lhs is unsigned and < 64-bit, rhs std::int64_t
        if( rhs < 0 )
        {
            if( lhs >= ~(std::uint64_t)( rhs ) + 1 )//negation is safe, since rhs is 64-bit
            {
                result = (T)( lhs + rhs );
                return true;
            }
        }
        else
        {
            // now we know that rhs can be safely cast into an std::uint64_t
            std::uint64_t tmp = (std::uint64_t)lhs + (std::uint64_t)rhs;

            // special case - rhs cannot be larger than 0x7fffffffffffffff, lhs cannot be larger than 0xffffffff
            // it is not possible for the operation above to overflow, so just check max
            if( tmp <= std::numeric_limits<T>::max() )
            {
                result = (T)tmp;
                return true;
            }
        }
        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs is unsigned and < 64-bit, rhs std::int64_t
        if( rhs < 0 )
        {
            if( lhs >= ~(std::uint64_t)( rhs ) + 1) //negation is safe, since rhs is 64-bit
            {
                result = (T)( lhs + rhs );
                return;
            }
        }
        else
        {
            // now we know that rhs can be safely cast into an std::uint64_t
            std::uint64_t tmp = (std::uint64_t)lhs + (std::uint64_t)rhs;

            // special case - rhs cannot be larger than 0x7fffffffffffffff, lhs cannot be larger than 0xffffffff
            // it is not possible for the operation above to overflow, so just check max
            if( tmp <= std::numeric_limits<T>::max() )
            {
                result = (T)tmp;
                return;
            }
        }
        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastInt64CheckOverflow>
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // lhs is std::int64_t, rhs signed
        std::int64_t tmp = (std::int64_t)((std::uint64_t)lhs + (std::uint64_t)rhs);

        if( lhs >= 0 )
        {
            // mixed sign cannot overflow
            if( rhs >= 0 && tmp < lhs )
                return false;
        }
        else
        {
            // lhs negative
            if( rhs < 0 && tmp > lhs )
                return false;
        }

        result = (T)tmp;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs is std::int64_t, rhs signed
        std::int64_t tmp = (std::int64_t)((std::uint64_t)lhs + (std::uint64_t)rhs);

        if( lhs >= 0 )
        {
            // mixed sign cannot overflow
            if( rhs >= 0 && tmp < lhs )
                E::SafeIntOnOverflow();
        }
        else
        {
            // lhs negative
            if( rhs < 0 && tmp > lhs )
                E::SafeIntOnOverflow();
        }

        result = (T)tmp;
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastInt64CheckOverflowSafeIntMinMax>
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        //rhs is std::int64_t, lhs signed
        std::int64_t tmp = 0;

        if( AdditionHelper< std::int64_t, std::int64_t, AdditionState_CastInt64CheckOverflow >::Addition( (std::int64_t)lhs, (std::int64_t)rhs, tmp ) &&
            tmp <= std::numeric_limits<T>::max() &&
            tmp >= std::numeric_limits<T>::min() )
        {
            result = (T)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        //rhs is std::int64_t, lhs signed
        std::int64_t tmp = 0;

        AdditionHelper< std::int64_t, std::int64_t, AdditionState_CastInt64CheckOverflow >::AdditionThrow< E >( (std::int64_t)lhs, (std::int64_t)rhs, tmp );

        if( tmp <= std::numeric_limits<T>::max() &&
            tmp >= std::numeric_limits<T>::min() )
        {
            result = (T)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_CastInt64CheckOverflowMax>
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        //lhs is std::int64_t, rhs unsigned < 64-bit
        std::uint64_t tmp = (std::uint64_t)lhs + (std::uint64_t)rhs;

        if( (std::int64_t)tmp >= lhs )
        {
            result = (T)(std::int64_t)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs is std::int64_t, rhs unsigned < 64-bit
        // Some compilers get optimization-happy, let's thwart them

        std::uint64_t tmp = (std::uint64_t)lhs + (std::uint64_t)rhs;

        if( (std::int64_t)tmp >= lhs )
        {
            result = (T)(std::int64_t)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_ManualCheckInt64Uint64 >
{
public:
    _CONSTEXPR14 static bool Addition( const std::int64_t& lhs, const std::uint64_t& rhs, T& result ) SAFEINT_NOTHROW
    {
        static_assert( safeint_internal::int_traits< T >::isInt64 && safeint_internal::int_traits< U >::isUint64, "T must be Int64, U Uint64" );
        // rhs is std::uint64_t, lhs std::int64_t
        // cast everything to unsigned, perform addition, then
        // cast back for check - this is done to stop optimizers from removing the code
        std::uint64_t tmp = (std::uint64_t)lhs + rhs;

        if( (std::int64_t)tmp >= lhs )
        {
            result = (std::int64_t)tmp;
            return true;
        }

        result = 0;
        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const std::int64_t& lhs, const std::uint64_t& rhs, T& result ) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits< T >::isInt64 && safeint_internal::int_traits< U >::isUint64, "T must be Int64, U Uint64");
        // rhs is std::uint64_t, lhs std::int64_t
        std::uint64_t tmp = (std::uint64_t)lhs + rhs;

        if( (std::int64_t)tmp >= lhs )
        {
            result = (std::int64_t)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class AdditionHelper < T, U, AdditionState_ManualCheck>
{
public:
    _CONSTEXPR14 static bool Addition( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // rhs is std::uint64_t, lhs signed, 32-bit or less
        if( (std::uint32_t)( rhs >> 32 ) == 0 )
        {
            // Now it just happens to work out that the standard behavior does what we want
            // Adding explicit casts to show exactly what's happening here
            // Note - this is tweaked to keep optimizers from tossing out the code.
            std::uint32_t tmp = (std::uint32_t)rhs + (std::uint32_t)lhs;

            if( (std::int32_t)tmp >= lhs && SafeCastHelper< T, std::int32_t, GetCastMethod< T, std::int32_t >::method >::Cast( (std::int32_t)tmp, result ) )
                return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void AdditionThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // rhs is std::uint64_t, lhs signed, 32-bit or less

        if( (std::uint32_t)( rhs >> 32 ) == 0 )
        {
            // Now it just happens to work out that the standard behavior does what we want
            // Adding explicit casts to show exactly what's happening here
            std::uint32_t tmp = (std::uint32_t)rhs + (std::uint32_t)lhs;

            if( (std::int32_t)tmp >= lhs )
            {
                SafeCastHelper< T, std::int32_t, GetCastMethod< T, std::int32_t >::method >::template CastThrow< E >( (std::int32_t)tmp, result );
                return;
            }
        }
        E::SafeIntOnOverflow();
    }
};

enum SubtractionState
{
    SubtractionState_BothUnsigned,
    SubtractionState_CastIntCheckSafeIntMinMax,
    SubtractionState_CastIntCheckMin,
    SubtractionState_CastInt64CheckSafeIntMinMax,
    SubtractionState_CastInt64CheckMin,
    SubtractionState_Uint64Int,
    SubtractionState_UintInt64,
    SubtractionState_Int64Int,
    SubtractionState_IntInt64,
    SubtractionState_Int64Uint,
    SubtractionState_IntUint64,
    SubtractionState_Int64Uint64,
    // states for SubtractionMethod2
    SubtractionState_BothUnsigned2,
    SubtractionState_CastIntCheckSafeIntMinMax2,
    SubtractionState_CastInt64CheckSafeIntMinMax2,
    SubtractionState_Uint64Int2,
    SubtractionState_UintInt642,
    SubtractionState_Int64Int2,
    SubtractionState_IntInt642,
    SubtractionState_Int64Uint2,
    SubtractionState_IntUint642,
    SubtractionState_Int64Uint642,
    SubtractionState_Error
};

template < typename T, typename U > class SubtractionMethod
{
public:
    enum
    {
                 // unsigned-unsigned
        method = ((IntRegion< T,U >::IntZone_UintLT32_UintLT32 ||
                 (IntRegion< T,U >::IntZone_Uint32_UintLT64)   ||
                 (IntRegion< T,U >::IntZone_UintLT32_Uint32)   ||
                 (IntRegion< T,U >::IntZone_Uint64_Uint)       ||
                 (IntRegion< T,U >::IntZone_UintLT64_Uint64))      ? SubtractionState_BothUnsigned :
                 // unsigned-signed
                 (IntRegion< T,U >::IntZone_UintLT32_IntLT32)      ? SubtractionState_CastIntCheckSafeIntMinMax :
                 (IntRegion< T,U >::IntZone_Uint32_IntLT64 ||
                  IntRegion< T,U >::IntZone_UintLT32_Int32)        ? SubtractionState_CastInt64CheckSafeIntMinMax :
                 (IntRegion< T,U >::IntZone_Uint64_Int ||
                  IntRegion< T,U >::IntZone_Uint64_Int64)          ? SubtractionState_Uint64Int :
                 (IntRegion< T,U >::IntZone_UintLT64_Int64)        ? SubtractionState_UintInt64 :
                 // signed-signed
                 (IntRegion< T,U >::IntZone_IntLT32_IntLT32)       ? SubtractionState_CastIntCheckSafeIntMinMax :
                 (IntRegion< T,U >::IntZone_Int32_IntLT64 ||
                  IntRegion< T,U >::IntZone_IntLT32_Int32)         ? SubtractionState_CastInt64CheckSafeIntMinMax :
                 (IntRegion< T,U >::IntZone_Int64_Int ||
                  IntRegion< T,U >::IntZone_Int64_Int64)           ? SubtractionState_Int64Int :
                 (IntRegion< T,U >::IntZone_IntLT64_Int64)         ? SubtractionState_IntInt64 :
                 // signed-unsigned
                 (IntRegion< T,U >::IntZone_IntLT32_UintLT32)      ? SubtractionState_CastIntCheckMin :
                 (IntRegion< T,U >::IntZone_Int32_UintLT32 ||
                  IntRegion< T,U >::IntZone_IntLT64_Uint32)        ? SubtractionState_CastInt64CheckMin :
                 (IntRegion< T,U >::IntZone_Int64_UintLT64)        ? SubtractionState_Int64Uint :
                 (IntRegion< T,U >::IntZone_Int_Uint64)            ? SubtractionState_IntUint64 :
                 (IntRegion< T,U >::IntZone_Int64_Uint64)          ? SubtractionState_Int64Uint64 :
                  SubtractionState_Error)
    };
};

// this is for the case of U - SafeInt< T, E >
template < typename T, typename U > class SubtractionMethod2
{
public:
    enum
    {
                 // unsigned-unsigned
        method = ((IntRegion< T,U >::IntZone_UintLT32_UintLT32 ||
                 (IntRegion< T,U >::IntZone_Uint32_UintLT64)   ||
                 (IntRegion< T,U >::IntZone_UintLT32_Uint32)   ||
                 (IntRegion< T,U >::IntZone_Uint64_Uint)       ||
                 (IntRegion< T,U >::IntZone_UintLT64_Uint64))     ? SubtractionState_BothUnsigned2 :
                 // unsigned-signed
                 (IntRegion< T,U >::IntZone_UintLT32_IntLT32)     ? SubtractionState_CastIntCheckSafeIntMinMax2 :
                 (IntRegion< T,U >::IntZone_Uint32_IntLT64 ||
                  IntRegion< T,U >::IntZone_UintLT32_Int32)       ? SubtractionState_CastInt64CheckSafeIntMinMax2 :
                 (IntRegion< T,U >::IntZone_Uint64_Int ||
                  IntRegion< T,U >::IntZone_Uint64_Int64)         ? SubtractionState_Uint64Int2 :
                 (IntRegion< T,U >::IntZone_UintLT64_Int64)       ? SubtractionState_UintInt642 :
                 // signed-signed
                 (IntRegion< T,U >::IntZone_IntLT32_IntLT32)      ? SubtractionState_CastIntCheckSafeIntMinMax2 :
                 (IntRegion< T,U >::IntZone_Int32_IntLT64 ||
                  IntRegion< T,U >::IntZone_IntLT32_Int32)        ? SubtractionState_CastInt64CheckSafeIntMinMax2 :
                 (IntRegion< T,U >::IntZone_Int64_Int ||
                  IntRegion< T,U >::IntZone_Int64_Int64)          ? SubtractionState_Int64Int2 :
                 (IntRegion< T,U >::IntZone_IntLT64_Int64)        ? SubtractionState_IntInt642 :
                 // signed-unsigned
                 (IntRegion< T,U >::IntZone_IntLT32_UintLT32)     ? SubtractionState_CastIntCheckSafeIntMinMax2 :
                 (IntRegion< T,U >::IntZone_Int32_UintLT32 ||
                  IntRegion< T,U >::IntZone_IntLT64_Uint32)       ? SubtractionState_CastInt64CheckSafeIntMinMax2 :
                 (IntRegion< T,U >::IntZone_Int64_UintLT64)       ? SubtractionState_Int64Uint2 :
                 (IntRegion< T,U >::IntZone_Int_Uint64)           ? SubtractionState_IntUint642 :
                 (IntRegion< T,U >::IntZone_Int64_Uint64)         ? SubtractionState_Int64Uint642 :
                                                                    SubtractionState_Error)
    };
};

template < typename T, typename U, int method > class SubtractionHelper;

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_BothUnsigned >
{
public:
    _CONSTEXPR14 static bool Subtract( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // both are unsigned - easy case
        if( rhs <= lhs )
        {
            result = (T)( lhs - rhs );
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // both are unsigned - easy case
        if( rhs <= lhs )
        {
            result = (T)( lhs - rhs );
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_BothUnsigned2 >
{
public:
    _CONSTEXPR14 static bool Subtract( const T& lhs, const U& rhs, U& result ) SAFEINT_NOTHROW
    {
        // both are unsigned - easy case
        // Except we do have to check for overflow - lhs could be larger than result can hold
        if( rhs <= lhs )
        {
            T tmp = (T)(lhs - rhs);
            return SafeCastHelper< U, T, GetCastMethod<U, T>::method>::Cast( tmp, result);
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const T& lhs, const U& rhs, U& result ) SAFEINT_CPP_THROW
    {
        // both are unsigned - easy case
        if( rhs <= lhs )
        {
            T tmp = (T)(lhs - rhs);
            SafeCastHelper< U, T, GetCastMethod<U, T>::method >::template CastThrow<E>( tmp, result);
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_CastIntCheckSafeIntMinMax >
{
public:
    _CONSTEXPR14 static bool Subtract( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // both values are 16-bit or less
        // rhs is signed, so could end up increasing or decreasing
        std::int32_t tmp = lhs - rhs;

        if( SafeCastHelper< T, std::int32_t, GetCastMethod< T, std::int32_t >::method >::Cast( tmp, result ) )
        {
            result = (T)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // both values are 16-bit or less
        // rhs is signed, so could end up increasing or decreasing
        std::int32_t tmp = lhs - rhs;

        SafeCastHelper< T, std::int32_t, GetCastMethod< T, std::int32_t >::method >::template CastThrow< E >( tmp, result );
    }
};

template <typename U, typename T> class SubtractionHelper< U, T, SubtractionState_CastIntCheckSafeIntMinMax2 >
{
public:
    _CONSTEXPR14 static bool Subtract( const U& lhs, const T& rhs, T& result ) SAFEINT_NOTHROW
    {
        // both values are 16-bit or less
        // rhs is signed, so could end up increasing or decreasing
        std::int32_t tmp = lhs - rhs;

        return SafeCastHelper< T, std::int32_t, GetCastMethod< T, std::int32_t >::method >::Cast( tmp, result );
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const U& lhs, const T& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // both values are 16-bit or less
        // rhs is signed, so could end up increasing or decreasing
        std::int32_t tmp = lhs - rhs;

        SafeCastHelper< T, std::int32_t, GetCastMethod< T, std::int32_t >::method >::template CastThrow< E >( tmp, result );
    }
};

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_CastIntCheckMin >
{
public:
    _CONSTEXPR14 static bool Subtract( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // both values are 16-bit or less
        // rhs is unsigned - check only minimum
        std::int32_t tmp = lhs - rhs;

        if( tmp >= (std::int32_t)std::numeric_limits<T>::min() )
        {
            result = (T)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // both values are 16-bit or less
        // rhs is unsigned - check only minimum
        std::int32_t tmp = lhs - rhs;

        if( tmp >= (std::int32_t)std::numeric_limits<T>::min() )
        {
            result = (T)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_CastInt64CheckSafeIntMinMax >
{
public:
    _CONSTEXPR14 static bool Subtract( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // both values are 32-bit or less
        // rhs is signed, so could end up increasing or decreasing
        std::int64_t tmp = (std::int64_t)lhs - (std::int64_t)rhs;

        return SafeCastHelper< T, std::int64_t, GetCastMethod< T, std::int64_t >::method >::Cast( tmp, result );
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // both values are 32-bit or less
        // rhs is signed, so could end up increasing or decreasing
        std::int64_t tmp = (std::int64_t)lhs - (std::int64_t)rhs;

        SafeCastHelper< T, std::int64_t, GetCastMethod< T, std::int64_t >::method >::template CastThrow< E >( tmp, result );
    }
};

template <typename U, typename T> class SubtractionHelper< U, T, SubtractionState_CastInt64CheckSafeIntMinMax2 >
{
public:
    _CONSTEXPR14 static bool Subtract( const U& lhs, const T& rhs, T& result ) SAFEINT_NOTHROW
    {
        // both values are 32-bit or less
        // rhs is signed, so could end up increasing or decreasing
        std::int64_t tmp = (std::int64_t)lhs - (std::int64_t)rhs;

        return SafeCastHelper< T, std::int64_t, GetCastMethod< T, std::int64_t >::method >::Cast( tmp, result );
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const U& lhs, const T& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // both values are 32-bit or less
        // rhs is signed, so could end up increasing or decreasing
        std::int64_t tmp = (std::int64_t)lhs - (std::int64_t)rhs;

        SafeCastHelper< T, std::int64_t, GetCastMethod< T, std::int64_t >::method >::template CastThrow< E >( tmp, result );
    }
};

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_CastInt64CheckMin >
{
public:
    _CONSTEXPR14 static bool Subtract( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // both values are 32-bit or less
        // rhs is unsigned - check only minimum
        std::int64_t tmp = (std::int64_t)lhs - (std::int64_t)rhs;

        if( tmp >= (std::int64_t)std::numeric_limits<T>::min() )
        {
            result = (T)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // both values are 32-bit or less
        // rhs is unsigned - check only minimum
        std::int64_t tmp = (std::int64_t)lhs - (std::int64_t)rhs;

        if( tmp >= (std::int64_t)std::numeric_limits<T>::min() )
        {
            result = (T)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_Uint64Int >
{
public:
    _CONSTEXPR14 static bool Subtract( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // lhs is an std::uint64_t, rhs signed
        // must first see if rhs is positive or negative
        if( rhs >= 0 )
        {
            if( (std::uint64_t)rhs <= lhs )
            {
                result = (T)( lhs - (std::uint64_t)rhs );
                return true;
            }
        }
        else
        {
            T tmp = lhs;
            // we're now effectively adding
            result = lhs + AbsValueHelper< U, GetAbsMethod< U >::method >::Abs( rhs );

            if(result >= tmp)
                return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs is an std::uint64_t, rhs signed
        // must first see if rhs is positive or negative
        if( rhs >= 0 )
        {
            if( (std::uint64_t)rhs <= lhs )
            {
                result = (T)( lhs - (std::uint64_t)rhs );
                return;
            }
        }
        else
        {
            T tmp = lhs;
            // we're now effectively adding
            result = lhs + AbsValueHelper< U, GetAbsMethod< U >::method >::Abs( rhs );

            if(result >= tmp)
                return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename U, typename T > class SubtractionHelper< U, T, SubtractionState_Uint64Int2 >
{
public:
    _CONSTEXPR14 static bool Subtract( const U& lhs, const T& rhs, T& result ) SAFEINT_NOTHROW
    {
        // U is std::uint64_t, T is signed
        if( rhs < 0 )
        {
            // treat this as addition
            std::uint64_t tmp = 0;

            tmp = lhs + (std::uint64_t)AbsValueHelper< T, GetAbsMethod< T >::method >::Abs( rhs );

            // must check for addition overflow and max
            if( tmp >= lhs && tmp <= std::numeric_limits<T>::max() )
            {
                result = (T)tmp;
                return true;
            }
        }
        else if( (std::uint64_t)rhs > lhs ) // now both are positive, so comparison always works
        {
            // result is negative
            // implies that lhs must fit into T, and result cannot overflow
            // Also allows us to drop to 32-bit math, which is faster on a 32-bit system
            result = (T)lhs - (T)rhs;
            return true;
        }
        else
        {
            // result is positive
            std::uint64_t tmp = (std::uint64_t)lhs - (std::uint64_t)rhs;

            if( tmp <= std::numeric_limits<T>::max() )
            {
                result = (T)tmp;
                return true;
            }
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const U& lhs, const T& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // U is std::uint64_t, T is signed
        if( rhs < 0 )
        {
            // treat this as addition
            std::uint64_t tmp = 0;

            tmp = lhs + (std::uint64_t)AbsValueHelper< T, GetAbsMethod< T >::method >::Abs( rhs );

            // must check for addition overflow and max
            if( tmp >= lhs && tmp <= (std::uint64_t)std::numeric_limits<T>::max() )
            {
                result = (T)tmp;
                return;
            }
        }
        else if( (std::uint64_t)rhs > lhs ) // now both are positive, so comparison always works
        {
            // result is negative
            // implies that lhs must fit into T, and result cannot overflow
            // Also allows us to drop to 32-bit math, which is faster on a 32-bit system
            result = (T)lhs - (T)rhs;
            return;
        }
        else
        {
            // result is positive
            std::uint64_t tmp = (std::uint64_t)lhs - (std::uint64_t)rhs;

            if( tmp <= (std::uint64_t)std::numeric_limits<T>::max() )
            {
                result = (T)tmp;
                return;
            }
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_UintInt64 >
{
public:
    _CONSTEXPR14 static bool Subtract( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // lhs is an unsigned int32 or smaller, rhs std::int64_t
        // must first see if rhs is positive or negative
        if( rhs >= 0 )
        {
            if( (std::uint64_t)rhs <= lhs )
            {
                result = (T)( lhs - (T)rhs );
                return true;
            }
        }
        else
        {
            // we're now effectively adding
            // since lhs is 32-bit, and rhs cannot exceed 2^63
            // this addition cannot overflow
            std::uint64_t tmp = lhs + ~(std::uint64_t)( rhs ) + 1; // negation safe

            // but we could exceed MaxInt
            if(tmp <= std::numeric_limits<T>::max())
            {
                result = (T)tmp;
                return true;
            }
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs is an unsigned int32 or smaller, rhs std::int64_t
        // must first see if rhs is positive or negative
        if( rhs >= 0 )
        {
            if( (std::uint64_t)rhs <= lhs )
            {
                result = (T)( lhs - (T)rhs );
                return;
            }
        }
        else
        {
            // we're now effectively adding
            // since lhs is 32-bit, and rhs cannot exceed 2^63
            // this addition cannot overflow
            std::uint64_t tmp = lhs + ~(std::uint64_t)( rhs ) + 1; // negation safe

            // but we could exceed MaxInt
            if(tmp <= std::numeric_limits<T>::max())
            {
                result = (T)tmp;
                return;
            }
        }

        E::SafeIntOnOverflow();
    }
};

template <typename U, typename T> class SubtractionHelper< U, T, SubtractionState_UintInt642 >
{
public:
    _CONSTEXPR14 static bool Subtract( const U& lhs, const T& rhs, T& result ) SAFEINT_NOTHROW
    {
        // U unsigned 32-bit or less, T std::int64_t
        if( rhs >= 0 )
        {
            // overflow not possible
            result = (T)( (std::int64_t)lhs - rhs );
            return true;
        }
        else
        {
            // we effectively have an addition
            // which cannot overflow internally
            std::uint64_t tmp = (std::uint64_t)lhs + (std::uint64_t)( -rhs );

            if( tmp <= (std::uint64_t)std::numeric_limits<T>::max() )
            {
                result = (T)tmp;
                return true;
            }
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const U& lhs, const T& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // U unsigned 32-bit or less, T std::int64_t
        if( rhs >= 0 )
        {
            // overflow not possible
            result = (T)( (std::int64_t)lhs - rhs );
            return;
        }
        else
        {
            // we effectively have an addition
            // which cannot overflow internally
            std::uint64_t tmp = (std::uint64_t)lhs + (std::uint64_t)( -rhs );

            if( tmp <= (std::uint64_t)std::numeric_limits<T>::max() )
            {
                result = (T)tmp;
                return;
            }
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_Int64Int >
{
public:
    _CONSTEXPR14 static bool Subtract( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // lhs is an std::int64_t, rhs signed (up to 64-bit)
        // we have essentially 4 cases:
        //
        // 1) lhs positive, rhs positive - overflow not possible
        // 2) lhs positive, rhs negative - equivalent to addition - result >= lhs or error
        // 3) lhs negative, rhs positive - check result <= lhs
        // 4) lhs negative, rhs negative - overflow not possible

        std::int64_t tmp = (std::int64_t)((std::uint64_t)lhs - (std::uint64_t)rhs);

        // Note - ideally, we can order these so that true conditionals
        // lead to success, which enables better pipelining
        // It isn't practical here
        if( ( lhs >= 0 && rhs < 0 && tmp < lhs ) || // condition 2
            ( rhs >= 0 && tmp > lhs ) )             // condition 3
        {
            return false;
        }

        result = (T)tmp;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs is an std::int64_t, rhs signed (up to 64-bit)
        // we have essentially 4 cases:
        //
        // 1) lhs positive, rhs positive - overflow not possible
        // 2) lhs positive, rhs negative - equivalent to addition - result >= lhs or error
        // 3) lhs negative, rhs positive - check result <= lhs
        // 4) lhs negative, rhs negative - overflow not possible

        std::int64_t tmp = (std::int64_t)((std::uint64_t)lhs - (std::uint64_t)rhs);

        // Note - ideally, we can order these so that true conditionals
        // lead to success, which enables better pipelining
        // It isn't practical here
        if( ( lhs >= 0 && rhs < 0 && tmp < lhs ) || // condition 2
            ( rhs >= 0 && tmp > lhs ) )             // condition 3
        {
            E::SafeIntOnOverflow();
        }

        result = (T)tmp;
    }
};

template < typename T, typename U, bool > class subtract_corner_case_max;

template < typename T, typename U> class subtract_corner_case_max < T, U, true>
{
public:
    _CONSTEXPR14 static bool isOverflowPositive(const T& rhs, const U& lhs, std::int64_t tmp)
    {
        return (tmp > std::numeric_limits<T>::max() || (rhs < 0 && tmp < lhs));
    }

    _CONSTEXPR14 static bool isOverflowNegative(const T& rhs, const U& lhs, std::int64_t tmp)
    {
         return (tmp < std::numeric_limits<T>::min() || (rhs >= 0 && tmp > lhs));
    }
};

template < typename T, typename U> class subtract_corner_case_max < T, U, false>
{
public:
    _CONSTEXPR14 static bool isOverflowPositive(const T& rhs, const U& lhs, std::int64_t tmp)
    {
        return (rhs < 0 && tmp < lhs);
    }

    _CONSTEXPR14 static bool isOverflowNegative(const T& rhs, const U& lhs, std::int64_t tmp)
    {
        return (rhs >= 0 && tmp > lhs);
    }
};

template < typename U, typename T > class SubtractionHelper< U, T, SubtractionState_Int64Int2 >
{
public:
    _CONSTEXPR14 static bool Subtract( const U& lhs, const T& rhs, T& result ) SAFEINT_NOTHROW
    {
        // lhs std::int64_t, rhs any signed int (including std::int64_t)
        std::int64_t tmp = lhs - rhs;

        // we have essentially 4 cases:
        //
        // 1) lhs positive, rhs positive - overflow not possible in tmp
        // 2) lhs positive, rhs negative - equivalent to addition - result >= lhs or error
        // 3) lhs negative, rhs positive - check result <= lhs
        // 4) lhs negative, rhs negative - overflow not possible in tmp

        if( lhs >= 0 )
        {
            // if both positive, overflow to negative not possible
            // which is why we'll explicitly check maxInt, and not call SafeCast
            if(subtract_corner_case_max< T, U, safeint_internal::int_traits< T >::isLT64Bit >::isOverflowPositive(rhs, lhs, tmp))
            {
                return false;
            }
        }
        else
        {
            // lhs negative
            if(subtract_corner_case_max< T, U, safeint_internal::int_traits< T >::isLT64Bit >::isOverflowNegative(rhs, lhs, tmp))
            {
                return false;
            }
        }

        result = (T)tmp;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const U& lhs, const T& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs std::int64_t, rhs any signed int (including std::int64_t)
        std::int64_t tmp = lhs - rhs;

        // we have essentially 4 cases:
        //
        // 1) lhs positive, rhs positive - overflow not possible in tmp
        // 2) lhs positive, rhs negative - equivalent to addition - result >= lhs or error
        // 3) lhs negative, rhs positive - check result <= lhs
        // 4) lhs negative, rhs negative - overflow not possible in tmp

        if( lhs >= 0 )
        {
            // if both positive, overflow to negative not possible
            // which is why we'll explicitly check maxInt, and not call SafeCast
            if (subtract_corner_case_max< T, U, safeint_internal::int_traits< T >::isLT64Bit>::isOverflowPositive(rhs, lhs, tmp))
            {
                E::SafeIntOnOverflow();
            }
        }
        else
        {
            // lhs negative
            if (subtract_corner_case_max< T, U, safeint_internal::int_traits< T >::isLT64Bit >::isOverflowNegative(rhs, lhs, tmp))
            {
                E::SafeIntOnOverflow();
            }
        }

        result = (T)tmp;
    }
};

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_IntInt64 >
{
public:
    _CONSTEXPR14 static bool Subtract( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // lhs is a 32-bit int or less, rhs std::int64_t
        // we have essentially 4 cases:
        //
        // lhs positive, rhs positive - rhs could be larger than lhs can represent
        // lhs positive, rhs negative - additive case - check tmp >= lhs and tmp > max int
        // lhs negative, rhs positive - check tmp <= lhs and tmp < min int
        // lhs negative, rhs negative - addition cannot internally overflow, check against max

        std::int64_t tmp = (std::int64_t)((std::uint64_t)lhs - (std::uint64_t)rhs);

        if( lhs >= 0 )
        {
            // first case
            if( rhs >= 0 )
            {
                if( tmp >= std::numeric_limits<T>::min() )
                {
                    result = (T)tmp;
                    return true;
                }
            }
            else
            {
                // second case
                if( tmp >= lhs && tmp <= std::numeric_limits<T>::max() )
                {
                    result = (T)tmp;
                    return true;
                }
            }
        }
        else
        {
            // lhs < 0
            // third case
            if( rhs >= 0 )
            {
                if( tmp <= lhs && tmp >= std::numeric_limits<T>::min() )
                {
                    result = (T)tmp;
                    return true;
                }
            }
            else
            {
                // fourth case
                if( tmp <= std::numeric_limits<T>::max() )
                {
                    result = (T)tmp;
                    return true;
                }
            }
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs is a 32-bit int or less, rhs std::int64_t
        // we have essentially 4 cases:
        //
        // lhs positive, rhs positive - rhs could be larger than lhs can represent
        // lhs positive, rhs negative - additive case - check tmp >= lhs and tmp > max int
        // lhs negative, rhs positive - check tmp <= lhs and tmp < min int
        // lhs negative, rhs negative - addition cannot internally overflow, check against max

        std::int64_t tmp = (std::int64_t)((std::uint64_t)lhs - (std::uint64_t)rhs);

        if( lhs >= 0 )
        {
            // first case
            if( rhs >= 0 )
            {
                if( tmp >= std::numeric_limits<T>::min() )
                {
                    result = (T)tmp;
                    return;
                }
            }
            else
            {
                // second case
                if( tmp >= lhs && tmp <= std::numeric_limits<T>::max() )
                {
                    result = (T)tmp;
                    return;
                }
            }
        }
        else
        {
            // lhs < 0
            // third case
            if( rhs >= 0 )
            {
                if( tmp <= lhs && tmp >= std::numeric_limits<T>::min() )
                {
                    result = (T)tmp;
                    return;
                }
            }
            else
            {
                // fourth case
                if( tmp <= std::numeric_limits<T>::max() )
                {
                    result = (T)tmp;
                    return;
                }
            }
        }

        E::SafeIntOnOverflow();
    }
};

template < typename U, typename T > class SubtractionHelper< U, T, SubtractionState_IntInt642 >
{
public:
    _CONSTEXPR14 static bool Subtract( const U& lhs, const T& rhs, T& result ) SAFEINT_NOTHROW
    {
        // lhs is any signed int32 or smaller, rhs is int64
        std::int64_t tmp = (std::int64_t)lhs - rhs;

        if( ( lhs >= 0 && rhs < 0 && tmp < lhs ) ||
            ( rhs > 0 && tmp > lhs ) )
        {
            return false;
            //else OK
        }

        result = (T)tmp;
        return true;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const U& lhs, const T& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs is any signed int32 or smaller, rhs is int64
        std::int64_t tmp = (std::int64_t)lhs - rhs;

        if( ( lhs >= 0 && rhs < 0 && tmp < lhs ) ||
            ( rhs > 0 && tmp > lhs ) )
        {
            E::SafeIntOnOverflow();
            //else OK
        }

        result = (T)tmp;
    }
};

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_Int64Uint >
{
public:
    _CONSTEXPR14 static bool Subtract( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // lhs is a 64-bit int, rhs unsigned int32 or smaller
        // perform test as unsigned to prevent unwanted optimizations
        std::uint64_t tmp = (std::uint64_t)lhs - (std::uint64_t)rhs;

        if( (std::int64_t)tmp <= lhs )
        {
            result = (T)(std::int64_t)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs is a 64-bit int, rhs unsigned int32 or smaller
        // perform test as unsigned to prevent unwanted optimizations
        std::uint64_t tmp = (std::uint64_t)lhs - (std::uint64_t)rhs;

        if( (std::int64_t)tmp <= lhs )
        {
            result = (T)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename U, typename T > class SubtractionHelper< U, T, SubtractionState_Int64Uint2 >
{
public:
    // lhs is std::int64_t, rhs is unsigned 32-bit or smaller
    _CONSTEXPR14 static bool Subtract( const U& lhs, const T& rhs, T& result ) SAFEINT_NOTHROW
    {
        // Do this as unsigned to prevent unwanted optimizations
        std::uint64_t tmp = (std::uint64_t)lhs - (std::uint64_t)rhs;

        if( (std::int64_t)tmp <= std::numeric_limits<T>::max() && (std::int64_t)tmp >= std::numeric_limits<T>::min() )
        {
            result = (T)(std::int64_t)tmp;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const U& lhs, const T& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // Do this as unsigned to prevent unwanted optimizations
        std::uint64_t tmp = (std::uint64_t)lhs - (std::uint64_t)rhs;

        if( (std::int64_t)tmp <= std::numeric_limits<T>::max() && (std::int64_t)tmp >= std::numeric_limits<T>::min() )
        {
            result = (T)(std::int64_t)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_IntUint64 >
{
public:
    _CONSTEXPR14 static bool Subtract( const T& lhs, const U& rhs, T& result ) SAFEINT_NOTHROW
    {
        // lhs is any signed int, rhs unsigned int64
        // check against available range

        // We need the absolute value of std::numeric_limits<T>::min()
        // This will give it to us without extraneous compiler warnings
        const std::uint64_t AbsMinIntT = (std::uint64_t)std::numeric_limits<T>::max() + 1;

        if( lhs < 0 )
        {
            if( rhs <= AbsMinIntT - AbsValueHelper< T, GetAbsMethod< T >::method >::Abs( lhs ) )
            {
                result = (T)( lhs - rhs );
                return true;
            }
        }
        else
        {
            if( rhs <= AbsMinIntT + (std::uint64_t)lhs )
            {
                result = (T)( lhs - rhs );
                return true;
            }
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const T& lhs, const U& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // lhs is any signed int, rhs unsigned int64
        // check against available range

        // We need the absolute value of std::numeric_limits<T>::min()
        // This will give it to us without extraneous compiler warnings
        const std::uint64_t AbsMinIntT = (std::uint64_t)std::numeric_limits<T>::max() + 1;

        if( lhs < 0 )
        {
            if( rhs <= AbsMinIntT - AbsValueHelper< T, GetAbsMethod< T >::method >::Abs( lhs ) )
            {
                result = (T)( lhs - rhs );
                return;
            }
        }
        else
        {
            if( rhs <= AbsMinIntT + (std::uint64_t)lhs )
            {
                result = (T)( lhs - rhs );
                return;
            }
        }

        E::SafeIntOnOverflow();
    }
};

template < typename U, typename T > class SubtractionHelper< U, T, SubtractionState_IntUint642 >
{
public:
    _CONSTEXPR14 static bool Subtract( const U& lhs, const T& rhs, T& result ) SAFEINT_NOTHROW
    {
        // We run into upcasting problems on comparison - needs 2 checks
        if( lhs >= 0 && (T)lhs >= rhs )
        {
            result = (T)((U)lhs - (U)rhs);
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const U& lhs, const T& rhs, T& result ) SAFEINT_CPP_THROW
    {
        // We run into upcasting problems on comparison - needs 2 checks
        if( lhs >= 0 && (T)lhs >= rhs )
        {
            result = (T)((U)lhs - (U)rhs);
            return;
        }

        E::SafeIntOnOverflow();
    }

};

template < typename T, typename U > class SubtractionHelper< T, U, SubtractionState_Int64Uint64 >
{
public:
    _CONSTEXPR14 static bool Subtract( const std::int64_t& lhs, const std::uint64_t& rhs, std::int64_t& result ) SAFEINT_NOTHROW
    {
        static_assert(safeint_internal::int_traits< T >::isInt64 && safeint_internal::int_traits< U >::isUint64, "T must be Int64, U Uint64");
        // if we subtract, and it gets larger, there's a problem
        // Perform test as unsigned to prevent unwanted optimizations
        std::uint64_t tmp = (std::uint64_t)lhs - rhs;

        if( (std::int64_t)tmp <= lhs )
        {
            result = (std::int64_t)tmp;
            return true;
        }
        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const std::int64_t& lhs, const std::uint64_t& rhs, T& result ) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits< T >::isInt64 && safeint_internal::int_traits< U >::isUint64, "T must be Int64, U Uint64");
        // if we subtract, and it gets larger, there's a problem
        // Perform test as unsigned to prevent unwanted optimizations
        std::uint64_t tmp = (std::uint64_t)lhs - rhs;

        if( (std::int64_t)tmp <= lhs )
        {
            result = (std::int64_t)tmp;
            return;
        }

        E::SafeIntOnOverflow();
    }

};

template < typename U, typename T > class SubtractionHelper< U, T, SubtractionState_Int64Uint642 >
{
public:
    // If lhs is negative, immediate problem - return must be positive, and subtracting only makes it
    // get smaller. If rhs > lhs, then it would also go negative, which is the other case
    _CONSTEXPR14 static bool Subtract( const std::int64_t& lhs, const std::uint64_t& rhs, T& result ) SAFEINT_NOTHROW
    {
        static_assert( safeint_internal::int_traits< T >::isUint64 && safeint_internal::int_traits< U >::isInt64, "T must be Uint64, U Int64" );
        if( lhs >= 0 && (std::uint64_t)lhs >= rhs )
        {
            result = (std::uint64_t)lhs - rhs;
            return true;
        }

        return false;
    }

    template < typename E >
    _CONSTEXPR14 static void SubtractThrow( const std::int64_t& lhs, const std::uint64_t& rhs, T& result ) SAFEINT_CPP_THROW
    {
        static_assert(safeint_internal::int_traits< T >::isUint64 && safeint_internal::int_traits< U >::isInt64, "T must be Uint64, U Int64");
        if( lhs >= 0 && (std::uint64_t)lhs >= rhs )
        {
            result = (std::uint64_t)lhs - rhs;
            return;
        }

        E::SafeIntOnOverflow();
    }

};

enum BinaryState
{
    BinaryState_OK,
    BinaryState_Int8,
    BinaryState_Int16,
    BinaryState_Int32
};

template < typename T, typename U > class BinaryMethod
{
public:
    enum
    {
        // If both operands are unsigned OR
        //    return type is smaller than rhs OR
        //    return type is larger and rhs is unsigned
        // Then binary operations won't produce unexpected results
        method = ( sizeof( T ) <= sizeof( U ) ||
                 safeint_internal::type_compare< T, U >::isBothUnsigned ||
                 !std::numeric_limits< U >::is_signed )          ? BinaryState_OK :
                 safeint_internal::int_traits< U >::isInt8               ? BinaryState_Int8 :
                 safeint_internal::int_traits< U >::isInt16              ? BinaryState_Int16
                                                      : BinaryState_Int32
    };
};

#ifdef SAFEINT_DISABLE_BINARY_ASSERT
#define BinaryAssert(x)
#else
#define BinaryAssert(x) SAFEINT_ASSERT(x)
#endif

template < typename T, typename U, int method > class BinaryAndHelper;

template < typename T, typename U > class BinaryAndHelper< T, U, BinaryState_OK >
{
public:
    _CONSTEXPR11 static T And( T lhs, U rhs ) SAFEINT_NOTHROW { return (T)( lhs & rhs ); }
};

template < typename T, typename U > class BinaryAndHelper< T, U, BinaryState_Int8 >
{
public:
    _CONSTEXPR14 static T And( T lhs, U rhs ) SAFEINT_NOTHROW
    {
        // cast forces sign extension to be zeros
        BinaryAssert( ( lhs & rhs ) == ( lhs & (std::uint8_t)rhs ) );
        return (T)( lhs & (std::uint8_t)rhs );
    }
};

template < typename T, typename U > class BinaryAndHelper< T, U, BinaryState_Int16 >
{
public:
    _CONSTEXPR14 static T And( T lhs, U rhs ) SAFEINT_NOTHROW
    {
        //cast forces sign extension to be zeros
        BinaryAssert( ( lhs & rhs ) == ( lhs & (std::uint16_t)rhs ) );
        return (T)( lhs & (std::uint16_t)rhs );
    }
};

template < typename T, typename U > class BinaryAndHelper< T, U, BinaryState_Int32 >
{
public:
    _CONSTEXPR14 static T And( T lhs, U rhs ) SAFEINT_NOTHROW
    {
        //cast forces sign extension to be zeros
        BinaryAssert( ( lhs & rhs ) == ( lhs & (std::uint32_t)rhs ) );
        return (T)( lhs & (std::uint32_t)rhs );
    }
};

template < typename T, typename U, int method > class BinaryOrHelper;

template < typename T, typename U > class BinaryOrHelper< T, U, BinaryState_OK >
{
public:
    _CONSTEXPR11 static T Or( T lhs, U rhs ) SAFEINT_NOTHROW { return (T)( lhs | rhs ); }
};

template < typename T, typename U > class BinaryOrHelper< T, U, BinaryState_Int8 >
{
public:
    _CONSTEXPR14 static T Or( T lhs, U rhs ) SAFEINT_NOTHROW
    {
        //cast forces sign extension to be zeros
        BinaryAssert( ( lhs | rhs ) == ( lhs | (std::uint8_t)rhs ) );
        return (T)( lhs | (std::uint8_t)rhs );
    }
};

template < typename T, typename U > class BinaryOrHelper< T, U, BinaryState_Int16 >
{
public:
    _CONSTEXPR14 static T Or( T lhs, U rhs ) SAFEINT_NOTHROW
    {
        //cast forces sign extension to be zeros
        BinaryAssert( ( lhs | rhs ) == ( lhs | (std::uint16_t)rhs ) );
        return (T)( lhs | (std::uint16_t)rhs );
    }
};

template < typename T, typename U > class BinaryOrHelper< T, U, BinaryState_Int32 >
{
public:
    _CONSTEXPR14 static T Or( T lhs, U rhs ) SAFEINT_NOTHROW
    {
        //cast forces sign extension to be zeros
        BinaryAssert( ( lhs | rhs ) == ( lhs | (std::uint32_t)rhs ) );
        return (T)( lhs | (std::uint32_t)rhs );
    }
};

template <typename T, typename U, int method > class BinaryXorHelper;

template < typename T, typename U > class BinaryXorHelper< T, U, BinaryState_OK >
{
public:
    _CONSTEXPR11 static T Xor( T lhs, U rhs ) SAFEINT_NOTHROW { return (T)( lhs ^ rhs ); }
};

template < typename T, typename U > class BinaryXorHelper< T, U, BinaryState_Int8 >
{
public:
    _CONSTEXPR14 static T Xor( T lhs, U rhs ) SAFEINT_NOTHROW
    {
        // cast forces sign extension to be zeros
        BinaryAssert( ( lhs ^ rhs ) == ( lhs ^ (std::uint8_t)rhs ) );
        return (T)( lhs ^ (std::uint8_t)rhs );
    }
};

template < typename T, typename U > class BinaryXorHelper< T, U, BinaryState_Int16 >
{
public:
    _CONSTEXPR14 static T Xor( T lhs, U rhs ) SAFEINT_NOTHROW
    {
        // cast forces sign extension to be zeros
        BinaryAssert( ( lhs ^ rhs ) == ( lhs ^ (std::uint16_t)rhs ) );
        return (T)( lhs ^ (std::uint16_t)rhs );
    }
};

template < typename T, typename U > class BinaryXorHelper< T, U, BinaryState_Int32 >
{
public:
    _CONSTEXPR14 static T Xor( T lhs, U rhs ) SAFEINT_NOTHROW
    {
        // cast forces sign extension to be zeros
        BinaryAssert( ( lhs ^ rhs ) == ( lhs ^ (std::uint32_t)rhs ) );
        return (T)( lhs ^ (std::uint32_t)rhs );
    }
};

/*****************  External functions ****************************************/

// External functions that can be used where you only need to check one operation
// non-class helper function so that you can check for a cast's validity
// and handle errors how you like
template < typename T, typename U >
_CONSTEXPR11 inline bool SafeCast( const T From, U& To ) SAFEINT_NOTHROW
{
    return SafeCastHelper< U, T, GetCastMethod< U, T >::method >::Cast( From, To );
}

template < typename T, typename U >
_CONSTEXPR11 inline bool SafeEquals( const T t, const U u ) SAFEINT_NOTHROW
{
    return EqualityTest< T, U, ValidComparison< T, U >::method >::IsEquals( t, u );
}

template < typename T, typename U >
_CONSTEXPR11 inline bool SafeNotEquals( const T t, const U u ) SAFEINT_NOTHROW
{
    return !EqualityTest< T, U, ValidComparison< T, U >::method >::IsEquals( t, u );
}

template < typename T, typename U >
_CONSTEXPR11 inline bool SafeGreaterThan( const T t, const U u ) SAFEINT_NOTHROW
{
    return GreaterThanTest< T, U, ValidComparison< T, U >::method >::GreaterThan( t, u );
}

template < typename T, typename U >
_CONSTEXPR11 inline bool SafeGreaterThanEquals( const T t, const U u ) SAFEINT_NOTHROW
{
    return !GreaterThanTest< U, T, ValidComparison< U, T >::method >::GreaterThan( u, t );
}

template < typename T, typename U >
_CONSTEXPR11 inline bool SafeLessThan( const T t, const U u ) SAFEINT_NOTHROW
{
    return GreaterThanTest< U, T, ValidComparison< U, T >::method >::GreaterThan( u, t );
}

template < typename T, typename U >
_CONSTEXPR11 inline bool SafeLessThanEquals( const T t, const U u ) SAFEINT_NOTHROW
{
    return !GreaterThanTest< T, U, ValidComparison< T, U >::method >::GreaterThan( t, u );
}

template < typename T, typename U >
_CONSTEXPR11 inline bool SafeModulus( const T& t, const U& u, T& result ) SAFEINT_NOTHROW
{
    return ( ModulusHelper< T, U, ValidComparison< T, U >::method >::Modulus( t, u, result ) == SafeIntError::SafeIntNoError );
}

template < typename T, typename U >
_CONSTEXPR14_MULTIPLY inline bool SafeMultiply( T t, U u, T& result ) SAFEINT_NOTHROW
{
    return MultiplicationHelper< T, U, MultiplicationMethod< T, U >::method >::Multiply( t, u, result );
}

template < typename T, typename U >
_CONSTEXPR11 inline bool SafeDivide( T t, U u, T& result ) SAFEINT_NOTHROW
{
    return ( DivisionHelper< T, U, DivisionMethod< T, U >::method >::Divide( t, u, result ) == SafeIntError::SafeIntNoError );
}

template < typename T, typename U >
_CONSTEXPR11 inline bool SafeAdd( T t, U u, T& result ) SAFEINT_NOTHROW
{
    return AdditionHelper< T, U, AdditionMethod< T, U >::method >::Addition( t, u, result );
}

template < typename T, typename U >
_CONSTEXPR11 inline bool SafeSubtract( T t, U u, T& result ) SAFEINT_NOTHROW
{
    return SubtractionHelper< T, U, SubtractionMethod< T, U >::method >::Subtract( t, u, result );
}

template < typename T >
_CONSTEXPR11 inline bool SafeNegation(T t, T& result) SAFEINT_NOTHROW
{
    return NegationHelper< T, std::numeric_limits<T>::is_signed>::Negative(t, result);
}

/*****************  end external functions ************************************/

// Main SafeInt class
// Assumes exceptions can be thrown
template < typename T, typename E = SafeIntDefaultExceptionHandler > class SafeInt
{
public:
    _CONSTEXPR11 SafeInt() SAFEINT_NOTHROW : m_int(0)
    {
        static_assert( safeint_internal::numeric_type< T >::isInt, "Integer type required" );
    }

    // Having a constructor for every type of int
    // avoids having the compiler evade our checks when doing implicit casts -
    // e.g., SafeInt<char> s = 0x7fffffff;
    _CONSTEXPR11 SafeInt( const T& i ) SAFEINT_NOTHROW : m_int(i)
    {
        static_assert(safeint_internal::numeric_type< T >::isInt, "Integer type required");
        //always safe
    }

    // provide explicit boolean converter
    _CONSTEXPR11 SafeInt( bool b ) SAFEINT_NOTHROW : m_int((T)(b ? 1 : 0))
    {
        static_assert(safeint_internal::numeric_type< T >::isInt, "Integer type required");
    }

    template < typename U >
    _CONSTEXPR14 SafeInt(const SafeInt< U, E >& u) SAFEINT_CPP_THROW : m_int(0)
    {
        static_assert(safeint_internal::numeric_type< T >::isInt, "Integer type required");
        m_int = (T)SafeInt< T, E >( (U)u );
    }

    template < typename U >
    _CONSTEXPR14 SafeInt( const U& i ) SAFEINT_CPP_THROW : m_int(0)
    {
        // m_int must be initialized to something to work with constexpr, because if it throws, then m_int is unknown
        static_assert(safeint_internal::numeric_type< T >::isInt, "Integer type required");
        // SafeCast will throw exceptions if i won't fit in type T
        
        SafeCastHelper< T, U, GetCastMethod< T, U >::method >::template CastThrow< E >( i, m_int );
    }

    // The destructor is intentionally commented out - no destructor
    // vs. a do-nothing destructor makes a huge difference in
    // inlining characteristics. It wasn't doing anything anyway.
    // ~SafeInt(){};


    // now start overloading operators
    // assignment operator
    // constructors exist for all int types and will ensure safety

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator =( const U& rhs ) SAFEINT_CPP_THROW
    {
        // use constructor to test size
        // constructor is optimized to do minimal checking based
        // on whether T can contain U
        // note - do not change this
        m_int = SafeInt< T, E >( rhs );
        return *this;
    }

    _CONSTEXPR14 SafeInt< T, E >& operator =( const T& rhs ) SAFEINT_NOTHROW
    {
        m_int = rhs;
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator =( const SafeInt< U, E >& rhs ) SAFEINT_CPP_THROW
    {
        SafeCastHelper< T, U, GetCastMethod< T, U >::method >::template CastThrow< E >( rhs.Ref(), m_int );
        return *this;
    }

    _CONSTEXPR14 SafeInt< T, E >& operator =( const SafeInt< T, E >& rhs ) SAFEINT_NOTHROW
    {
        m_int = rhs.m_int;
        return *this;
    }

    // Casting operators

    _CONSTEXPR11 operator bool() const SAFEINT_NOTHROW
    {
        return !!m_int;
    }

    _CONSTEXPR14 operator char() const SAFEINT_CPP_THROW
    {
        char val = 0;
        SafeCastHelper< char, T, GetCastMethod< char, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }

    _CONSTEXPR14 operator signed char() const SAFEINT_CPP_THROW
    {
        signed char val = 0;
        SafeCastHelper< signed char, T, GetCastMethod< signed char, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }

    _CONSTEXPR14 operator unsigned char() const SAFEINT_CPP_THROW
    {
        unsigned char val = 0;
        SafeCastHelper< unsigned char, T, GetCastMethod< unsigned char, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }

    _CONSTEXPR14 operator short() const SAFEINT_CPP_THROW
    {
        short val = 0;
        SafeCastHelper< short, T, GetCastMethod< short, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }

    _CONSTEXPR14 operator unsigned short() const SAFEINT_CPP_THROW
    {
        unsigned short val = 0;
        SafeCastHelper< unsigned short, T, GetCastMethod< unsigned short, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }

    _CONSTEXPR14 operator int() const SAFEINT_CPP_THROW
    {
        int val = 0;
        SafeCastHelper< int, T, GetCastMethod< int, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }

    _CONSTEXPR14 operator unsigned int() const SAFEINT_CPP_THROW
    {
        unsigned int val = 0;
        SafeCastHelper< unsigned int, T, GetCastMethod< unsigned int, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }

    // The compiler knows that int == std::int32_t
    // but not that long == std::int32_t, because on some systems, long == std::int64_t
    _CONSTEXPR14 operator long() const SAFEINT_CPP_THROW
    {
        long val = 0;
        SafeCastHelper< long, T, GetCastMethod< long, T >::method >::template CastThrow< E >( m_int, val );
        return  val;
    }

    _CONSTEXPR14 operator unsigned long() const SAFEINT_CPP_THROW
    {
        unsigned long val = 0;
        SafeCastHelper< unsigned long, T, GetCastMethod< unsigned long, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }

    _CONSTEXPR14 operator long long() const SAFEINT_CPP_THROW
    {
        long long val = 0;
        SafeCastHelper< long long, T, GetCastMethod< long long, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }

    _CONSTEXPR14 operator unsigned long long() const SAFEINT_CPP_THROW
    {
        unsigned long long val = 0;
        SafeCastHelper< unsigned long long, T, GetCastMethod< unsigned long long, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }

#if !defined(_MSC_VER) || defined(NATIVE_WCHAR_T_DEFINED)
    _CONSTEXPR14 operator wchar_t() const SAFEINT_CPP_THROW
    {
        wchar_t val = 0;
        SafeCastHelper< wchar_t, T, GetCastMethod< wchar_t, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }
#endif    

#ifdef SIZE_T_CAST_NEEDED
    // We also need an explicit cast to size_t, or the compiler will complain
    // Apparently, only SOME compilers complain, and cl 14.00.50727.42 isn't one of them
    // Leave here in case we decide to backport this to an earlier compiler
    _CONSTEXPR14 operator size_t() const SAFEINT_CPP_THROW
    {
        size_t val = 0;
        SafeCastHelper< size_t, T, GetCastMethod< size_t, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }
#endif

    // Also provide a cast operator for floating point types
    _CONSTEXPR14 operator float() const SAFEINT_CPP_THROW
    {
        float val = 0.0;
        SafeCastHelper< float, T, GetCastMethod< float, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }

    _CONSTEXPR14 operator double() const SAFEINT_CPP_THROW
    {
        double val = 0.0;
        SafeCastHelper< double, T, GetCastMethod< double, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }
    _CONSTEXPR14 operator long double() const SAFEINT_CPP_THROW
    {
        long double val = 0.0;
        SafeCastHelper< long double, T, GetCastMethod< long double, T >::method >::template CastThrow< E >( m_int, val );
        return val;
    }

    // If you need a pointer to the data
    // this could be dangerous, but allows you to correctly pass
    // instances of this class to APIs that take a pointer to an integer
    // also see overloaded address-of operator below
    T* Ptr() SAFEINT_NOTHROW { return &m_int; }
    const T* Ptr() const SAFEINT_NOTHROW { return &m_int; }

    // The above are not modern naming conventions, introducing these
    // to move forward with:
    T* data_ptr() SAFEINT_NOTHROW { return &m_int; }
    const T* data_ptr() const SAFEINT_NOTHROW { return &m_int; }

    // This method is antiquated, and really only makes sense with 
    // 64-bit values on a 32-bit processor. Leaving it for now, in case
    // someone is using it. A better approach is to just unbox it by casting
    // it back to the base type as static_cast<T>( my_safeint )
    _CONSTEXPR14 const T& Ref() const SAFEINT_NOTHROW { return m_int; }

#if !defined SAFEINT_DISABLE_ADDRESS_OPERATOR
    // Note - in the future, this is slated for deprecation

    // Or if SafeInt< T, E >::Ptr() is inconvenient, use the overload
    // operator &
    // This allows you to do unsafe things!
    // It is meant to allow you to more easily
    // pass a SafeInt into things like ReadFile
    T* operator &() SAFEINT_NOTHROW { return &m_int; }
    const T* operator &() const SAFEINT_NOTHROW { return &m_int; }
#endif

    // Unary operators
    _CONSTEXPR11 bool operator !() const SAFEINT_NOTHROW { return (!m_int) ? true : false; }

    // operator + (unary)
    // note - normally, the '+' and '-' operators will upcast to a signed int
    // for T < 32 bits. This class changes behavior to preserve type
    _CONSTEXPR11 const SafeInt< T, E >& operator +() const SAFEINT_NOTHROW { return *this; }

    //unary  -

    _CONSTEXPR14 SafeInt< T, E > operator -() const SAFEINT_CPP_THROW
    {
        // Note - unsigned still performs the bitwise manipulation
        // will warn at level 2 or higher if the value is 32-bit or larger
        return SafeInt<T, E>(NegationHelper<T, std::numeric_limits< T >::is_signed>::template NegativeThrow<E>(m_int));
    }

    // prefix increment operator
    _CONSTEXPR14 SafeInt< T, E >& operator ++() SAFEINT_CPP_THROW
    {
        if( m_int != std::numeric_limits<T>::max() )
        {
            ++m_int;
            return *this;
        }
        E::SafeIntOnOverflow();
    }

    // prefix decrement operator
    _CONSTEXPR14 SafeInt< T, E >& operator --() SAFEINT_CPP_THROW
    {
        if( m_int != std::numeric_limits<T>::min() )
        {
            --m_int;
            return *this;
        }
        E::SafeIntOnOverflow();
    }

    // note that postfix operators have inherently worse perf
    // characteristics

    // postfix increment operator
    _CONSTEXPR14 SafeInt< T, E > operator ++( int )  SAFEINT_CPP_THROW // dummy arg to comply with spec
    {
        if( m_int != std::numeric_limits<T>::max() )
        {
            SafeInt< T, E > tmp( m_int );

            m_int++;
            return tmp;
        }
        E::SafeIntOnOverflow();
    }

    // postfix decrement operator
    _CONSTEXPR14 SafeInt< T, E > operator --( int ) SAFEINT_CPP_THROW // dummy arg to comply with spec
    {
        if( m_int != std::numeric_limits<T>::min() )
        {
            SafeInt< T, E > tmp( m_int );
            m_int--;
            return tmp;
        }
        E::SafeIntOnOverflow();
    }

    // One's complement
    // Note - this operator will normally change size to an int
    // cast in return improves perf and maintains type
    _CONSTEXPR11 SafeInt< T, E > operator ~() const SAFEINT_NOTHROW { return SafeInt< T, E >( (T)~m_int ); }

    // Binary operators
    //
    // arithmetic binary operators
    // % modulus
    // * multiplication
    // / division
    // + addition
    // - subtraction
    //
    // For each of the arithmetic operators, you will need to
    // use them as follows:
    //
    // SafeInt<char> c = 2;
    // SafeInt<int>  i = 3;
    //
    // SafeInt<int> i2 = i op (char)c;
    // OR
    // SafeInt<char> i2 = (int)i op c;
    //
    // The base problem is that if the lhs and rhs inputs are different SafeInt types
    // it is not possible in this implementation to determine what type of SafeInt
    // should be returned. You have to let the class know which of the two inputs
    // need to be the return type by forcing the other value to the base integer type.
    //
    // Note - as per feedback from Scott Meyers, I'm exploring how to get around this.
    // 3.0 update - I'm still thinking about this. It can be done with template metaprogramming,
    // but it is tricky, and there's a perf vs. correctness tradeoff where the right answer
    // is situational.
    //
    // The case of:
    //
    // SafeInt< T, E > i, j, k;
    // i = j op k;
    //
    // works just fine and no unboxing is needed because the return type is not ambiguous.

    // Modulus
    // Modulus has some convenient properties -
    // first, the magnitude of the return can never be
    // larger than the lhs operand, and it must be the same sign
    // as well. It does, however, suffer from the same promotion
    // problems as comparisons, division and other operations
    template < typename U >
    _CONSTEXPR14 SafeInt< T, E > operator %( U rhs ) const SAFEINT_CPP_THROW
    {
        T result = 0;
        ModulusHelper< T, U, ValidComparison< T, U >::method >::template ModulusThrow< E >( m_int, rhs, result );
        return SafeInt< T, E >( result );
    }

    _CONSTEXPR14 SafeInt< T, E > operator %( SafeInt< T, E > rhs ) const SAFEINT_CPP_THROW
    {
        T result = 0;
        ModulusHelper< T, T, ValidComparison< T, T >::method >::template ModulusThrow< E >( m_int, rhs, result );
        return SafeInt< T, E >( result );
    }

    // Modulus assignment
    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator %=( U rhs ) SAFEINT_CPP_THROW
    {
        ModulusHelper< T, U, ValidComparison< T, U >::method >::template ModulusThrow< E >( m_int, rhs, m_int );
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator %=( SafeInt< U, E > rhs ) SAFEINT_CPP_THROW
    {
        ModulusHelper< T, U, ValidComparison< T, U >::method >::template ModulusThrow< E >( m_int, (U)rhs, m_int );
        return *this;
    }

    // Multiplication
    template < typename U >
    _CONSTEXPR14_MULTIPLY SafeInt< T, E > operator *( U rhs ) const SAFEINT_CPP_THROW
    {
        T ret( 0 );
        MultiplicationHelper< T, U, MultiplicationMethod< T, U >::method >::template MultiplyThrow< E >( m_int, rhs, ret );
        return SafeInt< T, E >( ret );
    }

    _CONSTEXPR14 SafeInt< T, E > operator *( SafeInt< T, E > rhs ) const SAFEINT_CPP_THROW
    {
        T ret( 0 );
        MultiplicationHelper< T, T, MultiplicationMethod< T, T >::method >::template MultiplyThrow< E >( m_int, (T)rhs, ret );
        return SafeInt< T, E >( ret );
    }

    // Multiplication assignment
    _CONSTEXPR14 SafeInt< T, E >& operator *=( SafeInt< T, E > rhs ) SAFEINT_CPP_THROW
    {
        MultiplicationHelper< T, T, MultiplicationMethod< T, T >::method >::template MultiplyThrow< E >( m_int, (T)rhs, m_int );
        return *this;
    }

    template < typename U >
    _CONSTEXPR14_MULTIPLY SafeInt< T, E >& operator *=( U rhs ) SAFEINT_CPP_THROW
    {
        MultiplicationHelper< T, U, MultiplicationMethod< T, U >::method >::template MultiplyThrow< E >( m_int, rhs, m_int );
        return *this;
    }

    template < typename U >
    _CONSTEXPR14_MULTIPLY SafeInt< T, E >& operator *=( SafeInt< U, E > rhs ) SAFEINT_CPP_THROW
    {
        MultiplicationHelper< T, U, MultiplicationMethod< T, U >::method >::template MultiplyThrow< E >( m_int, rhs.Ref(), m_int );
        return *this;
    }

    // Division
    template < typename U >
    _CONSTEXPR14 SafeInt< T, E > operator /( U rhs ) const SAFEINT_CPP_THROW
    {
        T ret( 0 );
        DivisionHelper< T, U, DivisionMethod< T, U >::method >::template DivideThrow< E >( m_int, rhs, ret );
        return SafeInt< T, E >( ret );
    }

    _CONSTEXPR14 SafeInt< T, E > operator /( SafeInt< T, E > rhs ) const SAFEINT_CPP_THROW
    {
        T ret( 0 );
        DivisionHelper< T, T, DivisionMethod< T, T >::method >::template DivideThrow< E >( m_int, (T)rhs, ret );
        return SafeInt< T, E >( ret );
    }

    // Division assignment
    _CONSTEXPR14 SafeInt< T, E >& operator /=( SafeInt< T, E > i ) SAFEINT_CPP_THROW
    {
        DivisionHelper< T, T, DivisionMethod< T, T >::method >::template DivideThrow< E >( m_int, (T)i, m_int );
        return *this;
    }

    template < typename U > 
    _CONSTEXPR14 SafeInt< T, E >& operator /=( U i ) SAFEINT_CPP_THROW
    {
        DivisionHelper< T, U, DivisionMethod< T, U >::method >::template DivideThrow< E >( m_int, i, m_int );
        return *this;
    }

    template < typename U > 
    _CONSTEXPR14 SafeInt< T, E >& operator /=( SafeInt< U, E > i )
    {
        DivisionHelper< T, U, DivisionMethod< T, U >::method >::template DivideThrow< E >( m_int, (U)i, m_int );
        return *this;
    }

    // For addition and subtraction

    // Addition
    _CONSTEXPR14 SafeInt< T, E > operator +( SafeInt< T, E > rhs ) const SAFEINT_CPP_THROW
    {
        T ret( 0 );
        AdditionHelper< T, T, AdditionMethod< T, T >::method >::template AdditionThrow< E >( m_int, (T)rhs, ret );
        return SafeInt< T, E >( ret );
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E > operator +( U rhs ) const SAFEINT_CPP_THROW
    {
        T ret( 0 );
        AdditionHelper< T, U, AdditionMethod< T, U >::method >::template AdditionThrow< E >( m_int, rhs, ret );
        return SafeInt< T, E >( ret );
    }

    //addition assignment
    _CONSTEXPR14 SafeInt< T, E >& operator +=( SafeInt< T, E > rhs ) SAFEINT_CPP_THROW
    {
        AdditionHelper< T, T, AdditionMethod< T, T >::method >::template AdditionThrow< E >( m_int, (T)rhs, m_int );
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator +=( U rhs ) SAFEINT_CPP_THROW
    {
        AdditionHelper< T, U, AdditionMethod< T, U >::method >::template AdditionThrow< E >( m_int, rhs, m_int );
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator +=( SafeInt< U, E > rhs ) SAFEINT_CPP_THROW
    {
        AdditionHelper< T, U, AdditionMethod< T, U >::method >::template AdditionThrow< E >( m_int, (U)rhs, m_int );
        return *this;
    }

    // Subtraction
    template < typename U >
    _CONSTEXPR14 SafeInt< T, E > operator -( U rhs ) const SAFEINT_CPP_THROW
    {
        T ret( 0 );
        SubtractionHelper< T, U, SubtractionMethod< T, U >::method >::template SubtractThrow< E >( m_int, rhs, ret );
        return SafeInt< T, E >( ret );
    }

    _CONSTEXPR14 SafeInt< T, E > operator -(SafeInt< T, E > rhs) const SAFEINT_CPP_THROW
    {
        T ret( 0 );
        SubtractionHelper< T, T, SubtractionMethod< T, T >::method >::template SubtractThrow< E >( m_int, (T)rhs, ret );
        return SafeInt< T, E >( ret );
    }

    // Subtraction assignment
    _CONSTEXPR14 SafeInt< T, E >& operator -=( SafeInt< T, E > rhs ) SAFEINT_CPP_THROW
    {
        SubtractionHelper< T, T, SubtractionMethod< T, T >::method >::template SubtractThrow< E >( m_int, (T)rhs, m_int );
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator -=( U rhs ) SAFEINT_CPP_THROW
    {
        SubtractionHelper< T, U, SubtractionMethod< T, U >::method >::template SubtractThrow< E >( m_int, rhs, m_int );
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator -=( SafeInt< U, E > rhs ) SAFEINT_CPP_THROW
    {
        SubtractionHelper< T, U, SubtractionMethod< T, U >::method >::template SubtractThrow< E >( m_int, (U)rhs, m_int );
        return *this;
    }

    // Shift operators
    // Note - shift operators ALWAYS return the same type as the lhs
    // specific version for SafeInt< T, E > not needed -
    // code path is exactly the same as for SafeInt< U, E > as rhs

    // Left shift
    // Also, shifting > bitcount is undefined - trap in debug
    #define ShiftAssert(x) SAFEINT_ASSERT(x)

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E > operator <<( U bits ) const SAFEINT_NOTHROW
    {
        ShiftAssert( !std::numeric_limits< U >::is_signed || bits >= 0 );
        ShiftAssert( bits < (int)safeint_internal::int_traits< T >::bitCount );

        return SafeInt< T, E >( (T)( m_int << bits ) );
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E > operator <<( SafeInt< U, E > bits ) const SAFEINT_NOTHROW
    {
        ShiftAssert( !std::numeric_limits< U >::is_signed || (U)bits >= 0 );
        ShiftAssert( (U)bits < (int)safeint_internal::int_traits< T >::bitCount );

        return SafeInt< T, E >( (T)( m_int << (U)bits ) );
    }

    // Left shift assignment

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator <<=( U bits ) SAFEINT_NOTHROW
    {
        ShiftAssert( !std::numeric_limits< U >::is_signed || bits >= 0 );
        ShiftAssert( bits < (int)safeint_internal::int_traits< T >::bitCount );

        m_int <<= bits;
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator <<=( SafeInt< U, E > bits ) SAFEINT_NOTHROW
    {
        ShiftAssert( !std::numeric_limits< U >::is_signed || (U)bits >= 0 );
        ShiftAssert( (U)bits < (int)safeint_internal::int_traits< T >::bitCount );

        m_int <<= (U)bits;
        return *this;
    }

    // Right shift
    template < typename U >
    _CONSTEXPR14 SafeInt< T, E > operator >>( U bits ) const SAFEINT_NOTHROW
    {
        ShiftAssert( !std::numeric_limits< U >::is_signed || bits >= 0 );
        ShiftAssert( bits < (int)safeint_internal::int_traits< T >::bitCount );

        return SafeInt< T, E >( (T)( m_int >> bits ) );
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E > operator >>( SafeInt< U, E > bits ) const SAFEINT_NOTHROW
    {
        ShiftAssert( !std::numeric_limits< U >::is_signed || (U)bits >= 0 );
        ShiftAssert( (U)bits < (int)safeint_internal::int_traits< T >::bitCount );

        return SafeInt< T, E >( (T)(m_int >> (U)bits) );
    }

    // Right shift assignment
    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator >>=( U bits ) SAFEINT_NOTHROW
    {
        ShiftAssert( !std::numeric_limits< U >::is_signed || bits >= 0 );
        ShiftAssert( bits < (int)safeint_internal::int_traits< T >::bitCount );

        m_int >>= bits;
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator >>=( SafeInt< U, E > bits ) SAFEINT_NOTHROW
    {
        ShiftAssert( !std::numeric_limits< U >::is_signed || (U)bits >= 0 );
        ShiftAssert( (U)bits < (int)safeint_internal::int_traits< T >::bitCount );

        m_int >>= (U)bits;
        return *this;
    }

    // Bitwise operators
    // This only makes sense if we're dealing with the same type and size
    // demand a type T, or something that fits into a type T

    // Bitwise &
    _CONSTEXPR14 SafeInt< T, E > operator &( SafeInt< T, E > rhs ) const SAFEINT_NOTHROW
    {
        return SafeInt< T, E >( m_int & (T)rhs );
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E > operator &( U rhs ) const SAFEINT_NOTHROW
    {
        // we want to avoid setting bits by surprise
        // consider the case of lhs = int, value = 0xffffffff
        //                      rhs = char, value = 0xff
        //
        // programmer intent is to get only the lower 8 bits
        // normal behavior is to upcast both sides to an int
        // which then sign extends rhs, setting all the bits

        // If you land in the assert, this is because the bitwise operator
        // was causing unexpected behavior. Fix is to properly cast your inputs
        // so that it works like you meant, not unexpectedly

        return SafeInt< T, E >( BinaryAndHelper< T, U, BinaryMethod< T, U >::method >::And( m_int, rhs ) );
    }

    // Bitwise & assignment
    _CONSTEXPR14 SafeInt< T, E >& operator &=( SafeInt< T, E > rhs ) SAFEINT_NOTHROW
    {
        m_int &= (T)rhs;
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator &=( U rhs ) SAFEINT_NOTHROW
    {
        m_int = BinaryAndHelper< T, U, BinaryMethod< T, U >::method >::And( m_int, rhs );
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator &=( SafeInt< U, E > rhs ) SAFEINT_NOTHROW
    {
        m_int = BinaryAndHelper< T, U, BinaryMethod< T, U >::method >::And( m_int, (U)rhs );
        return *this;
    }

    // XOR
    _CONSTEXPR14 SafeInt< T, E > operator ^( SafeInt< T, E > rhs ) const SAFEINT_NOTHROW
    {
        return SafeInt< T, E >( (T)( m_int ^ (T)rhs ) );
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E > operator ^( U rhs ) const SAFEINT_NOTHROW
    {
        // If you land in the assert, this is because the bitwise operator
        // was causing unexpected behavior. Fix is to properly cast your inputs
        // so that it works like you meant, not unexpectedly

        return SafeInt< T, E >( BinaryXorHelper< T, U, BinaryMethod< T, U >::method >::Xor( m_int, rhs ) );
    }

    // XOR assignment
    _CONSTEXPR14 SafeInt< T, E >& operator ^=( SafeInt< T, E > rhs ) SAFEINT_NOTHROW
    {
        m_int ^= (T)rhs;
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator ^=( U rhs ) SAFEINT_NOTHROW
    {
        m_int = BinaryXorHelper< T, U, BinaryMethod< T, U >::method >::Xor( m_int, rhs );
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator ^=( SafeInt< U, E > rhs ) SAFEINT_NOTHROW
    {
        m_int = BinaryXorHelper< T, U, BinaryMethod< T, U >::method >::Xor( m_int, (U)rhs );
        return *this;
    }

    // bitwise OR
    _CONSTEXPR14 SafeInt< T, E > operator |( SafeInt< T, E > rhs ) const SAFEINT_NOTHROW
    {
        return SafeInt< T, E >( (T)( m_int | (T)rhs ) );
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E > operator |( U rhs ) const SAFEINT_NOTHROW
    {
        return SafeInt< T, E >( BinaryOrHelper< T, U, BinaryMethod< T, U >::method >::Or( m_int, rhs ) );
    }

    // bitwise OR assignment
    _CONSTEXPR14 SafeInt< T, E >& operator |=( SafeInt< T, E > rhs ) SAFEINT_NOTHROW
    {
        m_int |= (T)rhs;
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator |=( U rhs ) SAFEINT_NOTHROW
    {
        m_int = BinaryOrHelper< T, U, BinaryMethod< T, U >::method >::Or( m_int, rhs );
        return *this;
    }

    template < typename U >
    _CONSTEXPR14 SafeInt< T, E >& operator |=( SafeInt< U, E > rhs ) SAFEINT_NOTHROW
    {
        m_int = BinaryOrHelper< T, U, BinaryMethod< T, U >::method >::Or( m_int, (U)rhs );
        return *this;
    }

    // Miscellaneous helper functions
    SafeInt< T, E > Min( SafeInt< T, E > test, const T floor = std::numeric_limits<T>::min() ) const SAFEINT_NOTHROW
    {
        T tmp = test < m_int ? (T)test : m_int;
        return tmp < floor ? floor : tmp;
    }

    SafeInt< T, E > Max( SafeInt< T, E > test, const T upper = std::numeric_limits<T>::max() ) const SAFEINT_NOTHROW
    {
        T tmp = test > m_int ? (T)test : m_int;
        return tmp > upper ? upper : tmp;
    }

    void Swap( SafeInt< T, E >& with ) SAFEINT_NOTHROW
    {
        T temp( m_int );
        m_int = with.m_int;
        with.m_int = temp;
    }

    static SafeInt< T, E > SafeAtoI( const char* input ) SAFEINT_CPP_THROW
    {
        return SafeTtoI( input );
    }

    static SafeInt< T, E > SafeWtoI( const wchar_t* input )
    {
        return SafeTtoI( input );
    }

    enum alignBits
    {
        align2 = 1,
        align4 = 2,
        align8 = 3,
        align16 = 4,
        align32 = 5,
        align64 = 6,
        align128 = 7,
        align256 = 8
    };

    template < alignBits bits >
    const SafeInt< T, E >& Align() SAFEINT_CPP_THROW
    {
        // Zero is always aligned
        if( m_int == 0 )
            return *this;

        // We don't support aligning negative numbers at this time
        // Can't align unsigned numbers on bitCount (e.g., 8 bits = 256, unsigned char max = 255)
        // or signed numbers on bitCount-1 (e.g., 7 bits = 128, signed char max = 127).
        // Also makes no sense to try to align on negative or no bits.

        ShiftAssert( ( ( std::numeric_limits< T >::is_signed && bits < (int)safeint_internal::int_traits< T >::bitCount - 1 )
                    || ( !std::numeric_limits< T >::is_signed && bits < (int)safeint_internal::int_traits< T >::bitCount ) ) &&
                    bits >= 0 && ( !std::numeric_limits< T >::is_signed || m_int > 0 ) );

        const T AlignValue = ( (T)1 << bits ) - 1;

        m_int = (T)( ( m_int + AlignValue ) & ~AlignValue );

        if( m_int <= 0 )
            E::SafeIntOnOverflow();

        return *this;
    }

    // Commonly needed alignments:
    const SafeInt< T, E >& Align2()  { return Align< align2 >(); }
    const SafeInt< T, E >& Align4()  { return Align< align4 >(); }
    const SafeInt< T, E >& Align8()  { return Align< align8 >(); }
    const SafeInt< T, E >& Align16() { return Align< align16 >(); }
    const SafeInt< T, E >& Align32() { return Align< align32 >(); }
    const SafeInt< T, E >& Align64() { return Align< align64 >(); }
private:

    // This is almost certainly not the best optimized version of atoi,
    // but it does not display a typical bug where it isn't possible to set MinInt
    // and it won't allow you to overflow your integer.
    // This is here because it is useful, and it is an example of what
    // can be done easily with SafeInt.
    template < typename U >
    static SafeInt< T, E > SafeTtoI( U* input ) SAFEINT_CPP_THROW
    {
        U* tmp  = input;
        SafeInt< T, E > s;
        bool negative = false;

        // Bad input, or empty string
        if( input == nullptr || input[0] == 0 )
            E::SafeIntOnOverflow();

        switch( *tmp )
        {
        case '-':
            tmp++;
            negative = true;
            break;
        case '+':
            tmp++;
            break;
        }

        while( *tmp != 0 )
        {
            if( *tmp < '0' || *tmp > '9' )
                break;

            if( (T)s != 0 )
                s *= (T)10;

            if( !negative )
                s += (T)( *tmp - '0' );
            else
                s -= (T)( *tmp - '0' );

            tmp++;
        }

        return s;
    }

    T m_int;
};

// Helper function used to subtract pointers.
// Used to squelch warnings
template <typename P>
_CONSTEXPR11 SafeInt<ptrdiff_t, SafeIntDefaultExceptionHandler> SafePtrDiff(const P* p1, const P* p2) SAFEINT_CPP_THROW
{
    return SafeInt<ptrdiff_t, SafeIntDefaultExceptionHandler>( p1 - p2 );
}

// Comparison operators

//Less than
template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator <( U lhs, SafeInt< T, E > rhs ) SAFEINT_NOTHROW
{
    return GreaterThanTest< T, U, ValidComparison< T, U >::method >::GreaterThan( (T)rhs, lhs );
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator <( SafeInt<T, E> lhs, U rhs ) SAFEINT_NOTHROW
{
    return GreaterThanTest< U, T, ValidComparison< U, T >::method >::GreaterThan( rhs, (T)lhs );
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator <( SafeInt< U, E > lhs, SafeInt< T, E > rhs ) SAFEINT_NOTHROW
{
    return GreaterThanTest< T, U, ValidComparison< T, U >::method >::GreaterThan( (T)rhs, (U)lhs );
}

// Greater than
template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator >( U lhs, SafeInt< T, E > rhs ) SAFEINT_NOTHROW
{
    return GreaterThanTest< U, T, ValidComparison< U, T >::method >::GreaterThan( lhs, (T)rhs );
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator >( SafeInt<T, E> lhs, U rhs ) SAFEINT_NOTHROW
{
    return GreaterThanTest< T, U, ValidComparison< T, U >::method >::GreaterThan( (T)lhs, rhs );
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator >( SafeInt< T, E > lhs, SafeInt< U, E > rhs ) SAFEINT_NOTHROW
{
    return GreaterThanTest< T, U, ValidComparison< T, U >::method >::GreaterThan( (T)lhs, (U)rhs );
}

// Greater than or equal
template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator >=( U lhs, SafeInt< T, E > rhs ) SAFEINT_NOTHROW
{
    return !GreaterThanTest< T, U, ValidComparison< T, U >::method >::GreaterThan( (T)rhs, lhs );
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator >=( SafeInt<T, E> lhs, U rhs ) SAFEINT_NOTHROW
{
    return !GreaterThanTest< U, T, ValidComparison< U, T >::method >::GreaterThan( rhs, (T)lhs );
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator >=( SafeInt< T, E > lhs, SafeInt< U, E > rhs ) SAFEINT_NOTHROW
{
    return !GreaterThanTest< U, T, ValidComparison< U, T >::method >::GreaterThan( (U)rhs, (T)lhs );
}

// Less than or equal
template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator <=( U lhs, SafeInt< T, E > rhs ) SAFEINT_NOTHROW
{
    return !GreaterThanTest< U, T, ValidComparison< U, T >::method >::GreaterThan( lhs, (T)rhs );
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator <=( SafeInt< T, E > lhs, U rhs ) SAFEINT_NOTHROW
{
    return !GreaterThanTest< T, U, ValidComparison< T, U >::method >::GreaterThan( (T)lhs, rhs );
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator <=( SafeInt< T, E > lhs, SafeInt< U, E > rhs ) SAFEINT_NOTHROW
{
    return !GreaterThanTest< T, U, ValidComparison< T, U >::method >::GreaterThan( (T)lhs, (U)rhs );
}

// equality
// explicit overload for bool
template < typename T, typename E >
_CONSTEXPR11 bool operator ==( bool lhs, SafeInt< T, E > rhs ) SAFEINT_NOTHROW
{
    return lhs == ( (T)rhs == 0 ? false : true );
}

template < typename T, typename E >
_CONSTEXPR11 bool operator ==( SafeInt< T, E > lhs, bool rhs ) SAFEINT_NOTHROW
{
    return rhs == ( (T)lhs == 0 ? false : true );
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator ==( U lhs, SafeInt< T, E > rhs ) SAFEINT_NOTHROW
{
    return EqualityTest< T, U, ValidComparison< T, U >::method >::IsEquals((T)rhs, lhs);
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator ==( SafeInt< T, E > lhs, U rhs ) SAFEINT_NOTHROW
{
    return EqualityTest< T, U, ValidComparison< T, U >::method >::IsEquals( (T)lhs, rhs );
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator ==( SafeInt< T, E > lhs, SafeInt< U, E > rhs ) SAFEINT_NOTHROW
{
    return EqualityTest< T, U, ValidComparison< T, U >::method >::IsEquals( (T)lhs, (U)rhs );
}

//not equals
template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator !=( U lhs, SafeInt< T, E > rhs ) SAFEINT_NOTHROW
{
    return !EqualityTest< T, U, ValidComparison< T, U >::method >::IsEquals( (T)rhs, lhs );
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator !=( SafeInt< T, E > lhs, U rhs ) SAFEINT_NOTHROW
{
    return !EqualityTest< T, U, ValidComparison< T, U >::method >::IsEquals( (T)lhs, rhs );
}

template < typename T, typename U, typename E >
_CONSTEXPR11 bool operator !=( SafeInt< T, E > lhs, SafeInt< U, E > rhs ) SAFEINT_NOTHROW
{
    return !EqualityTest< T, U, ValidComparison< T, U >::method >::IsEquals( lhs, rhs );
}


template < typename T, typename E >
_CONSTEXPR11 bool operator !=( bool lhs, SafeInt< T, E > rhs ) SAFEINT_NOTHROW
{
    return ( (T)rhs == 0 ? false : true ) != lhs;
}

template < typename T, typename E >
_CONSTEXPR11 bool operator !=( SafeInt< T, E > lhs, bool rhs ) SAFEINT_NOTHROW
{
    return ( (T)lhs == 0 ? false : true ) != rhs;
}


template < typename T, typename U, typename E, int method > class ModulusSimpleCaseHelper;

template < typename T, typename E, int method > class ModulusSignedCaseHelper;

template < typename T, typename E > class ModulusSignedCaseHelper < T, E, true >
{
public:
    _CONSTEXPR14 static bool SignedCase( SafeInt< T, E > rhs, SafeInt< T, E >& result ) SAFEINT_NOTHROW
    {
        if( (T)rhs == (T)-1 )
        {
            result = 0;
            return true;
        }
        return false;
    }
};

template < typename T, typename E > class ModulusSignedCaseHelper < T, E, false >
{
public:
    _CONSTEXPR11 static bool SignedCase( SafeInt< T, E > /*rhs*/, SafeInt< T, E >& /*result*/ ) SAFEINT_NOTHROW
    {
        return false;
    }
};

template < typename T, typename U, typename E >
class ModulusSimpleCaseHelper < T, U, E, true >
{
public:
    _CONSTEXPR14 static bool ModulusSimpleCase( U lhs, SafeInt< T, E > rhs, SafeInt< T, E >& result ) SAFEINT_CPP_THROW
    {
        if( rhs != 0 )
        {
            if( ModulusSignedCaseHelper< T, E, std::numeric_limits< T >::is_signed >::SignedCase( rhs, result ) )
            return true;

            result = (T)( lhs % (T)rhs );
            return true;
        }

        E::SafeIntOnDivZero();
    }
};

template< typename T, typename U, typename E >
class ModulusSimpleCaseHelper < T, U, E, false >
{
public:
    _CONSTEXPR11 static bool ModulusSimpleCase( U /*lhs*/, SafeInt< T, E > /*rhs*/, SafeInt< T, E >& /*result*/ ) SAFEINT_NOTHROW
    {
        return false;
    }
};

// Modulus
template < typename T, typename U, typename E >
_CONSTEXPR14 SafeInt< T, E > operator %( U lhs, SafeInt< T, E > rhs ) SAFEINT_CPP_THROW
{
    // Value of return depends on sign of lhs
    // This one may not be safe - bounds check in constructor
    // if lhs is negative and rhs is unsigned, this will throw an exception.

    // Fast-track the simple case
    // same size and same sign
    SafeInt< T, E > result;

    if( ModulusSimpleCaseHelper< T, U, E, (sizeof(T) == sizeof(U)) && ((bool)std::numeric_limits< T >::is_signed == (bool)std::numeric_limits< U >::is_signed) >::ModulusSimpleCase( lhs, rhs, result ) )
        return result;

    result = (SafeInt< U, E >(lhs) % (T)rhs);
    return result;
}

// Multiplication
template < typename T, typename U, typename E >
_CONSTEXPR14_MULTIPLY SafeInt< T, E > operator *( U lhs, SafeInt< T, E > rhs ) SAFEINT_CPP_THROW
{
    T ret( 0 );
    MultiplicationHelper< T, U, MultiplicationMethod< T, U >::method >::template MultiplyThrow< E >( (T)rhs, lhs, ret );
    return SafeInt< T, E >(ret);
}

template < typename T, typename U, typename E, int method > class DivisionNegativeCornerCaseHelper;

template < typename T, typename U, bool > class division_negative_negateU;

template < typename T, typename U > class division_negative_negateU< T, U, true>
{
public:
    // sizeof(T) == 4
    _CONSTEXPR14 static U div(T rhs, U lhs) { return lhs / (U)(~(std::uint32_t)(T)rhs + 1); }
};

template < typename T, typename U > class division_negative_negateU< T, U, false>
{
public:
    _CONSTEXPR14 static U div(T rhs, U lhs) { return lhs / (U)(~(std::uint64_t)(T)rhs + 1); }
};

template < typename T, typename U, typename E > class DivisionNegativeCornerCaseHelper< T, U, E, true >
{
public:
    static bool NegativeCornerCase( U lhs, SafeInt< T, E > rhs, SafeInt<T, E>& result ) SAFEINT_CPP_THROW
    {
        // Problem case - normal casting behavior changes meaning
        // flip rhs to positive
        // any operator casts now do the right thing
        U tmp = division_negative_negateU< T, U, sizeof(T) == 4>::div(rhs, lhs);

        if( tmp <= (U)std::numeric_limits<T>::max() )
        {
            result = SafeInt< T, E >( (T)(~(std::uint64_t)tmp + 1) );
            return true;
        }

        // Corner case
        T maxT = std::numeric_limits<T>::max();
        if( tmp == (U)maxT + 1 )
        {
            T minT = std::numeric_limits<T>::min();
            result = SafeInt< T, E >( minT );
            return true;
        }

        E::SafeIntOnOverflow();
    }
};

template < typename T, typename U, typename E > class DivisionNegativeCornerCaseHelper< T, U, E, false >
{
public:
    _CONSTEXPR11 static bool NegativeCornerCase( U /*lhs*/, SafeInt< T, E > /*rhs*/, SafeInt<T, E>& /*result*/ ) SAFEINT_NOTHROW
    {
        return false;
    }
};

template < typename T, typename U, typename E, int method > class DivisionCornerCaseHelper;

template < typename T, typename U, typename E > class DivisionCornerCaseHelper < T, U, E, true >
{
public:
    _CONSTEXPR14 static bool DivisionCornerCase1( U lhs, SafeInt< T, E > rhs, SafeInt<T, E>& result ) SAFEINT_CPP_THROW
    {
        if( (T)rhs > 0 )
        {
            result = SafeInt< T, E >( lhs/(T)rhs );
            return true;
        }

        // Now rhs is either negative, or zero
        if( (T)rhs != 0 )
        {
            if( DivisionNegativeCornerCaseHelper< T, U, E, sizeof( U ) >= 4 && sizeof( T ) <= sizeof( U ) >::NegativeCornerCase( lhs, rhs, result ) )
                return true;

            result = SafeInt< T, E >(lhs/(T)rhs);
            return true;
        }

        E::SafeIntOnDivZero();
    }
};

template < typename T, typename U, typename E > class DivisionCornerCaseHelper < T, U, E, false >
{
public:
    _CONSTEXPR11 static bool DivisionCornerCase1( U /*lhs*/, SafeInt< T, E > /*rhs*/, SafeInt<T, E>& /*result*/ ) SAFEINT_NOTHROW
    {
        return false;
    }
};

template < typename T, typename U, typename E, int method > class DivisionCornerCaseHelper2;

template < typename T, typename U, bool > class div_negate_min;

template < typename T, typename U > class div_negate_min < T, U , true >
{
public:
    _CONSTEXPR14 static bool Value(T& ret)
    {
        ret = (T)(-(T)std::numeric_limits< U >::min());
        return true;
    }
};

template < typename T, typename U > class div_negate_min < T, U, false >
{
public:
    _CONSTEXPR14 static bool Value(T& )
    {
        return false;
    }
};

template < typename T, typename U, typename E > class DivisionCornerCaseHelper2 < T, U, E, true >
{
public:
    _CONSTEXPR14 static bool DivisionCornerCase2( U lhs, SafeInt< T, E > rhs, SafeInt<T, E>& result ) SAFEINT_CPP_THROW
    {
        if( lhs == std::numeric_limits< U >::min() && (T)rhs == -1 )
        {
            // corner case of a corner case - lhs = min int, rhs = -1,
            // but rhs is the return type, so in essence, we can return -lhs
            // if rhs is a larger type than lhs
            // If types are wrong, throws
            T tmp = 0;

            if (div_negate_min< T, U, sizeof(U) < sizeof(T) > ::Value(tmp))
                result = tmp;
            else
                E::SafeIntOnOverflow();

            return true;
        }

        return false;
    }
};

template < typename T, typename U, typename E > class DivisionCornerCaseHelper2 < T, U, E, false >
{
public:
    _CONSTEXPR11 static bool DivisionCornerCase2( U /*lhs*/, SafeInt< T, E > /*rhs*/, SafeInt<T, E>& /*result*/ ) SAFEINT_NOTHROW
    {
        return false;
    }
};

// Division
template < typename T, typename U, typename E > 
_CONSTEXPR14 SafeInt< T, E > operator /( U lhs, SafeInt< T, E > rhs ) SAFEINT_CPP_THROW
{
    // Corner case - has to be handled seperately
    SafeInt< T, E > result;
    if( DivisionCornerCaseHelper< T, U, E, (int)DivisionMethod< U, T >::method == (int)DivisionState_UnsignedSigned >::DivisionCornerCase1( lhs, rhs, result ) )
        return result;

    if( DivisionCornerCaseHelper2< T, U, E, safeint_internal::type_compare< T, U >::isBothSigned >::DivisionCornerCase2( lhs, rhs, result ) )
        return result;

    // Otherwise normal logic works with addition of bounds check when casting from U->T
    U ret = 0;
    DivisionHelper< U, T, DivisionMethod< U, T >::method >::template DivideThrow< E >( lhs, (T)rhs, ret );
    return SafeInt< T, E >( ret );
}

// Addition
template < typename T, typename U, typename E >
_CONSTEXPR14 SafeInt< T, E > operator +( U lhs, SafeInt< T, E > rhs ) SAFEINT_CPP_THROW
{
    T ret( 0 );
    AdditionHelper< T, U, AdditionMethod< T, U >::method >::template AdditionThrow< E >( (T)rhs, lhs, ret );
    return SafeInt< T, E >( ret );
}

// Subtraction
template < typename T, typename U, typename E >
_CONSTEXPR14 SafeInt< T, E > operator -( U lhs, SafeInt< T, E > rhs ) SAFEINT_CPP_THROW
{
    T ret( 0 );
    SubtractionHelper< U, T, SubtractionMethod2< U, T >::method >::template SubtractThrow< E >( lhs, rhs.Ref(), ret );

    return SafeInt< T, E >( ret );
}

// Overrides designed to deal with cases where a SafeInt is assigned out
// to a normal int - this at least makes the last operation safe
// +=
template < typename T, typename U, typename E >
_CONSTEXPR14 T& operator +=( T& lhs, SafeInt< U, E > rhs ) SAFEINT_CPP_THROW
{
    T ret( 0 );
    AdditionHelper< T, U, AdditionMethod< T, U >::method >::template AdditionThrow< E >( lhs, (U)rhs, ret );
    lhs = ret;
    return lhs;
}

template < typename T, typename U, typename E >
_CONSTEXPR14 T& operator -=( T& lhs, SafeInt< U, E > rhs ) SAFEINT_CPP_THROW
{
    T ret( 0 );
    SubtractionHelper< T, U, SubtractionMethod< T, U >::method >::template SubtractThrow< E >( lhs, (U)rhs, ret );
    lhs = ret;
    return lhs;
}

template < typename T, typename U, typename E >
_CONSTEXPR14 T& operator *=( T& lhs, SafeInt< U, E > rhs ) SAFEINT_CPP_THROW
{
    T ret( 0 );
    MultiplicationHelper< T, U, MultiplicationMethod< T, U >::method >::template MultiplyThrow< E >( lhs, (U)rhs, ret );
    lhs = ret;
    return lhs;
}

template < typename T, typename U, typename E >
_CONSTEXPR14 T& operator /=( T& lhs, SafeInt< U, E > rhs ) SAFEINT_CPP_THROW
{
    T ret( 0 );
    DivisionHelper< T, U, DivisionMethod< T, U >::method >::template DivideThrow< E >( lhs, (U)rhs, ret );
    lhs = ret;
    return lhs;
}

template < typename T, typename U, typename E >
_CONSTEXPR14 T& operator %=( T& lhs, SafeInt< U, E > rhs ) SAFEINT_CPP_THROW
{
    T ret( 0 );
    ModulusHelper< T, U, ValidComparison< T, U >::method >::template ModulusThrow< E >( lhs, (U)rhs, ret );
    lhs = ret;
    return lhs;
}

template < typename T, typename U, typename E >
_CONSTEXPR14 T& operator &=( T& lhs, SafeInt< U, E > rhs ) SAFEINT_NOTHROW
{
    lhs = BinaryAndHelper< T, U, BinaryMethod< T, U >::method >::And( lhs, (U)rhs );
    return lhs;
}

template < typename T, typename U, typename E >
_CONSTEXPR14 T& operator ^=( T& lhs, SafeInt< U, E > rhs ) SAFEINT_NOTHROW
{
    lhs = BinaryXorHelper< T, U, BinaryMethod< T, U >::method >::Xor( lhs, (U)rhs );
    return lhs;
}

template < typename T, typename U, typename E >
_CONSTEXPR14 T& operator |=( T& lhs, SafeInt< U, E > rhs ) SAFEINT_NOTHROW
{
    lhs = BinaryOrHelper< T, U, BinaryMethod< T, U >::method >::Or( lhs, (U)rhs );
    return lhs;
}

template < typename T, typename U, typename E >
_CONSTEXPR14 T& operator <<=( T& lhs, SafeInt< U, E > rhs ) SAFEINT_NOTHROW
{
    lhs = (T)( SafeInt< T, E >( lhs ) << (U)rhs );
    return lhs;
}

template < typename T, typename U, typename E >
_CONSTEXPR14 T& operator >>=( T& lhs, SafeInt< U, E > rhs ) SAFEINT_NOTHROW
{
    lhs = (T)( SafeInt< T, E >( lhs ) >> (U)rhs );
    return lhs;
}

// Specific pointer overrides
// Note - this function makes no attempt to ensure
// that the resulting pointer is still in the buffer, only
// that no int overflows happened on the way to getting the new pointer
template < typename T, typename U, typename E >
T*& operator +=( T*& lhs, SafeInt< U, E > rhs ) SAFEINT_CPP_THROW
{
    // Cast the pointer to a number so we can do arithmetic
    // Note: this doesn't really make sense as a constexpr, but cannot be because of the reinterpret_cast
    SafeInt< size_t, E > ptr_val = reinterpret_cast< size_t >( lhs );
    // Check first that rhs is valid for the type of ptrdiff_t
    // and that multiplying by sizeof( T ) doesn't overflow a ptrdiff_t
    // Next, we need to add 2 SafeInts of different types, so unbox the ptr_diff
    // Finally, cast the number back to a pointer of the correct type
    lhs = reinterpret_cast< T* >( (size_t)( ptr_val + (ptrdiff_t)( SafeInt< ptrdiff_t, E >( rhs ) * sizeof( T ) ) ) );
    return lhs;
}

template < typename T, typename U, typename E >
T*& operator -=( T*& lhs, SafeInt< U, E > rhs ) SAFEINT_CPP_THROW
{
    // Cast the pointer to a number so we can do arithmetic
    SafeInt< size_t, E > ptr_val = reinterpret_cast< size_t >( lhs );
    // See above for comments
    lhs = reinterpret_cast< T* >( (size_t)( ptr_val - (ptrdiff_t)( SafeInt< ptrdiff_t, E >( rhs ) * sizeof( T ) ) ) );
    return lhs;
}

template < typename T, typename U, typename E >
T*& operator *=( T*& lhs, SafeInt< U, E > ) SAFEINT_NOTHROW
{
    // This operator explicitly not supported
    static_assert( sizeof(T) == 0, "Unsupported operator" );
    return (lhs = nullptr);
}

template < typename T, typename U, typename E >
T*& operator /=( T*& lhs, SafeInt< U, E > ) SAFEINT_NOTHROW
{
    // This operator explicitly not supported
    static_assert(sizeof(T) == 0, "Unsupported operator");
    return (lhs = nullptr);
}

template < typename T, typename U, typename E >
T*& operator %=( T*& lhs, SafeInt< U, E > ) SAFEINT_NOTHROW
{
    // This operator explicitly not supported
    static_assert(sizeof(T) == 0, "Unsupported operator");
    return (lhs = nullptr);
}

template < typename T, typename U, typename E >
T*& operator &=( T*& lhs, SafeInt< U, E > ) SAFEINT_NOTHROW
{
    // This operator explicitly not supported
    static_assert(sizeof(T) == 0, "Unsupported operator");
    return (lhs = nullptr);
}

template < typename T, typename U, typename E >
T*& operator ^=( T*& lhs, SafeInt< U, E > ) SAFEINT_NOTHROW
{
    // This operator explicitly not supported
    static_assert(sizeof(T) == 0, "Unsupported operator");
    return (lhs = nullptr);
}

template < typename T, typename U, typename E >
T*& operator |=( T*& lhs, SafeInt< U, E > ) SAFEINT_NOTHROW
{
    // This operator explicitly not supported
    static_assert(sizeof(T) == 0, "Unsupported operator");
    return (lhs = nullptr);
}

template < typename T, typename U, typename E >
_CONSTEXPR14 T*& operator <<=( T*& lhs, SafeInt< U, E > ) SAFEINT_NOTHROW
{
    // This operator explicitly not supported
    static_assert(sizeof(T) == 0, "Unsupported operator");
    return (lhs = nullptr);
}

template < typename T, typename U, typename E >
_CONSTEXPR14 T*& operator >>=( T*& lhs, SafeInt< U, E > ) SAFEINT_NOTHROW
{
    // This operator explicitly not supported
    static_assert(sizeof(T) == 0, "Unsupported operator");
    return (lhs = nullptr);
}

// Shift operators
// NOTE - shift operators always return the type of the lhs argument

// Left shift
template < typename T, typename U, typename E >
_CONSTEXPR14 SafeInt< U, E > operator <<( U lhs, SafeInt< T, E > bits ) SAFEINT_NOTHROW
{
    ShiftAssert( !std::numeric_limits< T >::is_signed || (T)bits >= 0 );
    ShiftAssert( (T)bits < (int)safeint_internal::int_traits< U >::bitCount );

    return SafeInt< U, E >( (U)( lhs << (T)bits ) );
}

// Right shift
template < typename T, typename U, typename E >
_CONSTEXPR14 SafeInt< U, E > operator >>( U lhs, SafeInt< T, E > bits ) SAFEINT_NOTHROW
{
    ShiftAssert( !std::numeric_limits< T >::is_signed || (T)bits >= 0 );
    ShiftAssert( (T)bits < (int)safeint_internal::int_traits< U >::bitCount );

    return SafeInt< U, E >( (U)( lhs >> (T)bits ) );
}

// Bitwise operators
// This only makes sense if we're dealing with the same type and size
// demand a type T, or something that fits into a type T.

// Bitwise &
template < typename T, typename U, typename E >
_CONSTEXPR11 SafeInt< T, E > operator &( U lhs, SafeInt< T, E > rhs ) SAFEINT_NOTHROW
{
    return SafeInt< T, E >( BinaryAndHelper< T, U, BinaryMethod< T, U >::method >::And( (T)rhs, lhs ) );
}

// Bitwise XOR
template < typename T, typename U, typename E >
_CONSTEXPR11 SafeInt< T, E > operator ^( U lhs, SafeInt< T, E > rhs ) SAFEINT_NOTHROW
{
    return SafeInt< T, E >(BinaryXorHelper< T, U, BinaryMethod< T, U >::method >::Xor( (T)rhs, lhs ) );
}

// Bitwise OR
template < typename T, typename U, typename E >
_CONSTEXPR11 SafeInt< T, E > operator |( U lhs, SafeInt< T, E > rhs ) SAFEINT_NOTHROW
{
    return SafeInt< T, E >( BinaryOrHelper< T, U, BinaryMethod< T, U >::method >::Or( (T)rhs, lhs ) );
}

#if defined VISUAL_STUDIO_SAFEINT_COMPAT
} // utilities
} // msl
#endif

#endif //SAFEINT_HPP