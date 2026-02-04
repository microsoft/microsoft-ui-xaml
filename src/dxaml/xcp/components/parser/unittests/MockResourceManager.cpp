// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ParserUnitTestIncludes.h"
#include "MockResourceManager.h"
#include <XamlLogging.h>

using namespace DirectUI;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    _Check_return_ HRESULT MockResourceManager::TryGetLocalResource(_In_ IPALUri* pResourceUri, _Outptr_result_maybenull_ IPALResource** ppResource)
    {
        return TryGetLocalResourceCallback(pResourceUri, ppResource);
    }

    IPALUri* MockResource::GetPhysicalResourceUriNoRef()
    {
        return GetPhysicalResourceUriNoRefCallback();
    }

} } } } }

