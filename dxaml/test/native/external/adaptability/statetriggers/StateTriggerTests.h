// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CustomMetadataRegistrar.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Adaptability { 

        using namespace ::Windows::Foundation;
        using namespace ::Windows::UI;
        using namespace Microsoft::UI::Xaml;
        using namespace Microsoft::UI::Xaml::Controls;
        using namespace Microsoft::UI::Xaml::Markup;
        using namespace Microsoft::UI::Xaml::Tests::Common;

        using namespace test_infra;
        using namespace ::Windows::Storage::Streams;
        using namespace ::Windows::Foundation::Collections;

        class StateTriggersIntegrationTests : public WEX::TestClass<StateTriggersIntegrationTests>
        {
        public:
            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_CLEANUP(TestCleanup)
            TEST_METHOD_SETUP(TestSetup)

            BEGIN_TEST_CLASS(StateTriggersIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"c228d235-833c-46b2-858c-130c2cd3da83;f6edc090-2624-4657-9dee-b5437970e657")
            END_TEST_CLASS()


            BEGIN_TEST_METHOD(BasicMarkup)
                TEST_METHOD_PROPERTY(L"Description", L"Baseline markup. No StateTriggers.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControl)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControl_Groups)
                TEST_METHOD_PROPERTY(L"Description", L"User control, multiple groups, single VisualState, single StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControl_Groups_VisualStates)
                TEST_METHOD_PROPERTY(L"Description", L"User control, multiple groups, multiple VisualStates, single StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControl_Groups_VisualStates_StateTriggers)
                TEST_METHOD_PROPERTY(L"Description", L"User control, multiple groups, multiple VisualStates, multiple StateTriggers.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControl)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, single group, single VisualState, single StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControl_Groups)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, multiple groups, single VisualState, single StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControl_Groups_VisualStates)
                TEST_METHOD_PROPERTY(L"Description", L"User control, multiple groups, multiple VisualStates, multiple StateTriggers.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlWithTransitions)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control with StateTriggers and Transitions.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ContentControl)
                TEST_METHOD_PROPERTY(L"Description", L"Content control with StateTriggers.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Grid)
                TEST_METHOD_PROPERTY(L"Description", L"UserControl with Grid + StateTriggers with Storyboards.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GridWithSetters)
                TEST_METHOD_PROPERTY(L"Description", L"UserControl with Grid + StateTriggers with VSM Setters.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GridLegacy)
                TEST_METHOD_PROPERTY(L"Description", L"UserControl with Grid + StateTriggers with Storyboards [Legacy load path]")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GridWithSettersLegacy)
                TEST_METHOD_PROPERTY(L"Description", L"UserControl with Grid + StateTriggers with VSM Setters [Legacy load path]")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(UserControlWithExtensibleStateTriggers)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, with extensible StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlWithExtensibleStateTrigger)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, with extensible StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlWithExtensibleStateTriggersTrigger)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, with extensible StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlWithExtensibleStateTriggerTrigger)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, with extensible StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlWithMultipleExtensibleStateTriggersTrigger)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, multiple groups, single VisualState, multiple StateTriggers, with extensible StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlWithMultipleExtensibleStateTriggersTrigger)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, multiple groups, single VisualState, multiple StateTriggers, with extensible StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlWithExtensibleStateTriggerFault)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, single group, single VisualState, single StateTrigger, with extensible StateTrigger, fault after optimized load.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlWithExtensibleStateTriggerFaultNamed)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, single group, single VisualState, single StateTrigger, with extensible StateTrigger, fault after optimized load. Test name lookup.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlWithExtensibleStateTriggerFaultNamedTest)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, single group, single VisualState, single StateTrigger, with extensible StateTrigger, fault after optimized load. Test name lookup.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlWithExtensibleStateTriggerFaultNamed2)
                TEST_METHOD_PROPERTY(L"Description", L"Teamplted control, single group, single VisualState, single StateTrigger, with extensible StateTrigger, fault after optimized load. Test name lookup.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlWithExtensibleStateTriggerFaultGetStateTriggers)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, single group, single VisualState, single StateTrigger, with extensible StateTrigger, fault after optimized load. Test name lookup.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlModifyAdaptiveTriggerInCode)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger. Get an AdaptiveTrigger by name, modify it, and verify triggers automatically transition to the correct state.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlModifyAdaptiveTriggerInCode)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, single group, single VisualState, single StateTrigger. Get an AdaptiveTrigger by name, modify it, and verify triggers automatically transition to the correct state.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlModifyCollectionInUserCode)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger. Modify the trigger collection in use code and verify triggers automatically transition to the correct VisualState.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlModifyCollectionInUserCode)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, single group, single VisualState, single StateTrigger. Modify the trigger collection in user code and verify triggers automatically transition to correct VisualState.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlExtensibleStateTriggerBinding)
                TEST_METHOD_PROPERTY(L"Description", L"UserControl with simple binding.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlExtensibleStateTriggerBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control with simple deferred binding.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlExtensibleStateTriggerBindingModifyInUserCode)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control with deferred binding. Modify extensible trigger in user code and verify triggers evaluate correctly.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlWithStateTriggersTrigger)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, with extensible StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlWithStateTriggerTrigger)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, with extensible StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlWithUnnamedVisualStates)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState with no x:Name, single StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlWithUnnamedVisualStates)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, single group, single unnamed VisualState, single StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlWithExtensibleStateTriggerFaultUnnamedVisualStates)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, single group, multiple VisualStates with no x:Names, multiple StateTriggers, with extensible StateTrigger, fault after optimized load, verify state transition after fault when extensible trigger is modified by user code.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlResize)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, multiple VisualStates, single StateTrigger, resize window and verify VisualState.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlResize)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, single group, multiple VisualStates, single StateTrigger, resize window and verify VisualState.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlSettersExtensibleStateTriggerBinding)
                TEST_METHOD_PROPERTY(L"Description", L"UserControl using Setters with simple binding.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlSettersExtensibleStateTriggerBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control using Setters with simple deferred binding.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GridManualVisualStateTransitions)
                TEST_METHOD_PROPERTY(L"Description", L"Verify Setters can set the same value multiple times, transition with GoToState calls.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlSettersResize)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, multiple VisualStates, single StateTrigger, resize window and verify VisualState. Use Setters rather than Storyboards to set values.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlSettersResize)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, single group, multiple VisualStates, single StateTrigger, resize window and verify VisualState. Use Setters rather than Storyboards to set values.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlWithStateTriggers)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlWithStateTriggersBinding)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlCustomVSM)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, custom VSM.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlCustomVSMUnnamed)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, custom VSM.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlCustomVSMUnnamedResize)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, custom VSM.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(UserControlResourceDictionaryTrigger)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, ResourceDictionary.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlResourceDictionaryTrigger2)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, ResourceDictionary.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlResourceDictionaryTrigger3)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, ResourceDictionary.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlResourceDictionaryTrigger4)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, ResourceDictionary.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlResourceDictionary1)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, ResourceDictionary.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlResourceDictionary2)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger, ResourceDictionary.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlResourceDictionarySharedExtensibleTrigger)
                TEST_METHOD_PROPERTY(L"Description", L"User control, extensible trigger: verify a single trigger instance shared in multiple locations.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlResourceDictionarySharedAdaptiveTrigger)
                TEST_METHOD_PROPERTY(L"Description", L"User control, adaptive trigger: verify a single trigger instance shared in muliple locations.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserControlAdaptiveTriggerBinding)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NestedGrids)
                TEST_METHOD_PROPERTY(L"Description", L"UserControl with Grid + StateTriggers with Storyboards.")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(UserControlModifyProgrammaticallyAddedTriggers)
                TEST_METHOD_PROPERTY(L"Description", L"User control, single group, single VisualState, single StateTrigger. Modify the trigger collection in use code and verify triggers automatically transition to the correct VisualState.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedControlModifyProgrammaticallyAddedTriggers)
                TEST_METHOD_PROPERTY(L"Description", L"Templated control, single group, single VisualState, single StateTrigger. Modify the trigger collection in user code and verify triggers automatically transition to correct VisualState.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VSParserError)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies ContentControl with setters is parsed without errors - Optimized load path.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VSParserErrorLegacy)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies ContentControl with setters is parsed without errors - XamlReader.Load, Legacy load path.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VSParserErrorTemplateValidation)
                TEST_METHOD_PROPERTY(L"Description", L"VerifiesContentControl with setters is parsed without errors - XamlReader.LoadWithInitialTemplateValidation, Optimized.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VSParserErrorTemplateValidationLegacy)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies ContentControl with setters is parsed without errors - XamlReader.LoadWithInitialTemplateValidation, Legacy.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TemplatedPivotHeaderItem)
                TEST_METHOD_PROPERTY(L"Description", L"PivotHeaderItem does not check for NULL after retrieving VisualState.  If PivotHeaderItem is retemplated to use VisualState triggers, the VisualState may be NULL if no triggers evaluate to true when a context change occurs.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VisualStateGroupsNullState)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies NULL State from VisualStateGroup doesn't affect other VisualStateGroup in same object.")
            END_TEST_METHOD()

        private:
            void ExecuteVSParserError(bool LoadWithInitialTemplateValidation = false);
            static Platform::String^ textBlockName;
            Control^ CreateXamlTree(Platform::String^ xamlString);
            void VerifyCurrentVisualState(Control^ rootUserControl, Platform::String^ expectedVisualStateName, Platform::String^ targetName = textBlockName);
            DependencyObject^ VerifyGetNamedObject(Control^ rootUserControl, Platform::String^ objectName);
            IVector<Microsoft::UI::Xaml::StateTriggerBase ^>^ GetStateTriggerCollection(Control^ rootUserControl, unsigned int groupIndex, unsigned int visualStateIndex);
            void CauseVisualStateGroupCollectionToFaultIn(Control^ rootUserControl);
            void ResizeDesktopWindow(unsigned int width, unsigned int height);
            void MaximizeDesktopWindow();
            void ChangeVisualState(Control^ rootUserControl, Platform::String^ newVisualState, bool useTransitions = true);


            bool m_isTemplatedControl;
        };

     }
} } } }


