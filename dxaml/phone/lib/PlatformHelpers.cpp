// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <microsoft.ui.input.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

wrl::ComPtr<xaml::IApplication> PlatformHelpers::s_spApp;


_Check_return_ HRESULT
PlatformHelpers::LookupThemeResource(_In_ HSTRING themeResourceName, _Outptr_ IInspectable** ppThemeResource)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IResourceDictionary> spResourceDictionary;
    wrl::ComPtr<wfc::IMap<IInspectable*, IInspectable*>> spResourcesMap;
    wrl::ComPtr<IInspectable> spThemeResourceNameAsII;

    ARG_VALIDRETURNPOINTER(ppThemeResource);

    if(nullptr == s_spApp)
    {
        IFC(GetCurrentApp());
    }

    IFCPTR(s_spApp);

    IFC(s_spApp->get_Resources(&spResourceDictionary));
    IFC(spResourceDictionary.As(&spResourcesMap));
    IFC(Private::ValueBoxer::CreateString(themeResourceName, &spThemeResourceNameAsII));
    IFC(spResourcesMap->Lookup(spThemeResourceNameAsII.Get(), ppThemeResource));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT PlatformHelpers::GetKeyboardModifiers(_Out_ wsy::VirtualKeyModifiers* pnKeyboardModifiers)
{
    IFCEXPECT_RETURN(pnKeyboardModifiers);
    *pnKeyboardModifiers = wsy::VirtualKeyModifiers_None;

    wrl::ComPtr<ixp::IInputKeyboardSourceStatics> InputKeyboardSourceStatics;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_InputKeyboardSource).Get(),
        &InputKeyboardSourceStatics));

    wuc::CoreVirtualKeyStates keyState = {};
    IFC_RETURN(InputKeyboardSourceStatics->GetKeyStateForCurrentThread(wsy::VirtualKey_Menu, &keyState));
    if ((keyState & wuc::CoreVirtualKeyStates_Down))
    {
        *pnKeyboardModifiers |= wsy::VirtualKeyModifiers_Menu;
    }

    IFC_RETURN(InputKeyboardSourceStatics->GetKeyStateForCurrentThread(wsy::VirtualKey_Control, &keyState));
    if ((keyState & wuc::CoreVirtualKeyStates_Down))
    {
        *pnKeyboardModifiers |= wsy::VirtualKeyModifiers_Control;
    }

    IFC_RETURN(InputKeyboardSourceStatics->GetKeyStateForCurrentThread(wsy::VirtualKey_Shift, &keyState));
    if ((keyState & wuc::CoreVirtualKeyStates_Down))
    {
        *pnKeyboardModifiers |= wsy::VirtualKeyModifiers_Shift;
    }

    IFC_RETURN(InputKeyboardSourceStatics->GetKeyStateForCurrentThread(wsy::VirtualKey_RightWindows, &keyState));
    if ((keyState & wuc::CoreVirtualKeyStates_Down))
    {
        *pnKeyboardModifiers |= wsy::VirtualKeyModifiers_Windows;
    }

    IFC_RETURN(InputKeyboardSourceStatics->GetKeyStateForCurrentThread(wsy::VirtualKey_LeftWindows, &keyState));
    if ((keyState & wuc::CoreVirtualKeyStates_Down))
    {
        *pnKeyboardModifiers |= wsy::VirtualKeyModifiers_Windows;
    }

    return S_OK;
}

_Check_return_ HRESULT PlatformHelpers::GetCurrentApp()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IApplicationStatics> spAppStatics;

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Application).Get(),
        &spAppStatics));
    IFC(spAppStatics->get_Current(&s_spApp));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT PlatformHelpers::RequestInteractionSoundForElement(_In_ xaml::ElementSoundKind soundToPlay, _In_ xaml::IDependencyObject* element)
{
    wrl::ComPtr<xaml::IElementSoundPlayerStatics> elementSoundsPlayer;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_ElementSoundPlayer).Get(),
        &elementSoundsPlayer));

    wrl::ComPtr<xaml::IElementSoundPlayerStaticsPrivate> elementSoundsPlayerPrivate;
    IFC_RETURN(elementSoundsPlayer.As(&elementSoundsPlayerPrivate));

    IFC_RETURN(elementSoundsPlayerPrivate->RequestInteractionSoundForElement(soundToPlay, element));

    return S_OK;
}

_Check_return_ HRESULT PlatformHelpers::GetEffectiveSoundMode(_In_ xaml::IDependencyObject* element, _Out_ xaml::ElementSoundMode* soundMode)
{
    wrl::ComPtr<xaml::IElementSoundPlayerStatics> elementSoundsPlayer;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_ElementSoundPlayer).Get(),
        &elementSoundsPlayer));

    wrl::ComPtr<xaml::IElementSoundPlayerStaticsPrivate> elementSoundsPlayerPrivate;
    IFC_RETURN(elementSoundsPlayer.As(&elementSoundsPlayerPrivate));

    IFC_RETURN(elementSoundsPlayerPrivate->GetEffectiveSoundMode(element, soundMode));

    return S_OK;
}

// If we want to test if two objects are the same object, but use the equals operator,
// then what we're really testing is whether the vtables are the same.
// If one of the two items are an interface type and the other is an implementation of that,
// then these two vtables will not be the same, and we'll return false, even though
// they actually represent the same object. The solution is to first QI both pointers
// to a known derived type, and then compare the resulting pointers from that -
// in that case, since they're both implementations of the same interface,
// the vtables for the same object will be the same, so comparing pointer equality will work OK.
bool PlatformHelpers::AreSameObject(_In_ IUnknown* first, _In_ IUnknown* second)
{
    auto firstIdentity = QueryInterfaceCast<IUnknown>(first);
    auto secondIdentity = QueryInterfaceCast<IUnknown>(second);

    // QI from IUnknown to IUnknown should never fail.
    ASSERT(!first || firstIdentity);
    ASSERT(!second || secondIdentity);

    return firstIdentity.Get() == secondIdentity.Get();
}

}}}} XAML_ABI_NAMESPACE_END
