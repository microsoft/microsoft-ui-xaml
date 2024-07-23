// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{

LoopingSelectorItemDataAutomationPeer::LoopingSelectorItemDataAutomationPeer() :
    _itemIndex(-1)
{
}

_Check_return_ HRESULT
LoopingSelectorItemDataAutomationPeer::InitializeImpl(
    IInspectable* pItem,
    xaml_automation_peers::ILoopingSelectorAutomationPeer* pOwner)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeerFactory> spInnerFactory;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spInnerInstance;
    wrl::ComPtr<IInspectable> spInnerInspectable;

    IFC(LoopingSelectorItemDataAutomationPeerGenerated::InitializeImpl(pItem, pOwner));

    IFC(wf::GetActivationFactory(
          wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_AutomationPeer).Get(),
          &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<xaml_automation_peers::ILoopingSelectorItemDataAutomationPeer*>(this),
        &spInnerInspectable,
        &spInnerInstance));

    IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

    IFC(SetParent(pOwner));
    IFC(SetItem(pItem));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
LoopingSelectorItemDataAutomationPeer::SetParent(_In_opt_ xaml_automation_peers::ILoopingSelectorAutomationPeer* pOwner)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spThisAsAP;

    IFC(QueryInterface(
        __uuidof(xaml::Automation::Peers::IAutomationPeer),
        &spThisAsAP));

    IFC(wrl::ComPtr<xaml_automation_peers::ILoopingSelectorAutomationPeer>(pOwner).AsWeak(&_wrParent));

    if (pOwner)
    {
        wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spLSAsAP;
        IFC(pOwner->QueryInterface<xaml::Automation::Peers::IAutomationPeer>(&spLSAsAP));
        // NOTE: This causes an addref most likely, I think that's a good idea.
        IFC(spThisAsAP->SetParent(spLSAsAP.Get()));
    }
    else
    {
        IFC(spThisAsAP->SetParent(nullptr));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
LoopingSelectorItemDataAutomationPeer::SetItem(_In_ IInspectable* pItem)
{
    RRETURN(SetPtrValue(_tpItem, pItem));
}

_Check_return_ HRESULT
LoopingSelectorItemDataAutomationPeer::GetItem(_Outptr_result_maybenull_ IInspectable** ppItem)
{
    *ppItem = nullptr;
    _tpItem.CopyTo(ppItem);
    RRETURN(S_OK);
}

HRESULT LoopingSelectorItemDataAutomationPeer::SetItemIndex(int index)
{
    _itemIndex = index;
    RRETURN(S_OK);
}

_Check_return_ HRESULT
LoopingSelectorItemDataAutomationPeer::ThrowElementNotAvailableException()
{
    RRETURN(UIA_E_INVALIDOPERATION);
}

_Check_return_ HRESULT
LoopingSelectorItemDataAutomationPeer::GetContainerAutomationPeer(_Outptr_result_maybenull_ xaml::Automation::Peers::IAutomationPeer** ppContainer)
{
    *ppContainer = nullptr;
    wrl::ComPtr<xaml_automation_peers::ILoopingSelectorAutomationPeer> spParent;
    IFC_RETURN(_wrParent.As(&spParent));
    if (spParent)
    {
        wrl::ComPtr<ILoopingSelectorItemAutomationPeer> spLSIAP;
        IFC_RETURN((static_cast<LoopingSelectorAutomationPeer*>(spParent.Get()))->GetContainerAutomationPeerForItem(_tpItem.Get(), &spLSIAP));

        if (!spLSIAP)
        {
            // If the item has not been realized, spLSIAP will be null.
            // Realize the item on demand now and try again
            IFC_RETURN(Realize());
            IFC_RETURN((static_cast<LoopingSelectorAutomationPeer*>(spParent.Get()))->GetContainerAutomationPeerForItem(_tpItem.Get(), &spLSIAP));
        }

        if (spLSIAP)
        {
            IFC_RETURN(spLSIAP.CopyTo(ppContainer));
        }
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT
LoopingSelectorItemDataAutomationPeer::RealizeImpl()
{
    wrl::ComPtr<xaml_automation_peers::ILoopingSelectorAutomationPeer> spParent;
    IFC_RETURN(_wrParent.As(&spParent));
    if (spParent && _tpItem)
    {
        IFC_RETURN((static_cast<LoopingSelectorAutomationPeer*>(spParent.Get()))->RealizeItemAtIndex(_itemIndex));
    }

    RRETURN(S_OK);
}

#pragma region Method forwarders

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetPatternCoreImpl(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml::Automation::Peers::PatternInterface_VirtualizedItem)
    {
        *returnValue = static_cast<ILoopingSelectorItemDataAutomationPeer*>(this);
        AddRef();
    }
    else
    {
        wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
        IFC(GetContainerAutomationPeer(&spAutomationPeer));

        if (spAutomationPeer)
        {
            IFC(spAutomationPeer->GetPattern(patternInterface, returnValue));
        }
        else
        {
            IFC(LoopingSelectorItemDataAutomationPeerGenerated::GetPatternCoreImpl(patternInterface, returnValue));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetAcceleratorKeyCoreImpl(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = nullptr;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetAcceleratorKey(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetAccessKeyCoreImpl(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = nullptr;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetAccessKey(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetAutomationControlTypeCoreImpl(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetAutomationControlType(returnValue));
    }
    else
    {
        *returnValue = xaml::Automation::Peers::AutomationControlType_ListItem;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetAutomationIdCoreImpl(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    
    *returnValue = nullptr;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetAutomationId(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetBoundingRectangleCoreImpl(_Out_ wf::Rect* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = {0, 0, 0, 0};
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetBoundingRectangle(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetChildrenCoreImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetChildren(returnValue));
    }
    else
    {
        *returnValue = nullptr;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetClassNameCoreImpl(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;
    
    *returnValue = nullptr;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetClassName(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetClickablePointCoreImpl(_Out_ wf::Point* returnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetClickablePoint(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetHelpTextCoreImpl(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = nullptr;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetHelpText(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetItemStatusCoreImpl(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = nullptr;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetItemStatus(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetItemTypeCoreImpl(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = nullptr;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetItemType(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetLabeledByCoreImpl(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetLabeledBy(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetLocalizedControlTypeCoreImpl(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = nullptr;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetLocalizedControlType(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetNameCoreImpl(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = nullptr;

    if (_tpItem)
    {
        IFC(Private::AutomationHelper::GetPlainText(_tpItem.Get(), returnValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetOrientationCoreImpl(_Out_ xaml_automation_peers::AutomationOrientation* returnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetOrientation(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetLiveSettingCoreImpl(_Out_ xaml_automation_peers::AutomationLiveSetting* returnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->GetLiveSetting(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetControlledPeersCoreImpl(_Outptr_ wfc::IVectorView<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = nullptr;

    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::HasKeyboardFocusCoreImpl(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->HasKeyboardFocus(returnValue));
    }
    else
    {
        *returnValue = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::IsContentElementCoreImpl(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = FALSE;    
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->IsContentElement(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::IsControlElementCoreImpl(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = FALSE;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->IsControlElement(returnValue));
    }
    else
    {        
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::IsEnabledCoreImpl(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = FALSE;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->IsEnabled(returnValue));
    }
    else
    {        
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::IsKeyboardFocusableCoreImpl(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = FALSE;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->IsKeyboardFocusable(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::IsOffscreenCoreImpl(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->IsOffscreen(returnValue));
    }
    else
    {
        *returnValue = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::IsPasswordCoreImpl(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    
    *returnValue = FALSE;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->IsPassword(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::IsRequiredForFormCoreImpl(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = FALSE;
    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->IsRequiredForForm(returnValue));
    }
    else
    {
        IFC(ThrowElementNotAvailableException());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::SetFocusCoreImpl()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
    IFC(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC(spAutomationPeer->SetFocus());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetAnnotationsCoreImpl(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeerAnnotation*>** returnValue)
{
    wrl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    IFC_RETURN(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC_RETURN(spAutomationPeer->GetAnnotations(returnValue));
    }
    else
    {        
        IFC_RETURN(ThrowElementNotAvailableException());
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetPositionInSetCoreImpl(_Out_ INT* returnValue)
{
    *returnValue = 0;
    wrl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    IFC_RETURN(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC_RETURN(spAutomationPeer->GetPositionInSet(returnValue));
    }
    else
    {
        IFC_RETURN(ThrowElementNotAvailableException());
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetSizeOfSetCoreImpl(_Out_ INT* returnValue)
{
    *returnValue = 0;
    wrl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    IFC_RETURN(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC_RETURN(spAutomationPeer->GetSizeOfSet(returnValue));
    }
    else
    {
        IFC_RETURN(ThrowElementNotAvailableException());
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetLevelCoreImpl(_Out_ INT* returnValue)
{
    *returnValue = 0;
    wrl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    IFC_RETURN(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC_RETURN(spAutomationPeer->GetLevel(returnValue));
    }
    else
    {
        IFC_RETURN(ThrowElementNotAvailableException());
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetLandmarkTypeCoreImpl(_Out_ xaml_automation_peers::AutomationLandmarkType* returnValue)
{
    wrl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    IFC_RETURN(GetContainerAutomationPeer(&spAutomationPeer));
    
    if (spAutomationPeer)
    {
        IFC_RETURN(spAutomationPeer->GetLandmarkType(returnValue));
    }
    else
    {
        *returnValue = xaml_automation_peers::AutomationLandmarkType::AutomationLandmarkType_None;
        IFC_RETURN(ThrowElementNotAvailableException());
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT LoopingSelectorItemDataAutomationPeer::GetLocalizedLandmarkTypeCoreImpl(_Out_ HSTRING* returnValue)
{
    wrl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    IFC_RETURN(GetContainerAutomationPeer(&spAutomationPeer));

    if (spAutomationPeer)
    {
        IFC_RETURN(spAutomationPeer->GetLocalizedLandmarkType(returnValue));
    }
    else
    {
        *returnValue = nullptr;    
        IFC_RETURN(ThrowElementNotAvailableException());
    }

    RRETURN(S_OK);
}

#pragma endregion

} } } } } XAML_ABI_NAMESPACE_END
