// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Tests {
    namespace Adaptability {
    namespace StateTriggerTests {

        using namespace Microsoft::UI::Xaml;
        using namespace Platform;
        using namespace Microsoft::UI::Xaml::Controls;

        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class CustomStateTrigger sealed : public Microsoft::UI::Xaml::StateTriggerBase 
        {
            public:
            CustomStateTrigger();
            
            property bool TriggerValue
            {
                bool get() { return (bool)GetValue(TriggerValueDP); };
                void set(bool value) 
                { 
                    SetValue(TriggerValueDP, value); 
                    SetActive(value);
                };
            };

            static property DependencyProperty^ TriggerValueDP 
            {
                DependencyProperty^ get() {return _TriggerValueDP;}
                void set(DependencyProperty^ value) 
                { 
                    _TriggerValueDP = value; 
                }
            };

            static void RegisterDependencyProperties();

            static void ClearDependencyProperties();

            static void OnTriggerChanged(DependencyObject^ d, DependencyPropertyChangedEventArgs^ e);

            private:
                static DependencyProperty^ _TriggerValueDP;
        };

        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class MyCustomVSM sealed : public Microsoft::UI::Xaml::VisualStateManager
        {
            public:
                MyCustomVSM();

        };

        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class MyCustomVSMWithGoToStateCoreOverride sealed : public Microsoft::UI::Xaml::VisualStateManager
        {
            public:
                MyCustomVSMWithGoToStateCoreOverride();

                // Test verification methods
                bool VerifyGoToStateCoreWasCalled();

                // Resets all the test object's values to nullptr/false
                void Reset();

                void ShouldCallBaseGoToStateCore(bool shoudCallBase);

          protected:
            virtual bool GoToStateCore(
                  Control^ control, 
                  FrameworkElement^ templateRoot, 
                  String^ stateName, 
                  VisualStateGroup^ group, 
                  VisualState^ state, 
                  bool useTransitions
                ) override;

            private:       
                bool wasGoToStateCoreCalled;
                bool shouldCallBaseGoToStateCore;
        };
    }
} } }
