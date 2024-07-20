// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ToolTipService.g.h"
#include "ToolTip.g.h"
#include "KeyboardAccelerator.g.h"
#include "KeyboardAcceleratorInvokedEventArgs.g.h"
#include "CKeyboardAccelerator.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// These VK codes were added in a later SDK than the one we are using, so we define them here.
#define VK_IME_ON         0x16
#define VK_IME_OFF        0x1A

XUINT32 GetResourceStringIdFromVirtualKey(_In_ wsy::VirtualKey key);

/* static */ _Check_return_ HRESULT KeyboardAccelerator::RaiseKeyboardAcceleratorInvoked(
    _In_ CKeyboardAccelerator* pNativeAccelerator,
    _In_ CDependencyObject* pElement,
    _Out_ BOOLEAN *pIsHandled)
{
    ctl::ComPtr<DependencyObject> spPeerDO;
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pNativeAccelerator, &spPeerDO));

    if (!spPeerDO)
    {
        // There is no need to fire the event if there is no peer, since no one is listening
        return S_FALSE;
    }

    ctl::ComPtr<DependencyObject> spElementPeerDO;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pElement, &spElementPeerDO));

    ctl::ComPtr<IKeyboardAccelerator> spIKA;
    IFC_RETURN(spPeerDO.As(&spIKA));

    ctl::ComPtr<KeyboardAccelerator> spAccelerator = spIKA.AsOrNull<KeyboardAccelerator>();

    KeyboardAccelerator::InvokedEventSourceType* pEventSource = nullptr;
    IFC_RETURN(spAccelerator->GetInvokedEventSourceNoRef(&pEventSource));

    ctl::ComPtr<KeyboardAcceleratorInvokedEventArgs> spArgs;
    IFC_RETURN(ctl::make(&spArgs));
    IFCFAILFAST(spArgs->put_Element(spElementPeerDO.Cast<IDependencyObject>()));
    IFCFAILFAST(spArgs->put_KeyboardAccelerator(spIKA.Get()));

    IFC_RETURN(pEventSource->Raise(spIKA.Get(), spArgs.Get()));
    IFCFAILFAST(spArgs->get_Handled(pIsHandled));

    // If not handled, raise an event on parent element to give it a chance to handle the event.
    // This will enable controls which don't have automated invoked action, to handle the event. e.g Pivot
    if (*pIsHandled == FALSE)
    {
        IFC_RETURN(UIElement::RaiseKeyboardAcceleratorInvokedStatic(pElement, spArgs.Get(), pIsHandled));
    }

    return S_OK;
}

/* static */ _Check_return_ HRESULT KeyboardAccelerator::GetStringRepresentationForUIElement(_In_ DirectUI::UIElement* uiElement, _Outptr_result_maybenull_ HSTRING *stringRepresentation)
{
    *stringRepresentation = nullptr;

    // We don't want to bother doing anything if we've never actually set a keyboard accelerator,
    // so we'll just return null unless we have.
    if (!uiElement->GetHandle()->CheckOnDemandProperty(KnownPropertyIndex::UIElement_KeyboardAccelerators).IsNull())
    {
        ctl::ComPtr<wfc::IVector<xaml_input::KeyboardAccelerator*>> keyboardAccelerators;
        UINT keyboardAcceleratorCount;
        ctl::ComPtr<xaml_input::IKeyboardAccelerator> firstKeyboardAccelerator;

        IFC_RETURN(uiElement->get_KeyboardAccelerators(&keyboardAccelerators));
        IFC_RETURN(keyboardAccelerators->get_Size(&keyboardAcceleratorCount));

        if (keyboardAcceleratorCount > 0)
        {
            wrl_wrappers::HString keyboardAcceleratorStringRepresentation;
            IFC_RETURN(keyboardAccelerators->GetAt(0, &firstKeyboardAccelerator));
            IFC_RETURN(firstKeyboardAccelerator.Cast<KeyboardAccelerator>()->GetStringRepresentation(keyboardAcceleratorStringRepresentation.ReleaseAndGetAddressOf()));
            *stringRepresentation = keyboardAcceleratorStringRepresentation.Detach();
        }
    }

    return S_OK;
}

_Check_return_ HRESULT KeyboardAccelerator::GetStringRepresentation(_Out_ HSTRING *stringRepresentation)
{
    wsy::VirtualKey key;
    wsy::VirtualKeyModifiers modifiers;
    wrl_wrappers::HString stringRepresentationLocal;

    *stringRepresentation = nullptr;
    IFC_RETURN(get_Key(&key));
    IFC_RETURN(get_Modifiers(&modifiers));

    if ((modifiers & wsy::VirtualKeyModifiers_Control) != 0)
    {
        IFC_RETURN(ConcatVirtualKey(wsy::VirtualKey_Control, stringRepresentationLocal));
    }

    if ((modifiers & wsy::VirtualKeyModifiers_Menu) != 0)
    {
        IFC_RETURN(ConcatVirtualKey(wsy::VirtualKey_Menu, stringRepresentationLocal));
    }

    if ((modifiers & wsy::VirtualKeyModifiers_Windows) != 0)
    {
        IFC_RETURN(ConcatVirtualKey(wsy::VirtualKey_LeftWindows, stringRepresentationLocal));
    }

    if ((modifiers & wsy::VirtualKeyModifiers_Shift) != 0)
    {
        IFC_RETURN(ConcatVirtualKey(wsy::VirtualKey_Shift, stringRepresentationLocal));
    }

    IFC_RETURN(ConcatVirtualKey(key, stringRepresentationLocal));

    *stringRepresentation = stringRepresentationLocal.Detach();
    return S_OK;
}

_Check_return_ HRESULT KeyboardAccelerator::ConcatVirtualKey(_In_ wsy::VirtualKey key, _Inout_ wrl_wrappers::HString& keyboardAcceleratorString)
{

    wrl_wrappers::HString keyName;
    IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(GetResourceStringIdFromVirtualKey(key), keyName.ReleaseAndGetAddressOf()));

    if (WindowsIsStringEmpty(keyboardAcceleratorString.Get()))
    {
        // If this is the first key string we've accounted for, then
        // we can just set the keyboard accelerator string equal to it.
        keyboardAcceleratorString = std::move(keyName);
    }
    else
    {
        // Otherwise, if we already had an existing keyboard accelerator string,
        // then we'll use the formatting string to join strings together
        // to combine it with the new key string.
        wrl_wrappers::HString joiningFormatString;
        IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(KEYBOARD_ACCELERATOR_TEXT_JOIN, joiningFormatString.ReleaseAndGetAddressOf()));

        WCHAR buffer[256];
        IFCEXPECT_RETURN(swprintf_s(
            buffer, ARRAYSIZE(buffer),
            joiningFormatString.GetRawBuffer(nullptr),
            keyboardAcceleratorString.GetRawBuffer(nullptr),
            keyName.GetRawBuffer(nullptr)) >= 0);

        IFC_RETURN(keyboardAcceleratorString.Set(buffer));
    }

    return S_OK;
}

XUINT32 GetResourceStringIdFromVirtualKey(_In_ wsy::VirtualKey key)
{
    switch (key)
    {
        case wsy::VirtualKey_A: return TEXT_VK_A;
        case wsy::VirtualKey_Accept: return TEXT_VK_ACCEPT;
        case wsy::VirtualKey_Add:
        #pragma warning(suppress : 4063)
        case VK_OEM_PLUS: return TEXT_VK_ADD;
        case wsy::VirtualKey_Application: return TEXT_VK_APPLICATION;
        case wsy::VirtualKey_B: return TEXT_VK_B;
        case wsy::VirtualKey_Back: return TEXT_VK_BACK;
        case wsy::VirtualKey_C: return TEXT_VK_C;
        case wsy::VirtualKey_Cancel: return TEXT_VK_CANCEL;
        case wsy::VirtualKey_CapitalLock: return TEXT_VK_CAPITALLOCK;
        case wsy::VirtualKey_Clear: return TEXT_VK_CLEAR;
        case wsy::VirtualKey_Control: return TEXT_VK_CONTROL;
        case wsy::VirtualKey_Convert: return TEXT_VK_CONVERT;
        case wsy::VirtualKey_D: return TEXT_VK_D;
        case wsy::VirtualKey_Decimal: return TEXT_VK_DECIMAL;
        case wsy::VirtualKey_Delete: return TEXT_VK_DELETE;
        case wsy::VirtualKey_Divide: return TEXT_VK_DIVIDE;
        case wsy::VirtualKey_Down: return TEXT_VK_DOWN;
        case wsy::VirtualKey_E: return TEXT_VK_E;
        case wsy::VirtualKey_End: return TEXT_VK_END;
        case wsy::VirtualKey_Enter: return TEXT_VK_ENTER;
        case wsy::VirtualKey_Escape: return TEXT_VK_ESCAPE;
        case wsy::VirtualKey_Execute: return TEXT_VK_EXECUTE;
        case wsy::VirtualKey_F: return TEXT_VK_F;
        case wsy::VirtualKey_F1: return TEXT_VK_F1;
        case wsy::VirtualKey_F10: return TEXT_VK_F10;
        case wsy::VirtualKey_F11: return TEXT_VK_F11;
        case wsy::VirtualKey_F12: return TEXT_VK_F12;
        case wsy::VirtualKey_F13: return TEXT_VK_F13;
        case wsy::VirtualKey_F14: return TEXT_VK_F14;
        case wsy::VirtualKey_F15: return TEXT_VK_F15;
        case wsy::VirtualKey_F16: return TEXT_VK_F16;
        case wsy::VirtualKey_F17: return TEXT_VK_F17;
        case wsy::VirtualKey_F18: return TEXT_VK_F18;
        case wsy::VirtualKey_F19: return TEXT_VK_F19;
        case wsy::VirtualKey_F2: return TEXT_VK_F2;
        case wsy::VirtualKey_F20: return TEXT_VK_F20;
        case wsy::VirtualKey_F21: return TEXT_VK_F21;
        case wsy::VirtualKey_F22: return TEXT_VK_F22;
        case wsy::VirtualKey_F23: return TEXT_VK_F23;
        case wsy::VirtualKey_F24: return TEXT_VK_F24;
        case wsy::VirtualKey_F3: return TEXT_VK_F3;
        case wsy::VirtualKey_F4: return TEXT_VK_F4;
        case wsy::VirtualKey_F5: return TEXT_VK_F5;
        case wsy::VirtualKey_F6: return TEXT_VK_F6;
        case wsy::VirtualKey_F7: return TEXT_VK_F7;
        case wsy::VirtualKey_F8: return TEXT_VK_F8;
        case wsy::VirtualKey_F9: return TEXT_VK_F9;
        case wsy::VirtualKey_Favorites: return TEXT_VK_FAVORITES;
        case wsy::VirtualKey_Final: return TEXT_VK_FINAL;
        case wsy::VirtualKey_G: return TEXT_VK_G;
        case wsy::VirtualKey_GamepadA: return TEXT_VK_GAMEPADA;
        case wsy::VirtualKey_GamepadB: return TEXT_VK_GAMEPADB;
        case wsy::VirtualKey_GamepadDPadDown: return TEXT_VK_GAMEPADDPADDOWN;
        case wsy::VirtualKey_GamepadDPadLeft: return TEXT_VK_GAMEPADDPADLEFT;
        case wsy::VirtualKey_GamepadDPadRight: return TEXT_VK_GAMEPADDPADRIGHT;
        case wsy::VirtualKey_GamepadDPadUp: return TEXT_VK_GAMEPADDPADUP;
        case wsy::VirtualKey_GamepadLeftShoulder: return TEXT_VK_GAMEPADLEFTSHOULDER;
        case wsy::VirtualKey_GamepadLeftThumbstickButton: return TEXT_VK_GAMEPADLEFTTHUMBSTICKBUTTON;
        case wsy::VirtualKey_GamepadLeftThumbstickDown: return TEXT_VK_GAMEPADLEFTTHUMBSTICKDOWN;
        case wsy::VirtualKey_GamepadLeftThumbstickLeft: return TEXT_VK_GAMEPADLEFTTHUMBSTICKLEFT;
        case wsy::VirtualKey_GamepadLeftThumbstickRight: return TEXT_VK_GAMEPADLEFTTHUMBSTICKRIGHT;
        case wsy::VirtualKey_GamepadLeftThumbstickUp: return TEXT_VK_GAMEPADLEFTTHUMBSTICKUP;
        case wsy::VirtualKey_GamepadLeftTrigger: return TEXT_VK_GAMEPADLEFTTRIGGER;
        case wsy::VirtualKey_GamepadMenu: return TEXT_VK_GAMEPADMENU;
        case wsy::VirtualKey_GamepadRightShoulder: return TEXT_VK_GAMEPADRIGHTSHOULDER;
        case wsy::VirtualKey_GamepadRightThumbstickButton: return TEXT_VK_GAMEPADRIGHTTHUMBSTICKBUTTON;
        case wsy::VirtualKey_GamepadRightThumbstickDown: return TEXT_VK_GAMEPADRIGHTTHUMBSTICKDOWN;
        case wsy::VirtualKey_GamepadRightThumbstickLeft: return TEXT_VK_GAMEPADRIGHTTHUMBSTICKLEFT;
        case wsy::VirtualKey_GamepadRightThumbstickRight: return TEXT_VK_GAMEPADRIGHTTHUMBSTICKRIGHT;
        case wsy::VirtualKey_GamepadRightThumbstickUp: return TEXT_VK_GAMEPADRIGHTTHUMBSTICKUP;
        case wsy::VirtualKey_GamepadRightTrigger: return TEXT_VK_GAMEPADRIGHTTRIGGER;
        case wsy::VirtualKey_GamepadView: return TEXT_VK_GAMEPADVIEW;
        case wsy::VirtualKey_GamepadX: return TEXT_VK_GAMEPADX;
        case wsy::VirtualKey_GamepadY: return TEXT_VK_GAMEPADY;
        case wsy::VirtualKey_GoBack: return TEXT_VK_GOBACK;
        case wsy::VirtualKey_GoForward: return TEXT_VK_GOFORWARD;
        case wsy::VirtualKey_GoHome: return TEXT_VK_GOHOME;
        case wsy::VirtualKey_H: return TEXT_VK_H;
        case wsy::VirtualKey_Hangul: return TEXT_VK_HANGUL;
        case wsy::VirtualKey_Hanja: return TEXT_VK_HANJA;
        case wsy::VirtualKey_Help: return TEXT_VK_HELP;
        case wsy::VirtualKey_Home: return TEXT_VK_HOME;
        case wsy::VirtualKey_I: return TEXT_VK_I;
        #pragma warning(suppress : 4063)
        case VK_IME_ON: return TEXT_VK_IMEON;
        #pragma warning(suppress : 4063)
        case VK_IME_OFF: return TEXT_VK_IMEOFF;
        case wsy::VirtualKey_Insert: return TEXT_VK_INSERT;
        case wsy::VirtualKey_J: return TEXT_VK_J;
        case wsy::VirtualKey_Junja: return TEXT_VK_JUNJA;
        case wsy::VirtualKey_K: return TEXT_VK_K;
        case wsy::VirtualKey_L: return TEXT_VK_L;
        case wsy::VirtualKey_Left: return TEXT_VK_LEFT;
        case wsy::VirtualKey_LeftButton: return TEXT_VK_LEFTBUTTON;
        case wsy::VirtualKey_LeftControl: return TEXT_VK_CONTROL;
        case wsy::VirtualKey_LeftMenu: return TEXT_VK_MENU;
        case wsy::VirtualKey_LeftShift: return TEXT_VK_SHIFT;
        case wsy::VirtualKey_LeftWindows: return TEXT_VK_WINDOWS;
        case wsy::VirtualKey_M: return TEXT_VK_M;
        case wsy::VirtualKey_Menu: return TEXT_VK_MENU;
        case wsy::VirtualKey_MiddleButton: return TEXT_VK_MIDDLEBUTTON;
        case wsy::VirtualKey_ModeChange: return TEXT_VK_MODECHANGE;
        case wsy::VirtualKey_Multiply: return TEXT_VK_MULTIPLY;
        case wsy::VirtualKey_N: return TEXT_VK_N;
        case wsy::VirtualKey_NavigationAccept: return TEXT_VK_NAVIGATIONACCEPT;
        case wsy::VirtualKey_NavigationCancel: return TEXT_VK_NAVIGATIONCANCEL;
        case wsy::VirtualKey_NavigationDown: return TEXT_VK_NAVIGATIONDOWN;
        case wsy::VirtualKey_NavigationLeft: return TEXT_VK_NAVIGATIONLEFT;
        case wsy::VirtualKey_NavigationMenu: return TEXT_VK_NAVIGATIONMENU;
        case wsy::VirtualKey_NavigationRight: return TEXT_VK_NAVIGATIONRIGHT;
        case wsy::VirtualKey_NavigationUp: return TEXT_VK_NAVIGATIONUP;
        case wsy::VirtualKey_NavigationView: return TEXT_VK_NAVIGATIONVIEW;
        case wsy::VirtualKey_NonConvert: return TEXT_VK_NONCONVERT;
        case wsy::VirtualKey_None: return TEXT_VK_NONE;
        case wsy::VirtualKey_Number0: return TEXT_VK_NUMBER0;
        case wsy::VirtualKey_Number1: return TEXT_VK_NUMBER1;
        case wsy::VirtualKey_Number2: return TEXT_VK_NUMBER2;
        case wsy::VirtualKey_Number3: return TEXT_VK_NUMBER3;
        case wsy::VirtualKey_Number4: return TEXT_VK_NUMBER4;
        case wsy::VirtualKey_Number5: return TEXT_VK_NUMBER5;
        case wsy::VirtualKey_Number6: return TEXT_VK_NUMBER6;
        case wsy::VirtualKey_Number7: return TEXT_VK_NUMBER7;
        case wsy::VirtualKey_Number8: return TEXT_VK_NUMBER8;
        case wsy::VirtualKey_Number9: return TEXT_VK_NUMBER9;
        case wsy::VirtualKey_NumberKeyLock: return TEXT_VK_NUMBERKEYLOCK;
        case wsy::VirtualKey_NumberPad0: return TEXT_VK_NUMBERPAD0;
        case wsy::VirtualKey_NumberPad1: return TEXT_VK_NUMBERPAD1;
        case wsy::VirtualKey_NumberPad2: return TEXT_VK_NUMBERPAD2;
        case wsy::VirtualKey_NumberPad3: return TEXT_VK_NUMBERPAD3;
        case wsy::VirtualKey_NumberPad4: return TEXT_VK_NUMBERPAD4;
        case wsy::VirtualKey_NumberPad5: return TEXT_VK_NUMBERPAD5;
        case wsy::VirtualKey_NumberPad6: return TEXT_VK_NUMBERPAD6;
        case wsy::VirtualKey_NumberPad7: return TEXT_VK_NUMBERPAD7;
        case wsy::VirtualKey_NumberPad8: return TEXT_VK_NUMBERPAD8;
        case wsy::VirtualKey_NumberPad9: return TEXT_VK_NUMBERPAD9;
        case wsy::VirtualKey_O: return TEXT_VK_O;
        case wsy::VirtualKey_P: return TEXT_VK_P;
        case wsy::VirtualKey_PageDown: return TEXT_VK_PAGEDOWN;
        case wsy::VirtualKey_PageUp: return TEXT_VK_PAGEUP;
        case wsy::VirtualKey_Pause: return TEXT_VK_PAUSE;
        case wsy::VirtualKey_Print: return TEXT_VK_PRINT;
        case wsy::VirtualKey_Q: return TEXT_VK_Q;
        case wsy::VirtualKey_R: return TEXT_VK_R;
        case wsy::VirtualKey_Refresh: return TEXT_VK_REFRESH;
        case wsy::VirtualKey_Right: return TEXT_VK_RIGHT;
        case wsy::VirtualKey_RightButton: return TEXT_VK_RIGHTBUTTON;
        case wsy::VirtualKey_RightControl: return TEXT_VK_CONTROL;
        case wsy::VirtualKey_RightMenu: return TEXT_VK_MENU;
        case wsy::VirtualKey_RightShift: return TEXT_VK_SHIFT;
        case wsy::VirtualKey_RightWindows: return TEXT_VK_WINDOWS;
        case wsy::VirtualKey_S: return TEXT_VK_S;
        case wsy::VirtualKey_Scroll: return TEXT_VK_SCROLL;
        case wsy::VirtualKey_Search: return TEXT_VK_SEARCH;
        case wsy::VirtualKey_Select: return TEXT_VK_SELECT;
        case wsy::VirtualKey_Separator: return TEXT_VK_SEPARATOR;
        case wsy::VirtualKey_Shift: return TEXT_VK_SHIFT;
        case wsy::VirtualKey_Sleep: return TEXT_VK_SLEEP;
        case wsy::VirtualKey_Snapshot: return TEXT_VK_SNAPSHOT;
        case wsy::VirtualKey_Space: return TEXT_VK_SPACE;
        case wsy::VirtualKey_Stop: return TEXT_VK_STOP;
        case wsy::VirtualKey_Subtract: return TEXT_VK_SUBTRACT;
        case wsy::VirtualKey_T: return TEXT_VK_T;
        case wsy::VirtualKey_Tab: return TEXT_VK_TAB;
        case wsy::VirtualKey_U: return TEXT_VK_U;
        case wsy::VirtualKey_Up: return TEXT_VK_UP;
        case wsy::VirtualKey_V: return TEXT_VK_V;
        case wsy::VirtualKey_W: return TEXT_VK_W;
        case wsy::VirtualKey_X: return TEXT_VK_X;
        case wsy::VirtualKey_XButton1: return TEXT_VK_XBUTTON1;
        case wsy::VirtualKey_XButton2: return TEXT_VK_XBUTTON2;
        case wsy::VirtualKey_Y: return TEXT_VK_Y;
        case wsy::VirtualKey_Z: return TEXT_VK_Z;
        default: IFCFAILFAST(E_FAIL); return TEXT_VK_NONE;
    }
}

/*static*/_Check_return_ HRESULT KeyboardAccelerator::SetToolTip(
    _In_ CKeyboardAccelerator* pNativeAccelerator,
    _In_ CDependencyObject* pParentElement)
{
    //Retrive the string representing accelerator defined on element.
    HSTRING stringRepresentation;
    ctl::ComPtr<DependencyObject> spAcceleratorDO;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pNativeAccelerator, &spAcceleratorDO));
    IFC_RETURN(spAcceleratorDO.Cast<KeyboardAccelerator>()->GetStringRepresentation(&stringRepresentation));

    ctl::ComPtr<IInspectable> textValue;
    wf::IPropertyValueStatics* pPropertyValueStatic = pParentElement->GetContext()->GetPropertyValueStatics();
    pPropertyValueStatic->CreateString(stringRepresentation, &textValue);

    CDependencyObject* pToolTipTargetDO = nullptr;
    CValue value;

    IFC_RETURN(pParentElement->GetValueByIndex(KnownPropertyIndex::UIElement_KeyboardAcceleratorPlacementTarget, &value));
    IFC_RETURN(DirectUI::CValueBoxer::UnwrapWeakRef(
        &value,
        DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_KeyboardAcceleratorPlacementTarget),
        &pToolTipTargetDO));

    if (pToolTipTargetDO != nullptr)
    {
        pParentElement = pToolTipTargetDO;
    }
    ctl::ComPtr<DependencyObject> spParentDO;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pParentElement, &spParentDO));

    IFC_RETURN(ToolTipServiceFactory::SetKeyboardAcceleratorToolTipStatic(spParentDO.Get(), textValue.Get()));

    return S_OK;
}
