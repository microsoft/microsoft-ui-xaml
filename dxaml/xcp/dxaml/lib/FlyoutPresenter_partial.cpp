// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FlyoutPresenter.g.h"
#include "FlyoutPresenterAutomationPeer.g.h"
#include "ScrollViewer.g.h"
#include "Flyout.g.h"
#include "Popup.g.h"
#include "AutomationProperties.h"
#include "ElevationHelper.h"
#include "VisualTreeHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

FlyoutPresenter::FlyoutPresenter()
{
}

FlyoutPresenter::~FlyoutPresenter()
{
    m_tpInnerScrollViewer.Clear();
}

HRESULT FlyoutPresenter::put_Flyout(_In_ xaml_primitives::IFlyoutBase * pFlyoutBase)
{
    ASSERT(!m_wrFlyout);

    IFC_RETURN(ctl::AsWeak(pFlyoutBase, &m_wrFlyout));
    return S_OK;
}

// Create FlyoutPresenterAutomationPeer to represent the FlyoutPresenter.
IFACEMETHODIMP FlyoutPresenter::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IFlyoutPresenterAutomationPeer> spFlyoutPresenterAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IFlyoutPresenterAutomationPeerFactory> spFlyoutPresenterAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    IFCPTR(ppAutomationPeer);
    *ppAutomationPeer = NULL;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::FlyoutPresenterAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spFlyoutPresenterAPFactory));

    IFC(spFlyoutPresenterAPFactory.Cast<FlyoutPresenterAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spFlyoutPresenterAutomationPeer));
    IFC(spFlyoutPresenterAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
FlyoutPresenter::OnApplyTemplate()
{
    ctl::ComPtr<IScrollViewer> spInnerScrollViewer;

    CleanupTemplateParts();

    IFC_RETURN(FlyoutPresenterGenerated::OnApplyTemplate());

    IFC_RETURN(GetTemplatePart<IScrollViewer>(STR_LEN_PAIR(L"ScrollViewer"), spInnerScrollViewer.ReleaseAndGetAddressOf()));
    //
    // Block calling SetPtrValue() temporary by failing DRT ImmersiveColorMediumCppFlyout.
    //
    //IFC(SetPtrValue(m_tpInnerScrollViewer, spInnerScrollViewer));
    //if (m_tpInnerScrollViewer)
    //{
    //    m_tpInnerScrollViewer.Cast<ScrollViewer>()->m_isFocusableOnFlyoutScrollViewer = TRUE;
    //}
    //
    // The below code must be removed when SetPtrValue is called.
    //
    if (spInnerScrollViewer)
    {
        spInnerScrollViewer.Cast<ScrollViewer>()->m_isFocusableOnFlyoutScrollViewer = TRUE;
    }

    // Apply a shadow
    // Deliverable 19819460: Allow LTEs to target Popup.Child when Popup is windowed
    // ThemeTransition can't be applied to child of a windowed popup, we are targeting grandchild in FlyoutBase_partial.
    // Shadows need to be on the same element to work right, so we are applying shadows to grandchild too.
    ctl::ComPtr<IDependencyObject> spChild;
    IFC_RETURN(VisualTreeHelper::GetChildStatic(this, 0, &spChild));
    auto spChildAsUIE = spChild.AsOrNull<IUIElement>();

    BOOLEAN isDefaultShadowEnabled;
    IFC_RETURN(get_IsDefaultShadowEnabled(&isDefaultShadowEnabled));
    if (isDefaultShadowEnabled && spChildAsUIE)
    {
        IFC_RETURN(ApplyElevationEffect(spChildAsUIE.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
FlyoutPresenter::GetPlainText(_Out_ HSTRING* strPlainText)
{
    ctl::ComPtr<IDependencyObject> ownerFlyout;
    HSTRING automationName = nullptr;

    IFC_RETURN(m_wrFlyout.As(&ownerFlyout));

    if (ownerFlyout)
    {
        // If an automation name is set on the owner flyout, we'll use that as our plain text.
        // Otherwise, we'll report the default plain text.
        IFC_RETURN(DirectUI::AutomationProperties::GetNameStatic(ownerFlyout.Get(), &automationName));
    }

    if (automationName != nullptr)
    {
        *strPlainText = automationName;
    }
    else
    {
        // If we have no title, we'll fall back to the default implementation,
        // which retrieves our content as plain text (e.g., if our content is a string,
        // it returns that; if our content is a TextBlock, it returns its Text value, etc.)
        IFC_RETURN(FlyoutPresenterGenerated::GetPlainText(strPlainText));

        // If we get the plain text from the content, then we want to truncate it,
        // in case the resulting automation name is very long.
        IFC_RETURN(Popup::TruncateAutomationName(strPlainText));
    }

    return S_OK;
}

void
FlyoutPresenter::CleanupTemplateParts()
{
    if (m_tpInnerScrollViewer)
    {
        m_tpInnerScrollViewer.Cast<ScrollViewer>()->m_isFocusableOnFlyoutScrollViewer = FALSE;
    }

    m_tpInnerScrollViewer.Clear();
}

_Check_return_
HRESULT FlyoutPresenter::GetOwnerFlyout(
    _Outptr_ FlyoutBase** ppOwnerFlyout)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFlyoutBase> spOwner;

    *ppOwnerFlyout = NULL;

    IFC(m_wrFlyout.As(&spOwner));
    *ppOwnerFlyout = static_cast<FlyoutBase*>(spOwner.Detach());

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT FlyoutPresenter::GetOwnerName(_Out_ HSTRING* pName)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<FlyoutBase> spOwnerFlyout;

    IFC(GetOwnerFlyout(&spOwnerFlyout));
    if (spOwnerFlyout != nullptr)
    {
        ctl::ComPtr<IInspectable> spName;

        IFC(spOwnerFlyout->GetValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::DependencyObject_Name),
            &spName));

        IFC(ctl::do_get_value(*pName, spName.Get()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FlyoutPresenter::GetTargetIfOpenedAsTransientStatic(_In_ CDependencyObject* nativeControl, _Outptr_ CDependencyObject** nativeTarget)
{
    ctl::ComPtr<DependencyObject> flyoutPresenterAsDO;
    ctl::ComPtr<FlyoutPresenter> flyoutPresenter;
    ctl::ComPtr<DependencyObject> target;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nativeControl, &flyoutPresenterAsDO));
    IFC_RETURN(flyoutPresenterAsDO.As(&flyoutPresenter));

    IFC_RETURN(flyoutPresenter->GetTargetIfOpenedAsTransient(&target));

    *nativeTarget = target ? target->GetHandle() : nullptr;
    return S_OK;
}

_Check_return_ HRESULT FlyoutPresenter::GetTargetIfOpenedAsTransient(_Outptr_ DependencyObject** target)
{
    ctl::ComPtr<FlyoutBase> ownerFlyout;
    IFC_RETURN(GetOwnerFlyout(&ownerFlyout));

    if (ownerFlyout == nullptr)
    {
        return S_OK;
    }

    xaml_primitives::FlyoutShowMode showMode;
    IFC_RETURN(ownerFlyout->get_ShowMode(&showMode));

    if (showMode == xaml_primitives::FlyoutShowMode_Auto ||
        showMode == xaml_primitives::FlyoutShowMode_Standard)
    {
        return S_OK;
    }

    ctl::ComPtr<xaml::IFrameworkElement> targetLocal;
    IFC_RETURN(ownerFlyout->get_Target(&targetLocal));

    if (!targetLocal)
    {
        return S_OK;
    }

    ctl::ComPtr<DependencyObject> targetAsDO;
    IFC_RETURN(targetLocal.As(&targetAsDO));

    *target = targetAsDO.Detach();
    return S_OK;
}