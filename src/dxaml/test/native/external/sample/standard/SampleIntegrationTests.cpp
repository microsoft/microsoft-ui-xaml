// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SampleIntegrationTests.h"
#include <ppltasks.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Sample {

        bool SampleIntegrationTests::ClassSetup()
        {
            // It's very important to call EnsureInitialized on TestServices
            // from ClassSetup. This method will wait for the window to be
            // activated on launch, which avoids a race condition that will block
            // input from being routed to the app. It will also wait for the
            // debugger to attach when the waitForDebugger runtime parameter is
            // specified.
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool SampleIntegrationTests::TestCleanup()
        {
            // It's very important to have your test clean up the window contents
            // when it completes. When creating new tests be sure to copy this
            // method over or implement it in a similar way. By cleaning
            // up the window content and waiting for the page to go idle you ensure
            // that if your test fails while the UI element tree is being torn down
            // that the failure is associated with your test and doesn't occur
            // nondeterministically in the future. By waiting for the page to go
            // idle you ensure that all transitions have completed and that jupiter
            // is in a 'tabula rasa' state for the next test.
            //
            // Use the TestCleanupWrapper in each test method to handle cleanup, even
            // in cases of failure or repeated runs. Use VerifyTestCleanup here to
            // ensure that the test was cleaned up correctly.
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void SampleIntegrationTests::CreateAGrid()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&] () {
                Grid^ grid = ref new Grid();
            });
        }

        void SampleIntegrationTests::LoadedEvent()
        {
            TestCleanupWrapper cleanup;

            // Any registered event handlers must be unregistered at the end of the test. Otherwise
            // there will be a race condition between XAML resetting the root for test cleanup and
            // TAEF unloading the test binaries. If this is the last test and TAEF unloads the dll
            // containing the event handler before XAML finishes cleaning up, then XAML will AV when
            // it tries to release the event handler. Use the SafeEventRegistration mechanism to
            // unregister the event when the test method exits successfully or fails.
            std::shared_ptr<Event> loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Canvas, Loaded);

            Canvas^ rootPanel = nullptr;

            RunOnUIThread([&] ()
            {
                rootPanel = ref new Canvas();

                loadedRegistration.Attach(rootPanel, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
                {
                    loadedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            loadedEvent->WaitForDefault();
        }

        void SampleIntegrationTests::LoadSingleXamlFile()
        {
            TestCleanupWrapper cleanup;

            auto rootPanel = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"Sample.xaml"));
            VERIFY_IS_NOT_NULL(rootPanel);
        }

        void SampleIntegrationTests::LoadMultipleXamlFilesAsync()
        {
            TestCleanupWrapper cleanup;

            // Queue up a bunch of files to load
            std::vector<Platform::String^> filenames(8, GetPathToFiles() + L"Sample.xaml");

            auto loadedObjects = LoadXamlFilesOnUIThread(filenames.begin(), filenames.end());

            for (auto loadedObject : loadedObjects)
            {
                VERIFY_IS_NOT_NULL(safe_cast<Canvas^>(loadedObject));
            }
        }

        void SampleIntegrationTests::SimpleDCompValidation()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            // Any registered event handlers must be unregistered at the end of the test. Otherwise
            // there will be a race condition between XAML resetting the root for test cleanup and
            // TAEF unloading the test binaries. If this is the last test and TAEF unloads the dll
            // containing the event handler before XAML finishes cleaning up, then XAML will AV when
            // it tries to release the event handler. Use the SafeEventRegistration mechanism to
            // unregister the event when the test method exits successfully or fails.
            std::shared_ptr<Event> loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Canvas, Loaded);

            // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(300, 300));

            RunOnUIThread([&] ()
            {
                auto ellipse = ref new xaml::Shapes::Ellipse();
                ellipse->Width = 100;
                ellipse->Height = 100;
                ellipse->StrokeThickness = 10;
                ellipse->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                ellipse->Stroke = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);

                auto rootPanel = ref new Canvas();
                rootPanel->Children->Append(ellipse);

                loadedRegistration.Attach(rootPanel, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
                {
                    loadedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            loadedEvent->WaitForDefault();

            // The masters will come from test\resources\masters, and will be named after the test:
            // - Sample_SampleIntegrationTests_SimpleDCompValidation.master.xml contains the DComp tree.
            // - Sample_SampleIntegrationTests_SimpleDCompValidation.n.master.png contains the nth surface
            //   created on the MockDCompDevice.
            //
            // The MockDComp::SurfaceComparison::ReferencedOnly comparison mode will output and compare only
            // the surfaces that are referenced from the DComp tree at the time of the snapshot. AllSurfaces
            // will output and compare every surface created up to that point, with the released surfaces being
            // empty files, and NoComparison will skip surface comparison altogether.
            //
            // Failed comparisons are copied to the Pictures library (C:\Data\Users\Public\Pictures\ on phone)
            // under the XamlTAEFOutput folder. If any master is missing, the comparison automatically fails.
            // This can be used to create masters for new tests: run the test without any masters, then verify
            // that the output is expected, copy the file back to the test\resources\masters folder, and give
            // them the proper name.
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
        }

        Platform::String^ SampleIntegrationTests::GetPathToFiles() const
        {
            TestCleanupWrapper cleanup;

            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\sample\\");
        }
    }
} } } }
