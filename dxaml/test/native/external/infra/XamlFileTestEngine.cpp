// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>
#include "XamlFileTestEngine.h"

using namespace concurrency;
using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation {

        void XamlFileTestEngine::Execute()
        {
            WUCRenderingScopeGuard guard(m_dcompRenderingMode, false, m_mockDcompVerificationEnabled, false);

            if (m_mockDcompVerificationEnabled)
            {
                TestServices::Utilities->SetMockDCompSurfaceIdMode(m_mockDCompSurfaceIdMode);
            }

            TestServices::WindowHelper->SetWindowSizeOverrideWithScale(m_windowSize, m_zoomScale);
            TestServices::WindowHelper->WaitForIdle();

            FrameworkElement^ rootElement = safe_cast<FrameworkElement^>(LoadXamlFileOnUIThread(m_xamlFilePath));
            RunOnUIThread([&] ()
            {
                TestServices::WindowHelper->WindowContent = rootElement;
            });
            TestServices::WindowHelper->WaitForIdle();

            if (m_postInitCallback != nullptr)
            {
                RunOnUIThread([&] ()
                {
                    m_postInitCallback(rootElement);
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            if (m_postInitWaitCallback != nullptr)
            {
                m_postInitWaitCallback(rootElement);
            }

            if (m_validationCallback != nullptr)
            {
                RunOnUIThread([&] ()
                {
                    m_validationCallback(rootElement);
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            if (m_mockDcompVerificationEnabled)
            {
                if (m_mockDCompVariation == nullptr)
                {
                    TestServices::Utilities->VerifyMockDCompOutput(m_mockDCompVerification);
                }
                else
                {
                    TestServices::Utilities->VerifyMockDCompOutput(m_mockDCompVerification, m_mockDCompVariation);
                }
            }
        }
    }
} } } }