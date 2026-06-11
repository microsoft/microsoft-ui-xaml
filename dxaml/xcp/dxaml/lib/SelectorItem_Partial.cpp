// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SelectorItem.g.h"
#include "SelectorItemAutomationPeer.g.h"
#include "Selector.g.h"
#include "ContentPresenter.g.h"
#include "AutomationProperties.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Gets the parent Selector.
_Check_return_ HRESULT SelectorItem::GetParentSelector(
    _Outptr_ Selector** ppParentSelector)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_primitives::ISelector> spParentSelectorAsI;

    IFCPTR(ppParentSelector);
    *ppParentSelector = NULL;

    IFC(m_wrParentSelector.As(&spParentSelectorAsI));
    *ppParentSelector = static_cast<Selector*>(spParentSelectorAsI.Detach());

Cleanup:
    RRETURN(hr);
}

// Sets the parent Selector.
_Check_return_ HRESULT SelectorItem::SetParentSelector(
    _In_opt_ Selector* pParentSelector)
{
    HRESULT hr = S_OK;

    IFC(ctl::AsWeak(ctl::as_iinspectable(pParentSelector), &m_wrParentSelector));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT SelectorItem::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(SelectorItemGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::SelectorItem_IsSelected:
            {
                IFC(OnIsSelectedChanged(!!args.m_pNewValue->AsBool()));
                break;
            }
    }

Cleanup:
    RRETURN(hr);
}

// Called when IsSelected property has changed
_Check_return_
HRESULT
SelectorItem::OnIsSelectedChanged(
    _In_ BOOLEAN isSelected)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Selector> spParentSelector;

    IFC(GetParentSelector(&spParentSelector));
    if (spParentSelector)
    {
        IFC(spParentSelector->NotifyListItemSelected(this, isSelected));
        IFC(spParentSelector->RaiseIsSelectedChangedAutomationEvent(this, isSelected));
    }
    IFC(ChangeVisualState(true));

Cleanup:
    RRETURN(hr);
}

// Called when the Content property changes.
IFACEMETHODIMP SelectorItem::OnContentChanged(
    _In_ IInspectable* oldContent,
    _In_ IInspectable* newContent)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Selector> spSelector = NULL;

    IFC(SelectorItemGenerated::OnContentChanged(oldContent, newContent));

    // Check if this SelectorItem is a data virtualized item that hasn't been
    // realized yet (or was a data virtualized item we just realized now)
    IFC(GetParentSelector(&spSelector));
    if (spSelector)
    {
        IFC(spSelector->ShowPlaceholderIfVirtualized(this));
    }

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the SelectorItem.
_Check_return_ HRESULT SelectorItem::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    VisualStateManagerBatchContext vsmContext(this);

    // Update the VisualStates of parent classes
    IFC(SelectorItemGenerated::ChangeVisualState(bUseTransitions));

    // And batch the changes of the VisualStates for SelectorItem and derived classes
    IFC(ChangeVisualStateWithContext(&vsmContext, bUseTransitions));

Cleanup:
    RRETURN(hr);
}


// Change to the correct visual state for the SelectorItem using
// an existing VisualStateManagerBatchContext
_Check_return_ HRESULT SelectorItem::ChangeVisualStateWithContext(
    _In_ VisualStateManagerBatchContext *pContext,
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIgnored = FALSE;

    // DataVirtualization state group
    if (m_isPlaceholder || m_isUIPlaceholder)
    {
        IFC(pContext->GoToState(bUseTransitions, STR_LEN_PAIR(L"DataPlaceholder"), &bIgnored))
    }
    else
    {
        IFC(pContext->GoToState(bUseTransitions, STR_LEN_PAIR(L"DataAvailable"), &bIgnored))
    }

Cleanup:
    RRETURN(hr);
}

// If this item is unfocused, sets focus on the SelectorItem.
// Otherwise, sets focus to whichever element currently has focus
// (so focusState can be propagated).
_Check_return_ HRESULT SelectorItem::FocusSelfOrChild(
    _In_ xaml::FocusState focusState,
    _In_ BOOLEAN animateIfBringIntoView,
    _Out_ BOOLEAN* pFocused,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    InputActivationBehavior inputActivationBehavior)
{
    HRESULT hr = S_OK;
    BOOLEAN isItemAlreadyFocused = FALSE;
    ctl::ComPtr<DependencyObject> spItemToFocus = NULL;

    IFCPTR(pFocused);

    *pFocused = FALSE;

    IFC(HasFocus(&isItemAlreadyFocused));
    if (isItemAlreadyFocused)
    {
        // Re-focus the currently focused item to propagate focusState (the item might be focused
        // under a different FocusState value).
        IFC(GetFocusedElement(&spItemToFocus));
    }
    else
    {
        spItemToFocus = this;
    }

    if (spItemToFocus)
    {
        bool forceBringIntoView = false;
        IFC(SetFocusedElementWithDirection(spItemToFocus.Get(), focusState, animateIfBringIntoView, pFocused, focusNavigationDirection, forceBringIntoView, inputActivationBehavior));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT SelectorItem::GetValue(
    _In_ const CDependencyProperty* pDP,
    _Outptr_ IInspectable **ppValue)
{
    if (pDP->GetIndex() == KnownPropertyIndex::SelectorItem_IsSelected)
    {
        BOOLEAN isSelected = FALSE;
        CValue boxedValue;

        IFC_RETURN(get_IsSelected(&isSelected));

        boxedValue.SetBool(!!isSelected);

        IFC_RETURN(CValueBoxer::UnboxObjectValue(&boxedValue, pDP->GetPropertyType(), __uuidof(IInspectable), reinterpret_cast<void**>(ppValue)));
    }
    else
    {
        IFC_RETURN(SelectorItemGenerated::GetValue(pDP, ppValue));
    }

    return S_OK;
}

_Check_return_ HRESULT SelectorItem::get_IsSelectedImpl(
    _Out_ BOOLEAN* pValue)
{
    bool isValueSet = false;

    if (m_wrParentSelector)
    {
        ctl::ComPtr<Selector> spSelector = nullptr;

        IFC_RETURN(GetParentSelector(&spSelector));
        if (spSelector)
        {
            IFC_RETURN(spSelector->DataSourceGetIsSelected(this, pValue, &isValueSet));
        }
    }

    if (!isValueSet)
    {
        IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::SelectorItem_IsSelected, pValue));
    }

    return S_OK;
}

_Check_return_ HRESULT SelectorItem::put_IsSelectedImpl(
    _In_ BOOLEAN value)
{
    // SetValue triggers OnPropertyChanged2
    // OnPropertyChanged2 triggers OnIsSelectedChanged
    // OnIsSelectedChanged triggers NotifyListIsItemSelected
    // NotifyListIsItemSelected triggers DataSource->SelectRange if SelectionInfo interface is implemented
    return SetValueByKnownIndex(KnownPropertyIndex::SelectorItem_IsSelected, value);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a plain text string to provide a default AutomationProperties.Name
//      in the absence of an explicitly defined one
//
//---------------------------------------------------------------------------

_Check_return_ HRESULT SelectorItem::GetPlainText(_Out_ HSTRING* strPlainText)
{
    *strPlainText = nullptr;
    int length = 0;
    ctl::ComPtr<xaml::IUIElement> contentTemplateRootAsIUIE;

    IFC_RETURN(get_ContentTemplateRoot(&contentTemplateRootAsIUIE));

    if (contentTemplateRootAsIUIE)
    {
        // we have the first child of the content. Check whether it has an automation name

        IFC_RETURN(AutomationProperties::GetNameStatic(contentTemplateRootAsIUIE.Cast<UIElement>(), strPlainText));

        if (*strPlainText != nullptr)
        {
            length = ::WindowsGetStringLen(*strPlainText);
        }

        // fallback: use getplain text on it
        if (length == 0)
        {
            ctl::ComPtr<xaml::IFrameworkElement> contentTemplateRootAsIFE =
                contentTemplateRootAsIUIE.AsOrNull<xaml::IFrameworkElement>();

            *strPlainText = nullptr;

            if (contentTemplateRootAsIFE)
            {
                IFC_RETURN(contentTemplateRootAsIFE.Cast<FrameworkElement>()->GetPlainText(strPlainText));
            }

            if (*strPlainText != nullptr)
            {
                length = ::WindowsGetStringLen(*strPlainText);
            }
        }


        // fallback, use GetPlainText on the contentpresenter, who has some special logic to account for old templates
        if (length == 0)
        {
            ctl::ComPtr<xaml::IFrameworkElement> contentTemplateRootAsIFE = contentTemplateRootAsIUIE.AsOrNull<xaml::IFrameworkElement>();

            *strPlainText = nullptr;

            if (contentTemplateRootAsIFE)
            {
                auto pParent = contentTemplateRootAsIFE.Cast<FrameworkElement>()->GetHandle()->GetParent();
                if (auto cp = do_pointer_cast<CContentPresenter>(pParent))
                {
                    IFC_RETURN(cp->GetTextBlockText(strPlainText));

                    if (*strPlainText != nullptr)
                    {
                        length = ::WindowsGetStringLen(*strPlainText);
                    }
                }
            }
        }
    }

    // Fallback is to call the ancestor's GetPlainText. SelectorItemGenerated doesn't have a GetPlainText
    // implementation, so it would find something in the parent. As of this writing, it should be the
    // ContentControl.
    if (length == 0)
    {
        IFC_RETURN(SelectorItemGenerated::GetPlainText(strPlainText));
    }

    return S_OK;
}
