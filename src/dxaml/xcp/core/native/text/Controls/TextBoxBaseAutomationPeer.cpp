// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

CTextBoxBaseAutomationPeer::CTextBoxBaseAutomationPeer(
    _In_ CCoreServices *core,
    _In_ CValue &value)
    : CFrameworkElementAutomationPeer(core, value)
{
    ASSERT(value.GetType() == valueObject);

    CTextBoxBase* textBoxBase;

    VERIFYHR(DoPointerCast(textBoxBase, value.AsObject()));
    ASSERT(textBoxBase);

    m_textBoxBaseWeakRef = xref::get_weakref(textBoxBase);
}

// Tells the UIAWrapper that this AutomationPeer will contain a windowless control whose true
// IRawElementProviderSimple needs to be retrieved. Instead of just calling GetFirstAP, call
// GetWindowlessRawElementProviderSimple for the first child.
bool CTextBoxBaseAutomationPeer::IsHostedWindowlessElementProvider()
{
    return true;
}

// Tells the UIAWrapper whether the touch keyboard should display on programmatic focus or not.
bool CTextBoxBaseAutomationPeer::GetPreventKeyboardDisplayOnProgrammaticFocus()
{
    return GetTextBoxBaseWeakRef().lock()->m_bPreventKeyboardDisplayOnProgrammaticFocus;
}

// Provides the first UIA child (i.e. the true IRawElementProviderSimple, not our version) for any
// UIA object whose corresponding UI tree contains a windowless control.
void* CTextBoxBaseAutomationPeer::GetWindowlessRawElementProviderSimple()
{
    HRESULT hr = S_OK;
    void* provider = nullptr;
    auto textBoxBase = GetTextBoxBaseWeakRef().lock();

    if (textBoxBase)
    {
        hr = textBoxBase->GetRichEditRawElementProviderSimple(&provider);
    }

    if (FAILED(hr))
    {
        return nullptr;
    }
    
    return provider;
}

// To obtain true supported patterns on window less control.
void* CTextBoxBaseAutomationPeer::GetUnwrappedPattern(_In_ XINT32 patternID)
{
    HRESULT hr = S_OK;
    void* pattern = nullptr;
    auto textBoxBase = GetTextBoxBaseWeakRef().lock();

    if (textBoxBase)
    {
        hr = textBoxBase->GetUnwrappedPattern(
            patternID,
            textBoxBase->AcceptsRichText(),
            &pattern);
    }

    if (FAILED(hr))
    {
        return nullptr;
    }
    
    return pattern;
}

HRESULT CTextBoxBaseAutomationPeer::SetFocusHelper()
{
    auto textBoxBase = GetTextBoxBaseWeakRef().lock();

    if (textBoxBase)
    {
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(textBoxBase);
        contentRoot->GetInputManager().OnSetFocusFromUIA();

        if (!textBoxBase->IsFocused())
        {
            // SIP on phone blocks on ProgrammaticFocus. But when Assistive technologies like Narrator
            // puts insertion point on edit control it calls setfocus via UIA. As with this interaction
            // we do not have the information whether the interaction happened due to Touch or Keyboard,
            // setting Programmatic focus seemed ideal.This worked fine on phone as Xaml on phone was
            // not respecting PreventKeyboardDisplayOnProgrammaticFocus, but with that fixed now, when 
            // Narrator interacts with edit boxes on phone with this setting SIP doesn't show up.
            // Here we want to make a very scoped change to set Pointer focus only when this property
            // is set and we are on edit controls.
            bool focusUpdated;

            if (GetPreventKeyboardDisplayOnProgrammaticFocus())
            {
                IFC_RETURN(textBoxBase->Focus(
                    DirectUI::FocusState::Pointer,
                    false /*animateIfBringIntoView*/,
                    &focusUpdated));
            }
            else
            {
                IFC_RETURN(textBoxBase->Focus(
                    DirectUI::FocusState::Programmatic,
                    false /*animateIfBringIntoView*/,
                    &focusUpdated));
            }
        }
    }

    return S_OK;
}
