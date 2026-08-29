// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

CXamlIslandRootCollection::~CXamlIslandRootCollection()
{
}

_Check_return_ HRESULT CXamlIslandRootCollection::MeasureOverride(_In_ XSIZEF availableSize, _Out_ XSIZEF& desiredSize)
{
    desiredSize = {0,0};
    
    auto collection = static_cast<CUIElementCollection*>(GetChildren());
    if (collection)
    {
        for (auto child : *collection)
        {
            auto island = do_pointer_cast<CXamlIslandRoot>(child);
            if (island)
            {
                if (island->GetIsMeasureDirty() || island->GetIsOnMeasureDirtyPath())
                {
                    const auto islandSize = island->GetSize();
                    IFC_RETURN(island->Measure({islandSize.Width, islandSize.Height}));
                    desiredSize.width = std::max(desiredSize.width, islandSize.Width);
                    desiredSize.height = std::max(desiredSize.height, islandSize.Height);
                }
            }
       }
    }
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRootCollection::ArrangeOverride(_In_ XSIZEF finalSize, _Out_ XSIZEF& newFinalSize)
{
    auto collection = static_cast<CUIElementCollection*>(GetChildren());

    if (collection)
    {
        for (auto child : (*collection))
        {
            auto island = do_pointer_cast<CXamlIslandRoot>(child);
            if (island)
            {
                if (island->GetIsArrangeDirty() || island->GetIsOnArrangeDirtyPath())
                {
                    // We arrange to the island's given size, its top left always at 0,0.
                    const auto islandSize = island->GetSize();
                    IFC_RETURN(island->Arrange({0.0f, 0.0f, islandSize.Width, islandSize.Height}));
                }
            }
        }
    }

    newFinalSize = finalSize;
    return S_OK;
}



