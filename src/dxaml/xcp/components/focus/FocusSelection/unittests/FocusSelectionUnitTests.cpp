// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "FocusSelection.h"
#include "FocusSelectionUnitTests.h"
#include <CxxMock.h>

#include <CDependencyObject.h>

using namespace CxxMock;
using namespace Focus;

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Focus {

        struct Base {};

        MOCK_CLASS(MockElement, Base)
            MockElement(bool value) : m_value(value) {}

            HRESULT GetValueInherited(const CDependencyProperty*, CValue* val)
            {
                val->SetBool(m_value);
                return S_OK;
            }

            template <KnownTypeIndex targetTypeIndex>
            bool OfTypeByIndex()
            {
                return OfTypeByIndex(targetTypeIndex);
            }

            STUB_METHOD(bool, OfTypeByIndex, 1(KnownTypeIndex))

        private:
            bool m_value;
        END_MOCK

        MOCK_CLASS(DirectionFocusMockElement, CDependencyObject)
            DirectionFocusMockElement(DirectUI::XYFocusKeyboardNavigationMode mode, bool isSetLocally) : m_mode(mode), m_isSetLocally(isSetLocally) {}

            HRESULT GetValueByIndex(KnownPropertyIndex, CValue* val)
            {
                val->SetEnum(static_cast<int>(m_mode));
                val->GetCustomData().SetIsSetLocally(m_isSetLocally);
                return S_OK;
            }

            template <KnownTypeIndex targetTypeIndex>
            bool OfTypeByIndex()
            {
                return OfTypeByIndex(targetTypeIndex);
            }

            STUB_METHOD(bool, OfTypeByIndex, 1(KnownTypeIndex))

        private:
            DirectUI::XYFocusKeyboardNavigationMode m_mode;
            bool m_isSetLocally;
        END_MOCK

        struct Options
        {
            DirectionFocusMockElement* searchRoot = nullptr;
            bool shouldConsiderXYFocusKeyboardNavigation = true;
        };

        MOCK_CLASS(MockFocusManager, Base)
            STUB_METHOD(DirectionFocusMockElement*, FindNextFocus, 4(DirectUI::FocusNavigationDirection, Options, CDependencyObject*, bool))
            STUB_METHOD(FocusMovementResult, SetFocusedElement, 1(const FocusMovement&))
            STUB_METHOD(void, RaiseNoFocusCandidateFoundEvent, 2(DirectUI::FocusState, DirectUI::FocusNavigationDirection))
        END_MOCK

        void FocusSelectionUnitTests::ValidateShouldUpdateFocusWhenAllowFocusOnInteractionDisabled()
        {
            MockElement elementA(false);
            MockElement elementB(true);

            Expect(elementA, OfTypeByIndex)
                .ReturnValue(false);
            Expect(elementB, OfTypeByIndex)
                .ReturnValue(false);

            bool update = FocusSelection::ShouldUpdateFocus(&elementA, DirectUI::FocusState::Pointer);
            VERIFY_IS_FALSE(update);

            //Even if the element is supposed to not allow focus, if the focus state is not a pointer, we should
            //still update focus
            update = FocusSelection::ShouldUpdateFocus(&elementA, DirectUI::FocusState::Keyboard);
            VERIFY_IS_TRUE(update);

            //Regardless of FocusState, if we should allow focus, then we should update focus
            update = FocusSelection::ShouldUpdateFocus(&elementB, DirectUI::FocusState::Pointer);
            VERIFY_IS_TRUE(update);
        }

        void FocusSelectionUnitTests::ValidateNavigationDirection()
        {
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirection(wsy::VirtualKey_GamepadDPadUp), DirectUI::FocusNavigationDirection::Up);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirection(wsy::VirtualKey_GamepadLeftThumbstickUp), DirectUI::FocusNavigationDirection::Up);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirection(wsy::VirtualKey_Up), DirectUI::FocusNavigationDirection::Up);

            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirection(wsy::VirtualKey_GamepadDPadDown), DirectUI::FocusNavigationDirection::Down);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirection(wsy::VirtualKey_GamepadLeftThumbstickDown), DirectUI::FocusNavigationDirection::Down);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirection(wsy::VirtualKey_Down), DirectUI::FocusNavigationDirection::Down);

            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirection(wsy::VirtualKey_GamepadDPadLeft), DirectUI::FocusNavigationDirection::Left);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirection(wsy::VirtualKey_GamepadLeftThumbstickLeft), DirectUI::FocusNavigationDirection::Left);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirection(wsy::VirtualKey_Left), DirectUI::FocusNavigationDirection::Left);

            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirection(wsy::VirtualKey_GamepadDPadRight), DirectUI::FocusNavigationDirection::Right);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirection(wsy::VirtualKey_GamepadLeftThumbstickRight), DirectUI::FocusNavigationDirection::Right);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirection(wsy::VirtualKey_Right), DirectUI::FocusNavigationDirection::Right);
        }

        void FocusSelectionUnitTests::ValidateNavigationDirectionForKeyboardArrow()
        {
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirectionForKeyboardArrow(wsy::VirtualKey_GamepadDPadUp), DirectUI::FocusNavigationDirection::None);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirectionForKeyboardArrow(wsy::VirtualKey_GamepadLeftThumbstickUp), DirectUI::FocusNavigationDirection::None);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirectionForKeyboardArrow(wsy::VirtualKey_Up), DirectUI::FocusNavigationDirection::Up);

            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirectionForKeyboardArrow(wsy::VirtualKey_GamepadDPadDown), DirectUI::FocusNavigationDirection::None);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirectionForKeyboardArrow(wsy::VirtualKey_GamepadLeftThumbstickDown), DirectUI::FocusNavigationDirection::None);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirectionForKeyboardArrow(wsy::VirtualKey_Down), DirectUI::FocusNavigationDirection::Down);

            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirectionForKeyboardArrow(wsy::VirtualKey_GamepadDPadLeft), DirectUI::FocusNavigationDirection::None);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirectionForKeyboardArrow(wsy::VirtualKey_GamepadLeftThumbstickLeft), DirectUI::FocusNavigationDirection::None);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirectionForKeyboardArrow(wsy::VirtualKey_Left), DirectUI::FocusNavigationDirection::Left);

            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirectionForKeyboardArrow(wsy::VirtualKey_GamepadDPadRight), DirectUI::FocusNavigationDirection::None);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirectionForKeyboardArrow(wsy::VirtualKey_GamepadLeftThumbstickRight), DirectUI::FocusNavigationDirection::None);
            VERIFY_ARE_EQUAL(FocusSelection::GetNavigationDirectionForKeyboardArrow(wsy::VirtualKey_Right), DirectUI::FocusNavigationDirection::Right);
        }

        void FocusSelectionUnitTests::ValidateTryDirectionalFocus()
        {
            DirectionFocusMockElement element(DirectUI::XYFocusKeyboardNavigationMode::Enabled, true);
            DirectionFocusMockElement candidate(DirectUI::XYFocusKeyboardNavigationMode::Disabled, false);
            MockFocusManager focusManager;

            Expect(element, OfTypeByIndex)
                .ReturnValue(true);

            const FocusMovementResult result(true, false, S_OK);

            Expect(focusManager, FindNextFocus)
                .ReturnValue(&candidate);
            Expect(focusManager, SetFocusedElement)
                .ReturnValue(result);

            FocusSelection::DirectionalFocusInfo info = FocusSelection::TryDirectionalFocus<Options>(&focusManager, DirectUI::FocusNavigationDirection::Right, &element);
            VERIFY_IS_TRUE(info.handled);
        }

        void FocusSelectionUnitTests::ValidateTryDirectionalFocusMarksUnhandled()
        {
            DirectionFocusMockElement element(DirectUI::XYFocusKeyboardNavigationMode::Enabled, true);
            MockFocusManager focusManager;

            Expect(element, OfTypeByIndex)
                .ReturnValue(true);
            Expect(focusManager, FindNextFocus)
                .ReturnValue(nullptr);

            FocusSelection::DirectionalFocusInfo info = FocusSelection::TryDirectionalFocus<Options>(&focusManager, DirectUI::FocusNavigationDirection::Right, &element);
            VERIFY_IS_FALSE(info.handled);
        }

        void FocusSelectionUnitTests::ValidateThatNotHandledWhenModeInherited()
        {
            DirectionFocusMockElement element(DirectUI::XYFocusKeyboardNavigationMode::Auto, false);
            MockFocusManager focusManager;

            Expect(element, OfTypeByIndex)
                .ReturnValue(true);

            FocusSelection::DirectionalFocusInfo info = FocusSelection::TryDirectionalFocus<Options>(&focusManager, DirectUI::FocusNavigationDirection::Right, &element);
            VERIFY_IS_FALSE(info.handled);
        }

        void FocusSelectionUnitTests::ValidateShouldNotBubbleWhenModeNone()
        {
            DirectionFocusMockElement element(DirectUI::XYFocusKeyboardNavigationMode::Disabled, false);
            MockFocusManager focusManager;

            Expect(element, OfTypeByIndex)
                .ReturnValue(true);

            FocusSelection::DirectionalFocusInfo info = FocusSelection::TryDirectionalFocus<Options>(&focusManager, DirectUI::FocusNavigationDirection::Right, &element);
            VERIFY_IS_FALSE(info.handled);
            VERIFY_IS_FALSE(info.shouldBubble);
        }

        void FocusSelectionUnitTests::ValidateNotHandledWhenNotUIElement()
        {
            DirectionFocusMockElement element(DirectUI::XYFocusKeyboardNavigationMode::Enabled, true);
            MockFocusManager focusManager;

            Expect(element, OfTypeByIndex)
                .ReturnValue(false);

            FocusSelection::DirectionalFocusInfo info = FocusSelection::TryDirectionalFocus<Options>(&focusManager, DirectUI::FocusNavigationDirection::Right, &element);
            VERIFY_IS_FALSE(info.handled);
        }
    }
}}}}
