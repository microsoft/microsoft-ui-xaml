// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents the implementation of our wrapping panel.
//      This is only a configuration class. All work actually happens in 
//      strategy that is separately defined. We do this for testability
//      and flexibility in layering.

#include "precomp.h"
#include "ItemsWrapGrid.g.h"
#include "WrappingLayoutStrategy.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

ItemsWrapGrid::ItemsWrapGrid()
{

}

ItemsWrapGrid::~ItemsWrapGrid()
{

}

_Check_return_ HRESULT ItemsWrapGrid::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<WrappingLayoutStrategy> spWrapping;

    // Initalize the base class first.
    IFC(ItemsWrapGridGenerated::Initialize());

    IFC(ctl::make<WrappingLayoutStrategy>(&spWrapping));
    IFC(SetLayoutStrategyBase(spWrapping.Get()));
    
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT 
ItemsWrapGrid::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(ItemsWrapGridGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ItemsWrapGrid_Orientation:
        {
            xaml_controls::Orientation orientation = static_cast<xaml_controls::Orientation>(args.m_pNewValue->AsEnum());
            xaml_controls::Orientation oldOrientation = static_cast<xaml_controls::Orientation>(args.m_pOldValue->AsEnum());

            if (orientation != oldOrientation) 
            {
                ctl::ComPtr<xaml_controls::ILayoutStrategy> spWrapping;
                IFC(GetLayoutStrategy(&spWrapping));

                IFC(CacheFirstVisibleElementBeforeOrientationChange());

                // IWG orientation is the stacking direction. Which is the opposite of the
                // virtualization direction.
                if (orientation == xaml_controls::Orientation_Horizontal)
                {
                    spWrapping.Cast<WrappingLayoutStrategy>()->SetVirtualizationDirection(xaml_controls::Orientation_Vertical);
                }
                else
                {
                    spWrapping.Cast<WrappingLayoutStrategy>()->SetVirtualizationDirection(xaml_controls::Orientation_Horizontal);
                }


                IFC(ReevaluateGroupHeaderStrategy());
                // let the base know
                IFC(ProcessOrientationChange());
            }
        }
        break;

    case KnownPropertyIndex::ItemsWrapGrid_GroupPadding:
        {
            ctl::ComPtr<xaml_controls::ILayoutStrategy> spWrapping;
            xaml::Thickness thickness = {};

            IFC(CValueBoxer::UnboxValue(args.m_pNewValue, &thickness));
            IFC(GetLayoutStrategy(&spWrapping));

            spWrapping.Cast<WrappingLayoutStrategy>()->SetGroupPadding(thickness);
            IFC(InvalidateMeasure());
        }
        break;

    case KnownPropertyIndex::ItemsWrapGrid_MaximumRowsOrColumns:
        {
            ctl::ComPtr<xaml_controls::ILayoutStrategy> spWrapping;
            INT32 maxrowsorcolumns = args.m_pNewValue->AsSigned();
            IFC(GetLayoutStrategy(&spWrapping));

            spWrapping.Cast<WrappingLayoutStrategy>()->SetMaximumRowsOrColumns(maxrowsorcolumns);
            IFC(InvalidateMeasure());

            // Likely to cause a change in wrapping that could bring in many more containers
            // Reset the cache length to avoid a delay
            IFC(ResetCacheBuffers());
        }
        break;

    case KnownPropertyIndex::ItemsWrapGrid_ItemWidth:
        {
            ctl::ComPtr<xaml_controls::ILayoutStrategy> spWrapping;
            DOUBLE itemWidth = args.m_pNewValue->AsDouble();

            IFC(GetLayoutStrategy(&spWrapping));

            spWrapping.Cast<WrappingLayoutStrategy>()->SetItemWidth(itemWidth);
            IFC(InvalidateMeasure());

            // Likely to cause a change in wrapping that could bring in many more containers
            // Reset the cache length to avoid a delay
            IFC(ResetCacheBuffers());
        }
        break;

    case KnownPropertyIndex::ItemsWrapGrid_ItemHeight:
        {
            ctl::ComPtr<xaml_controls::ILayoutStrategy> spWrapping;
            DOUBLE itemHeight = args.m_pNewValue->AsDouble();

            IFC(GetLayoutStrategy(&spWrapping));

            spWrapping.Cast<WrappingLayoutStrategy>()->SetItemHeight(itemHeight);
            IFC(InvalidateMeasure());

            // Likely to cause a change in wrapping that could bring in many more containers
            // Reset the cache length to avoid a delay
            IFC(ResetCacheBuffers());
        }
        break;
    
    case KnownPropertyIndex::ItemsWrapGrid_GroupHeaderPlacement:
        {
            xaml_primitives::GroupHeaderPlacement placement = static_cast<xaml_primitives::GroupHeaderPlacement>(args.m_pNewValue->AsEnum());
            IFC(SetGroupHeaderPlacement(placement));
            IFC(InvalidateMeasure());
        }
        break;

    case KnownPropertyIndex::ItemsWrapGrid_CacheLength:
        {
            DOUBLE newCacheLength = args.m_pNewValue->AsDouble();
            IFC(ModernCollectionBasePanel::put_CacheLengthBase(newCacheLength));
        }
        break;

    case KnownPropertyIndex::ItemsWrapGrid_AreStickyGroupHeadersEnabled:
        {
            // pass the new value to ModernCollectionBasePanel
            IFC(ModernCollectionBasePanel::put_AreStickyGroupHeadersEnabledBase(!!args.m_pNewValue->AsBool()));
        }
        break;
    }

Cleanup:
    RRETURN(hr);
}

// Logical Orientation override
_Check_return_ HRESULT ItemsWrapGrid::get_LogicalOrientation(
    _Out_ xaml_controls::Orientation* pValue) 
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFCPTR(pValue);
    *pValue = orientation;
     
    IFC(get_Orientation(&orientation));
    *pValue = orientation;

Cleanup:
    RRETURN(hr);
}

// Physical Orientation override
_Check_return_ HRESULT ItemsWrapGrid::get_PhysicalOrientation(
    _Out_ xaml_controls::Orientation* pValue) 
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;

    IFCPTR(pValue);
    *pValue = orientation;

    IFC(get_Orientation(&orientation));
    if(orientation == xaml_controls::Orientation_Vertical)
    {
        *pValue = xaml_controls::Orientation_Horizontal;
    }
    else
    {
        *pValue = xaml_controls::Orientation_Vertical;
    }

Cleanup:
    RRETURN(hr);
}

// Virtual helper method to get the ItemsPerPage that can be overridden by derived classes.
_Check_return_ HRESULT
ItemsWrapGrid::GetItemsPerPageImpl(
    _In_ wf::Rect window,
    _Out_ DOUBLE* pItemsPerPage)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spWrapping;
    
    IFC(GetLayoutStrategy(&spWrapping));
    *pItemsPerPage = spWrapping.Cast<WrappingLayoutStrategy>()->GetItemsPerPage(window);

Cleanup:
    RRETURN(hr);
}

#pragma region Special elements overrides

_Check_return_ HRESULT
ItemsWrapGrid::NeedsSpecialItem(_Out_ bool* pResult) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;
    *pResult = false;

    IFC(GetLayoutStrategy(&spStacking));
    *pResult = spStacking.Cast<WrappingLayoutStrategy>()->NeedsSpecialItem();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemsWrapGrid::NeedsSpecialGroup(_Out_ bool* pResult) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;
    *pResult = false;

    IFC(GetLayoutStrategy(&spStacking));
    *pResult = spStacking.Cast<WrappingLayoutStrategy>()->NeedsSpecialGroup();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemsWrapGrid::GetSpecialItemIndex(_Out_ int* pResult) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;
    *pResult = -1;

    IFC(GetLayoutStrategy(&spStacking));
    *pResult = spStacking.Cast<WrappingLayoutStrategy>()->GetSpecialItemIndex();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemsWrapGrid::GetSpecialGroupIndex(_Out_ int* pResult) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;
    *pResult = -1;

    IFC(GetLayoutStrategy(&spStacking));
    *pResult = spStacking.Cast<WrappingLayoutStrategy>()->GetSpecialGroupIndex();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemsWrapGrid::RegisterSpecialContainerSize(int itemIndex, wf::Size containerDesiredSize) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;

    IFC(GetLayoutStrategy(&spStacking));
    IFC(spStacking.Cast<WrappingLayoutStrategy>()->RegisterSpecialContainerSize(itemIndex, containerDesiredSize));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemsWrapGrid::RegisterSpecialHeaderSize(int groupIndex, wf::Size headerDesiredSize) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;

    IFC(GetLayoutStrategy(&spStacking));
    IFC(spStacking.Cast<WrappingLayoutStrategy>()->RegisterSpecialHeaderSize(groupIndex, headerDesiredSize));

Cleanup:
    RRETURN(hr);
}

#pragma endregion

// Notify the layout strategy of the new header placement.
_Check_return_ HRESULT
ItemsWrapGrid::SetGroupHeaderStrategy(_In_ GroupHeaderStrategy strategy) /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spStacking;

    IFC(GetLayoutStrategy(&spStacking));
    spStacking.Cast<WrappingLayoutStrategy>()->SetGroupHeaderStrategy(strategy);

Cleanup:
    RRETURN(hr);
}

// Invalidates the group cache.
_Check_return_ HRESULT 
ItemsWrapGrid::OnCollectionChangeProcessed() /*override*/
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::ILayoutStrategy> spWrapping;

    // call base
    IFC(ModernCollectionBasePanel::OnCollectionChangeProcessed());

    IFC(GetLayoutStrategy(&spWrapping));
    spWrapping.Cast<WrappingLayoutStrategy>()->InvalidateGroupCache();

Cleanup:
    RRETURN(hr);
}
