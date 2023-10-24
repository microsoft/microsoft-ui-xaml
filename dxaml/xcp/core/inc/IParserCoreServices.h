// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once 

class XamlNodeStreamCacheManager;
struct IErrorService;

class IParserCoreServices
{
public:
    virtual _Check_return_ HRESULT GetXamlNodeStreamCacheManager(
        _Out_ std::shared_ptr<XamlNodeStreamCacheManager>& spXamlNodeStreamCacheManager) = 0;

    virtual _Check_return_ HRESULT GetParserErrorService(
        _Out_ IErrorService **ppErrorService) = 0;

    virtual _Check_return_ HRESULT GetResourceManager(IPALResourceManager** ppResourceManager) = 0;
};

