// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "VsmIntegrationTests.h"
#include <ppltasks.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"
#include <RuntimeEnabledFeatureOverride.h>
#include <StoryboardMonitorWrapper.h>
#include <CustomPropertySupport.h>
#include <WUCRenderingScopeGuard.h>
#include <ControlWithAttachedProperty.h>
#include "CustomTypes.XamlTypeInfo.g.h"
#include <TreeHelper.h>

using namespace Platform;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Automation;
using namespace ::Tests::Native::External::Framework;

using namespace test_infra;
using namespace MockDComp;

using Colors = Microsoft::UI::Colors;
using Color = ::Windows::UI::Color;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        static const wchar_t genericVsm1[] =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button.Template>"
            L"    <ControlTemplate TargetType='ButtonBase'>"
            L"<StackPanel>"
            L"<VisualStateManager.VisualStateGroups>"
            L"    <VisualStateGroup x:Name='Group1'>"
            L"        <VisualStateGroup.Transitions>"
            L"            <VisualTransition From='VisualState1' To='VisualState2'>"
            L"                <Storyboard x:Name='VisualTransitionStoryboard'>"
            L"                    <FadeOutThemeAnimation TargetName='Rect1' />"
            L"                </Storyboard>"
            L"            </VisualTransition>"
            L"            <VisualTransition From='VisualState3'>"
            L"            </VisualTransition>"
            L"            <VisualTransition To='VisualState3'>"
            L"            </VisualTransition>"
            L"            <VisualTransition>"
            L"            </VisualTransition>"
            L"        </VisualStateGroup.Transitions>"
            L"        <VisualState x:Name='VisualState1' />"
            L"        <VisualState x:Name='VisualState2'>"
            L"            <Storyboard x:Name='VisualState2Storyboard'>"
            L"                <FadeOutThemeAnimation TargetName='Rect1' />"
            L"            </Storyboard>"
            L"        </VisualState>"
            L"        <VisualState x:Name='VisualState3'>"
            L"        </VisualState>"
            L"        <VisualState x:Name='VisualState4'>"
            L"            <Storyboard>"
            L"                 <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Rect1'"
            L"                     Storyboard.TargetProperty='Width'>"
            L"                     <DiscreteObjectKeyFrame KeyTime='0' Value='{Binding RelativeSource={RelativeSource TemplatedParent}, Path=MinWidth}' />"
            L"                 </ObjectAnimationUsingKeyFrames>"
            L"            </Storyboard>"
            L"        </VisualState>"
            L"    </VisualStateGroup>"
            L"</VisualStateManager.VisualStateGroups>"
            L"<Rectangle Width='100' Height='100' Fill='Red' x:Name='Rect1' />"
            L"</StackPanel>"
            L"    </ControlTemplate>"
            L"  </Button.Template>"
            L"</Button>";

        bool VsmIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool VsmIntegrationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<ControlWithAttachedProperty>());
            return true;
        }

        bool VsmIntegrationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();

            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void VsmIntegrationTests::TextParsingStoryboardAssociationRegression()
        {
            TestCleanupWrapper cleanup;

            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

            // This XAML is making sure that legacy apps using the old deferred VSM function properly. The old
            // deferred VSM breaks DO's rules for association by performing a pseudo-enter on its child storyboard.
            // This allows for a Storyboard in a ResourceDictionary to be placed in a VisualState. Typically you would
            // be unable to do something like this.
            RunOnUIThread([&] () {
                xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"<StackPanel.Resources>"
                    L"    <Storyboard x:Name='HideUI' x:Uid='HideUI'>"
                    L"        <FadeOutThemeAnimation BeginTime='0:0:5' TargetName='PageStackPanel'/>"
                    L"    </Storyboard>"
                    L"</StackPanel.Resources>"
                    L"<VisualStateManager.VisualStateGroups>"
                    L"    <VisualStateGroup x:Name='ShowHideStates'>"
                    L"        <VisualState x:Name='LabelShow'>"
                    L"            <Storyboard>"
                    L"                <FadeInThemeAnimation TargetName='PageEditText' />"
                    L"            </Storyboard>"
                    L"        </VisualState>"
                    L"        <VisualState x:Name='LabelHide'>"
                    L"            <StaticResource ResourceKey='HideUI'/>"
                    L"        </VisualState>"
                    L"        <VisualState x:Name='FinalLabelShowAndHide'>"
                    L"            <Storyboard>"
                    L"                <DoubleAnimation Storyboard.TargetName='PageEditText' Storyboard.TargetProperty='Opacity' To='0' Duration='0'/>"
                    L"            </Storyboard>"
                    L"        </VisualState>"
                    L"    </VisualStateGroup>"
                    L"</VisualStateManager.VisualStateGroups>"
                    L"</StackPanel>"
                );
            });
        }

        void VsmIntegrationTests::ReferenceToControlTemplateResources()
        {
            TestCleanupWrapper cleanup;

            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

            RunOnUIThread([&]() {
                auto root = safe_cast<xaml_controls::Button^> (xaml_markup::XamlReader::Load(
                    L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button.Template>"
                    L"    <ControlTemplate TargetType='ButtonBase'>"
                    L"      <Grid Background='Transparent'>"
                    L"        <Grid.Resources>"
                    L"          <SolidColorBrush x:Key='customColor' Color='#CCCCCCCC' />"
                    L"        </Grid.Resources>"
                    L"        <ContentPresenter x:Name='Text' Content='{TemplateBinding Content}' />"
                    L"        <Rectangle"
                    L"            x:Name='FocusVisual'"
                    L"            Fill='Red' />"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"          <VisualStateGroup x:Name='FocusStates'>"
                    L"            <VisualState x:Name='Focused'>"
                    L"              <Storyboard>"
                    L"                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='FocusVisual' Storyboard.TargetProperty='Fill'>"
                    L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='{StaticResource customColor}'/>"
                    L"                </ObjectAnimationUsingKeyFrames>"
                    L"              </Storyboard>"
                    L"            </VisualState>"
                    L"            <VisualState x:Name='Unfocused'/>"
                    L"          </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"      </Grid>"
                    L"    </ControlTemplate>"
                    L"  </Button.Template>"
                    L"</Button>"
                    ));

                root->ApplyTemplate();

                VisualStateManager::GoToState(root, L"Focused", false);
            });
        }

        void VsmIntegrationTests::FindNameOutsideControlTemplate()
        {
            TestCleanupWrapper cleanup;

            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);

            RunOnUIThread([&]() {
                auto root = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"<VisualStateManager.VisualStateGroups>"
                    L"    <VisualStateGroup x:Name='VSG1'>"
                    L"        <VisualState x:Name='VS1'>"
                    L"            <Storyboard Duration='0:0:0.1' >"
                    L"                <DoubleAnimation Storyboard.TargetName='AnimationTarget' Storyboard.TargetProperty='Opacity' From='0' To='0.5'/>"
                    L"            </Storyboard>"
                    L"        </VisualState>"
                    L"        <VisualState x:Name='VS2'/>"
                    L"        <VisualStateGroup.Transitions>"
                    L"            <VisualTransition GeneratedDuration='0:0:0.1'>"
                    L"                <Storyboard Duration='0:0:0.1'>"
                    L"                    <DoubleAnimation Storyboard.TargetName='AnimationTarget' Storyboard.TargetProperty='Opacity' From='1' To='0'/>"
                    L"                </Storyboard>"
                    L"            </VisualTransition>"
                    L"        </VisualStateGroup.Transitions>"
                    L"    </VisualStateGroup>"
                    L"</VisualStateManager.VisualStateGroups>"
                    L"</Grid>"));

                VERIFY_IS_FALSE(root->FindName(L"VSG1") == nullptr);
            });
        }

        void VsmIntegrationTests::TransitionNamesAreCaseInsensitive()
        {
            TestCleanupWrapper cleanup;
            auto storyboardCompletedRegistration = CreateSafeEventRegistration(xaml_animation::Storyboard, Completed);
            auto storyboardCompletedEvent = std::make_shared<Event>();

            RunOnUIThread([&]() {
                auto root = safe_cast<xaml_controls::Button^>(xaml_markup::XamlReader::Load(
                    L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button.Template>"
                    L"    <ControlTemplate TargetType='ButtonBase'>"
                    L"      <Grid Background='Transparent'>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"          <VisualStateGroup>"
                    L"            <VisualStateGroup.Transitions>"
                    L"              <VisualTransition From='FOCUSED' To='UNFOCUSED'>"
                    L"                <Storyboard x:Name='myStoryboard' />"
                    L"              </VisualTransition>"
                    L"            </VisualStateGroup.Transitions>"
                    L"            <VisualState x:Name='Focused' />"
                    L"            <VisualState x:Name='Unfocused' />"
                    L"          </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"      </Grid>"
                    L"    </ControlTemplate>"
                    L"  </Button.Template>"
                    L"</Button>"
                    ));

                root->ApplyTemplate();

                auto grid = (FrameworkElement^)xaml_media::VisualTreeHelper::GetChild(root, 0);

                auto transitionStoryboard = VisualStateManager::GetVisualStateGroups(grid)->GetAt(0)->Transitions->GetAt(0)->Storyboard;
                storyboardCompletedRegistration.Attach(transitionStoryboard,
                    ref new wf::EventHandler<Object^>(
                    [storyboardCompletedEvent]
                (Object^ sender, Object^ e)
                {
                    storyboardCompletedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = root;
                root->UpdateLayout();

                VisualStateManager::GoToState(root, L"Focused", /* useTransitions */ false);
                VisualStateManager::GoToState(root, L"Unfocused", /* useTransitions */ true);
            });

            storyboardCompletedEvent->WaitForDefault();
        }

        void VsmIntegrationTests::FaultedInRelativeBindingEvaluation()
        {
            RelativeBindingBaseTest();
        }

        void VsmIntegrationTests::TextParsingRelativeBindingEvaluation()
        {
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(
                RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
            RelativeBindingBaseTest();
        }

        void VsmIntegrationTests::RelativeBindingBaseTest()
        {
            TestCleanupWrapper cleanup;

            Button^ button1 = nullptr;
            RunOnUIThread([&]() {
                button1 = safe_cast<xaml_controls::Button^> (xaml_markup::XamlReader::Load(
                    Platform::StringReference(genericVsm1)));
                button1->MinWidth = 200;
                TestServices::WindowHelper->WindowContent = button1;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                // Ensure VSM is faulted in.
                VisualStateManager::GetVisualStateGroups(button1)->GetAt(0);

                // Attempt to go to a state and ensure the RelativeSource binding
                // is evaluated.
                VisualStateManager::GoToState(button1, "VisualState4", true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                // If the binding was successfully evaluated it will have set the Width of the
                // rectangle to the MinWidth of the button.
                StackPanel^ stackPanel = safe_cast<xaml_controls::StackPanel^>(VisualTreeHelper::GetChild(button1, 0));
                xaml_shapes::Rectangle^ rect = safe_cast<xaml_shapes::Rectangle^>(stackPanel->Children->GetAt(0));
                VERIFY_ARE_EQUAL(rect->Width, button1->MinWidth);
            });
        }

        void VsmIntegrationTests::VisualTransitionsWithDynamicTimelinesNonzero()
        {
            TestCleanupWrapper cleanup;

            auto storyboardMonitor = ref new StoryboardMonitorWrapper();

            int fadeOutAnimationCount = 0;
            storyboardMonitor->AttachStartedHandler(
                [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
            {
                if (storyboard->Children->Size > 0 &&
                    storyboard->Children->GetAt(0)->GetType()->FullName == L"Microsoft.UI.Xaml.Media.Animation.FadeOutThemeAnimation")
                {
                    fadeOutAnimationCount++;
                    LOG_OUTPUT(L"Saw a fadeout animation.");
                }
            });

            Button^ button1 = nullptr;
            RunOnUIThread([&]() {
                button1 = safe_cast<xaml_controls::Button^> (xaml_markup::XamlReader::Load(
                    Platform::StringReference(genericVsm1)));
                TestServices::WindowHelper->WindowContent = button1;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                VisualStateManager::GoToState(button1, "VisualState1", true);
                VisualStateManager::GoToState(button1, "VisualState2", true);
                VERIFY_ARE_EQUAL(fadeOutAnimationCount, 1);
            });
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_ARE_EQUAL(fadeOutAnimationCount, 2);
        }

        void VsmIntegrationTests::ReRegistrationOfTemplatedNames()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::ToggleSwitch^ toggleSwitch = nullptr;

            RunOnUIThread([&]()
            {
                toggleSwitch = ref new xaml_controls::ToggleSwitch();
                TestServices::WindowHelper->WindowContent = toggleSwitch;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(toggleSwitch, L"On", true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(toggleSwitch, L"Off", true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(toggleSwitch, L"On", true);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void VsmIntegrationTests::FaultedInTransitionNonMatchingStateName()
        {
            TestCleanupWrapper cleanup;
            auto storyboardCompletedRegistration = CreateSafeEventRegistration(xaml_animation::Storyboard, Completed);
            auto storyboardCompletedEvent = std::make_shared<Event>();

            RunOnUIThread([&]() {
                auto root = safe_cast<xaml_controls::Button^>(xaml_markup::XamlReader::Load(
                    L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button.Template>"
                    L"    <ControlTemplate TargetType='ButtonBase'>"
                    L"      <Grid Background='Transparent'>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"          <VisualStateGroup>"
                    L"            <VisualStateGroup.Transitions>"
                    L"              <VisualTransition From='FOCUSED' To='*'>"
                    L"                <Storyboard x:Name='myStoryboard' />"
                    L"              </VisualTransition>"
                    L"            </VisualStateGroup.Transitions>"
                    L"            <VisualState x:Name='Focused' />"
                    L"            <VisualState x:Name='Unfocused' />"
                    L"          </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"      </Grid>"
                    L"    </ControlTemplate>"
                    L"  </Button.Template>"
                    L"</Button>"
                    ));

                root->ApplyTemplate();
                auto grid = (FrameworkElement^)xaml_media::VisualTreeHelper::GetChild(root, 0);

                auto transitionStoryboard = VisualStateManager::GetVisualStateGroups(grid)->GetAt(0)->Transitions->GetAt(0)->Storyboard;
                storyboardCompletedRegistration.Attach(transitionStoryboard,
                    ref new wf::EventHandler<Object^>(
                    [storyboardCompletedEvent]
                (Object^ sender, Object^ e)
                {
                    storyboardCompletedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = root;
                root->UpdateLayout();

                VisualStateManager::GoToState(root, L"Focused", /* useTransitions */ false);
                VisualStateManager::GoToState(root, L"Unfocused", /* useTransitions */ true);
            });

            storyboardCompletedEvent->WaitForDefault();
        }

        void VsmIntegrationTests::FindNameFaultsInChildrenInTemplate()
        {
            TestCleanupWrapper cleanup;
            RunOnUIThread([&]() {
                auto root = safe_cast<xaml_controls::Button^>(xaml_markup::XamlReader::Load(
                    L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button.Template>"
                    L"    <ControlTemplate TargetType='ButtonBase'>"
                    L"      <Grid Background='Transparent'>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"          <VisualStateGroup>"
                    L"            <VisualStateGroup.Transitions>"
                    L"              <VisualTransition From='FOCUSED' To='*'>"
                    L"                <Storyboard x:Name='myStoryboard' />"
                    L"              </VisualTransition>"
                    L"            </VisualStateGroup.Transitions>"
                    L"            <VisualState x:Name='Focused' />"
                    L"            <VisualState x:Name='Unfocused' />"
                    L"          </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"      </Grid>"
                    L"    </ControlTemplate>"
                    L"  </Button.Template>"
                    L"</Button>"
                    ));
                root->ApplyTemplate();
                auto grid = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(root, 0));

                auto result = safe_cast<VisualState^>(grid->FindName(L"Focused"));
                VERIFY_IS_TRUE(result != nullptr);
            });
        }

        void VsmIntegrationTests::FindNameWorksForTransitioningVsm()
        {
            TestCleanupWrapper cleanup;

            Button^ button1 = nullptr;
            RunOnUIThread([&]() {
                button1 = safe_cast<xaml_controls::Button^> (xaml_markup::XamlReader::Load(
                    Platform::StringReference(genericVsm1)));
                TestServices::WindowHelper->WindowContent = button1;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                VisualStateManager::GoToState(button1, "VisualState1", true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                auto grid = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(button1, 0));
                auto result = safe_cast<VisualState^>(grid->FindName(L"VisualState1"));
                VERIFY_IS_TRUE(result != nullptr);
            });
        }

        void VsmIntegrationTests::GoToStateWithPiledUpStoryboardCompletionEvents()
        {
            TestCleanupWrapper cleanup;

            Button^ button1 = nullptr;
            Grid^ grid1 = nullptr;
            RunOnUIThread([&]() {
                button1 = safe_cast<xaml_controls::Button^> (xaml_markup::XamlReader::Load(
                    Platform::StringReference(genericVsm1)));

                grid1 = ref new Grid();
                TestServices::WindowHelper->WindowContent = grid1;
                grid1->Children->Append(button1);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                // Fault in the VSM so we're reusing the same storyboards
                auto grid = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(button1, 0));
                auto result = safe_cast<VisualState^>(grid->FindName(L"VisualState1"));

                VisualStateManager::GoToState(button1, "VisualState1", true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                VisualStateManager::GoToState(button1, "VisualState2", true);
                grid1->Children->Clear();
                grid1->Children->Append(button1);
                VisualStateManager::GoToState(button1, "VisualState1", true);
                VisualStateManager::GoToState(button1, "VisualState2", true);
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void VsmIntegrationTests::VsmAppliesFirstAndLastFrameOfTransitionAnimationSynchronously()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureDisableGlobalAnimations(
                RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableGlobalAnimations, true);

            Button^ button1 = nullptr;
            RunOnUIThread([&]() {
                button1 = safe_cast<xaml_controls::Button^>(xaml_markup::XamlReader::Load(
                    L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button.Template>"
                    L"    <ControlTemplate TargetType='ButtonBase'>"
                    L"      <Grid Background='Transparent'>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"          <VisualStateGroup x:Name='VSG1'>"
                    L"              <VisualState x:Name='VS1'>"
                    L"                  <Storyboard x:Name='SB1'>"
                    L"                      <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Rect1' Storyboard.TargetProperty='Fill'>"
                    L"                          <DiscreteObjectKeyFrame KeyTime='0:0:0' Value='Green' />"
                    L"                      </ObjectAnimationUsingKeyFrames>"
                    L"                  </Storyboard>"
                    L"              </VisualState>"
                    L"              <VisualState x:Name='VS2'>"
                    L"                  <Storyboard x:Name='SB2'>"
                    L"                      <DoubleAnimation Storyboard.TargetName='Rect1' Storyboard.TargetProperty='Width' To='200'/>"
                    L"                  </Storyboard>"
                    L"              </VisualState>"
                    L"              <VisualStateGroup.Transitions>"
                    L"                  <VisualTransition>"
                    L"                      <Storyboard x:Name='SB3'>"
                    L"                          <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Rect1' Storyboard.TargetProperty='Fill'>"
                    L"                              <DiscreteObjectKeyFrame KeyTime='0:0:0' Value='Red' />"
                    L"                              <DiscreteObjectKeyFrame KeyTime='0:0:1' Value='Blue' />"
                    L"                          </ObjectAnimationUsingKeyFrames>"
                    L"                      </Storyboard>"
                    L"                  </VisualTransition>"
                    L"              </VisualStateGroup.Transitions>"
                    L"          </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"        <Rectangle x:Name='Rect1' Fill='Red' Width='100' />"
                    L"      </Grid>"
                    L"    </ControlTemplate>"
                    L"  </Button.Template>"
                    L"</Button>"));
                TestServices::WindowHelper->WindowContent = button1;
            });
            TestServices::WindowHelper->WaitForIdle();

            xaml_shapes::Rectangle^ rect = nullptr;
            RunOnUIThread([&]() {
                auto grid = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(button1, 0));
                rect = safe_cast<xaml_shapes::Rectangle^>(grid->FindName(L"Rect1"));
                VisualStateManager::GoToState(button1, L"VS2", true);
            });
            TestServices::WindowHelper->WaitForIdle();

            // When system animations are disabled VSM will first skip the VisualTransition to the begin value, then
            // skip the VisualTransition storyboard to its final frame, synchronously applying its property values each time.
            // At the same time the VT's Storyboard will fire off an asynchronous completed event that will, upon execution,
            // start the destination storyboard, which will set the rectangle Green.
            int propertyChangedCounter = 0;
            auto handlerSimple = ref new DependencyPropertyChangedCallback([&](DependencyObject^ sender, DependencyProperty^ prop) {
                auto fillBrush = safe_cast<SolidColorBrush^>(rect->Fill);
                auto c1 = fillBrush->Color;
                Color c2 = Colors::Black;
                VERIFY_IS_TRUE(propertyChangedCounter < 2);
                if (propertyChangedCounter == 0) c2 = Colors::Red;
                if (propertyChangedCounter == 1) c2 = Colors::Blue;
                VERIFY_IS_TRUE(c1.A == c2.A && c1.R == c2.R && c1.G == c2.G && c1.B == c2.B);
                propertyChangedCounter++;
            });

            RunOnUIThread([&]() {
                auto token = rect->RegisterPropertyChangedCallback(xaml_shapes::Shape::FillProperty, handlerSimple);

                VisualStateManager::GoToState(button1, L"VS1", true);
                auto fillBrush = safe_cast<SolidColorBrush^>(rect->Fill);
                auto c1 = fillBrush->Color;
                auto c2 = Colors::Blue;
                VERIFY_IS_TRUE(c1.A == c2.A && c1.R == c2.R && c1.G == c2.G && c1.B == c2.B);

                rect->UnregisterPropertyChangedCallback(xaml_shapes::Shape::FillProperty, token);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                auto fillBrush = safe_cast<SolidColorBrush^>(rect->Fill);
                auto c1 = fillBrush->Color;
                auto c2 = Colors::Green;
                VERIFY_IS_TRUE(c1.A == c2.A && c1.R == c2.R && c1.G == c2.G && c1.B == c2.B);
            });
        }

        void VsmIntegrationTests::PeerLifetimePreservedDuringAnimation()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureDisableGlobalAnimations(
                RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableGlobalAnimations, true);

            // Create a ContentControl with the Content locally-set to a SymbolIcon, and
            // animated to a different SymbolIcon.
            UserControl^ root = nullptr;
            RunOnUIThread([&]() {
                root = safe_cast<xaml_controls::UserControl^>(xaml_markup::XamlReader::Load(
                    L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Grid>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"            <VisualStateGroup x:Name='PlayPauseStates'>"
                    L"                <VisualState x:Name='PlayState'/>"
                    L"                <VisualState x:Name='PauseState'>"
                    L"                    <Storyboard>"
                    L"                        <ObjectAnimationUsingKeyFrames "
                    L"                            Storyboard.TargetName='ContentControl' "
                    L"                            Storyboard.TargetProperty='Content' >"
                    L"                            <DiscreteObjectKeyFrame KeyTime='0'>"
                    L"                                <DiscreteObjectKeyFrame.Value>"
                    L"                                    <SymbolIcon Symbol='Pause' />"
                    L"                                </DiscreteObjectKeyFrame.Value>"
                    L"                            </DiscreteObjectKeyFrame>"
                    L"                        </ObjectAnimationUsingKeyFrames>"
                    L"                    </Storyboard>"
                    L"                </VisualState>"
                    L"            </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L""
                    L"        <ContentControl x:Name='ContentControl' >"
                    L"            <SymbolIcon Symbol='Play' />"
                    L"        </ContentControl>"
                    L"    </Grid>"
                    L"</UserControl>"));

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {

                auto setAutomationId = ref new String(L"ID");

                // Get a weak ref on the locally-set SymbolIcon for testing.
                auto contentControl = static_cast<ContentControl^>(root->FindName(L"ContentControl"));
                WeakReference weakRef(static_cast<SymbolIcon^>(contentControl->Content));

                // Set a property on this SymbolIcon that will force the DXaml peer to have state.
                auto symbolIcon = weakRef.Resolve<SymbolIcon>();
                AutomationProperties::SetAutomationId(symbolIcon, setAutomationId);
                symbolIcon = nullptr;

                // Replace the locally-set SymbolIcon with a animated one, then bring it back.
                // When it comes back on a chk build, it shouldn't assert in OnManagedPeerCreated
                VisualStateManager::GoToState(root, L"PauseState", false);
                VisualStateManager::GoToState(root, L"PlayState", false);

                // Use the weak ref to verify that the locally-set icon's DXaml peer survived.
                symbolIcon = weakRef.Resolve<SymbolIcon>();
                VERIFY_IS_TRUE(symbolIcon != nullptr );
                auto getAutomationId = AutomationProperties::GetAutomationId(symbolIcon);
                VERIFY_IS_TRUE(getAutomationId == setAutomationId);
            });
        }

        void VsmIntegrationTests::GoToStateWithRunningTransitionStoryboard()
        {
            TestCleanupWrapper cleanup;

            String^ xamlString =
                L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Button.Template>"
                L"    <ControlTemplate TargetType='ButtonBase'>"
                L"      <StackPanel>"
                L"        <VisualStateManager.VisualStateGroups>"
                L"            <VisualStateGroup x:Name='Group1'>"
                L"                <VisualStateGroup.Transitions>"
                L"                    <VisualTransition From='VisualState1' To='VisualState2'>"
                L"                        <Storyboard x:Name='VisualTransitionStoryboard'>"
                L"                            <PointerUpThemeAnimation TargetName='Border1' />"
                L"                        </Storyboard>"
                L"                    </VisualTransition>"
                L"                </VisualStateGroup.Transitions>"
                L"                <VisualState x:Name='VisualState1' />"
                L"                <VisualState x:Name='VisualState2' />"
                L"            </VisualStateGroup>"
                L"        </VisualStateManager.VisualStateGroups>"
                L"        <Border x:Name='Border1' Width='100' Height='100' />"
                L"      </StackPanel>"
                L"    </ControlTemplate>"
                L"  </Button.Template>"
                L"</Button>";

            Button^ button1 = nullptr;
            Grid^ grid1 = nullptr;
            Border^ border1 = nullptr;

            RunOnUIThread([&]() {
                button1 = safe_cast<xaml_controls::Button^> (xaml_markup::XamlReader::Load(xamlString));

                grid1 = ref new Grid();
                TestServices::WindowHelper->WindowContent = grid1;
                grid1->Children->Append(button1);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                // Fault in the VSM so we're reusing the same storyboards
                auto grid = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(button1, 0));
                auto result = safe_cast<VisualState^>(grid->FindName(L"VisualState1"));

                // Start transition. This begins the storyboard, but the PointerUpThemeAnimation won't generate
                // child timelines unless the animation target has both Projection and RenderTransform values.
                VisualStateManager::GoToState(button1, "VisualState1", true);
                VisualStateManager::GoToState(button1, "VisualState2", true);

                // Set Projection and RenderTransform on the animation target.
                border1 = safe_cast<Border^>(grid->FindName(L"Border1"));
                border1->Projection = ref new PlaneProjection();
                border1->RenderTransform = ref new ScaleTransform();

                // Start transition again. This time the PointerUpThemeAnimation will generate child
                // timelines, which will trigger an error if VSM lets the storyboard continue running.
                VisualStateManager::GoToState(button1, "VisualState1", true);
                VisualStateManager::GoToState(button1, "VisualState2", true);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        ref class CustomVsm sealed:
            public xaml::VisualStateManager
        {
        public:
            bool GoToStateCore(
                Control^ control,
                FrameworkElement^ templateRoot,
                String^ stateName,
                VisualStateGroup^ group,
                VisualState^ state,
                bool useTransitions
                ) override
            {
                return xaml::VisualStateManager::GoToStateCore(
                    control, templateRoot, stateName, group, state, useTransitions);
            }
        };

        void VsmIntegrationTests::CannotTargetNonControlInOverridenVsm()
        {
            TestCleanupWrapper cleanup;
            LOG_OUTPUT(L"Rendering a XAML framework with a FrameworkElement containing a VSM "
                L"that isn't a direct root of a Control.");

            Grid^ root = nullptr;
            CustomVsm^ customVsm = nullptr;
            RunOnUIThread([&]() {
                root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Background='Transparent'>"
                    L"  <VisualStateManager.VisualStateGroups>"
                    L"    <VisualStateGroup x:Name='VSG1'>"
                    L"        <VisualState x:Name='VS1'>"
                    L"            <Storyboard x:Name='SB1'>"
                    L"                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Rect1' Storyboard.TargetProperty='Fill'>"
                    L"                    <DiscreteObjectKeyFrame KeyTime='0:0:0' Value='Green' />"
                    L"                </ObjectAnimationUsingKeyFrames>"
                    L"            </Storyboard>"
                    L"        </VisualState>"
                    L"    </VisualStateGroup>"
                    L"  </VisualStateManager.VisualStateGroups>"
                    L"  <Rectangle x:Name='Rect1' Fill='Red' Width='100' />"
                    L"</Grid>"));

                customVsm = ref new CustomVsm();
                VisualStateManager::SetCustomVisualStateManager(root, customVsm);
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Calling GoToStateCore via the Custom VSM.");
            RunOnUIThread([&]() {
                auto vsmGroup = safe_cast<VisualStateGroup^>(root->FindName(L"VSG1"));
                auto vsmState = safe_cast<VisualState^>(root->FindName(L"VS1"));
                VERIFY_IS_FALSE(customVsm->GoToStateCore(ref new UserControl(), root, L"VS1", vsmGroup, vsmState, true));

                LOG_OUTPUT(L"Verifying the state was not applied.");
                auto rect = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"Rect1"));
                auto fillBrush = safe_cast<SolidColorBrush^>(rect->Fill);
                auto c1 = fillBrush->Color;
                auto c2 = Colors::Red;
                VERIFY_IS_TRUE(c1.A == c2.A && c1.R == c2.R && c1.G == c2.G && c1.B == c2.B);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        ref class StringDataSource sealed
            : public Microsoft::UI::Xaml::Tests::Common::CustomPropertyProviderBase
        {
            Platform::String^ value_;
        protected:
            void AddCustomProperties() override
            {
                AddCustomProperty(L"Value", Platform::String::typeid,
                    MAKEPROPGET(StringDataSource^, Value),
                    MAKEPROPSET(StringDataSource^, Value, Platform::String^)
                    );
            }
        public:
            property Platform::String^ Value
            {
                Platform::String^ get() { return value_; }
                void set(Platform::String^ value)
                {
                    if (value != value_)
                    {
                        value_ = value;
                        FirePropertyChanged(L"Value");
                    }
                }
            }
        };

        void VsmIntegrationTests::GoToStateWithoutTransitionsReappliesBindings()
        {
            TestCleanupWrapper cleanup;

            Button^ root = nullptr;
            StringDataSource^ stringSource = nullptr;

            LOG_OUTPUT(L"Rendering a Button with a VSM targeting the Content of a TextBlock.");
            RunOnUIThread([&]() {
                stringSource = ref new StringDataSource();
                stringSource->Value = "HelloWorld2";
                root = safe_cast<xaml_controls::Button^> (xaml_markup::XamlReader::Load(
                    L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button.Template>"
                    L"    <ControlTemplate TargetType='ButtonBase'>"
                    L"      <Grid>"
                    L"        <TextBlock x:Name='MyTextBlock' Text='HelloWorld1' />"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"          <VisualStateGroup x:Name='VSG1'>"
                    L"            <VisualState x:Name='VS1'>"
                    L"              <Storyboard>"
                    L"                <ObjectAnimationUsingKeyFrames Storyboard.TargetName='MyTextBlock' Storyboard.TargetProperty='Text'>"
                    L"                  <DiscreteObjectKeyFrame KeyTime='0' Value='{Binding RelativeSource={RelativeSource TemplatedParent}, Path=Tag.Value}'/>"
                    L"                </ObjectAnimationUsingKeyFrames>"
                    L"              </Storyboard>"
                    L"            </VisualState>"
                    L"          </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"      </Grid>"
                    L"    </ControlTemplate>"
                    L"  </Button.Template>"
                    L"</Button>"));
                root->Tag = stringSource;
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            TextBlock^ targetTextBlock = nullptr;
            RunOnUIThread([&]() {
                LOG_OUTPUT(L"Retrieving the TextBlock and verifying its initial value is correct.");
                auto grid = (FrameworkElement^)xaml_media::VisualTreeHelper::GetChild(root, 0);
                targetTextBlock = safe_cast<TextBlock^>(grid->FindName(L"MyTextBlock"));
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(targetTextBlock->Text, L"HelloWorld1") == 0);

                LOG_OUTPUT(L"Calling GoToState and ensuring its final value is applied synchronously.");
                VisualStateManager::GoToState(root, L"VS1", true);
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(targetTextBlock->Text, L"HelloWorld2") == 0);
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]() {
                LOG_OUTPUT(L"Updating the value of the Binding expression and ensuring it is NOT applied to the TextBlock");
                stringSource->Value = L"HelloWorld3";
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(targetTextBlock->Text, L"HelloWorld2") == 0);
                LOG_OUTPUT(L"Calling GoToState(useTransitions:false) and ensuring the updated binding value is NOT applied synchronously.");
                VisualStateManager::GoToState(root, L"VS1", false);
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(targetTextBlock->Text, L"HelloWorld2") == 0);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                LOG_OUTPUT(L"Finally ensuring, after the UI thread goes idle, that the binding is updated.");
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(targetTextBlock->Text, L"HelloWorld3") == 0);
            });
        }

        void VsmIntegrationTests::AnimationsDisabledDynamicTransitionOnlyApplied()
        {
            TestCleanupWrapper cleanup;

            LOG_OUTPUT(L"Disabling system animations and rendering a Button to the screen.");
            RuntimeEnabledFeatureOverride featureDisableGlobalAnimations(
                RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableGlobalAnimations, true);
            Button^ button1 = nullptr;
            RunOnUIThread([&]() {
                button1 = safe_cast<xaml_controls::Button^>(xaml_markup::XamlReader::Load(
                    L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button.Template>"
                    L"    <ControlTemplate TargetType='ButtonBase'>"
                    L"      <Grid Background='Transparent'>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"          <VisualStateGroup x:Name='VSG1'>"
                    L"              <VisualState x:Name='VS1'>"
                    L"                  <Storyboard>"
                    L"                      <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Rect1' Storyboard.TargetProperty='Fill'>"
                    L"                          <DiscreteObjectKeyFrame KeyTime='0:0:0' Value='Green' />"
                    L"                      </ObjectAnimationUsingKeyFrames>"
                    L"                  </Storyboard>"
                    L"              </VisualState>"
                    L"              <VisualStateGroup.Transitions>"
                    L"                  <VisualTransition GeneratedDuration='0:0:1' />"
                    L"              </VisualStateGroup.Transitions>"
                    L"          </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"        <Rectangle x:Name='Rect1' Fill='Red' Width='100' />"
                    L"      </Grid>"
                    L"    </ControlTemplate>"
                    L"  </Button.Template>"
                    L"</Button>"));
                TestServices::WindowHelper->WindowContent = button1;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Calling GoToState(VS1) and verifying the dynamic transition animation is generated and fires the completion event "
                L" to trigger the application of the destination storyboard.");
            xaml_shapes::Rectangle^ rect = nullptr;
            RunOnUIThread([&]() {
                auto grid = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(button1, 0));
                rect = safe_cast<xaml_shapes::Rectangle^>(grid->FindName(L"Rect1"));
                VisualStateManager::GoToState(button1, L"VS1", true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                auto fillBrush = safe_cast<SolidColorBrush^>(rect->Fill);
                auto c1 = fillBrush->Color;
                auto c2 = Colors::Green;
                VERIFY_IS_TRUE(c1.A == c2.A && c1.R == c2.R && c1.G == c2.G && c1.B == c2.B);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        Platform::String^ VsmIntegrationTests::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\framework\\");
        }

        void VsmIntegrationTests::VerifyGeneratedTransitionBetweenEmptyAndSetter()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

            // The following UserControl has a VisualStateManager with two states.
            // State 1 is empty and State 2 uses three Setters.
            // The Setters target a color property and a double property that will also be the
            // target of the generated VisualTransition.
            // The third Setter is for a property that is targeted explicitly by an animation
            // specified by the user in the VisualTransition; the idea is to test that we
            // respect these animations too.
            UserControl^ root = nullptr;
            RunOnUIThread([&]() {
                root = safe_cast<xaml_controls::UserControl^>(xaml_markup::XamlReader::Load(
                    L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Grid x:Name='Root' Background='Transparent' HorizontalAlignment='Left' VerticalAlignment='Top'>"
                    L"        <Grid.RenderTransform>"
                    L"            <TranslateTransform x:Name='NGAT' Y='0'/>"
                    L"        </Grid.RenderTransform>"
                    L"        <Grid.ColumnDefinitions>"
                    L"            <ColumnDefinition/>"
                    L"            <ColumnDefinition/>"
                    L"        </Grid.ColumnDefinitions>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"            <VisualStateGroup x:Name='CommonStates'>"
                    L"                <VisualState x:Name='State1'/>"
                    L"                <VisualState x:Name='State2'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='CAT.(Shape.Fill).(SolidColorBrush.Color)' Value='#FF00FF00'/>"
                    L"                        <Setter Target='DAT.Opacity' Value='0.67'/>"
                    L"                        <Setter Target='NGAT.(TranslateTransform.Y)' Value='10'/>"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"                <VisualStateGroup.Transitions>"
                    L"                    <VisualTransition From='State1' To='State2' GeneratedDuration='0:0:1'>"
                    L"                        <Storyboard>"
                    L"                            <DoubleAnimation Storyboard.TargetName='NGAT' Storyboard.TargetProperty='Y' To='10' Duration='0:0:0.5'/>"
                    L"                        </Storyboard>"
                    L"                    </VisualTransition>"
                    L"                    <VisualTransition From='State2' To='State1' GeneratedDuration='0:0:1'>"
                    L"                        <Storyboard>"
                    L"                            <DoubleAnimation Storyboard.TargetName='NGAT' Storyboard.TargetProperty='Y' To='0' Duration='0:0:0.5'/>"
                    L"                        </Storyboard>"
                    L"                    </VisualTransition>"
                    L"                </VisualStateGroup.Transitions>"
                    L"            </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"        <Rectangle x:Name='CAT' Grid.Column='0' Width='200' Height='200' Fill='#FFFF0000'/>"
                    L"        <Rectangle x:Name='DAT' Grid.Column='1' Width='200' Height='200' Fill='#FFFFFFFF' Opacity='1.0'/>"
                    L"    </Grid>"
                    L"</UserControl>"));

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Start in State 1.
            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State1", false);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Go to State 2 using transitions.
            // Tick the UI thread synchronously so the animations start.
            // At this point, the mock DComp output should show six animations:
            // One for the generated double animation, four for the generated color animation
            // (i.e. A, R, G and B) and one for the non-generated animation.
            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State2", true);
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(1);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

            // Skip to State 2.
            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State2", false);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Go back to State 1 using transitions.
            // Verify mock DComp output in the same way as before.
            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State1", true);
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(1);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State1", false);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void VsmIntegrationTests::VerifyGeneratedTransitionBetweenSetterAndSetter()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

            // The following UserControl has a VisualStateManager with two states.
            // Both State 1 and State 2 use three Setters.
            // The Setters include a color property and a double property that will also be the
            // target of the generated VisualTransition.
            // The third Setter is for a property that is targeted explicitly by an animation
            // specified by the user in the VisualTransition; the idea is to test that we
            // respect these animations too.
            UserControl^ root = nullptr;
            RunOnUIThread([&]() {
                root = safe_cast<xaml_controls::UserControl^>(xaml_markup::XamlReader::Load(
                    L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Grid x:Name='Root' Background='Transparent' HorizontalAlignment='Left' VerticalAlignment='Top'>"
                    L"        <Grid.RenderTransform>"
                    L"            <TranslateTransform x:Name='NGAT' Y='0'/>"
                    L"        </Grid.RenderTransform>"
                    L"        <Grid.ColumnDefinitions>"
                    L"            <ColumnDefinition/>"
                    L"            <ColumnDefinition/>"
                    L"        </Grid.ColumnDefinitions>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"            <VisualStateGroup x:Name='CommonStates'>"
                    L"                <VisualState x:Name='State1'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='CAT.(Shape.Fill).(SolidColorBrush.Color)' Value='#FF00FF00'/>"
                    L"                        <Setter Target='DAT.Opacity' Value='0.67'/>"
                    L"                        <Setter Target='NGAT.(TranslateTransform.Y)' Value='10'/>"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"                <VisualState x:Name='State2'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='CAT.(Shape.Fill).(SolidColorBrush.Color)' Value='#FF0000FF'/>"
                    L"                        <Setter Target='DAT.Opacity' Value='0.33'/>"
                    L"                        <Setter Target='NGAT.(TranslateTransform.Y)' Value='20'/>"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"                <VisualStateGroup.Transitions>"
                    L"                    <VisualTransition From='State1' To='State2' GeneratedDuration='0:0:1'>"
                    L"                        <Storyboard>"
                    L"                            <DoubleAnimation Storyboard.TargetName='NGAT' Storyboard.TargetProperty='Y' To='20' Duration='0:0:0.5'/>"
                    L"                        </Storyboard>"
                    L"                    </VisualTransition>"
                    L"                    <VisualTransition From='State2' To='State1' GeneratedDuration='0:0:1'>"
                    L"                        <Storyboard>"
                    L"                            <DoubleAnimation Storyboard.TargetName='NGAT' Storyboard.TargetProperty='Y' To='10' Duration='0:0:0.5'/>"
                    L"                        </Storyboard>"
                    L"                    </VisualTransition>"
                    L"                </VisualStateGroup.Transitions>"
                    L"            </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"        <Rectangle x:Name='CAT' Grid.Column='0' Width='200' Height='200' Fill='#FFFF0000'/>"
                    L"        <Rectangle x:Name='DAT' Grid.Column='1' Width='200' Height='200'  Fill='#FFFFFFFF' Opacity='1.0'/>"
                    L"    </Grid>"
                    L"</UserControl>"));

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Start in State 1.
            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State1", false);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Go to State 2 using transitions.
            // Tick the UI thread synchronously so the animations start.
            // At this point, the mock DComp output should show six animations:
            // One for the generated double animation, four for the generated color animation
            // (i.e. A, R, G and B) and one for the non-generated animation.
            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State2", true);
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(1);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

            // Skip to State 2.
            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State2", false);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Go back to State 1 using transitions.
            // Verify mock DComp output in the same way as before.
            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State1", true);
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(1);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State1", false);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void VsmIntegrationTests::VerifyGeneratedTransitionBetweenSetterAndStoryboard()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));

            // The following UserControl has a VisualStateManager with two states.
            // State 1 uses three Setters and State 2 uses three Storyboards.
            // The Setters and Storyboards respectively target a color property and a double
            // property that will also be the target of the generated VisualTransition.
            // The third Setter and Storyboard are for a property that is targeted explicitly by
            // an animation specified by the user in the VisualTransition; the idea is to
            // test that we respect these animations too.
            UserControl^ root = nullptr;
            RunOnUIThread([&]() {
                root = safe_cast<xaml_controls::UserControl^>(xaml_markup::XamlReader::Load(
                    L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Grid x:Name='Root' Background='Transparent' HorizontalAlignment='Left' VerticalAlignment='Top'>"
                    L"        <Grid.RenderTransform>"
                    L"            <TranslateTransform x:Name='NGAT' Y='0'/>"
                    L"        </Grid.RenderTransform>"
                    L"        <Grid.ColumnDefinitions>"
                    L"            <ColumnDefinition/>"
                    L"            <ColumnDefinition/>"
                    L"        </Grid.ColumnDefinitions>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"            <VisualStateGroup x:Name='CommonStates'>"
                    L"                <VisualState x:Name='State1'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='CAT.(Shape.Fill).(SolidColorBrush.Color)' Value='#FF0000FF'/>"
                    L"                        <Setter Target='DAT.Opacity' Value='0.33'/>"
                    L"                        <Setter Target='NGAT.(TranslateTransform.Y)' Value='20'/>"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"                <VisualState x:Name='State2'>"
                    L"                    <Storyboard>"
                    L"                        <ColorAnimation Storyboard.TargetName='CAT' Storyboard.TargetProperty='(Shape.Fill).(SolidColorBrush.Color)' To='#FFFFFFFF' Duration='0:0:0'/>"
                    L"                        <DoubleAnimation Storyboard.TargetName='DAT' Storyboard.TargetProperty='Opacity' To='0.0' Duration='0:0:0'/>"
                    L"                        <DoubleAnimation Storyboard.TargetName='NGAT' Storyboard.TargetProperty='Y' To='30' Duration='0:0:0'/>"
                    L"                    </Storyboard>"
                    L"                </VisualState>"
                    L"                <VisualStateGroup.Transitions>"
                    L"                    <VisualTransition From='State1' To='State2' GeneratedDuration='0:0:1'>"
                    L"                        <Storyboard>"
                    L"                            <DoubleAnimation Storyboard.TargetName='NGAT' Storyboard.TargetProperty='Y' To='30' Duration='0:0:0.5'/>"
                    L"                        </Storyboard>"
                    L"                    </VisualTransition>"
                    L"                    <VisualTransition From='State2' To='State1' GeneratedDuration='0:0:1'>"
                    L"                        <Storyboard>"
                    L"                            <DoubleAnimation Storyboard.TargetName='NGAT' Storyboard.TargetProperty='Y' To='20' Duration='0:0:0.5'/>"
                    L"                        </Storyboard>"
                    L"                    </VisualTransition>"
                    L"                </VisualStateGroup.Transitions>"
                    L"            </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"        <Rectangle x:Name='CAT' Grid.Column='0' Width='200' Height='200' Fill='#FFFF0000'/>"
                    L"        <Rectangle x:Name='DAT' Grid.Column='1' Width='200' Height='200'  Fill='#FFFFFFFF' Opacity='1.0'/>"
                    L"    </Grid>"
                    L"</UserControl>"));


                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Start in State 1.
            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State1", false);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Go to State 2 using transitions.
            // Tick the UI thread synchronously so the animations start.
            // At this point, the mock DComp output should show six animations:
            // One for the generated double animation, four for the generated color animation
            // (i.e. A, R, G and B) and one for the non-generated animation.
            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State2", true);
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(1);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

            // Skip to State 2.
            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State2", false);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Go back to State 1 using transitions.
            // Verify mock DComp output in the same way as before.
            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State1", true);
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(1);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State1", false);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void VsmIntegrationTests::MultiStorybardDynamicTimelinesDeduplicate()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            Button^ button1 = nullptr;
            RunOnUIThread([&]() {
                button1 = safe_cast<xaml_controls::Button^>(xaml_markup::XamlReader::Load(
                    L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button.Template>"
                    L"    <ControlTemplate TargetType='ButtonBase'>"
                    L"      <Grid Name='MyGrid' Background='Transparent'>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"          <VisualStateGroup x:Name='VSG1'>"
                    L"              <VisualState x:Name='VS1'>"
                    L"                  <Storyboard>"
                    L"                      <PointerDownThemeAnimation TargetName='Rect1' />"
                    L"                  </Storyboard>"
                    L"              </VisualState>"
                    L"              <VisualStateGroup.Transitions>"
                    L"                  <VisualTransition GeneratedDuration='0:0:1' />"
                    L"              </VisualStateGroup.Transitions>"
                    L"          </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"        <Rectangle x:Name='Rect1' Fill='Red' Width='100' />"
                    L"      </Grid>"
                    L"    </ControlTemplate>"
                    L"  </Button.Template>"
                    L"</Button>"));
                TestServices::WindowHelper->WindowContent = button1;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                // At this point, the PointerDownThemeAnimation would have been expanded out into
                // generated Timelines to fill the non-zero VisualTransition. Previously it would
                // be expanded out once for each of its three timelines, causing a crash when the
                // three sets of timelines, all targeting the same property, were started simultaneously.
                VisualStateManager::GoToState(button1, L"VS1", true);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void VsmIntegrationTests::VerifyDoNotCrashOnSettersWithBadlyAuthoredFullyQualifiedPropertyPaths()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

            // The scenario here is that we try to pre-resolve at least twice a fully qualified property which does not exist.
            // Due to some missing logic, this would result in us trying to party on garbage memory, with predictable results.
            Button^ button1 = nullptr;
            RunOnUIThread([&]() {
                button1 = safe_cast<xaml_controls::Button^>(xaml_markup::XamlReader::Load(
                    L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button.Template>"
                    L"    <ControlTemplate TargetType='ButtonBase'>"
                    L"      <Grid Name='MyGrid' Background='Transparent'>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"          <VisualStateGroup x:Name='VSG1'>"
                    L"              <VisualState x:Name='VS1'>"
                    L"                  <VisualState.Setters>"
                    L"                      <Setter Target='Rect1.(NonexistentClass.NonexistentProperty)' Value='foo' />"
                    L"                      <Setter Target='Rect2.(NonexistentClass.NonexistentProperty)' Value='foo' />"
                    L"                  </VisualState.Setters>"
                    L"                  <Storyboard>"
                    L"                      <PointerDownThemeAnimation TargetName='Rect1' />"
                    L"                  </Storyboard>"
                    L"              </VisualState>"
                    L"              <VisualStateGroup.Transitions>"
                    L"                  <VisualTransition GeneratedDuration='0:0:1' />"
                    L"              </VisualStateGroup.Transitions>"
                    L"          </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"        <Rectangle x:Name='Rect1' Fill='Red' Width='100' />"
                    L"        <Rectangle x:Name='Rect2' Fill='Red' Width='100' />"
                    L"      </Grid>"
                    L"    </ControlTemplate>"
                    L"  </Button.Template>"
                    L"</Button>"));
                TestServices::WindowHelper->WindowContent = button1;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Force fault-in of VSM so that the VSM Setters are created
            RunOnUIThread([&]() {
                auto groups = xaml::VisualStateManager::GetVisualStateGroups(safe_cast<FrameworkElement^>(VisualTreeHelper::GetChild(button1, 0)));
                VERIFY_IS_TRUE(groups->Size > 0);
                auto visualStateGroup = groups->GetAt(0);
                auto visualState = visualStateGroup->States->GetAt(0);
                VERIFY_IS_TRUE(visualState->Setters->Size == 2);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void VsmIntegrationTests::PrepareCustomAttachedPropertiesRoot(Button^& root, Button^& targetControl, bool optimized)
        {
            RunOnUIThread([&]()
            {
                Platform::String^ xamlString =
                    L"<Button xmlns = 'http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                    L"        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"        xmlns:local='using:Tests.Native.External.Framework'>"
                    L"  <Button.Template>"
                    L"      <ControlTemplate TargetType = 'Button'>"
                    L"          <Grid>"
                    L"              <VisualStateManager.VisualStateGroups>"
                    L"                  <VisualStateGroup>"
                    L"                      <VisualState x:Name = 'State1'>"
                    L"                          <VisualState.Setters>"
                    L"                              <Setter Target = 'magicControl.(local:ControlWithAttachedProperty.CustomAttached)' Value = 'Attached String'/>"
                    L"                              <Setter Target = 'magicControl.(local:ControlWithAttachedProperty.CustomAttachedInt)' Value = '1'/>"
                    L"                              <Setter Target = 'magicControl.(local:ControlWithAttachedProperty.CustomAttachedBrush)' Value = 'Red'/>"
                    L"                              <Setter Target = 'magicControl.(local:ControlWithAttachedProperty.CustomAttachedBrushCollection)[0].(SolidColorBrush.Color)' Value = 'Blue'/>"
                    L"                              <Setter Target = 'magicControl.(local:ControlWithAttachedProperty.CustomAttachedBrushCollection)[1].(SolidColorBrush.Color)' Value = 'Orange'/>"
                    L"                          </VisualState.Setters>"
                    L"                      </VisualState>"
                    L"                      <VisualState x:Name = 'State2'>"
                    L"                          <VisualState.Setters>"
                    L"                              <Setter Target = 'magicControl.(local:ControlWithAttachedProperty.CustomAttached)' Value = 'Attached String 2'/>"
                    L"                              <Setter Target = 'magicControl.(local:ControlWithAttachedProperty.CustomAttachedInt)' Value = '2'/>"
                    L"                              <Setter Target = 'magicControl.(local:ControlWithAttachedProperty.CustomAttachedBrush)' Value = 'Blue'/>"
                    L"                              <Setter Target = 'magicControl.(local:ControlWithAttachedProperty.CustomAttachedBrushCollection)[0].(SolidColorBrush.Color)' Value = 'Yellow'/>"
                    L"                              <Setter Target = 'magicControl.(local:ControlWithAttachedProperty.CustomAttachedBrushCollection)[1].(SolidColorBrush.Color)' Value = 'Green'/>"
                    L"                          </VisualState.Setters>"
                    L"                      </VisualState>"
                    L"                  </VisualStateGroup>"
                    L"              </VisualStateManager.VisualStateGroups>"
                    L"              <Button x:Name = 'magicControl'/>"
                    L"          </Grid>"
                    L"      </ControlTemplate>"
                    L"  </Button.Template>"
                    L"</Button>";

                root = safe_cast<Button^>(xaml_markup::XamlReader::Load(xamlString));

                TestServices::WindowHelper->WindowContent = root;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                targetControl = safe_cast<Button^>(TreeHelper::GetVisualChildByName(root, L"magicControl"));

                auto brushCollection = ref new BrushCollection();
                brushCollection->Append(ref new SolidColorBrush());
                brushCollection->Append(ref new SolidColorBrush());

                ControlWithAttachedProperty::SetCustomAttachedBrushCollection(targetControl, brushCollection);

                auto groups = xaml::VisualStateManager::GetVisualStateGroups(safe_cast<FrameworkElement^>(VisualTreeHelper::GetChild(root, 0)));
                VERIFY_IS_TRUE(groups->Size > 0);

                if (!optimized)
                {
                    auto visualStateGroup = groups->GetAt(0);
                }
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void VsmIntegrationTests::ValidateCustomAttachedProperties(Button^ targetControl, String^ stringValue, int intValue, Color brushColor, Color indexedColor1, Color indexedColor2)
        {
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(stringValue == ControlWithAttachedProperty::GetCustomAttached(targetControl), L"ControlWithAttachedProperty.CustomAttached property should be set by the VisualState.");
                VERIFY_ARE_EQUAL(intValue, ControlWithAttachedProperty::GetCustomAttachedInt(targetControl), L"ControlWithAttachedProperty.CustomAttachedInt property should be set by the VisualState.");

                auto attachedBrush = static_cast<SolidColorBrush^>(ControlWithAttachedProperty::GetCustomAttachedBrush(targetControl))->Color;

                // Validating attached Brush changes color with VisualState.
                VERIFY_ARE_EQUAL(brushColor.R, attachedBrush.R);
                VERIFY_ARE_EQUAL(brushColor.G, attachedBrush.G);
                VERIFY_ARE_EQUAL(brushColor.B, attachedBrush.B);
                VERIFY_ARE_EQUAL(brushColor.A, attachedBrush.A);

                auto brushCollection = ControlWithAttachedProperty::GetCustomAttachedBrushCollection(targetControl);

                auto attachedCollectionBrush1 = static_cast<SolidColorBrush^>(brushCollection->GetAt(0))->Color;
                auto attachedCollectionBrush2 = static_cast<SolidColorBrush^>(brushCollection->GetAt(1))->Color;

                // Validating multi-part indexed attached property changes value with VisualState.
                VERIFY_ARE_EQUAL(indexedColor1.R, attachedCollectionBrush1.R);
                VERIFY_ARE_EQUAL(indexedColor1.G, attachedCollectionBrush1.G);
                VERIFY_ARE_EQUAL(indexedColor1.B, attachedCollectionBrush1.B);
                VERIFY_ARE_EQUAL(indexedColor1.A, attachedCollectionBrush1.A);

                VERIFY_ARE_EQUAL(indexedColor2.R, attachedCollectionBrush2.R);
                VERIFY_ARE_EQUAL(indexedColor2.G, attachedCollectionBrush2.G);
                VERIFY_ARE_EQUAL(indexedColor2.B, attachedCollectionBrush2.B);
                VERIFY_ARE_EQUAL(indexedColor2.A, attachedCollectionBrush2.A);
            });
        }

        void VsmIntegrationTests::VerifyVsmSettersSetCustomAttachedProperties(bool optimized)
        {
            TestCleanupWrapper cleanup;

            Button^ root;
            Button^ targetControl;

            PrepareCustomAttachedPropertiesRoot(root, targetControl, optimized);

            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State1", false);
            });

            TestServices::WindowHelper->WaitForIdle();

            ValidateCustomAttachedProperties(targetControl, L"Attached String", 1, Colors::Red, Colors::Blue, Colors::Orange);

            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(root, L"State2", false);
            });

            TestServices::WindowHelper->WaitForIdle();

            ValidateCustomAttachedProperties(targetControl, L"Attached String 2", 2, Colors::Blue, Colors::Yellow, Colors::Green);
        }

        void VsmIntegrationTests::VerifyVsmSettersSetCustomAttachedPropertiesOptimized()
        {
            VerifyVsmSettersSetCustomAttachedProperties(true);
        }

        void VsmIntegrationTests::VerifyVsmSettersSetCustomAttachedPropertiesNonOptimized()
        {
            VerifyVsmSettersSetCustomAttachedProperties(false);
        }
    }
} } } }
