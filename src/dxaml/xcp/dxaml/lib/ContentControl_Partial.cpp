// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContentControl.g.h"
#include "ItemContainerGenerator.g.h"
#include "RootScale.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Provides the behavior for the Measure pass of layout. Classes can
// override this method to define their own Measure pass behavior.
IFACEMETHODIMP ContentControl::MeasureOverride(
    // Measurement constraints, a control cannot return a size
    // larger than the constraint.
    _In_ wf::Size availableSize,
    // The desired size of the control.
    _Out_ wf::Size* pReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spIsRecycling;

    BOOLEAN isRecycling = FALSE;
    BOOLEAN isUnsetValue = FALSE;

    // check if this container is actually already placed in the recycle queue
    // if so, stop measure and return previous desired size
    IFC(ReadLocalValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_IsRecycledContainer),
        &spIsRecycling));
    IFC(DependencyPropertyFactory::IsUnsetValue(spIsRecycling.Get(), isUnsetValue));

    if (!isUnsetValue)
    {
        IFC(ctl::do_get_value(isRecycling, spIsRecycling.Get()));
    }

    if (!isRecycling)
    {
        IFC(ContentControlGenerated::MeasureOverride(availableSize, pReturnValue));
    }
    else
    {
        IFC(get_DesiredSize(pReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

// Provides the behavior for the Arrange pass of layout.  Classes
// can override this method to define their own Arrange pass
// behavior.
IFACEMETHODIMP ContentControl::ArrangeOverride(
    // The computed size that is used to arrange the content.
    _In_ wf::Size arrangeSize,
    // The size of the control.
    _Out_ wf::Size* returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spIsRecycling;

    BOOLEAN isRecycling = FALSE;
    VirtualizationInformation* pVirtualizationInformation = nullptr;

    // check if this container is actually already placed in the recycle queue
    // if so, stop arrange and return arrange size
    pVirtualizationInformation = GetVirtualizationInformation();

    if (pVirtualizationInformation)
    {
        isRecycling = !pVirtualizationInformation->GetIsRealized();
    }
    else
    {
        BOOLEAN isUnsetValue = FALSE;

        IFC(ReadLocalValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_IsRecycledContainer),
            &spIsRecycling));
        IFC(DependencyPropertyFactory::IsUnsetValue(spIsRecycling.Get(), isUnsetValue));

        if (!isUnsetValue)
        {
            IFC(ctl::do_get_value(isRecycling, spIsRecycling.Get()));
        }
    }

    if (!isRecycling)
    {
        IFC(ContentControlGenerated::ArrangeOverride(arrangeSize, returnValue));
    }
    else
    {
        *returnValue = arrangeSize;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ContentControl::OnContentChangedCallback(
    _In_ CDependencyObject* nativeTarget,
    _In_ CValue* oldContentValue,
    _In_ CValue* newContentValue,
    _In_opt_ IInspectable* pValueOuter)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<ContentControl> spContentControl;
    ctl::ComPtr<IInspectable> spOldContentValue;
    ctl::ComPtr<IInspectable> spOldContentValueUnwrapped;
    ctl::ComPtr<IInspectable> spNewContentValue;
    ctl::ComPtr<IInspectable> spNewContentValueUnwrapped;
    bool areEqual = false;

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    IFC(spTarget.As(&spContentControl));

    if (pValueOuter)
    {
        spNewContentValue = pValueOuter;
        CValueBoxer::UnwrapExternalObjectReferenceIfPresent(pValueOuter, &spNewContentValueUnwrapped);
    }
    else
    {
        IFC(CValueBoxer::UnboxObjectValue(newContentValue, nullptr, &spNewContentValue));
        spNewContentValueUnwrapped = spNewContentValue;
    }

    IFC(spContentControl->StorePeerPropertyReferenceToObject(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentControl_Content),
        spNewContentValue.Get(),
        false, // bPreservePegNoRef
        &spOldContentValue));

    // Unwrap EOR.
    if (spOldContentValue != nullptr)
    {
        CValueBoxer::UnwrapExternalObjectReferenceIfPresent(spOldContentValue.Get(), &spOldContentValueUnwrapped);
    }

    // At this point we only know that the Content property was set.
    //  Check to see if content actually *changed*.
    IFC(PropertyValue::AreEqual(spOldContentValueUnwrapped.Get(), spNewContentValueUnwrapped.Get(), &areEqual));

    if (!areEqual)
    {
        IFC(spContentControl->OnContentChangedProtected(spOldContentValueUnwrapped.Get(), spNewContentValueUnwrapped.Get()));
    }

Cleanup:
    return hr;
}

// Handle the custom property changed event and call the
// OnPropertyChanged methods.
_Check_return_ HRESULT ContentControl::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOldValue, spNewValue;

    IFC(ContentControlGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ContentControl_ContentTemplate:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
        IFC(OnContentTemplateChanged(spOldValue.Get(), spNewValue.Get()));
        break;
    case KnownPropertyIndex::ContentControl_ContentTemplateSelector:
        IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
        IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
        IFC(OnContentTemplateSelectorChanged(spOldValue.Get(), spNewValue.Get()));
        break;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ContentControl::OnContentChangedImpl(
    _In_ IInspectable* oldContent,
    _In_ IInspectable* newContent)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    ctl::ComPtr<IDataTemplate> spContentTemplate;

    if (newContent)
    {
        IFC(get_ContentTemplate(&spContentTemplate));
        if (!spContentTemplate)
        {
            ctl::ComPtr<IDataTemplateSelector> spContentTemplateSelector;

            IFC(get_ContentTemplateSelector(&spContentTemplateSelector));

            if (spContentTemplateSelector)
            {
                IFC(RefreshSelectedTemplate(spContentTemplateSelector.Get(), newContent, FALSE /* reloadContent */, &spContentTemplate));
            }

            IFC(put_SelectedContentTemplate(spContentTemplate.Get()));
        }
    }

Cleanup:
    RRETURN(S_OK);
}

_Check_return_ HRESULT ContentControl::OnContentTemplateChangedImpl(
    _In_ xaml::IDataTemplate* oldContentTemplate,
    _In_ xaml::IDataTemplate* newContentTemplate)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    ctl::ComPtr<IDataTemplate> spContentTemplate;

    if (!newContentTemplate)
    {
        ctl::ComPtr<IDataTemplateSelector> spContentTemplateSelector;

        IFC(get_ContentTemplateSelector(&spContentTemplateSelector));

        if (spContentTemplateSelector)
        {
            IFC(RefreshSelectedTemplate(spContentTemplateSelector.Get(), nullptr /* pContent */, TRUE/* reloadContent */, &spContentTemplate));
        }

        IFC(put_SelectedContentTemplate(spContentTemplate.Get()));
    }

Cleanup:
    RRETURN(S_OK);
}

_Check_return_
HRESULT
ContentControl::OnContentTemplateChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDataTemplate> spOldContentTemplate = ctl::query_interface_cast<IDataTemplate>(pOldValue);
    ctl::ComPtr<IDataTemplate> spNewContentTemplate = ctl::query_interface_cast<IDataTemplate>(pNewValue);

    IFC(OnContentTemplateChangedProtected(spOldContentTemplate.Get(), spNewContentTemplate.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ContentControl::OnContentTemplateSelectorChangedImpl(
    _In_ xaml_controls::IDataTemplateSelector* oldContentTemplateSelector,
    _In_ xaml_controls::IDataTemplateSelector* newContentTemplateSelector)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    ctl::ComPtr<IDataTemplate> spContentTemplate;

    IFC(get_ContentTemplate(&spContentTemplate));
    if (!spContentTemplate)
    {
        if (newContentTemplateSelector)
        {
            IFC(RefreshSelectedTemplate(newContentTemplateSelector, nullptr /* pContent */, TRUE/* reloadContent */, &spContentTemplate));
        }

        IFC(put_SelectedContentTemplate(spContentTemplate.Get()));
    }

Cleanup:
    // BLUE: 267486 - ContentControl::OnContentTemplateSelectorChangedImpl() swallows hr and always returns S_OK
    // Consider returning the hr here (and see other methods in this file where we are always returning S_OK).
    // We are not making that change now in MP milestone of Windows BLUE because when CS 650111 tried to change this
    // to return the hr as code cleanup, it regressed the System Settings app (BLUE 255197).
    RRETURN(S_OK);
}

_Check_return_
HRESULT
ContentControl::OnContentTemplateSelectorChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDataTemplateSelector> spOldContentTemplateSelector = ctl::query_interface_cast<IDataTemplateSelector>(pOldValue);
    ctl::ComPtr<IDataTemplateSelector> spNewContentTemplateSelector = ctl::query_interface_cast<IDataTemplateSelector>(pNewValue);

    IFC(OnContentTemplateSelectorChangedProtected(spOldContentTemplateSelector.Get(), spNewContentTemplateSelector.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ContentControl::SetContentIsNotLogical()
{
    HRESULT hr = S_OK;
    IFC(CoreImports::ContentControl_SetContentIsNotLogical(static_cast<CContentControl*>(GetHandle())));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
ContentControl::GetPlainText(_Out_ HSTRING* strPlainText)
{
    ctl::ComPtr<IInspectable> spContent;
    *strPlainText = nullptr;

    IFC_RETURN(get_Content(&spContent));

    if (spContent != nullptr)
    {
        IFC_RETURN(FrameworkElement::GetStringFromObject(spContent.Get(), strPlainText));
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
//
//  OnDisconnectVisualChildren
//
//  During a DisconnectVisualChildrenRecursive tree walk, also clear the Content property.
//
//-----------------------------------------------------------------------------

IFACEMETHODIMP
ContentControl::OnDisconnectVisualChildren()
{
    HRESULT hr = S_OK;

    IFC(ClearValue(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentControl_Content)));

    IFC(ContentControlGenerated::OnDisconnectVisualChildren());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ContentControl::get_ContentTemplateRootImpl(_Outptr_ IUIElement** pValue)
{
    HRESULT hr = S_OK;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    CUIElement* pContentTemplateRoot = NULL;

    *pValue = NULL;

    IFC(CoreImports::ContentControl_TryGetContentTemplateRoot(static_cast<CContentControl*>(this->GetHandle()), &pContentTemplateRoot));

    if (pContentTemplateRoot)
    {
        // We got a core object, but it might not have a peer, so we only Try to get it.
        // (This can happen if the peer has been GC'd but not released yet.  In that case, the
        // core object will be going away soon too.  This is necessary because the CContentControl
        // only keeps a core weak reference to the content template root, and core weak references
        // aren't updated as part of the reference tracker walking.)

        ctl::ComPtr<DependencyObject> peer = NULL;
        IFC(pCore->TryGetOrCreatePeer(pContentTemplateRoot, &peer));

        if (peer)
        {
            *pValue = peer.AsOrNull<IUIElement>().Detach();
        }
    }

Cleanup:
    ReleaseInterface(pContentTemplateRoot);
    RRETURN(hr);
}


_Check_return_ HRESULT ContentControl::RefreshSelectedTemplate(
    _In_ xaml_controls::IDataTemplateSelector* pContentTemplateSelector,
    _In_opt_ IInspectable* pContent,
    _In_ BOOLEAN reloadContent,
    _Outptr_ xaml::IDataTemplate** ppContentTemplate)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<xaml::IDataTemplate> spContentTemplate;
    VirtualizationInformation *pVirtualizationInformation = GetVirtualizationInformation();

    if (reloadContent)
    {
        if (pVirtualizationInformation)
        {
            ctl::ComPtr<ICollectionViewGroup> spGroup;
            // content might be stale or null (we might never set it)
            spContent = pVirtualizationInformation->GetItem();

            spGroup = spContent.AsOrNull<ICollectionViewGroup>();
            if (spGroup)
            {
                // a little hacky, we actually want to present the user with the groupdata
                IFC(spGroup->get_Group(&spContent));
            }
        }
        else
        {
            IFC(get_Content(&spContent));
        }
    }

    if (pVirtualizationInformation)
    {
        // We are inside a modern panel, the template could have ben set during virtualization
        spContentTemplate = pVirtualizationInformation->GetSelectedTemplate();
        if (!spContentTemplate)
        {
            // No DataTemplate has been selected, we need to ask the ContentTemplateSelector
            IFC(pContentTemplateSelector->SelectTemplate(reloadContent ? spContent.Get() : pContent, this, &spContentTemplate));
        }
    }
    else
    {
        IFC(pContentTemplateSelector->SelectTemplate(reloadContent ? spContent.Get() : pContent, this, &spContentTemplate));
    }
    IFC(spContentTemplate.MoveTo(ppContentTemplate));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ContentControl::GetGlobalBoundsImpl(_Out_ wf::Rect* pReturnValue)
{
    XRECTF_RB pBounds = { };

    *pReturnValue = { };

    IFC_RETURN(GetHandle()->GetGlobalBounds(&pBounds));

    pReturnValue->X = pBounds.left;
    pReturnValue->Y = pBounds.top;
    pReturnValue->Width = pBounds.right - pBounds.left;
    pReturnValue->Height = pBounds.bottom - pBounds.top;

    return S_OK;
}

_Check_return_ HRESULT ContentControl::GetRasterizationScaleImpl(_Out_ FLOAT* returnValue)
{
    const float scale = RootScale::GetRasterizationScaleForElement(GetHandle());
    *returnValue = scale;
    return S_OK;
}

