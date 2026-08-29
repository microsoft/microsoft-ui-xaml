// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"

using namespace DirectUI;

ModernCollectionBasePanel::LayoutDataInfoProvider::LayoutDataInfoProvider()
: _pMCBP(nullptr)
{}

_Check_return_ HRESULT 
ModernCollectionBasePanel::LayoutDataInfoProvider::Initialize(_In_ IModernCollectionBasePanel *pPanel)
{
    _pMCBP = pPanel;
    return S_OK;
}

void ModernCollectionBasePanel::LayoutDataInfoProvider::Uninitialize()
{
    _pMCBP = nullptr;
}

IFACEMETHODIMP
ModernCollectionBasePanel::LayoutDataInfoProvider::GetTotalItemCount(_Out_ INT* pReturnValue) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IModernCollectionBasePanel> spPanel;
    IFC(GetPanelRef(&spPanel));
    IFC(static_cast<ModernCollectionBasePanel*>(spPanel.Get())->GetTotalItemCountImpl(pReturnValue));
Cleanup:
    return hr;
}

IFACEMETHODIMP
ModernCollectionBasePanel::LayoutDataInfoProvider::GetTotalGroupCount(_Out_ INT* pReturnValue) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IModernCollectionBasePanel> spPanel;
    IFC(GetPanelRef(&spPanel));
    IFC(static_cast<ModernCollectionBasePanel*>(spPanel.Get())->GetTotalGroupCountImpl(pReturnValue));
Cleanup:
    return hr;
}

IFACEMETHODIMP
ModernCollectionBasePanel::LayoutDataInfoProvider::GetGroupInformationFromItemIndex(_In_ INT itemIndex, _Out_ INT* pIndexOfGroup, _Out_ INT* pIndexInsideGroup, _Out_ INT* pItemCountInGroup) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IModernCollectionBasePanel> spPanel;
    IFC(GetPanelRef(&spPanel));
    IFC(static_cast<ModernCollectionBasePanel*>(spPanel.Get())->GetGroupInformationFromItemIndexImpl(itemIndex, pIndexOfGroup, pIndexInsideGroup, pItemCountInGroup));
Cleanup:
    return hr;
}

IFACEMETHODIMP
ModernCollectionBasePanel::LayoutDataInfoProvider::GetGroupInformationFromGroupIndex(_In_ INT groupIndex, _Out_ INT* pStartItemIndex, _Out_ INT* pItemCountInGroup) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IModernCollectionBasePanel> spPanel;
    IFC(GetPanelRef(&spPanel));
    IFC(static_cast<ModernCollectionBasePanel*>(spPanel.Get())->GetGroupInformationFromGroupIndexImpl(groupIndex, pStartItemIndex, pItemCountInGroup));
Cleanup:
    return hr;
}

_Check_return_ HRESULT
ModernCollectionBasePanel::LayoutDataInfoProvider::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppInterface) /*override*/
{
    if (iid == __uuidof(ILayoutDataInfoProvider))
    {
        *ppInterface = static_cast<ILayoutDataInfoProvider*>(this);
        AddRefOuter();
        return S_OK;
    }
    return ComBase::QueryInterfaceImpl(iid, ppInterface);
}

_Check_return_ HRESULT 
ModernCollectionBasePanel::LayoutDataInfoProvider::GetPanelRef(_Outptr_ IModernCollectionBasePanel **ppMCBP)
{
    HRESULT hr = S_OK;
    
    *ppMCBP = nullptr;
    IFCEXPECT(_pMCBP);
    *ppMCBP = ctl::ComPtr<IModernCollectionBasePanel>(_pMCBP).Detach();

Cleanup:
    return hr;
}

//
// Actual implementation of ILayoutDataInfoProvider.
//

_Check_return_ HRESULT
ModernCollectionBasePanel::GetTotalItemCountImpl(_Out_ INT* pReturnValue)
{
    *pReturnValue = m_cacheManager.GetTotalItemCount();
    return S_OK;
}

_Check_return_ HRESULT
ModernCollectionBasePanel::GetTotalGroupCountImpl(_Out_ INT* pReturnValue)
{
    *pReturnValue = m_cacheManager.GetTotalLayoutGroupCount();
    return S_OK;
}

_Check_return_ HRESULT
ModernCollectionBasePanel::GetGroupInformationFromItemIndexImpl(_In_ INT itemIndex, _Out_ INT* pIndexOfGroup, _Out_ INT* pIndexInsideGroup, _Out_ INT* pItemCountInGroup)
{
    HRESULT hr = S_OK;
    IFC(m_cacheManager.GetGroupInformationFromItemIndex(itemIndex, pIndexOfGroup, pIndexInsideGroup, pItemCountInGroup));

    if (pIndexOfGroup)
    {
        *pIndexOfGroup = m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, *pIndexOfGroup);
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
ModernCollectionBasePanel::GetGroupInformationFromGroupIndexImpl(_In_ INT groupIndex, _Out_ INT* pStartItemIndex, _Out_ INT* pItemCountInGroup)
{
    groupIndex = m_cacheManager.LayoutIndexToDataIndex(xaml_controls::ElementType_GroupHeader, groupIndex);
    return m_cacheManager.GetGroupInformationFromGroupIndex(groupIndex, pStartItemIndex, pItemCountInGroup);
}

