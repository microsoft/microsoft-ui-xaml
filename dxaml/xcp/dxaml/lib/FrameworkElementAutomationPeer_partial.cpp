// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FrameworkElementAutomationPeer.g.h"
#include "ToolTipService.g.h"
#include "Popup.g.h"
#include "AutomationAnnotation.g.h"
#include "AutomationPeerAnnotation.g.h"
#include "ScrollItemAdapter.g.h"
#include "AutomationProperties.h"
#include "AccessKeyStringBuilder.h"
#include <XamlOneCoreTransforms.h>
#include "RootScale.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT FrameworkElementAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml::IFrameworkElement* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IFrameworkElementAutomationPeer** ppInstance)
{
    ctl::ComPtr<xaml_automation_peers::IFrameworkElementAutomationPeer> spInstance;
    ctl::ComPtr<IInspectable> spInner;
    ctl::ComPtr<xaml::IUIElement> spOwnerAsUIE;

    IFCPTR_RETURN(ppInstance);
    IFCEXPECT_RETURN(pOuter == nullptr || ppInner != nullptr);
    IFCPTR_RETURN(owner);

    IFC_RETURN(ctl::do_query_interface(spOwnerAsUIE, owner));

    IFC_RETURN(ActivateInstance(pOuter,
        static_cast<FrameworkElement*>(owner)->GetHandle(),
        &spInner));
    IFC_RETURN(spInner.As<xaml_automation_peers::IFrameworkElementAutomationPeer>(&spInstance));
    IFC_RETURN(spInstance.Cast<FrameworkElementAutomationPeer>()->put_Owner(spOwnerAsUIE.Get()));

    if (ppInner)
    {
        *ppInner = spInner.Detach();
    }

    *ppInstance = spInstance.Detach();

    return S_OK;
}

// Initializes a new instance of the FrameworkElementAutomationPeer class.
FrameworkElementAutomationPeer::FrameworkElementAutomationPeer()
{
    // Default ControlType is Custom as this will either get used by custom controls
    // or Standard controls that will define/set their own ControlType.
    m_ControlType = xaml_automation_peers::AutomationControlType::AutomationControlType_Custom;
    VERIFYHR(m_ClassName.Set(L""));
}

// Deconstructor
FrameworkElementAutomationPeer::~FrameworkElementAutomationPeer()
{
}

// Set the owner of AutomationPeer
_Check_return_ HRESULT
FrameworkElementAutomationPeer::put_Owner(
    xaml::IUIElement* pOwner)
{
    return ctl::AsWeak(pOwner, &m_wpOwner);
}

// Retrieve a AutomationPeer owner
IFACEMETHODIMP FrameworkElementAutomationPeer::get_Owner(
    _Outptr_ xaml::IUIElement** pValue)
{
    auto spOwner = m_wpOwner.AsOrNull<IUIElement>();

    IFCPTR_RETURN(pValue);
    *pValue = nullptr;

    if (!spOwner)
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    *pValue = spOwner.Detach();

    return S_OK;
}

// Notify Corresponding core object about managed owner(UI) being dead.
void FrameworkElementAutomationPeer::NotifyManagedUIElementIsDead()
{
    m_wpOwner.Reset();
    AutomationPeer::NotifyManagedUIElementIsDead();
}


IFACEMETHODIMP FrameworkElementAutomationPeer::GetAcceleratorKeyCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));
    IFC_RETURN(AutomationProperties::GetAcceleratorKeyStatic(spOwner.Cast<UIElement>(), returnValue));
    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetAccessKeyCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<IInspectable> value;
    ctl::ComPtr<IUIElement> spOwner;
    xaml::IDependencyProperty* prop = nullptr;
    BOOLEAN isUnset = FALSE;

    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    //Check to see if the value is unset
    MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_AccessKey, &prop);
    IFC_RETURN(spOwner.Cast<UIElement>()->ReadLocalValue(prop, &value));
    IFC_RETURN(DependencyPropertyFactory::IsUnsetValue(value.Get(), isUnset));

    //If value is unset, then fallback to the AccessKey property on Framework Element
    if (isUnset)
    {
      ctl::ComPtr<DependencyObject> spOwnerAsDO(spOwner.Cast<FrameworkElement>());
      IFC_RETURN(AccessKeyStringBuilder::GetAccessKeyMessageFromElement(spOwnerAsDO, returnValue));
    }
    else
    {
        //Find the value normally
        IFC_RETURN(AutomationProperties::GetAccessKeyStatic(spOwner.Cast<UIElement>(), returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetAutomationIdCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetAutomationIdStatic(spOwner.Cast<UIElement>(), returnValue));

    XUINT32 length = WindowsGetStringLen(*returnValue);
    if (length == 0)
    {
        xstring_ptr strAutomationId;
        xruntime_string_ptr strAutomationIdRuntime;
        IFC_RETURN(static_cast<CAutomationPeer*>(GetHandle())->GetAutomationIdHelper(&strAutomationId));

        IFC_RETURN(strAutomationId.Promote(&strAutomationIdRuntime));
        *returnValue = strAutomationIdRuntime.DetachHSTRING();
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetAutomationControlType(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    ctl::ComPtr<IUIElement> owner;
    IFC_RETURN(get_Owner(&owner));

    // If AutomationProperties.AutomationControlType is set, we'll return that value.
    // Otherwise, we'll fall back to the GetAutomationControlTypeCore override.
    if (!owner.Cast<UIElement>()->GetHandle()->IsPropertyDefault(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_AutomationControlType)))
    {
        IFC_RETURN(AutomationProperties::GetAutomationControlTypeStatic(owner.Cast<UIElement>(), returnValue));
    }
    else
    {
        IFC_RETURN(FrameworkElementAutomationPeerGenerated::GetAutomationControlType(returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = m_ControlType;
    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetBoundingRectangleCore(_Out_ wf::Rect* returnValue)
{
    XRECTF rect = {};
    BOOLEAN isOffscreen = true;
    ctl::ComPtr<IUIElement> spOwner;

    const bool isOneCoreTransforms = XamlOneCoreTransforms::IsEnabled();

    // In OneCoreTransforms mode, we ignore the clip on the all CScrollContentPresenters for the magnifier. This is
    // needed because Santorini's Magnifier places a RenderTransform on itself to do the magnification, which will
    // push parts of the shell (which lives underneath the Magnifier in the tree) beyond the bounds of the window.
    // Those parts still need to report bounds in order to be accessed by UIA and be scrolled back into view by the
    // shell.
    //
    // Note that ignoring the root CScrollContentPresenter clip alone does not guarantee non-zero bounds to be
    // returned. The window size could have been given to layout and could have caused layout clips to be applied
    // in the tree. This works for Magnifier because the Magnifier control uses only a RenderTransform for
    // magnification, which does not affect layout at all.
    //
    // Also note that we still respect the root CScrollContentPresenter clip for IsOffscreen (see IsOffscreenCore).
    // GetBoundingRectangle is not required to clip the bounds to the window, but IsOffscreen needs to remain accurate.
    // https://docs.microsoft.com/en-us/windows/desktop/api/uiautomationcore/nf-uiautomationcore-irawelementproviderfragment-get_boundingrectangle
    const bool ignoreClippingOnScrollContentPresenters = isOneCoreTransforms;

    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(static_cast<CAutomationPeer*>(GetHandle())->IsOffscreenHelper(ignoreClippingOnScrollContentPresenters, &isOffscreen));

    if (!isOffscreen)
    {
        XRECTF_RB bounds = {};

        IFC_RETURN(static_cast<CUIElement*>(spOwner.Cast<UIElement>()->GetHandle())->GetGlobalBoundsWithOptions(
            &bounds,
            false /* ignoreClipping */,
            ignoreClippingOnScrollContentPresenters,
            false /* useTargetInformation */));

        rect = ToXRectF(bounds);
    }

    if (isOneCoreTransforms)
    {
        // In OneCoreTransforms mode, GetGlobalBounds returns logical pixels so we must convert to RasterizedClient
        const float scale = RootScale::GetRasterizationScaleForElement(static_cast<CAutomationPeer*>(GetHandle())->GetRootNoRef());
        const auto logicalRect = rect;
        const auto physicalRect = logicalRect * scale;
        rect = physicalRect;
    }

    returnValue->X = rect.X;
    returnValue->Y = rect.Y;
    returnValue->Width = rect.Width;
    returnValue->Height = rect.Height;

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    ctl::ComPtr<wfc::IVector<xaml_automation_peers::AutomationPeer*>> spAPChildren;
    ctl::ComPtr<IUIElement> spOwner;

    *returnValue = nullptr;

    IFC_RETURN(ctl::ComObject<TrackerCollection<xaml_automation_peers::AutomationPeer*>>::CreateInstance(spAPChildren.ReleaseAndGetAddressOf()));

    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(GetAutomationPeerChildren(spOwner.Get(), spAPChildren.Get()));

    *returnValue = spAPChildren.Detach();

    return S_OK;
}

_Check_return_ HRESULT
FrameworkElementAutomationPeer::GetAutomationPeerChildren(
    _In_ xaml::IUIElement* pElement,
    _In_ wfc::IVector<xaml_automation_peers::AutomationPeer*>* pAPChildren)
{
    INT childCount = 0;

    IFCPTR_RETURN(pAPChildren);
    IFCPTR_RETURN(pElement);

    IFC_RETURN(static_cast<UIElement*>(pElement)->GetChildrenCount(&childCount));
    if (childCount)
    {
        BOOLEAN reverseOrder = static_cast<UIElement*>(pElement)->AreAutomationPeerChildrenReversed();
        for (INT nIndex = reverseOrder ? childCount - 1 : 0; reverseOrder ? nIndex >= 0 : nIndex < childCount; nIndex += reverseOrder ? -1 : 1)
        {
            ctl::ComPtr<IDependencyObject> spChildAsDo;
            ctl::ComPtr<IUIElement> spChildAsUie;
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spChildAP;
            BOOLEAN bChildIsAcceptable = false;

            IFC_RETURN(static_cast<UIElement*>(pElement)->GetChild(nIndex, &spChildAsDo));
            spChildAsUie = spChildAsDo.AsOrNull<IUIElement>();
            IFC_RETURN(ChildIsAcceptable(spChildAsUie.Cast<UIElement>(), &bChildIsAcceptable));
            if (bChildIsAcceptable)
            {
                IFC_RETURN(spChildAsUie.Cast<UIElement>()->GetOrCreateAutomationPeer(&spChildAP));

                if (spChildAP)
                {
                    IFC_RETURN(pAPChildren->Append(spChildAP.Get()));
                }
                else
                {
                    IFC_RETURN(GetAutomationPeerChildren(spChildAsUie.Cast<UIElement>(), pAPChildren));
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetAutomationPeersForChildrenOfElementImpl(
    _In_ xaml::IUIElement* pElement,
    _Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    *ppReturnValue = nullptr;

    ctl::ComPtr<wfc::IVector<xaml_automation_peers::AutomationPeer*>> spAPChildren;
    IFC_RETURN(ctl::ComObject<TrackerCollection<xaml_automation_peers::AutomationPeer*>>::CreateInstance(spAPChildren.ReleaseAndGetAddressOf()));

    IFC_RETURN(GetAutomationPeerChildren(pElement, spAPChildren.Get()));

    *ppReturnValue = spAPChildren.Detach();

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    return m_ClassName.CopyTo(returnValue);
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetClickablePointCore(_Out_ wf::Point* returnValue)
{
    XPOINTF point{};
    ctl::ComPtr<IUIElement> spOwner;

    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(static_cast<CUIElement*>(spOwner.Cast<UIElement>()->GetHandle())->GetClickablePointRasterizedClient(&point));

    returnValue->X = point.x;
    returnValue->Y = point.y;

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetHelpTextCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetHelpTextStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetItemStatusCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetItemStatusStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetItemTypeCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetItemTypeStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetLabeledByCore(
    _Outptr_ xaml_automation_peers::IAutomationPeer**  returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    ctl::ComPtr<IUIElement> spLabeledByUie;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetLabeledByStatic(spOwner.Cast<UIElement>(), &spLabeledByUie));
    if (spLabeledByUie)
    {
        IFC_RETURN(spLabeledByUie.Cast<UIElement>()->GetOrCreateAutomationPeer(returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetLocalizedControlType(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<IUIElement> owner;
    IFC_RETURN(get_Owner(&owner));

    // If AutomationProperties.LocalizedControlType is set, we'll return that value.
    // Otherwise, we'll fall back to the GetLocalizedControlTypeCore override.
    if (!owner.Cast<UIElement>()->GetHandle()->IsPropertyDefault(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_LocalizedControlType)))
    {
        IFC_RETURN(AutomationProperties::GetLocalizedControlTypeStatic(owner.Cast<UIElement>(), returnValue));
    }
    else
    {
        IFC_RETURN(FrameworkElementAutomationPeerGenerated::GetLocalizedControlType(returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetLocalizedControlTypeCore(_Out_ HSTRING* returnValue)
{
    // If it's a standard "Localized Control Type" is required we fetch the data
    // from Core layer for the stadard control types.
    if (m_LocalizedControlType.Get() == nullptr)
    {
        ctl::ComPtr<IUIElement> spOwner;
        XUINT32 length = 0;
        *returnValue = nullptr;

        IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

        IFC_RETURN(AutomationProperties::GetLocalizedControlTypeStatic(spOwner.Cast<UIElement>(), returnValue));
        length = WindowsGetStringLen(*returnValue);

        if (length == 0)
        {
            IFC_RETURN(FrameworkElementAutomationPeerGenerated::GetLocalizedControlTypeCore(returnValue));
        }
    }
    else
    {
        IFC_RETURN(m_LocalizedControlType.CopyTo(returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::IsRequiredForFormCore(_Out_ BOOLEAN* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    *returnValue = FALSE;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));
    IFC_RETURN(AutomationProperties::GetIsRequiredForFormStatic(spOwner.Cast<UIElement>(), returnValue));
    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    XUINT32 length = 0;

    *returnValue = nullptr;

    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetNameStatic(spOwner.Cast<UIElement>(), returnValue));

    length = WindowsGetStringLen(*returnValue);
    if (length == 0)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spLabeledByAP;
        IFC_RETURN(GetLabeledBy(&spLabeledByAP));

        if (spLabeledByAP)
        {
            IFC_RETURN(spLabeledByAP->GetName(returnValue));
            length = WindowsGetStringLen(*returnValue);
        }

        if (length == 0)
        {
            ctl::ComPtr<IFrameworkElement> spFe;
            spFe = spOwner.AsOrNull<IFrameworkElement>();
            if (spFe)
            {
                IFC_RETURN(spFe.Cast<FrameworkElement>()->GetPlainText(returnValue));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetDefaultPattern(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue)
{
    *returnValue = nullptr;

    if (patternInterface == xaml_automation_peers::PatternInterface_ScrollItem)
    {
        if (!m_spScrollItemAdapter)
        {
            ctl::ComPtr<ScrollItemAdapter> spScrollItemAdapter;

            IFC_RETURN(ctl::make<ScrollItemAdapter>(&spScrollItemAdapter));
            IFCPTR_RETURN(spScrollItemAdapter.Get());

            m_spScrollItemAdapter = spScrollItemAdapter;
            IFC_RETURN(m_spScrollItemAdapter->put_Owner(this));
        }
        IFC_RETURN(m_spScrollItemAdapter.CopyTo(returnValue));
    }
    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::GetLiveSettingCore(_Out_ xaml_automation_peers::AutomationLiveSetting* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetLiveSettingStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetPositionInSetCoreImpl(_Out_ INT* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetPositionInSetStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetSizeOfSetCoreImpl(_Out_ INT* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetSizeOfSetStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetLevelCoreImpl(_Out_ INT* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetLevelStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetLandmarkTypeCoreImpl(_Out_ xaml_automation_peers::AutomationLandmarkType* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetLandmarkTypeStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetLocalizedLandmarkTypeCoreImpl(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetLocalizedLandmarkTypeStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::IsContentElementCore(_Out_ BOOLEAN* returnValue)
{
    xaml_automation_peers::AccessibilityView accessibilityView = xaml_automation_peers::AccessibilityView_Content;
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetAccessibilityViewStatic(spOwner.Cast<UIElement>(), &accessibilityView));

    *returnValue = accessibilityView == xaml_automation_peers::AccessibilityView_Content;

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::IsControlElementCore(_Out_ BOOLEAN* returnValue)
{
    xaml_automation_peers::AccessibilityView accessibilityView = xaml_automation_peers::AccessibilityView_Content;
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetAccessibilityViewStatic(spOwner.Cast<UIElement>(), &accessibilityView));

    // Content view is subset of Control View so anything which is in Content View will have ControlElement as True
    *returnValue = (accessibilityView == xaml_automation_peers::AccessibilityView_Control) || (accessibilityView == xaml_automation_peers::AccessibilityView_Content);

    return S_OK;
}

IFACEMETHODIMP FrameworkElementAutomationPeer::IsEnabledCore(_Out_ BOOLEAN* returnValue)
{
    return (static_cast<CAutomationPeer*>(GetHandle())->IsEnabledHelper(returnValue));
}

IFACEMETHODIMP FrameworkElementAutomationPeer::IsKeyboardFocusableCore(_Out_ BOOLEAN* returnValue)
{
    return (static_cast<CAutomationPeer*>(GetHandle())->IsKeyboardFocusableHelper(returnValue));
}

IFACEMETHODIMP FrameworkElementAutomationPeer::IsOffscreenCore(_Out_ BOOLEAN* returnValue)
{
    return (static_cast<CAutomationPeer*>(GetHandle())->IsOffscreenHelper(false /* ignoreClippingOnScrollContentPresenters */, returnValue));
}

IFACEMETHODIMP FrameworkElementAutomationPeer::SetFocusCore()
{
    IFC_RETURN(static_cast<CAutomationPeer*>(GetHandle())->SetFocusHelper());

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::ShowContextMenuCoreImpl()
{
    return (static_cast<CAutomationPeer*>(GetHandle())->ShowContextMenuHelper());
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::IsPeripheralCoreImpl(_Out_ BOOLEAN* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetIsPeripheralStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::IsDataValidForFormCoreImpl(_Out_ BOOLEAN* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetIsDataValidForFormStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetFullDescriptionCoreImpl(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetFullDescriptionStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetDescribedByCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    IFC_RETURN(GetAutomationPeerCollection(UIAXcp::APDescribedByProperty, returnValue));
    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetFlowsToCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    IFC_RETURN(GetAutomationPeerCollection(UIAXcp::APFlowsToProperty, returnValue));
    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetFlowsFromCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    IFC_RETURN(GetAutomationPeerCollection(UIAXcp::APFlowsFromProperty, returnValue));
    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetHeadingLevelCoreImpl(_Out_ xaml_automation_peers::AutomationHeadingLevel* returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetHeadingLevelStatic(spOwner.Cast<UIElement>(), returnValue));

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::IsDialogCoreImpl(_Out_ BOOLEAN* returnValue)
{
    ctl::ComPtr<IUIElement> owner;
    IFC_RETURN(get_Owner(owner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetIsDialogStatic(owner.Cast<UIElement>(), returnValue));

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetControlledPeersCoreImpl(_Outptr_ wfc::IVectorView<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spElements;
    ctl::ComPtr<IUIElement> spOwner;
    unsigned size = 0;

    *returnValue = nullptr;

    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    // AutomationProperties deals with UIElements but the peer world wants to work in AutomationPeers.
    // Here we do the translation.

    IFC_RETURN(DirectUI::AutomationProperties::GetControlledPeersStatic(spOwner.Cast<UIElement>(), &spElements));
    if (spElements)
    {
        IFC_RETURN(spElements->get_Size(&size));
    }

    if (size > 0)
    {
        ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spPeers;
        IFC_RETURN(ctl::make(&spPeers));

        for (unsigned i = 0; i < size; ++i)
        {
            ctl::ComPtr<xaml::IUIElement> spUie;
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;

            IFC_RETURN(spElements->GetAt(i, &spUie));
            IFC_RETURN(spUie.Cast<UIElement>()->GetOrCreateAutomationPeer(&spAP));
            IFC_RETURN(spPeers->Append(spAP.Get()));
        }
        IFC_RETURN(spPeers->GetView(returnValue));
    }

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetAnnotationsCoreImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeerAnnotation*>** returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    ctl::ComPtr<wfc::IVector<xaml_automation::AutomationAnnotation*>> spUIElementAnnotations;
    ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeerAnnotation*>> spAutomationPeerAnnotations;
    unsigned size = 0;

    *returnValue = nullptr;

    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    // AutomationProperties deals with UIElements but the peer world wants to work in AutomationPeers.
    // Here we do the translation.

    IFC_RETURN(AutomationProperties::GetAnnotationsStatic(spOwner.Cast<UIElement>(), spUIElementAnnotations.GetAddressOf()));

    if (spUIElementAnnotations)
    {
        IFC_RETURN(spUIElementAnnotations->get_Size(&size));
    }
    if (size == 0)
    {
        return S_OK;
    }

    IFC_RETURN(ctl::make(&spAutomationPeerAnnotations));
    for (unsigned i = 0; i < size; ++i)
    {
        ctl::ComPtr<xaml_automation::IAutomationAnnotation> spUieAnnotation;
        ctl::ComPtr<AutomationPeerAnnotation> spApAnnotation;
        ctl::ComPtr<xaml::IUIElement> spUie;
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;
        xaml_automation::AnnotationType type;

        IFC_RETURN(spUIElementAnnotations->GetAt(i, &spUieAnnotation));
        IFC_RETURN(spUieAnnotation.Cast<AutomationAnnotation>()->get_Type(&type));
        IFC_RETURN(spUieAnnotation.Cast<AutomationAnnotation>()->get_Element(&spUie));
        if (spUie.Get() != nullptr)
        {
            IFC_RETURN(spUie.Cast<UIElement>()->GetOrCreateAutomationPeer(&spAP));
        }
        IFC_RETURN(ctl::make(&spApAnnotation));
        IFC_RETURN(spApAnnotation->put_Type(type));
        IFC_RETURN(spApAnnotation->put_Peer(spAP.Get()));
        IFC_RETURN(spAutomationPeerAnnotations->Append(static_cast<xaml_automation_peers::IAutomationPeerAnnotation*>(spApAnnotation.Get())));
    }
    *returnValue = spAutomationPeerAnnotations.Detach();

    return S_OK;
}

void FrameworkElementAutomationPeer::SetControlType(xaml_automation_peers::AutomationControlType controlType)
{
    m_ControlType = controlType;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::SetLocalizedControlType(_In_ HSTRING localizedControlType)
{
    return m_LocalizedControlType.Set(localizedControlType);
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::SetClassName(_In_ HSTRING className)
{
    return m_ClassName.Set(className);
}

_Check_return_ HRESULT FrameworkElementAutomationPeerFactory::FromElementImpl(
    _In_ xaml::IUIElement* pElement,
    _Outptr_ xaml_automation_peers::IAutomationPeer** ppReturnValue)
{
    return ((static_cast<UIElement*>(pElement))->GetOrCreateAutomationPeer(ppReturnValue));
}

_Check_return_ HRESULT FrameworkElementAutomationPeerFactory::CreatePeerForElementImpl(
    _In_ xaml::IUIElement* pElement,
    _Outptr_ xaml_automation_peers::IAutomationPeer** ppReturnValue)
{
    return ((static_cast<UIElement*>(pElement))->GetOrCreateAutomationPeer(ppReturnValue));
}

// <summary>
// Virtual helper method which provide ability for any specific Automation peers
// do not allows including Automation peer of child elements' in to the Automation
// peer tree.
// </summary>
// We don't accept nonUI or null elements by default
// <param name="child">
// Child element to be decided to include it to
// Automation peer's tree
// </param>
// <returns>true if the child element is acceptable</returns>
_Check_return_ HRESULT FrameworkElementAutomationPeer::ChildIsAcceptable(
    _In_ xaml::IUIElement* pElement,
    _Out_ BOOLEAN* bchildIsAcceptable)
{
    xaml::Visibility value = xaml::Visibility_Visible;
    BOOLEAN bIsPopupOpen = TRUE;

    *bchildIsAcceptable = pElement != nullptr;
    if (pElement)
    {
        if (ctl::is<xaml_primitives::IPopup>(pElement))
        {
            IFC_RETURN((static_cast<Popup*>(pElement))->get_IsOpen(&bIsPopupOpen));
        }
        IFC_RETURN(pElement->get_Visibility(&value));

        // this condition checks that if Control is visible and if it's popup then it must be open
        *bchildIsAcceptable = bIsPopupOpen && value == xaml::Visibility_Visible;
    }

    return S_OK;
}

_Check_return_ HRESULT FrameworkElementAutomationPeer::GetAutomationPeerCollection(
    _In_ UIAXcp::APAutomationProperties eProperty,
    _Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> spElements;
    ctl::ComPtr<IUIElement> spOwner;
    unsigned size = 0;

    *returnValue = nullptr;

    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    switch (eProperty)
    {
    case UIAXcp::APDescribedByProperty:
        IFC_RETURN(DirectUI::AutomationProperties::GetDescribedByStatic(spOwner.Cast<UIElement>(), &spElements));
        break;
    case UIAXcp::APFlowsToProperty:
        IFC_RETURN(DirectUI::AutomationProperties::GetFlowsToStatic(spOwner.Cast<UIElement>(), &spElements));
        break;
    case UIAXcp::APFlowsFromProperty:
        IFC_RETURN(DirectUI::AutomationProperties::GetFlowsFromStatic(spOwner.Cast<UIElement>(), &spElements));
        break;
    }

    // AutomationProperties deals with UIElements but the peer world wants to work in AutomationPeers.
    // Here we do the translation.
    if (spElements)
    {
        IFC_RETURN(spElements->get_Size(&size));
    }

    if (size > 0)
    {
        ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spPeers;
        IFC_RETURN(ctl::make(&spPeers));

        for (unsigned i = 0; i < size; ++i)
        {
            ctl::ComPtr<xaml::IDependencyObject> spDO;
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;

            IFC_RETURN(spElements->GetAt(i, &spDO));
            IFC_RETURN(spDO.Cast<UIElement>()->GetOrCreateAutomationPeer(&spAP));
            IFC_RETURN(spPeers->Append(spAP.Get()));
        }
        *returnValue = spPeers.Detach();
    }

    return S_OK;
}
