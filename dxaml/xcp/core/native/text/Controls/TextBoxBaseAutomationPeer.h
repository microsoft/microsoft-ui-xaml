// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef TEXT_BOX_BASE_AUTOMATIONPEER_H
#define TEXT_BOX_BASE_AUTOMATIONPEER_H

class CTextBoxBase;

// Base class is for accessibility of editable DirectUI text controls. This class is not exposed
// publicly but consolidates implementation across public derived classes. This class builds off
// of Window's ITextServices, the windowless RichEdit control.
class CTextBoxBaseAutomationPeer : public CFrameworkElementAutomationPeer
{
public:
    // CDependencyObject overrides.
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextBoxBaseAutomationPeer>::Index;
    }

    // CAutomationPeer overrides.
    bool IsHostedWindowlessElementProvider() final;

    void* GetWindowlessRawElementProviderSimple() final;

    void* GetUnwrappedPattern(_In_ XINT32 patternID) final;

    bool GetPreventKeyboardDisplayOnProgrammaticFocus() final;

    HRESULT SetFocusHelper() final;

private:
    xref::weakref_ptr<CTextBoxBase> m_textBoxBaseWeakRef;

protected:
    const xref::weakref_ptr<CTextBoxBase>& GetTextBoxBaseWeakRef() const
    {
        return m_textBoxBaseWeakRef;
    }

    CTextBoxBaseAutomationPeer(
        _In_ CCoreServices *core,
        _In_ CValue &value);
};

#endif // TEXT_BOX_BASE_AUTOMATIONPEER_H
