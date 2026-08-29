// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RichTextServices
{
    //---------------------------------------------------------------------------
    //
    //  Result
    //
    //  A value used to describe the outcome of a function call.
    //
    //  Result is used as a return value whenever a RichTextServices API might
    //  fail.  For example,
    //
    //      Result::Enum GetOffset(_Out_ int *pOffset);
    //
    //  GetOffset in this example comes from the TextPosition class, and the
    //  method typically returns Result::Success.  However, when called in
    //  certain contexts it may return Result::InvalidOperation, indicating
    //  caller error and failure.
    //
    //  Out parameters are undefined on exit when a RichTextServices method
    //  returns any Result other than Result::Success.
    //
    //  RichTextServices provides several macros that aid error handling. All of
    //  them rely on the presence of local Result::Enum variable named "hr" and
    //  a label named "Cleanup" which is jumped to on error.
    //
    //  The idiom looks like
    //
    //  extern Result::Enum RtsAPI();
    //
    //  Result::Enum Function()
    //  {
    //      Result::Enum txhr = Result::Success;
    //      XINT32 *p = NULL;
    //
    //      // "If Fail Cleanup" -- jump to Cleanup on any return value other than
    //      // Result::Success.  hr is assigned the error code.
    //      IFC(RtsAPI());
    //
    //      // "If Fail Cleanup Out Of Memory" -- assign Result::OutOfMemory to hr 
    //      // if a pointer is NULL and jump to Cleanup.
    //      IFC_OOM_RTS(p = new XINT32[10]);
    //
    //      // "If Fail Cleanup Expect" -- assign Result::Unexpected to hr if an
    //      // invariant evaluates to false and jump to Cleanup.
    //      IFC_EXPECT_RTS(p != NULL);
    //
    //      // Like IFC_EXPECT_RTS, except in debug the ASSERT macro is invoked first.
    //      // The expression in only evaluated once in debug or retail.
    //      ASSERT_EXPECT_RTS(p != NULL);
    //
    //  Cleanup:
    //      // IFC macros jump here on failure.  Perform any cleanup before returning.
    //      delete[] p;
    //      return hr;
    //  }
    //      
    //---------------------------------------------------------------------------
    namespace Result
    {
        enum EnumImpl
        {
            // The call succeeded.
            Success =            0,

            // Failure, due to lack of memory.
            OutOfMemory =       -1,

            // Failure, unspecified reason.
            Unexpected =        -2,

            // Failure, due to a bad input parameter.
            InvalidParameter =  -3,

            // Failure, while formatting.
            FormattingError =   -4,

            // Failure, method not implemented.
            NotImplemented =    -5,

            // Failure, call is invalid given object's current state.
            InvalidOperation =  -6,

            // Error that XAML should keep internal and not pass back to application.
            InternalError =     -7,
        };

        // This typedef is an unfortunate bit of cleverness required to embed SAL annotation in
        // code like
        //
        //  Result::Enum Foo() { .. }
        //
        // which expands out to
        //
        //  _Check_return_ _Success_(return == Result::Success) Result::EnumImpl Foo() { .. }
        //
        typedef _Check_return_ _Success_(return == 0) EnumImpl Enum;
    }

    /** Verify that `val` is a Result::Enum value.
    Other types will generate an intentional compilation error.
    This is based on the verify_ methods in wil/include/common.h
    @param val The error code returning expression
    @return A Result::Enum representing the evaluation of `val`. */
    template <typename T>
    _Post_satisfies_(return == val)
    __forceinline constexpr int verify_TxError(T val)
    {
        static_assert((wistd::is_same<T, Result::Enum>::value), "Wrong Type: Result::Enum expected");
        return val;
    }

#if DBG && XCP_MONITOR

    #define IFCTEXT(x)                                                                                      \
    {                                                                                                       \
        txhr = (x);                                                                                         \
        verify_TxError(txhr);                                                                               \
        if (txhr != Result::Success)                                                                        \
        {                                                                                                   \
            goto Cleanup;                                                                                   \
        }                                                                                                   \
    }

#else // #if DBG && XCP_MONITOR

    #define IFCTEXT(x)                                      \
    {                                                       \
        txhr = (x);                                         \
        verify_TxError(txhr);                               \
        if (txhr != Result::Success)                        \
        {                                                   \
            goto Cleanup;                                   \
        }                                                   \
    }

#endif // #else #if DBG && XCP_MONITOR

    //---------------------------------------------------------------------------
    //
    //  Macro:
    //      IFC_OOM_RTS(x)
    //
    //  Synopsis:
    //      Exits the current method with Result::OutOfMemory when an input
    //      pointer has a NULL value.
    //
    //  Notes:
    //
    //      Result::Enum Sample()
    //      {
    //          Result::Enum txhr = Result::Success;
    //          Foo *pFoo = new Foo();
    //          IFC_OOM_RTS(pFoo);
    //      Cleanup:
    //          return hr;
    //      }
    //
    //---------------------------------------------------------------------------
    #define IFC_OOM_RTS(x) if ((x) == NULL) {txhr = Result::OutOfMemory; goto Cleanup;}

    //---------------------------------------------------------------------------
    //
    //  Macro:
    //      IFC_EXPECT_RTS(x)
    //
    //  Synopsis:
    //      Exits the current method with Result::Unexpected when an input
    //      expression evaluates false.
    //
    //  Notes:
    //
    //      Result::Enum Sample()
    //      {
    //          Result::Enum txhr = Result::Success;
    //          IFC_EXPECT_RTS(m_invariant);
    //      Cleanup:
    //          return hr;
    //      }
    //
    //---------------------------------------------------------------------------
    #define IFC_EXPECT_RTS(x)                                                                       \
    {                                                                                               \
        if (!(x))                                                                                   \
        {                                                                                           \
            txhr = Result::Unexpected;                                                              \
            goto Cleanup;                                                                           \
        }                                                                                           \
    }

    //---------------------------------------------------------------------------
    //
    //  Macro:
    //      ASSERT_EXPECT_RTS(x)
    //
    //  Synopsis:
    //      Exits the current method with Result::Unexpected when an input
    //      expression evaluates false.
    //
    //  Notes:
    //      In debug builds, additionally asserts the condition.
    //
    //      Result::Enum Sample()
    //      {
    //          Result::Enum txhr = Result::Success;
    //          ASSERT_EXPECT_RTS(m_invariant);
    //      Cleanup:
    //          return hr;
    //      }
    //
    //---------------------------------------------------------------------------
    #define ASSERT_EXPECT_RTS(x) { bool invariant = x; ASSERT(invariant); IFC_EXPECT_RTS(invariant); }
}
