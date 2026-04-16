// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "AnimatedValueTests.h"
#include "XamlDiagnosticsTestHelpers.h"
#include <CustomUserControl.h>
#include "CustomTypes.XamlTypeInfo.g.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"
#include <TestEvent.h>
#include "MainPage.xaml.h"
#include <CustomMetadataRegistrar.h>
#include "RuleTesterHelper.h"
#include "SafeEventRegistration.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Data;
using namespace ::Tests::Tools::XamlDiagnostics;
using namespace test_infra;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;

namespace shared_types = ::Tests::Tools::Shared;
#define HNS_FROM_SECOND(x) ((x)*10 * 1000 * 1000)

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {

        #pragma region Test Methods

        bool AnimatedValueTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            featureDisableTransitionsForTest.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTransitionsForTest, true);

            return true;
        }

        bool AnimatedValueTests::ClassCleanup()
        {
            return true;
        }

        bool AnimatedValueTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<shared_types::CustomUserControl>());

            return EnsureTapLoaded();
        }

        bool AnimatedValueTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            test_infra::TestServices::WindowHelper->VerifyTestCleanup();

            return true;
        }


        void AnimatedValueTests::TestAnimatedTriggersBase(bool forceXbf)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;

            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, forceXbf);
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::visualStatesAndTriggersString, callback);

            TestServices::WindowHelper->WaitForIdle();

            auto triggerGrid = callback->GetElementByName(L"visualStateTriggerTest");

            //First, verify the Width/Height animated values have the correct source info.
            //Height should go back to the DoubleAnimation in the Storyboard
            wil::unique_propertychainsource heightAnimatedSource = m_tap->GetSourceForProperty(triggerGrid.Handle, L"Height", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(heightAnimatedSource.SrcInfo.LineNumber, 17u);

            //Width should go back to the Setter in the VisualState's setters.
            wil::unique_propertychainsource widthAnimatedSource = m_tap->GetSourceForProperty(triggerGrid.Handle, L"Width", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(widthAnimatedSource.SrcInfo.LineNumber, 24u);

            //Verify we can still get the locally set Width/Height values, and their sources info is intact.
            wil::unique_propertychainsource heightLocalSource = m_tap->GetSourceForProperty(triggerGrid.Handle, L"Height", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(heightLocalSource.SrcInfo.LineNumber, 6u);

            wil::unique_propertychainsource widthLocalSource = m_tap->GetSourceForProperty(triggerGrid.Handle, L"Width", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(widthLocalSource.SrcInfo.LineNumber, 6u);

            //Lastly, make sure the local/animated values are correct.
            wil::unique_propertychainvalue heightAnimatedValue = GetPropertyChainValue(triggerGrid.Handle, L"Height", BaseValueSource::Animation);

            //Animated value is somewhere between 110-300, but not 100
            VERIFY_IS_GREATER_THAN(std::stoi(heightAnimatedValue.Value), 109);
            VERIFY_IS_LESS_THAN(std::stoi(heightAnimatedValue.Value), 301);

            wil::unique_propertychainvalue widthAnimatedValue = GetPropertyChainValue(triggerGrid.Handle, L"Width", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(std::stoi(widthAnimatedValue.Value), 300);

            wil::unique_propertychainvalue heightLocalValue = GetPropertyChainValue(triggerGrid.Handle, L"Height", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(std::stoi(heightLocalValue.Value), 100);

            wil::unique_propertychainvalue widthLocalValue = GetPropertyChainValue(triggerGrid.Handle, L"Width", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(std::stoi(widthLocalValue.Value), 100);
        }

        void AnimatedValueTests::TestAnimatedTriggers()
        {
            TestAnimatedTriggersBase(false);
        }

        void AnimatedValueTests::TestAnimatedTriggersXBF()
        {
            TestAnimatedTriggersBase(true);
        }

        void AnimatedValueTests::TestAnimatedValuesManualBase(bool forceXbf)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;

            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, forceXbf);
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::visualStatesAndTriggersString, callback);

            TestServices::WindowHelper->WaitForIdle();

            auto manualGrid = callback->GetElementByName(L"visualStateManualTest");

            //Get the content control for setting visual states manually
            auto contentControl = callback->GetElementByName(L"contentControl");
            auto contentControlTyped = safe_cast<ContentControl^>(reinterpret_cast<Platform::Object^>(reinterpret_cast<IInspectable*>(contentControl.Handle)));

            // We need to go to the correct visual state in order for the value to take effect.
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(VisualStateManager::GoToState(contentControlTyped, "VisualStateSetterStoryboard", false));
            });

            TestServices::WindowHelper->WaitForIdle();

            //First, verify the Width/Height animated values have the correct source info.
            //Height should go back to the DoubleAnimation in the Storyboard
            wil::unique_propertychainsource heightAnimatedSource = m_tap->GetSourceForProperty(manualGrid.Handle, L"Height", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(heightAnimatedSource.SrcInfo.LineNumber, 33u);

            //Width should go back to the Setter in the VisualState's setters.
            wil::unique_propertychainsource widthAnimatedSource = m_tap->GetSourceForProperty(manualGrid.Handle, L"Width", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(widthAnimatedSource.SrcInfo.LineNumber, 40u);

            //Verify we can still get the locally set Width/Height values, and their sources info is intact.
            wil::unique_propertychainsource heightLocalSource = m_tap->GetSourceForProperty(manualGrid.Handle, L"Height", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(heightLocalSource.SrcInfo.LineNumber, 7u);

            wil::unique_propertychainsource widthLocalSource = m_tap->GetSourceForProperty(manualGrid.Handle, L"Width", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(widthLocalSource.SrcInfo.LineNumber, 7u);

            //Lastly, make sure the local/animated values are correct.
            wil::unique_propertychainvalue heightAnimatedValue = GetPropertyChainValue(manualGrid.Handle, L"Height", BaseValueSource::Animation);

            //Animated value is somewhere between 110-300, but not 100
            VERIFY_IS_GREATER_THAN(std::stoi(heightAnimatedValue.Value), 109);
            VERIFY_IS_LESS_THAN(std::stoi(heightAnimatedValue.Value), 301);

            wil::unique_propertychainvalue widthAnimatedValue = GetPropertyChainValue(manualGrid.Handle, L"Width", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(std::stoi(widthAnimatedValue.Value), 300);

            wil::unique_propertychainvalue heightLocalValue = GetPropertyChainValue(manualGrid.Handle, L"Height", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(std::stoi(heightLocalValue.Value), 100);

            wil::unique_propertychainvalue widthLocalValue = GetPropertyChainValue(manualGrid.Handle, L"Width", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(std::stoi(widthLocalValue.Value), 100);

            //Apply a new Visual State which will be applied at the same time as the current one, and which
            //should overwrite its Width.
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(VisualStateManager::GoToState(contentControlTyped, "VisualStateOverlap", false));
            });

            TestServices::WindowHelper->WaitForIdle();

            //Verify the animated value is updated to VisualStateOverlap's setter and that the local value is unchanged.
            widthAnimatedSource = m_tap->GetSourceForProperty(manualGrid.Handle, L"Width", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(widthAnimatedSource.SrcInfo.LineNumber, 47u);

            widthLocalSource = m_tap->GetSourceForProperty(manualGrid.Handle, L"Width", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(widthLocalSource.SrcInfo.LineNumber, 7u);

            widthAnimatedValue = GetPropertyChainValue(manualGrid.Handle, L"Width", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(std::stoi(widthAnimatedValue.Value), 330);

            widthLocalValue = GetPropertyChainValue(manualGrid.Handle, L"Width", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(std::stoi(widthLocalValue.Value), 100);
        }

        void AnimatedValueTests::TestAnimatedValuesManual()
        {
            TestAnimatedValuesManualBase(false);
        }

        void AnimatedValueTests::TestAnimatedValuesManualXBF()
        {
            TestAnimatedValuesManualBase(true);
        }

        void AnimatedValueTests::TestAnimatedValuesStoryboardInDictionaryBase(bool forceXbf)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, forceXbf);
            auto cleanup = m_connectionHelper->Advise(XamlDiagnosticsTestHelpers::modifiableSetterAndStoryboardString, callback);

            TestServices::WindowHelper->WaitForIdle();

            auto parent = callback->GetElementByName(L"parent");
            unsigned int indexGetPropertyIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(parent.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources", &indexGetPropertyIndex));
            InstanceHandle dictionaryHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(parent.Handle, indexGetPropertyIndex, &dictionaryHandle));
            ResourceDictionary^ dictionary = safe_cast<ResourceDictionary^>(reinterpret_cast<Platform::Object^>(reinterpret_cast<IInspectable*>(dictionaryHandle)));
            Platform::String^ spKeyStr = L"storyboardKey";

            //Start the Storyboard which will animate the Height property of the Button.
            RunOnUIThread([&]()
            {
                Storyboard^ storyboardTyped = safe_cast<Storyboard^>(dictionary->Lookup(spKeyStr));
                storyboardTyped->Begin();
            });

            TestServices::WindowHelper->WaitForIdle();

            auto button = callback->GetElementByName(L"button");

            //Verify the Storyboard is animating the Height property correctly
            //by checking animated/local Height source/value
            wil::unique_propertychainsource heightAnimatedSource = m_tap->GetSourceForProperty(button.Handle, L"Height", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(heightAnimatedSource.SrcInfo.LineNumber, 6u);

            wil::unique_propertychainsource heightLocalSource = m_tap->GetSourceForProperty(button.Handle, L"Height", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(heightLocalSource.SrcInfo.LineNumber, 9u);

            wil::unique_propertychainvalue heightAnimatedValue = GetPropertyChainValue(button.Handle, L"Height", BaseValueSource::Animation);

            //Animated value is somewhere between 110-200, but not 100
            VERIFY_IS_GREATER_THAN(std::stoi(heightAnimatedValue.Value), 109);
            VERIFY_IS_LESS_THAN(std::stoi(heightAnimatedValue.Value), 201);

            wil::unique_propertychainvalue heightLocalValue = GetPropertyChainValue(button.Handle, L"Height", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(std::stoi(heightLocalValue.Value), 100);
        }

        void AnimatedValueTests::TestAnimatedValuesStoryboardInDictionary()
        {
            TestAnimatedValuesStoryboardInDictionaryBase(false);
        }

        void AnimatedValueTests::TestAnimatedValuesStoryboardInDictionaryXBF()
        {
            TestAnimatedValuesStoryboardInDictionaryBase(true);
        }

        void AnimatedValueTests::TestCanAddVsmSetters()
        {
            TestCanAddVsmSetters(false);
        }

        void AnimatedValueTests::TestCanAddVsmSettersToActiveState()
        {
            TestCanAddVsmSetters(true);
        }

        void AnimatedValueTests::TestCanAddVsmSetters(bool setActiveBeforeAddSetter)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement parent = callback->GetElementByName(L"LayoutRoot");

            // Get the visual group collection. We expect there to be three groups
            auto visualGroupCollection = GetVisualStateGroupsForElement(parent.Handle);
            VERIFY_ARE_EQUAL(visualGroupCollection.Count, 3u);

            // Get the actual visual state groups.
            auto visualStateCollection = GetStatesInVisualStateGroup(std::stoll(visualGroupCollection.Elements[0].Value));

            // Verify we can add a setter to this visual state collection and have it be reflected
            InstanceHandle redBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Red");

            // Set the button as the target.
            VisualElement button = callback->GetElementByName(L"visualStateManualTest");
            InstanceHandle visualState = std::stoll(visualStateCollection.Elements[0].Value);
            auto setter = CreateSetterWithTarget(redBrush, button.Handle, L"Background");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, GetBackgroundColor(button.Handle, BaseValueSourceLocal));

            auto goToState = [&]()
            {
                RunOnUIThread([&] {
                        auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                        VERIFY_IS_TRUE(VisualStateManager::GoToState(page, "VisualStateTriggerTest", false));
                    });
            };

            if (setActiveBeforeAddSetter)
            {
                goToState();
                AddSetterToVisualState(setter, visualState, 1u);
            }
            else
            {
                AddSetterToVisualState(setter, visualState, 1u);
                goToState();
            }

            TestServices::WindowHelper->WaitForIdle();

            // Get the animated background color and make sure that baby is red
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, GetBackgroundColor(button.Handle, BaseValueSource::Animation));

            // Clear the setter value, make sure the button is updated

            ClearProperty(setter, L"Microsoft.UI.Xaml.Setter.Value");
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_THROWS_SPECIFIC(GetPropertyChainValue(button.Handle, L"Background", BaseValueSource::Animation), WEX::Common::Exception, [](WEX::Common::Exception ex) {return ex.ErrorCode() == E_NOTFOUND; });
            RunOnUIThread([&] {
                auto buttonObj = ih_cast<xaml_controls::Button>(button.Handle);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, safe_cast<xaml_media::SolidColorBrush^>(buttonObj->Background)->Color);
            });

            // Put the setter in a wonky state, make sure things are happy
            ClearProperty(setter, L"Microsoft.UI.Xaml.Setter.Target");
            SetProperty(setter, redBrush, L"Microsoft.UI.Xaml.Setter.Value");
            SetTargetOnSetter(setter, button.Handle, L"Background");
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, GetBackgroundColor(button.Handle, BaseValueSource::Animation));
            RunOnUIThread([&] {
                auto buttonObj = ih_cast<xaml_controls::Button>(button.Handle);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, safe_cast<xaml_media::SolidColorBrush^>(buttonObj->Background)->Color);
            });
        }

        void AnimatedValueTests::TestRemoveSetterBase(const std::wstring& stateName, const std::wstring& buttonName, int expectedWidth, int vsGroupIndex)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement parent = callback->GetElementByName(L"LayoutRoot");

            // Get the visual group collection.
            auto visualGroupCollection = GetVisualStateGroupsForElement(parent.Handle);

            // Get the actual visual state groups.
            auto visualStateCollection = GetStatesInVisualStateGroup(std::stoll(visualGroupCollection.Elements[vsGroupIndex].Value));
            InstanceHandle visualState = std::stoll(visualStateCollection.Elements[0].Value);

            // Get the Setters property
            unsigned int settersIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(visualState, L"Microsoft.UI.Xaml.VisualState.Setters", &settersIndex));

            InstanceHandle setters = 0;
            VERIFY_SUCCEEDED(m_tap->GetProperty(visualState, settersIndex, &setters));

            VERIFY_SUCCEEDED(m_tap->RemoveChild(setters, 0u));

            RunOnUIThread([&] {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                // Go to the visual state that the new setter was added.
                VERIFY_IS_TRUE(VisualStateManager::GoToState(page, ref new Platform::String(stateName.c_str()), false));
            });

            TestServices::WindowHelper->WaitForIdle();

            VisualElement button = callback->GetElementByName(buttonName.c_str());
            // Get the animated width value and make sure that throws since we removed it.
            VERIFY_THROWS(GetPropertyChainValue(button.Handle, L"Width", BaseValueSource::Animation), WEX::Common::Exception);

            // Verify the width hasn't changed.
            auto width = GetPropertyChainValue(button.Handle, L"Width", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(expectedWidth, std::stoi(width.Value));
        }

        void AnimatedValueTests::TestCanRemoveVsmSettersPreState()
        {
            std::wstring stateName = L"VisualStateManualTest";
            std::wstring buttonName = L"visualStateManualTest";
            int expectedWidth = 100;
            int vsGroupIndex = 1;
            TestRemoveSetterBase(stateName, buttonName, expectedWidth, vsGroupIndex);
        }

        void AnimatedValueTests::TestCanRemoveVsmSettersInActiveState()
        {
            // Set window size to 600. The trigger we are modifying is set at a MinWindowWidth of 500.
            ::Windows::Foundation::Size size(600, 600);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            std::wstring stateName = L"VisualStateTriggerTest";
            std::wstring buttonName = L"visualStateTriggerTest";
            int expectedWidth = 100;
            int vsGroupIndex = 0;
            TestRemoveSetterBase(stateName, buttonName, expectedWidth, vsGroupIndex);
        }

        void AnimatedValueTests::TestCanModifyVsmSettersInUnnamedState()
        {
            // Set window size to 400. The trigger we are modifying is set at a MinWindowWidth of 300. So the state is active.
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            // First, verify the animated width value since the state should be active.
            VisualElement button = callback->GetElementByName(L"unnamedVisualStateTest");
            auto animatedWidth = GetPropertyChainValue(button.Handle, L"Width", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(std::stoi(animatedWidth.Value), 200);

            VisualElement parent = callback->GetElementByName(L"LayoutRoot");

            // Get the visual group collection. We expect there to be three groups
            auto visualGroupCollection = GetVisualStateGroupsForElement(parent.Handle);
            VERIFY_ARE_EQUAL(visualGroupCollection.Count, 3u);

            //Get the actual visual state groups. There is only one VisualState in it currently
            auto visualStateCollection = GetStatesInVisualStateGroup(std::stoll(visualGroupCollection.Elements[2].Value));
            VERIFY_ARE_EQUAL(visualStateCollection.Count, 1u);

            InstanceHandle unnamedVisualState = std::stoll(visualStateCollection.Elements[0].Value);

            // Get the Setters property
            auto setters = GetVisualStateProperty(unnamedVisualState, L"Setters");
            VERIFY_ARE_EQUAL(setters.Count, 1u);

            InstanceHandle setterHandle = std::stoll(setters.Elements[0].Value);

            // Update the setter and change the value to 250.
            InstanceHandle newSetterValue = CreateInstance(L"Windows.Foundation.Double", L"250");
            SetProperty(setterHandle, newSetterValue, L"Microsoft.UI.Xaml.Setter.Value");

            // Verify the property has changed. This is expected, since the state is active
            animatedWidth = GetPropertyChainValue(button.Handle, L"Width", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(std::stoi(animatedWidth.Value), 250);
        }

        void AnimatedValueTests::TestCanModifyExistingVsmTriggersInExistingVisualState()
        {
            AnimationChangedParams inactive{ 100, 100, Microsoft::UI::Colors::Black };
            AnimationChangedParams active{ 300, 300, Microsoft::UI::Colors::Yellow };
            auto componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            TriggerChangedTestCase testCases[] = {
                // The trigger's MinWindowWidth is set to 500 to start out.
                MakeTestCase(wf::Size(400,400), 300.00, inactive, active),
                MakeTestCase(wf::Size(600,600), 700.00, active, inactive)
            };

            const int vsGroupIndex = 0, vsIndex = 0;
            for (const auto& testCase : testCases)
            {
                TestModifyVsmTriggerBase(
                    componentLocation,
                    vsGroupIndex,
                    vsIndex,
                    L"LayoutRoot",
                    L"visualStateTriggerTest",
                    testCase);
            }
        }

        void AnimatedValueTests::TestCanModifyNewVsmTriggersInNewVisualState()
        {
            AnimationChangedParams inactive{ 100, 100, Microsoft::UI::Colors::Yellow };
            AnimationChangedParams active{ 300, 300, Microsoft::UI::Colors::Blue };
            auto componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            TriggerChangedTestCase testCases[] = {
                // The trigger's MinWindowWidth is set to 500 to start out.
                MakeTestCase(wf::Size(400,400), 300.00, inactive, active),
                MakeTestCase(wf::Size(600,600), 700.00, active, inactive)
            };

            const int vsGroupIndex = 1, vsIndex = 1;
            for (const auto& testCase : testCases)
            {
                TestModifyVsmTriggerBase(
                    componentLocation,
                    vsGroupIndex,
                    vsIndex,
                    L"LayoutRoot",
                    L"visualStateManualTest",
                    testCase);
            }
        }

        void AnimatedValueTests::TestCanModifyNewVsmTriggersInExistingVisualState()
        {
            AnimationChangedParams inactive{ 100, 100, Microsoft::UI::Colors::Yellow };
            AnimationChangedParams active{ 300, 300, Microsoft::UI::Colors::Blue };
            auto componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            TriggerChangedTestCase testCases[] = {
                // The trigger's MinWindowWidth is set to 500 to start out.
                MakeTestCase(wf::Size(400,400), 300.00, inactive, active),
                MakeTestCase(wf::Size(600,600), 700.00, active, inactive)
            };

            const int vsGroupIndex = 1, vsIndex = 0;
            for (const auto& testCase : testCases)
            {
                TestModifyVsmTriggerBase(
                    componentLocation,
                    vsGroupIndex,
                    vsIndex,
                    L"LayoutRoot",
                    L"visualStateManualTest",
                    testCase);
            }
        }

        void AnimatedValueTests::CanModifyVsmTriggersInUnnamedState()
        {
            AnimationChangedParams active{ 200, 300, Microsoft::UI::Colors::Yellow };
            AnimationChangedParams inactive{ 100, 100, Microsoft::UI::Colors::Purple };

            TriggerChangedTestCase testCases[] = {
                // The trigger's MinWindowWidth is set to 500 to start out.
                MakeTestCase(wf::Size(400,400), 300.00, inactive, active),
                MakeTestCase(wf::Size(600,600), 700.00, active, inactive)
            };

            auto componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/UnnamedStatePage.xaml");
            const int vsGroupIndex = 0, vsIndex = 0;
            for (const auto& testCase : testCases)
            {
                TestModifyVsmTriggerBase(
                    componentLocation,
                    vsGroupIndex,
                    vsIndex,
                    L"LayoutRoot",
                    L"bananas",
                    testCase);
            }
        }

        void AnimatedValueTests::TestModifyVsmTriggerBase(
            ::Windows::Foundation::Uri^ componentLocation,
            int groupIndex,
            int vsIndex,
            const std::wstring& vsmRoot,
            const std::wstring& buttonName,             // Name of the element we are testing
            const TriggerChangedTestCase& testParams
        )
        {
            TestServices::WindowHelper->SetWindowSizeOverride(testParams.WindowSize);

            wrl::ComPtr<VisualTreeServiceCallback> callback;

            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto parent = callback->GetElementByName(vsmRoot.c_str());
            auto button = callback->GetElementByName(buttonName.c_str());

            auto visualGroupCollection = GetVisualStateGroupsForElement(parent.Handle);

            auto visualStateGroup = ih_cast(visualGroupCollection.Elements[groupIndex].Value);
            InstanceHandle visualState = 0;

            const double newTriggerMinWidth = 500;
            const bool willNewTriggerBeActive = testParams.WindowSize.Width > newTriggerMinWidth;
            if (!TryGetStateInVisualStateGroup(visualStateGroup, vsIndex, visualState))
            {
                // If the setter is supposed to be originally active, then we should use the "before" values. Otherwise,
                // use the after ones.
                auto colorToUse = willNewTriggerBeActive ? testParams.BeforeValues.Color : testParams.AfterValues.Color;
                LOG_OUTPUT(L"No visual state, creating new one");
                auto color = CreateInstance(L"Windows.UI.Color", GetColorName(colorToUse));
                auto newAnimation = CreateAnimation(L"ColorAnimation", buttonName, L"(Button.Foreground).(SolidColorBrush.Color)", color);
                AddAnimationToStoryboard(parent.Handle, groupIndex, vsIndex, newAnimation);

                auto widthToUse = willNewTriggerBeActive ? testParams.BeforeValues.Width : testParams.AfterValues.Width;
                visualState = GetStateInVisualStateGroup(visualStateGroup, vsIndex);
                auto newWidth = CreateInstance(L"Windows.Foundation.Double", std::to_wstring(widthToUse));
                auto widthSetter = CreateSetterWithTarget(newWidth, button.Handle, L"Width");
                AddSetterToVisualState(widthSetter, visualState, 0u);

                auto heightToUse = willNewTriggerBeActive ? testParams.BeforeValues.Height : testParams.AfterValues.Height;
                auto newHeight = CreateInstance(L"Windows.Foundation.Double", std::to_wstring(heightToUse));
                auto heightSetter = CreateSetterWithTarget(newHeight, button.Handle, L"Height");
                AddSetterToVisualState(heightSetter, visualState, 1u);
            }

            // Register for the storyboard completion so we know when it's changed
            auto storyboardCompletedEvent = std::make_shared<Event>();
            auto storyboardCompletedRegistration = CreateSafeEventRegistration(xaml_animation::Storyboard, Completed);

            auto storyboard = GetProperty<IStoryboard>(visualState, L"Microsoft.UI.Xaml.VisualState.Storyboard");
            storyboardCompletedRegistration.Attach(safe_cast<Storyboard^>(storyboard),
                ref new ::Windows::Foundation::EventHandler<Platform::Object ^>(
                    [storyboardCompletedEvent](Platform::Object ^sender, Platform::Object ^e)
            {
                storyboardCompletedEvent->Set();
            }));

            InstanceHandle triggerHandle = 0;
            if (!TryGetItemInVisualStateProperty(visualState, L"StateTriggers", 0, triggerHandle))
            {
                LOG_OUTPUT(L"No trigger in collection, creating one now with MinWindowWidth='%.2f'", newTriggerMinWidth);
                triggerHandle = CreateAdaptiveTrigger(L"MinWindowWidth", newTriggerMinWidth);
                AddTriggerToVisualState(visualState, triggerHandle);

                // This means that we should just be active, so wait
                if (willNewTriggerBeActive)
                {
                    storyboardCompletedEvent->WaitForDefault();
                    storyboardCompletedEvent->Reset();
                }
            }

            VERIFY_ARE_EQUAL(testParams.BeforeValues.Color, GetColorProperty(button.Handle, L"Foreground", BaseValueSourceLocal));

            xaml_animation::ClockState stateBeforeChange = xaml_animation::ClockState::Stopped;
            RunOnUIThread([&] {
                stateBeforeChange = storyboard->GetCurrentState();
            });
            LOG_OUTPUT(L"Storyboard state: %s", m_tap->ConvertEnumValue(L"Microsoft.UI.Xaml.Media.Animation.ClockState", (int)stateBeforeChange).c_str());
            bool wasStopped = stateBeforeChange == xaml_animation::ClockState::Stopped;

            // Update the trigger property and verify it actually was applied
            auto previousMinWidth = GetProperty<wf::IPropertyValue>(triggerHandle, L"Microsoft.UI.Xaml.AdaptiveTrigger.MinWindowWidth");
            VERIFY_ARE_EQUAL(wf::PropertyType::Double, previousMinWidth->Type);

            auto verifyNoAnimatedProperties = [button, this](const AnimationChangedParams& expectedAnimatedValues) {
                // Make sure the animated properties aren't applied and make sure the locally reported values are as expected
                VERIFY_THROWS_SPECIFIC(GetPropertyChainValue(button.Handle, L"Width", BaseValueSource::Animation), WEX::Common::Exception, [](WEX::Common::Exception ex) {return ex.ErrorCode() == E_NOTFOUND; });
                VERIFY_THROWS_SPECIFIC(GetPropertyChainValue(button.Handle, L"Height", BaseValueSource::Animation), WEX::Common::Exception, [](WEX::Common::Exception ex) {return ex.ErrorCode() == E_NOTFOUND; });

                VERIFY_ARE_EQUAL(expectedAnimatedValues.Color, GetColorProperty(button.Handle, L"Foreground", BaseValueSourceLocal));
                auto widthValue = GetPropertyChainValue(button.Handle, L"Width", BaseValueSourceLocal);
                auto heightValue = GetPropertyChainValue(button.Handle, L"Height", BaseValueSourceLocal);
                VERIFY_ARE_EQUAL(std::stoi(widthValue.Value), expectedAnimatedValues.Width);
                VERIFY_ARE_EQUAL(std::stoi(heightValue.Value), expectedAnimatedValues.Height);
            };

            auto verifyAnimatedProperies = [button, this](const AnimationChangedParams& expectedAnimatedValues) {
                // Even though the color property is animated, the actual brush isn't so it still reports as being local.
                VERIFY_ARE_EQUAL(expectedAnimatedValues.Color, GetColorProperty(button.Handle, L"Foreground", BaseValueSourceLocal));

                auto widthAnimatedValue = GetPropertyChainValue(button.Handle, L"Width", BaseValueSource::Animation);
                auto heightAnimatedValue = GetPropertyChainValue(button.Handle, L"Height", BaseValueSource::Animation);

                VERIFY_ARE_EQUAL(std::stoi(heightAnimatedValue.Value), expectedAnimatedValues.Height);
                VERIFY_ARE_EQUAL(std::stoi(widthAnimatedValue.Value), expectedAnimatedValues.Width);
            };

            SetPropertyOnTrigger(triggerHandle, L"MinWindowWidth", testParams.NewTriggerMinWindowWidth);

            auto propertyChainValue = GetPropertyChainValue(triggerHandle, L"MinWindowWidth", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(std::stoi(propertyChainValue.Value), (int)testParams.NewTriggerMinWindowWidth);

            if (wasStopped)
            {
                // If the storyboard was stopped before  wait for the storyboard to complete and verify they are set
                storyboardCompletedEvent->WaitForDefault();
                verifyAnimatedProperies(testParams.AfterValues);
            }
            else
            {
                // otherwise, verify the animated values were removed
                verifyNoAnimatedProperties(testParams.AfterValues);
            }

            auto triggerTest = ih_cast<xaml_controls::Button>(button.Handle);

            // Make sure the actual value as seen by the application has updated
            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(testParams.AfterValues.Width, (int)triggerTest->Width);
                VERIFY_ARE_EQUAL(testParams.AfterValues.Height, (int)triggerTest->Height);
                VERIFY_ARE_EQUAL(testParams.AfterValues.Color, safe_cast<SolidColorBrush^>(triggerTest->Foreground)->Color);
            });

            // Set the property back to what it was before and make sure it isn't applied
            SetPropertyOnTrigger(triggerHandle, L"MinWindowWidth", previousMinWidth->GetDouble());
            if (wasStopped)
            {
                // If the animation was stopped before, make sure we can't get them after removing the animation.
                verifyNoAnimatedProperties(testParams.BeforeValues);
            }
            else
            {
                // If active before, wait for the storyboard to complete and verify the animated properties are
                // as expected
                storyboardCompletedEvent->WaitForDefault();
                verifyAnimatedProperies(testParams.BeforeValues);
            }

            // Make sure the actual value has updated
            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(testParams.BeforeValues.Width, (int)triggerTest->Width);
                VERIFY_ARE_EQUAL(testParams.BeforeValues.Height, (int)triggerTest->Height);
                VERIFY_ARE_EQUAL(testParams.BeforeValues.Color, safe_cast<SolidColorBrush^>(triggerTest->Foreground)->Color);
            });
        }

        void AnimatedValueTests::TestCanAddTrigger()
        {
            // Set window size to 400. The trigger we are modifying is set at a MinWindowWidth of 400.
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement parent = callback->GetElementByName(L"LayoutRoot");

            // Get the visual group collection. We expect there to be three groups
            auto visualGroupCollection = GetVisualStateGroupsForElement(parent.Handle);
            VERIFY_ARE_EQUAL(visualGroupCollection.Count, 3u);

            //Get the actual visual state groups. There is only one VisualState in it currently
            auto visualStateCollection = GetStatesInVisualStateGroup(std::stoll(visualGroupCollection.Elements[0].Value));
            VERIFY_ARE_EQUAL(visualStateCollection.Count, 1u);

            InstanceHandle visualState = std::stoll(visualStateCollection.Elements[0].Value);

            // Get the StateTriggers property
            auto triggers = GetVisualStateProperty(visualState, L"StateTriggers");
            VERIFY_ARE_EQUAL(triggers.Count, 1u);

            // Create a new trigger and add it to the collection. This trigger should
            // be evaluated immediately and cause the state to kick in.
            auto newTrigger = CreateAdaptiveTrigger(L"MinWindowHeight", 300.0);

            VERIFY_SUCCEEDED(m_tap->AddChild(triggers.Handle, newTrigger, triggers.Count));

            auto button = callback->GetElementByName(L"visualStateTriggerTest");

            wil::unique_propertychainvalue widthAnimatedValue = GetPropertyChainValue(button.Handle, L"Width", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(std::stoi(widthAnimatedValue.Value), 300);
        }

        void AnimatedValueTests::TestRemoveTriggerBase(bool isCurrentlySet)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement parent = callback->GetElementByName(L"LayoutRoot");

            // Get the visual group collection. We expect there to be three groups
            auto visualGroupCollection = GetVisualStateGroupsForElement(parent.Handle);
            VERIFY_ARE_EQUAL(visualGroupCollection.Count, 3u);

            //Get the actual visual state groups. There is only one VisualState in it currently
            auto visualStateCollection = GetStatesInVisualStateGroup(std::stoll(visualGroupCollection.Elements[0].Value));
            VERIFY_ARE_EQUAL(visualStateCollection.Count, 1u);

            InstanceHandle visualState = std::stoll(visualStateCollection.Elements[0].Value);

            // Get the StateTriggers property
            auto triggers = GetVisualStateProperty(visualState, L"StateTriggers");
            VERIFY_ARE_EQUAL(triggers.Count, 1u);

            auto button = callback->GetElementByName(L"visualStateTriggerTest");

            wil::unique_propertychainvalue widthValue = GetPropertyChainValue(button.Handle, L"Width", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(std::stoi(widthValue.Value), 100);

            // If it's set, then verify that we have the animated value before removing it.
            if (isCurrentlySet)
            {
                auto animatedWidthProperty = GetPropertyChainValue(button.Handle, L"Width", BaseValueSource::Animation);
                VERIFY_ARE_EQUAL(std::stoi(animatedWidthProperty.Value), 300);
            }

            // Remove the child and increase the window size (if not in the state already). The trigger we just removed had a MinWindowWidth
            // of 500, increasing the window size to 600 would've caused the trigger to evaluate and cause us to transition to the visual state.
            // If we are already in the state, we want to verify that removing it was all we needed to do.
            VERIFY_SUCCEEDED(m_tap->RemoveChild(triggers.Handle, 0u));

            if (!isCurrentlySet)
            {
                ::Windows::Foundation::Size newSize(600, 600);
                TestServices::WindowHelper->SetWindowSizeOverride(newSize);

                TestServices::WindowHelper->WaitForIdle();
            }

            // Verify we no longer have an animated value
            VERIFY_THROWS_SPECIFIC(GetPropertyChainValue(button.Handle, L"Width", BaseValueSource::Animation), WEX::Common::Exception, [](WEX::Common::Exception ex) {return ex.ErrorCode() == E_NOTFOUND; });

            widthValue = GetPropertyChainValue(button.Handle, L"Width", BaseValueSourceLocal);
            VERIFY_ARE_EQUAL(std::stoi(widthValue.Value), 100);

            // get the actual object, and verify its size
            RunOnUIThread([&]() {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);

                auto button = safe_cast<Button^>(page->FindName("visualStateTriggerTest"));

                VERIFY_ARE_EQUAL(static_cast<int>(button->Width), 100);
            });
        }

        void AnimatedValueTests::TestCanRemoveTriggerPreState()
        {
            // Set window size to 400. The trigger we are modifying is set at a MinWindowWidth of 500.
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            TestRemoveTriggerBase(false /*isTriggerSet*/);
        }

        void AnimatedValueTests::TestCanRemoveTriggerInState()
        {
            // Set window size to 600. The trigger we are removing is set at a MinWindowWidth of 500.
            // We want to make sure that removing a trigger while it is currently in the state will cause
            // the state to change.
            ::Windows::Foundation::Size size(600, 600);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            TestRemoveTriggerBase(true /*isTriggerSet*/);
        }

        void AnimatedValueTests::TestCanAddVisualState()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            LOG_OUTPUT(L"Scenario 1: Non-template case");
            {
                VisualElement parent = callback->GetElementByName(L"LayoutRoot");

                // Get the visual group collection. We expect there to be three groups
                auto visualGroupCollection = GetVisualStateGroupsForElement(parent.Handle);
                VERIFY_ARE_EQUAL(visualGroupCollection.Count, 3u);

                auto newHandle = AddNewVisualStateToGroup(std::stoll(visualGroupCollection.Elements[0].Value), L"NewState");
                VERIFY_ARE_NOT_EQUAL(newHandle, 0u);

                InstanceHandle redBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Red");

                VisualElement button = callback->GetElementByName(L"visualStateManualTest");
                auto setter = CreateSetterWithTarget(redBrush, button.Handle, L"Background");

                // Add setter at index 0 since the visual state doesn't have any setters.
                AddSetterToVisualState(setter, newHandle, 0u);

                // Verify we can go to the visual state and have it be reflected
                RunOnUIThread([&] {
                    auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                    // Go to the visual state that the new setter was added.
                    VERIFY_IS_TRUE(VisualStateManager::GoToState(page, "NewState", false));
                });

                auto background = GetPropertyChainValue(button.Handle, L"Background", BaseValueSource::Animation);
                auto color = GetPropertyChainValue(std::stoll(background.Value), L"Color", BaseValueSourceLocal);
                VERIFY_IS_TRUE(wcscmp(L"Red", color.Value) == 0);
            }

            LOG_OUTPUT(L"Scenario 2: Template case");
            {
                VisualElement parent = callback->GetElementByName(L"TemplatedLayoutRoot");

                // Get the visual group collection. We expect there to be three groups
                auto visualGroupCollection = GetVisualStateGroupsForElement(parent.Handle);
                VERIFY_ARE_EQUAL(visualGroupCollection.Count, 2u);

                auto newHandle = AddNewVisualStateToGroup(std::stoll(visualGroupCollection.Elements[0].Value), L"NewTemplateState");
                VERIFY_ARE_NOT_EQUAL(newHandle, 0u);

                InstanceHandle redBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Red");

                auto setter = CreateSetterWithTarget(redBrush, parent.Handle, L"Background");

                // Add setter at index 0 since the visual state doesn't have any setters.
                AddSetterToVisualState(setter, newHandle, 0u);

                // Verify we can go to the visual state and have it be reflected
                auto button = ih_cast<Button>(callback->GetElementByName(L"visualStateTemplateTest").Handle);
                RunOnUIThread([&] {
                    // Go to the visual state that the new setter was added.
                    VERIFY_IS_TRUE(VisualStateManager::GoToState(button, "NewTemplateState", false));
                });

                auto background = GetPropertyChainValue(parent.Handle, L"Background", BaseValueSource::Animation);
                auto color = GetPropertyChainValue(std::stoll(background.Value), L"Color", BaseValueSourceLocal);
                VERIFY_IS_TRUE(wcscmp(L"Red", color.Value) == 0);
            }
        }

        void AnimatedValueTests::TestCanRemoveVisualState()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement parent = callback->GetElementByName(L"LayoutRoot");

            // Get the visual group collection. We expect there to be three groups
            auto visualGroupCollection = GetVisualStateGroupsForElement(parent.Handle);
            VERIFY_ARE_EQUAL(visualGroupCollection.Count, 3u);

            // Remove the visual state.
            RemoveVisualStateFromGroup(std::stoll(visualGroupCollection.Elements[0].Value), 0u);

            RunOnUIThread([&] {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                // Try to go to the visual state that we just removed and make sure it returns false.
                VERIFY_IS_FALSE(VisualStateManager::GoToState(page, "VisualStateTriggerTest", false));
            });
        }


        void AnimatedValueTests::TestCanAddAnimationToStoryboard()
        {
            TestAddAnimationToStoryboard(false);
        }

        void AnimatedValueTests::TestCanAddAnimationToStoryboardWhileActive()
        {
            TestAddAnimationToStoryboard(true);
        }

        void AnimatedValueTests::TestAddAnimationToStoryboard(bool isActive)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            /*
            Imagine the following markup:
            <VisualStateGroup x:Name="Manual">
              <VisualState x:Name="VisualStateManualTest">
                <Storyboard x:Name="myStoryboard2">
                  <DoubleAnimation EnableDependentAnimation="True" From="110" To="300" Duration="00:00:1"
                    Storyboard.TargetName="visualStateManualTest"
                    Storyboard.TargetProperty="Height">
                   <DoubleAnimation.EasingFunction>
                        <BounceEase Bounces="2" EasingMode="EaseOut" Bounciness="2" />
                   </DoubleAnimation.EasingFunction>
                  </DoubleAnimation>
                  <ObjectAnimationUsingKeyFrames
                    Storyboard.TargetName="Foo"
                    Storyboard.TargetProperty="Foreground">
                        <DiscreteObjectKeyFrame KeyTime="0" Value="Blue" />
                   </ObjectAnimationUsingKeyFrames>
                </Storyboard>
                <VisualState.Setters>
                  <Setter Target="visualStateManualTest.Width" Value="300" />
                </VisualState.Setters>
              </VisualState>
            </VisualStateGroup>
            <Button x:Name="visualStateManualTest" Height="100" Width="100">
              <Button.Background>
                <SolidColorBrush x:Name="visualStateManualTestBrush" Color="Green"/>
              </Button.Background>
            </Button>

            After (edits noted by *****):
            <VisualStateGroup x:Name="Manual">
              <VisualState x:Name="VisualStateManualTest">
                <Storyboard x:Name="myStoryboard2">
                  <DoubleAnimation EnableDependentAnimation="True" From="110" To="300" Duration="00:00:1"
                    Storyboard.TargetName="visualStateManualTest"
                    Storyboard.TargetProperty="Height">
                   <DoubleAnimation.EasingFunction>
                        <BounceEase Bounces="2" EasingMode="EaseOut" Bounciness="2" />
                   </DoubleAnimation.EasingFunction>
                  </DoubleAnimation>
              *****<ColorAnimation Storyboard.TargetName = "visualStateManualTestBrush"
                    Storyboard.TargetProperty = "Color" To="Blue" / > *********
                   <ObjectAnimationUsingKeyFrames
                    Storyboard.TargetName="Foo"
                    Storyboard.TargetProperty="Foreground">
                        <DiscreteObjectKeyFrame KeyTime="0" Value="***** Red ****" />
                   </ObjectAnimationUsingKeyFrames>
                </Storyboard>
                <VisualState.Setters>
                  <Setter Target="visualStateManualTest.Width" Value="300" />
                </VisualState.Setters>
              </VisualState>
            </VisualStateGroup>
            <Button x:Name="visualStateManualTest" Height="100" Width="100">
              <Button.Background>
                <SolidColorBrush x:Name="visualStateManualTestBrush" Color="Green"/>
              </Button.Background>
            </Button>
            */
            InstanceHandle brush = CreateInstance(L"Windows.UI.Color", L"Green");
            InstanceHandle newAnimation = CreateAnimation(L"ColorAnimation", L"visualStateManualTestBrush", L"Color", brush);

            VisualElement vsRoot = callback->GetElementByName(L"LayoutRoot");

            if (isActive)
            {
                // If active before adding, then do so.
                RunOnUIThread([&] {
                    auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                    VERIFY_IS_TRUE(VisualStateManager::GoToState(page, "VisualStateManualTest", false));
                });
            }

            InstanceHandle keyFrameAnimation = GetAnimationFromStoryboard(vsRoot.Handle, L"Manual", L"VisualStateManualTest", 1);
            InstanceHandle keyFrame = GetCollectionItem(keyFrameAnimation, L"Microsoft.UI.Xaml.Media.Animation.ObjectAnimationUsingKeyFrames.KeyFrames", 0);
            auto redBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Red");
            SetProperty(keyFrame, redBrush, L"Microsoft.UI.Xaml.Media.Animation.ObjectKeyFrame.Value");

            AddAnimationToStoryboard(vsRoot.Handle, L"Manual", L"VisualStateManualTest", newAnimation);

            if (!isActive)
            {
                // If not active before adding, then do so right meow.
                RunOnUIThread([&] {
                    auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                    VERIFY_IS_TRUE(VisualStateManager::GoToState(page, "VisualStateManualTest", false));
                });
            }

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&] {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                auto button = safe_cast<Button^>(page->FindName("visualStateManualTest"));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, safe_cast<SolidColorBrush^>(button->Background)->Color);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, safe_cast<SolidColorBrush^>(button->Foreground)->Color);
            });
        }


        // This test validates the scenario where the visual state doesn't have a storyboard, so we create it on-demand. In order for this work,
        // we need to make sure the storyboard has it's templated parent set, otherwise it won't be able to find "Foo".
        void AnimatedValueTests::TestCanAddAnimationWithNamedTargetToNewStoryboard()
        {
            /*
            Imagine the following markup:
              <Button x:Name="visualStateTemplateTest" Width="100" Height="100" >
                <Button.Template>
                  <ControlTemplate TargetType="Button">
                    <Grid x:Name="TemplatedLayoutRoot">
                      <VisualStateManager.VisualStateGroups>
                        <VisualStateGroup x:Name="CommonStates">
                          <VisualState x:Name="Normal"/>
                        </VisualStateGroup>
                        <VisualStateGroup x:Name="OtherStates">
                        </VisualStateGroup>
                      </VisualStateManager.VisualStateGroups>
                      <Grid.Background>
                        <SolidColorBrush x:Name="ButtonBrush" Color="Green"/>
                      </Grid.Background>
                      <Grid x:Name="InsetBackground" Width ="50" Height="50">
                        <Grid.Background>
                          <SolidColorBrush x:Name="InsetBrush" Color="Red"/>
                        </Grid.Background>
                      </Grid>
                    </Grid>
                  </ControlTemplate>
                </Button.Template>
              </Button>


            After (edits noted by *****):
              <Button x:Name="visualStateTemplateTest" Width="100" Height="100" >
                <Button.Template>
                  <ControlTemplate TargetType="Button">
                    <Grid x:Name="TemplatedLayoutRoot">
                      <VisualStateManager.VisualStateGroups>
                        <VisualStateGroup x:Name="CommonStates">
                          <VisualState x:Name="Normal"/>
                        </VisualStateGroup>
                        <VisualStateGroup x:Name="OtherStates">
                  ****** <VisualState x:Name="Other">
                            <Storyboard>
                              <ColorAnimation Storyboard.TargetName="Foo"
                              Storyboard.TargetProperty="(Grid.Background).(SolidColorBrush.Color)" To="Green" />
                            </Storyboard>
                          </VisualState> **********
                        </VisualStateGroup>
                      </VisualStateManager.VisualStateGroups>
                      <Grid.Background>
                        <SolidColorBrush x:Name="ButtonBrush" Color="Green"/>
                      </Grid.Background>
                      <Grid x:Name="InsetBackground" Width ="50" Height="50">
                        <Grid.Background>
                          <SolidColorBrush x:Name="InsetBrush" Color="Red"/>
                        </Grid.Background>
                        *********<Button x:Name="Foo" Background="Yellow" />**********
                      </Grid>
                    </Grid>
                  </ControlTemplate>
                </Button.Template>
              </Button>
            */

           auto PostStateChangeValidate = []{
                TestServices::WindowHelper->WaitForIdle();
                // Validate everything is as expected
                RunOnUIThread([&] {
                    auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                    auto button = safe_cast<Button^>(page->FindName("visualStateTemplateTest"));
                    auto foo = safe_cast<Button^>(button->FindName("Foo"));
                    VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, safe_cast<SolidColorBrush^>(foo->Background)->Color);
                });
           };

           auto AddAnimation = [&](const VisualElement& vsRoot, const std::wstring& stateName){
                InstanceHandle greenBrush = CreateInstance(L"Windows.UI.Color", L"Green");
                InstanceHandle newAnimation = CreateAnimation(L"ColorAnimation", L"Foo", L"(Grid.Background).(SolidColorBrush.Color)", greenBrush);
                AddAnimationToStoryboard(vsRoot.Handle, L"OtherStates", stateName, newAnimation);
           };

           auto SetupButton = [&](const VisualElement& parent){
                InstanceHandle button = CreateControlWithBackground(L"Button", L"Yellow", L"Foo");
                auto children = GetCollectionProperty(parent.Handle, L"Microsoft.UI.Xaml.Controls.Panel.Children");
                VERIFY_SUCCEEDED(m_tap->AddChild(children.Handle, button, children.Count));
           };

           auto GoToState = [](Platform::String^ state){
                RunOnUIThread([&] {
                    auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                    auto button = safe_cast<Button^>(page->FindName("visualStateTemplateTest"));
                    VERIFY_IS_TRUE(VisualStateManager::GoToState(button, state, false));
                });
           };

           LOG_OUTPUT(L"Adding before state is active");
           {
                wrl::ComPtr<VisualTreeServiceCallback> callback;
                ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
                auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

                SetupButton(callback->GetElementByName(L"InsetBackground"));
                AddAnimation(callback->GetElementByName(L"TemplatedLayoutRoot"), L"Other");
                GoToState("Other");
                PostStateChangeValidate();
            }

            LOG_OUTPUT(L"Adding while state is active");
            {
                wrl::ComPtr<VisualTreeServiceCallback> callback;
                ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
                auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

                SetupButton(callback->GetElementByName(L"InsetBackground"));
                GoToState("StoryboardState");
                AddAnimation(callback->GetElementByName(L"TemplatedLayoutRoot"), L"StoryboardState");
                PostStateChangeValidate();
            }
        }

        // This test validates the scenario where we have an existing Storyboard, and we just try to add an animation to it.
        void AnimatedValueTests::TestCanAddAnimationWithNamedTarget()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);
            /*
            Imagine the following markup:
               <Button x:Name="visualStateTemplateTest" Width="100" Height="100" >
                <Button.Template>
                  <ControlTemplate TargetType="Button">
                    <Grid x:Name="TemplatedLayoutRoot">
                      <VisualStateManager.VisualStateGroups>
                        <VisualStateGroup x:Name="CommonStates">
                          <VisualState x:Name="Normal"/>
                        </VisualStateGroup>
                        <VisualStateGroup x:Name="OtherStates">
                          <VisualState x:Name="StoryboardState">
                            <Storyboard>
                              <DoubleAnimation
                                  EnableDependentAnimation="True" From="110" To="300" Duration="00:00:1"
                                  Storyboard.TargetName="visualStateManualTest"
                                  Storyboard.TargetProperty="Height"/>
                            </Storyboard>
                          </VisualState>
                        </VisualStateGroup>
                      </VisualStateManager.VisualStateGroups>
                    </Grid>
                  </ControlTemplate>
                </Button.Template>
              </Button>


            After (edits noted by *****):
               <Button x:Name="visualStateTemplateTest" Width="100" Height="100" >
                <Button.Template>
                  <ControlTemplate TargetType="Button">
                    <Grid x:Name="TemplatedLayoutRoot">
                      <VisualStateManager.VisualStateGroups>
                        <VisualStateGroup x:Name="CommonStates">
                          <VisualState x:Name="Normal"/>
                        </VisualStateGroup>
                        <VisualStateGroup x:Name="OtherStates">
                          <VisualState x:Name="StoryboardState">
                            <Storyboard>
                              <DoubleAnimation
                                  EnableDependentAnimation="True" From="110" To="300" Duration="00:00:1"
                                  Storyboard.TargetName="InsetBackground"
                                  Storyboard.TargetProperty="Height"/>
                     *********<ColorAnimation
                                  Storyboard.TargetName="Foo"
                                  Storyboard.TargetProperty="(Grid.Background).(SolidColorBrush.Color)"
                                  To="Blue"/> ***********
                            </Storyboard>
                          </VisualState>
                        </VisualStateGroup>
                      </VisualStateManager.VisualStateGroups>
                      *********<Button x:Name="Foo" Background="Yellow" />**********
                      </Grid>
                  </ControlTemplate>
                </Button.Template>
              </Button>
            */

            InstanceHandle button = CreateControlWithBackground(L"Button", L"Yellow", L"Foo");
            VisualElement parent = callback->GetElementByName(L"InsetBackground");
            auto children = GetCollectionProperty(parent.Handle, L"Microsoft.UI.Xaml.Controls.Panel.Children");
            VERIFY_SUCCEEDED(m_tap->AddChild(children.Handle, button, children.Count));

            InstanceHandle blueBrush = CreateInstance(L"Windows.UI.Color", L"Blue");
            InstanceHandle newAnimation = CreateAnimation(L"ColorAnimation", L"Foo", L"(Grid.Background).(SolidColorBrush.Color)", blueBrush);
            VisualElement vsRoot = callback->GetElementByName(L"TemplatedLayoutRoot");
            AddAnimationToStoryboard(vsRoot.Handle, L"OtherStates", L"StoryboardState", newAnimation);

            RunOnUIThread([&] {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                auto button = safe_cast<Button^>(page->FindName("visualStateTemplateTest"));
                // Try to go to the visual state that we just removed and make sure it returns false.
                VERIFY_IS_TRUE(VisualStateManager::GoToState(button, "StoryboardState", false));
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&] {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                auto button = safe_cast<Button^>(page->FindName("visualStateTemplateTest"));
                auto foo = safe_cast<Button^>(button->FindName("Foo"));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<SolidColorBrush^>(foo->Background)->Color);
            });
        }

        // This test validates the same addition as TestCanRemoveNewlyCreatedStoryboardWithAnimation. But then it also verifies
        // that we can remove the storyboard and everything is ok.
        void AnimatedValueTests::TestCanRemoveNewlyCreatedStoryboard()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            InstanceHandle button = CreateControlWithBackground(L"Button", L"Yellow", L"Foo");
            VisualElement parent = callback->GetElementByName(L"InsetBackground");
            auto children = GetCollectionProperty(parent.Handle, L"Microsoft.UI.Xaml.Controls.Panel.Children");
            VERIFY_SUCCEEDED(m_tap->AddChild(children.Handle, button, children.Count));

            InstanceHandle greenBrush = CreateInstance(L"Windows.UI.Color", L"Green");
            InstanceHandle newAnimation = CreateAnimation(L"ColorAnimation", L"Foo", L"(Grid.Background).(SolidColorBrush.Color)", greenBrush);
            VisualElement vsRoot = callback->GetElementByName(L"TemplatedLayoutRoot");
            AddAnimationToStoryboard(vsRoot.Handle, L"OtherStates", L"Other", newAnimation);

            Page^ rootPage = nullptr;
            Button^ visualStateTemplateTest = nullptr;
            Button^ foo = nullptr;

            RunOnUIThread([&] {
                rootPage = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                visualStateTemplateTest = safe_cast<Button^>(rootPage->FindName("visualStateTemplateTest"));
                VERIFY_IS_TRUE(VisualStateManager::GoToState(visualStateTemplateTest, "Other", false));
            });

            TestServices::WindowHelper->WaitForIdle();

            // Validate everything is as expected
            RunOnUIThread([&] {
                foo = safe_cast<Button^>(visualStateTemplateTest->FindName("Foo"));
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, safe_cast<SolidColorBrush^>(foo->Background)->Color);
            });

            // Now, remove the storyboard and try to leave the state, bad things should happen.
            auto visualGroupCollection = GetNamedVisualStateGroupForElement(vsRoot.Handle, L"OtherStates");
            auto visualState = GetNamedVisualStateInGroup(visualGroupCollection, L"Other");

            ClearProperty(visualState, L"Microsoft.UI.Xaml.VisualState.Storyboard");

            RunOnUIThread([&] {
                VERIFY_IS_TRUE(VisualStateManager::GoToState(visualStateTemplateTest, "StoryboardState", false));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&] {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Yellow, safe_cast<SolidColorBrush^>(foo->Background)->Color);
            });
        }

        void AnimatedValueTests::TestCanRemoveAnimation()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement parent = callback->GetElementByName(L"TemplatedLayoutRoot");

            // Get the visual group collection. We expect there to be 1 group
            auto visualGroupCollection = GetNamedVisualStateGroupForElement(parent.Handle, L"OtherStates");
            auto visualState = GetNamedVisualStateInGroup(visualGroupCollection, L"StoryboardState");

            auto storyBoard = GetVisualStateProperty(visualState, L"Storyboard");
            auto children = GetCollectionProperty(storyBoard.Handle, L"Microsoft.UI.Xaml.Media.Animation.Storyboard.Children");
            VERIFY_SUCCEEDED(m_tap->RemoveChild(children.Handle, 0u));

            RunOnUIThread([&] {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                auto button = safe_cast<Button^>(page->FindName("visualStateTemplateTest"));
                VERIFY_IS_TRUE(VisualStateManager::GoToState(button, "StoryboardState", false));
            });

            TestServices::WindowHelper->WaitForIdle();

            // Verify the height isn't set
            auto grid = callback->GetElementByName(L"InsetBackground");
            VERIFY_THROWS_SPECIFIC(GetPropertyChainValue(grid.Handle, L"Height", BaseValueSource::Animation), WEX::Common::Exception, [](WEX::Common::Exception ex) {return ex.ErrorCode() == E_NOTFOUND; });
        }

        void AnimatedValueTests::TestCanRemoveAnimationInCurrentState()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement parent = callback->GetElementByName(L"TemplatedLayoutRoot");

            // Get the visual group collection. We expect there to be 1 group
            auto visualGroupCollection = GetNamedVisualStateGroupForElement(parent.Handle, L"OtherStates");
            auto visualState = GetNamedVisualStateInGroup(visualGroupCollection, L"StoryboardState");

            auto storyBoard = GetVisualStateProperty(visualState, L"Storyboard");
            auto children = GetCollectionProperty(storyBoard.Handle, L"Microsoft.UI.Xaml.Media.Animation.Storyboard.Children");

            RunOnUIThread([&] {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                auto button = safe_cast<Button^>(page->FindName("visualStateTemplateTest"));
                // Try to go to the visual state that we just removed and make sure it returns false.
                VERIFY_IS_TRUE(VisualStateManager::GoToState(button, "StoryboardState", false));
            });

            VERIFY_ARE_EQUAL(children.Count, 1u);
            VERIFY_SUCCEEDED(m_tap->RemoveChild(children.Handle, 0u));

            TestServices::WindowHelper->WaitForIdle();

            // Verify the height isn't set
            auto grid = callback->GetElementByName(L"InsetBackground");
            VERIFY_THROWS_SPECIFIC(GetPropertyChainValue(grid.Handle, L"Height", BaseValueSource::Animation), WEX::Common::Exception, [](WEX::Common::Exception ex) {return ex.ErrorCode() == E_NOTFOUND; });
        }

        void AnimatedValueTests::TestCanAddCommonStateInControlTemplate()
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            // Imagine we have the following markup:
            //<VisualStateGroup x : Name = "CommonStates">
            //    <VisualState x : Name = "Normal" / >
            //  </VisualStateGroup>

            // This test is going to transform it to this:
            //<VisualStateGroup x : Name = "CommonStates">
            //    <VisualState x : Name = "Normal" / >
            //    <VisualState x : Name = "PointerOver">
            //    <Storyboard>
            //       <ColorAnimation Storyboard.TargetName = "InsetBackground"
            //          Storyboard.TargetProperty = "(Grid.Background).(SolidColorBrush.Color)" To = "Blue" / >
            //    </Storyboard>
            //    </VisualState>
            //  </VisualStateGroup>

            // Get the visual group collection.
            VisualElement vsRoot = callback->GetElementByName(L"TemplatedLayoutRoot");
            auto visualGroupCollection = GetNamedVisualStateGroupForElement(vsRoot.Handle, L"CommonStates");

            // Get the PointerOver state, verify it isn't there
            InstanceHandle pointerOverState = 0;
            VERIFY_IS_FALSE(TryGetNamedVisualStateInGroup(visualGroupCollection, L"PointerOver", pointerOverState));
            VERIFY_ARE_EQUAL(pointerOverState, 0u);

            // Now add it
            pointerOverState = AddNewVisualStateToGroup(visualGroupCollection, L"PointerOver");
            VERIFY_ARE_NOT_EQUAL(pointerOverState, 0u);

            // Create the animation and add it to the storyboard
            InstanceHandle blueBrush = CreateInstance(L"Windows.UI.Color", L"Blue");
            InstanceHandle newAnimation = CreateAnimation(L"ColorAnimation", L"InsetBackground", L"(Grid.Background).(SolidColorBrush.Color)", blueBrush);

            // Register for property changed and storyboard completed events so we can reliably detect changes
            auto propertyChangedEvent = std::make_shared<Event>();
            AddAnimationToStoryboard(vsRoot.Handle, L"CommonStates", L"PointerOver", newAnimation);

            VisualElement backgroundGrid = callback->GetElementByName(L"InsetBackground");
            auto grid = GetFromInstanceHandle<xaml_controls::Grid>(backgroundGrid.Handle);

            wf::EventRegistrationToken token = {};
            RunOnUIThread([&] {
                token.Value = grid->RegisterPropertyChangedCallback(xaml_controls::Grid::BackgroundProperty,
                    ref new DependencyPropertyChangedCallback([propertyChangedEvent](DependencyObject^ sender, DependencyProperty^ prop) {
                        propertyChangedEvent->Set();
                }));
            });

            Button^ templatedButton = nullptr;
            RunOnUIThread([&] {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                templatedButton = safe_cast<Button^>(page->FindName("visualStateTemplateTest"));
            });

            auto storyboardCompletedEvent = std::make_shared<Event>();
            auto storyboardCompletedRegistration = CreateSafeEventRegistration(xaml_animation::Storyboard, Completed);

            auto pointerOverStoryboard = safe_cast<Storyboard^>(GetProperty<IStoryboard>(pointerOverState, L"Microsoft.UI.Xaml.VisualState.Storyboard"));

            storyboardCompletedRegistration.Attach(pointerOverStoryboard,
                ref new ::Windows::Foundation::EventHandler<Platform::Object ^>(
                    [storyboardCompletedEvent](Platform::Object ^sender, Platform::Object ^e)
            {
                storyboardCompletedEvent->Set();
            }));

            VERIFY_IS_FALSE(TestServices::WindowHelper->IsStoryboardActive(pointerOverStoryboard), L"Storyboard was prematurely started");
            // Now that we've created everything, hover over the control

            // First move to the top of the screen, since the InsetBackground is at the bottom of the control template,
            // we want to make sure we reliably hover
            TestServices::InputHelper->MoveMouse(wf::Point(0, 0));
            TestServices::InputHelper->MoveMouse(templatedButton);
            TestServices::WindowHelper->WaitForIdle();

            storyboardCompletedEvent->WaitForDefault();

            propertyChangedEvent->WaitForDefault();

            RunOnUIThread([&] {
                auto brush = safe_cast<SolidColorBrush^>(grid->Background);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, brush->Color);
                grid->UnregisterPropertyChangedCallback(xaml_controls::Grid::BackgroundProperty, token.Value);
            });

        }

        void AnimatedValueTests::TestAddRemoveVisualStateName()
        {
            // Set window size to 200. The trigger for the unnamed state is set at a MinWindowWidth of 300. So the state is inactive.
            ::Windows::Foundation::Size size(200, 200);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            // First, verify the animated width value since the state should be active.
            VisualElement button = callback->GetElementByName(L"unnamedVisualStateTest");
            VERIFY_THROWS_SPECIFIC(GetPropertyChainValue(button.Handle, L"Width", BaseValueSource::Animation), WEX::Common::Exception, [](WEX::Common::Exception ex) {return ex.ErrorCode() == E_NOTFOUND; });

            VisualElement parent = callback->GetElementByName(L"LayoutRoot");

            // Get the visual group collection. We expect there to be three groups
            auto visualGroupCollection = GetVisualStateGroupsForElement(parent.Handle);
            VERIFY_ARE_EQUAL(visualGroupCollection.Count, 3u);

            //Get the actual visual state groups. There is only one VisualState in it currently
            auto visualStateCollection = GetStatesInVisualStateGroup(std::stoll(visualGroupCollection.Elements[2].Value));
            VERIFY_ARE_EQUAL(visualStateCollection.Count, 1u);

            InstanceHandle unnamedVisualState = std::stoll(visualStateCollection.Elements[0].Value);

            // Give the unnamed state a temporary name
            InstanceHandle temporaryName = CreateInstance(L"Windows.Foundation.String", L"TemporaryState");
            SetProperty(unnamedVisualState, temporaryName, L"Microsoft.UI.Xaml.VisualState.Name");

            VERIFY_THROWS_SPECIFIC(GetPropertyChainValue(button.Handle, L"Width", BaseValueSource::Animation), WEX::Common::Exception, [](WEX::Common::Exception ex) {return ex.ErrorCode() == E_NOTFOUND; });

            RunOnUIThread([&] {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                // Go to the visual state that the new setter was added.
                VERIFY_IS_TRUE(VisualStateManager::GoToState(page, "TemporaryState", false));
            });

            TestServices::WindowHelper->WaitForIdle();

            // Verify that the value changed
            auto animatedWidth = GetPropertyChainValue(button.Handle, L"Width", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(std::stoi(animatedWidth.Value), 200);

            // Clear the name and validate that we can't go to it anymore.
            ClearProperty(unnamedVisualState, L"Microsoft.UI.Xaml.VisualState.Name");
            RunOnUIThread([&] {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                // Go to the visual state that the new setter was added.
                VERIFY_IS_FALSE(VisualStateManager::GoToState(page, "TemporaryState", false));
            });

            // Verify that the value hasn't changed
            animatedWidth = GetPropertyChainValue(button.Handle, L"Width", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(std::stoi(animatedWidth.Value), 200);
        }

        void AnimatedValueTests::TestChangingLocalValueWithAnimated()
        {
            TestChangeLocalValueWhileAnimated(false, false);
        }

        void AnimatedValueTests::TestChangingLocalValueWithAnimated_Override()
        {
            TestChangeLocalValueWhileAnimated(true, false);
        }

        void AnimatedValueTests::TestSetBindingWithAnimated()
        {
            TestChangeLocalValueWhileAnimated(false, true);
        }

        void AnimatedValueTests::TestChangeLocalValueWhileAnimated(bool override, bool setBinding)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            VisualElement parent = callback->GetElementByName(L"LayoutRoot");

            // Get the visual group collection. We expect there to be three groups
            auto visualGroupCollection = GetVisualStateGroupsForElement(parent.Handle);
            VERIFY_ARE_EQUAL(visualGroupCollection.Count, 3u);

            // Get the actual visual state groups.
            auto visualStateCollection = GetStatesInVisualStateGroup(std::stoll(visualGroupCollection.Elements[0].Value));

            // Verify we can add a setter to this visual state collection and have it be reflected
            InstanceHandle redBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Red");

            // Set the button as the target.
            VisualElement button = callback->GetElementByName(L"visualStateManualTest");
            InstanceHandle visualState = std::stoll(visualStateCollection.Elements[0].Value);
            auto setter = CreateSetterWithTarget(redBrush, button.Handle, L"Background");

            AddSetterToVisualState(setter, visualState, 1u);

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, GetBackgroundColor(button.Handle, BaseValueSourceLocal));

            RunOnUIThread([&] {
                auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                // Go to the visual state that the new setter was added.
                VERIFY_IS_TRUE(VisualStateManager::GoToState(page, "VisualStateTriggerTest", false));
            });

            TestServices::WindowHelper->WaitForIdle();

            // Get the animated background color and make sure that baby is red
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, GetBackgroundColor(button.Handle, BaseValueSource::Animation));

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, GetBackgroundColor(button.Handle, BaseValueSourceLocal));
            auto localBrush = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceLocal);

            // If setting local, then change the brush on the button to blue. Do this through the public API, as if the
            // developer set the property themselves. This should result in the local changes overriding the animated ones,
            // but the animated value should take precedence once modifying the local value
            if (override)
            {
                RunOnUIThread([&]
                {
                    auto buttonObj = GetFromInstanceHandle<xaml_controls::Button>(button.Handle);

                    // Update to something different, but not what we expect. Make sure that since the value was set from
                    // code behind, that it actually gets updated.
                    buttonObj->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
                });

                RunOnUIThread([&]
                {
                    auto buttonObj = GetFromInstanceHandle<xaml_controls::Button>(button.Handle);

                    // Validate that the color actually is blue
                    VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<xaml_media::SolidColorBrush^>(buttonObj->Background)->Color);

                    // Validate the animated value is marked as overwritten, and the local value isn't
                    auto animatedBrush2 = GetPropertyChainValue(button.Handle, L"Background", BaseValueSource::Animation);
                    VERIFY_IS_TRUE(animatedBrush2.Overridden);

                    auto localBrush2 = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceLocal);
                    VERIFY_IS_FALSE(localBrush2.Overridden);
                });
            }

            std::wstring expectedLocalColor = L"Yellow";
            InstanceHandle setValueHandle = 0;
            if (setBinding)
            {
                setValueHandle = CreateBinding(L"unnamedVisualStateTest", L"Background");
                auto bindElem = callback->GetElementByName(L"unnamedVisualStateTest");
                auto background = GetPropertyChainValue(bindElem.Handle, L"Background", BaseValueSourceLocal);
                expectedLocalColor = background.Value;
            }
            else
            {
                setValueHandle = CreateInstance(localBrush.Type, expectedLocalColor);
            }

            // Set the local brush to yellow
            VERIFY_SUCCEEDED(TrySetPropertyByIndex(button.Handle, setValueHandle, localBrush.Index));

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                auto buttonObj = GetFromInstanceHandle<xaml_controls::Button>(button.Handle);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, safe_cast<xaml_media::SolidColorBrush^>(buttonObj->Background)->Color);
            });

            // Verify the reported animated color is still Red and the local value is the expected value.
            if (!setBinding)
            {
                // Setting a binding causes animated values to no longer show.
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, GetBackgroundColor(button.Handle, BaseValueSource::Animation));
            }

            auto background = GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceLocal);
            auto evaluatedValue = GetPropertyChainValue(std::stoll(background.Value), setBinding ? L"EvaluatedValue" : L"Color", BaseValueSourceLocal);
            std::wstring actualLocalColor(evaluatedValue.Value);
            VERIFY_ARE_EQUAL(expectedLocalColor, actualLocalColor);
            VERIFY_IS_FALSE(evaluatedValue.Overridden);

            // Verify at the end of all this that the Setter didn't actually change
            RunOnUIThread([&] {
                auto setterVal = GetProperty<xaml_media::ISolidColorBrush>(setter, L"Microsoft.UI.Xaml.Setter.Value");
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, setterVal->Color);
            });
        }

        void AnimatedValueTests::TestCanRemoveAnimationFromStoryboard()
        {
            TestRemoveAnimationFromStoryboard(false);
        }

        void AnimatedValueTests::TestCanRemoveAnimationFromStoryboardWhileActive()
        {
            TestRemoveAnimationFromStoryboard(true);
        }

        void AnimatedValueTests::TestRemoveAnimationFromStoryboard(bool isActive)
        {
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
            auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

            auto vsRoot = callback->GetElementByName(L"LayoutRoot");
            if (isActive)
            {
                RunOnUIThread([&]
                {
                    auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                    VERIFY_IS_TRUE(VisualStateManager::GoToState(page, "VisualStateManualTest", false));
                });
            }

            // Get the visual group and create the animation and add it to the storyboard.

            auto visualGroupCollection = GetNamedVisualStateGroupForElement(vsRoot.Handle, L"Manual");
            auto visualState = GetNamedVisualStateInGroup(visualGroupCollection, L"VisualStateManualTest");
            VERIFY_ARE_NOT_EQUAL(visualState, 0u);

            auto storyboard = GetVisualStateProperty(visualState, L"Storyboard");
            VERIFY_ARE_NOT_EQUAL(storyboard.Handle, 0u);

            auto storyBoardChildren = GetCollectionProperty(storyboard.Handle, L"Microsoft.UI.Xaml.Media.Animation.Storyboard.Children");

            VERIFY_SUCCEEDED(m_tap->RemoveChild(storyBoardChildren.Handle, 0u));
        }

        void AnimatedValueTests::CanSetStoryboardTargetName()
        {
            // Set window size to 610. The trigger we are modifying is set at a MinWindowWidth of 600.
            ::Windows::Foundation::Size size(610, 610);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto gridString = ref new Platform::String(
                L"<Page"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid x:Name='root' Background='{ThemeResource ApplicationPageBackgroundThemeBrush}'>"
                L"    <VisualStateManager.VisualStateGroups>"
                L"      <VisualStateGroup x:Name='Group'>"
                L"        <VisualState x:Name='State'>"
                L"          <VisualState.StateTriggers>"
                L"            <AdaptiveTrigger MinWindowWidth='600'/>"
                L"          </VisualState.StateTriggers>"
                L"          <Storyboard>"
                L"            <ColorAnimation Storyboard.TargetName='buttonForeground' Storyboard.TargetProperty='Color' To='Red'/>"
                L"          </Storyboard>"
                L"        </VisualState>"
                L"      </VisualStateGroup>"
                L"    </VisualStateManager.VisualStateGroups>"
                L"    <Button x:Name='myButton' Content='Hello World'>"
                L"      <Button.Foreground>"
                L"         <SolidColorBrush x:Name='buttonForeground' Color='Purple'/>"
                L"      </Button.Foreground>"
                L"      <Button.Background>"
                L"        <SolidColorBrush x:Name='buttonBackground' Color='Black'/>"
                L"      </Button.Background>"
                L"    </Button>"
                L"  </Grid>"
                L"</Page>");

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(gridString, callback);

            auto root = callback->GetElementByName(L"root");

            auto stateGroup = GetNamedVisualStateGroupForElement(root.Handle, L"Group");
            auto state = GetNamedVisualStateInGroup(stateGroup, L"State");
            auto storyboard = GetVisualStateProperty(state, L"Storyboard");

            auto storyBoardChildren = GetCollectionProperty(storyboard.Handle, L"Microsoft.UI.Xaml.Media.Animation.Storyboard.Children");

            InstanceHandle animation = std::stoll(storyBoardChildren.Elements[0].Value);

            auto button = callback->GetElementByName(L"myButton");
            auto backgroundBrush = GetProperty<xaml_media::SolidColorBrush>(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background");
            auto foregroundBrush = GetProperty<xaml_media::SolidColorBrush>(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Foreground");
            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, foregroundBrush->Color);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Black, backgroundBrush->Color);
            });

            auto elementName = CreateInstance(L"Windows.Foundation.String", L"buttonBackground");
            SetProperty(animation, elementName, L"Microsoft.UI.Xaml.Media.Animation.Storyboard.TargetName");

            TestServices::WindowHelper->WaitForIdle();
            // Verify it's updated
            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Red, backgroundBrush->Color);
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Purple, foregroundBrush->Color);
            });
        }

        void AnimatedValueTests::CanRemoveSetterWithBadTarget()
        {
            auto gridString = ref new Platform::String(
                L"<Page"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid x:Name='root' Background='{ThemeResource ApplicationPageBackgroundThemeBrush}'>"
                L"    <VisualStateManager.VisualStateGroups>"
                L"      <VisualStateGroup x:Name='Group'>"
                L"        <VisualState x:Name='State'>"
                L"          <VisualState.StateTriggers>"
                L"            <AdaptiveTrigger MinWindowWidth='10'/>"
                L"          </VisualState.StateTriggers>"
                L"          <VisualState.Setters>"
                L"          </VisualState.Setters>"
                L"        </VisualState>"
                L"      </VisualStateGroup>"
                L"    </VisualStateManager.VisualStateGroups>"
                L"    <Button Name='myButton' Background='Yellow' Foreground='Purple' Content='Hello World'/>"
                L"  </Grid>"
                L"</Page>");

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(gridString, callback);

            auto root = callback->GetElementByName(L"root");
            auto stateGroup = GetNamedVisualStateGroupForElement(root.Handle, L"Group");
            auto state = GetNamedVisualStateInGroup(stateGroup, L"State");
            auto button = callback->GetElementByName(L"myButton");

            auto blueBrush = CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", L"Blue");
            auto setter = CreateSetterWithTarget(blueBrush, button.Handle, L"For");

            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Purple, GetColorProperty(button.Handle, L"Foreground", BaseValueSourceLocal));

            AddSetterToVisualState(setter, state, 0u);

            // Ensure the invalid setter was added
            auto settersAfterAdd = GetVisualStateProperty(state, L"Setters");
            VERIFY_ARE_EQUAL(1u, settersAfterAdd.Count);
            VERIFY_ARE_EQUAL(setter, std::stoull(settersAfterAdd.Elements[0].Value));

            VERIFY_SUCCEEDED(m_tap->RemoveChild(settersAfterAdd.Handle, 0u));

            auto settersAfterRemove = GetVisualStateProperty(state, L"Setters");
            VERIFY_ARE_EQUAL(0u, settersAfterRemove.Count);

            // Make sure the foreground hasn't changed
            VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Purple, GetColorProperty(button.Handle, L"Foreground", BaseValueSourceLocal));
        }

        void AnimatedValueTests::TestNoLocalPropertyChainWhenAnimated()
        {
            auto gridString = ref new Platform::String(
                L"<Page"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <Grid x:Name='root' Background='{ThemeResource ApplicationPageBackgroundThemeBrush}'>"
                L"    <VisualStateManager.VisualStateGroups>"
                L"      <VisualStateGroup x:Name='Group'>"
                L"        <VisualState x:Name='State'>"
                L"          <VisualState.StateTriggers>"
                L"            <AdaptiveTrigger MinWindowWidth='600'/>"
                L"          </VisualState.StateTriggers>"
                L"          <VisualState.Setters>"
                L"            <Setter Target='myButton.Background' Value='Green' />"
                L"            <Setter Target='myButtonStyled.Background' Value='Orange' />"
                L"          </VisualState.Setters>"
                L"        </VisualState>"
                L"      </VisualStateGroup>"
                L"    </VisualStateManager.VisualStateGroups>"
                L"    <Grid.Resources>"
                L"      <Style x:Key='buttonStyle' TargetType='Button'>"
                L"        <Setter Property='Background' Value='Yellow' />"
                L"      </Style>"
                L"    </Grid.Resources>"
                L"    <Button x:Name='myButton' Content='Hello' />"
                L"    <Button x:Name='myButtonStyled' Content='World' Style='{StaticResource buttonStyle}' />"
                L"  </Grid>"
                L"</Page>");

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(gridString, callback);

            auto button = callback->GetElementByName(L"myButton");

            // Verify the Visual State's Green color was applied
            auto foregroundBrush = GetProperty<xaml_media::SolidColorBrush>(button.Handle, L"Microsoft.UI.Xaml.Controls.Control.Background");
            RunOnUIThread([&]
            {
                // First, check the Animation base value source
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, GetBackgroundColor(button.Handle, BaseValueSource::Animation));

                // Second, the actual value that's currently rendering
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Green, foregroundBrush->Color);
            });

            // Verify GetPropertyChainValue for a local value throws, since it shouldn't be in the property chain
            VERIFY_THROWS(GetPropertyChainValue(button.Handle, L"Background", BaseValueSourceLocal), WEX::Common::Exception);

            //Verify we have a styled and animated value for the styled button, but not a local value
            auto styledButton = callback->GetElementByName(L"myButtonStyled");

            RunOnUIThread([&]
            {
                // First, check the Animation base value source
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Orange, GetBackgroundColor(styledButton.Handle, BaseValueSource::Animation));

                // Check the styled value
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Yellow, GetBackgroundColor(styledButton.Handle, BaseValueSourceStyle));
            });

            // Verify there's no local value on the styled button
            VERIFY_THROWS(GetPropertyChainValue(styledButton.Handle, L"Background", BaseValueSourceLocal), WEX::Common::Exception);
        }

        void AnimatedValueTests::AnimateCustomDP()
        {
            Storyboard^ myStoryboard;
            VisualElement myControl;

            auto storyboardCompletedEvent = std::make_shared<Event>();
            auto storyboardCompletedRegistration = CreateSafeEventRegistration(xaml_animation::Storyboard, Completed);

            Platform::String^ xamlString =
                L"<StackPanel"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"    xmlns:local='using:Tests.Tools.Shared'"
                L"    x:Name='root'>"
                L"    <StackPanel.Resources>"
                L"        <Storyboard x:Name='myStoryboard' Duration='0:0:1'>"
                L"            <DoubleAnimation EnableDependentAnimation='True' Storyboard.TargetName='myControl' Storyboard.TargetProperty='CustomInt' To='42' />"
                L"        </Storyboard>"
                L"    </StackPanel.Resources>"
                L"    <local:CustomUserControl x:Name='myControl' CustomInt='5' />"
                L"</StackPanel>";

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlString, callback);

            auto root = callback->GetElementByName(L"root");

            unsigned int indexGetPropertyIndex = 0;
            VERIFY_SUCCEEDED(m_tap->GetPropertyIndex(root.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources", &indexGetPropertyIndex));
            InstanceHandle dictionaryHandle;
            VERIFY_SUCCEEDED(m_tap->GetProperty(root.Handle, indexGetPropertyIndex, &dictionaryHandle));
            ResourceDictionary^ dictionary = safe_cast<ResourceDictionary^>(reinterpret_cast<Platform::Object^>(reinterpret_cast<IInspectable*>(dictionaryHandle)));

            myControl = callback->GetElementByName(L"myControl");
            wil::unique_propertychainvalue customIntProperty = GetPropertyChainValue(myControl.Handle, L"CustomInt");

            int valBeforeAnim = std::stoi(customIntProperty.Value);

            RunOnUIThread([&] {
                myStoryboard = safe_cast<Storyboard^>(dictionary->Lookup(L"myStoryboard"));

                storyboardCompletedRegistration.Attach(safe_cast<Storyboard^>(myStoryboard),
                    ref new ::Windows::Foundation::EventHandler<Platform::Object ^>(
                        [this, storyboardCompletedEvent, valBeforeAnim, myControl](Platform::Object ^sender, Platform::Object ^e)
                {
                    storyboardCompletedEvent->Set();

                    wil::unique_propertychainvalue animationBaseValue = GetPropertyChainValue(myControl.Handle, L"CustomInt");
                    VERIFY_ARE_EQUAL(valBeforeAnim, std::stoi(animationBaseValue.Value));
                }));

                myStoryboard->Begin();
            });

            storyboardCompletedEvent->WaitForDefault();

            customIntProperty = GetPropertyChainValue(myControl.Handle, L"CustomInt", BaseValueSource::Animation);
            VERIFY_ARE_EQUAL(42, std::stoi(customIntProperty.Value), L"CustomInt did not change");
        }

        void AnimatedValueTests::DontResumePausedStoryboard()
        {
            Platform::String^ xamlString =
                L"<StackPanel"
                L"    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"    xmlns:local='using:Tests.Tools.Shared'"
                L"    x:Name='root'>"
                L"    <StackPanel.Resources>"
                L"        <Storyboard x:Name='Storyboard1'>"
                L"            <ColorAnimation Duration='0:0:4' Storyboard.TargetName='rect' Storyboard.TargetProperty='(Rectangle.Fill).(SolidColorBrush.Color)' To='Pink' />"
                L"        </Storyboard>"
                L"    </StackPanel.Resources>"
                L"    <Rectangle x:Name='rect' Fill='Red' Width='100' Height='100' />"
                L"</StackPanel>";

            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlString, callback);

            auto root = callback->GetElementByName(L"root");
            auto storyboard1Handle = GetItemFromElementResources(root, L"Storyboard1");
            auto storyboard = ih_cast<xaml_media::Animation::Storyboard>(storyboard1Handle);

            auto rect = callback->GetElementByName(L"rect");
            auto rectElement = ih_cast<xaml_shapes::Rectangle>(rect.Handle);
            RunOnUIThread([&] {
                storyboard->Stop();
                storyboard->Begin();
                wf::TimeSpan one_second{ HNS_FROM_SECOND(1)};
                storyboard->Seek(one_second);
                storyboard->Pause();
            });

            TestServices::WindowHelper->WaitForIdle();

            auto animation = GetCollectionItem(storyboard1Handle, L"Microsoft.UI.Xaml.Media.Animation.Storyboard.Children", 0);

            auto blueBrush = CreateInstance(L"Windows.UI.Color", L"Blue");
            SetProperty(animation, blueBrush,  L"Microsoft.UI.Xaml.Media.Animation.ColorAnimation.To");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&] {
                 // We're at 1 second, so we shouldn't be fully red nor fully blue
                VERIFY_ARE_NOT_EQUAL(Microsoft::UI::Colors::Red, safe_cast<xaml_media::SolidColorBrush^>(rectElement->Fill)->Color);
                VERIFY_ARE_NOT_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<xaml_media::SolidColorBrush^>(rectElement->Fill)->Color);
            });

            VERIFY_IS_FALSE(TestServices::WindowHelper->IsStoryboardActive(storyboard));
            // Seek to 4 seconds, the storyboard should complete
            RunOnUIThread([&] {
                wf::TimeSpan four_seconds{ HNS_FROM_SECOND(4) };
                storyboard->Seek(four_seconds);
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&] {
                // Make sure that the fill of the rectangle is now Blue
                VERIFY_ARE_EQUAL(Microsoft::UI::Colors::Blue, safe_cast<xaml_media::SolidColorBrush^>(rectElement->Fill)->Color);
            });

        }

        void AnimatedValueTests::CanChangeSetterValueTargetingBrush()
        {
            auto xamlString = ref new Platform::String(
                L"<Page\r\n"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'\r\n"
                L"  xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>\r\n"
                L"  <Page.Resources>\r\n"
                L"     <Style TargetType='Button'>\r\n"
                L"       <Setter Property='Template'>\r\n"
                L"         <Setter.Value>\r\n"
                L"              <ControlTemplate TargetType='Button'>\r\n"
                L"                <Grid x:Name='ButtonTemplateRoot' Background='Yellow'>\r\n"
                L"                  <VisualStateManager.VisualStateGroups>\r\n"
                L"                    <VisualStateGroup x:Name='CommonStates'>\r\n"
                L"                      <VisualState x:Name='Normal'>\r\n"
                L"                        <VisualState.Setters>\r\n"
                L"                          <Setter Target='ButtonTemplateRectangle.(Rectangle.Fill).(SolidColorBrush.Color)' Value='Green' />\r\n"
                L"                        </VisualState.Setters>\r\n"
                L"                      </VisualState>\r\n"
                L"                    </VisualStateGroup>\r\n"
                L"                  </VisualStateManager.VisualStateGroups>\r\n"
                L"                  <Rectangle Name='ButtonTemplateRectangle' Width='100' Height='100' Fill='Magenta'/>\r\n"
                L"                </Grid>\r\n"
                L"              </ControlTemplate>\r\n"
                L"         </Setter.Value>\r\n"
                L"       </Setter>\r\n"
                L"     </Style>\r\n"
                L"  </Page.Resources>\r\n"
                L"  <StackPanel x:Name='root' Background='Blue'>\r\n"
                L"    <Button x:Name='button' Width='100' Height='100'/>\r\n"
                L"  </StackPanel>\r\n"
                L"</Page>\r\n");
            wrl::ComPtr<VisualTreeServiceCallback> callback;
            auto cleanup = m_connectionHelper->Advise(xamlString, callback);
            LOG_OUTPUT(L"Test control template scenario");
            // Check the ControlTemplate case
            TestChangeSetterValueTargetingBrush(
                callback,
                L"ButtonTemplateRectangle",
                L"ButtonTemplateRoot",
                Microsoft::UI::Colors::Green,
                Microsoft::UI::Colors::Black);
        }

        void AnimatedValueTests::TestChangeSetterValueTargetingBrush(
            const wrl::ComPtr<VisualTreeServiceCallback>& callback,
            const std::wstring& rectName,
            const std::wstring& vsmRoot,
            const ::Windows::UI::Color& before,
            const ::Windows::UI::Color& after)
        {
            VisualElement rectangle = callback->GetElementByName(rectName.c_str());

            VisualElement vsRoot = callback->GetElementByName(vsmRoot.c_str());
            auto visualGroupCollection = GetNamedVisualStateGroupForElement(vsRoot.Handle, L"CommonStates");
            auto normalState = GetNamedVisualStateInGroup(visualGroupCollection, L"Normal");
            auto setter = GetCollectionItem(normalState, L"Microsoft.UI.Xaml.VisualState.Setters", 0);

            auto newBrush = CreateInstance(L"Windows.UI.Color", GetColorName(after));

            auto rect = ih_cast<xaml_shapes::Rectangle>(rectangle.Handle);
            RunOnUIThread([&] {
                auto brush = safe_cast<SolidColorBrush^>(rect->Fill);
                VERIFY_ARE_EQUAL(before, brush->Color);
            });
            SetProperty(setter, newBrush, L"Microsoft.UI.Xaml.Setter.Value");

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&] {
                auto brush = safe_cast<SolidColorBrush^>(rect->Fill);
                VERIFY_ARE_EQUAL(after, brush->Color);
            });
        }

        void AnimatedValueTests::PointerThemeUpAnimationDoesntAVWithInvalidTarget()
        {
            /*
            Imagine the following markup:
            <VisualStateGroup x:Name="Manual">
              <VisualState x:Name="VisualStateManualTest">
                <Storyboard x:Name="myStoryboard2">
                  <DoubleAnimation EnableDependentAnimation="True" From="110" To="300" Duration="00:00:1"
                    Storyboard.TargetName="visualStateManualTest"
                    Storyboard.TargetProperty="Height">
                   <DoubleAnimation.EasingFunction>
                        <BounceEase Bounces="2" EasingMode="EaseOut" Bounciness="2" />
                   </DoubleAnimation.EasingFunction>
                  </DoubleAnimation>
                </Storyboard>
                <VisualState.Setters>
                  <Setter Target="visualStateManualTest.Width" Value="300" />
                </VisualState.Setters>
              </VisualState>
            </VisualStateGroup>
            <Button x:Name="visualStateManualTest" Height="100" Width="100">
              <Button.Background>
                <SolidColorBrush x:Name="visualStateManualTestBrush" Color="Green"/>
              </Button.Background>
            </Button>

            After (edits noted by *****):
            <VisualStateGroup x:Name="Manual">
              <VisualState x:Name="VisualStateManualTest">
                <Storyboard x:Name="myStoryboard2">
                  <DoubleAnimation EnableDependentAnimation="True" From="110" To="300" Duration="00:00:1"
                    Storyboard.TargetName="visualStateManualTest"
                    Storyboard.TargetProperty="Height">
                   <DoubleAnimation.EasingFunction>
                        <BounceEase Bounces="2" EasingMode="EaseOut" Bounciness="2" />
                   </DoubleAnimation.EasingFunction>
                  </DoubleAnimation>
              *****<PointerUpThemeUpAnimation Storyboard.TargetName = "visualStateManual" />
                   <PointerDownThemeAnimation Storyboard.TargetName = "visualStateManual" />*****
                </Storyboard>
                <VisualState.Setters>
                  <Setter Target="visualStateManualTest.Width" Value="300" />
                </VisualState.Setters>
              </VisualState>
            </VisualStateGroup>
            <Button x:Name="visualStateManualTest" Height="100" Width="100">
              <Button.Background>
                <SolidColorBrush x:Name="visualStateManualTestBrush" Color="Green"/>
              </Button.Background>
            </Button>
            */
            for (int i = 0; i < 2; i++)
            {
                const bool isActive = i == 0;
                LOG_OUTPUT(L"Is Active: %s", isActive ? L"true" : L"false");
                wrl::ComPtr<VisualTreeServiceCallback> callback;
                ::Windows::Foundation::Uri^ componentLocation = ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/VisualStatePage.xaml");
                auto cleanup = m_connectionHelper->Advise(componentLocation, callback);

                VisualElement vsRoot = callback->GetElementByName(L"LayoutRoot");
                InstanceHandle pointerUpThemeAnimation = CreateAnimationWithTarget(L"PointerUpThemeAnimation", L"visualStateManual"); // Invalid, verify don't crash
                InstanceHandle pointerDownThemeAnimation = CreateAnimationWithTarget(L"PointerDownThemeAnimation", L"visualStateManual"); // Invalid, verify don't crash
                if (isActive)
                {
                    // If active before adding, then do so.
                    RunOnUIThread([&] {
                        auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                        VERIFY_IS_TRUE(VisualStateManager::GoToState(page, "VisualStateManualTest", false));
                    });
                }

                AddAnimationToStoryboard(vsRoot.Handle, L"Manual", L"VisualStateManualTest", pointerUpThemeAnimation);
                AddAnimationToStoryboard(vsRoot.Handle, L"Manual", L"VisualStateManualTest", pointerDownThemeAnimation);

                if (!isActive)
                {
                    // If not active before adding, then do so right meow.
                    RunOnUIThread([&] {
                        auto page = safe_cast<Page^>(TestServices::WindowHelper->WindowContent);
                        VERIFY_THROWS_WINRT(VisualStateManager::GoToState(page, "VisualStateManualTest", false), Platform::COMException^);
                    });
                }

                TestServices::WindowHelper->WaitForIdle();
            }
        }
        #pragma endregion

        ::Windows::UI::Color  AnimatedValueTests::GetBackgroundColor(InstanceHandle control, BaseValueSource source)
        {
            auto backgroundProp = GetPropertyChainValue(control, L"Background", source);
            ::Windows::UI::Color color;
            RunOnUIThread([&] {
                auto brush = GetFromInstanceHandle<xaml_media::SolidColorBrush>(std::stoll(backgroundProp.Value));
                color = brush->Color;
            });

            return color;
        }

    }
} } } } }
