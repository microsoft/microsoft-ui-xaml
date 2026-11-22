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

    Platform::String^ TranslateXProperty() { return L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateX"; }
    Platform::String^ TranslateYProperty() { return L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY"; }
    Platform::String^ ScaleYProperty() { return L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleY"; }
    Platform::String^ CenterYProperty() { return L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).CenterY"; }
    Platform::String^ ClipScaleYProperty() { return L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).(CompositeTransform.ScaleY)"; }
    Platform::String^ ClipTranslateYProperty() { return L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).(CompositeTransform.TranslateY)"; }
    Platform::String^ OpacityProperty() { return L"(UIElement.TransitionTarget).Opacity"; }

    StoryboardVector^ CreateTransitionStoryboardsWithParent(
        xaml_animation::Transition^ transition,
        xaml::UIElement^ target,
        xaml::TransitionTrigger trigger,
        wf::Rect start,
        wf::Rect destination,
        xaml::TransitionParent& parent)
    {
        auto storyboards = ref new StoryboardVector();
        safe_cast<xaml_animation::ITransitionPrivate^>(transition)->CreateStoryboard(
            target, start, destination, trigger, storyboards, &parent);
        return storyboards;
    }

    // Creates a framework transition (including ones that are only reachable through markup) from XAML.
    xaml_animation::Transition^ LoadTransition(Platform::String^ markup)
    {
        return safe_cast<xaml_animation::Transition^>(xaml_markup::XamlReader::Load(markup));
    }

    std::vector<double> GetKeyFrameValues(StoryboardVector^ storyboards, Platform::String^ targetProperty);

    void VerifyKeyFrameValues(StoryboardVector^ storyboards, Platform::String^ targetProperty, std::initializer_list<double> expected)
    {
        const std::vector<double> actual = GetKeyFrameValues(storyboards, targetProperty);
        LOG_OUTPUT(L"%s: %u key frames (expected %u)", targetProperty->Data(), static_cast<unsigned int>(actual.size()), static_cast<unsigned int>(expected.size()));
        VERIFY_ARE_EQUAL(expected.size(), actual.size());

        size_t i = 0;
        for (double value : expected)
        {
            if (i < actual.size())
            {
                LOG_OUTPUT(L"  key frame %u: expected %f, actual %f", static_cast<unsigned int>(i), value, actual[i]);
                VERIFY_IS_LESS_THAN(std::abs(value - actual[i]), 0.000001);
            }
            ++i;
        }
    }

    // Returns the values of all key frames, in key frame order, of the key frame animations in the storyboards
    // that target the given property.
    std::vector<double> GetKeyFrameValues(StoryboardVector^ storyboards, Platform::String^ targetProperty)
    {
        std::vector<double> values;

        for (xaml_animation::Storyboard^ storyboard : storyboards)
        {
            for (xaml_animation::Timeline^ timeline : storyboard->Children)
            {
                auto animation = dynamic_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(timeline);
                if (animation != nullptr && xaml_animation::Storyboard::GetTargetProperty(animation) == targetProperty)
                {
                    for (unsigned int i = 0; i < animation->KeyFrames->Size; ++i)
                    {
                        values.push_back(animation->KeyFrames->GetAt(i)->Value);
                    }
                }
            }
        }

        return values;
    }

    // Verifies that the storyboards translate the target from (fromX, fromY) to (toX, toY).
    void VerifyTranslation(StoryboardVector^ storyboards, double fromX, double fromY, double toX, double toY)
    {
        const auto xValues = GetKeyFrameValues(storyboards, TranslateXProperty());
        const auto yValues = GetKeyFrameValues(storyboards, TranslateYProperty());

        VERIFY_IS_FALSE(xValues.empty());
        VERIFY_IS_FALSE(yValues.empty());

        LOG_OUTPUT(L"TranslateX: %f -> %f (expected %f -> %f), TranslateY: %f -> %f (expected %f -> %f)",
            xValues.front(), xValues.back(), fromX, toX,
            yValues.front(), yValues.back(), fromY, toY);

        VERIFY_ARE_EQUAL(fromX, xValues.front());
        VERIFY_ARE_EQUAL(toX, xValues.back());
        VERIFY_ARE_EQUAL(fromY, yValues.front());
        VERIFY_ARE_EQUAL(toY, yValues.back());
    }

    struct EdgeTranslation
    {
        xaml_primitives::EdgeTransitionLocation edge;
        double offsetX;
        double offsetY;
    };

    // Load/Unload offsets for a destination of 120x80: Top/Bottom use the height, Left/Right use the width.
    const EdgeTranslation c_edgeTranslations[] =
    {
        { xaml_primitives::EdgeTransitionLocation::Top, 0.0, -80.0 },
        { xaml_primitives::EdgeTransitionLocation::Bottom, 0.0, 80.0 },
        { xaml_primitives::EdgeTransitionLocation::Left, -120.0, 0.0 },
        { xaml_primitives::EdgeTransitionLocation::Right, 120.0, 0.0 },
    };

    Platform::String^ PaneClipTranslateXProperty() { return L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).TranslateX"; }
    Platform::String^ PaneClipTranslateYProperty() { return L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).TranslateY"; }

    // Verifies that the storyboards translate the target's clip from (fromX, fromY) to (toX, toY).
    void VerifyClipTranslation(StoryboardVector^ storyboards, double fromX, double fromY, double toX, double toY)
    {
        const auto xValues = GetKeyFrameValues(storyboards, PaneClipTranslateXProperty());
        const auto yValues = GetKeyFrameValues(storyboards, PaneClipTranslateYProperty());

        VERIFY_IS_FALSE(xValues.empty());
        VERIFY_IS_FALSE(yValues.empty());

        LOG_OUTPUT(L"Clip TranslateX: %f -> %f (expected %f -> %f), Clip TranslateY: %f -> %f (expected %f -> %f)",
            xValues.front(), xValues.back(), fromX, toX,
            yValues.front(), yValues.back(), fromY, toY);

        VERIFY_ARE_EQUAL(fromX, xValues.front());
        VERIFY_ARE_EQUAL(toX, xValues.back());
        VERIFY_ARE_EQUAL(fromY, yValues.front());
        VERIFY_ARE_EQUAL(toY, yValues.back());
    }

    void VerifyNoClipTranslation(StoryboardVector^ storyboards)
    {
        VERIFY_IS_TRUE(GetKeyFrameValues(storyboards, PaneClipTranslateXProperty()).empty());
        VERIFY_IS_TRUE(GetKeyFrameValues(storyboards, PaneClipTranslateYProperty()).empty());
    }

    // Verifies the storyboards of an Edge-based transition (EdgeUIThemeTransition or PaneThemeTransition).
    // PaneThemeTransition additionally slides a clip in the opposite direction on Load/Unload (so the pane appears to
    // slide out from behind its edge) and animates relative to the common parent.
    void VerifyEdgeBasedTransitionStoryboards(
        std::function<xaml_animation::Transition^(xaml_primitives::EdgeTransitionLocation)> createTransition,
        xaml::UIElement^ target,
        bool isPane = false)
    {
        const wf::Rect start(40, 70, 120, 80);
        const wf::Rect destination(10, 20, 120, 80);

        for (const auto& edgeTranslation : c_edgeTranslations)
        {
            LOG_OUTPUT(L"Edge = %d", static_cast<int>(edgeTranslation.edge));
            auto transition = createTransition(edgeTranslation.edge);

            // Load slides in from the edge, by the destination size, to the final position.
            xaml::TransitionParent parent = xaml::TransitionParent::ParentToRoot;
            auto load = CreateTransitionStoryboardsWithParent(transition, target, xaml::TransitionTrigger::Load, start, destination, parent);
            VERIFY_ARE_EQUAL(1u, load->Size);
            VerifyTranslation(load, edgeTranslation.offsetX, edgeTranslation.offsetY, 0.0, 0.0);
            if (isPane)
            {
                VERIFY_ARE_EQUAL(xaml::TransitionParent::ParentToCommonParent, parent);
                VerifyClipTranslation(load, -edgeTranslation.offsetX, -edgeTranslation.offsetY, 0.0, 0.0);
            }
            else
            {
                VerifyNoClipTranslation(load);
            }

            // Unload slides out from the current position to the edge.
            parent = xaml::TransitionParent::ParentToRoot;
            auto unload = CreateTransitionStoryboardsWithParent(transition, target, xaml::TransitionTrigger::Unload, start, destination, parent);
            VERIFY_ARE_EQUAL(1u, unload->Size);
            VerifyTranslation(unload, 0.0, 0.0, edgeTranslation.offsetX, edgeTranslation.offsetY);
            if (isPane)
            {
                VERIFY_ARE_EQUAL(xaml::TransitionParent::ParentToCommonParent, parent);
                VerifyClipTranslation(unload, 0.0, 0.0, -edgeTranslation.offsetX, -edgeTranslation.offsetY);
            }
            else
            {
                VerifyNoClipTranslation(unload);
            }

            // Layout moves from the old position to the new one, regardless of the edge, without a clip.
            parent = xaml::TransitionParent::ParentToRoot;
            auto layout = CreateTransitionStoryboardsWithParent(transition, target, xaml::TransitionTrigger::Layout, start, destination, parent);
            VERIFY_ARE_EQUAL(1u, layout->Size);
            VerifyTranslation(layout, start.X - destination.X, start.Y - destination.Y, 0.0, 0.0);
            VerifyNoClipTranslation(layout);
            if (isPane)
            {
                VERIFY_ARE_EQUAL(xaml::TransitionParent::ParentToCommonParent, parent);
            }

            // Reparent is not supported by these transitions.
            auto reparent = CreateTransitionStoryboards(transition, target, xaml::TransitionTrigger::Reparent, start, destination);
            VERIFY_ARE_EQUAL(0u, reparent->Size);
        }
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

void ThemeTransitionTests::ValidateStaggeredEntranceVisibilityTiming()
{
    TestCleanupWrapper cleanup;
    RuntimeEnabledFeatureScopeGuard<RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableGlobalAnimations> enableAnimations;
    WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
    const auto& wh = TestServices::WindowHelper;
    wh->SetTimeManagerClockOverrideConstant(0);

    xaml_controls::StackPanel^ panel = nullptr;
    auto transitionTargets = ref new Platform::Collections::Vector<xaml::UIElement^>();
    auto startedStoryboards = ref new StoryboardVector();
    auto storyboardMonitor = ref new StoryboardMonitorWrapper();
    storyboardMonitor->AttachStartedHandler([&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
    {
        unsigned int childIndex;
        if (panel == nullptr || !panel->Children->IndexOf(target, &childIndex))
        {
            return;
        }

        startedStoryboards->Append(storyboard);
    });

    for (bool staggeringEnabled : { false, true })
    {
        for (int reload = 0; reload < 2; ++reload)
        {
            RunOnUIThread([&]()
            {
                startedStoryboards->Clear();
                panel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                    L"    <StackPanel.ChildrenTransitions>"
                    L"        <EntranceThemeTransition FromVerticalOffset='100' />"
                    L"    </StackPanel.ChildrenTransitions>"
                    L"    <Rectangle Width='100' Height='50' Fill='Red' />"
                    L"    <Rectangle Width='100' Height='50' Fill='Green' />"
                    L"    <Rectangle Width='100' Height='50' Fill='Blue' />"
                    L"</StackPanel>"));
                safe_cast<xaml_animation::EntranceThemeTransition^>(panel->ChildrenTransitions->GetAt(0))->IsStaggeringEnabled = staggeringEnabled;
                transitionTargets->Clear();

                // Deferred storyboard creation uses TryGetPeer, so keep the children's managed peers alive.
                for (auto child : panel->Children)
                {
                    transitionTargets->Append(child);
                }

                wh->WindowContent = panel;
            });

            wh->SynchronouslyTickUIThread(2);

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(3u, startedStoryboards->Size);
                unsigned int delayedStoryboards = 0;
                for (auto storyboard : startedStoryboards)
                {
                    VERIFY_IS_NOT_NULL(storyboard->BeginTime);
                    if (storyboard->BeginTime->Value.Duration > 0)
                    {
                        ++delayedStoryboards;

                        auto visibilityAnimation = safe_cast<xaml_animation::DoubleAnimationUsingKeyFrames^>(
                            storyboard->Children->GetAt(storyboard->Children->Size - 1));
                        VERIFY_ARE_EQUAL(1u, visibilityAnimation->KeyFrames->Size);
                        VERIFY_IS_NOT_NULL(dynamic_cast<xaml_animation::DiscreteDoubleKeyFrame^>(visibilityAnimation->KeyFrames->GetAt(0)));
                        VERIFY_ARE_EQUAL(0LL, visibilityAnimation->KeyFrames->GetAt(0)->KeyTime.TimeSpan.Duration);
                        VERIFY_ARE_EQUAL(xaml::DurationType::TimeSpan, visibilityAnimation->Duration.Type);
                        VERIFY_IS_GREATER_THAN(visibilityAnimation->Duration.TimeSpan.Duration, 0LL);

                        auto storyboards = ref new StoryboardVector();
                        storyboards->Append(storyboard);
                        TransitionAnimationTiming timing;
                        VERIFY_IS_TRUE(TryGetTransitionAnimationTiming(
                            storyboards,
                            L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY",
                            100.0,
                            timing));
                        const auto transitionDuration = timing.latestEnd - storyboard->BeginTime->Value.Duration;
                        const auto visibilityDuration = visibilityAnimation->Duration.TimeSpan.Duration;

                        // Natural durations are stored as floats; allow one 100-nanosecond tick of rounding.
                        VERIFY_IS_TRUE(visibilityDuration >= transitionDuration - 1 && visibilityDuration <= transitionDuration + 1);
                    }
                }

                VERIFY_ARE_EQUAL(staggeringEnabled ? 3u : 0u, delayedStoryboards);
            });

            if (staggeringEnabled && reload == 1)
            {
                TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
            }
        }
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

void ThemeTransitionTests::ValidateEdgeUIThemeTransitionStoryboards()
{
    TestCleanupWrapper cleanup;
    xaml_controls::Border^ target = nullptr;

    RunOnUIThread([&]()
    {
        target = ref new xaml_controls::Border();
        target->Width = 120;
        target->Height = 80;

        auto root = ref new xaml_controls::StackPanel();
        root->Children->Append(target);
        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VerifyEdgeBasedTransitionStoryboards(
            [](xaml_primitives::EdgeTransitionLocation edge) -> xaml_animation::Transition^
            {
                auto transition = ref new xaml_animation::EdgeUIThemeTransition();
                transition->Edge = edge;
                VERIFY_ARE_EQUAL(edge, transition->Edge);
                return transition;
            },
            target);
    });
}

void ThemeTransitionTests::ValidatePaneThemeTransitionStoryboards()
{
    TestCleanupWrapper cleanup;
    xaml_controls::Border^ target = nullptr;

    RunOnUIThread([&]()
    {
        target = ref new xaml_controls::Border();
        target->Width = 120;
        target->Height = 80;

        auto root = ref new xaml_controls::StackPanel();
        root->Children->Append(target);
        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VerifyEdgeBasedTransitionStoryboards(
            [](xaml_primitives::EdgeTransitionLocation edge) -> xaml_animation::Transition^
            {
                auto transition = ref new xaml_animation::PaneThemeTransition();
                transition->Edge = edge;
                VERIFY_ARE_EQUAL(edge, transition->Edge);
                return transition;
            },
            target,
            true /* isPane */);
    });
}

void ThemeTransitionTests::ValidatePopupThemeTransitionStoryboards()
{
    TestCleanupWrapper cleanup;
    xaml_controls::Border^ target = nullptr;

    RunOnUIThread([&]()
    {
        target = ref new xaml_controls::Border();
        target->Width = 120;
        target->Height = 80;

        auto root = ref new xaml_controls::StackPanel();
        root->Children->Append(target);
        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        const wf::Rect start(40, 70, 120, 80);
        const wf::Rect destination(10, 20, 120, 80);

        auto transition = ref new xaml_animation::PopupThemeTransition();
        transition->FromHorizontalOffset = 30;
        transition->FromVerticalOffset = -45;

        // Load slides in from the "From" offsets to the final position.
        auto load = CreateTransitionStoryboards(transition, target, xaml::TransitionTrigger::Load, start, destination);
        VERIFY_ARE_EQUAL(1u, load->Size);
        VerifyTranslation(load, 30.0, -45.0, 0.0, 0.0);

        // Unload hides the popup in place: the "From" offsets are not used.
        auto unload = CreateTransitionStoryboards(transition, target, xaml::TransitionTrigger::Unload, start, destination);
        VERIFY_ARE_EQUAL(1u, unload->Size);
        VERIFY_IS_GREATER_THAN(unload->GetAt(0)->Children->Size, 0u);
        for (double value : GetKeyFrameValues(unload, TranslateXProperty()))
        {
            VERIFY_ARE_EQUAL(0.0, value);
        }
        for (double value : GetKeyFrameValues(unload, TranslateYProperty()))
        {
            VERIFY_ARE_EQUAL(0.0, value);
        }

        // Layout and Reparent are not supported by PopupThemeTransition.
        VERIFY_ARE_EQUAL(0u, CreateTransitionStoryboards(transition, target, xaml::TransitionTrigger::Layout, start, destination)->Size);
        VERIFY_ARE_EQUAL(0u, CreateTransitionStoryboards(transition, target, xaml::TransitionTrigger::Reparent, start, destination)->Size);
    });
}

void ThemeTransitionTests::ValidateMenuPopupThemeTransitionStoryboards()
{
    TestCleanupWrapper cleanup;
    xaml_controls::MenuFlyoutPresenter^ presenter = nullptr;
    xaml_controls::Border^ plainTarget = nullptr;

    RunOnUIThread([&]()
    {
        // An app-supplied MenuFlyoutPresenter template with a "MenuFlyoutPresenterBorder" part, whose scale is animated
        // together with the presenter when the menu opens.
        presenter = safe_cast<xaml_controls::MenuFlyoutPresenter^>(xaml_markup::XamlReader::Load(
            L"<MenuFlyoutPresenter xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Width='100' Height='120'>"
            L"  <MenuFlyoutPresenter.Template>"
            L"    <ControlTemplate TargetType='MenuFlyoutPresenter'>"
            L"      <Border x:Name='MenuFlyoutPresenterBorder' Background='Red'/>"
            L"    </ControlTemplate>"
            L"  </MenuFlyoutPresenter.Template>"
            L"</MenuFlyoutPresenter>"));

        plainTarget = ref new xaml_controls::Border();
        plainTarget->Width = 100;
        plainTarget->Height = 120;

        auto root = ref new xaml_controls::StackPanel();
        root->Children->Append(presenter);
        root->Children->Append(plainTarget);
        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        const wf::Rect bounds(0, 0, 100, 120);

        // Direction=Top: the presenter slides down from above (TranslateY -OpenedLength*ClosedRatio) while its clip
        // moves the opposite way, and the border part scales up from (1 - ClosedRatio) around its bottom (CenterY = OpenedLength).
        auto top = LoadTransition(L"<MenuPopupThemeTransition xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' OpenedLength='120' Direction='Top' ClosedRatio='0.25'/>");
        auto topLoad = CreateTransitionStoryboards(top, presenter, xaml::TransitionTrigger::Load, bounds, bounds);
        VERIFY_ARE_EQUAL(1u, topLoad->Size);
        VerifyKeyFrameValues(topLoad, ClipTranslateYProperty(), { 30.0, 0.0 });
        VerifyKeyFrameValues(topLoad, TranslateYProperty(), { -30.0, 0.0 });
        VerifyKeyFrameValues(topLoad, ScaleYProperty(), { 0.75, 1.0 });
        VerifyKeyFrameValues(topLoad, CenterYProperty(), { 120.0 });

        // Direction=Bottom: the presenter slides up from below; the border scales around its top (no CenterY).
        auto bottom = LoadTransition(L"<MenuPopupThemeTransition xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' OpenedLength='120' Direction='Bottom' ClosedRatio='0.25'/>");
        auto bottomLoad = CreateTransitionStoryboards(bottom, presenter, xaml::TransitionTrigger::Load, bounds, bounds);
        VERIFY_ARE_EQUAL(1u, bottomLoad->Size);
        VerifyKeyFrameValues(bottomLoad, ClipTranslateYProperty(), { -30.0, 0.0 });
        VerifyKeyFrameValues(bottomLoad, TranslateYProperty(), { 30.0, 0.0 });
        VerifyKeyFrameValues(bottomLoad, ScaleYProperty(), { 0.75, 1.0 });
        VerifyKeyFrameValues(bottomLoad, CenterYProperty(), {});

        // A target that isn't a MenuFlyoutPresenter (and isn't hosted in one) gets no border animation.
        auto plainLoad = CreateTransitionStoryboards(top, plainTarget, xaml::TransitionTrigger::Load, bounds, bounds);
        VERIFY_ARE_EQUAL(1u, plainLoad->Size);
        VerifyKeyFrameValues(plainLoad, TranslateYProperty(), { -30.0, 0.0 });
        VerifyKeyFrameValues(plainLoad, ScaleYProperty(), {});
        VerifyKeyFrameValues(plainLoad, CenterYProperty(), {});

        // Unload fades the presenter out without moving it.
        auto unload = CreateTransitionStoryboards(top, presenter, xaml::TransitionTrigger::Unload, bounds, bounds);
        VERIFY_ARE_EQUAL(1u, unload->Size);
        VerifyKeyFrameValues(unload, OpacityProperty(), { 1.0, 0.0 });
        VerifyKeyFrameValues(unload, TranslateYProperty(), {});
        VerifyKeyFrameValues(unload, ScaleYProperty(), {});

        presenter = nullptr;
        plainTarget = nullptr;
    });
}

void ThemeTransitionTests::ValidatePickerFlyoutThemeTransitionStoryboards()
{
    TestCleanupWrapper cleanup;
    xaml_controls::Border^ target = nullptr;

    RunOnUIThread([&]()
    {
        target = ref new xaml_controls::Border();
        target->Width = 120;
        target->Height = 200;

        auto root = ref new xaml_controls::StackPanel();
        root->Children->Append(target);
        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        const wf::Rect bounds(0, 0, 120, 200);
        auto createTransition = [](Platform::String^ openedLength, Platform::String^ offsetFromCenter)
        {
            return LoadTransition(
                L"<PickerFlyoutThemeTransition xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' OpenedLength='" + openedLength +
                L"' OffsetFromCenter='" + offsetFromCenter + L"'/>");
        };

        // With OpenedLength = 200, the opening clip starts at ClosedRatio 0.5 (100 px) and the closing clip ends at
        // ClosedRatio 0.15 (30 px) unless the selected item is so far from the center that the clip would leave the
        // element: |offset| > 50 when opening and |offset| > 85 when closing. The clip always ends/starts at
        // (0.5 + |offset| / OpenedLength) * 2.
        struct PickerCase
        {
            const wchar_t* offset;
            double offsetValue;
            double loadInitialScale;
            double loadFinalScale;
            double unloadInitialScale;
            double unloadFinalScale;
        };
        const PickerCase cases[] =
        {
            { L"90", 90.0, 0.9, 1.9, 1.9, 0.2 },        // beyond both maximum offsets
            { L"-90", -90.0, 0.9, 1.9, 1.9, 0.2 },      // the absolute offset is used
            { L"-10", -10.0, 0.5, 1.1, 1.1, 0.15 },     // within both maximum offsets
            { L"50", 50.0, 0.5, 1.5, 1.5, 0.15 },       // exactly the opening maximum offset
            { L"85", 85.0, 0.85, 1.85, 1.85, 0.15 },    // exactly the closing maximum offset
            { L"30", 30.0, 0.5, 1.3, 1.3, 0.15 },       // within the opening maximum offset of OpenedLength * (1 - 0.5) / 2 = 50
        };

        for (const auto& pickerCase : cases)
        {
            LOG_OUTPUT(L"OffsetFromCenter = %s", pickerCase.offset);
            auto transition = createTransition(L"200", ref new Platform::String(pickerCase.offset));

            auto load = CreateTransitionStoryboards(transition, target, xaml::TransitionTrigger::Load, bounds, bounds);
            VERIFY_ARE_EQUAL(1u, load->Size);
            VerifyKeyFrameValues(load, ClipScaleYProperty(), { pickerCase.loadInitialScale, pickerCase.loadFinalScale });
            VerifyKeyFrameValues(load, ClipTranslateYProperty(), { pickerCase.offsetValue });
            VerifyKeyFrameValues(load, OpacityProperty(), { 1.0 });

            auto unload = CreateTransitionStoryboards(transition, target, xaml::TransitionTrigger::Unload, bounds, bounds);
            VERIFY_ARE_EQUAL(1u, unload->Size);
            VerifyKeyFrameValues(unload, ClipScaleYProperty(), { pickerCase.unloadInitialScale, pickerCase.unloadFinalScale });
            VerifyKeyFrameValues(unload, ClipTranslateYProperty(), { pickerCase.offsetValue });
            VerifyKeyFrameValues(unload, OpacityProperty(), { 1.0, 1.0, 0.0 });

            // Other triggers produce an empty storyboard.
            auto layout = CreateTransitionStoryboards(transition, target, xaml::TransitionTrigger::Layout, bounds, bounds);
            VERIFY_ARE_EQUAL(1u, layout->Size);
            VERIFY_ARE_EQUAL(0u, layout->GetAt(0)->Children->Size);
        }

        // Without an opened length there is nothing to animate.
        auto empty = createTransition(L"0", L"10");
        VERIFY_ARE_EQUAL(0u, CreateTransitionStoryboards(empty, target, xaml::TransitionTrigger::Load, bounds, bounds)->Size);
        VERIFY_ARE_EQUAL(0u, CreateTransitionStoryboards(empty, target, xaml::TransitionTrigger::Unload, bounds, bounds)->Size);

        target = nullptr;
    });
}

void ThemeTransitionTests::ValidateInputPaneThemeTransitionStoryboards()
{
    TestCleanupWrapper cleanup;
    xaml_controls::Border^ target = nullptr;

    RunOnUIThread([&]()
    {
        target = ref new xaml_controls::Border();
        target->Width = 120;
        target->Height = 80;

        auto root = ref new xaml_controls::StackPanel();
        root->Children->Append(target);
        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        const wf::Rect start(40, 70, 120, 80);
        const wf::Rect destination(10, 20, 120, 80);

        auto transition = LoadTransition(L"<InputPaneThemeTransition xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'/>");

        // Without a showing input pane, the element moves (hide-panel animation) from its old position to its new one,
        // relative to the root.
        xaml::TransitionParent parent = xaml::TransitionParent::ParentToCommonParent;
        auto layout = CreateTransitionStoryboardsWithParent(transition, target, xaml::TransitionTrigger::Layout, start, destination, parent);
        VERIFY_ARE_EQUAL(1u, layout->Size);
        VERIFY_ARE_EQUAL(xaml::TransitionParent::ParentToRoot, parent);
        VerifyTranslation(layout, start.X - destination.X, start.Y - destination.Y, 0.0, 0.0);

        target = nullptr;
    });
}

void ThemeTransitionTests::ValidateAddDeleteThemeTransitionOutsideItemsControl()
{
    TestCleanupWrapper cleanup;
    xaml_controls::Border^ target = nullptr;

    RunOnUIThread([&]()
    {
        target = ref new xaml_controls::Border();
        target->Width = 120;
        target->Height = 40;

        auto root = ref new xaml_controls::StackPanel();
        root->Children->Append(target);
        TestServices::WindowHelper->WindowContent = root;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        const wf::Rect start(0, 40, 120, 40);
        const wf::Rect destination(0, 0, 120, 40);
        auto addDelete = ref new xaml_animation::AddDeleteThemeTransition();

        // Outside of an ItemsControl there is no collection context: deleted elements fade out only after the
        // affected elements had time to move (300 ms).
        xaml::TransitionParent parent = xaml::TransitionParent::ParentToRoot;
        auto unload = CreateTransitionStoryboardsWithParent(addDelete, target, xaml::TransitionTrigger::Unload, destination, destination, parent);
        VERIFY_ARE_EQUAL(1u, unload->Size);
        VERIFY_ARE_EQUAL(xaml::TransitionParent::ParentToCommonParent, parent);
        TransitionAnimationTiming timing;
        VERIFY_IS_TRUE(TryGetTransitionAnimationTiming(unload, OpacityProperty(), 1.0, timing));
        LOG_OUTPUT(L"Delete fade starts at %lld (100ns units)", timing.earliestStart);
        VERIFY_IS_GREATER_THAN_OR_EQUAL(timing.earliestStart, 300 * 10000LL);

        // Reparenting is animated relative to the root, layout changes relative to the common parent.
        parent = xaml::TransitionParent::ParentToCommonParent;
        auto reparent = CreateTransitionStoryboardsWithParent(addDelete, target, xaml::TransitionTrigger::Reparent, start, destination, parent);
        VERIFY_ARE_EQUAL(xaml::TransitionParent::ParentToRoot, parent);
        VERIFY_IS_GREATER_THAN_OR_EQUAL(reparent->Size, 1u);

        parent = xaml::TransitionParent::ParentToRoot;
        auto layout = CreateTransitionStoryboardsWithParent(addDelete, target, xaml::TransitionTrigger::Layout, start, destination, parent);
        VERIFY_ARE_EQUAL(xaml::TransitionParent::ParentToCommonParent, parent);
        VerifyTranslation(layout, start.X - destination.X, start.Y - destination.Y, 0.0, 0.0);

        target = nullptr;
    });
}
} } } } } }
