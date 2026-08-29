// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemAutomationPeer.g.h"
#include "ItemsControlAutomationPeer.g.h"
#include "ItemsControl.g.h"
#include "ItemCollection.g.h"
#include "ModernCollectionBasePanel.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;


_Check_return_ HRESULT ItemAutomationPeerFactory::CreateInstanceWithParentAndItemImpl(
    _In_ IInspectable* item,
    _In_ xaml_automation_peers::IItemsControlAutomationPeer* parent,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IItemAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml_automation_peers::IItemsControlAutomationPeer* pParentAsItemsControl = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(parent);
    IFCPTR(item);

    IFC(ctl::do_query_interface(pParentAsItemsControl, parent));
    IFC(ActivateInstance(pOuter,
            static_cast<ItemsControlAutomationPeer*>(parent)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ItemAutomationPeer*>(pInstance)->put_Parent(pParentAsItemsControl));
    IFC(static_cast<ItemAutomationPeer*>(pInstance)->put_Item(item));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(pInstance);
    ReleaseInterface(pParentAsItemsControl);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

// Initializes a new instance of the ItemAutomationPeer class.
ItemAutomationPeer::ItemAutomationPeer()
{
}

// Deconstructor
ItemAutomationPeer::~ItemAutomationPeer()
{
    m_tpItem.Clear();
}

// Set the Item
_Check_return_ HRESULT
ItemAutomationPeer::put_Item(
    _In_ IInspectable* pItem)
{
    SetPtrValue( m_tpItem, pItem );
    return S_OK;
}

// Release the Item to break the circular chain(mainly in case of when container is Item itself) from parent AutomationPeer when that goes away.
// It's should be only called from ItemsControlAutomationPeer(parent of this peer) as it's life time
// is controlled by that, when parent goes away this should go away.
void
ItemAutomationPeer::ReleaseItemAndParent()
{
    // There's a circular reference between the automation peer and its control. Make sure that
    // when we release the reference to our control, it won't try to re-enter this method and
    // make us execute another release call on the control.

    TrackerPtr<IInspectable> tpItem( std::move(m_tpItem));
    tpItem.Clear();

    m_wpItemsControlAutomationPeer.Reset();

    // This makes sure that UIA Disconnect happens on the IREPS corresponding to this AP in UIAutomationCore
    NotifyManagedUIElementIsDead();
}

// This releases the link the corresponding ContainerPeer has of Item Peer via EventsSource we break this link when ItemAutomationPeer is not required anymore.
// This function needs to be called before ReleaseItemAndParent. but not when called from dtor of ItemsControlAutomationPeer.
void ItemAutomationPeer::ReleaseEventsSourceLink()
{
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    // This is to ensure the ordering of ReleaseEventsSourceLink and ReleaseItemAndParent, we can't call this function inside ReleaseItemAndParent
    // as in dtor of ItemsControlAutomationPeer we have to avoid calling ReleaseEventsSourceLink while we call ReleaseItemAndParent.
    ASSERT(m_tpItem && m_wpItemsControlAutomationPeer != NULL);

    IGNOREHR(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IGNOREHR(static_cast<FrameworkElementAutomationPeer*>(pContainerPeer)->put_EventsSource(NULL));
    }

    ReleaseInterface(pContainerPeer);
}


// Set the parent ItemsControlAutomationPeer
_Check_return_ HRESULT
ItemAutomationPeer::put_Parent(
    _In_ xaml_automation_peers::IItemsControlAutomationPeer* pItemsControlAutomationPeer)
{
   HRESULT hr = S_OK;

   if(!pItemsControlAutomationPeer)
   {
       IFC(ErrorHelper::OriginateError(AgError(UIA_INVALID_ITEMSCONTROL_PARENT)));
   }

   // Hold a weak reference to the parent, to avoid a circular reference.
   IFC( ctl::AsWeak( pItemsControlAutomationPeer, &m_wpItemsControlAutomationPeer ));

   // Set parent in core layer as well from here as the items might be queried irrespective of GetChildren.
   // Also as this is not a FrameworkElement element either the parent info in core won't be available either by Visual tree.
   IFC(CoreImports::SetAutomationPeerParent(static_cast<CAutomationPeer*>(GetHandle()),
       static_cast<CAutomationPeer*>(static_cast<ItemsControlAutomationPeer*>(pItemsControlAutomationPeer)->GetHandle())));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemAutomationPeer::ThrowElementNotAvailableException()
{
    HRESULT hr = S_OK;

    IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));

Cleanup:
    RRETURN(hr);
}


// Retrieve the Item
IFACEMETHODIMP ItemAutomationPeer::get_Item(
    _Outptr_ IInspectable** pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = NULL;

    if(m_tpItem)
    {
        *pValue = m_tpItem.Get();
        AddRefInterface(*pValue);
    }

Cleanup:

    RRETURN(hr);
}

// Retrieve the parent ItemsControlAutomationPeer
IFACEMETHODIMP ItemAutomationPeer::get_ItemsControlAutomationPeer(
    _Outptr_ xaml_automation_peers::IItemsControlAutomationPeer** ppItemsControlAutomationPeer)
{
    HRESULT hr = S_OK;

    auto spItemsControlAutomationPeer = m_wpItemsControlAutomationPeer.AsOrNull<IItemsControlAutomationPeer>();

    IFCPTR(ppItemsControlAutomationPeer);
    *ppItemsControlAutomationPeer = NULL;

    if( spItemsControlAutomationPeer )
    {
        *ppItemsControlAutomationPeer = spItemsControlAutomationPeer.Detach();
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_ HRESULT ItemAutomationPeer::GetContainer(_Outptr_ xaml::IUIElement** ppContainer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spContainer;
    ctl::ComPtr<IUIElement> spItemsControlOwnerAsUIE;
    ctl::ComPtr<IItemsControl> spItemsControlOwner;

    if(m_tpItem && m_wpItemsControlAutomationPeer != NULL)
    {
        auto spItemsControlAutomationPeer = m_wpItemsControlAutomationPeer.AsOrNull<IItemsControlAutomationPeer>();
        if( spItemsControlAutomationPeer )
        {
            IFC(spItemsControlAutomationPeer.Cast<ItemsControlAutomationPeer>()->get_Owner(&spItemsControlOwnerAsUIE));

            auto peggedItemsControl = ctl::try_make_autopeg(static_cast<UIElement*>(spItemsControlOwnerAsUIE.Get()));
            // peg the ItmesControl here so as to make sure it doesn't get neutered.
            if(peggedItemsControl)
            {
                IFC(spItemsControlOwnerAsUIE.As(&spItemsControlOwner));
                // TODO: once UIA is handling null items propertly by passing indices (bug 366772) call to UIA_GetContainerForDataItemOverride
                // should set the itemIndex parameter appropriately.  For now, to fix the drag and drop of null items scenario this
                // behavior is special cased and old code is executed (ContainerFromItem) by passing -1.
                IFC(spItemsControlOwner.Cast<ItemsControl>()->UIA_GetContainerForDataItemOverride(
                    m_tpItem.Get(),
                    -1,         // itemIndex
                    &spContainer));
            }
        }
    }
    IFC(spContainer.MoveTo(ppContainer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemAutomationPeer::GetContainerPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppContainerPeer, _Outptr_opt_ xaml::IUIElement** ppContainer)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = nullptr;
    xaml::IUIElement* pContainer = nullptr;
    xaml::IFrameworkElement* pContainerAsFrameworkElement = nullptr;

    xaml_automation_peers::IFrameworkElementAutomationPeer* pFrameworkElementAutomationPeer = nullptr;
    xaml_automation_peers::IFrameworkElementAutomationPeerFactory* pFrameworkElementAPFactory = nullptr;
    IActivationFactory* pActivationFactory = nullptr;
    IInspectable* inner = nullptr;

    IFC(GetContainer(&pContainer));
    if (pContainer != nullptr)
    {
        IFC(static_cast<UIElement*>(pContainer)->GetOrCreateAutomationPeer(&pContainerPeer));

        if (pContainerPeer == nullptr)
        {
            pActivationFactory = ctl::ActivationFactoryCreator<DirectUI::FrameworkElementAutomationPeerFactory>::CreateActivationFactory();
            IFC(ctl::do_query_interface(pFrameworkElementAPFactory, pActivationFactory));
            IFC(ctl::do_query_interface(pContainerAsFrameworkElement, pContainer));
            IFC(static_cast<FrameworkElementAutomationPeerFactory*>(pFrameworkElementAPFactory)->CreateInstanceWithOwner(static_cast<FrameworkElement*>(pContainerAsFrameworkElement),
                nullptr,
                &inner,
                &pFrameworkElementAutomationPeer));
            IFC(ctl::do_query_interface(pContainerPeer, pFrameworkElementAutomationPeer));
        }
    }

    *ppContainerPeer = pContainerPeer;
    pContainerPeer = nullptr;

    if (ppContainer != nullptr)
    {
        *ppContainer = pContainer;
        pContainer = nullptr;
    }

Cleanup:
    ReleaseInterface(pContainer);
    ReleaseInterface(pContainerAsFrameworkElement);
    ReleaseInterface(pContainerPeer);
    ReleaseInterface(pFrameworkElementAutomationPeer);
    ReleaseInterface(pFrameworkElementAPFactory);
    ReleaseInterface(pActivationFactory);
    ReleaseInterface(inner);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml_automation_peers::PatternInterface_VirtualizedItem)
    {
        *returnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        ctl::ComPtr<IAutomationPeer> spContainerPeer;
        IFC(GetContainerPeer(&spContainerPeer));

        if (spContainerPeer)
        {
            hr = spContainerPeer->GetPattern(patternInterface, returnValue);
            if (FAILED(hr))
            {
                IFC(ItemAutomationPeerGenerated::GetPatternCore(patternInterface, returnValue));
            }

            if (*returnValue == nullptr)
            {
                if (auto spContainerAsFrameworkElementAP = spContainerPeer.AsOrNull<IFrameworkElementAutomationPeer>())
                {
                    IFC(spContainerAsFrameworkElementAP.Cast<FrameworkElementAutomationPeer>()->GetDefaultPattern(patternInterface, returnValue));
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


IFACEMETHODIMP ItemAutomationPeer::GetAcceleratorKeyCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetAcceleratorKey(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetAccessKeyCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetAccessKey(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetAutomationControlType(returnValue));
    }
    else
    {
        *returnValue = xaml_automation_peers::AutomationControlType_ListItem;
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetAutomationIdCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetAutomationId(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetBoundingRectangleCore(_Out_ wf::Rect* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetBoundingRectangle(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetChildren(returnValue));
    }
    else
    {
        *returnValue = NULL;
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetClassName(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetClickablePointCore(_Out_ wf::Point* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;
    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetClickablePoint(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetHelpTextCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetHelpText(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetItemStatusCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetItemStatus(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetItemTypeCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetItemType(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetLabeledByCore(_Outptr_ IAutomationPeer** ppReturnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetLabeledBy(ppReturnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetLocalizedControlTypeCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetLocalizedControlType(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    XUINT32 pLength = 0;

    *returnValue = NULL;
    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        // TODO: Check about GetPlainText
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetName(returnValue));
    }

    pLength = WindowsGetStringLen(*returnValue);

    if((pLength == 0) && m_tpItem )
    {
        IFC(FrameworkElement::GetStringFromObject(m_tpItem.Get(), returnValue));
    }

Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetOrientationCore(_Out_ xaml_automation_peers::AutomationOrientation* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetOrientation(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::GetLiveSettingCore(_Out_ xaml_automation_peers::AutomationLiveSetting* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetLiveSetting(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

_Check_return_ HRESULT ItemAutomationPeer::GetControlledPeersCoreImpl(_Outptr_ wfc::IVectorView<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->GetControlledPeersCore(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::HasKeyboardFocusCore(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->HasKeyboardFocus(returnValue));
    }
    else
    {
        *returnValue = FALSE;
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::IsContentElementCore(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->IsContentElement(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::IsControlElementCore(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->IsControlElement(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::IsEnabledCore(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->IsEnabled(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::IsKeyboardFocusableCore(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->IsKeyboardFocusable(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::IsOffscreenCore(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->IsOffscreen(returnValue));
    }
    else
    {
        *returnValue = TRUE;
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::IsPasswordCore(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->IsPassword(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::IsRequiredForFormCore(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->IsRequiredForForm(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }
Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

// NavigateCore override is not required for ItemAutomationPeer as it can't be consumed by overrides and there's no plan to do so.
// For any further ItemsControl override please try to avoid using ItemAutomationPeer and DataItems. Usage of ItemContainer pattern
// and virtualized item pattern the sole reason this design pattern existed is not recommended anymore.
_Check_return_ HRESULT ItemAutomationPeer::GetElementFromPointCoreImpl(_In_ wf::Point point, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spContainerPeer;

    IFC(GetContainerPeer(&spContainerPeer));
    if (spContainerPeer)
    {
        IFC(spContainerPeer->GetElementFromPoint(point, ppReturnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemAutomationPeer::GetFocusedElementCoreImpl(_Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spContainerPeer;

    IFC(GetContainerPeer(&spContainerPeer));
    if (spContainerPeer)
    {
        IFC(spContainerPeer->GetFocusedElement(ppReturnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ItemAutomationPeer::SetFocusCore()
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pContainerPeer = NULL;

    IFC(GetContainerPeer(&pContainerPeer));
    if(pContainerPeer != NULL)
    {
        IFC(static_cast<AutomationPeer*>(pContainerPeer)->SetFocus());
    }

Cleanup:
    ReleaseInterface(pContainerPeer);
    RRETURN(hr);
}

_Check_return_ HRESULT ItemAutomationPeer::ShowContextMenuCoreImpl()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spContainerPeer;

    IFC(GetContainerPeer(&spContainerPeer));
    if (spContainerPeer)
    {
        IFC(spContainerPeer->ShowContextMenu());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::ItemAutomationPeer::GetPositionInSetCoreImpl(_Out_ INT* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = -1;
    auto spItemsControlAutomationPeer = m_wpItemsControlAutomationPeer.AsOrNull<IItemsControlAutomationPeer>();

    if (spItemsControlAutomationPeer)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spContainerPeer;
        ctl::ComPtr<xaml::IUIElement> spContainer;
        IFC_RETURN(GetContainerPeer(&spContainerPeer, &spContainer));

        if (spContainerPeer)
        {
            // First retrieve any valid value being directly set on the container, that value will get precedence.
            ASSERT(spContainer);
            IFC_RETURN(spContainerPeer->GetPositionInSet(pReturnValue));

            // if it still is default value, calculate it ourselves.
            if (*pReturnValue == -1)
            {
                IFC_RETURN(spItemsControlAutomationPeer.Cast<ItemsControlAutomationPeer>()->GetPositionInSetHelper(spContainer.Get(), pReturnValue));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::ItemAutomationPeer::GetSizeOfSetCoreImpl(_Out_ INT* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = -1;
    auto spItemsControlAutomationPeer = m_wpItemsControlAutomationPeer.AsOrNull<IItemsControlAutomationPeer>();

    if (spItemsControlAutomationPeer)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spContainerPeer;
        ctl::ComPtr<xaml::IUIElement> spContainer;
        IFC_RETURN(GetContainerPeer(&spContainerPeer, &spContainer));

        if (spContainerPeer)
        {
            // First retrieve any valid value being directly set on the container, that value will get precedence.
            ASSERT(spContainer);
            IFC_RETURN(spContainerPeer->GetSizeOfSet(pReturnValue));

            // if it still is default value, calculate it ourselves.
            if (*pReturnValue == -1)
            {
                IFC_RETURN(spItemsControlAutomationPeer.Cast<ItemsControlAutomationPeer>()->GetSizeOfSetHelper(spContainer.Get(), pReturnValue));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::ItemAutomationPeer::GetLevelCoreImpl(_Out_ INT* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = -1;
    auto spItemsControlAutomationPeer = m_wpItemsControlAutomationPeer.AsOrNull<IItemsControlAutomationPeer>();

    if (spItemsControlAutomationPeer)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spContainerPeer;
        IFC_RETURN(GetContainerPeer(&spContainerPeer));

        if (spContainerPeer)
        {
            // First retrieve any valid value being directly set on the container, that value will get precedence.
            IFC_RETURN(spContainerPeer->GetLevel(pReturnValue));

            // if it still is default value, calculate it ourselves.
            if (*pReturnValue == -1)
            {
                ctl::ComPtr<xaml::IUIElement> spItemsControlAsUIE;
                IFC_RETURN((spItemsControlAutomationPeer.Cast<ItemsControlAutomationPeer>())->get_Owner(&spItemsControlAsUIE));
                auto spItemsControl = spItemsControlAsUIE.AsOrNull<IItemsControl>();
                if (spItemsControl)
                {
                    BOOLEAN isGrouping = false;
                    IFC_RETURN(spItemsControl.Cast<ItemsControl>()->get_IsGrouping(&isGrouping));

                    if (isGrouping)
                    {
                        // If it's grouped list items are at level 2, otherwise it's not supported.
                        *pReturnValue = 2;
                    }
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::ItemAutomationPeer::GetAnnotationsCoreImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeerAnnotation*>** returnValue)
{
    auto spItemsControlAutomationPeer = m_wpItemsControlAutomationPeer.AsOrNull<IItemsControlAutomationPeer>();

    if (spItemsControlAutomationPeer)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spContainerPeer;
        IFC_RETURN(GetContainerPeer(&spContainerPeer));

        if (spContainerPeer)
        {
            // Retrieve any annotations provided on actual Item Containers (e.g ListViewItem)
            IFC_RETURN(spContainerPeer->GetAnnotations(returnValue));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::ItemAutomationPeer::GetLandmarkTypeCoreImpl(_Out_ xaml_automation_peers::AutomationLandmarkType* returnValue)
{
    auto spItemsControlAutomationPeer = m_wpItemsControlAutomationPeer.AsOrNull<IItemsControlAutomationPeer>();

    if (spItemsControlAutomationPeer)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spContainerPeer;
        IFC_RETURN(GetContainerPeer(&spContainerPeer));

        if (spContainerPeer)
        {
            // Retrieve any Landmark types provided on actual Item Containers (e.g ListViewItem)
            IFC_RETURN(spContainerPeer->GetLandmarkType(returnValue));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DirectUI::ItemAutomationPeer::GetLocalizedLandmarkTypeCoreImpl(_Out_ HSTRING* returnValue)
{
    auto spItemsControlAutomationPeer = m_wpItemsControlAutomationPeer.AsOrNull<IItemsControlAutomationPeer>();

    if (spItemsControlAutomationPeer)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spContainerPeer;
        IFC_RETURN(GetContainerPeer(&spContainerPeer));

        if (spContainerPeer)
        {
            // Retrieve any localized Landmark type value provided on actual Item Containers (e.g ListViewItem)
            IFC_RETURN(spContainerPeer->GetLocalizedLandmarkType(returnValue));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT  DirectUI::ItemAutomationPeer::GetCultureCoreImpl(_Out_ INT* returnValue)
{
    *returnValue = 0;
    ctl::ComPtr<IItemsControlAutomationPeer> spItemsControlAutomationPeer = m_wpItemsControlAutomationPeer.AsOrNull<IItemsControlAutomationPeer>();

    if (spItemsControlAutomationPeer)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spContainerPeer;
        IFC_RETURN(GetContainerPeer(&spContainerPeer));

        if (spContainerPeer)
        {
            // Retrieve Item Container's culture
            IFC_RETURN(spContainerPeer->GetCulture(returnValue));
        }
    }
    else
    {
        IFC_RETURN(ThrowElementNotAvailableException());
    }

    return S_OK;
}

_Check_return_ HRESULT ItemAutomationPeer::RealizeImpl()
{
    RRETURN(S_OK);
}

_Check_return_ HRESULT ItemAutomationPeer::GetItemAsStringFromPropertyValue(
                _In_ wf::IPropertyValue* pItemAsPropertValue,
                _Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    *returnValue = NULL;
    IFC(pItemAsPropertValue->GetString(returnValue));

    // TODO: Add all other cases as well, Double, int etc, ohh this is where ToString was most useful. How to let business object describe themselves.

Cleanup:
    RRETURN(hr);
}

// Notify Owner(which is ItemsControlAutomationPeer in this case) to realse AP as no UIA client is holding on to it.
_Check_return_ HRESULT ItemAutomationPeer::NotifyNoUIAClientObjectToOwner()
{
    HRESULT hr = S_OK;

    auto spItemsControlAutomationPeer = m_wpItemsControlAutomationPeer.AsOrNull<IItemsControlAutomationPeer>();

    if(spItemsControlAutomationPeer)
    {
        IFC((spItemsControlAutomationPeer.Cast<ItemsControlAutomationPeer>())->RemoveItemAutomationPeerFromStorage(this));
    }

Cleanup:
    RRETURN(hr);
}

// Raise IsSelected PropertyChanged Event.
_Check_return_ HRESULT ItemAutomationPeer::RaiseAutomationIsSelectedChanged(BOOLEAN isSelected)
{
    HRESULT hr = S_OK;
    BOOLEAN bAutomationListener = FALSE;
    BOOLEAN negIsSelected = !isSelected;
    IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bAutomationListener));
    if(bAutomationListener)
    {
        CValue valueOld;
        CValue valueNew;

        IFC(CValueBoxer::BoxValue(&valueOld, negIsSelected));
        IFC(CValueBoxer::BoxValue(&valueNew, isSelected));
        IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), UIAXcp::APAutomationProperties::APIsSelectedProperty, valueOld, valueNew));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ItemAutomationPeer::RemoveItemAutomationPeerFromItemsControlStorage()
{
    auto itemsControlAutomationPeer = m_wpItemsControlAutomationPeer.AsOrNull<IItemsControlAutomationPeer>();

    if (itemsControlAutomationPeer)
    {
        IFC_RETURN((itemsControlAutomationPeer.Cast<ItemsControlAutomationPeer>())->RemoveItemAutomationPeerFromStorage(this, true /*forceRemove*/));
    }

    return S_OK;
}
