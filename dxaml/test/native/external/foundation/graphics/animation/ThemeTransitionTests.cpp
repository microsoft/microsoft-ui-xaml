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
#include <Collection.h>
#include <algorithm>
#include <initializer_list>
#include <limits>
#include <utility>

#include <CustomTypeMetadataProvider.h>
#include <NavigationThemeTransitionTestPage.xaml.h>
#include <WUCRenderingScopeGuard.h>

using namespace test_infra;
using namespace Private::Foundation::CustomTypes;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

namespace
{
    using StoryboardVector = Platform::Collections::Vector<xaml_animation::Storyboard^>;

    enum class ItemsControlTransitionBatch
    {
        Remove,
        RemoveAndAppend,
        Add
    };

    StoryboardVector^ CreateTransitionStoryboards(
        xaml_animation::Transition^ transition,
        xaml::UIElement^ target,
        xaml::TransitionTrigger trigger,
        wf::Rect start,
        wf::Rect destination)
    {
        auto storyboards = ref new StoryboardVector();
        xaml::TransitionParent parent;
        safe_cast<xaml_animation::ITransitionPrivate^>(transition)->CreateStoryboard(
            target, start, destination, trigger, storyboards, &parent);
        return storyboards;
    }

    struct TransitionAnimationTiming
    {
        long long earliestStart = (std::numeric_limits<long long>::max)();
        long long latestStart = 0;
        long long latestEnd = 0;
    };

    bool TryGetTransitionAnimationTiming(
        StoryboardVector^ storyboards,
        Platform::String^ targetProperty,
        double initialValue,
        TransitionAnimationTiming& timing)
    {
        unsigned int matchingAnimations = 0;

        for (xaml_animation::Storyboard^ storyboard : storyboards)
        {
            const auto storyboardBegin = storyboard->BeginTime ? storyboard->BeginTime->Value.Duration : 0LL;

            for (xaml_animation::Timeline^ timeline : storyboard->Children)
            {
                auto animation = dynamic_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(timeline);
                if (animation == nullptr || xaml_animation::Storyboard::GetTargetProperty(animation) != targetProperty)
                {
                    continue;
                }

                const auto begin = storyboardBegin + (animation->BeginTime ? animation->BeginTime->Value.Duration : 0LL);
                double previousValue = initialValue;
                long long previousTime = 0;

                // Time-zero keyframes establish the initial value. Otherwise, even a single
                // keyframe can interpolate from the base value supplied by the caller.
                for (unsigned int i = 0; i < animation->KeyFrames->Size; ++i)
                {
                    auto current = animation->KeyFrames->GetAt(i);
                    const auto currentTime = current->KeyTime.TimeSpan.Duration;
                    if (currentTime > 0 && previousValue != current->Value)
                    {
                        auto firstChange = dynamic_cast<xaml_animation::DiscreteDoubleKeyFrame^>(current)
                            ? currentTime
                            : previousTime;
                        timing.earliestStart = (std::min)(timing.earliestStart, begin + firstChange);
                        timing.latestStart = (std::max)(timing.latestStart, begin + firstChange);
                        timing.latestEnd = (std::max)(timing.latestEnd, begin + animation->KeyFrames->GetAt(animation->KeyFrames->Size - 1)->KeyTime.TimeSpan.Duration);
                        ++matchingAnimations;
                        break;
                    }

                    previousValue = current->Value;
                    previousTime = currentTime;
                }
            }
        }

        return matchingAnimations != 0;
    }

    long long VerifyTransitionAnimationStartsAt(
        StoryboardVector^ storyboards,
        Platform::String^ targetProperty,
        long long expectedStartMilliseconds)
    {
        TransitionAnimationTiming timing;
        VERIFY_IS_TRUE(TryGetTransitionAnimationTiming(storyboards, targetProperty, 0.0, timing));
        VERIFY_ARE_EQUAL(expectedStartMilliseconds * 10000LL, timing.earliestStart);
        VERIFY_ARE_EQUAL(expectedStartMilliseconds * 10000LL, timing.latestStart);
        return timing.latestEnd;
    }

    void VerifyItemsControlTransitionStoryboards(
        ItemsControlTransitionBatch batch,
        bool hasAddDelete = true,
        bool useLocalTransitions = false,
        bool staggeringEnabled = false)
    {
        RuntimeEnabledFeatureScopeGuard<RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableGlobalAnimations> enableAnimations;
        TestCleanupWrapper cleanup;
        xaml_controls::ItemsControl^ itemsControl = nullptr;
        Platform::Collections::Vector<Platform::String^>^ items = nullptr;
        xaml_controls::ContentPresenter^ target = nullptr;
        xaml_animation::AddDeleteThemeTransition^ addDelete = nullptr;
        xaml_animation::ReorderThemeTransition^ reorder = nullptr;
        xaml_animation::RepositionThemeTransition^ reposition = nullptr;
        xaml_animation::TransitionCollection^ localTransitions = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ItemsControl, Loaded);
        Platform::String^ opacityProperty = L"(UIElement.TransitionTarget).Opacity";
        Platform::String^ translationProperty = L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY";
        Platform::String^ addedItem = L"New item";
        xaml_controls::ContentPresenter^ removedContainer = nullptr;
        xaml_controls::ContentPresenter^ lastSurvivor = nullptr;
        bool sawRemoval = false;
        bool sawFirstSurvivor = false;
        bool sawLastSurvivor = false;
        bool sawAddition = false;
        bool checkedRepositionSuppression = false;
        long long removalStart = (std::numeric_limits<long long>::max)();
        long long removalEnd = 0;
        long long firstMovementStart = (std::numeric_limits<long long>::max)();
        long long lastMovementEnd = 0;
        long long additionStart = (std::numeric_limits<long long>::max)();
        long long initialAdditionStart = 0;
        auto storyboardMonitor = ref new StoryboardMonitorWrapper();
        auto detachMonitor = wil::scope_exit([&]() { storyboardMonitor->DetachStartedHandler(); });

        RunOnUIThread([&]()
        {
            itemsControl = safe_cast<xaml_controls::ItemsControl^>(xaml_markup::XamlReader::Load(
                LR"(<ItemsControl xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" Width="200" Height="240">
                        <ItemsControl.ItemTemplate>
                            <DataTemplate>
                                <TextBlock Text="{Binding}" Height="40"/>
                            </DataTemplate>
                        </ItemsControl.ItemTemplate>
                    </ItemsControl>)"));

            addDelete = ref new xaml_animation::AddDeleteThemeTransition();
            reorder = ref new xaml_animation::ReorderThemeTransition();
            reposition = ref new xaml_animation::RepositionThemeTransition();
            reposition->IsStaggeringEnabled = staggeringEnabled;
            auto transitions = ref new xaml_animation::TransitionCollection();
            if (hasAddDelete)
            {
                if (!useLocalTransitions)
                {
                    transitions->Append(addDelete);
                }
                transitions->Append(reorder);
            }
            transitions->Append(reposition);
            itemsControl->ItemContainerTransitions = transitions;

            if (useLocalTransitions)
            {
                localTransitions = ref new xaml_animation::TransitionCollection();
                localTransitions->Append(addDelete);
                localTransitions->Append(reorder);
                localTransitions->Append(reposition);
                auto containerStyle = ref new xaml::Style();
                containerStyle->TargetType = wxaml_interop::TypeName(xaml_controls::ContentPresenter::typeid);
                containerStyle->Setters->Append(ref new xaml::Setter(xaml::UIElement::TransitionsProperty, localTransitions));
                itemsControl->ItemContainerStyle = containerStyle;
            }

            items = ref new Platform::Collections::Vector<Platform::String^>();
            for (int i = 0; i < 4; ++i)
            {
                auto item = ref new Platform::String(L"Item ");
                item += i;
                items->Append(item);
            }
            itemsControl->ItemsSource = items;
            loadedRegistration.Attach(itemsControl, ref new xaml::RoutedEventHandler(
                [loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    loadedEvent->Set();
                }));
            TestServices::WindowHelper->WindowContent = itemsControl;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            target = safe_cast<xaml_controls::ContentPresenter^>(itemsControl->ContainerFromIndex(2));
            VERIFY_IS_NOT_NULL(target);
            if (hasAddDelete)
            {
                auto bounds = xaml_controls::Primitives::LayoutInformation::GetLayoutSlot(target);
                TransitionAnimationTiming timing;
                VERIFY_IS_TRUE(TryGetTransitionAnimationTiming(
                    CreateTransitionStoryboards(addDelete, target, xaml::TransitionTrigger::Load, bounds, bounds),
                    opacityProperty, 0.0, timing));
                // The platform theme contributes its own opacity delay in addition to
                // the 300 ms collection scheduling offset.
                VERIFY_IS_GREATER_THAN_OR_EQUAL(timing.earliestStart, 300 * 10000LL);
                VERIFY_ARE_EQUAL(timing.earliestStart, timing.latestStart);
                initialAdditionStart = timing.earliestStart;
            }
        });

        // Start the mutation batch on a new tick, after consuming the initial-load context.
        TestServices::WindowHelper->SynchronouslyTickUIThread(1);

        RunOnUIThread([&]()
        {
            auto start = xaml_controls::Primitives::LayoutInformation::GetLayoutSlot(target);
            const bool removesItem = batch != ItemsControlTransitionBatch::Add;
            const bool addsItem = batch != ItemsControlTransitionBatch::Remove;
            lastSurvivor = safe_cast<xaml_controls::ContentPresenter^>(itemsControl->ContainerFromIndex(3));
            if (removesItem)
            {
                removedContainer = safe_cast<xaml_controls::ContentPresenter^>(itemsControl->ContainerFromIndex(1));
            }

            storyboardMonitor->AttachStartedHandler(
                [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ animationTarget)
                {
                    auto storyboards = ref new StoryboardVector();
                    storyboards->Append(storyboard);
                    TransitionAnimationTiming timing;
                    if (animationTarget == removedContainer && removedContainer != nullptr)
                    {
                        if (TryGetTransitionAnimationTiming(storyboards, opacityProperty, 1.0, timing))
                        {
                            sawRemoval = true;
                            removalStart = (std::min)(removalStart, timing.earliestStart);
                            removalEnd = (std::max)(removalEnd, timing.latestEnd);
                        }
                    }
                    else if (animationTarget == target || animationTarget == lastSurvivor)
                    {
                        if (TryGetTransitionAnimationTiming(storyboards, translationProperty, 0.0, timing))
                        {
                            sawFirstSurvivor |= animationTarget == target;
                            sawLastSurvivor |= animationTarget == lastSurvivor;
                            firstMovementStart = (std::min)(firstMovementStart, timing.earliestStart);
                            lastMovementEnd = (std::max)(lastMovementEnd, timing.latestEnd);

                            if (!checkedRepositionSuppression && hasAddDelete && batch != ItemsControlTransitionBatch::Add)
                            {
                                // Normal storyboard creation has now latched this batch's context.
                                checkedRepositionSuppression = true;
                                const auto savedStaggeringEnabled = reposition->IsStaggeringEnabled;
                                auto restoreStaggering = wil::scope_exit([&]()
                                {
                                    reposition->IsStaggeringEnabled = savedStaggeringEnabled;
                                });
                                auto destination = xaml_controls::Primitives::LayoutInformation::GetLayoutSlot(
                                    safe_cast<xaml_controls::ContentPresenter^>(animationTarget));
                                auto start = destination;
                                start.Y += 40;
                                for (bool factoryStaggeringEnabled : { false, true })
                                {
                                    reposition->IsStaggeringEnabled = factoryStaggeringEnabled;
                                    VERIFY_ARE_EQUAL(0u, CreateTransitionStoryboards(
                                        reposition, animationTarget, xaml::TransitionTrigger::Layout, start, destination)->Size);
                                }
                            }
                        }
                    }
                    else
                    {
                        auto container = dynamic_cast<xaml_controls::ContentPresenter^>(animationTarget);
                        if (container && dynamic_cast<Platform::String^>(container->Content) == addedItem &&
                            TryGetTransitionAnimationTiming(storyboards, opacityProperty, 0.0, timing))
                        {
                            sawAddition = true;
                            additionStart = (std::min)(additionStart, timing.earliestStart);
                        }
                    }
                });

            if (removesItem)
            {
                items->RemoveAt(1);

                if (hasAddDelete)
                {
                    // Registration must not consume the removal before a same-batch addition arrives.
                    safe_cast<xaml_animation::ITransitionPrivate^>(addDelete)->ParticipatesInTransition(
                        target, xaml::TransitionTrigger::Unload);
                }
            }
            if (addsItem)
            {
                if (removesItem)
                {
                    items->Append(addedItem);
                }
                else
                {
                    items->InsertAt(0, addedItem);
                }
            }
            itemsControl->UpdateLayout();

            VERIFY_ARE_EQUAL(target, itemsControl->ContainerFromItem(target->Content));
            auto destination = xaml_controls::Primitives::LayoutInformation::GetLayoutSlot(target);
            VERIFY_ARE_NOT_EQUAL(start.Y, destination.Y);

            // Let the framework create this batch's storyboards. Calling the private factories
            // ourselves here would consume mutation context ahead of normal transition processing.
        });

        TestServices::WindowHelper->WaitForIdle();
        storyboardMonitor->DetachStartedHandler();

        VERIFY_IS_TRUE(sawFirstSurvivor);
        VERIFY_IS_TRUE(sawLastSurvivor);
        if (hasAddDelete && batch != ItemsControlTransitionBatch::Add)
        {
            VERIFY_IS_TRUE(sawRemoval);
            VERIFY_IS_TRUE(checkedRepositionSuppression);
            VERIFY_ARE_EQUAL(0LL, removalStart);
            VERIFY_IS_GREATER_THAN(removalEnd, 0LL);
            VERIFY_IS_LESS_THAN_OR_EQUAL(removalEnd, firstMovementStart);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(firstMovementStart, 220 * 10000LL);
        }
        else
        {
            VERIFY_IS_FALSE(sawRemoval);
            VERIFY_ARE_EQUAL(0LL, firstMovementStart);
        }
        if (hasAddDelete && batch != ItemsControlTransitionBatch::Remove)
        {
            VERIFY_IS_TRUE(sawAddition);
            VERIFY_IS_LESS_THAN_OR_EQUAL(lastMovementEnd, additionStart);
            if (batch == ItemsControlTransitionBatch::RemoveAndAppend)
            {
                VERIFY_ARE_EQUAL(initialAdditionStart + 300 * 10000LL, additionStart);
            }
            else
            {
                VERIFY_ARE_EQUAL(initialAdditionStart, additionStart);
            }
        }

        RunOnUIThread([&]()
        {
            // A subsequent layout-only change must not inherit the previous batch's deletion phase.
            auto start = xaml_controls::Primitives::LayoutInformation::GetLayoutSlot(target);
            auto firstContainer = safe_cast<xaml_controls::ContentPresenter^>(itemsControl->ContainerFromIndex(0));
            firstContainer->Height = firstContainer->ActualHeight + 10;
            itemsControl->UpdateLayout();
            auto destination = xaml_controls::Primitives::LayoutInformation::GetLayoutSlot(target);
            VERIFY_ARE_NOT_EQUAL(start.Y, destination.Y);

            for (bool factoryStaggeringEnabled : { false, true })
            {
                reposition->IsStaggeringEnabled = factoryStaggeringEnabled;
                VerifyTransitionAnimationStartsAt(
                    CreateTransitionStoryboards(reposition, target, xaml::TransitionTrigger::Layout, start, destination),
                    translationProperty, 0);
            }
        });
    }
}

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

void ThemeTransitionTests::ValidateItemsControlTransitionTimingHelper()
{
    RunOnUIThread([&]()
    {
        auto verifyTiming = [](
            std::initializer_list<std::pair<long long, double>> keyframes,
            double initialValue,
            bool discrete,
            bool expectAnimation,
            long long expectedStart,
            long long expectedEnd)
        {
            Platform::String^ property = L"(UIElement.TransitionTarget).Opacity";
            auto storyboard = ref new xaml_animation::Storyboard();
            auto animation = ref new xaml_animation::DoubleAnimationUsingKeyFrames();
            wf::TimeSpan storyboardBegin = { 25 * 10000LL };
            wf::TimeSpan animationBegin = { 50 * 10000LL };
            storyboard->BeginTime = storyboardBegin;
            animation->BeginTime = animationBegin;
            xaml_animation::Storyboard::SetTargetProperty(animation, property);

            for (const auto& entry : keyframes)
            {
                xaml_animation::DoubleKeyFrame^ keyframe;
                if (discrete)
                {
                    keyframe = ref new xaml_animation::DiscreteDoubleKeyFrame();
                }
                else
                {
                    keyframe = ref new xaml_animation::LinearDoubleKeyFrame();
                }
                xaml_animation::KeyTime keyTime = {};
                keyTime.TimeSpan.Duration = entry.first * 10000LL;
                keyframe->KeyTime = keyTime;
                keyframe->Value = entry.second;
                animation->KeyFrames->Append(keyframe);
            }
            storyboard->Children->Append(animation);
            auto storyboards = ref new StoryboardVector();
            storyboards->Append(storyboard);

            TransitionAnimationTiming timing;
            VERIFY_ARE_EQUAL(expectAnimation, TryGetTransitionAnimationTiming(storyboards, property, initialValue, timing));
            if (expectAnimation)
            {
                VERIFY_ARE_EQUAL((75 + expectedStart) * 10000LL, timing.earliestStart);
                VERIFY_ARE_EQUAL((75 + expectedStart) * 10000LL, timing.latestStart);
                VERIFY_ARE_EQUAL((75 + expectedEnd) * 10000LL, timing.latestEnd);
            }
        };

        verifyTiming({ {100, 0.0} }, 1.0, false, true, 0, 100);
        verifyTiming({ {100, 0.0} }, 1.0, true, true, 100, 100);
        verifyTiming({ {100, 0.0} }, 0.0, false, false, 0, 0);
        verifyTiming({ {0, 0.0}, {466, 0.0}, {799, 1.0} }, 1.0, false, true, 466, 799);
        verifyTiming({ {0, 40.0}, {220, 40.0}, {553, 0.0} }, 0.0, false, true, 220, 553);
        verifyTiming({}, 0.0, false, false, 0, 0);
    });
}

void ThemeTransitionTests::ValidateItemsControlDeleteTransitionStoryboards()
{
    for (bool staggeringEnabled : { false, true })
    {
        VerifyItemsControlTransitionStoryboards(ItemsControlTransitionBatch::Remove, true, false, staggeringEnabled);
    }
}

void ThemeTransitionTests::ValidateItemsControlMixedTransitionStoryboards()
{
    for (bool staggeringEnabled : { false, true })
    {
        VerifyItemsControlTransitionStoryboards(ItemsControlTransitionBatch::RemoveAndAppend, true, false, staggeringEnabled);
    }
}

void ThemeTransitionTests::ValidateItemsControlLocalTransitionStoryboards()
{
    VerifyItemsControlTransitionStoryboards(ItemsControlTransitionBatch::RemoveAndAppend, true, true);
}

void ThemeTransitionTests::ValidateItemsControlAddTransitionStoryboards()
{
    VerifyItemsControlTransitionStoryboards(ItemsControlTransitionBatch::Add);
}

void ThemeTransitionTests::ValidateItemsControlStandaloneRepositionStoryboards()
{
    VerifyItemsControlTransitionStoryboards(ItemsControlTransitionBatch::RemoveAndAppend, false);
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
