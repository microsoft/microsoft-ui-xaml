// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AlphaMaskTests.h"
#include <TreeHelper.h>
#include <WUCRenderingScopeGuard.h>
#include <xamltailored.h>
#include <vector>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestEvent.h>
#include <MUX-ETWEvents.h>
#include <ETWWaiterProxy.h>

// Since this module was compiled with CX, the windows.foundation.h cannot be included without a lot of pain points.
#include <Closable.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Composition;
using namespace ::Windows::Foundation;

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;

using ColorHelper = Microsoft::UI::ColorHelper;
using Color = ::Windows::UI::Color;
using Colors = Microsoft::UI::Colors;

#include <PopupHelper.h>
#include <TestComparisonGuards.h>

namespace
{
    struct VerificationSettings
    {
        bool textBlock = true;
        bool shape = true;
        bool image = false;

        bool simulateDeviceLost = false;
        bool simulatePlateauScaleChange = false;

        MockDComp::SurfaceIdMode surfaceIdMode = MockDComp::SurfaceIdMode::CRC;

        Platform::String^ startingParentName = nullptr;
    };

    // TODO: allow bit flags to this that allow indicating if it should only check TextBlock/Shapes...
    //                 For instance, TextBlockLineStacking should skip the shapes?
    // TODO: Modify this to wait on all image opened events before proceeding with comparison?
    void VerifyAlphaMasksInFile(
        Platform::String^ filename,
        Platform::String^ variation,
        MockDComp::SurfaceComparison comparisonMode,
        VerificationSettings verificationSettings)
    {
        TestCleanupWrapper cleanup;
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

        TestServices::Utilities->SetMockDCompSurfaceIdMode(verificationSettings.surfaceIdMode);

        auto root = safe_cast<FrameworkElement^>(LoadXamlFileOnUIThread(filename));
        UIElement^ startingParent = nullptr;
        Popup^ popup = nullptr;
        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = root;

            if (verificationSettings.startingParentName == nullptr)
            {
                startingParent = root;
            }
            else
            {
                startingParent = safe_cast<UIElement^>(root->FindName(verificationSettings.startingParentName));
            }
            popup = safe_cast<Popup^>(root->FindName(L"myPopup"));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            if (popup)
            {
                popup->IsOpen = true;
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        std::vector<CompositionBrush^> alphaMaskBrushes;
        RunOnUIThread([&]
        {
            TreeHelper::WalkTree(
                startingParent,
                [&](DependencyObject^ current)
            {
                if (verificationSettings.textBlock &&
                    dynamic_cast<TextBlock^>(current))
                {
                    alphaMaskBrushes.emplace_back(dynamic_cast<TextBlock^>(current)->GetAlphaMask());
                }
                else if (verificationSettings.shape &&
                    dynamic_cast<Shape^>(current))
                {
                    alphaMaskBrushes.emplace_back(dynamic_cast<Shape^>(current)->GetAlphaMask());
                }
                else if (verificationSettings.image &&
                    dynamic_cast<Image^>(current))
                {
                    alphaMaskBrushes.emplace_back(dynamic_cast<Image^>(current)->GetAlphaMask());
                }
            });
        });
        TestServices::WindowHelper->WaitForIdle();

        if (verificationSettings.simulatePlateauScaleChange)
        {
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(::Windows::Foundation::Size(400, 300), 2.0f);
            TestServices::WindowHelper->WaitForIdle();
        }

        if (verificationSettings.simulateDeviceLost)
        {
            // Use the ETW event to verify there was another image decode and update
            ETWWaiterProxy imageEtwWaiter;

            imageEtwWaiter.Start(
                WINDOWS_UI_XAML_ETW_PROVIDER,
                ImageUpdateHardwareResourcesEnd_value);

            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            imageEtwWaiter.WaitForDefault();
        }

        TestServices::Utilities->VerifyMockDCompOutput(comparisonMode, variation);
    }

    Platform::String^ GetGeneralPath()
    {
        return GetPackageFolder() + L"resources\\native\\external\\foundation\\general\\";
    }

    Platform::String^ GetRenderingPath()
    {
        return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
    }

    Platform::String^ GetImagesPath()
    {
        return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
    }
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

    bool AlphaMaskTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

        bool AlphaMaskTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }

    bool AlphaMaskTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void AlphaMaskTests::TextBlockTests()
    {
        VerificationSettings verificationSettings;
        VerifyAlphaMasksInFile(
            GetRenderingPath() + L"AlphaMaskTextBlockTests.xaml",
            L"4",
            MockDComp::SurfaceComparison::AllSurfaces,
            verificationSettings);
    }

    void AlphaMaskTests::TextBlockBeforeLayout()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);
        TextBlock^ tb = nullptr;
        CompositionBrush^ cb = nullptr;
        RunOnUIThread([&]
        {
            tb = ref new TextBlock;
            tb->FontSize = 15;
            cb = tb->GetAlphaMask();
            TestServices::WindowHelper->WindowContent = tb;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, L"1");

        WEX::Logging::Log::Comment(L"Set TextBlock content and verify update");
        RunOnUIThread([&]
        {
            tb->Text = L"AlphaMask content";
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, L"2");
    }

    void AlphaMaskTests::TextBlockSelection()
    {
        // This test renders 7 TextBlocks (Latin "abc", several Arabic runs, and one
        // Thai run) in two states (before and after SelectAll) and CRC-compares each
        // text alpha-mask surface. Most masks are byte-identical across OS builds.
        // The one surface we parameterize here via $$TextMaskCRC$$ is the Thai run
        // (tb6 in the test XAML). Thai stacks combining vowel marks above the base
        // glyph, and that glyph shaping/positioning got touched in newer OS builds,
        // so its rasterized mask (and thus its CRC) moved. Verified on real VMs: the
        // older baseline (rs5_release) renders CRC 980343538, and newer builds
        // (ge_current) render 1545754801. There's no JPEG here -- this is a real
        // text-rendering difference, not decode rounding. (A couple of other surfaces in
        // this test also shift between OS builds; those already ship both CRC variants as
        // additive .master.png files, so only this one needs the per-OS swap.)
        if (IsOSBuildAtLeast(26200))
        {
            TestServices::Utilities->SetDCompXmlVariable(L"TextMaskCRC", L"1545754801");
        }
        else
        {
            TestServices::Utilities->SetDCompXmlVariable(L"TextMaskCRC", L"980343538");
        }
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree, false);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 800));
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);
        // One of the TextBlocks in this file has CacheMode=BitmapCache set, which is a no-op now
        auto root = safe_cast<UIElement^>(LoadXamlFileOnUIThread(GetRenderingPath() + L"TextBlockSelectionTests.xaml"));
        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        std::vector<CompositionBrush^> alphaMaskBrushes;
        RunOnUIThread([&]
        {
            TreeHelper::WalkTree(root,
                [&](DependencyObject^ current)
            {
                if (dynamic_cast<TextBlock^>(current)) {
                    alphaMaskBrushes.emplace_back(dynamic_cast<TextBlock^>(current)->GetAlphaMask());
                }
            });
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, L"1");

        RunOnUIThread([&]
        {
            TreeHelper::WalkTree(root,
                [&](DependencyObject^ current)
            {
                auto tb = dynamic_cast<TextBlock^>(current);
                if (tb) {
                    tb->SelectAll();
                }
            });
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, L"2");
    }

    void AlphaMaskTests::RenderTransforms()
    {
        VerificationSettings verificationSettings;
        VerifyAlphaMasksInFile(
            GetRenderingPath() + L"AlphaMaskRenderTransforms.xaml",
            nullptr,
            MockDComp::SurfaceComparison::AllSurfaces,
            verificationSettings);
    }

    void AlphaMaskTests::BasicImageTests()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);

        auto openedRegistration1 = CreateSafeEventRegistration(BitmapImage, ImageOpened);
        auto openedRegistration2 = CreateSafeEventRegistration(BitmapImage, ImageOpened);
        auto imageOpenedEvent1 = std::make_shared<Event>();
        auto imageOpenedEvent2 = std::make_shared<Event>();

        CompositionBrush^ mask1 = nullptr;
        CompositionBrush^ mask2 = nullptr;

        Image^ image1 = nullptr;
        Image^ image2 = nullptr;

        RunOnUIThread([&]
        {
            auto root = ref new StackPanel;
            TestServices::WindowHelper->WindowContent = root;

            // Load up a couple of images and obtain their alpha masks
            image1 = ref new Image;
            root->Children->Append(image1);
            auto bitmapImage1 = ref new BitmapImage;
            image1->Source = bitmapImage1;
            auto uri1 = ref new Uri(GetImagesPath() + L"ImageAlphaGradient.png");
            openedRegistration1.Attach(
                bitmapImage1,
                ref new xaml::RoutedEventHandler([imageOpenedEvent1](auto, auto)
            {
                imageOpenedEvent1->Set();
            }));
            bitmapImage1->UriSource = uri1;

            image2 = ref new Image;
            root->Children->Append(image2);
            auto bitmapImage2 = ref new BitmapImage;
            image2->Source = bitmapImage2;
            auto uri2 = ref new Uri(GetImagesPath() + L"ImageAlphaStep.png");
            openedRegistration2.Attach(
                bitmapImage2,
                ref new xaml::RoutedEventHandler([imageOpenedEvent2](auto, auto)
            {
                imageOpenedEvent2->Set();
            }));
            bitmapImage2->UriSource = uri2;
        });
        imageOpenedEvent1->WaitForDefault();
        imageOpenedEvent2->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            mask1 = image1->GetAlphaMask();
            mask2 = image2->GetAlphaMask();
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
    }

    void AlphaMaskTests::BasicShapeTests()
    {
        VerificationSettings verificationSettings;
        VerifyAlphaMasksInFile(
            GetRenderingPath() + L"AlphaMask1.xaml",
            L"1",
            MockDComp::SurfaceComparison::AllSurfaces,
            verificationSettings);
        VerifyAlphaMasksInFile(
            GetRenderingPath() + L"AlphaMask2.xaml",
            L"2",
            MockDComp::SurfaceComparison::AllSurfaces,
            verificationSettings);
        VerifyAlphaMasksInFile(
            GetRenderingPath() + L"AlphaMask3.xaml",
            L"3",
            MockDComp::SurfaceComparison::AllSurfaces,
            verificationSettings);
        VerifyAlphaMasksInFile(
            GetRenderingPath() + L"AlphaMask4.xaml",
            L"4",
            MockDComp::SurfaceComparison::AllSurfaces,
            verificationSettings);
    }

    void AlphaMaskTests::ShapeFillAlpha()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);

        CompositionBrush^ cb = nullptr;
        xaml_shapes::Rectangle^ rect = nullptr;
        RunOnUIThread([&]
        {
            rect = ref new xaml_shapes::Rectangle;
            rect->Width = 200;
            rect->Height = 200;
            rect->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(128, 255, 255, 255));
            cb = rect->GetAlphaMask();
            TestServices::WindowHelper->WindowContent = rect;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
    }

    void AlphaMaskTests::ShapeFillAlphaCleanup()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);
        CompositionBrush^ cb = nullptr;
        RunOnUIThread([&]
        {
            auto rect = ref new xaml_shapes::Rectangle;
            rect->Width = 200;
            rect->Height = 200;
            rect->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(128, 255, 255, 255));
            cb = rect->GetAlphaMask();
            TestServices::WindowHelper->WindowContent = rect;
        });
        TestServices::WindowHelper->WaitForIdle();

        // No AlphaMask should be available anymore since the shape will be cleaned up.
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
    }

    void AlphaMaskTests::ShapeFillImageBrush()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);

        auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
        auto imageOpenedEvent = std::make_shared<Event>();

        CompositionBrush^ cb = nullptr;
        RunOnUIThread([&]
        {
            auto bitmapImage = ref new BitmapImage;
            openedRegistration.Attach(
                bitmapImage,
                ref new xaml::RoutedEventHandler([imageOpenedEvent](auto, auto)
            {
                imageOpenedEvent->Set();
            }));
            bitmapImage->UriSource = ref new Uri(GetImagesPath() + L"ImageAlphaGradient.png");

            auto brush = ref new ImageBrush;
            brush->ImageSource = bitmapImage;

            auto rect = ref new xaml_shapes::Rectangle;
            rect->Width = 200;
            rect->Height = 200;
            rect->Fill = brush;
            TestServices::WindowHelper->WindowContent = rect;
            cb = rect->GetAlphaMask();
        });
        imageOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
    }

    void AlphaMaskTests::MultiElement()
    {
        VerificationSettings verificationSettings;
        verificationSettings.image = true;
        VerifyAlphaMasksInFile(
            GetRenderingPath() + L"AlphaMaskMultiElement.xaml",
            L"1",
            MockDComp::SurfaceComparison::AllSurfaces,
            verificationSettings);
    }

    void AlphaMaskTests::MultiElementPlateauScaleChange()
    {
        VerificationSettings verificationSettings;
        verificationSettings.image = true;
        verificationSettings.simulatePlateauScaleChange = true;
        VerifyAlphaMasksInFile(
            GetRenderingPath() + L"AlphaMaskMultiElement.xaml",
            L"1",
            MockDComp::SurfaceComparison::AllSurfaces,
            verificationSettings);
    }

    void AlphaMaskTests::MultiElementDeviceLost()
    {
        VerificationSettings verificationSettings;
        verificationSettings.image = true;
        verificationSettings.simulateDeviceLost = true;
        VerifyAlphaMasksInFile(
            GetRenderingPath() + L"AlphaMaskMultiElement.xaml",
            L"1",
            MockDComp::SurfaceComparison::AllSurfaces,
            verificationSettings);
    }

    void AlphaMaskTests::MultiElementPopup()
    {
        VerificationSettings verificationSettings;
        verificationSettings.image = true;
        verificationSettings.startingParentName = L"popupPanel";
        VerifyAlphaMasksInFile(
            GetRenderingPath() + L"AlphaMaskMultiElementPopup.xaml",
            L"1",
            MockDComp::SurfaceComparison::AllSurfaces,
            verificationSettings);
    }

    void AlphaMaskTests::ParentlessRTLPopup()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);

        auto root = safe_cast<UIElement^>(LoadXamlFileOnUIThread(GetGeneralPath() + L"EmptyGrid.xaml"));

        CompositionBrush^ cb1 = nullptr;
        TextBlock^ tb1 = nullptr;
        Popup^ popup1 = nullptr;
        RunOnUIThread([&]
        {
            TestServices::WindowHelper->WindowContent = root;

            // Parentless popup with RTL flow direction
            tb1 = ref new TextBlock;
            tb1->Width = 200;
            tb1->Height = 200;
            tb1->FontSize = 15;
            tb1->Text = L"PP_RTL";

            popup1 = ref new Popup;
            auto xamlRoot = root->XamlRoot;
            if (xamlRoot)
            {
                // UAP will return a null content root and does not need this to be set
                popup1->XamlRoot = xamlRoot;
            }
            popup1->Child = tb1;
            popup1->Width = 200;
            popup1->Height = 200;
            popup1->FlowDirection = FlowDirection::RightToLeft;

            cb1 = tb1->GetAlphaMask();
        });
        PopupHelper::OpenPopup(popup1);
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
    }

    void AlphaMaskTests::RequestWhileOffered()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);

        auto wh = TestServices::WindowHelper;
        Image^ image;
        auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
        auto imageOpenedEvent = std::make_shared<Event>();

        RunOnUIThread([&]
        {
            LOG_OUTPUT(L"Creating Image");
            Canvas^ root = ref new Canvas();
            TestServices::WindowHelper->WindowContent = root;

            image = ref new Image;
            auto bitmapImage = ref new BitmapImage;
            image->Source = bitmapImage;
            auto uri = ref new Uri(GetImagesPath() + L"ImageAlphaGradient.png");
            openedRegistration.Attach(
                bitmapImage,
                ref new xaml::RoutedEventHandler([imageOpenedEvent](auto, auto)
            {
                imageOpenedEvent->Set();
            }));
            bitmapImage->UriSource = uri;

            root->Children->Append(image);
        });
        imageOpenedEvent->WaitForDefault();
        wh->WaitForIdle();

        LOG_OUTPUT(L"Triggering Suspend");
        wh->TriggerSuspend(true, true);

        RunOnUIThread([&]
        {
            LOG_OUTPUT(L"Getting AlphaMask, verify we don't crash while suspended'");
            CompositionBrush^ cb = image->GetAlphaMask();
        });
        wh->WaitForIdle();

        LOG_OUTPUT(L"Triggering Resume.  Expect a follow-up RenderWalk to popuplate AlphaMask surface");
        wh->TriggerResume();
        wh->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
    }

    void AlphaMaskTests::RequestWhileDeviceLost()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);

        auto wh = TestServices::WindowHelper;
        auto u = TestServices::Utilities;
        Image^ image;
        auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
        auto imageOpenedEvent = std::make_shared<Event>();

        RunOnUIThread([&]
        {
            LOG_OUTPUT(L"Creating Image");
            Canvas^ root = ref new Canvas();
            TestServices::WindowHelper->WindowContent = root;

            image = ref new Image;
            auto bitmapImage = ref new BitmapImage;
            image->Source = bitmapImage;
            auto uri = ref new Uri(GetImagesPath() + L"ImageAlphaGradient.png");
            openedRegistration.Attach(
                bitmapImage,
                ref new xaml::RoutedEventHandler([imageOpenedEvent](auto, auto)
            {
                imageOpenedEvent->Set();
            }));
            bitmapImage->UriSource = uri;

            root->Children->Append(image);
        });
        imageOpenedEvent->WaitForDefault();
        wh->WaitForIdle();

        // Use the ETW event to verify there was another image decode and update
        ETWWaiterProxy imageEtwWaiter;

        imageEtwWaiter.Start(
            WINDOWS_UI_XAML_ETW_PROVIDER,
            ImageUpdateHardwareResourcesEnd_value);

        LOG_OUTPUT(L"Putting MockDevice in pseudo-device-lost state");
        u->SetReturnDeviceLostWhenCreatingSurfaces(true);

        RunOnUIThread([&]
        {
            LOG_OUTPUT(L"Getting AlphaMask");
            CompositionBrush^ cb = image->GetAlphaMask();
        });

        LOG_OUTPUT(L"Putting MockDevice back in normal state.  Expect recovery and RenderWalk to popuplate AlphaMask surface.");
        u->SetReturnDeviceLostWhenCreatingSurfaces(false);
        wh->WaitForIdle();
        imageEtwWaiter.WaitForDefault();

        u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
    }

    void AlphaMaskTests::RequestWhileDeviceGone()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);

        auto wh = TestServices::WindowHelper;
        Image^ image;
        auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
        auto imageOpenedEvent = std::make_shared<Event>();

        RunOnUIThread([&]
        {
            LOG_OUTPUT(L"Creating Image");
            Canvas^ root = ref new Canvas();
            TestServices::WindowHelper->WindowContent = root;

            image = ref new Image;
            auto bitmapImage = ref new BitmapImage;
            image->Source = bitmapImage;
            auto uri = ref new Uri(GetImagesPath() + L"ImageAlphaGradient.png");
            openedRegistration.Attach(
                bitmapImage,
                ref new xaml::RoutedEventHandler([imageOpenedEvent](auto, auto)
            {
                imageOpenedEvent->Set();
            }));
            bitmapImage->UriSource = uri;

            root->Children->Append(image);
        });
        imageOpenedEvent->WaitForDefault();
        wh->WaitForIdle();

        // Use the ETW event to verify there was another image decode and update
        ETWWaiterProxy imageEtwWaiter;

        imageEtwWaiter.Start(
            WINDOWS_UI_XAML_ETW_PROVIDER,
            ImageUpdateHardwareResourcesEnd_value);

        LOG_OUTPUT(L"Triggering Suspend");
        wh->TriggerSuspend(true, true);

        LOG_OUTPUT(L"Triggering device lost while in suspend state");
        wh->SimulateDeviceLost();

        LOG_OUTPUT(L"Ticking once to run device removal code");
        wh->SynchronouslyTickUIThread(1);

        RunOnUIThread([&]
        {
            wh->TriggerResume();
            LOG_OUTPUT(L"Getting AlphaMask, verify we don't crash due to having no device'");
            CompositionBrush^ cb = image->GetAlphaMask();
        });
        wh->WaitForIdle();
        imageEtwWaiter.WaitForDefault();

        //TO-DO: Fix the bug that MOckDCompDevice keeps all Surfaces regardless of their lifetime
        //and then enable this check, is a known issue
        //TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
    }

    void AlphaMaskTests::ZeroSizeElementTests()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
        TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);

        // Note: These brushes aren't used, but will have their surfaces dumped in the AllSurfaces dumps.
        CompositionBrush^ cb1 = nullptr;
        CompositionBrush^ cb2 = nullptr;
        CompositionBrush^ cb3 = nullptr;

        Grid^ grid = nullptr;
        xaml_shapes::Rectangle^ rect1 = nullptr;
        xaml_shapes::Rectangle^ rect2 = nullptr;
        xaml_shapes::Rectangle^ rect3 = nullptr;

        RunOnUIThread([&]
        {
            grid = ref new Grid;
            TestServices::WindowHelper->WindowContent = grid;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify we can create when the element size is 0x0");
        RunOnUIThread([&]
        {
            rect1 = ref new xaml_shapes::Rectangle;
            rect1->Width = 0;
            rect1->Height = 0;
            rect1->Fill = ref new SolidColorBrush(Colors::White);
            cb1 = rect1->GetAlphaMask();
            grid->Children->Append(rect1);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Verify we can create when the element size is floating point rounding to 0x0");
        RunOnUIThread([&]
        {
            rect2 = ref new xaml_shapes::Rectangle;
            rect2->UseLayoutRounding = false;
            rect2->Width = 0.1;
            rect2->Height = 0.1;
            rect2->Fill = ref new SolidColorBrush(Colors::Red);
            cb2 = rect2->GetAlphaMask();
            grid->Children->Append(rect2);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Create with an element which has 10x10 size...");
        RunOnUIThread([&]
        {
            rect3 = ref new xaml_shapes::Rectangle;
            rect3->UseLayoutRounding = false;
            rect3->Width = 10;
            rect3->Height = 10;
            rect3->Fill = ref new SolidColorBrush(Colors::Blue);
            cb3 = rect3->GetAlphaMask();
            grid->Children->Append(rect3);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Expecting just one mask surface in addition to rect3's rendering (plus hit-testing surface)
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, L"1");

        LOG_OUTPUT(L"...and resize it to near zero...");
        RunOnUIThread([&]
        {
            rect3->Width = 0.1;
            rect3->Height = 0.1;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"...and resize it to zero.");
        RunOnUIThread([&]
        {
            rect3->Width = 0;
            rect3->Height = 0;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Expecting just rect3's rendering (plus hit-testing surface)
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, L"2");

        LOG_OUTPUT(L"Resize to non-zero to ensure masks are created");
        RunOnUIThread([&]
        {
            rect1->Width = 20;
            rect1->Height = 20;
            rect2->Width = 40;
            rect2->Height = 40;
            rect3->Width = 60;
            rect3->Height = 60;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Expecting three masks plus three rects (plus hit-testing surface)
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces, L"3");
    }

    void AlphaMaskTests::UseAfterClose()
    {
        WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

        CompositionBrush^ cb1;

        Grid^ grid;
        xaml_shapes::Rectangle^ rect1;

        RunOnUIThread([&]
        {
            grid = ref new Grid;
            TestServices::WindowHelper->WindowContent = grid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            rect1 = ref new xaml_shapes::Rectangle;
            rect1->Width = 100;
            rect1->Height = 100;
            rect1->Fill = ref new SolidColorBrush(Colors::Purple);
            cb1 = rect1->GetAlphaMask();
            grid->Children->Append(rect1);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            // Close the AlphaMask brush, then force a redraw.
            wrl::ComPtr<IUnknown> cbAsUnk(reinterpret_cast<IUnknown*>(cb1));
            wrl::ComPtr<ABI::Windows::Foundation::IClosable> cbAsClosable;
            VERIFY_SUCCEEDED(cbAsUnk.As(&cbAsClosable));
            VERIFY_SUCCEEDED(cbAsClosable->Close());

            // This will force a redraw, and attempt to update the closed AlphaMask surface.
            // Verify we don't crash.
            rect1->Width = 200;
            rect1->Height = 200;
        });
        TestServices::WindowHelper->WaitForIdle();
    }


} } } } } }
