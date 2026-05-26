// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
ModernCollectionBasePanel::GetItemContainerMappingImpl(
    _Outptr_ IItemContainerMapping** ppReturnValue)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IItemContainerMapping> result(this);
    IFC(result.MoveTo(ppReturnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::GetGroupHeaderMappingImpl(
    _Outptr_ DirectUI::IGroupHeaderMapping** ppReturnValue)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IGroupHeaderMapping> result(this);
    IFC(result.MoveTo(ppReturnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::ItemFromContainerImpl(
    _In_ xaml::IDependencyObject* container,
    _Outptr_ IInspectable** ppReturnValue )
{
    RRETURN(m_containerManager.IICM_ItemFromContainer(container, ppReturnValue));
}

_Check_return_ HRESULT
ModernCollectionBasePanel::ContainerFromItemImpl(
    _In_opt_ IInspectable* item,
    _Outptr_ xaml::IDependencyObject** ppReturnValue )
{
    RRETURN(m_containerManager.IICM_ContainerFromItem(item, ppReturnValue));
}

_Check_return_ HRESULT
ModernCollectionBasePanel::IndexFromContainerImpl(
    _In_ xaml::IDependencyObject* container,
    _Out_ INT* pReturnValue )
{
    RRETURN(m_containerManager.IICM_IndexFromContainer(container, pReturnValue));
}

_Check_return_ HRESULT
ModernCollectionBasePanel::ContainerFromIndexImpl(
    _In_ INT index,
    _Outptr_ xaml::IDependencyObject** ppReturnValue )
{
    RRETURN(m_containerManager.IICM_ContainerFromIndex(index, ppReturnValue));
}

_Check_return_ HRESULT 
ModernCollectionBasePanel::GroupHeaderContainerFromItemContainerImpl(
    _In_ xaml::IDependencyObject* pItemContainer, 
    _Outptr_result_maybenull_ xaml::IDependencyObject** ppReturnValue)
{
    RRETURN(m_containerManager.IICM_GroupHeaderContainerFromItemContainerImpl(pItemContainer, ppReturnValue));
}

_Check_return_ HRESULT ModernCollectionBasePanel::GroupFromHeaderImpl(_In_ xaml::IDependencyObject* header, _Outptr_ IInspectable** returnValue) { RRETURN(m_containerManager.IGHM_GroupFromHeader(header, returnValue)); }
_Check_return_ HRESULT ModernCollectionBasePanel::HeaderFromGroupImpl(_In_ IInspectable* group, _Outptr_ xaml::IDependencyObject** returnValue)  { RRETURN(m_containerManager.IGHM_HeaderFromGroup(group, returnValue)); }
_Check_return_ HRESULT ModernCollectionBasePanel::IndexFromHeaderImpl(_In_ xaml::IDependencyObject* header, _In_ BOOLEAN excludeHiddenEmptyGroups, _Out_ INT* returnValue)  { RRETURN(m_containerManager.IGHM_IndexFromHeader(header, excludeHiddenEmptyGroups, returnValue)); }
_Check_return_ HRESULT ModernCollectionBasePanel::HeaderFromIndexImpl(_In_ INT index, _Outptr_ xaml::IDependencyObject** returnValue)  { RRETURN(m_containerManager.IGHM_HeaderFromIndex(index, returnValue)); }
