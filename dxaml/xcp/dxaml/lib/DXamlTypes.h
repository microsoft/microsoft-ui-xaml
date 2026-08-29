// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines type aliases for core types.

#pragma once

DECLARE_HANDLE(XCONTEXT);

typedef std::wstring string;

// Even though HRESULTs and HRESULTs are very similar, we shouldn't implicitly
// convert them by using IFC on methods that return HRESULTs (like
// QueryInterface, etc.).
//
// IFH currently casts to an HRESULT and uses IFC to verify that it succeeded.
// By abstracting this out to a macro we at least have the option of easily
// changing how HRESULTs are interpreted in the future.
#define IFH(expr)   IFC(static_cast<HRESULT>(expr));

// Error codes
#define E_XAMLPARSEFAILED 10L
#define E_LAYOUTCYCLE 20L
#define E_ELEMENTNOTENABLED 30L
#define E_ELEMENTNOTAVAILABLE 31L

#define E_INVALID_OPERATION HRESULT_FROM_WIN32(ERROR_INVALID_OPERATION)

// Check if a bit flag is set on a value.  This is just a simple macro to make
// code using flags enums a little less verbose.
#define IsFlagSet(value, flag) (((flag) == ((value) & (flag))))


#define DELETE_ARRAY(arr) { CoTaskMemFree(arr); }

// Delete a string and propagate the failure code.
#define DELETE_STRING(hstr) { ::WindowsDeleteString(hstr);  }

#define VERIFYRETURNHR(cond) { HRESULT hr2 = (cond); VERIFYHR(hr2); if (SUCCEEDED(hr)) hr = hr2; }

// Argument checking macros.
#define ARG_EXPECT(expr, parameterName) { if (!(expr)) { IFC(DirectUI::ErrorHelper::OriginateError(E_INVALIDARG, SZ_COUNT(parameterName), L##parameterName)); } }
#define ARG_NOTNULL(parameter, parameterName) ARG_EXPECT(parameter, parameterName)

#define ARG_EXPECT_RETURN(expr, parameterName) { if (!(expr)) { IFC_RETURN(DirectUI::ErrorHelper::OriginateError(E_INVALIDARG, SZ_COUNT(parameterName), L##parameterName)); } }
#define ARG_NOTNULL_RETURN(parameter, parameterName) ARG_EXPECT_RETURN(parameter, parameterName)

#define ARG_VALIDRETURNPOINTER(pReturn) \
    {\
       if (!(pReturn))\
       {\
           RRETURN(DirectUI::ErrorHelper::OriginateError(E_INVALIDARG, SZ_COUNT("returnValue"), L"returnValue"));\
       }\
    }\

#define REFERENCE_ELEMENT_NAME_IMPL(TYPE, NAME) \
    template<> _Check_return_ HRESULT DirectUI::ReferenceBase<TYPE>::GetRuntimeClassNameImpl(_Out_ HSTRING* pClassName)\
    {\
        HRESULT hr = S_OK;\
        IFC(wrl_wrappers::HStringReference(L"Windows.Foundation.IReference`1<" NAME L">", SZ_COUNT(NAME) + 33).CopyTo(pClassName));\
    Cleanup:\
        RRETURN(hr);\
    }\

namespace DirectUI
{
    // Defines the different modes for opening an automatic ToolTip.
    enum AutomaticToolTipInputMode
    {
        None = 0,
        Touch = 1,
        Mouse = 2,
        Keyboard = 3
    };
}
