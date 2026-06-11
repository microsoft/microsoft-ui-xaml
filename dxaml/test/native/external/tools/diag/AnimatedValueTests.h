// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <memory>
#include <map>
#include <tuple>
#include <wrl\module.h>
#include <wil\resource.h>
#include "XamlOM.WinUI.h"
#include <TestCleanupWrapper.h>
#include "XamlDiagnosticsHelper.h"
#include "XamlDiagnosticsTestBase.h"
#include "RuntimeEnabledFeatureOverride.h"
using namespace Microsoft::UI::Xaml::Controls;
namespace wrl = Microsoft::WRL;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {

        struct AnimationChangedParams
        {
            int Width;
            int Height;
            ::Windows::UI::Color Color;
        };

        struct TriggerChangedTestCase
        {
            wf::Size WindowSize;
            double NewTriggerMinWindowWidth;
            AnimationChangedParams BeforeValues;
            AnimationChangedParams AfterValues;
        };

        inline TriggerChangedTestCase MakeTestCase(wf::Size size, double newVal, AnimationChangedParams before, AnimationChangedParams after)
        {
            return TriggerChangedTestCase{ size, newVal, before, after };
        }

        class AnimatedValueTests : public BaseTestClass<AnimatedValueTests>
        {
        public:
            BEGIN_TEST_CLASS(AnimatedValueTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\xamlom.idl")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(TestAnimatedTriggers)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that adaptive triggers work correctly with XamlDiagnostics in terms of source info and base/animated values")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestAnimatedTriggersXBF)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that adaptive triggers work correctly with XamlDiagnostics in terms of source info and base/animated values with V2 XBF")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestAnimatedValuesManual)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that going to a visual state with VisualStateManager::GoToState properly sets animated values and sources")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestAnimatedValuesManualXBF)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that going to a visual state with VisualStateManager::GoToState properly sets animated values and sources with V2 XBF")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestAnimatedValuesStoryboardInDictionary)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that a Storyboard outside of a VisualState is correctly treated as an animated value")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestAnimatedValuesStoryboardInDictionaryXBF)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that a Storyboard outside of a VisualState is correctly treated as an animated value with V2 XBF")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanAddVsmSetters)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can add a setter to VisualState.Setters")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanAddVsmSettersToActiveState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can add a setter to VisualState.Setters while the state is active")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanRemoveVsmSettersPreState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can remove a setter from VisualState.Setters before in the state")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanRemoveVsmSettersInActiveState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can remove a setter from VisualState.Setters before in the state")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanModifyVsmSettersInUnnamedState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can modify a setter in an unnamed state while the state is active")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanModifyExistingVsmTriggersInExistingVisualState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can modify adaptive triggers in vsm")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanModifyNewVsmTriggersInNewVisualState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can modify adaptive triggers in vsm")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanModifyNewVsmTriggersInExistingVisualState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can modify adaptive triggers in vsm")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

             BEGIN_TEST_METHOD(CanModifyVsmTriggersInUnnamedState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can modify adaptive triggers in an unnamed visual state")
                 TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
             END_TEST_METHOD()

             BEGIN_TEST_METHOD(TestCanAddTrigger)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can add a trigger to VisualState.Triggers")
                 TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanRemoveTriggerPreState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can remove a trigger from VisualState.Triggers before entering the state")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanRemoveTriggerInState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can remove a trigger from VisualState.Triggers while in the current state")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanAddVisualState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can add a visual state to a visual state group")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanRemoveVisualState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can modify setters in vsm")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanAddAnimationToStoryboard)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can add an animation to a storyboard")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanAddAnimationToStoryboardWhileActive)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can add an animation to a storyboard that's active")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanAddAnimationWithNamedTarget)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can add an animation to a storyboard that targets an element in a control template")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanAddAnimationWithNamedTargetToNewStoryboard)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can add an animation to a newly created storyboard that targets an element in a control template")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanRemoveNewlyCreatedStoryboard)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can add an animation to a newly created storyboard that targets an element in a control template")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanAddCommonStateInControlTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can add basic states such as 'PointerOver' and have them be applied")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanRemoveAnimation)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can remove an animation from a non-active state")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanRemoveAnimationInCurrentState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can remove an animation from the active state")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestAddRemoveVisualStateName)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can add a name to a VisualState and call GoToState and then remove it")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestChangingLocalValueWithAnimated)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we correctly handle setting local properties if there is an active animated value")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestSetBindingWithAnimated)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we correctly handle setting local properties if there is an active animated value. Setting the binding should result in the animated value updating")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestChangingLocalValueWithAnimated_Override)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we correctly handle setting local properties if there is an active animated value. This should override the local value set from code behind and change to the animated value")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanRemoveAnimationFromStoryboard)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can remove an animation from a storyboard")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestCanRemoveAnimationFromStoryboardWhileActive)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can remove an animation from a storyboard while the storyboard is active")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanSetStoryboardTargetName)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies we can update the Storyboard.TargetName property of an animation")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanRemoveSetterWithBadTarget)
                TEST_METHOD_PROPERTY(L"Description", L"Can remove a setter with a bad target that was added ")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimateCustomDP)
                TEST_METHOD_PROPERTY(L"Description", L"Jupiter modern resource loading tests that use Application::LoadComponent. Ported from Legacy()")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestNoLocalPropertyChainWhenAnimated)
                TEST_METHOD_PROPERTY(L"Description", L"Tests that when we have an animated value we don't erroneously report a local value exists when it doesn't")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DontResumePausedStoryboard)
                TEST_METHOD_PROPERTY(L"Description", L"Make sure we don't start a storyboard that was paused")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanChangeSetterValueTargetingBrush)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PointerThemeUpAnimationDoesntAVWithInvalidTarget)
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"True")  // Disabled due to #38164330
            END_TEST_METHOD()
            
        private:
            void TestAnimatedTriggersBase(bool forceXbf);
            void TestAnimatedValuesManualBase(bool forceXbf);
            void TestAnimatedValuesStoryboardInDictionaryBase(bool forceXbf);
            void TestRemoveTriggerBase(bool isCurrentlySet);
            void TestRemoveSetterBase(const std::wstring& stateName, const std::wstring& buttonName, int expectedWidth, int vsGroupIndex);
            void TestChangeLocalValueWhileAnimated(bool override, bool setBinding);
            void TestRemoveAnimationFromStoryboard(bool isActive);
            void TestAddAnimationToStoryboard(bool isActive);
            void TestCanAddVsmSetters(bool setActiveBeforeAddSetter);
            void TestModifyVsmTriggerBase(
                ::Windows::Foundation::Uri^ componentLocation,
                int groupIndex,
                int vsIndex,
                const std::wstring& vsmRoot,
                const std::wstring& buttonName,             // Name of the element we are testing
                const TriggerChangedTestCase& testCaseParams
            );

            void TestChangeSetterValueTargetingBrush(
                const Microsoft::WRL::ComPtr<Microsoft::UI::Xaml::Tests::Common::VisualTreeServiceCallback>& callback,
                const std::wstring& rectName,
                const std::wstring& vsmRoot,
                const ::Windows::UI::Color& before,
                const ::Windows::UI::Color& after);

            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureDisableTransitionsForTest;

            ::Windows::UI::Color GetBackgroundColor(InstanceHandle control, BaseValueSource source);
        };
    }
} } } } }
