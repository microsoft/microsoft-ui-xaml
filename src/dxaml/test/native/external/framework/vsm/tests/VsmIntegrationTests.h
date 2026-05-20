// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CustomMetadataRegistrar.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {
        class VsmIntegrationTests : public WEX::TestClass<VsmIntegrationTests>
        {

        public:
            BEGIN_TEST_CLASS(VsmIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"f62323d4-fd46-4c98-aa85-334db95ba8f6;0a9cdf5f-1e1b-4b1d-9659-b354bf5f4ca6")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(TextParsingStoryboardAssociationRegression)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that deferred text-parsed storyboards don't associate.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReferenceToControlTemplateResources)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that reference from VSM to ControlTemplate resources works")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FindNameOutsideControlTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the VSM faults in when outside of a ControlTemplate in XBFv2")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TransitionNamesAreCaseInsensitive)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FaultedInRelativeBindingEvaluation)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when a VSM is faulted in RelativeBinding are properly evaluated when "
                    L"Storyboards are started. VisualStates have a special Enter/Leave implementation for Storyboard because Storyboard is "
                    L"a method-based property and must manually propagate inheritance context changes.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextParsingRelativeBindingEvaluation)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when running in Threshold a text-parsed VSM exhibits the correct "
                    L"enter/leave behavior and that Binding are evaluated in the correct context")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VisualTransitionsWithDynamicTimelinesNonzero)
                TEST_METHOD_PROPERTY(L"Description", L"When a VisualTransition ONLY contains DynamicTimelines those timelines need to be "
                    L"fully-expanded before VSM determines if the VisualTransition is of zero duration.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ReRegistrationOfTemplatedNames)
                TEST_METHOD_PROPERTY(L"Description", L"When a deferred element with an x:Name specified is created, destroyed, and recreated its "
                    L"name will be reregistered. The ValueStore for the name, even if it's for a template, must allow this duplicate registration.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FaultedInTransitionNonMatchingStateName)
                TEST_METHOD_PROPERTY(L"Description", L"When transitioning from one state, VS1, to another state, VS2, the algorithm "
                    L"that identifies the VisualTransition should discredit a VisualTransition if a single transition matches but "
                    L"the other string doesn't match AND the other string doesn't match ANY other VisualStates in the VisualStateGroup.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FindNameFaultsInChildrenInTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when FindName occurs on a deferred element of "
                    L" a VSM that the VSM is faulted in and all names are properly registered.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FindNameWorksForTransitioningVsm)
                TEST_METHOD_PROPERTY(L"Description", L"Ensures that when a VSM is faulted in and moved to a new NameScope owner "
                    L"in a none-template scenario that FindName continues to work")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GoToStateWithPiledUpStoryboardCompletionEvents)
                TEST_METHOD_PROPERTY(L"Description", L"Ensures that in the faulted-in case duplicated OnCompleted events are handled gracefully when the VSM has "
                    L"seen a tree leave between them.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VsmAppliesFirstAndLastFrameOfTransitionAnimationSynchronously)
                TEST_METHOD_PROPERTY(L"Description", L"Ensures VSM will apply the last frame of a VisualTransition animation when "
                    L"system animations are disabled.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PeerLifetimePreservedDuringAnimation)
                TEST_METHOD_PROPERTY(L"Description", L"Ensures lifetime of DXaml peer is preserved during an animation.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GoToStateWithRunningTransitionStoryboard)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CannotTargetNonControlInOverridenVsm)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GoToStateWithoutTransitionsReappliesBindings)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AnimationsDisabledDynamicTransitionOnlyApplied)
                TEST_METHOD_PROPERTY(L"Description", L"When system animations are disabled and we have a VisualTransition with "
                    L"a duration specified but no storyboard we ensure the dynamic transition is generated and the destination state is "
                    L"applied.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyGeneratedTransitionBetweenEmptyAndSetter)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that VisualTransition animations are generated for properties interpolating "
                    L"back and forth between an empty VisualState and a VisualState that uses Setters to set those properties.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyGeneratedTransitionBetweenSetterAndSetter)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that VisualTransition animations are generated for properties interpolating "
                    L"back and forth between VisualStates that use Setters to set those properties.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()
                  
            BEGIN_TEST_METHOD(VerifyGeneratedTransitionBetweenSetterAndStoryboard)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that VisualTransition animations are generated for properties interpolating "
                    L"back and forth between a VisualState that uses Storyboards to set those properties and a VisualState that uses Setters instead.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MultiStorybardDynamicTimelinesDeduplicate)
                TEST_METHOD_PROPERTY(L"Description", L"When using dynamic transitions in the final state and having a VisualTransition of "
                    L"a non-zero duration that doesn't contain the dynamic transition, and when the dynamic transition has more than "
                    L"one storyboard a bug was causing us to duplicate the timelines for EVERY transition, leading to more than one "
                    L"storyboard targeting the same property and an application crash. This ensures that only a single set of storyboards "
                    L"are generated.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVsmSettersSetCustomAttachedPropertiesOptimized)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that Visual States can set Custom Attached Properties using Setters.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyVsmSettersSetCustomAttachedPropertiesNonOptimized)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that Visual States can set Custom Attached Properties using Setters, retrieves VisualStateGroup "
                    L"to force a non-optimized code path.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyDoNotCrashOnSettersWithBadlyAuthoredFullyQualifiedPropertyPaths)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that a VSM Setter with an incorrectly authored fully qualified property name does not "
                    L"result in a crash when attempting to pre-resolve custom attached properties.")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;
            void RelativeBindingBaseTest();
            void VerifyVsmSettersSetCustomAttachedProperties(bool optimized);
            void PrepareCustomAttachedPropertiesRoot(Microsoft::UI::Xaml::Controls::Button^& root, Microsoft::UI::Xaml::Controls::Button^& targetControl, bool optimized);
            void ValidateCustomAttachedProperties(Microsoft::UI::Xaml::Controls::Button^ targetControl, Platform::String^ stringValue, int intValue, ::Windows::UI::Color brushColor, ::Windows::UI::Color indexedColor1, ::Windows::UI::Color indexedColor2);
        };
    }
} } } }

