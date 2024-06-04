// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCharacterReceivedRoutedEventArgs final : public CRoutedEventArgs
{
public:
    CCharacterReceivedRoutedEventArgs()
    {
        m_platformKeyCode = wsy::VirtualKey::VirtualKey_None;
        m_msgID = XCP_NULL;
    }

    // Destructor
    ~CCharacterReceivedRoutedEventArgs() override
    {
        m_platformKeyCode = wsy::VirtualKey::VirtualKey_None;
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    // Public methods
    _Check_return_ HRESULT get_Character(_Out_ WCHAR* pKeyCode);

    // Public fields
    wsy::VirtualKey      m_platformKeyCode;
    PhysicalKeyStatus   m_physicalKeyStatus;
    MessageMap          m_msgID;
};
