// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LoadLibraryAbs.h"

HMODULE GetPhoneModule()
{
    static HMODULE module = ::LoadLibraryExWAbs(L"Microsoft.UI.Xaml.phone.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    return module;
}


