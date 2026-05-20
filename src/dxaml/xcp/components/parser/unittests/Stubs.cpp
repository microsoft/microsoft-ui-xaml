// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlLogging.h>

#include <windows.foundation.h>
#include <Microsoft.UI.Xaml.h>

#include <Indexes.g.h>
#include <macros.h>
#include <minxcptypes.h>
#include <minerror.h>
#include <minpal.h>

#include <xref_ptr.h>
#include <xstringmap.h>

#include <corep.h>
#include <CDependencyObject.h>
#include <cvalue.h>
#include <TypeTableStructs.h>
#include <MetadataAPI.h>
#include <DataStructureFunctionProvider.h>
#include <XamlNativeRuntime.h>
#include <XamlManagedRuntime.h>
#include <NodeStreamCache.h>
#include <XamlBinaryFormatSubReader2.h>
#include <XamlQualifiedObject.h>
#include <XamlSchemaContext.h>
#include <ObjectWriterRuntimeEncoder.h>

using namespace DirectUI;
using namespace xaml_interop;

class ObjectWriterCallbacksDelegate;

#pragma region Temporary stubs.

_Check_return_
HRESULT CCoreServices::getErrorService(_Out_ IErrorService **ppErrorService)
{
    if (ppErrorService == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    *ppErrorService = nullptr;

    return S_OK;
}

namespace Parser
{
    _Check_return_ HRESULT CreateObjectWriterRuntime(
        _In_ const std::shared_ptr<ObjectWriterContext>& spContext,
        _In_ const std::shared_ptr<ObjectWriterErrorService>& spErrorService,
        _In_ bool isEncoding,
        _Out_ std::shared_ptr<ObjectWriterCommonRuntime>& spRuntime)
    {
        ASSERT(isEncoding);
        auto encoder = std::make_shared<ObjectWriterRuntimeEncoder>(spContext, spErrorService);
        IFC_RETURN(encoder->Initialize());
        spRuntime = std::static_pointer_cast<ObjectWriterCommonRuntime>(std::move(encoder));
        return S_OK;
    }
}

#pragma endregion

