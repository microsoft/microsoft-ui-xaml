// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "StateTrigger.CustomStateTrigger.h"

using namespace Microsoft::UI::Xaml;
using namespace Platform;

namespace Private { namespace Tests {
    namespace Adaptability {
    namespace StateTriggerTests {

        using namespace Microsoft::UI::Xaml;

        DependencyProperty^ CustomStateTrigger::_TriggerValueDP = nullptr;

        CustomStateTrigger::CustomStateTrigger()
        {
            SetActive(false);
        }

        void CustomStateTrigger::RegisterDependencyProperties()
        {
            if (_TriggerValueDP == nullptr) 
            { 
                _TriggerValueDP = DependencyProperty::Register(
                  "TriggerValue", 
                  Platform::Boolean::typeid, 
                  CustomStateTrigger::typeid, 
                  ref new PropertyMetadata(nullptr,
                      ref new PropertyChangedCallback(&CustomStateTrigger::OnTriggerChanged))
                  ); 
            } 
        }

        void CustomStateTrigger::ClearDependencyProperties()
        {
            _TriggerValueDP = nullptr;
        }

 
        void CustomStateTrigger::OnTriggerChanged(DependencyObject^ d, DependencyPropertyChangedEventArgs^ e)
        {
            CustomStateTrigger^ stateTrigger = (CustomStateTrigger^)d;
            Platform::Boolean s = (Platform::Boolean)(e->NewValue);
            stateTrigger->TriggerValue = s;
        }

        MyCustomVSM::MyCustomVSM()
        {
        }

        MyCustomVSMWithGoToStateCoreOverride::MyCustomVSMWithGoToStateCoreOverride() :
            wasGoToStateCoreCalled(false),
            shouldCallBaseGoToStateCore(true)
        {
        }

        bool MyCustomVSMWithGoToStateCoreOverride::GoToStateCore(
                Control^ control, 
                FrameworkElement^ stateGroupsRoot, 
                String^ stateName, 
                VisualStateGroup^ group, 
                VisualState^ state, 
                bool useTransitions)
        {
            wasGoToStateCoreCalled = true;

            if(shouldCallBaseGoToStateCore)
            {
                VisualStateManager::GoToStateCore(
                    control, 
                    stateGroupsRoot, 
                    stateName, 
                    group, 
                    state, 
                    useTransitions);
            }

            return true;
        }

        bool MyCustomVSMWithGoToStateCoreOverride::VerifyGoToStateCoreWasCalled()
        {
            return wasGoToStateCoreCalled;
        }

        void MyCustomVSMWithGoToStateCoreOverride::ShouldCallBaseGoToStateCore(bool shouldCallBase)
        {
            shouldCallBaseGoToStateCore = shouldCallBase;
        }

        void MyCustomVSMWithGoToStateCoreOverride::Reset()
        {
             wasGoToStateCoreCalled = false;
        }
    }
} } }
