// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SliderKeyProcess.h"
#include "SliderUnitTests.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Controls {

        using namespace KeyPress;

        class SliderTestObject
        {
        public:
            SliderTestObject() :
                m_flowDirection(xaml::FlowDirection_LeftToRight),
                m_isDirectionReversed(false),
                m_Orientation(xaml_controls::Orientation_Horizontal),
                m_min(0),
                m_max(100),
                m_value(0){}

            void setFlowDirection(xaml::FlowDirection direction){ m_flowDirection = direction; }
            void setOrientation(xaml_controls::Orientation orientation){ m_Orientation = orientation; }
            void setIsReversed(BOOLEAN value){ m_isDirectionReversed = value; }
            void setMax(double value){ m_max = value; }
            void setMin(double value){ m_min = value; }
            double getValue(){ return m_value; }
            double getMax(){ return m_max; }
            double getMin(){ return m_min; }

            HRESULT get_FlowDirection(xaml::FlowDirection* pDirection)
            {
                *pDirection = m_flowDirection;
                return S_OK;
            }
            
            HRESULT get_Orientation(xaml_controls::Orientation* pOrientation)
            {
                *pOrientation = m_Orientation;
                return S_OK;
            }

            HRESULT get_IsDirectionReversed(BOOLEAN* pbValue)
            {
                *pbValue = m_isDirectionReversed;
                return S_OK;
            }

            HRESULT get_Minimum(double* pfValue)
            {
                *pfValue = m_min;
                return S_OK;
            }

            HRESULT get_Maximum(double* pfValue)
            {
                *pfValue = m_max;
                return S_OK;
            }

            HRESULT put_Value(double pfValue)
            {
                m_value = pfValue;
                return S_OK;
            }

            HRESULT Step(bool useSmallChange, BOOLEAN forward)
            {
                forward ? m_value++ : m_value--;
                return S_OK;
            }

        private:
            xaml::FlowDirection m_flowDirection;
            xaml_controls::Orientation m_Orientation;
            BOOLEAN m_isDirectionReversed;
            double m_min;
            double m_max;
            double m_value;
        };

        void SliderUnitTests::ValidateKeyDownWithNavigationOrGamepad()
        {
            SliderTestObject* sObject = new SliderTestObject();

            BOOLEAN handled;
            HRESULT succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_GamepadDPadRight, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), 1);

            handled = FALSE;
            succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_GamepadDPadLeft, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), 0);

            sObject->put_Value(0);

            handled = FALSE;
            succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_GamepadLeftThumbstickRight, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), 1);

            handled = FALSE;
            succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_GamepadLeftThumbstickLeft, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), 0);

            handled = FALSE;
            succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_GamepadRightShoulder, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), sObject->getMax());

            handled = FALSE;
            succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_GamepadLeftShoulder, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), sObject->getMin());
        }

        void SliderUnitTests::ValidateKeyDown()
        {
            SliderTestObject* sObject = new SliderTestObject();

            BOOLEAN handled;

            HRESULT succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_Up, sObject, &handled);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), 1);

            handled = FALSE;
            succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_Left, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), 0);

            handled = FALSE;
            succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_End, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), sObject->getMax());

            handled = FALSE;
            succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_Home, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), sObject->getMin());
        }

        void SliderUnitTests::ValidateKeyDownReversedWithNavigationOrGamepad()
        {
            SliderTestObject* sObject = new SliderTestObject();

            sObject->setIsReversed(true);
            BOOLEAN handled;
            HRESULT succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_GamepadDPadLeft, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), 1);

            handled = FALSE;
            succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_GamepadDPadRight, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), 0);

            handled = FALSE;
            succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_GamepadLeftThumbstickLeft, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), 1);

            handled = FALSE;
            succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_GamepadLeftThumbstickRight, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), 0);
        }

        void SliderUnitTests::ValidateKeyDownReversed()
        {
            SliderTestObject* sObject = new SliderTestObject();

            sObject->setIsReversed(true);
            BOOLEAN handled;
            HRESULT succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_Left, sObject, &handled);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), 1);

            handled = FALSE;
            succeeded = Slider::KeyDown<SliderTestObject>(wsy::VirtualKey_Up, sObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_ARE_EQUAL(sObject->getValue(), 0);
        }
    }
}}}}