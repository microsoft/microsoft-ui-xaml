// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ParserUnitTestIncludes.h"
#include "MockUri.h"
#include <XamlLogging.h>

using namespace DirectUI;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    _Check_return_ HRESULT MockUri::GetCanonical(_Out_ xstring_ptr* pstrCanonical) const
    {
        return GetCanonicalCallback(pstrCanonical);
    }

    _Check_return_ HRESULT MockUri::GetCanonical(_Inout_ UINT32* pBufferLength, _Out_writes_to_opt_(*pBufferLength, *pBufferLength) WCHAR* pszBuffer) const
    {
        return GetCanonicalCallback2(pBufferLength, pszBuffer);
    }

} } } } }

