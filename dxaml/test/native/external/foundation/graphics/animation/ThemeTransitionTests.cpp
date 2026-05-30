// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ThemeTransitionTests.h"

#include <XamlTailored.h>
#include <FileLoader.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <StoryboardMonitorWrapper.h>
#include <XamlTailored.h>

#include <CustomTypeMetadataProvider.h>
#include <NavigationThemeTransitionTestPage.xaml.h>
#include <WUCRenderingScopeGuard.h>

using namespace test_infra;
using namespace Private::Foundation::CustomTypes;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ ThemeTransitionTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\animation\\";
}

bool ThemeTransitionTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ThemeTransitionTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());
    return true;
}

bool ThemeTransitionTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void ThemeTransitionTests::ValidateStaggeringWorks()
{
    TestCleanupWrapper cleanup;

    xaml_controls::ListView^ list;

    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    auto loadedEvent = std::make_shared<Event>();
    auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ListView, Loaded);
    auto createNewListFn = []()
    {
        return safe_cast<xaml_controls::ListView^>(xaml_markup::XamlReader::Load(
            L"<ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' ScrollViewer.VerticalScrollMode='Disabled' ScrollViewer.VerticalScrollBarVisibility='Hidden'>"
            L"    <ListView.ItemContainerTransitions>"
            L"        <TransitionCollection>"
            L"            <EntranceThemeTransition />"
            L"            <RepositionThemeTransition />"
            L"        </TransitionCollection>"
            L"    </ListView.ItemContainerTransitions>"
            L"    <Rectangle Width='100' Height='100' Fill='YellowGreen' />"
            L"    <Rectangle Width='100' Height='100' Fill='Crimson' />"
            L"    <Rectangle Width='100' Height='100' Fill='Blue' />"
            L"</ListView>"
            ));
    };

    // Validation state.
    int storyboardStartedCounter = 0;
    long long lastBeginTime = 0;
    bool expectStaggering = false;

    storyboardMonitor->AttachStartedHandler(
    [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        // When ListView's template is applied, the first storyboard to play is due
        // to the inner ScrollViewer going to the NoIndicator state. BeginTime is going to
        // be null for that one, so let's ignore it. We just want to focus on storyboards
        // targeting the 3 rectangles inside the ListView.
        if (storyboard->BeginTime)
        {
            if (expectStaggering)
            {
                VERIFY_IS_GREATER_THAN(storyboard->BeginTime->Value.Duration, lastBeginTime);
            }
            else
            {
                VERIFY_ARE_EQUAL(0, storyboard->BeginTime->Value.Duration);
            }

            lastBeginTime = storyboard->BeginTime->Value.Duration;
            ++storyboardStartedCounter;
        }
    });

    RunOnUIThread([&]()
    {
        list = createNewListFn();

        VERIFY_IS_FALSE(safe_cast<xaml_animation::EntranceThemeTransition^>(list->ItemContainerTransitions->GetAt(0))->IsStaggeringEnabled);
        VERIFY_IS_TRUE(safe_cast<xaml_animation::RepositionThemeTransition^>(list->ItemContainerTransitions->GetAt(1))->IsStaggeringEnabled);

        loadedRegistration.Attach(list, ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"List loaded.");
            loadedEvent->Set();
        }));
        TestServices::WindowHelper->WindowContent = list;
    });

    LOG_OUTPUT(L"Waiting for List Loaded event.");
    loadedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Validating EntranceThemeTransition with IsStaggeringEnabled == false.");
    {
        // Each one of the three rectangles will have an entrance animation.
        VERIFY_ARE_EQUAL(3, storyboardStartedCounter);
        storyboardStartedCounter = 0;
        lastBeginTime = 0;
    }

    LOG_OUTPUT(L"Validating RepositionThemeTransition with IsStaggeringEnabled == true.");
    {
        RunOnUIThread([&]()
        {
            expectStaggering = true;
            list->Items->RemoveAt(0);
        });
        TestServices::WindowHelper->WaitForIdle();

        // We removed the first rectangle, the other 2 will have the reposition animation
        // on them as they slide up.
        VERIFY_ARE_EQUAL(2, storyboardStartedCounter);
        storyboardStartedCounter = 0;
        lastBeginTime = 0;
    }

    LOG_OUTPUT(L"Validating EntranceThemeTransition with IsStaggeringEnabled == true.");
    {
        RunOnUIThread([&]()
        {
            // We can't just reuse the existing list because its transition context remember it has already played
            // the entrance theme transition, and won't play it again.
            list = createNewListFn();
            safe_cast<xaml_animation::EntranceThemeTransition^>(list->ItemContainerTransitions->GetAt(0))->IsStaggeringEnabled = true;
            TestServices::WindowHelper->WindowContent = list;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Each one of the three rectangles will have an entrance animation.
        VERIFY_ARE_EQUAL(3, storyboardStartedCounter);
        storyboardStartedCounter = 0;
        lastBeginTime = 0;
    }

    LOG_OUTPUT(L"Validating RepositionThemeTransition with IsStaggeringEnabled == false.");
    {
        RunOnUIThread([&]()
        {
            // We can't just reuse the existing list because its transition context remember it has already played
            // the entrance theme transition, and won't play it again.
            expectStaggering = false;
            list = createNewListFn();
            safe_cast<xaml_animation::RepositionThemeTransition^>(list->ItemContainerTransitions->GetAt(1))->IsStaggeringEnabled = false;
            TestServices::WindowHelper->WindowContent = list;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Each one of the three rectangles will have an entrance animation.
        VERIFY_ARE_EQUAL(3, storyboardStartedCounter);
        storyboardStartedCounter = 0;
        lastBeginTime = 0;

        RunOnUIThread([&]()
        {
            list->Items->RemoveAt(0);
        });
        TestServices::WindowHelper->WaitForIdle();

        // We removed the first rectangle, the other 2 will have the reposition animation
        // on them as they slide up.
        VERIFY_ARE_EQUAL(2, storyboardStartedCounter);
    }
}

void ThemeTransitionTests::ValidateSlideThemeTransitionEffect()
{
    TestCleanupWrapper cleanup;
    xaml_controls::Page ^mainPage;

    bool needNavigateAway = true;
    int storyboardStartedCounter = 0;
    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    DOUBLE navigateAwayDistance = 0;
    DOUBLE navigateToDistance = 0;
    storyboardMonitor->AttachStartedHandler(
        [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        --storyboardStartedCounter;

        VERIFY_IS_NOT_NULL(storyboard);

        VERIFY_ARE_EQUAL(3u, storyboard->Children->Size);
        auto translateX = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));
        auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(1));
        auto isHitTestVisible = safe_cast<xaml_animation::ObjectAnimationUsingKeyFrames^>(storyboard->Children->GetAt(2));

        // The order of storyboard start notifications doesn't appear to be deterministic, so we
        // need to look at the storyboards themselves to see whether we are handling a navigated
        // to or a navigated away.
        bool isNavigateAway = false;
        if (needNavigateAway)
        {
            isNavigateAway = opacity->KeyFrames->GetAt(0)->Value == 1;
            needNavigateAway = !isNavigateAway;
        }

        if (isNavigateAway)
        {
            LOG_OUTPUT(L"Validate Navigate Away animations");

            VERIFY_ARE_EQUAL(2u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(1)->Value);

            VERIFY_ARE_EQUAL(2u, translateX->KeyFrames->Size);
            VERIFY_ARE_EQUAL(0.0, translateX->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(navigateAwayDistance, translateX->KeyFrames->GetAt(1)->Value);

            VERIFY_ARE_EQUAL(1u, isHitTestVisible->KeyFrames->Size);
        }
        else
        {
            LOG_OUTPUT(L"Validate Navigate To animations");

            VERIFY_ARE_EQUAL(2u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(1)->Value);

            VERIFY_ARE_EQUAL(2u, translateX->KeyFrames->Size);
            VERIFY_ARE_EQUAL(navigateToDistance, translateX->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(0.0, translateX->KeyFrames->GetAt(1)->Value);

            VERIFY_ARE_EQUAL(1u, isHitTestVisible->KeyFrames->Size);
        }
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating to main page.");
        mainPage = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    TestServices::WindowHelper->WaitForIdle();

    DOUBLE hideDistance = -150;
    DOUBLE showDistance = -200;

    LOG_OUTPUT(L"Navigating from main page to the target page using effect 'Left'");
    storyboardStartedCounter = 2;
    navigateAwayDistance = -hideDistance;
    navigateToDistance = showDistance;
    needNavigateAway = true;
    RunOnUIThread([&]()
    {
        auto transitionInfo = ref new xaml_animation::SlideNavigationTransitionInfo();
        VERIFY_ARE_EQUAL(transitionInfo->Effect, xaml_animation::SlideNavigationTransitionEffect::FromBottom);
        transitionInfo->Effect = xaml_animation::SlideNavigationTransitionEffect::FromLeft;
        VERIFY_ARE_EQUAL(transitionInfo->Effect, xaml_animation::SlideNavigationTransitionEffect::FromLeft);
        mainPage->Frame->Navigate(wxaml_interop::TypeName(NavigationThemeTransitionTestPage::typeid), nullptr, transitionInfo);
    });
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_ARE_EQUAL(0, storyboardStartedCounter);

    LOG_OUTPUT(L"Navigating back to main page from the target page (implied effect 'Left'");
    storyboardStartedCounter = 2;
    navigateAwayDistance = showDistance;
    navigateToDistance = -hideDistance;
    needNavigateAway = true;
    RunOnUIThread([&]()
    {
        mainPage->Frame->GoBack();
    });
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_ARE_EQUAL(0, storyboardStartedCounter);

    LOG_OUTPUT(L"Navigating from main page to the target page using effect 'Right'");
    storyboardStartedCounter = 2;
    navigateAwayDistance = hideDistance;
    navigateToDistance = -showDistance;
    needNavigateAway = true;
    RunOnUIThread([&]()
    {
        auto transitionInfo = ref new xaml_animation::SlideNavigationTransitionInfo();
        VERIFY_ARE_EQUAL(transitionInfo->Effect, xaml_animation::SlideNavigationTransitionEffect::FromBottom);
        transitionInfo->Effect = xaml_animation::SlideNavigationTransitionEffect::FromRight;
        VERIFY_ARE_EQUAL(transitionInfo->Effect, xaml_animation::SlideNavigationTransitionEffect::FromRight);
        mainPage->Frame->Navigate(wxaml_interop::TypeName(NavigationThemeTransitionTestPage::typeid), nullptr, transitionInfo);
    });
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_ARE_EQUAL(0, storyboardStartedCounter);

    LOG_OUTPUT(L"Navigating back to main page from the target page (implied effect 'Right'");
    storyboardStartedCounter = 2;
    navigateAwayDistance = -showDistance;
    navigateToDistance = hideDistance;
    needNavigateAway = true;
    RunOnUIThread([&]()
    {
        mainPage->Frame->GoBack();
    });
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_ARE_EQUAL(0, storyboardStartedCounter);

}

void ThemeTransitionTests::ValidateNavigationThemeTransitionWorksWhenNotPresent()
{
    TestCleanupWrapper cleanup;

    xaml_controls::Page ^firstPage;

    int storyboardStartedCounter = 0;
    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    bool needNavigateAway = true;
    storyboardMonitor->AttachStartedHandler(
        [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        ++storyboardStartedCounter;

        // The order of storyboard start notifications doesn't appear to be deterministic, so we
        // need to look at the storyboards themselves to see whether we are handling a navigated
        // to or a navigated away.
        bool isNavigateAway = false;
        if (needNavigateAway)
        {
            isNavigateAway = storyboard->Children->Size == 1;
            needNavigateAway = !isNavigateAway;
        }

        if (isNavigateAway)
        {
            LOG_OUTPUT(L"Validate Navigate Away animations");
            VERIFY_ARE_EQUAL(1u, storyboard->Children->Size);
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).Opacity"), xaml_animation::Storyboard::GetTargetProperty(opacity));

            VERIFY_ARE_EQUAL(2u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(1)->Value);
        }
        else
        {
            LOG_OUTPUT(L"Validate Navigate To animations");
            VERIFY_ARE_EQUAL(2u, storyboard->Children->Size);
            auto translateY = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(1));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY"), xaml_animation::Storyboard::GetTargetProperty(translateY));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).Opacity"), xaml_animation::Storyboard::GetTargetProperty(opacity));

            VERIFY_ARE_EQUAL(3u, translateY->KeyFrames->Size);
            VERIFY_ARE_EQUAL(140.0, translateY->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(140.0, translateY->KeyFrames->GetAt(1)->Value);
            VERIFY_ARE_EQUAL(0.0, translateY->KeyFrames->GetAt(2)->Value);

            VERIFY_ARE_EQUAL(2u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(1)->Value);
        }
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating to the first page.");
        firstPage = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // To repro this issue, the first page must have a NavigationThemeTransition in its
        // transition collection but the second page should not.
        // We should not crash after the Navigate call below.
        firstPage->Transitions = ref new xaml_animation::TransitionCollection();
        firstPage->Transitions->Append(ref new xaml_animation::NavigationThemeTransition());

        LOG_OUTPUT(L"Navigating to the second page.");
        firstPage->Frame->Navigate(::Windows::UI::Xaml::Interop::TypeName(firstPage->GetType()));
    });

    TestServices::WindowHelper->WaitForIdle();
    VERIFY_ARE_EQUAL(2, storyboardStartedCounter);
}

void ThemeTransitionTests::EdgeUIThemeTransition()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    int storyboardStartedCount = 0;

    storyboardMonitor->AttachStartedHandler(
    [&storyboardStartedCount]
    (xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        ++storyboardStartedCount;
    });

    TestThemeTransitionXaml(GetResourcesPath() + L"EdgeUIThemeTransition.xaml");

    VERIFY_ARE_EQUAL(storyboardStartedCount, 4);
}

void ThemeTransitionTests::PaneThemeTransition()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    int storyboardStartedCount = 0;

    storyboardMonitor->AttachStartedHandler(
    [&storyboardStartedCount]
    (xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        ++storyboardStartedCount;
    });

    TestThemeTransitionXaml(GetResourcesPath() + L"PaneThemeTransition.xaml");

    VERIFY_ARE_EQUAL(storyboardStartedCount, 4);
}

void ThemeTransitionTests::ContentThemeTransition()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    int storyboardStartedCount = 0;

    storyboardMonitor->AttachStartedHandler(
    [&storyboardStartedCount]
    (xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        ++storyboardStartedCount;
    });

    TestThemeTransitionXaml(GetResourcesPath() + L"ContentThemeTransition.xaml");

    VERIFY_ARE_EQUAL(storyboardStartedCount, 2);
}

void ThemeTransitionTests::TestThemeTransitionXaml(Platform::String^ path)
{
    auto rootPanel = safe_cast<xaml_controls::Panel^>(LoadXamlFileOnUIThread(path));

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ThemeTransitionTests::ValidateEntranceNavigationThemeTransition()
{
    TestCleanupWrapper cleanup;

    xaml_controls::Page ^mainPage;

    bool needNavigateAway = true;
    int storyboardStartedCounter = 2;
    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    storyboardMonitor->AttachStartedHandler(
        [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        --storyboardStartedCounter;

        VERIFY_IS_NOT_NULL(storyboard);

        // The order of storyboard start notifications doesn't appear to be deterministic, so we
        // need to look at the storyboards themselves to see whether we are handling a navigated
        // to or a navigated away.
        bool isNavigateAway = false;
        if (needNavigateAway)
        {
            isNavigateAway = storyboard->Children->Size == 1;
            needNavigateAway = !isNavigateAway;
        }

        if (isNavigateAway)
        {
            LOG_OUTPUT(L"Validate Navigate Away animations");
            VERIFY_ARE_EQUAL(1u, storyboard->Children->Size);
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).Opacity"), xaml_animation::Storyboard::GetTargetProperty(opacity));

            VERIFY_ARE_EQUAL(2u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(1)->Value);
        }
        else
        {
            LOG_OUTPUT(L"Validate Navigate To animations");
            VERIFY_ARE_EQUAL(2u, storyboard->Children->Size);
            auto translateY = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(1));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY"), xaml_animation::Storyboard::GetTargetProperty(translateY));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).Opacity"), xaml_animation::Storyboard::GetTargetProperty(opacity));

            VERIFY_ARE_EQUAL(3u, translateY->KeyFrames->Size);
            VERIFY_ARE_EQUAL(140.0, translateY->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(140.0, translateY->KeyFrames->GetAt(1)->Value);
            VERIFY_ARE_EQUAL(0.0, translateY->KeyFrames->GetAt(2)->Value);

            VERIFY_ARE_EQUAL(2u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(1)->Value);
        }
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating to main page.");
        mainPage = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating from main page to the target page with desired navigation theme transition.");
        mainPage->Frame->Navigate(wxaml_interop::TypeName(NavigationThemeTransitionTestPage::typeid), nullptr, ref new xaml_animation::EntranceNavigationTransitionInfo());
    });
    TestServices::WindowHelper->WaitForIdle();

    VERIFY_ARE_EQUAL(0, storyboardStartedCounter);
}

void ThemeTransitionTests::ValidateSlideNavigationThemeTransition()
{
    TestCleanupWrapper cleanup;

    xaml_controls::Page ^mainPage;

    bool needNavigateAway = true;
    int storyboardStartedCounter = 2;
    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    storyboardMonitor->AttachStartedHandler(
        [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        --storyboardStartedCounter;

        VERIFY_IS_NOT_NULL(storyboard);

        // The order of storyboard start notifications doesn't appear to be deterministic, so we
        // need to look at the storyboards themselves to see whether we are handling a navigated
        // to or a navigated away.
        bool isNavigateAway = false;
        if (needNavigateAway)
        {
            isNavigateAway = storyboard->Children->Size == 2;
            needNavigateAway = !isNavigateAway;
        }

        if (isNavigateAway)
        {
            LOG_OUTPUT(L"Validate Navigate Away animations");
            VERIFY_ARE_EQUAL(2u, storyboard->Children->Size);
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));
            auto isHitTestVisible = safe_cast<xaml_animation::ObjectAnimationUsingKeyFrames^>(storyboard->Children->GetAt(1));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).Opacity"), xaml_animation::Storyboard::GetTargetProperty(opacity));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"UIElement.IsHitTestVisible"), xaml_animation::Storyboard::GetTargetProperty(isHitTestVisible));

            VERIFY_ARE_EQUAL(3u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(1)->Value);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(2)->Value);

            VERIFY_ARE_EQUAL(1u, isHitTestVisible->KeyFrames->Size);
        }
        else
        {
            LOG_OUTPUT(L"Validate Navigate To animations");
            VERIFY_ARE_EQUAL(3u, storyboard->Children->Size);
            auto translateY = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(1));
            auto isHitTestVisible = safe_cast<xaml_animation::ObjectAnimationUsingKeyFrames^>(storyboard->Children->GetAt(2));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY"), xaml_animation::Storyboard::GetTargetProperty(translateY));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).Opacity"), xaml_animation::Storyboard::GetTargetProperty(opacity));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"UIElement.IsHitTestVisible"), xaml_animation::Storyboard::GetTargetProperty(isHitTestVisible));

            VERIFY_ARE_EQUAL(3u, translateY->KeyFrames->Size);
            VERIFY_ARE_EQUAL(200.0, translateY->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(200.0, translateY->KeyFrames->GetAt(1)->Value);
            VERIFY_ARE_EQUAL(0.0, translateY->KeyFrames->GetAt(2)->Value);

            VERIFY_ARE_EQUAL(3u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(1)->Value);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(2)->Value);

            VERIFY_ARE_EQUAL(1u, isHitTestVisible->KeyFrames->Size);
        }
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating to main page.");
        mainPage = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating from main page to the target page with desired navigation theme transition.");
        mainPage->Frame->Navigate(wxaml_interop::TypeName(NavigationThemeTransitionTestPage::typeid), nullptr, ref new xaml_animation::SlideNavigationTransitionInfo());
    });
    TestServices::WindowHelper->WaitForIdle();

    VERIFY_ARE_EQUAL(0, storyboardStartedCounter);
}

void ThemeTransitionTests::ValidateDrillInNavigationThemeTransition()
{
    TestCleanupWrapper cleanup;

    xaml_controls::Page ^mainPage;

    bool needNavigateAway = true;
    int storyboardStartedCounter = 2;
    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    storyboardMonitor->AttachStartedHandler(
        [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        --storyboardStartedCounter;

        VERIFY_IS_NOT_NULL(storyboard);

        // The order of storyboard start notifications doesn't appear to be deterministic, so we
        // need to look at the storyboards themselves to see whether we are handling a navigated
        // to or a navigated away.
        bool isNavigateAway = false;
        if (needNavigateAway)
        {
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(3));
            isNavigateAway = opacity->KeyFrames->GetAt(0)->Value == 1;
            needNavigateAway = !isNavigateAway;
        }

        if (isNavigateAway)
        {
            LOG_OUTPUT(L"Validate Navigate Away animations");
            VERIFY_ARE_EQUAL(4u, storyboard->Children->Size);
            auto transformOrigin = safe_cast<xaml_animation::PointAnimation^>(storyboard->Children->GetAt(0));
            auto scaleX = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(1));
            auto scaleY = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(2));
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(3));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).TransformOrigin"), xaml_animation::Storyboard::GetTargetProperty(transformOrigin));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleX"), xaml_animation::Storyboard::GetTargetProperty(scaleX));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleY"), xaml_animation::Storyboard::GetTargetProperty(scaleY));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).Opacity"), xaml_animation::Storyboard::GetTargetProperty(opacity));

            VERIFY_IS_NULL(transformOrigin->From);
            VERIFY_IS_NOT_NULL(transformOrigin->To);
            VERIFY_ARE_EQUAL(0.5, transformOrigin->To->Value.X);
            VERIFY_ARE_EQUAL(0.5, transformOrigin->To->Value.Y);
            VERIFY_IS_NULL(transformOrigin->By);

            VERIFY_ARE_EQUAL(2u, scaleX->KeyFrames->Size);
            VERIFY_ARE_EQUAL(1.0, scaleX->KeyFrames->GetAt(0)->Value);
            VERIFY_IS_TRUE(std::abs(1.04 - scaleX->KeyFrames->GetAt(1)->Value) < 0.00001);

            VERIFY_ARE_EQUAL(2u, scaleY->KeyFrames->Size);
            VERIFY_ARE_EQUAL(1.0, scaleY->KeyFrames->GetAt(0)->Value);
            VERIFY_IS_TRUE(std::abs(1.04 - scaleY->KeyFrames->GetAt(1)->Value) < 0.00001);

            VERIFY_ARE_EQUAL(2u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(1)->Value);
        }
        else
        {
            LOG_OUTPUT(L"Validate Navigate To animations");
            VERIFY_ARE_EQUAL(4u, storyboard->Children->Size);
            auto transformOrigin = safe_cast<xaml_animation::PointAnimation^>(storyboard->Children->GetAt(0));
            auto scaleX = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(1));
            auto scaleY = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(2));
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(3));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).TransformOrigin"), xaml_animation::Storyboard::GetTargetProperty(transformOrigin));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleX"), xaml_animation::Storyboard::GetTargetProperty(scaleX));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleY"), xaml_animation::Storyboard::GetTargetProperty(scaleY));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).Opacity"), xaml_animation::Storyboard::GetTargetProperty(opacity));

            VERIFY_IS_NULL(transformOrigin->From);
            VERIFY_IS_NOT_NULL(transformOrigin->To);
            VERIFY_ARE_EQUAL(0.5, transformOrigin->To->Value.X);
            VERIFY_ARE_EQUAL(0.5, transformOrigin->To->Value.Y);
            VERIFY_IS_NULL(transformOrigin->By);

            VERIFY_ARE_EQUAL(2u, scaleX->KeyFrames->Size);
            VERIFY_IS_TRUE(std::abs(0.94 - scaleX->KeyFrames->GetAt(0)->Value) < 0.00001);
            VERIFY_ARE_EQUAL(1.0, scaleX->KeyFrames->GetAt(1)->Value);

            VERIFY_ARE_EQUAL(2u, scaleY->KeyFrames->Size);
            VERIFY_IS_TRUE(std::abs(0.94 - scaleY->KeyFrames->GetAt(0)->Value) < 0.00001);
            VERIFY_ARE_EQUAL(1.0, scaleY->KeyFrames->GetAt(1)->Value);

            VERIFY_ARE_EQUAL(2u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(1)->Value);
        }
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating to main page.");
        mainPage = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating from main page to the target page with desired navigation theme transition.");
        mainPage->Frame->Navigate(wxaml_interop::TypeName(NavigationThemeTransitionTestPage::typeid), nullptr, ref new xaml_animation::DrillInNavigationTransitionInfo());
    });
    TestServices::WindowHelper->WaitForIdle();

    VERIFY_ARE_EQUAL(0, storyboardStartedCounter);
}

void ThemeTransitionTests::ValidateCommonNavigationThemeTransition()
{
    TestCleanupWrapper cleanup;

    xaml_controls::Page ^mainPage;

    bool needNavigateAway = true;
    int storyboardStartedCounter = 2;
    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    storyboardMonitor->AttachStartedHandler(
        [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        --storyboardStartedCounter;

        VERIFY_IS_NOT_NULL(storyboard);

        // The order of storyboard start notifications doesn't appear to be deterministic, so we
        // need to look at the storyboards themselves to see whether we are handling a navigated
        // to or a navigated away.
        bool isNavigateAway = false;
        if (needNavigateAway)
        {
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(3));
            isNavigateAway = opacity->KeyFrames->GetAt(0)->Value == 1;
            needNavigateAway = !isNavigateAway;
        }

        if (isNavigateAway)
        {
            LOG_OUTPUT(L"Validate Navigate Away animations");
            VERIFY_ARE_EQUAL(5u, storyboard->Children->Size);
            auto rotationY = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));
            auto centerOfRotationX = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(1));
            auto centerOfRotationZ = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(2));
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(3));
            auto isHitTestVisible = safe_cast<xaml_animation::ObjectAnimationUsingKeyFrames^>(storyboard->Children->GetAt(4));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.Projection).(PlaneProjection.RotationY)"), xaml_animation::Storyboard::GetTargetProperty(rotationY));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.Projection).(PlaneProjection.CenterOfRotationX)"), xaml_animation::Storyboard::GetTargetProperty(centerOfRotationX));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.Projection).(PlaneProjection.CenterOfRotationZ)"), xaml_animation::Storyboard::GetTargetProperty(centerOfRotationZ));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).Opacity"), xaml_animation::Storyboard::GetTargetProperty(opacity));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"UIElement.IsHitTestVisible"), xaml_animation::Storyboard::GetTargetProperty(isHitTestVisible));

            VERIFY_ARE_EQUAL(2u, rotationY->KeyFrames->Size);
            VERIFY_ARE_EQUAL(0.0, rotationY->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(50.0, rotationY->KeyFrames->GetAt(1)->Value);

            VERIFY_ARE_EQUAL(1u, centerOfRotationX->KeyFrames->Size);
            VERIFY_IS_TRUE(std::abs(-0.1 - centerOfRotationX->KeyFrames->GetAt(0)->Value) < 0.00001);

            VERIFY_ARE_EQUAL(1u, centerOfRotationZ->KeyFrames->Size);
            VERIFY_ARE_EQUAL(-100.0, centerOfRotationZ->KeyFrames->GetAt(0)->Value);

            VERIFY_ARE_EQUAL(3u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(1)->Value);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(2)->Value);

            VERIFY_ARE_EQUAL(1u, isHitTestVisible->KeyFrames->Size);
        }
        else
        {
            LOG_OUTPUT(L"Validate Navigate To animations");
            VERIFY_ARE_EQUAL(5u, storyboard->Children->Size);
            auto rotationY = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));
            auto centerOfRotationX = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(1));
            auto centerOfRotationZ = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(2));
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(3));
            auto isHitTestVisible = safe_cast<xaml_animation::ObjectAnimationUsingKeyFrames^>(storyboard->Children->GetAt(4));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.Projection).(PlaneProjection.RotationY)"), xaml_animation::Storyboard::GetTargetProperty(rotationY));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.Projection).(PlaneProjection.CenterOfRotationX)"), xaml_animation::Storyboard::GetTargetProperty(centerOfRotationX));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.Projection).(PlaneProjection.CenterOfRotationZ)"), xaml_animation::Storyboard::GetTargetProperty(centerOfRotationZ));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).Opacity"), xaml_animation::Storyboard::GetTargetProperty(opacity));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"UIElement.IsHitTestVisible"), xaml_animation::Storyboard::GetTargetProperty(isHitTestVisible));

            VERIFY_ARE_EQUAL(3u, rotationY->KeyFrames->Size);
            VERIFY_ARE_EQUAL(-80.0, rotationY->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(-80.0, rotationY->KeyFrames->GetAt(1)->Value);
            VERIFY_ARE_EQUAL(0.0, rotationY->KeyFrames->GetAt(2)->Value);

            VERIFY_ARE_EQUAL(1u, centerOfRotationX->KeyFrames->Size);
            VERIFY_IS_TRUE(std::abs(-0.1 - centerOfRotationX->KeyFrames->GetAt(0)->Value) < 0.00001);

            VERIFY_ARE_EQUAL(1u, centerOfRotationZ->KeyFrames->Size);
            VERIFY_ARE_EQUAL(-100.0, centerOfRotationZ->KeyFrames->GetAt(0)->Value);

            VERIFY_ARE_EQUAL(3u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(1)->Value);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(2)->Value);

            VERIFY_ARE_EQUAL(1u, isHitTestVisible->KeyFrames->Size);
        }
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating to main page.");
        mainPage = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating from main page to the target page with desired navigation theme transition.");
        mainPage->Frame->Navigate(wxaml_interop::TypeName(NavigationThemeTransitionTestPage::typeid), nullptr, ref new xaml_animation::CommonNavigationTransitionInfo());
    });
    TestServices::WindowHelper->WaitForIdle();

    VERIFY_ARE_EQUAL(0, storyboardStartedCounter);
}

void ThemeTransitionTests::ValidateContinuumNavigationThemeTransition()
{
    TestCleanupWrapper cleanup;

    xaml_controls::Page ^mainPage;

    bool needNavigateAway = true;
    int storyboardStartedCounter = 2;
    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    storyboardMonitor->AttachStartedHandler(
        [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        --storyboardStartedCounter;

        VERIFY_IS_NOT_NULL(storyboard);

        // The order of storyboard start notifications doesn't appear to be deterministic, so we
        // need to look at the storyboards themselves to see whether we are handling a navigated
        // to or a navigated away.
        bool isNavigateAway = false;
        if (needNavigateAway)
        {
            auto rotationY = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));
            isNavigateAway = storyboard->Children->Size == 2;
            needNavigateAway = !isNavigateAway;
        }

        if (isNavigateAway)
        {
            LOG_OUTPUT(L"Validate Navigate Away animations");
            VERIFY_ARE_EQUAL(2u, storyboard->Children->Size);
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));
            auto isHitTestVisible = safe_cast<xaml_animation::ObjectAnimationUsingKeyFrames^>(storyboard->Children->GetAt(1));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).Opacity"), xaml_animation::Storyboard::GetTargetProperty(opacity));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"UIElement.IsHitTestVisible"), xaml_animation::Storyboard::GetTargetProperty(isHitTestVisible));

            VERIFY_ARE_EQUAL(4u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(1)->Value);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(2)->Value);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(3)->Value);

            VERIFY_ARE_EQUAL(1u, isHitTestVisible->KeyFrames->Size);
        }
        else
        {
            LOG_OUTPUT(L"Validate Navigate To animations");
            VERIFY_ARE_EQUAL(5u, storyboard->Children->Size);
            auto scaleX = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(0));
            auto scaleY = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(1));
            auto opacity = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(storyboard->Children->GetAt(2));
            auto transformOrigin = safe_cast<xaml_animation::PointAnimation^>(storyboard->Children->GetAt(3));
            auto isHitTestVisible = safe_cast<xaml_animation::ObjectAnimationUsingKeyFrames^>(storyboard->Children->GetAt(4));

            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleX"), xaml_animation::Storyboard::GetTargetProperty(scaleX));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleY"), xaml_animation::Storyboard::GetTargetProperty(scaleY));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).Opacity"), xaml_animation::Storyboard::GetTargetProperty(opacity));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"(UIElement.TransitionTarget).TransformOrigin"), xaml_animation::Storyboard::GetTargetProperty(transformOrigin));
            VERIFY_ARE_EQUAL(ref new Platform::String(L"UIElement.IsHitTestVisible"), xaml_animation::Storyboard::GetTargetProperty(isHitTestVisible));

            VERIFY_ARE_EQUAL(2u, scaleX->KeyFrames->Size);
            VERIFY_IS_TRUE(std::abs(0.9 - scaleX->KeyFrames->GetAt(0)->Value) < 0.00001);
            VERIFY_ARE_EQUAL(1.0, scaleX->KeyFrames->GetAt(1)->Value);

            VERIFY_ARE_EQUAL(2u, scaleY->KeyFrames->Size);
            VERIFY_IS_TRUE(std::abs(0.9 - scaleY->KeyFrames->GetAt(0)->Value) < 0.00001);
            VERIFY_ARE_EQUAL(1.0, scaleY->KeyFrames->GetAt(1)->Value);

            VERIFY_ARE_EQUAL(3u, opacity->KeyFrames->Size);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(0)->Value);
            VERIFY_ARE_EQUAL(0.0, opacity->KeyFrames->GetAt(1)->Value);
            VERIFY_ARE_EQUAL(1.0, opacity->KeyFrames->GetAt(2)->Value);

            VERIFY_IS_NULL(transformOrigin->From);
            VERIFY_IS_NOT_NULL(transformOrigin->To);
            VERIFY_ARE_EQUAL(0.5, transformOrigin->To->Value.X);
            VERIFY_ARE_EQUAL(0.5, transformOrigin->To->Value.Y);
            VERIFY_IS_NULL(transformOrigin->By);

            VERIFY_ARE_EQUAL(1u, isHitTestVisible->KeyFrames->Size);
        }
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating to main page.");
        mainPage = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating from main page to the target page with desired navigation theme transition.");
        mainPage->Frame->Navigate(wxaml_interop::TypeName(NavigationThemeTransitionTestPage::typeid), nullptr, ref new xaml_animation::ContinuumNavigationTransitionInfo());
    });
    TestServices::WindowHelper->WaitForIdle();

    VERIFY_ARE_EQUAL(0, storyboardStartedCounter);
}

void ThemeTransitionTests::ValidateSuppressNavigationThemeTransition()
{
    TestCleanupWrapper cleanup;

    xaml_controls::Page ^mainPage;

    int storyboardStartedCounter = 0;
    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    storyboardMonitor->AttachStartedHandler(
        [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        --storyboardStartedCounter;
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating to main page.");
        mainPage = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating from main page to the target page with desired navigation theme transition.");
        mainPage->Frame->Navigate(wxaml_interop::TypeName(NavigationThemeTransitionTestPage::typeid), nullptr, ref new xaml_animation::SuppressNavigationTransitionInfo());
    });
    TestServices::WindowHelper->WaitForIdle();

    VERIFY_ARE_EQUAL(0, storyboardStartedCounter);
}

void ThemeTransitionTests::ValidateContentOverride()
{
    TestCleanupWrapper cleanup;

    NavigationThemeTransitionTestPageConfiguration config;
    config.PageInitialization = [&](Microsoft::UI::Xaml::Controls::Page^ page)
    {
        LOG_OUTPUT(L"Removing navigation transition from page transition collection");
        page->Transitions->Clear();
    };


    xaml_controls::Page ^mainPage;

    int storyboardStartedCounter = 0;
    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    storyboardMonitor->AttachStartedHandler(
        [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        --storyboardStartedCounter;
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating to main page.");
        mainPage = TestServices::WindowHelper->SetupSimulatedAppPage();
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Adding the suppress navigation transition info the content should override the default
        // and we should get no transition storyboards.
        LOG_OUTPUT(L"Set up content transition on frame");
        mainPage->Frame->ContentTransitions = ref new xaml_animation::TransitionCollection();
        auto transition = ref new xaml_animation::NavigationThemeTransition();
        transition->DefaultNavigationTransitionInfo = ref new xaml_animation::SuppressNavigationTransitionInfo();
        mainPage->Frame->ContentTransitions->Append(transition);
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Navigating from main page to the target page with default theme transition.");
        mainPage->Frame->Navigate(wxaml_interop::TypeName(NavigationThemeTransitionTestPage::typeid), nullptr, nullptr);
    });
    TestServices::WindowHelper->WaitForIdle();

    VERIFY_ARE_EQUAL(0, storyboardStartedCounter);
}
} } } } } }
