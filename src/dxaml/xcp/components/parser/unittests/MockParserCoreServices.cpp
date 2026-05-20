// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ParserUnitTestIncludes.h"
#include "MockParserCoreServices.h"
#include "MockResourceManager.h"
#include <XamlLogging.h>

using namespace DirectUI;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    _Check_return_ HRESULT MockParserCoreServices::GetXamlNodeStreamCacheManager(
        _Out_ std::shared_ptr<XamlNodeStreamCacheManager>& )
    {
        return S_OK;
    }

    _Check_return_ HRESULT MockParserCoreServices::GetParserErrorService(
        _Out_ IErrorService**)
    {
        return S_OK;
    }

    _Check_return_ HRESULT MockParserCoreServices::GetResourceManager(IPALResourceManager** ppResourceManager)
    {
        return GetResourceManagerCallback(ppResourceManager);
    }

} } } } }

