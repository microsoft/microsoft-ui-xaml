// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlNodeStreamCacheManagerUnitTests.h"
#include "ParserUtilities.h"
#include "ParserUnitTestIncludes.h"
#include "MockResourceManager.h"
#include "MockUri.h"
#include "MockParserCoreServices.h"
#include <XamlLogging.h>
#include <CStaticLock.h>

using namespace DirectUI;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    #pragma region Test Class Initialization & Cleanup
    bool XamlNodeStreamCacheManagerUnitTests::ClassSetup()
    {
        THROW_IF_FAILED(StaticLockGlobalInit());
        return true;
    }

    bool XamlNodeStreamCacheManagerUnitTests::ClassCleanup()
    {
        StaticLockGlobalDeinit();
        return true;
    }
    #pragma endregion

    void XamlNodeStreamCacheManagerUnitTests::Create()
    {
        ParserUtilities parserUtils;

        auto spSchemaContext = parserUtils.GetSchemaContext(nullptr);

        std::shared_ptr<IParserCoreServices> spParserCoreServices;
        spParserCoreServices.reset(new MockParserCoreServices());

        std::shared_ptr<XamlNodeStreamCacheManager> spXamlNodeStreamCacheManager;
        VERIFY_SUCCEEDED(XamlNodeStreamCacheManager::Create(spParserCoreServices.get(), spXamlNodeStreamCacheManager));
        VERIFY_IS_NOT_NULL(spXamlNodeStreamCacheManager);
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_XbfResourceStorage.size(), static_cast<size_t>(0));
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.size(), static_cast<size_t>(0));
    }

    void XamlNodeStreamCacheManagerUnitTests::GetBinaryResourceForMsAppXUri()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(msappxUri, L"ms-appx://Files/uri.xaml");
        DECLARE_CONST_STRING_IN_TEST_CODE(msappxUriPhysical, L"ms-appx://Files/uriphysical.xbf");
        GetBinaryResourceForSupportedUri(msappxUri, msappxUriPhysical);
    }

    void XamlNodeStreamCacheManagerUnitTests::GetBinaryResourceForMsResourceUri()
    {
        DECLARE_CONST_STRING_IN_TEST_CODE(msresourceUri, L"ms-resource://Files/uri.xaml");
        DECLARE_CONST_STRING_IN_TEST_CODE(msresourceUriPhysical, L"ms-resource://Files/uriphysical.xbf");
        GetBinaryResourceForSupportedUri(msresourceUri, msresourceUriPhysical);
    }

    void XamlNodeStreamCacheManagerUnitTests::GetBinaryResourceForSupportedUri(const xstring_ptr& uri, const xstring_ptr& physicalUri)
    {
        ParserUtilities parserUtils;

        auto spSchemaContext = parserUtils.GetSchemaContext(nullptr);

        std::shared_ptr<MockParserCoreServices> spParserCoreServices;
        spParserCoreServices.reset(new MockParserCoreServices());

        std::shared_ptr<XamlNodeStreamCacheManager> spXamlNodeStreamCacheManager;
        VERIFY_SUCCEEDED(XamlNodeStreamCacheManager::Create(spParserCoreServices.get(), spXamlNodeStreamCacheManager));

        xref_ptr<MockResourceManager> spMockResourceManager = make_xref<MockResourceManager>();
        spParserCoreServices->GetResourceManagerCallback =
            [&spMockResourceManager](IPALResourceManager** pptr) -> HRESULT
        {
            *pptr = spMockResourceManager;
            if (spMockResourceManager)
                spMockResourceManager->AddRef();
            return S_OK;
        };

        xref_ptr<MockResource> spMockResource = make_xref<MockResource>();
        spMockResourceManager->TryGetLocalResourceCallback =
            [&spMockResource](IPALUri*, IPALResource** pptr) -> HRESULT
        {
            *pptr = spMockResource;
            if (spMockResource)
                spMockResource->AddRef();
            return S_OK;
        };

        xref_ptr<MockUri> spMockUri = make_xref<MockUri>();
        spMockResource->GetPhysicalResourceUriNoRefCallback =
            [&spMockUri]() -> IPALUri*
        {
            return spMockUri.get();
        };

        spMockUri->GetCanonicalCallback =
            [&physicalUri](xstring_ptr* pstr) -> HRESULT
        {
            *pstr = physicalUri;
            return S_OK;
        };

        // Get resource for URI.
        // Verify we get the correct resource, and it's in the cache.
        xref_ptr<IPALResource> spXbfResource;
        VERIFY_SUCCEEDED(spXamlNodeStreamCacheManager->GetBinaryResourceForXamlUri(uri, spXbfResource));
        VERIFY_IS_NOT_NULL(spXbfResource);

        // Resource ref count:  1 ref created locally, 1 ref returned by call, 1 ref in map, 1 ref in storage
        VERIFY_ARE_EQUAL(spXbfResource.get(), spMockResource.get());
        VERIFY_ARE_EQUAL(spMockResource->m_cRef, 4);
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_XbfResourceStorage.size(), static_cast<size_t>(1));
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.size(), static_cast<size_t>(1));
        VERIFY_IS_TRUE(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.find(physicalUri) != spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.end());
        VERIFY_IS_TRUE(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.find(uri) == spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.end());

        auto itMappedResource1 = spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.find(physicalUri);
        VERIFY_IS_TRUE(itMappedResource1 != spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.end());
        VERIFY_ARE_EQUAL(itMappedResource1->second.get(), spMockResource.get());

        // Release temp local refs, remaining should be 1 ref created locally, 1 ref in map, 1 ref in storage
        spXbfResource.reset();
        VERIFY_ARE_EQUAL(spMockResource->m_cRef, 3);

        // Get resource for the same URI again.
        // Verify that we get the same resource as the first time, and the cache is unchanged.
        xref_ptr<MockResource> spMockResource2 = make_xref<MockResource>();
        spMockResourceManager->TryGetLocalResourceCallback =
            [&spMockResource2](IPALUri*, IPALResource** pptr) -> HRESULT
            {
                *pptr = spMockResource2;
                if (spMockResource2)
                    spMockResource2->AddRef();
                return S_OK;
            };

        spMockResource2->GetPhysicalResourceUriNoRefCallback =
            [&spMockUri]() -> IPALUri*
            {
                return spMockUri.get();
            };

        VERIFY_SUCCEEDED(spXamlNodeStreamCacheManager->GetBinaryResourceForXamlUri(uri, spXbfResource));
        VERIFY_IS_NOT_NULL(spXbfResource);

        // Resource ref count: 1 ref created locally, 1 ref returned by call, 1 ref in map, 1 ref in storage
        // MockResource2 ref count: 1 ref created locally
        VERIFY_ARE_EQUAL(spXbfResource.get(), spMockResource.get());
        VERIFY_ARE_EQUAL(spMockResource->m_cRef, 4);
        VERIFY_ARE_EQUAL(spMockResource2->m_cRef, 1);
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_XbfResourceStorage.size(), static_cast<size_t>(1));
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.size(), static_cast<size_t>(1));
        VERIFY_IS_TRUE(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.find(physicalUri) != spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.end());
        VERIFY_IS_TRUE(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.find(uri) == spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.end());
        auto itMappedResource2 = spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.find(physicalUri);
        VERIFY_IS_TRUE(itMappedResource2 != spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.end());
        VERIFY_ARE_EQUAL(itMappedResource2->second.get(), spMockResource.get());

        // Release temp local refs, remaining should be 1 ref created locally, 1 ref in map, 1 ref in storage
        spXbfResource.reset();
        VERIFY_ARE_EQUAL(spMockResource->m_cRef, 3);

        // Clear the cache.
        // Resource ref count:  1 ref created locally, 1 ref in manager's long term storage
        spXamlNodeStreamCacheManager->Flush();
        VERIFY_ARE_EQUAL(spMockResource->m_cRef, 2);
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.size(), static_cast<size_t>(0));
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_XbfResourceStorage.size(), static_cast<size_t>(1));
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_XbfResourceStorage[0].get(), spMockResource.get());

        VERIFY_ARE_EQUAL(spMockResourceManager->m_cRef, 1);
        VERIFY_ARE_EQUAL(spMockUri->m_cRef, 1);
    }

    void XamlNodeStreamCacheManagerUnitTests::GetBinaryResourceForUnsupportedUri()
    {
        ParserUtilities parserUtils;
        DECLARE_CONST_STRING_IN_TEST_CODE(unsupportedUri, L"x:\\unsupporteduri.xaml");

        auto spSchemaContext = parserUtils.GetSchemaContext(nullptr);

        std::shared_ptr<MockParserCoreServices> spParserCoreServices;
        spParserCoreServices.reset(new MockParserCoreServices());

        std::shared_ptr<XamlNodeStreamCacheManager> spXamlNodeStreamCacheManager;
        VERIFY_SUCCEEDED(XamlNodeStreamCacheManager::Create(spParserCoreServices.get(), spXamlNodeStreamCacheManager));

        // Get resource for unsupported URI.
        // Resource should be nullptr.
        xref_ptr<IPALResource> spXbfResource;
        VERIFY_SUCCEEDED(spXamlNodeStreamCacheManager->GetBinaryResourceForXamlUri(unsupportedUri, spXbfResource));
        VERIFY_IS_NULL(spXbfResource);
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_XbfResourceStorage.size(), static_cast<size_t>(0));
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.size(), static_cast<size_t>(1));
        VERIFY_IS_TRUE(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.find(unsupportedUri) != spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.end());
        xref_ptr<IPALResource> spMappedResource;
        auto itMappedResource1 = spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.find(unsupportedUri);
        VERIFY_IS_TRUE(itMappedResource1 != spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.end());
        VERIFY_IS_NULL(itMappedResource1->second);

        // Get resource again for unsupported URI.
        // Resource should be nullptr.
        VERIFY_SUCCEEDED(spXamlNodeStreamCacheManager->GetBinaryResourceForXamlUri(unsupportedUri, spXbfResource));
        VERIFY_IS_NULL(spXbfResource);
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_XbfResourceStorage.size(), static_cast<size_t>(0));
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.size(), static_cast<size_t>(1));
        VERIFY_IS_TRUE(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.find(unsupportedUri) != spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.end());
        auto itMappedResource2 = spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.find(unsupportedUri);
        VERIFY_IS_TRUE(itMappedResource2 != spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.end());
        VERIFY_IS_NULL(itMappedResource2->second);

        // Clear the cache.
        spXamlNodeStreamCacheManager->Flush();
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_XbfResourceStorage.size(), static_cast<size_t>(0));
        VERIFY_ARE_EQUAL(spXamlNodeStreamCacheManager->m_UriToXbfResourceMap.size(), static_cast<size_t>(0));
    }

} } } } }

