// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XYFocusMocks.h"
#include <DependencyObjectAbstractionHelpers.h>
#include <UIElement.h>
#include <CControl.h>

#include "CValueBoxer.h"

#include "corep.h"
#include "RichTextBlock.h"
#include "TextBlock.h"

#include "Focusability.h"
#include "FocusProperties.h"

#include "focusmgr.h"

using namespace Microsoft::UI::Xaml::Tests::Xaml::Focus::XYFocus;
using namespace Focus::XYFocusPrivate;

XYFocusCUIElement::XYFocusCUIElement(_In_ KnownTypeIndex index) : m_index(index) {}

bool XYFocusCUIElement::IsFocusable() const
{
    return false;
}

bool XYFocusCUIElement::IsOccluded() const
{
    return false;
}

KnownTypeIndex XYFocusCUIElement::GetTypeIndex() const
{
    return m_index;
}

#pragma region specializations
template<>
bool Focus::XYFocusPrivate::IsValidCandidate<CDependencyObject>(_In_ CDependencyObject* const element)
{
    FocusableXYFocusCUIElement* mockElement = dynamic_cast<FocusableXYFocusCUIElement*>(element);

    if (mockElement)
    {
        return mockElement->IsFocusable();
    }

    return false;
}
#pragma endregion

#pragma region stubs
_Check_return_ HRESULT DirectUI::CValueBoxer::UnwrapWeakRef(
    _In_ const CValue* const value,
    _In_ const CDependencyProperty* dp,
    _Outptr_result_maybenull_ CDependencyObject** unwrappedElement)
{
    *unwrappedElement = value->AsObject();
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetFocusableChildren(
    _Outptr_result_maybenull_ CDOCollection **ppFocusableChildren) {
    return S_OK;
}

_Check_return_ HRESULT CRichTextBlock::GetFocusableChildren(
    _Outptr_result_maybenull_ CDOCollection **ppFocusableChildren) {
    return S_OK;
}

bool CControl::LastInputGamepad() { return true; }

_Check_return_ HRESULT CUIElement::GetGlobalBoundsLogical(_Out_ XRECTF_RB*, _In_ bool, _In_ bool)
{
    return S_OK;
}

_Check_return_ HRESULT CUIElement::IsOccluded(
    _In_ CUIElement *pChildElement,
    _In_ const XRECTF_RB& elementBounds,
    _Out_ bool* isOccluded)
{
    *isOccluded = false;
    FocusableXYFocusCUIElement* mockElement = dynamic_cast<FocusableXYFocusCUIElement*>(pChildElement);

    if (mockElement)
    {
        *isOccluded = mockElement->IsOccluded();
    }

    return S_OK;
}

_Check_return_ CFocusManager* VisualTree::GetFocusManagerForElement(_In_ CDependencyObject* pObject,  LookupOptions options)
{
    return S_OK;
}

CUIElement* VisualTree::GetRootOrIslandForElement(_In_ CDependencyObject *)
{
    return nullptr;
}

CDependencyObject* CFocusManager::GetFocusedElementNoRef() const
{
    return nullptr;
}

#pragma endregion

namespace DirectUI
{
    namespace DXamlServices
    {
        HRESULT TryGetPeer(_In_ CDependencyObject* dobj, _Outptr_result_maybenull_ DependencyObject** peer) { *peer = nullptr;  return E_NOTIMPL; }
    }
}
