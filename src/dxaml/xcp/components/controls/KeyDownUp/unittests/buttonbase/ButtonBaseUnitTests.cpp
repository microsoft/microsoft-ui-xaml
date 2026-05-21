// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ButtonBaseKeyProcess.h"
#include "ButtonBaseUnitTests.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Controls {

        using namespace KeyPress;

        class ButtonBaseTestObject
        {
        public:
            ButtonBaseTestObject() :
                m_bIsEnabled(false),
                m_bClickMode(xaml_controls::ClickMode_Release),
                m_bIsPressed(false),
                m_bIsSpaceOrEnterKeyDown(false),
                m_bIsNavigationAcceptOrGamepadAKeyDown(false),
                m_bIsPointerCaptured(false),
                m_bIsPointerLeftButtonDown(false),
                m_iCounter(0),
                hr(S_OK) {}

            void SetIsEnabled(bool value) { m_bIsEnabled = value; }
            void SetClickMode(xaml_controls::ClickMode value) { m_bClickMode = value; }
            void SetIsPressed(bool value) { m_bIsPressed = value; }
            BOOLEAN GetIsPressed() { return m_bIsPressed; }
            int getCounter(){ return m_iCounter; }

            HRESULT get_IsEnabled(BOOLEAN* pbIsEnabled)
            {
                *pbIsEnabled = m_bIsEnabled;
                return hr;
            }

            HRESULT get_ClickMode(xaml_controls::ClickMode* pbClickMode)
            {
                *pbClickMode = m_bClickMode;
                return hr;
            }

            HRESULT get_IsPressed(BOOLEAN *pbIsPressed)
            {
                *pbIsPressed = m_bIsPressed;
                return hr;
            }

            HRESULT put_IsPressed(BOOLEAN bIsPressed)
            {
                m_bIsPressed = bIsPressed;
                return hr;
            }

            HRESULT OnClick()
            {
                m_iCounter++;
                return hr;
            }

            bool m_bIsSpaceOrEnterKeyDown;
            bool m_bIsNavigationAcceptOrGamepadAKeyDown;
            bool m_bIsPointerCaptured;
            bool m_bIsPointerLeftButtonDown;

        private:
            BOOLEAN m_bIsEnabled;
            xaml_controls::ClickMode m_bClickMode;
            BOOLEAN m_bIsPressed;
            HRESULT hr;
            int m_iCounter;
        };

        void ButtonBaseUnitTests::ValidateKeyDownWithNavigationOrGamepad()
        {
            ButtonBaseTestObject* bbObject = new ButtonBaseTestObject();

            BOOLEAN handled = FALSE;
            bbObject->SetIsEnabled(true);

            HRESULT succeeded = ButtonBase::KeyDown<ButtonBaseTestObject>(wsy::VirtualKey_GamepadA, &handled, false, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_IS_TRUE(bbObject->GetIsPressed());
            VERIFY_IS_TRUE(bbObject->m_bIsNavigationAcceptOrGamepadAKeyDown);
        }

        void ButtonBaseUnitTests::ValidateKeyDown()
        {
            ButtonBaseTestObject* bbObject = new ButtonBaseTestObject();

            BOOLEAN handled;
            bbObject->SetIsEnabled(true);

            HRESULT succeeded = ButtonBase::KeyDown<ButtonBaseTestObject>(wsy::VirtualKey_Space, &handled, false, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_IS_TRUE(bbObject->GetIsPressed());
            VERIFY_IS_TRUE(bbObject->m_bIsSpaceOrEnterKeyDown);

            bbObject->m_bIsSpaceOrEnterKeyDown = false;
            bbObject->SetIsPressed(false);
            handled = FALSE;
            succeeded = ButtonBase::KeyDown<ButtonBaseTestObject>(wsy::VirtualKey_Enter, &handled, true, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_IS_TRUE(bbObject->GetIsPressed());
            VERIFY_IS_TRUE(bbObject->m_bIsSpaceOrEnterKeyDown);

            bbObject->m_bIsSpaceOrEnterKeyDown = false;
            bbObject->SetIsPressed(false);
            handled = FALSE;
            succeeded = ButtonBase::KeyDown<ButtonBaseTestObject>(wsy::VirtualKey_Enter, &handled, false, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_FALSE(handled);
            VERIFY_IS_FALSE(bbObject->GetIsPressed());
            VERIFY_IS_FALSE(bbObject->m_bIsSpaceOrEnterKeyDown);
        }

        void ButtonBaseUnitTests::ValidateOnClickCalledKeyDown()
        {
            ButtonBaseTestObject* bbObject = new ButtonBaseTestObject();

            const int numberOfClicks = 10;

            bbObject->SetIsEnabled(true);
            bbObject->SetClickMode(xaml_controls::ClickMode_Press);

            HRESULT succeeded;
            BOOLEAN handled = FALSE;

            for (int count = 0; count < numberOfClicks; count++)
            {
                succeeded = ButtonBase::KeyDown<ButtonBaseTestObject>(wsy::VirtualKey_Space, &handled, false, bbObject);
                VERIFY_ARE_EQUAL(succeeded, S_OK);
                VERIFY_IS_TRUE(handled);

                bbObject->m_bIsSpaceOrEnterKeyDown = false;
                handled = FALSE;
            }

            VERIFY_ARE_EQUAL(bbObject->getCounter(), numberOfClicks);
        }

        void ButtonBaseUnitTests::ValidateOnClickCalledKeyDownNavigationOrGamepad()
        {
            ButtonBaseTestObject* bbObject = new ButtonBaseTestObject();

            const int numberOfClicks = 10;

            bbObject->SetIsEnabled(true);
            bbObject->SetClickMode(xaml_controls::ClickMode_Press);

            HRESULT succeeded;
            BOOLEAN handled = FALSE;

            for (int count = 0; count < numberOfClicks; count++)
            {
                succeeded = ButtonBase::KeyDown<ButtonBaseTestObject>(wsy::VirtualKey_GamepadA, &handled, false, bbObject);
                VERIFY_ARE_EQUAL(succeeded, S_OK);
                VERIFY_IS_TRUE(handled);

                bbObject->m_bIsNavigationAcceptOrGamepadAKeyDown = false;
                handled = FALSE;
            }
        }

        void ButtonBaseUnitTests::ValidateKeyUpWithNavigationOrGamepad()
        {
            ButtonBaseTestObject* bbObject = new ButtonBaseTestObject();

            BOOLEAN handled;
            bbObject->SetIsEnabled(true);
            bbObject->SetIsPressed(true);
            bbObject->m_bIsNavigationAcceptOrGamepadAKeyDown = false;

            HRESULT succeeded = ButtonBase::KeyUp<ButtonBaseTestObject>(wsy::VirtualKey_GamepadA, &handled, false, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_IS_FALSE(bbObject->GetIsPressed());
            VERIFY_IS_FALSE(bbObject->m_bIsNavigationAcceptOrGamepadAKeyDown);

            bbObject->SetIsEnabled(true);
            bbObject->SetIsPressed(true);
            bbObject->m_bIsNavigationAcceptOrGamepadAKeyDown = false;
        }

        void ButtonBaseUnitTests::ValidateKeyUp()
        {
            ButtonBaseTestObject* bbObject = new ButtonBaseTestObject();

            BOOLEAN handled;
            bbObject->SetIsEnabled(true);
            bbObject->SetIsPressed(true);
            bbObject->m_bIsSpaceOrEnterKeyDown = true;

            HRESULT succeeded = ButtonBase::KeyUp<ButtonBaseTestObject>(wsy::VirtualKey_Space, &handled, false, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_IS_FALSE(bbObject->GetIsPressed());
            VERIFY_IS_FALSE(bbObject->m_bIsSpaceOrEnterKeyDown);

            bbObject->m_bIsSpaceOrEnterKeyDown = true;
            bbObject->SetIsPressed(true);
            handled = FALSE;
            succeeded = ButtonBase::KeyUp<ButtonBaseTestObject>(wsy::VirtualKey_Enter, &handled, true, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_TRUE(handled);
            VERIFY_IS_FALSE(bbObject->GetIsPressed());
            VERIFY_IS_FALSE(bbObject->m_bIsSpaceOrEnterKeyDown);

            bbObject->m_bIsSpaceOrEnterKeyDown = true;
            bbObject->SetIsPressed(true);
            handled = FALSE;
            succeeded = ButtonBase::KeyUp<ButtonBaseTestObject>(wsy::VirtualKey_Enter, &handled, false, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_FALSE(handled);
            VERIFY_IS_TRUE(bbObject->GetIsPressed());
            VERIFY_IS_TRUE(bbObject->m_bIsSpaceOrEnterKeyDown);
        }

        void ButtonBaseUnitTests::ValidateOnClickCalledKeyUp()
        {
            ButtonBaseTestObject* bbObject = new ButtonBaseTestObject();

            const int numberOfClicks = 10;

            bbObject->SetIsEnabled(true);
            bbObject->SetIsPressed(true);
            bbObject->SetClickMode(xaml_controls::ClickMode_Release);

            HRESULT succeeded;
            BOOLEAN handled = FALSE;

            for (int count = 0; count < numberOfClicks; count++)
            {
                succeeded = ButtonBase::KeyUp<ButtonBaseTestObject>(wsy::VirtualKey_Space, &handled, false, bbObject);
                VERIFY_ARE_EQUAL(succeeded, S_OK);
                VERIFY_IS_TRUE(handled);

                handled = FALSE;
                bbObject->SetIsPressed(true);
            }

            VERIFY_ARE_EQUAL(bbObject->getCounter(), numberOfClicks);
        }

        void ButtonBaseUnitTests::ValidateOnClickCalledKeyUpNavigationOrGamepad()
        {
            ButtonBaseTestObject* bbObject = new ButtonBaseTestObject();

            const int numberOfClicks = 10;

            bbObject->SetIsEnabled(true);
            bbObject->SetIsPressed(true);
            bbObject->SetClickMode(xaml_controls::ClickMode_Release);

            HRESULT succeeded;
            BOOLEAN handled = FALSE;

            for (int count = 0; count < numberOfClicks; count++)
            {
                succeeded = ButtonBase::KeyUp<ButtonBaseTestObject>(wsy::VirtualKey_GamepadA, &handled, false, bbObject);
                VERIFY_ARE_EQUAL(succeeded, S_OK);
                VERIFY_IS_TRUE(handled);

                handled = FALSE;
                bbObject->SetIsPressed(true);
            }
        }

        void ButtonBaseUnitTests::ValidatePressUnsupportedKeyDownUp()
        {
            ButtonBaseTestObject* bbObject = new ButtonBaseTestObject();
            bbObject->SetIsEnabled(true);

            BOOLEAN handled = FALSE;
            HRESULT succeeded = ButtonBase::KeyDown<ButtonBaseTestObject>(wsy::VirtualKey_Z, &handled, false, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_FALSE(handled);
            VERIFY_IS_FALSE(bbObject->GetIsPressed());
            VERIFY_IS_FALSE(bbObject->m_bIsSpaceOrEnterKeyDown);

            succeeded = ButtonBase::KeyUp<ButtonBaseTestObject>(wsy::VirtualKey_Z, &handled, false, bbObject);
            
            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_FALSE(handled);
            VERIFY_IS_FALSE(bbObject->GetIsPressed());
            VERIFY_IS_FALSE(bbObject->m_bIsSpaceOrEnterKeyDown);
        }

        void ButtonBaseUnitTests::ValidatePressUnsupportedKeyDownUpNavigationOrGamepad()
        {
            ButtonBaseTestObject* bbObject = new ButtonBaseTestObject();

            BOOLEAN handled = FALSE;
            HRESULT succeeded = ButtonBase::KeyDown<ButtonBaseTestObject>(wsy::VirtualKey_GamepadB, &handled, false, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_FALSE(handled);
            VERIFY_IS_FALSE(bbObject->GetIsPressed());
            VERIFY_IS_FALSE(bbObject->m_bIsNavigationAcceptOrGamepadAKeyDown);

            succeeded = ButtonBase::KeyUp<ButtonBaseTestObject>(wsy::VirtualKey_GamepadB, &handled, false, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_FALSE(handled);
            VERIFY_IS_FALSE(bbObject->GetIsPressed());
            VERIFY_IS_FALSE(bbObject->m_bIsNavigationAcceptOrGamepadAKeyDown);

            succeeded = ButtonBase::KeyDown<ButtonBaseTestObject>(wsy::VirtualKey_B, &handled, false, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_FALSE(handled);
            VERIFY_IS_FALSE(bbObject->GetIsPressed());
            VERIFY_IS_FALSE(bbObject->m_bIsNavigationAcceptOrGamepadAKeyDown);

            succeeded = ButtonBase::KeyUp<ButtonBaseTestObject>(wsy::VirtualKey_B, &handled, false, bbObject);

            VERIFY_ARE_EQUAL(succeeded, S_OK);
            VERIFY_IS_FALSE(handled);
            VERIFY_IS_FALSE(bbObject->GetIsPressed());
            VERIFY_IS_FALSE(bbObject->m_bIsNavigationAcceptOrGamepadAKeyDown);
        }
    }
}}}}