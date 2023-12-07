// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class ObjectWriterCommonRuntime;
class ObjectWriterContext;
class ObjectWriterErrorService;

namespace Parser
{
    _Check_return_ HRESULT CreateObjectWriterRuntime(
        _In_ const std::shared_ptr<ObjectWriterContext>& spContext,
        _In_ const std::shared_ptr<ObjectWriterErrorService>& spErrorService,
        _In_ bool isEncoding,
        _Out_ std::shared_ptr<ObjectWriterCommonRuntime>& spRuntime);
}

