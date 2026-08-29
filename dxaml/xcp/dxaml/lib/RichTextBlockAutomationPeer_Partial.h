// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RichTextBlockAutomationPeer.g.h"

namespace DirectUI
{
    class TextAdapter;

    PARTIAL_CLASS(RichTextBlockAutomationPeer)
    {
        public:
            RichTextBlockAutomationPeer();
            ~RichTextBlockAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);
            IFACEMETHOD(GetChildrenCore)(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue);

            DirectUI::TextAdapter* m_pTextPattern;
    };
}
