// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WUCRenderingScopeGuard.h>

// Provides a generic mechanism for loading and testing a XAML file.
// A lot of classes define their own that look almost identical to this.  Use this one
// instead and build upon it.  Tasks and validation specific to the test can be done
// via the callbacks.  Try to keep this class as generic as possible so that any test
// can use it.
// NOTE: While using this object, it is still safest to use TestCleanupWrapper in order
//       to cleanup objects weak referenced with SafeEventRegistration in the client test.
namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation {
        class XamlFileTestEngine
        {
        public:

            using Callback = std::function<void(Microsoft::UI::Xaml::FrameworkElement^ rootElement)>;

            void SetXamlFilePath(Platform::String^ value) { m_xamlFilePath = value; }

            void SetWindowSize(::Windows::Foundation::Size value) { m_windowSize = value; }
            void SetZoomScale(float value) { m_zoomScale = value; }

            void SetPostInitCallback(Callback value) { m_postInitCallback = value; }
            void SetPostInitWaitCallback(Callback value) { m_postInitWaitCallback = value; }
            void SetValidationCallback(Callback value) { m_validationCallback = value; }

            void SetMockDCompVerificationEnabled(bool enabled) { m_mockDcompVerificationEnabled = enabled; }
            void SetDCompRenderingMode(Microsoft::UI::Xaml::Tests::Common::DCompRendering value) { m_dcompRenderingMode = value; }
            void SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode value) { m_mockDCompSurfaceIdMode = value; }
            void SetMockDCompVerification(MockDComp::SurfaceComparison value) { m_mockDCompVerification = value; }
            void SetMockDCompVariation(Platform::String^ value) { m_mockDCompVariation = value; }

            void Execute();

        private:
            Platform::String^ m_xamlFilePath = nullptr;

            ::Windows::Foundation::Size m_windowSize = { 400, 300 };
            float m_zoomScale = 1.0f;

            Callback m_postInitCallback;
            Callback m_postInitWaitCallback;
            Callback m_validationCallback;

            bool m_mockDcompVerificationEnabled = true;
            Microsoft::UI::Xaml::Tests::Common::DCompRendering m_dcompRenderingMode = Microsoft::UI::Xaml::Tests::Common::DCompRendering::WUCCompleteSynchronousCompTree;
            MockDComp::SurfaceComparison m_mockDCompVerification = MockDComp::SurfaceComparison::ReferencedOnly;
            MockDComp::SurfaceIdMode m_mockDCompSurfaceIdMode = MockDComp::SurfaceIdMode::XmlOrder;
            Platform::String^ m_mockDCompVariation = nullptr;
        };
    }
} } } }