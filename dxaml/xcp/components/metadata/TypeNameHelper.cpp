// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TypeNameHelper.h"
#include <xstrutil.h>

//------------------------------------------------------------------------
//
//  Method:   TypeNameHelper::ParseAssemblyQualifiedTypeName
//
//  Synopsis: Parses a CLR assembly-qualified type name in the format:
//            "Namespace.TypeName, AssemblyShortName, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null"
//
//            *pwszTypeName returns the simple typename (including namespace):
//            "Namespace.TypeName"
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TypeNameHelper::ParseAssemblyQualifiedTypeName(
    _In_ const xstring_ptr& strAssemblyQualifiedTypeName,
    _Out_ xstring_ptr* pstrTypeName,
    _Out_ xstring_ptr* pstrAssemblyName)
{
    pstrTypeName->Reset();
    pstrAssemblyName->Reset();

    // find first comma
    auto iComma = strAssemblyQualifiedTypeName.FindChar(L',');
    if (iComma == xstring_ptr_view::npos)
    {
        // no comma, so assume the entire string is the simple typename
        *pstrTypeName = strAssemblyQualifiedTypeName;
        return S_OK;
    }

    // the comma should not be the first character
    IFCEXPECT_RETURN(0 != iComma);

    // substring from beginning up to first comma
    IFC_RETURN(strAssemblyQualifiedTypeName.SubString(0, iComma, pstrTypeName));

    // after the comma, there should be a space, followed by more characters
    // advance the string past the first command subsequent space
    {
        IFCEXPECT_RETURN(strAssemblyQualifiedTypeName.GetCount() > iComma + 2);

        const WCHAR* wszAssemblyQualifiedTypeName = strAssemblyQualifiedTypeName.GetBuffer() + iComma + 2;
        XUINT32 cchAssemblyQualifiedTypeName = strAssemblyQualifiedTypeName.GetCount() - iComma - 2;

        // find next comma
        iComma = xfindchar(wszAssemblyQualifiedTypeName, cchAssemblyQualifiedTypeName, L',');

        // the comma should not be the first character
        IFCEXPECT_RETURN(XUINT32(~0) != iComma && 0 != iComma);

        IFC_RETURN(xstring_ptr::CloneBuffer(wszAssemblyQualifiedTypeName, iComma, pstrAssemblyName));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TypeNameHelper::GetNamespace
//
//  Synopsis: Returns the namespace of the given typename. The namespace
//            is the substring of the typename from the beginning up to
//            (but not including) the last '.' character.
//
//            If the typename doesn't have any '.' characters, or if the
//            first character is '.', this function returns NULL
//            for pwszNamespace.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TypeNameHelper::GetNamespace(_In_z_ const WCHAR* wszTypeName, _Outptr_result_maybenull_z_ WCHAR** pwszNamespace)
{
    XUINT32 cchTypeName = xstrlen(wszTypeName);

    *pwszNamespace = NULL;

    XUINT32 iLastDot = xfindcharreverse(wszTypeName, cchTypeName, L'.');
    if (XUINT32(~0) == iLastDot ||
        0 == iLastDot)
    {
        return S_OK;
    }

    *pwszNamespace = xsubstr(wszTypeName, cchTypeName, 0, iLastDot);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TypeNameHelper::IsFrameworkNamespace
//
//  Synopsis: Checks whether the given namespace is a Jupiter framework
//            namespace.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TypeNameHelper::IsFrameworkNamespace(_In_z_ const WCHAR* wszNamespace, _Out_ bool* fFrameworkNamespace)
{
    *fFrameworkNamespace =
        XSTRING_PTR_EPHEMERAL2(wszNamespace, xstrlen(wszNamespace)).StartsWith(
            XSTRING_PTR_EPHEMERAL(L"Microsoft.UI.Xaml"),
            xstrCompareCaseSensitive);

    RRETURN(S_OK);
}

