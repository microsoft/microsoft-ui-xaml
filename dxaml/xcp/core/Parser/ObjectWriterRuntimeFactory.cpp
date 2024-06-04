// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ObjectWriterRuntimeFactory.h"
#include "ObjectWriterRuntime.h"
#include "ObjectWriterRuntimeEncoder.h"
#include "ObjectWriterContext.h"
#include "ObjectWriterErrorService.h"

namespace Parser
{
    _Check_return_ HRESULT CreateObjectWriterRuntime(
        _In_ const std::shared_ptr<ObjectWriterContext>& spContext,
        _In_ const std::shared_ptr<ObjectWriterErrorService>& spErrorService,
        _In_ bool isEncoding,
        _Out_ std::shared_ptr<ObjectWriterCommonRuntime>& spRuntime)
    {
        if (isEncoding)
        {
            auto encoder = std::make_shared<ObjectWriterRuntimeEncoder>(spContext, spErrorService);
            encoder->Initialize();
            spRuntime = std::static_pointer_cast<ObjectWriterCommonRuntime>(std::move(encoder));
        }
        else
        {
            spRuntime = std::make_shared<ObjectWriterRuntime>(spContext, spErrorService, true /* validate */);
        }
        return S_OK;
    }
}

