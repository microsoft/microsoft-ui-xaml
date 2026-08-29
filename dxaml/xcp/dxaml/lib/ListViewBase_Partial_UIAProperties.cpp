// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBase.g.h"
#include "ListViewBaseItem.g.h"
#include "ListViewBaseAutomationPeer.g.h"
#include "DragDropInternal.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace xaml_automation::Provider;

_Check_return_ HRESULT ListViewBase::GetDropTargetDropEffect(_Out_ HSTRING* phsValue)
{
    HRESULT hr = S_OK;
    if (!m_strDropTargetDropEffect.Get())
    {
        IFC(UpdateDropTargetDropEffect(TRUE, FALSE /*isLeaving*/));
    }
    IFC(m_strDropTargetDropEffect.CopyTo(phsValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBase::GetDropTargetDropEffects(_Out_ UINT* cArray, _Out_ HSTRING** phsArray)
{
    HRESULT hr = S_OK;
    HSTRING* phsNewArray = static_cast<HSTRING*>(CoTaskMemAlloc(sizeof(HSTRING)));
    IFCOOMFAILFAST(phsNewArray);

    IFC(GetDropTargetDropEffect(phsNewArray));

    *cArray = 1;
    *phsArray = phsNewArray;
    phsNewArray = nullptr;

Cleanup:
    DELETE_ARRAY(phsNewArray);
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBase::GetDragGrabbedItems(_Out_ UINT* cRepsArray,
                                                        _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple*** pRepsArray)
{
    HRESULT hr = S_OK;

    if (!m_strDropTargetDropEffect.Get())
    {
        IFC(UpdateDragGrabbedItems(TRUE));
    }
    IFC(CreateCopyOfDragGrabbedItems(cRepsArray, pRepsArray));

Cleanup:
    RRETURN(hr);
}

static _Check_return_ HRESULT GetString(_In_ XUINT32 id, _Out_ HSTRING* phsString, _Out_ WCHAR** ppszString)
{
    HRESULT hr = S_OK;

    if (!*phsString)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(id, phsString));
        *ppszString = const_cast<WCHAR*>(HStringUtil::GetRawBuffer(*phsString, nullptr));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBase::UpdateDropTargetDropEffect(_In_ BOOLEAN forceUpdate, _In_ BOOLEAN isLeaving)
{
    return UpdateDropTargetDropEffect(forceUpdate, isLeaving, nullptr);
}

_Check_return_ HRESULT ListViewBase::UpdateDropTargetDropEffect(_In_ BOOLEAN forceUpdate, _In_ BOOLEAN isLeaving, _In_opt_ UIElement* keyboardReorderedContainer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;
    ctl::ComPtr<IUIElement> spDragItem;
    ctl::WeakRefPtr wrDragItemWeak;
    DragDrop* pDragDrop = nullptr;

    WCHAR szBuffer[MAX_PATH];
    WCHAR szItemBuffer[MAX_PATH];
    WCHAR szNumber[32];
    int cchBuffer = 0;

    wrl_wrappers::HString strOldValue;

    WCHAR* pszItem = nullptr;
    WCHAR* pszLeft = nullptr;
    WCHAR* pszRight = nullptr;
    WCHAR* pszOver = nullptr;

    // These strings are used to construct UIAutomation messages for drag and drop events.
    wrl_wrappers::HString strMsgItem;
    wrl_wrappers::HString strMsgDragItemsCount;
    wrl_wrappers::HString strMsgCancelDragging;
    wrl_wrappers::HString strMsgPlaceBetween;
    wrl_wrappers::HString strMsgAfter;
    wrl_wrappers::HString strMsgBefore;
    wrl_wrappers::HString strMsgPlace3;
    wrl_wrappers::HString strMsgPlaceItem;
    wrl_wrappers::HString strMsgDropInto;
    wrl_wrappers::HString strItem;

    WCHAR* pszMsgDragItemsCount = nullptr;
    WCHAR* pszMsgCancelDragging = nullptr;
    WCHAR* pszMsgPlaceBetween = nullptr;
    WCHAR* pszMsgAfter = nullptr;
    WCHAR* pszMsgBefore = nullptr;
    WCHAR* pszMsgPlace3 = nullptr;
    WCHAR* pszMsgPlaceItem = nullptr;
    WCHAR* pszMsgDropInto = nullptr;

    IFC(GetOrCreateAutomationPeer(&spAP));

    if (!spAP || (!forceUpdate && !AutomationPeer::ArePropertyChangedListeners()))
    {
        m_strDropTargetDropEffect.Release();
        goto Cleanup;
    }

    if (keyboardReorderedContainer)
    {
        spDragItem = keyboardReorderedContainer;
    }
    else
    {
        pDragDrop = DXamlCore::GetCurrent()->GetDragDrop();
        if (pDragDrop && pDragDrop->GetIsDragDropInProgress())
        {
            wrDragItemWeak = pDragDrop->GetCurrentDragItem();
        }
        if (wrDragItemWeak)
        {
            IFC(wrDragItemWeak.As<IUIElement>(&spDragItem));
        }
    }

    if (spDragItem)
    {
        XINT32 cItemCount = -1;

        if (pDragDrop)
        {
            cItemCount = pDragDrop->GetItemCount();
        }

        if (cItemCount != -1 && 1 < cItemCount)
        {
            XUINT32 cch;
            IFC(GetString(UIA_DRAG_ITEMS_COUNT, strMsgDragItemsCount.GetAddressOf(), &pszMsgDragItemsCount));
            IFC(_itow_s(cItemCount, szNumber, 10) ? E_FAIL : S_OK);
            cch = FormatMsg(szItemBuffer, pszMsgDragItemsCount, szNumber);
            IFC(1 < cch ? S_OK : E_FAIL);
            pszItem = szItemBuffer;
        }
        else
        {
            IFC(spDragItem.Cast<UIElement>()->GetOrCreateAutomationPeer(&spAP));
            if (spAP)
            {
                IFC(spAP.Cast<DirectUI::AutomationPeer>()->GetName(strItem.GetAddressOf()));
            }

            if (strItem.Get())
            {
                pszItem = const_cast<WCHAR*>(strItem.GetRawBuffer(NULL));
            }

            if (!pszItem)
            {
                IFC(GetString(UIA_DRAG_ITEM, strMsgItem.GetAddressOf(), &pszItem));
            }
        }

        if (isLeaving)
        {
            IFC(GetString(UIA_DRAG_CANCEL_DRAGGING, strMsgCancelDragging.GetAddressOf(), &pszMsgCancelDragging));
            cchBuffer = FormatMsg(szBuffer, pszMsgCancelDragging, pszItem);
        }
        else
        {
            ctl::ComPtr<xaml::IDependencyObject> spLeftItem;
            ctl::ComPtr<xaml::IDependencyObject> spRightItem;

            if (m_tpDragOverItem)
            {
                ctl::ComPtr<xaml::IDependencyObject> spOverItem;

                IFC(m_tpDragOverItem.As(&spOverItem));
                IFC(GetAutomationString(spOverItem.Get(), &pszOver));
            }
            else
            {
                int rightIndex;
                int leftIndex;

                if (keyboardReorderedContainer)
                {
                    int index = 0;
                    IFC(IndexFromContainer(keyboardReorderedContainer, &index));
                    rightIndex = index + 1;
                    leftIndex = index - 1;
                }
                else
                {
                    rightIndex = GetInsertionIndexForLiveReorder();
                    leftIndex = rightIndex - 1;
                }

                int itemsCount = -1;

                IFC(GetItemsCount(&itemsCount));

                if (leftIndex >= 0 && leftIndex < itemsCount)
                {
                    IFC(ContainerFromIndex(leftIndex, &spLeftItem));
                }

                if (rightIndex >= 0 && rightIndex < itemsCount)
                {
                    IFC(ContainerFromIndex(rightIndex, &spRightItem));
                }
            }

            if (spLeftItem)
            {
                IFC(GetAutomationString(spLeftItem.Get(), &pszLeft));
            }

            if (spRightItem)
            {
                IFC(GetAutomationString(spRightItem.Get(), &pszRight));
            }

            if (pszLeft && pszRight)
            {
                IFC(GetString(UIA_DRAG_PLACE_BETWEEN, strMsgPlaceBetween.GetAddressOf(), &pszMsgPlaceBetween));
                cchBuffer = FormatMsg(szBuffer, pszMsgPlaceBetween, pszItem, pszLeft, pszRight);
            }
            else if (pszLeft)
            {
                IFC(GetString(UIA_DRAG_AFTER   , strMsgAfter.GetAddressOf()  , &pszMsgAfter   ));
                IFC(GetString(UIA_DRAG_PLACE_3, strMsgPlace3.GetAddressOf(), &pszMsgPlace3  ));

                cchBuffer = FormatMsg(szBuffer, pszMsgPlace3, pszItem, pszMsgAfter, pszLeft);
            }
            else if (pszRight)
            {
                IFC(GetString(UIA_DRAG_BEFORE, strMsgBefore.GetAddressOf(), &pszMsgBefore ));
                IFC(GetString(UIA_DRAG_PLACE_3, strMsgPlace3.GetAddressOf(), &pszMsgPlace3 ));

                cchBuffer = FormatMsg(szBuffer, pszMsgPlace3, pszItem, pszMsgBefore, pszRight);
            }
            else if (pszOver)
            {
                IFC(GetString(UIA_DRAG_DROPINTO, strMsgDropInto.GetAddressOf(), &pszMsgDropInto));
                cchBuffer = FormatMsg(szBuffer, pszMsgDropInto, pszItem, pszOver);
            }
            else
            {
                IFC(GetString(UIA_DRAG_PLACE_ITEM, strMsgPlaceItem.GetAddressOf(), &pszMsgPlaceItem ));
                cchBuffer = FormatMsg(szBuffer, pszMsgPlaceItem, pszItem);
            }
        }

        IFC(cchBuffer < 0 ? E_FAIL : S_OK);
    }
    else
    {
        szBuffer[0] = 0;
        cchBuffer = 0;
    }

    strOldValue = std::move(m_strDropTargetDropEffect);

    IFC(m_strDropTargetDropEffect.Set(szBuffer, cchBuffer));

    if (!forceUpdate && spAP && AutomationPeer::ArePropertyChangedListeners())
    {
        if (strOldValue != m_strDropTargetDropEffect)
        {
            IFC(spAP.Cast<FrameworkElementAutomationPeer>()->RaisePropertyChangedEventById(UIAXcp::APDropTargetEffectProperty, strOldValue, m_strDropTargetDropEffect))
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBase::UpdateDragGrabbedItems(_In_ BOOLEAN forceUpdate)
{
    // The dragged items collection is really a function of the ListView but the UIA property is surfaced on the ListviewBaseItem.
    // Since ListView has all the data we do the implementation here but are careful to raise UIA events on the ListViewBaseItem's automation peer.
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;
    DragDrop* pDragDrop = nullptr;

    if (!m_tpDragGrabbedItems)
    {
        ctl::ComPtr<TrackerCollection<xaml_automation::Provider::IRawElementProviderSimple*>> spDragGrabbedItems;
        IFC(ctl::make<TrackerCollection<xaml_automation::Provider::IRawElementProviderSimple*>>(&spDragGrabbedItems));
        SetPtrValue(m_tpDragGrabbedItems, spDragGrabbedItems);
    }
    else
    {
        IFC(m_tpDragGrabbedItems->Clear());
    }

    pDragDrop = DXamlCore::GetCurrent()->GetDragDrop();
    if (pDragDrop && pDragDrop->GetIsDragDropInProgress() && m_tpPrimaryDraggedContainer)
    {
        IFC(m_tpPrimaryDraggedContainer.Cast<ListViewBaseItem>()->GetOrCreateAutomationPeer(&spAP));
    }

    if (spAP && (forceUpdate || AutomationPeer::ArePropertyChangedListeners()))
    {
        ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;
        std::vector<UINT> draggedItemIndices;

        IFC(GetDraggedItems(m_tpPrimaryDraggedContainer.Cast<ListViewBaseItem>(), &spItems, &draggedItemIndices));

#if DBG
        {
            UINT srcSize = 0;
            IFC(spItems->get_Size(&srcSize));
            ASSERT(srcSize == draggedItemIndices.size());
        }
#endif

        for (UINT i = 0; i < draggedItemIndices.size(); ++i)
        {
            ctl::ComPtr<IInspectable> spItem;
            ctl::ComPtr<IUIElement> spUIE;
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spRepsAP;
            AutomationPeer* pAP;

            IFC(spItems->GetAt(i, &spItem));
            IFC(UIA_GetContainerForDataItemOverride(spItem.Get(), static_cast<INT>(draggedItemIndices[i]), &spUIE));
            if (spUIE)
            {
                IFC(spUIE.Cast<UIElement>()->GetOrCreateAutomationPeer(&spRepsAP));
                if (spRepsAP)
                {
                    ctl::ComPtr<xaml_automation::Provider::IIRawElementProviderSimple> spItemProvider;
                    pAP = spRepsAP.Cast<AutomationPeer>();
                    ctl::addref_expected(pAP, ctl::ExpectedRef_Exception);
                    IFC(pAP->ProviderFromPeer(pAP, &spItemProvider));
                    IFC(m_tpDragGrabbedItems->Append(spItemProvider.Get()));
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBase::CreateCopyOfDragGrabbedItems(
    _Out_ UINT* pcDragGrabbedItems,
    _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple*** pppDragGrabbedItems)
{
    HRESULT hr = S_OK;
    xaml_automation::Provider::IIRawElementProviderSimple** ppDragGrabbedItems = NULL;
    UINT cDragGrabbedItems;

    // m_tpDragGrabbedItems might not have been set yet. ListViewBase::UpdateDragGrabbedItems is the only place where m_tpDragGrabbedItems gets
    // set, but this can be skipped when the first drag in the process triggers ListViewBase::OnDragGesture and UpdateDropTargetDropEffect, the
    // old string (“”) Doesn’t match the new UIA drag string, a property event fires, UIA tries to access m_tpDragGrabbedItems
    if (!m_tpDragGrabbedItems.Get())
    {
        IFC(UpdateDragGrabbedItems(FALSE));

        // If for some reason m_tpDragGrabbedItems is sill not set then something catastrophic reallly did happen!
        IFCCATASTROPHIC(m_tpDragGrabbedItems.Get());
    }
    IFC(m_tpDragGrabbedItems->get_Size(&cDragGrabbedItems));

    ppDragGrabbedItems = static_cast<xaml_automation::Provider::IIRawElementProviderSimple**>(
        CoTaskMemAlloc(sizeof(xaml_automation::Provider::IIRawElementProviderSimple*) * cDragGrabbedItems));
    IFCOOMFAILFAST(ppDragGrabbedItems);

    for (UINT i = 0; i < cDragGrabbedItems; ++i)
    {
        xaml_automation::Provider::IIRawElementProviderSimple* pItem = NULL;
        IFC(m_tpDragGrabbedItems->GetAt(i, &pItem));
        ppDragGrabbedItems[i] = pItem;
    }

    *pcDragGrabbedItems = cDragGrabbedItems;
    *pppDragGrabbedItems = ppDragGrabbedItems;
    ppDragGrabbedItems = NULL;

Cleanup:
    if (ppDragGrabbedItems)
    {
        CoTaskMemFree(ppDragGrabbedItems);
    }

    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBase::GetAutomationString(
    _In_ xaml::IDependencyObject* item,
    _Outptr_result_maybenull_ WCHAR** automationString)
{
    ctl::ComPtr<xaml::IDependencyObject> spItem(item);

    *automationString = nullptr;

    if (spItem)
    {
        wrl_wrappers::HString name;
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;

        IFC_RETURN(spItem.Cast<ListViewBaseItem>()->GetOrCreateAutomationPeer(&spAP));
        if (spAP)
        {
            IFC_RETURN(spAP.Cast<DirectUI::AutomationPeer>()->GetName(name.GetAddressOf()));
        }

        if (name.Get())
        {
            *automationString = const_cast<WCHAR*>(name.GetRawBuffer(nullptr));
        }
    }

    return S_OK;
}


