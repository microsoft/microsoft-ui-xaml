// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Derivation of UICommand that sets default property values
//      based on an enum value representing standard commands
//      (cut, copy, select all, etc.).

#include "precomp.h"
#include "StandardUICommand.g.h"
#include "KeyboardAccelerator.g.h"
#include "SymbolIconSource.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT StandardUICommand::PrepareState()
{
    IFC_RETURN(StandardUICommandGenerated::PrepareState());

    xaml_input::StandardUICommandKind kind;
    IFC_RETURN(get_Kind(&kind));

    IFC_RETURN(PopulateForKind(kind));
    return S_OK;
}

_Check_return_ HRESULT StandardUICommand::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(StandardUICommandGenerated::OnPropertyChanged2(args));

    if (m_settingPropertyInternally)
    {
        return S_OK;
    }

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::StandardUICommand_Kind:
            IFC_RETURN(PopulateForKind(static_cast<xaml_input::StandardUICommandKind>(args.m_pNewValue->AsEnum())));
            break;

        case KnownPropertyIndex::XamlUICommand_Label:
            m_ownsLabel = false;
            break;

        case KnownPropertyIndex::XamlUICommand_IconSource:
            m_ownsIconSource = false;
            break;

        case KnownPropertyIndex::XamlUICommand_Description:
            m_ownsDescription = false;
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT StandardUICommand::EnterImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bUseLayoutRounding)
{
    if (bLive)
    {
        xaml_input::StandardUICommandKind kind;
        IFC_RETURN(get_Kind(&kind));

        if (kind == xaml_input::StandardUICommandKind_None)
        {
            IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_STANDARDUICOMMAND_KINDNOTSET));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT StandardUICommand::PopulateForKind(xaml_input::StandardUICommandKind kind)
{
    switch (kind)
    {
    case xaml_input::StandardUICommandKind_None:
        break;

    case xaml_input::StandardUICommandKind_Cut:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_CUT,
            xaml_controls::Symbol_Cut,
            TEXT_COMMAND_KEYBOARDACCELERATORKEY_CUT,
            wsy::VirtualKeyModifiers_Control,
            TEXT_COMMAND_DESCRIPTION_CUT));
        break;

    case xaml_input::StandardUICommandKind_Copy:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_COPY,
            xaml_controls::Symbol_Copy,
            TEXT_COMMAND_KEYBOARDACCELERATORKEY_COPY,
            wsy::VirtualKeyModifiers_Control,
            TEXT_COMMAND_DESCRIPTION_COPY));
        break;

    case xaml_input::StandardUICommandKind_Paste:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_PASTE,
            xaml_controls::Symbol_Paste,
            TEXT_COMMAND_KEYBOARDACCELERATORKEY_PASTE,
            wsy::VirtualKeyModifiers_Control,
            TEXT_COMMAND_DESCRIPTION_PASTE));
        break;

    case xaml_input::StandardUICommandKind_SelectAll:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_SELECTALL,
            xaml_controls::Symbol_SelectAll,
            TEXT_COMMAND_KEYBOARDACCELERATORKEY_SELECTALL,
            wsy::VirtualKeyModifiers_Control,
            TEXT_COMMAND_DESCRIPTION_SELECTALL));
        break;

    case xaml_input::StandardUICommandKind_Delete:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_DELETE,
            xaml_controls::Symbol_Delete,
            wsy::VirtualKey_Delete,
            wsy::VirtualKeyModifiers_None,
            TEXT_COMMAND_DESCRIPTION_DELETE));
        break;

    case xaml_input::StandardUICommandKind_Share:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_SHARE,
            xaml_controls::Symbol_Share,
            wsy::VirtualKey_None,
            wsy::VirtualKeyModifiers_None,
            TEXT_COMMAND_DESCRIPTION_SHARE));
        break;

    case xaml_input::StandardUICommandKind_Save:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_SAVE,
            xaml_controls::Symbol_Save,
            TEXT_COMMAND_KEYBOARDACCELERATORKEY_SAVE,
            wsy::VirtualKeyModifiers_Control,
            TEXT_COMMAND_DESCRIPTION_SAVE));
        break;

    case xaml_input::StandardUICommandKind_Open:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_OPEN,
            xaml_controls::Symbol_OpenFile,
            TEXT_COMMAND_KEYBOARDACCELERATORKEY_OPEN,
            wsy::VirtualKeyModifiers_Control,
            TEXT_COMMAND_DESCRIPTION_OPEN));
        break;

    case xaml_input::StandardUICommandKind_Close:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_CLOSE,
            xaml_controls::Symbol_Cancel,
            TEXT_COMMAND_KEYBOARDACCELERATORKEY_CLOSE,
            wsy::VirtualKeyModifiers_Control,
            TEXT_COMMAND_DESCRIPTION_CLOSE));
        break;

    case xaml_input::StandardUICommandKind_Pause:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_PAUSE,
            xaml_controls::Symbol_Pause,
            wsy::VirtualKey_None,
            wsy::VirtualKeyModifiers_None,
            TEXT_COMMAND_DESCRIPTION_PAUSE));
        break;

    case xaml_input::StandardUICommandKind_Play:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_PLAY,
            xaml_controls::Symbol_Play,
            wsy::VirtualKey_None,
            wsy::VirtualKeyModifiers_None,
            TEXT_COMMAND_DESCRIPTION_PLAY));
        break;

    case xaml_input::StandardUICommandKind_Stop:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_STOP,
            xaml_controls::Symbol_Stop,
            wsy::VirtualKey_None,
            wsy::VirtualKeyModifiers_None,
            TEXT_COMMAND_DESCRIPTION_STOP));
        break;

    case xaml_input::StandardUICommandKind_Forward:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_FORWARD,
            xaml_controls::Symbol_Forward,
            wsy::VirtualKey_None,
            wsy::VirtualKeyModifiers_None,
            TEXT_COMMAND_DESCRIPTION_FORWARD));
        break;

    case xaml_input::StandardUICommandKind_Backward:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_BACKWARD,
            xaml_controls::Symbol_Back,
            wsy::VirtualKey_None,
            wsy::VirtualKeyModifiers_None,
            TEXT_COMMAND_DESCRIPTION_BACKWARD));
        break;

    case xaml_input::StandardUICommandKind_Undo:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_UNDO,
            xaml_controls::Symbol_Undo,
            TEXT_COMMAND_KEYBOARDACCELERATORKEY_UNDO,
            wsy::VirtualKeyModifiers_Control,
            TEXT_COMMAND_DESCRIPTION_UNDO));
        break;

    case xaml_input::StandardUICommandKind_Redo:
        IFC_RETURN(PopulateWithProperties(
            TEXT_COMMAND_LABEL_REDO,
            xaml_controls::Symbol_Redo,
            TEXT_COMMAND_KEYBOARDACCELERATORKEY_REDO,
            wsy::VirtualKeyModifiers_Control,
            TEXT_COMMAND_DESCRIPTION_REDO));
        break;

    default:
        ASSERT(FALSE);
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT StandardUICommand::PopulateWithProperties(
    int labelResourceId,
    xaml_controls::Symbol symbol,
    int acceleratorKeyResourceId,
    wsy::VirtualKeyModifiers acceleratorModifiers,
    int descriptionResourceId)
{
    // If the accelerator key we've been passed is a string,
    // we need to convert it to an enum value.
    wrl_wrappers::HString acceleratorKeyString;
    IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(acceleratorKeyResourceId, acceleratorKeyString.ReleaseAndGetAddressOf()));

    WCHAR acceleratorKeyChar = acceleratorKeyString.Length() >= 1 ? acceleratorKeyString.GetRawBuffer(nullptr)[0] : '\0';
    wsy::VirtualKey acceleratorKey = wsy::VirtualKey_None;

    switch (acceleratorKeyChar)
    {
    case L'A':
    case L'a':
        acceleratorKey = wsy::VirtualKey_A;
        break;
    case L'B':
    case L'b':
        acceleratorKey = wsy::VirtualKey_B;
        break;
    case L'C':
    case L'c':
        acceleratorKey = wsy::VirtualKey_C;
        break;
    case L'D':
    case L'd':
        acceleratorKey = wsy::VirtualKey_D;
        break;
    case L'E':
    case L'e':
        acceleratorKey = wsy::VirtualKey_E;
        break;
    case L'F':
    case L'f':
        acceleratorKey = wsy::VirtualKey_F;
        break;
    case L'G':
    case L'g':
        acceleratorKey = wsy::VirtualKey_G;
        break;
    case L'H':
    case L'h':
        acceleratorKey = wsy::VirtualKey_H;
        break;
    case L'I':
    case L'i':
        acceleratorKey = wsy::VirtualKey_I;
        break;
    case L'J':
    case L'j':
        acceleratorKey = wsy::VirtualKey_J;
        break;
    case L'K':
    case L'k':
        acceleratorKey = wsy::VirtualKey_K;
        break;
    case L'L':
    case L'l':
        acceleratorKey = wsy::VirtualKey_L;
        break;
    case L'M':
    case L'm':
        acceleratorKey = wsy::VirtualKey_M;
        break;
    case L'N':
    case L'n':
        acceleratorKey = wsy::VirtualKey_N;
        break;
    case L'O':
    case L'o':
        acceleratorKey = wsy::VirtualKey_O;
        break;
    case L'P':
    case L'p':
        acceleratorKey = wsy::VirtualKey_P;
        break;
    case L'Q':
    case L'q':
        acceleratorKey = wsy::VirtualKey_Q;
        break;
    case L'R':
    case L'r':
        acceleratorKey = wsy::VirtualKey_R;
        break;
    case L'S':
    case L's':
        acceleratorKey = wsy::VirtualKey_S;
        break;
    case L'T':
    case L't':
        acceleratorKey = wsy::VirtualKey_T;
        break;
    case L'U':
    case L'u':
        acceleratorKey = wsy::VirtualKey_U;
        break;
    case L'V':
    case L'v':
        acceleratorKey = wsy::VirtualKey_V;
        break;
    case L'W':
    case L'w':
        acceleratorKey = wsy::VirtualKey_W;
        break;
    case L'X':
    case L'x':
        acceleratorKey = wsy::VirtualKey_X;
        break;
    case L'Y':
    case L'y':
        acceleratorKey = wsy::VirtualKey_Y;
        break;
    case L'Z':
    case L'z':
        acceleratorKey = wsy::VirtualKey_Z;
        break;
    }

    IFC_RETURN(PopulateWithProperties(
        labelResourceId,
        symbol,
        acceleratorKey,
        acceleratorKey != wsy::VirtualKey_None ? acceleratorModifiers : wsy::VirtualKeyModifiers_None,
        descriptionResourceId));

    return S_OK;
}

_Check_return_ HRESULT StandardUICommand::PopulateWithProperties(
    int labelResourceId,
    xaml_controls::Symbol symbol,
    wsy::VirtualKey acceleratorKey,
    wsy::VirtualKeyModifiers acceleratorModifiers,
    int descriptionResourceId)
{
    IFC_RETURN(SetLabelIfUnset(labelResourceId));
    IFC_RETURN(SetIconSourceIfUnset(symbol));
    IFC_RETURN(SetKeyboardAcceleratorIfUnset(acceleratorKey, acceleratorModifiers));
    IFC_RETURN(SetDescriptionIfUnset(descriptionResourceId));
    return S_OK;
}

_Check_return_ HRESULT StandardUICommand::SetLabelIfUnset(int labelResourceId)
{
    if (m_ownsLabel)
    {
        auto scopeGuard = wil::scope_exit([&]
        {
            m_settingPropertyInternally = false;
        });

        m_settingPropertyInternally = true;

        wrl_wrappers::HString label;
        IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(labelResourceId, label.ReleaseAndGetAddressOf()));
        IFC_RETURN(put_Label(label.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT StandardUICommand::SetIconSourceIfUnset(xaml_controls::Symbol symbol)
{
    if (m_ownsIconSource)
    {
        auto scopeGuard = wil::scope_exit([&]
        {
            m_settingPropertyInternally = false;
        });

        m_settingPropertyInternally = true;

        ctl::ComPtr<SymbolIconSource> symbolIconSource;
        IFC_RETURN(ctl::make(&symbolIconSource));
        IFC_RETURN(symbolIconSource->put_Symbol(symbol));

        IFC_RETURN(put_IconSource(symbolIconSource.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT StandardUICommand::SetKeyboardAcceleratorIfUnset(
    wsy::VirtualKey acceleratorKey,
    wsy::VirtualKeyModifiers acceleratorModifiers)
{
    if (!m_ownsKeyboardAccelerator)
    {
        return S_OK;
    }

    ctl::ComPtr<wfc::IVector<xaml_input::KeyboardAccelerator*>> keyboardAccelerators;
    IFC_RETURN(get_KeyboardAccelerators(&keyboardAccelerators));

    UINT keyboardAcceleratorCount;
    IFC_RETURN(keyboardAccelerators->get_Size(&keyboardAcceleratorCount));

    // If we ever detect a keyboard accelerator value that we didn't set,
    // we'll cede ownership at that point to the app developer, and won't
    // touch the keyboard accelerators further.
    if (keyboardAcceleratorCount == 0 &&
        m_previousAcceleratorKey != wsy::VirtualKey_None)
    {
        // Unless the accelerator key was "None", we should always have an accelerator -
        // so, if we don't, then we know that the app developer has changed things.
        m_ownsKeyboardAccelerator = false;
        return S_OK;
    }
    else if (keyboardAcceleratorCount == 1)
    {
        ctl::ComPtr<IKeyboardAccelerator> keyboardAccelerator;
        IFC_RETURN(keyboardAccelerators->GetAt(0, &keyboardAccelerator));

        wsy::VirtualKey currentAcceleratorKey;
        wsy::VirtualKeyModifiers currentAcceleratorModifiers;
        IFC_RETURN(keyboardAccelerator->get_Key(&currentAcceleratorKey));
        IFC_RETURN(keyboardAccelerator->get_Modifiers(&currentAcceleratorModifiers));

        if (currentAcceleratorKey != m_previousAcceleratorKey ||
            currentAcceleratorModifiers != m_previousAcceleratorModifiers)
        {
            m_ownsKeyboardAccelerator = false;
            return S_OK;
        }
    }
    else if (keyboardAcceleratorCount > 1)
    {
        // We'll never set more than one keyboard accelerator,
        // so if we have more than one, then we know that the app developer
        // has changed things.
        m_ownsKeyboardAccelerator = false;
        return S_OK;
    }

    IFC_RETURN(keyboardAccelerators->Clear());

    if (acceleratorKey != wsy::VirtualKey_None)
    {
        ctl::ComPtr<KeyboardAccelerator> keyboardAccelerator;
        IFC_RETURN(ctl::make(&keyboardAccelerator));
        IFC_RETURN(keyboardAccelerator->put_Key(acceleratorKey));
        IFC_RETURN(keyboardAccelerator->put_Modifiers(acceleratorModifiers));
        IFC_RETURN(keyboardAccelerators->Append(keyboardAccelerator.Get()));
    }

    m_previousAcceleratorKey = acceleratorKey;
    m_previousAcceleratorModifiers = acceleratorModifiers;

    return S_OK;
}

_Check_return_ HRESULT StandardUICommand::SetDescriptionIfUnset(int descriptionResourceId)
{
    if (m_ownsDescription)
    {
        auto scopeGuard = wil::scope_exit([&]
        {
            m_settingPropertyInternally = false;
        });

        m_settingPropertyInternally = true;

        wrl_wrappers::HString description;
        IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(descriptionResourceId, description.ReleaseAndGetAddressOf()));
        IFC_RETURN(put_Description(description.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT StandardUICommandFactory::CreateInstanceWithKindImpl(
    _In_ xaml_input::StandardUICommandKind kind,
    _In_opt_ IInspectable* outer,
    _Outptr_ IInspectable** inner,
    _Outptr_ IStandardUICommand** instance)
{
    ctl::ComPtr<IStandardUICommand> standardUICommand;
    IFC_RETURN(CreateInstance(outer, inner, &standardUICommand));

    ctl::ComPtr<StandardUICommand> standardUICommandImpl;
    IFC_RETURN(standardUICommand.As(&standardUICommandImpl));
    IFC_RETURN(standardUICommandImpl->SetValueByKnownIndex(KnownPropertyIndex::StandardUICommand_Kind, kind));

    *instance = standardUICommand.Detach();
    return S_OK;
}

_Check_return_ HRESULT StandardUICommandFactory::get_KindPropertyImpl(_Outptr_result_maybenull_ xaml::IDependencyProperty** ppValue)
{
    IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::StandardUICommand_Kind, ppValue));
    return S_OK;
}