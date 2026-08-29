// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "StrictModeTests.h"
#include "Layering.CustomTypes.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FeatureFlags.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace ::Windows::UI::Composition;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Layering {

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)

// TODO:  Dynamically enable velocity instead of skipping test
#define VELOCITY_TESTGUARD_XAML2018 if (!Feature_Xaml2018::IsEnabled()) { LOG_OUTPUT(L"XAML2018 velocity feature disabled, skipping test"); return; }

    bool StrictModeTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool StrictModeTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool StrictModeTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void StrictModeTests::ValidateNonStrictType()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            auto element = ref new CustomFrameworkElement();

            LOG_OUTPUT(L"Validate that accessing a strict-agnostic API does not throw when the element is not strict.");
            VERIFY_NO_THROW(element->Interactions);

            LOG_OUTPUT(L"Put the element into strict mode.");
            element->Rotation = 45;

            LOG_OUTPUT(L"Validate that accessing a strict-agnostic API does not throw when the element is strict.");
            VERIFY_NO_THROW(element->Interactions);

            LOG_OUTPUT(L"Validate that accessing a strict-only API does not throw when the element is strict.");
            VERIFY_NO_THROW(element->Rotation = 90);

            LOG_OUTPUT(L"Validate that accessing a non-strict-only property getter succeeds when the element is strict.");
            VERIFY_NO_THROW(element->RenderTransform);

            LOG_OUTPUT(L"Validate that accessing a non-strict-only property setter throws when the element is strict.");
            VERIFY_THROWS_SPECIFIC_CX(element->RenderTransform = ref new TranslateTransform(), Platform::Exception, [](Platform::Exception^ ex) {return ex->HResult == E_ACCESSDENIED; });

            LOG_OUTPUT(L"Validate that attempting to set property via SetValue API throws when the element is strict.");
            VERIFY_THROWS_SPECIFIC_CX(element->SetValue(UIElement::RenderTransformProperty, nullptr), Platform::Exception, [](Platform::Exception^ ex) {return ex->HResult == E_ACCESSDENIED; });

            LOG_OUTPUT(L"Validate that attempting to start a Storyboard targeting non-strict property throws when the element is strict.");
            auto pa = ref new PointAnimation();
            pa->From = wf::Point(0, 0);
            pa->To = wf::Point(1, 1);
            ::Windows::Foundation::TimeSpan span;
            span.Duration = 10000000000L;
            pa->Duration = DurationHelper::FromTimeSpan(span);
            Storyboard::SetTarget(pa, element);
            Storyboard::SetTargetProperty(pa, L"RenderTransformOrigin");
            auto sb = ref new Storyboard();
            sb->Children->Append(pa);
            VERIFY_THROWS_SPECIFIC_CX(sb->Begin(), Platform::Exception, [](Platform::Exception^ ex) {return ex->HResult == E_ACCESSDENIED; });

            LOG_OUTPUT(L"Set property back to default, allowing a transition away from strict");
            element->Rotation = 0;

            LOG_OUTPUT(L"Validate that setting a non-strict-only property setter auto-demotes the object back to non-strict-only and does not fail");
            VERIFY_NO_THROW(element->RenderTransform = ref new TranslateTransform());

            LOG_OUTPUT(L"Validate that accessing a strict-agnostic API does not throw when the element is non-strict-only.");
            VERIFY_NO_THROW(element->Interactions);

            LOG_OUTPUT(L"Validate that accessing a non-strict-only API does not throw when the element is non-strict-only.");
            VERIFY_NO_THROW(element->RenderTransform = ref new ScaleTransform());

            LOG_OUTPUT(L"Validate that accessing a strict-only property getter succeeds when the element is non-strict-only.");
            VERIFY_NO_THROW(element->Translation);

            LOG_OUTPUT(L"Validate that accessing a strict-only property setter throws when the element is non-strict-only.");
            VERIFY_THROWS_SPECIFIC_CX(element->Rotation = 45, Platform::Exception, [](Platform::Exception^ ex) {return ex->HResult == E_ACCESSDENIED; });

            LOG_OUTPUT(L"Set property back to default, allowing a transition back to strict");
            element->ClearValue(UIElement::RenderTransformProperty);

            VERIFY_NO_THROW(element->Rotation = 45);
        });

        CustomFrameworkElement^ element2;
        Storyboard^ sb2;

        RunOnUIThread([&]()
        {
            element2 = ref new CustomFrameworkElement();

            LOG_OUTPUT(L"Validate that accessing a strict-only property setter throws when the element is animating a non-strict-only property via Storyboard");
            auto pa2 = ref new PointAnimation();
            pa2->EnableDependentAnimation = true;
            pa2->From = wf::Point(0, 0);
            pa2->To = wf::Point(1, 1);
            ::Windows::Foundation::TimeSpan span2;
            span2.Duration = 10000000000L;
            pa2->Duration = DurationHelper::FromTimeSpan(span2);
            Storyboard::SetTarget(pa2, element2);
            Storyboard::SetTargetProperty(pa2, L"RenderTransformOrigin");
            sb2 = ref new Storyboard();
            sb2->Children->Append(pa2);
            sb2->Begin();

            TestServices::WindowHelper->WindowContent = element2;
        });
        TestServices::WindowHelper->SynchronouslyTickUIThread(3);

        RunOnUIThread([&]()
        {
            VERIFY_THROWS_SPECIFIC_CX(element2->Rotation = 45, Platform::Exception, [](Platform::Exception^ ex) {return ex->HResult == E_ACCESSDENIED; });
            sb2->Stop();
        });

        CustomFrameworkElement^ element3;
        Vector3KeyFrameAnimation^ animation;

        RunOnUIThread([&]()
        {
            element3 = ref new CustomFrameworkElement();

            LOG_OUTPUT(L"Validate that accessing a non-strict-only property setter throws when the element is animating a strict-only property via facade animation");
            auto compositor = CompositionTarget::GetCompositorForCurrentThread();
            animation = compositor->CreateVector3KeyFrameAnimation();
            animation->InsertKeyFrame(1.0, wfn_::float3(2.0f, 3.0f, 1.0f));
            ::Windows::Foundation::TimeSpan span = {10000000000L};    // 1000 seconds
            animation->Duration = span;
            animation->Target = "Scale";
            animation->StopBehavior = AnimationStopBehavior::SetToFinalValue;

            element3->StartAnimation(animation);

            TestServices::WindowHelper->WindowContent = element3;
        });
        TestServices::WindowHelper->SynchronouslyTickUIThread(3);

        RunOnUIThread([&]()
        {
            VERIFY_THROWS_SPECIFIC_CX(element3->RenderTransform = ref new TranslateTransform(), Platform::Exception, [](Platform::Exception^ ex) {return ex->HResult == E_ACCESSDENIED; });
            element3->StopAnimation(animation);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void StrictModeTests::ValidateStrictType()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            auto element = ref new CustomFrameworkElement();

            LOG_OUTPUT(L"Put the element into strict mode.");
            element->Rotation = 45;

            LOG_OUTPUT(L"Validate that accessing a default API does not throw for a strict element");
            VERIFY_NO_THROW(element->Opacity);

            LOG_OUTPUT(L"Validate that VisualTreeHelper.GetChildrenCount() succeeds with a strict element.");
            VERIFY_SUCCEEDED(xaml_media::VisualTreeHelper::GetChildrenCount(element));

            LOG_OUTPUT(L"Validate that VisualTreeHelper.GetChild() succeeds with a strict element.");
            xaml_media::VisualTreeHelper::GetChild(element, 0);
        });

        RunOnUIThread([]()
        {
            LOG_OUTPUT(L"Do validation with an element that is strict by default.");
            auto element = ref new CustomFrameworkElementEx();

            LOG_OUTPUT(L"Validate that accessing a strict-agnostic API does not throw for an element of strict-type.");
            VERIFY_NO_THROW(element->Interactions);

            LOG_OUTPUT(L"Validate that accessing a strict-only API does not throw for an element of strict-type.");
            VERIFY_NO_THROW(element->Scale);

            LOG_OUTPUT(L"Validate that accessing a default property getter succeeds for an element of strict-type.");
            element->Opacity;

            LOG_OUTPUT(L"Validate that accessing a default property setter throws for an element of strict-type.");
            VERIFY_THROWS_SPECIFIC_CX(element->Opacity = 1, Platform::Exception, [](Platform::Exception^ ex) {return ex->HResult == E_ACCESSDENIED; });

            LOG_OUTPUT(L"Validate that accessing a non-strict-only property getter succeeds for an element of strict-type.");
            element->RenderTransform;

            LOG_OUTPUT(L"Validate that accessing a non-strict-only property setter throws for an element of strict-type.");
            VERIFY_THROWS_SPECIFIC_CX(element->RenderTransform = ref new TranslateTransform(), Platform::Exception, [](Platform::Exception^ ex) {return ex->HResult == E_ACCESSDENIED; });

            LOG_OUTPUT(L"Validate that attempting to start a Storyboard targeting non-strict property throws for an element of strict-type.");
            auto pa = ref new PointAnimation();
            pa->From = wf::Point(0, 0);
            pa->To = wf::Point(1, 1);
            ::Windows::Foundation::TimeSpan span;
            span.Duration = 20000000L;
            pa->Duration = DurationHelper::FromTimeSpan(span);
            Storyboard::SetTarget(pa, element);
            Storyboard::SetTargetProperty(pa, L"RenderTransformOrigin");
            auto sb = ref new Storyboard();
            sb->Children->Append(pa);
            VERIFY_THROWS_SPECIFIC_CX(sb->Begin(), Platform::Exception, [](Platform::Exception^ ex) {return ex->HResult == E_ACCESSDENIED; });
        });
    }

    void StrictModeTests::ValidateVisualTreeHelperDisallowedInStrictMode()
    {
        VELOCITY_TESTGUARD_XAML2018

        TestCleanupWrapper cleanup;

        RunOnUIThread([]()
        {
            auto element = ref new CustomFrameworkElement();

            LOG_OUTPUT(L"Put the element into strict mode.");
            element->Rotation = 45;

            LOG_OUTPUT(L"Validate that VisualTreeHelper.GetChildrenCount() succeeds with a strict element.");
            VERIFY_SUCCEEDED(xaml_media::VisualTreeHelper::GetChildrenCount(element));

            LOG_OUTPUT(L"Validate that VisualTreeHelper.GetChild() succeeds with a strict element.");
            xaml_media::VisualTreeHelper::GetChild(element, 0);
        });

        RunOnUIThread([]()
        {
            LOG_OUTPUT(L"Do validation with an element that is strict by default.");
            auto element = ref new CustomFrameworkElementEx();

            LOG_OUTPUT(L"Validate that VisualTreeHelper.GetChildrenCount() fails with a strict element.");
            VERIFY_THROWS_SPECIFIC_CX(xaml_media::VisualTreeHelper::GetChildrenCount(element), Platform::Exception, [](Platform::Exception^ ex) {return ex->HResult == E_ACCESSDENIED; });

            LOG_OUTPUT(L"Validate that VisualTreeHelper.GetChild() fails with a strict element.");
            VERIFY_THROWS_SPECIFIC_CX(xaml_media::VisualTreeHelper::GetChild(element, 0), Platform::Exception, [](Platform::Exception^ ex) {return ex->HResult == E_ACCESSDENIED; });
        });
    }
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)

} } } } } } // Microsoft::UI::Xaml::Tests::Framework::Layering
