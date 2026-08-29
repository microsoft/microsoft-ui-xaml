// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace MemoryDiagnostics {
        class MemoryInfoTests : public WEX::TestClass<MemoryInfoTests>
        {
        public:
            BEGIN_TEST_CLASS(MemoryInfoTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\Microsoft.UI.Xaml.hosting.referencetracker.idl")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(TestGetCountOfDescendantUIElements)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that private IMemoryInfoPrivate::GetCountOfDescendantUIElements returns the expected values.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetCountOfDescendantUIElementsWithVirtualization)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that private IMemoryInfoPrivate::GetCountOfDescendantUIElements returns the expected values when virtualization is in play.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetEstimatedSizeOfDescendantImages)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that private IMemoryInfoPrivate::GetEstimatedSizeOfDescendantImages returns the expected values.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetEstimatedSizeOfDescendantImagesHDR)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that private IMemoryInfoPrivate::GetEstimatedSizeOfDescendantImages returns the expected values when HDR is enabled.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestGetEstimatedSizeOfDescendantImagesSoftwareRasterization)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that private IMemoryInfoPrivate::GetEstimatedSizeOfDescendantImages returns the expected values when software rasterization is used.")
            END_TEST_METHOD()

        private:
            void TestGetEstimatedSizeOfDescendantImages_Helper(bool useHdr);
        };
    }
} } } } }
