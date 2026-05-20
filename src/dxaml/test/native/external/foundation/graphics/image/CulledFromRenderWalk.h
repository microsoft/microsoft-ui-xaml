// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Image {

        class CulledFromRenderWalk : public WEX::TestClass<CulledFromRenderWalk>
        {
        public:
            BEGIN_TEST_CLASS(CulledFromRenderWalk)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"9882729e-eea8-4b89-99e7-92145be50e76;bd1463b3-e5f2-4d54-9394-63a431c53a6e;d04573b8-e899-4822-bb72-9f4743c89d36")

#if defined(_WIN64) // Disable the test on 64-bit build because of floating point differences between x87 and sse instructions
                TEST_CLASS_PROPERTY(L"Ignore", L"TRUE")
#endif
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(Basics)
                TEST_METHOD_PROPERTY(L"Description", L"Creates an Image element that is culled from the RenderWalk and verifies it still uses DecodeToRenderSize feature")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Basics2)
                TEST_METHOD_PROPERTY(L"Description", L"Creates a Canvas element with background image that is culled from the RenderWalk and verifies it still uses DecodeToRenderSize feature")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Basics3WUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"Creates an Image element with BitmapCache that is culled from the RenderWalk, verifies it falls back to natural size due to software rendering which disables DecodeToRenderSize")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Basics4)
                TEST_METHOD_PROPERTY(L"Description", L"Creates an Ellipse element that is culled from the RenderWalk, verifies it falls back to natural size due to software rendering which disables DecodeToRenderSize")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Advanced1)
                TEST_METHOD_PROPERTY(L"Description", L"Creates two Image elements that share the same BitmapImage, both are culled from the RenderWalk, and verifies both still use DecodeToRenderSize feature")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Advanced2)
                TEST_METHOD_PROPERTY(L"Description", L"Creates an Image element inside an InlineUIContainer, both are culled from the RenderWalk, and verifies both still use DecodeToRenderSize feature")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EdgeCase1)
                TEST_METHOD_PROPERTY(L"Description", L"Creates two Image elements that share the same BitmapImage, one is culled from the RenderWalk, the other is not in the live tree, and verifies we don't crash")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

        private:
            Platform::String^ GetResourcesPath() const;
            void Basics3(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

        private:
            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride m_backgroundThreadImageLoadingFeature;

        };

    } } }
} } } }

