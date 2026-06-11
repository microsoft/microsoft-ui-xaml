// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents the implementation of our stacking panel.
//      This is only a configuration class. All work actually happens in 
//      strategy that is separately defined. We do this for testability
//      and flexibility in layering.

#include "precomp.h"
#include "ItemsStackPanel.g.h"
#include "StackingLayoutStrategy.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

ItemsStackPanel::ItemsStackPanel()
    : m_itemsUpdatingScrollMode(xaml_controls::ItemsUpdatingScrollMode_KeepItemsInView)
{

}

ItemsStackPanel::~ItemsStackPanel()
{

}

_Check_return_ HRESULT ItemsStackPanel::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<StackingLayoutStrategy> spStacking;

    // Initialize the base class first.
    IFC(ItemsStackPanelGenerated::Initialize());

    IFC(ctl::make<StackingLayoutStrategy>(&spStacking));
    IFC(SetLayoutStrategyBase(spStacking.Get()));

    // communicate the default
    spStacking.Cast<StackingLayoutStrategy>()->SetVirtualizationDirection(xaml_controls::Orientation::Orientation_Vertical);

Cleanup:
    RRETURN(hr);
}

_Check_return_ 
    HRESULT 
    ItemsStackPanel::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(ItemsStackPanelGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ItemsStackPanel_Orientation:
        {
            xaml_controls::Orientation orientation = static_cast<xaml_controls::Orientation>(args.m_pNewValue->AsEnum());
            xaml_controls::Orientation oldOrientation = static_cast<xaml_controls::Orientation>(args.m_pOldValue->AsEnum());

            if (orientation != oldOrientation) 
            {
                ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;
                IFC(GetLayoutStrategy(&spStacking));

                IFC(CacheFirstVisibleElementBeforeOrientationChange());
                spStacking.Cast<StackingLayoutStrategy>()->SetVirtualizationDirection(orientation);
                IFC(ReevaluateGroupHeaderStrategy());
                // let the base know
                IFC(ProcessOrientationChange());
            }
        }
        break;

    case KnownPropertyIndex::ItemsStackPanel_GroupPadding:
        {
            ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;
            xaml::Thickness thickness = {};

            IFC(CValueBoxer::UnboxValue(args.m_pNewValue, &thickness));
            IFC(GetLayoutStrategy(&spStacking));

            spStacking.Cast<StackingLayoutStrategy>()->SetGroupPadding(thickness);
            IFC(InvalidateMeasure());
        }
        break;

    case KnownPropertyIndex::ItemsStackPanel_GroupHeaderPlacement:
        {
            xaml_primitives::GroupHeaderPlacement placement = static_cast<xaml_primitives::GroupHeaderPlacement>(args.m_pNewValue->AsEnum());
            IFC(SetGroupHeaderPlacement(placement));
            IFC(InvalidateMeasure());
        }
        break;

    case KnownPropertyIndex::ItemsStackPanel_ItemsUpdatingScrollMode:
        m_itemsUpdatingScrollMode = static_cast<xaml_controls::ItemsUpdatingScrollMode>(args.m_pNewValue->AsEnum());
        break;

    case KnownPropertyIndex::ItemsStackPanel_CacheLength:
        {
            DOUBLE newCacheLength = args.m_pNewValue->AsDouble();
            IFC(ModernCollectionBasePanel::put_CacheLengthBase(newCacheLength));
        }
        break;

    case KnownPropertyIndex::ItemsStackPanel_AreStickyGroupHeadersEnabled:
        {
            // pass the new value to ModernCollectionBasePanel
            IFC(ModernCollectionBasePanel::put_AreStickyGroupHeadersEnabledBase(!!args.m_pNewValue->AsBool()));
        }
        break;
    }
Cleanup:
    RRETURN(hr);
}

#pragma region Special elements overrides

_Check_return_ HRESULT
ItemsStackPanel::NeedsSpecialItem(_Out_ bool* pResult) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;
    *pResult = false;

    IFC(GetLayoutStrategy(&spStacking));
    *pResult = spStacking.Cast<StackingLayoutStrategy>()->NeedsSpecialItem();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemsStackPanel::NeedsSpecialGroup(_Out_ bool* pResult) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;
    *pResult = false;

    IFC(GetLayoutStrategy(&spStacking));
    *pResult = spStacking.Cast<StackingLayoutStrategy>()->NeedsSpecialGroup();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemsStackPanel::GetSpecialItemIndex(_Out_ int* pResult) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;
    *pResult = -1;

    IFC(GetLayoutStrategy(&spStacking));
    *pResult = spStacking.Cast<StackingLayoutStrategy>()->GetSpecialItemIndex();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemsStackPanel::GetSpecialGroupIndex(_Out_ int* pResult) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;
    *pResult = -1;

    IFC(GetLayoutStrategy(&spStacking));
    *pResult = spStacking.Cast<StackingLayoutStrategy>()->GetSpecialGroupIndex();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemsStackPanel::RegisterSpecialContainerSize(int itemIndex, wf::Size containerDesiredSize) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;

    IFC(GetLayoutStrategy(&spStacking));
    IFC(spStacking.Cast<StackingLayoutStrategy>()->RegisterSpecialContainerSize(itemIndex, containerDesiredSize));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemsStackPanel::RegisterSpecialHeaderSize(int groupIndex, wf::Size headerDesiredSize) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;

    IFC(GetLayoutStrategy(&spStacking));
    IFC(spStacking.Cast<StackingLayoutStrategy>()->RegisterSpecialHeaderSize(groupIndex, headerDesiredSize));

Cleanup:
    RRETURN(hr);
}

#pragma endregion

// Notify the layout strategy of the new header placement.
_Check_return_ HRESULT 
ItemsStackPanel::SetGroupHeaderStrategy(_In_ GroupHeaderStrategy strategy) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;

    IFC(GetLayoutStrategy(&spStacking));
    spStacking.Cast<StackingLayoutStrategy>()->SetGroupHeaderStrategy(strategy);

Cleanup:
    RRETURN(hr);
}

// Returns the size of the potential GroupPadding in the virtualizing direction.
_Check_return_ HRESULT ItemsStackPanel::GroupPaddingSizeInVirtualizingDirection(_Out_ double* groupPadding) /*override*/
{
    *groupPadding = 0.0;

    xaml::Thickness thickness = {};

    IFC_RETURN(get_GroupPadding(&thickness));

    *groupPadding = SizeFromThicknessInVirtualizingDirection(thickness);

    return S_OK;
}

// Used to estimate tracked element reposition during items source updates, when a group header is inserted.
_Check_return_ HRESULT ItemsStackPanel::GetAverageHeaderSize(_Out_ float* averageHeaderSize) /*override*/
{
    *averageHeaderSize = -1.0f;

    ctl::ComPtr<xaml_controls::ILayoutStrategy> stackingLayoutStrategy;

    IFC_RETURN(GetLayoutStrategy(&stackingLayoutStrategy));
    
    *averageHeaderSize = stackingLayoutStrategy.Cast<StackingLayoutStrategy>()->GetAverageHeaderSize();

    return S_OK;
}

// Used to estimate tracked element reposition during items source updates, when a container is inserted.
_Check_return_ HRESULT ItemsStackPanel::GetAverageContainerSize(_Out_ float* averageContainerSize) /*override*/
{
    *averageContainerSize = -1.0f;

    ctl::ComPtr<xaml_controls::ILayoutStrategy> stackingLayoutStrategy;

    IFC_RETURN(GetLayoutStrategy(&stackingLayoutStrategy));

    *averageContainerSize = stackingLayoutStrategy.Cast<StackingLayoutStrategy>()->GetAverageContainerSize();

    return S_OK;
}

// Logical Orientation override
_Check_return_ HRESULT ItemsStackPanel::get_LogicalOrientation(
    _Out_ xaml_controls::Orientation* pValue) 
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

    IFCPTR(pValue);
    *pValue = orientation;

    IFC(get_Orientation(&orientation));
    *pValue = orientation;

Cleanup:
    RRETURN(hr);
}

// Physical Orientation override
_Check_return_ HRESULT ItemsStackPanel::get_PhysicalOrientation(
    _Out_ xaml_controls::Orientation* pValue) 
{
    RRETURN(get_LogicalOrientation(pValue));
}
