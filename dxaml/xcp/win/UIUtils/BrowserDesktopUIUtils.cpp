// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "UIUtils.h"

//------------------------------------------------------------------------
//
//  Method:   FindResourceExWithFallback
//
//  Synopsis:
//      Encapsulates the FindResourceEx, if the resource cannot be found on the
//      specify language, we fallback to english.
//
//------------------------------------------------------------------------
HRSRC FindResourceExWithFallback(
    HINSTANCE hModule,
    LPCTSTR lpType,
    LPCTSTR  lpName,
    LANGID wLanguage)
{
    HRSRC hIncreaseDialogReference = NULL;

    hIncreaseDialogReference = FindResourceEx(hModule, lpType, lpName, wLanguage);

    if (hIncreaseDialogReference == NULL) {
        hIncreaseDialogReference = FindResourceEx(hModule, lpType, lpName, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
    }

    return hIncreaseDialogReference;
}


