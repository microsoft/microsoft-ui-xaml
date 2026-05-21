// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define USE_SHARED_PTR_POOLED_ALLOCATOR 1

#include <minxcptypes.h>
#include <minerror.h>
#include <palcore.h>
#include <IParserCoreServices.h>

struct IErrorService;
class XamlNodeStreamCacheManager;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    class MockParserCoreServices : public IParserCoreServices
    {
    public:
        MockParserCoreServices()
        {}

        _Check_return_ HRESULT GetXamlNodeStreamCacheManager(
            _Out_ std::shared_ptr<XamlNodeStreamCacheManager>& spXamlNodeStreamCacheManager) override;

        _Check_return_ HRESULT GetParserErrorService(
            _Out_ IErrorService **ppErrorService) override;

        _Check_return_ HRESULT GetResourceManager(IPALResourceManager** ppResourceManager) override;

        std::function<HRESULT(IPALResourceManager**)> GetResourceManagerCallback;
    };

} } } } }

