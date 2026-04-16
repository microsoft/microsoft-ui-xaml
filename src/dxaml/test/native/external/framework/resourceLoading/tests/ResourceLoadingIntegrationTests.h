// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <XamlMetadataProviderOverrider.h>
#include <memory>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace ResourceLoading {

    class ResourceLoadingIntegrationTests
    {
    public:
        ResourceLoadingIntegrationTests() {}

        BEGIN_TEST_CLASS(ResourceLoadingIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"f62323d4-fd46-4c98-aa85-334db95ba8f6;0a9cdf5f-1e1b-4b1d-9659-b354bf5f4ca6")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)

        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanLoadComponentOutsideVisualTree)
            TEST_METHOD_PROPERTY(L"Description",
                L"Use LoadComponent to load a file before adding it to the visual tree")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanLoadComponentInsideVisualTree)
            TEST_METHOD_PROPERTY(L"Description",
                L"Use LoadComponent to load a file after adding it to the visual tree")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanLoadCustomResourcesFromXaml)
            TEST_METHOD_PROPERTY(L"Description",
                L"Load a Xaml snippet containing custom resources, and verify the values are hooked up correctly")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanLoadCustomResourcesFromComponent)
            TEST_METHOD_PROPERTY(L"Description",
                L"Load a component containing custom resources, and verify the values are hooked up correctly")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPreResolveOfResourcesInTemplate)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verifies that resources in a template are pre-resolved so that changes to the public dictionary"
                L"don't cause later expansions of the template to change.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCanReplaceGenericXaml)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verifies that resources are loaded from replacement generic.xaml")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP: Test ResourceLoadingIntegrationTests hitting targettype assert
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyMissingGenericXamlReplacement)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verifies that resources are loaded from default generic.xaml when replacement is missing")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyIncompleteGenericXamlReplacement)
            TEST_METHOD_PROPERTY(L"Description",
                L"Verifies that resources are missing when replacement is incomplete")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP: Test ResourceLoadingIntegrationTests hitting targettype assert
        END_TEST_METHOD()
    private:
    };
} } } } } }
