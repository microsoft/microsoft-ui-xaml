// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "LockScreenTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Shell {

        Platform::String^ LockScreenTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\general\\";
        }

        bool LockScreenTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool LockScreenTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool LockScreenTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case: Checks for a transparent background when the app is on lock
        //            screen
        //------------------------------------------------------------------------
        void LockScreenTests::TransparentBackground()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            // Note: Real shell applications should hide the window while updating TransparentBackground before showing it so no
            // XAML frame gets rendered

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

            Grid^ rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"EmptyGrid.xaml"));
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                xaml::Window^ xamlWindow = xaml::Window::Current;
                xaml::IWindowPrivate^ windowPrivate = dynamic_cast<::IWindowPrivate^>(xamlWindow);
                
                windowPrivate->TransparentBackground = false;
                VERIFY_IS_FALSE(windowPrivate->TransparentBackground);
                windowPrivate->TransparentBackground = true;
                VERIFY_IS_TRUE(windowPrivate->TransparentBackground);
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "transparent");

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                xaml::Window^ xamlWindow = xaml::Window::Current;
                xaml::IWindowPrivate^ windowPrivate = dynamic_cast<::IWindowPrivate^>(xamlWindow);

                VERIFY_IS_TRUE(windowPrivate->TransparentBackground);
                windowPrivate->TransparentBackground = false;
                VERIFY_IS_FALSE(windowPrivate->TransparentBackground);
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, "opaque");
        }

    } }
} } } }
