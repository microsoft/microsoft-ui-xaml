// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ToggleSwitchKeyProcess.h"
#include "ToggleSwitchUnitTests.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Controls {

        using namespace KeyPress;

        class ToggleSwitchTestObject
        {
        public:
            ToggleSwitchTestObject() :
                m_handledKeyDown(false),
                m_isDragging(false),
                m_isOn(false),
                m_flowDirection(xaml::FlowDirection_LeftToRight){}

            void setHandlesKeyDown(bool value){ m_handledKeyDown = value; }
            void setIsOn(bool value){ m_isOn = value; }
            BOOLEAN getIsOn(){ return m_isOn; }
            
            HRESULT get_FlowDirection(xaml::FlowDirection* pFlowDirection) { *pFlowDirection = m_flowDirection; return S_OK; }
            void put_FlowDirection(xaml::FlowDirection flowDirection) { m_flowDirection = flowDirection; }

            bool HandlesKey(wsy::VirtualKey key)
            {
                return wsy::VirtualKey_Space == key ||
                    key == wsy::VirtualKey_GamepadA;
            }

            HRESULT Toggle()
            {
                m_isOn = !m_isOn;
                return S_OK;
            }

            HRESULT get_IsOn(BOOLEAN* pbValue)
            {
                *pbValue = m_isOn;
                return S_OK;
            }
            
            bool m_handledKeyDown;
            bool m_isDragging;

        private:
            BOOLEAN m_isOn;
            xaml::FlowDirection m_flowDirection;
        };

        void ToggleSwitchUnitTests::ValidateKeyDown()
        {
            ToggleSwitchTestObject* tsObject = new ToggleSwitchTestObject();

            BOOLEAN handled;
            HRESULT succeeded = ToggleSwitch::KeyDown<ToggleSwitchTestObject>(wsy::VirtualKey_Space, tsObject, &handled);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);

            handled = FALSE;
            succeeded = ToggleSwitch::KeyDown<ToggleSwitchTestObject>(wsy::VirtualKey_Z, tsObject, &handled);
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_FALSE(handled);
        }

        void ToggleSwitchUnitTests::ValidateKeyUp()
        {
            ToggleSwitchTestObject* tsObject = new ToggleSwitchTestObject();

            BOOLEAN handled = FALSE;
            tsObject->setHandlesKeyDown(true);

            HRESULT succeeded = ToggleSwitch::KeyUp<ToggleSwitchTestObject>(wsy::VirtualKey_Space, tsObject, &handled);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_IS_TRUE(tsObject->getIsOn());
        }

        void ToggleSwitchUnitTests::ValidateAlwaysTogglesWithSpace()
        {
            ToggleSwitchTestObject* tsObject = new ToggleSwitchTestObject();

            BOOLEAN handled = FALSE;
            tsObject->setHandlesKeyDown(true);

            HRESULT succeeded = ToggleSwitch::KeyUp<ToggleSwitchTestObject>(wsy::VirtualKey_Space, tsObject, &handled);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_IS_TRUE(tsObject->getIsOn());

            tsObject->setHandlesKeyDown(true);
            handled = FALSE;
            succeeded = ToggleSwitch::KeyUp<ToggleSwitchTestObject>(wsy::VirtualKey_Space, tsObject, &handled);
            VERIFY_IS_FALSE(tsObject->getIsOn());

            tsObject->setHandlesKeyDown(true);
            handled = FALSE;
            succeeded = ToggleSwitch::KeyUp<ToggleSwitchTestObject>(wsy::VirtualKey_Space, tsObject, &handled);
            VERIFY_IS_TRUE(tsObject->getIsOn());
        }
    }
}}}}