// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "VsmSettersTests.h"
#include <ppltasks.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"
#include <RuntimeEnabledFeatureOverride.h>
#include <StoryboardMonitorWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include <WUCRenderingScopeGuard.h>

using namespace Platform;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Adaptability {

        static const wchar_t genericTemplatedVsm[] =
            L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <Button.Style>"
            L"    <Style TargetType='ButtonBase'>"
            L"      <Setter Target='Template'>"
            L"        <Setter.Value>"
            L"          <ControlTemplate TargetType='ButtonBase'>"
            L"            <StackPanel>"
            L"              <VisualStateManager.VisualStateGroups>"
            L"                <VisualStateGroup x:Name='Group1'>"
            L"                  <VisualStateGroup.Transitions>"
            L"                    <VisualTransition From='VisualState1' To='VisualState2'>"
            L"                      <Storyboard x:Name='VisualTransitionStoryboard'>"
            L"                        <FadeOutThemeAnimation TargetName='Rect1' />"
            L"                      </Storyboard>"
            L"                    </VisualTransition>"
            L"                  </VisualStateGroup.Transitions>"
            L"                  <VisualState x:Name='VisualState1' />"
            L"                  <VisualState x:Name='VisualState2'>"
            L"                    <VisualState.Setters>"
            L"                      <Setter Target='Rect2.(Shape.Fill).(SolidColorBrush.Color)' Value='Green' />"
            L"                      <Setter Target='Rect2.Width' Value='200' />"
            L"                    </VisualState.Setters>"
            L"                    <Storyboard>"
            L"                      <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Rect1'"
            L"                          Storyboard.TargetProperty='Width'>"
            L"                        <DiscreteObjectKeyFrame KeyTime='0' Value='300' />"
            L"                      </ObjectAnimationUsingKeyFrames>"
            L"                    </Storyboard>"
            L"                  </VisualState>"
            L"                  <VisualState x:Name='VisualState3'>"
            L"                    <VisualState.Setters>"
            L"                      <Setter Target='Rect1.Width' Value='150' />"
            L"                      <Setter Target='Rect2.(Shape.Fill).(SolidColorBrush.Color)' Value='Purple' />"
            L"                      <Setter Target='Rect2.(Shape.Fill).(SolidColorBrush.Color)' Value='Yellow' />"
            L"                      <Setter Target='Rect2.Width' Value='300' />"
            L"                    </VisualState.Setters>"
            L"                  </VisualState>"
            L"                </VisualStateGroup>"
            L"              </VisualStateManager.VisualStateGroups>"
            L"              <Rectangle Width='100' Height='100' Fill='Red' x:Name='Rect1' />"
            L"              <Rectangle Width='100' Height='100' Fill='Blue' x:Name='Rect2' />"
            L"            </StackPanel>"
            L"          </ControlTemplate>"
            L"        </Setter.Value>"
            L"      </Setter>"
            L"    </Style>"
            L"  </Button.Style>"
            L"</Button>";

        static const wchar_t genericUserControlVsm[] =
            L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"    <StackPanel>"
            L"        <VisualStateManager.VisualStateGroups>"
            L"            <VisualStateGroup x:Name='Group1'>"
            L"                <VisualStateGroup.Transitions>"
            L"                    <VisualTransition From='VisualState1' To='VisualState2'>"
            L"                        <Storyboard x:Name='VisualTransitionStoryboard'>"
            L"                            <FadeOutThemeAnimation TargetName='Rect1' />"
            L"                        </Storyboard>"
            L"                    </VisualTransition>"
            L"                </VisualStateGroup.Transitions>"
            L"                <VisualState x:Name='VisualState1' />"
            L"                <VisualState x:Name='VisualState2'>"
            L"                    <VisualState.Setters>"
            L"                        <Setter Target='Rect2.(Shape.Fill).(SolidColorBrush.Color)' Value='Green' />"
            L"                        <Setter Target='Rect2.Width' Value='200' />"
            L"                    </VisualState.Setters>"
            L"                    <Storyboard>"
            L"                         <ObjectAnimationUsingKeyFrames Storyboard.TargetName='Rect1'"
            L"                             Storyboard.TargetProperty='Width'>"
            L"                             <DiscreteObjectKeyFrame KeyTime='0' Value='300' />"
            L"                         </ObjectAnimationUsingKeyFrames>"
            L"                    </Storyboard>"
            L"                </VisualState>"
            L"                <VisualState x:Name='VisualState3'>"
            L"                    <VisualState.Setters>"
            L"                        <Setter Target='Rect1.Width' Value='150' />"
            L"                        <Setter Target='Rect2.(Shape.Fill).(SolidColorBrush.Color)' Value='Purple' />"
            L"                        <Setter Target='Rect2.(Shape.Fill).(SolidColorBrush.Color)' Value='Yellow' />"
            L"                        <Setter Target='Rect2.Width' Value='300' />"
            L"                    </VisualState.Setters>"
            L"                </VisualState>"
            L"            </VisualStateGroup>"
            L"        </VisualStateManager.VisualStateGroups>"
            L"        <Rectangle Width='100' Height='100' Fill='Red' x:Name='Rect1' />"
            L"        <Rectangle Width='100' Height='100' Fill='Blue' x:Name='Rect2' />"
            L"    </StackPanel>"
            L"</UserControl>";

        bool VsmSetterIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool VsmSetterIntegrationTests::ClassCleanup()
        {
            return true;
        }

         bool VsmSetterIntegrationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }


        bool VsmSetterIntegrationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void VsmSetterIntegrationTests::VerifyTargetPropertyPathConstructor()
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]() {
                auto widthTPP = ref new xaml::TargetPropertyPath(xaml_controls::Button::WidthProperty);
                auto canvasLeftTPP = ref new xaml::TargetPropertyPath(xaml_controls::Canvas::LeftProperty);

                VERIFY_IS_NULL(widthTPP->Target, L"A TargetPropertyPath constructed with a DependencyProperty should have a null Target.");
                VERIFY_ARE_EQUAL(ref new Platform::String(L"Width"), widthTPP->Path->Path, L"A TargetPropertyPath constructed with a non-attached DP should have a Path that is just the property name.");

                VERIFY_IS_NULL(canvasLeftTPP->Target, L"A TargetPropertyPath constructed with an attached DependencyProperty should have a null Target.");
                VERIFY_ARE_EQUAL(ref new Platform::String(L"(Canvas.Left)"), canvasLeftTPP->Path->Path, L"A TargetPropertyPath constructed with an attached DP should have a Path that is the fully qualified attached DP name.");
            });
        }

        void VsmSetterIntegrationTests::VerifyTargetPropertyPathTypeConverter()
        {
            TestCleanupWrapper cleanup;

            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

            RunOnUIThread([&]() {
                auto root = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"<VisualStateManager.VisualStateGroups>"
                    L"    <VisualStateGroup x:Name='Group1'>"
                    L"        <VisualState x:Name='State1'>"
                    L"            <VisualState.Setters>"
                    L"                <Setter Target='Button1.Arbitrary.Property.Path' Value='Foobar' />"
                    L"            </VisualState.Setters>"
                    L"        </VisualState>"
                    L"    </VisualStateGroup>"
                    L"</VisualStateManager.VisualStateGroups>"
                    L"<Button x:Name='Button1' />"
                    L"<Button x:Name='Button2' />"
                    L"</StackPanel>"
                    ));

                auto state1 = safe_cast<VisualState^>(root->FindName(L"State1"));
                auto targetPropertyPath = (safe_cast<Setter^>(state1->Setters->GetAt(0)))->Target;
                auto button1 = safe_cast<Button^>(root->FindName(L"Button1"));
                auto target = targetPropertyPath->Target;

                VERIFY_IS_TRUE(button1->Equals(target));
                VERIFY_ARE_EQUAL(ref new Platform::String(L"Arbitrary.Property.Path"), targetPropertyPath->Path->Path);
            });
        }

        void VsmSetterIntegrationTests::VerifyTargetPropertyPathTypeConverterExceptions()
        {
            TestCleanupWrapper cleanup;

            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
            DisableErrorReportingScopeGuard disableErrors;

            RunOnUIThread([&]()
            {
                auto Xaml_missingPath = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"<VisualStateManager.VisualStateGroups>"
                    L"    <VisualStateGroup x:Name='Group1'>"
                    L"        <VisualState x:Name='State1'>"
                    L"            <VisualState.Setters>"
                    L"                <Setter Target='Button1' Value='Foobar' />"
                    L"            </VisualState.Setters>"
                    L"        </VisualState>"
                    L"    </VisualStateGroup>"
                    L"</VisualStateManager.VisualStateGroups>"
                    L"<Button x:Name='Button1' />"
                    L"</StackPanel>"));

                auto state1 = safe_cast<VisualState^>(Xaml_missingPath->FindName(L"State1"));
                VERIFY_IS_NOT_NULL(state1);
                VERIFY_THROWS_WINRT((safe_cast<Setter^>(state1->Setters->GetAt(0)))->Target, Platform::COMException^, L"An exception should be thrown if a TargetPropertyPath is missing a path.");
            });

            RunOnUIThread([&]()
            {
                auto Xaml_emptyPath = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"<VisualStateManager.VisualStateGroups>"
                    L"    <VisualStateGroup x:Name='Group1'>"
                    L"        <VisualState x:Name='State1'>"
                    L"            <VisualState.Setters>"
                    L"                <Setter Target='Button1.' Value='Foobar' />"
                    L"            </VisualState.Setters>"
                    L"        </VisualState>"
                    L"    </VisualStateGroup>"
                    L"</VisualStateManager.VisualStateGroups>"
                    L"<Button x:Name='Button1' />"
                    L"</StackPanel>"));
                auto state1 = safe_cast<VisualState^>(Xaml_emptyPath->FindName(L"State1"));
                VERIFY_IS_NOT_NULL(state1);
                VERIFY_THROWS_WINRT((safe_cast<Setter^>(state1->Setters->GetAt(0)))->Target, Platform::COMException^, L"An exception should be thrown if a TargetPropertyPath has an empty path.");
            });

            RunOnUIThread([&]()
            {
                auto Xaml_emptyTarget = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"<VisualStateManager.VisualStateGroups>"
                    L"    <VisualStateGroup x:Name='Group1'>"
                    L"        <VisualState x:Name='State1'>"
                    L"            <VisualState.Setters>"
                    L"                <Setter Target='.Arbitrary.Property.Path' Value='Foobar' />"
                    L"            </VisualState.Setters>"
                    L"        </VisualState>"
                    L"    </VisualStateGroup>"
                    L"</VisualStateManager.VisualStateGroups>"
                    L"<Button x:Name='Button1' />"
                    L"</StackPanel>"));
                auto state1 = safe_cast<VisualState^>(Xaml_emptyTarget->FindName(L"State1"));
                VERIFY_IS_NOT_NULL(state1);
                VERIFY_THROWS_WINRT((safe_cast<Setter^>(state1->Setters->GetAt(0)))->Target, Platform::COMException^, L"An exception should be thrown if a TargetPropertyPath has an empty target name.");
            });
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSettersExceptions()
        {
            TestCleanupWrapper cleanup;

            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
            DisableErrorReportingScopeGuard disableErrors;

            // Check SetterBaseCollection's validation when adding a Setter (can't have both Property and Target set)
            RunOnUIThread([&]()
            {
                xaml::TargetPropertyPath^ targetPropertyPath = ref new xaml::TargetPropertyPath();
                targetPropertyPath->Target = ref new xaml_controls::Button();
                targetPropertyPath->Path = ref new xaml::PropertyPath(Platform::StringReference(L"(Shape.Fill).(SolidColorBrush.Color)"));
                xaml::Setter^ setter = ref new xaml::Setter();
                setter->Property = xaml_controls::Button::WidthProperty;
                setter->Target = targetPropertyPath;
                xaml::SetterBaseCollection^ setterCollection = ref new xaml::SetterBaseCollection();

                VERIFY_THROWS_WINRT(setterCollection->Append(setter), Platform::COMException^, L"An exception should be thrown if we try to use a setter that has both Property and Target set.");
            });

            // Validate various forms of invalid Setters inside VisualStates
            RunOnUIThread([&]()
            {
                auto Xaml_vsmSetterMissingTarget = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"<VisualStateManager.VisualStateGroups>"
                    L"    <VisualStateGroup x:Name='Group1'>"
                    L"        <VisualState x:Name='State1'>"
                    L"            <VisualState.Setters>"
                    L"                <Setter Value='Green' />"
                    L"            </VisualState.Setters>"
                    L"        </VisualState>"
                    L"    </VisualStateGroup>"
                    L"</VisualStateManager.VisualStateGroups>"
                    L"<Button x:Name='Button1' />"
                    L"</StackPanel>"));
                auto state1 = safe_cast<VisualState^>(Xaml_vsmSetterMissingTarget->FindName(L"State1"));
                VERIFY_IS_NOT_NULL(state1);
                VERIFY_THROWS_WINRT((safe_cast<Setter^>(state1->Setters->GetAt(0)))->Target, Platform::COMException^, L"An exception should be thrown if a VSM Setter has a missing target.");
            });

            // Exceptions are only thrown if debugger is attached
            if (IsDebuggerPresent())
            {
                xaml_controls::Control^ root = nullptr;
                RunOnUIThread([&]() {
                    root = safe_cast<xaml_controls::Control^> (xaml_markup::XamlReader::Load(
                        L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                        L"    <Border x:Name='MyBorder' Background='Pink'>"
                        L"        <VisualStateManager.VisualStateGroups>"
                        L"            <VisualStateGroup x:Name='Group1'>"
                        L"                <VisualState x:Name='BogusTarget'>"
                        L"                    <VisualState.Setters>"
                        L"                        <Setter Target='Button2.Width' Value='300' />"
                        L"                    </VisualState.Setters>"
                        L"                </VisualState>"
                        L"                <VisualState x:Name='BogusPath'>"
                        L"                    <VisualState.Setters>"
                        L"                        <Setter Target='Button1.Foobar' Value='300' />"
                        L"                    </VisualState.Setters>"
                        L"                </VisualState>"
                        L"            </VisualStateGroup>"
                        L"        </VisualStateManager.VisualStateGroups>"
                        L"        <Button x:Name='Button1' />"
                        L"    </Border>"
                        L"</UserControl>"
                        ));
                    TestServices::WindowHelper->WindowContent = root;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VERIFY_THROWS_WINRT(VisualStateManager::GoToState(root, Platform::StringReference(L"BogusTarget"), true), Platform::COMException^, L"An exception should be thrown if a VSM setter has an unresolvable target.");
                    VERIFY_THROWS_WINRT(VisualStateManager::GoToState(root, Platform::StringReference(L"BogusPath"), true), Platform::COMException^, L"An exception should be thrown if a VSM setter has an unresolvable path.");
                });
                TestServices::WindowHelper->WaitForIdle();
            }
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSettersDComp()
        {
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 400));
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream;
            featureEnforceXbfV2Stream.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

            xaml::UIElement^ root = nullptr;
            RunOnUIThread([&]()
            {
                // Load the XAML and set it as the window content
                const wchar_t rootXaml[] =
                    L"<Grid xmlns = 'http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Grid.RowDefinitions>"
                    L"      <RowDefinition Height='Auto' />"
                    L"      <RowDefinition Height='*' />"
                    L"  </Grid.RowDefinitions>"
                    L"  <StackPanel x:Name='m_contentStackPanel' Grid.Row='0' Orientation='Horizontal' />"
                    L"</Grid>";

                root = safe_cast<xaml::UIElement^>(xaml_markup::XamlReader::Load(Platform::StringReference(rootXaml)));

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::Control^ userControl = nullptr;
            xaml_controls::Control^ templatedControl = nullptr;
            RunOnUIThread([&]()
            {
                xaml_controls::StackPanel^ stackPanel = nullptr;
                stackPanel = safe_cast<xaml_controls::StackPanel^>(VisualTreeHelper::GetChild(root, 0));

                // Save away various parts of the UI
                userControl = safe_cast<xaml_controls::Control^>(xaml_markup::XamlReader::Load(Platform::StringReference(genericUserControlVsm)));
                templatedControl = safe_cast<xaml_controls::Control^>(xaml_markup::XamlReader::Load(Platform::StringReference(genericTemplatedVsm)));

                stackPanel->Children->Append(userControl);
                stackPanel->Children->Append(templatedControl);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Before");

            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(userControl, Platform::StringReference(L"VisualState2"), false);
                VisualStateManager::GoToState(templatedControl, Platform::StringReference(L"VisualState2"), false);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "After");
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSetterCanUseXNull()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Control^ root = nullptr;
            xaml_controls::Border^ border = nullptr;

            RunOnUIThread([&]() {
                root = safe_cast<xaml_controls::Control^> (xaml_markup::XamlReader::Load(
                    L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Border x:Name='MyBorder' Background='Pink'>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"            <VisualStateGroup x:Name='Group1'>"
                    L"                <VisualState x:Name='State1'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='MyBorder.Background' Value='{x:Null}' />"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"            </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"        <Button x:Name='Button1' />"
                    L"    </Border>"
                    L"</UserControl>"
                    ));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                border = safe_cast<xaml_controls::Border^>(root->FindName(L"MyBorder"));

                ::Windows::UI::Color bgColor = safe_cast<xaml_media::SolidColorBrush^>(border->Background)->Color;
                VERIFY_ARE_EQUAL(bgColor.A, Microsoft::UI::Colors::Pink.A);
                VERIFY_ARE_EQUAL(bgColor.R, Microsoft::UI::Colors::Pink.R);
                VERIFY_ARE_EQUAL(bgColor.G, Microsoft::UI::Colors::Pink.G);
                VERIFY_ARE_EQUAL(bgColor.B, Microsoft::UI::Colors::Pink.B);
                VisualStateManager::GoToState(root, Platform::StringReference(L"State1"), true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                VERIFY_IS_NULL(border->Background);
            });
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSettersCanHaveUnsetValue()
        {
            TestCleanupWrapper cleanup;

            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
            DisableErrorReportingScopeGuard disableErrors;

            xaml_controls::Control^ root = nullptr;
            RunOnUIThread([&]() {
                root = safe_cast<xaml_controls::Control^> (xaml_markup::XamlReader::Load(
                    L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Border x:Name='MyBorder' Background='Pink'>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"            <VisualStateGroup x:Name='Group1'>"
                    L"                <VisualState x:Name='UnsetValue'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='MyBorder.Background' />"
                    L"                        <Setter Target='Button1.Background' Value='Purple' />"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"            </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"        <Button x:Name='Button1' />"
                    L"    </Border>"
                    L"</UserControl>"
                    ));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_NO_THROW(VisualStateManager::GoToState(root, Platform::StringReference(L"UnsetValue"), true)); // An exception should not be thrown if a VSM setter has an unset value.
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                xaml_controls::Border^ border = safe_cast<xaml_controls::Border^>(root->FindName(L"MyBorder"));

                ::Windows::UI::Color borderBgColor = safe_cast<xaml_media::SolidColorBrush^>(border->Background)->Color;
                VERIFY_ARE_EQUAL(borderBgColor.A, Microsoft::UI::Colors::Pink.A);
                VERIFY_ARE_EQUAL(borderBgColor.R, Microsoft::UI::Colors::Pink.R);
                VERIFY_ARE_EQUAL(borderBgColor.G, Microsoft::UI::Colors::Pink.G);
                VERIFY_ARE_EQUAL(borderBgColor.B, Microsoft::UI::Colors::Pink.B);

                xaml_controls::Button^ button = safe_cast<xaml_controls::Button^>(root->FindName(L"Button1"));
                ::Windows::UI::Color buttonBgColor = safe_cast<xaml_media::SolidColorBrush^>(button->Background)->Color;
                VERIFY_ARE_EQUAL(buttonBgColor.A, Microsoft::UI::Colors::Purple.A);
                VERIFY_ARE_EQUAL(buttonBgColor.R, Microsoft::UI::Colors::Purple.R);
                VERIFY_ARE_EQUAL(buttonBgColor.G, Microsoft::UI::Colors::Purple.G);
                VERIFY_ARE_EQUAL(buttonBgColor.B, Microsoft::UI::Colors::Purple.B);
            });
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSetterDoesNotIncorrectlyClearValueDuringTransition()
        {
            xaml_controls::Control^ root = nullptr;
            xaml_controls::Border^ border = nullptr;
            int propertyChangedCount = 0;
            long long eventToken = 0;
            TestCleanupWrapper cleanup([&]()
            {
                border->UnregisterPropertyChangedCallback(xaml_controls::Border::BackgroundProperty, eventToken);
            });


            RunOnUIThread([&]() {
                root = safe_cast<xaml_controls::Control^> (xaml_markup::XamlReader::Load(
                    L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Border x:Name='MyBorder' Background='Pink' Width='200' Height='200'>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"            <VisualStateGroup x:Name='Group1'>"
                    L"                <VisualStateGroup.Transitions>"
                    L"                    <VisualTransition GeneratedDuration = '0:0:3' />"
                    L"                </VisualStateGroup.Transitions>"
                    L"                <VisualState x:Name='State1'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='MyBorder.Background' Value='Green' />"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"                <VisualState x:Name='State2'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='MyBorder.Background' Value='Blue' />"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"            </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"    </Border>"
                    L"</UserControl>"
                    ));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                border = safe_cast<xaml_controls::Border^>(root->FindName(L"MyBorder"));
                eventToken = border->RegisterPropertyChangedCallback(
                    xaml_controls::Border::BackgroundProperty,
                    ref new DependencyPropertyChangedCallback([&](DependencyObject^ /*sender*/, DependencyProperty^ /*prop*/)
                    {
                        propertyChangedCount++;
                    }));

                VisualStateManager::GoToState(root, Platform::StringReference(L"State1"), true);
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(1, propertyChangedCount);

            RunOnUIThread([&]() {
                VisualStateManager::GoToState(root, Platform::StringReference(L"State2"), true);
            });
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(2, propertyChangedCount);
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSetterCanUseThemeResource()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Control^ root = nullptr;
            xaml_controls::Grid^ grid = nullptr;
            const ::Windows::UI::Color buttonPressedBGColorLight = Microsoft::UI::Colors::Red;
            const ::Windows::UI::Color buttonPressedBGColorDark = Microsoft::UI::Colors::Blue;

            RunOnUIThread([&]() {
                root = safe_cast<xaml_controls::Control^> (xaml_markup::XamlReader::Load(
                    L"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button.Resources>"
                    L"    <ResourceDictionary>"
                    L"      <ResourceDictionary.ThemeDictionaries>"
                    L"        <ResourceDictionary x:Key='Light'>"
                    L"          <SolidColorBrush x:Key='ButtonThemeBackground' Color='Pink' />"
                    L"          <SolidColorBrush x:Key='ButtonPressedBackground' Color='Red' />"
                    L"        </ResourceDictionary>"
                    L"        <ResourceDictionary x:Key='Dark'>"
                    L"          <SolidColorBrush x:Key='ButtonThemeBackground' Color='Purple' />"
                    L"          <SolidColorBrush x:Key='ButtonPressedBackground' Color='Blue' />"
                    L"        </ResourceDictionary>"
                    L"      </ResourceDictionary.ThemeDictionaries>"
                    L"    </ResourceDictionary>"
                    L"  </Button.Resources>"
                    L"  <Button.Style>"
                    L"    <Style TargetType='ButtonBase'>"
                    L"      <Setter Target='Background' Value='{ThemeResource ButtonThemeBackground}' />"
                    L"      <Setter Target='Template'>"
                    L"        <Setter.Value>"
                    L"          <ControlTemplate TargetType='ButtonBase'>"
                    L"            <Grid x:Name='RootGrid' Background='{TemplateBinding Background}' Width='100' Height='50'>"
                    L"              <VisualStateManager.VisualStateGroups>"
                    L"                <VisualStateGroup x:Name='Group1'>"
                    L"                  <VisualState x:Name='Normal' />"
                    L"                  <VisualState x:Name='Pressed'>"
                    L"                    <VisualState.Setters>"
                    L"                      <Setter Target='RootGrid.Background' Value='{ThemeResource ButtonPressedBackground}' />"
                    L"                    </VisualState.Setters>"
                    L"                  </VisualState>"
                    L"                </VisualStateGroup>"
                    L"              </VisualStateManager.VisualStateGroups>"
                    L"            </Grid>"
                    L"          </ControlTemplate>"
                    L"        </Setter.Value>"
                    L"      </Setter>"
                    L"    </Style>"
                    L"  </Button.Style>"
                    L"</Button>"
                    ));
                TestServices::WindowHelper->WindowContent = root;
                root->RequestedTheme = xaml::ElementTheme::Light;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                VisualStateManager::GoToState(root, Platform::StringReference(L"Pressed"), true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                grid = safe_cast<xaml_controls::Grid^>(VisualTreeHelper::GetChild(root, 0));

                ::Windows::UI::Color bgColor = safe_cast<xaml_media::SolidColorBrush^>(grid->Background)->Color;
                // Verify that value matches 'Light' theme
                VERIFY_ARE_EQUAL(bgColor.A, buttonPressedBGColorLight.A);
                VERIFY_ARE_EQUAL(bgColor.R, buttonPressedBGColorLight.R);
                VERIFY_ARE_EQUAL(bgColor.G, buttonPressedBGColorLight.G);
                VERIFY_ARE_EQUAL(bgColor.B, buttonPressedBGColorLight.B);

                // Change theme to 'Dark'
                root->RequestedTheme = xaml::ElementTheme::Dark;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                ::Windows::UI::Color bgColor = safe_cast<xaml_media::SolidColorBrush^>(grid->Background)->Color;
                // Verify that value matches 'Dark' theme
                VERIFY_ARE_EQUAL(bgColor.A, buttonPressedBGColorDark.A);
                VERIFY_ARE_EQUAL(bgColor.R, buttonPressedBGColorDark.R);
                VERIFY_ARE_EQUAL(bgColor.G, buttonPressedBGColorDark.G);
                VERIFY_ARE_EQUAL(bgColor.B, buttonPressedBGColorDark.B);
            });
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSetterCanReevaluateBinding()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Control^ root = nullptr;
            xaml_controls::Button^ widthControl = nullptr;
            xaml_controls::Button^ button1 = nullptr;
            xaml_controls::Button^ button2 = nullptr;

            RunOnUIThread([&]() {
                root = safe_cast<xaml_controls::Control^> (xaml_markup::XamlReader::Load(
                    L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <StackPanel>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"            <VisualStateGroup x:Name='Group1'>"
                    L"                <VisualState x:Name='State1'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='Button1.Width' Value='{Binding Width, ElementName=WidthControl}' />"
                    L"                        <Setter Target='Button2.Width' Value='{Binding Width, ElementName=WidthControl}' />"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"            </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"        <Button x:Name='WidthControl' Width='100' />"
                    L"        <Button x:Name='Button1' Width='50' />"
                    L"        <Button x:Name='Button2' Width='75' />"
                    L"    </StackPanel>"
                    L"</UserControl>"
                    ));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                widthControl = safe_cast<xaml_controls::Button^>(root->FindName(L"WidthControl"));
                button1 = safe_cast<xaml_controls::Button^>(root->FindName(L"Button1"));
                button2 = safe_cast<xaml_controls::Button^>(root->FindName(L"Button2"));

                VERIFY_ARE_EQUAL(100.0, widthControl->Width);
                VERIFY_ARE_EQUAL(50.0, button1->Width);
                VERIFY_ARE_EQUAL(75.0, button2->Width);
                VisualStateManager::GoToState(root, Platform::StringReference(L"State1"), true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                VERIFY_ARE_EQUAL(100.0, widthControl->Width);
                VERIFY_ARE_EQUAL(100.0, button1->Width);
                VERIFY_ARE_EQUAL(100.0, button2->Width);

                widthControl->Width = 200.0;
                VisualStateManager::GoToState(root, Platform::StringReference(L"State1"), false);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                VERIFY_ARE_EQUAL(200.0, widthControl->Width);
                VERIFY_ARE_EQUAL(200.0, button1->Width, L"Calling GoToState(useTransitions=false) should have caused the VSM Setter to reevaluate bindings.");
                VERIFY_ARE_EQUAL(200.0, button2->Width, L"Calling GoToState(useTransitions=false) should have caused the VSM Setter to reevaluate bindings.");
            });
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSettersInVsm_WithTransitions_Deferred_Templated()
        {
            TestCleanupWrapper cleanup;

            VerifyVisualStateSettersInVsm_Helper(
                true,
                false,
                true,
                InitializeExpectedVisualStateLayouts(true));
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSettersInVsm_WithoutTransitions_Deferred_Templated()
        {
            TestCleanupWrapper cleanup;

            VerifyVisualStateSettersInVsm_Helper(
                true,
                false,
                false,
                InitializeExpectedVisualStateLayouts(false));
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSettersInVsm_WithTransitions_Legacy_Templated()
        {
            TestCleanupWrapper cleanup;

            VerifyVisualStateSettersInVsm_Helper(
                true,
                true,
                true,
                InitializeExpectedVisualStateLayouts(true));
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSettersInVsm_WithoutTransitions_Legacy_Templated()
        {
            TestCleanupWrapper cleanup;

            VerifyVisualStateSettersInVsm_Helper(
                true,
                true,
                false,
                InitializeExpectedVisualStateLayouts(false));
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSettersInVsm_WithTransitions_Deferred()
        {
            TestCleanupWrapper cleanup;

            VerifyVisualStateSettersInVsm_Helper(
                false,
                false,
                true,
                InitializeExpectedVisualStateLayouts(true));
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSettersInVsm_WithoutTransitions_Deferred()
        {
            TestCleanupWrapper cleanup;

            VerifyVisualStateSettersInVsm_Helper(
                false,
                false,
                false,
                InitializeExpectedVisualStateLayouts(false));
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSettersInVsm_WithTransitions_Legacy()
        {
            TestCleanupWrapper cleanup;

            VerifyVisualStateSettersInVsm_Helper(
                false,
                true,
                true,
                InitializeExpectedVisualStateLayouts(true));
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSettersInVsm_WithoutTransitions_Legacy()
        {
            TestCleanupWrapper cleanup;

            VerifyVisualStateSettersInVsm_Helper(
                false,
                true,
                false,
                InitializeExpectedVisualStateLayouts(false));
        }

        static const wchar_t genericUserControlVsmBindings[] =
            L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
            L"    <StackPanel x:Name = 'StackPanel1'>\r\n"
            L"        <VisualStateManager.VisualStateGroups>\r\n"
            L"            <VisualStateGroup x:Name='Group1'>\r\n"
            L"                <VisualStateGroup.Transitions>\r\n"
            L"                    <VisualTransition From='VisualState1' To='VisualState2'>\r\n"
            L"                        <Storyboard x:Name='VisualTransitionStoryboard'>\r\n"
            L"                            <FadeOutThemeAnimation TargetName='Rect1' />\r\n"
            L"                        </Storyboard>\r\n"
            L"                    </VisualTransition>\r\n"
            L"                </VisualStateGroup.Transitions>\r\n"
            L"                <VisualState x:Name='VisualState1'/>\r\n"
            L"                <VisualState x:Name='VisualState2'>\r\n"
            L"                    <VisualState.Setters>\r\n"
            L"                        <Setter Target='Rect2.Width' Value='{Binding ElementName=StackPanel1, Path=ActualWidth}' />\r\n"
            L"                    </VisualState.Setters>\r\n"
            L"                </VisualState>\r\n"
            L"                <VisualState x:Name='VisualState3'>\r\n"
            L"                    <VisualState.Setters>\r\n"
            L"                        <Setter Target='Rect2.Width' Value='{Binding ElementName=StackPanel1, Path=ActualHeight}' />\r\n"
            L"                    </VisualState.Setters>\r\n"
            L"                </VisualState>\r\n"
            L"            </VisualStateGroup>\r\n"
            L"        </VisualStateManager.VisualStateGroups>\r\n"
            L"        <Rectangle Width='100' Height='100' Fill='Red' x:Name='Rect1' />\r\n"
            L"        <Rectangle Width='10' Height='10' Fill='Blue' x:Name='Rect2' />\r\n"
            L"    </StackPanel>\r\n"
            L"</UserControl>";

        void VsmSetterIntegrationTests::VerifyVisualStateSettersBinding()
        {
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 400));
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            xaml::Controls::Control^ root = nullptr;
            xaml_controls::StackPanel^ stackPanel = nullptr;
            xaml_shapes::Rectangle^ rect1 = nullptr;
            xaml_shapes::Rectangle^ rect2 = nullptr;

            RunOnUIThread([&]()
            {
                root = safe_cast<xaml_controls::Control^>(xaml_markup::XamlReader::Load(Platform::StringReference(genericUserControlVsmBindings)));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            for(int i = 0; i < 3; ++i)
            {
                RunOnUIThread([&]()
                {
                    stackPanel = safe_cast<xaml_controls::StackPanel^>(VisualTreeHelper::GetChild(root, 0));
                    rect1 = safe_cast<xaml_shapes::Rectangle^>(stackPanel->Children->GetAt(0));
                    rect2 = safe_cast<xaml_shapes::Rectangle^>(stackPanel->Children->GetAt(1));

                    VisualStateManager::GoToState(root, L"VisualState2", true);
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(600, rect2->ActualWidth);
                });

                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(root, L"VisualState3", true);
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(400, rect2->ActualWidth);
                });
            }
        }

        void VsmSetterIntegrationTests::VerifyAsyncSetterDoesnotClobberLaterSyncSetter()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::Control^ root = nullptr;
            xaml_controls::Border^ border = nullptr;

            RunOnUIThread([&]()
            {
                root = safe_cast<xaml_controls::Control^> (xaml_markup::XamlReader::Load(
                    L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Border x:Name='MyBorder' Background='Pink'>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"            <VisualStateGroup x:Name='Group1'>"
                    L"                <VisualState x:Name='State1'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='MyBorder.Background' Value='Red' />"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"                <VisualState x:Name='State2'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='MyBorder.Background' Value='Blue' />"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"            </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"        <Button x:Name='Button1' />"
                    L"    </Border>"
                    L"</UserControl>"
                ));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                border = safe_cast<xaml_controls::Border^>(root->FindName(L"MyBorder"));

                VisualStateManager::GoToState(root, Platform::StringReference(L"State1"), true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // The property change is asynchronous because we are already in State1 and useTransitions = false
                // Let's queue up a bunch because why not
                VisualStateManager::GoToState(root, Platform::StringReference(L"State1"), false);
                VisualStateManager::GoToState(root, Platform::StringReference(L"State1"), false);
                VisualStateManager::GoToState(root, Platform::StringReference(L"State1"), false);
                VisualStateManager::GoToState(root, Platform::StringReference(L"State1"), false);
                VisualStateManager::GoToState(root, Platform::StringReference(L"State1"), false);
                // The property change is synchronous because useTransitions = true, going to a different state,
                // and it's a built-in property
                // As such, this GoToState call will clear out the previous pending asynchronous modifications,
                // leaving the Border's background blue
                VisualStateManager::GoToState(root, Platform::StringReference(L"State2"), true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                ::Windows::UI::Color bgColor = safe_cast<xaml_media::SolidColorBrush^>(border->Background)->Color;
                VERIFY_ARE_EQUAL(bgColor.A, Microsoft::UI::Colors::Blue.A);
                VERIFY_ARE_EQUAL(bgColor.R, Microsoft::UI::Colors::Blue.R);
                VERIFY_ARE_EQUAL(bgColor.G, Microsoft::UI::Colors::Blue.G);
                VERIFY_ARE_EQUAL(bgColor.B, Microsoft::UI::Colors::Blue.B);
            });
        }

        void VsmSetterIntegrationTests::VerifyVsmSettersCanApplyStyle()
        {
            TestCleanupWrapper cleanup;
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream;
            featureEnforceXbfV2Stream.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
            xaml_controls::Control^ root = nullptr;

            RunOnUIThread([&]()
            {
                root = safe_cast<xaml_controls::Control^> (xaml_markup::XamlReader::Load(
                    L"<UserControl xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"    <Border x:Name='MyBorder' Background='Pink'>"
                    L"        <VisualStateManager.VisualStateGroups>"
                    L"            <VisualStateGroup x:Name='Group1'>"
                    L"                <VisualState x:Name='State1'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='MyBorder.Style'>"
                    L"                          <Setter.Value>"
                    L"                            <Style TargetType='Border'>"
                    L"                              <Setter Property='Background' Value='Red' />"
                    L"                            </Style>"
                    L"                          </Setter.Value>"
                    L"                        </Setter>"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"                <VisualState x:Name='State2'>"
                    L"                    <VisualState.Setters>"
                    L"                        <Setter Target='MyBorder.Background' Value='Blue' />"
                    L"                    </VisualState.Setters>"
                    L"                </VisualState>"
                    L"            </VisualStateGroup>"
                    L"        </VisualStateManager.VisualStateGroups>"
                    L"        <Button x:Name='Button1' />"
                    L"    </Border>"
                    L"</UserControl>"
                ));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            // If we got here, that means no exception, so we're all good!
        }

        void VsmSetterIntegrationTests::VerifyVisualStateSettersInVsm_Helper(
            bool useTemplatedVsm,
            bool usePessimalCodePath,
            bool useTransitions,
            const std::vector<ExpectedLayoutSpec>& expectedVisualStateLayouts)
        {
            RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream;
            if (usePessimalCodePath)
            {
                featureEnforceXbfV2Stream.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
            }
            else
            {
                featureEnforceXbfV2Stream.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
            }

            auto storyboardMonitor = ref new StoryboardMonitorWrapper();

            bool sawFadeoutAnimation = false;
            storyboardMonitor->AttachStartedHandler(
                [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
            {
                if (storyboard->Children->Size > 0 &&
                    storyboard->Children->GetAt(0)->GetType()->FullName == L"Microsoft.UI.Xaml.Media.Animation.FadeOutThemeAnimation")
                {
                    sawFadeoutAnimation = true;
                    LOG_OUTPUT(L"Saw a fadeout animation.");
                }
            });

            xaml_controls::Control^ root = nullptr;
            RunOnUIThread([&]()
            {
                // Load the XAML and set it as the window content
                if (useTemplatedVsm)
                {
                    root = safe_cast<xaml_controls::Control^>(xaml_markup::XamlReader::Load(Platform::StringReference(genericTemplatedVsm)));
                }
                else
                {
                    root = safe_cast<xaml_controls::Control^>(xaml_markup::XamlReader::Load(Platform::StringReference(genericUserControlVsm)));
                }

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            xaml_shapes::Rectangle^ rect1 = nullptr;
            xaml_shapes::Rectangle^ rect2 = nullptr;
            xaml::DependencyObject^ rect1Parent = nullptr;
            xaml::DependencyObject^ rect2Parent = nullptr;
            RunOnUIThread([&]()
            {
                xaml_controls::StackPanel^ stackPanel = nullptr;
                stackPanel = safe_cast<xaml_controls::StackPanel^>(VisualTreeHelper::GetChild(root, 0));

                // Save away various parts of the UI
                rect1 = safe_cast<xaml_shapes::Rectangle^>(stackPanel->Children->GetAt(0));
                rect2 = safe_cast<xaml_shapes::Rectangle^>(stackPanel->Children->GetAt(1));
                rect1Parent = rect1->Parent;
                rect2Parent = rect2->Parent;
            });

#pragma region Begin actual test loop

            for (const auto stateDescription : expectedVisualStateLayouts)
            {
                // Transition to the specified visual state
                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(root, std::get<0>(stateDescription), useTransitions);
                });
                TestServices::WindowHelper->WaitForIdle();

                // Verify that the current layout matches our expectations
                RunOnUIThread([&]()
                {
                    // This verifies that the VisualState's Storyboard was applied
                    VERIFY_ARE_EQUAL(std::get<2>(stateDescription), rect1->Width);

                    // This verifies that the VisualState's Setters were applied
                    auto rect2FillColor = (((xaml_media::SolidColorBrush^)rect2->Fill))->Color;
                    VERIFY_ARE_EQUAL(std::get<3>(stateDescription), rect2->Width);
                    VERIFY_ARE_EQUAL(std::get<4>(stateDescription).A, rect2FillColor.A);
                    VERIFY_ARE_EQUAL(std::get<4>(stateDescription).R, rect2FillColor.R);
                    VERIFY_ARE_EQUAL(std::get<4>(stateDescription).G, rect2FillColor.G);
                    VERIFY_ARE_EQUAL(std::get<4>(stateDescription).B, rect2FillColor.B);
                    VERIFY_ARE_EQUAL(std::get<1>(stateDescription), sawFadeoutAnimation);
                    sawFadeoutAnimation = false;

                    // Test for regression of fix by verifying that applying a setter doesn't cause the target's Parent to change
                    VERIFY_IS_TRUE(rect1Parent->Equals(rect1->Parent), L"Applying a setter shouldn't cause the target's parent to change");
                    VERIFY_IS_TRUE(rect2Parent->Equals(rect2->Parent), L"Applying a setter shouldn't cause the target's parent to change");
                });
            }

#pragma endregion
        }

        const std::vector<ExpectedLayoutSpec> VsmSetterIntegrationTests::InitializeExpectedVisualStateLayouts(bool usingTransitions)
        {
            // State transition sequence is: VisualState1 -> VisualState2 -> VisualState1 -> VisualState2 -> VisualState3
            // Only VisualState1 -> VisualState2 has a VisualTransition animation
            std::vector<ExpectedLayoutSpec> expectedVisualStateLayouts;
            expectedVisualStateLayouts.push_back(std::make_tuple(Platform::StringReference(L"VisualState1"), false && usingTransitions, 100, 100, Microsoft::UI::Colors::Blue));
            expectedVisualStateLayouts.push_back(std::make_tuple(Platform::StringReference(L"VisualState2"), true && usingTransitions, 300, 200, Microsoft::UI::Colors::Green));
            expectedVisualStateLayouts.push_back(std::make_tuple(Platform::StringReference(L"VisualState1"), false && usingTransitions, 100, 100, Microsoft::UI::Colors::Blue));
            expectedVisualStateLayouts.push_back(std::make_tuple(Platform::StringReference(L"VisualState2"), true && usingTransitions, 300, 200, Microsoft::UI::Colors::Green));
            expectedVisualStateLayouts.push_back(std::make_tuple(Platform::StringReference(L"VisualState3"), false && usingTransitions, 150, 300, Microsoft::UI::Colors::Yellow));
            expectedVisualStateLayouts.push_back(std::make_tuple(Platform::StringReference(L"VisualState2"), false && usingTransitions, 300, 200, Microsoft::UI::Colors::Green));

            return expectedVisualStateLayouts;
        }
    }
} } } }
