// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Metadata {

        class MetadataUnitTests : public WEX::TestClass < MetadataUnitTests >
        {
        public:
            BEGIN_TEST_CLASS(MetadataUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)

            TEST_METHOD(StorageAccessIsThreadSafe)
            TEST_METHOD(CanResolveCustomTypeByTypeName)
            TEST_METHOD(CanResolveDirectiveOnCustomType)
            TEST_METHOD(IsAssignableFrom)
            TEST_METHOD(GetClassInfoByName)
            TEST_METHOD(GetClassInfoByFullName)
            TEST_METHOD(GetTypeNameByClassInfo)
            TEST_METHOD(IsConstructible)
            TEST_METHOD(BaseTypes)
            TEST_METHOD(ValidateISupportInitializeFlagOnTypes)
            TEST_METHOD(DependencyObjectIsInGoodState)
            TEST_METHOD(GetPrimitiveClassInfo)
            TEST_METHOD(CanExtractNamespaceNameAndShortName)
            
            BEGIN_TEST_METHOD(IXamlMemberTypeMayReturnNull)
                TEST_METHOD_PROPERTY(L"Description", L"Validates MetadataAPI::ImportPropertyInfo can deal with IXamlMember.Type returning nullptr.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FoundationTypesInCorrectNamespace)
                TEST_METHOD_PROPERTY(L"Description", L"Validates wf::{Size, Rect, Point} are in the correct namespace.")
            END_TEST_METHOD()

            TEST_METHOD(RequiresPeerActivation)
            TEST_METHOD(GetStorageType)
            TEST_METHOD(GetOffset)
            TEST_METHOD(GetGroupOffset)

            BEGIN_TEST_METHOD(ReRegisteringBuiltinDPDoesNotAV)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReRegisteringCustomDPSetsUnderlyingDP)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RunClassConstructorIsDelayed)
                TEST_METHOD_PROPERTY(L"Description", L"Validates CClassInfo::RunClassConstructorIfNecessary is not called when we import ClassInfo. But called when we import custom property info.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RunClassConstructorIsNotDelayed)
                TEST_METHOD_PROPERTY(L"Description", L"For older builds, Validates CClassInfo::RunClassConstructorIfNecessary is called when we import ClassInfo.")
            END_TEST_METHOD()

            TEST_METHOD(TestPropertyGetters)
        };
    }
} } } }
