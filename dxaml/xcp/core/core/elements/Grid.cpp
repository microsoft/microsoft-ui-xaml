// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "definitioncollection.h"
#include <stack_vector.h>

#undef max

CGrid::~CGrid()
{
    ReleaseInterface(m_pRows);
    ReleaseInterface(m_pColumns);
    ReleaseInterface(m_pColumnDefinitions);
    ReleaseInterface(m_pRowDefinitions);
    delete [] m_ppTempDefinitions;
}

// Get the row index of a child.
unsigned int CGrid::GetRowIndex(
    _In_ const CUIElement* const child) const
{
    return std::min(
        static_cast<const CFrameworkElement* const>(child)->m_pLayoutProperties->m_nGridRow,
        m_pRows->GetCount() - 1);
}

// Get the column index of a child.
unsigned int CGrid::GetColumnIndex(
    _In_ const CUIElement* const child) const
{
    return std::min(
        static_cast<const CFrameworkElement* const>(child)->m_pLayoutProperties->m_nGridColumn,
        m_pColumns->GetCount() - 1);
}

// Get the row span value of a child.
unsigned int CGrid::GetRowSpan(
    _In_ const CUIElement* const child) const
{
    return std::min(
        static_cast<const CFrameworkElement* const>(child)->m_pLayoutProperties->m_nGridRowSpan,
        m_pRows->GetCount() - GetRowIndex(child));
}

// Get the column span value of a child.
unsigned int CGrid::GetColumnSpan(
    _In_ const CUIElement* const child) const
{
    return std::min(
        static_cast<const CFrameworkElement* const>(child)->m_pLayoutProperties->m_nGridColumnSpan,
        m_pColumns->GetCount() - GetColumnIndex(child));
}

//------------------------------------------------------------------------
//
//  Method: GetRow
//
//  Synopsis: Get the row definition for a Grid child
//
//------------------------------------------------------------------------
CDefinitionBase* CGrid::GetRowNoRef(
    _In_ CUIElement* pChild)
{
    return static_cast<CDefinitionBase*>(m_pRows->GetItemImpl(GetRowIndex(pChild)));
}


//------------------------------------------------------------------------
//
//  Method: GetColumn
//
//  Synopsis: Get the column definition for a Grid child
//
//------------------------------------------------------------------------
CDefinitionBase* CGrid::GetColumnNoRef(
    _In_ CUIElement* pChild)
{
    return static_cast<CDefinitionBase*>(m_pColumns->GetItemImpl(GetColumnIndex(pChild)));
}

// Locks user-defined RowDefinitions and ColumnDefinitions so that user code
// cannot change them while we are working with them.
void CGrid::LockDefinitions()
{
    if (m_pRowDefinitions)
    {
        m_pRowDefinitions->Lock();
    }

    if (m_pColumnDefinitions)
    {
        m_pColumnDefinitions->Lock();
    }
}

// Unlocks user-defined RowDefinitions and ColumnDefinitions.
void CGrid::UnlockDefinitions()
{
    if (m_pRowDefinitions)
    {
        m_pRowDefinitions->Unlock();
    }

    if (m_pColumnDefinitions)
    {
        m_pColumnDefinitions->Unlock();
    }
}

//------------------------------------------------------------------------
//
//  Method:   ValidateDefinitionStructure
//
//  Synopsis: Initializes m_pRows and  m_pColumns either to user supplied ColumnDefinitions collection
//                 or to a default single element collection. This is the only method where user supplied
//                 row or column definitions is directly used. All other must use m_pRows/m_pColumns
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGrid::InitializeDefinitionStructure()
{
    HRESULT hr = S_OK;
    CRowDefinition* emptyRow = nullptr;
    CColumnDefinition* emptyColumn = nullptr;

    ASSERT(!IsWithoutRowAndColumnDefinitions());
    ReleaseInterface(m_pRows);
    ReleaseInterface(m_pColumns);

    if(m_pRowDefinitions == nullptr || m_pRowDefinitions->GetCount() == 0)
    {
        //empty collection defaults to single row
        CValue value;
        value.WrapObjectNoRef(nullptr);
        CREATEPARAMETERS param(this->GetContext(), value);
        IFC(CRowDefinitionCollection::Create((CDependencyObject**)(&m_pRows), &param));

        IFC(CRowDefinition::Create((CDependencyObject**)(&emptyRow), &param));

        IFC(m_pRows->Append(emptyRow));
    }
    else
    {
        m_pRows = m_pRowDefinitions;
        m_pRowDefinitions->AddRef();
    }

    if(m_pColumnDefinitions == nullptr || m_pColumnDefinitions->GetCount() == 0)
    {
        //empty collection defaults to single row
        CValue value;
        value.WrapObjectNoRef(nullptr);
        CREATEPARAMETERS param(this->GetContext(), value);
        IFC(CColumnDefinitionCollection::Create((CDependencyObject**)(&m_pColumns), &param));

        IFC(CColumnDefinition::Create((CDependencyObject**)(&emptyColumn), &param));

        IFC(m_pColumns->Append(emptyColumn));
    }
    else
    {
        m_pColumns = m_pColumnDefinitions;
        m_pColumnDefinitions->AddRef();
    }

Cleanup:
    ReleaseInterface(emptyRow);
    ReleaseInterface(emptyColumn);
    RRETURN(hr);
}

// Sets the initial, effective values of a CDefinitionCollectionBase.
void CGrid::ValidateDefinitions(
    _In_ const CDefinitionCollectionBase* const definitions,
    const bool treatStarAsAuto)
{
    for (auto& cdo : *definitions)
    {
        auto def = static_cast<CDefinitionBase*>(cdo);
        const bool useLayoutRounding = GetUseLayoutRounding();
        float userSize = std::numeric_limits<float>::infinity();
        float userMinSize = useLayoutRounding
            ? LayoutRound(def->GetUserMinSize())
            : def->GetUserMinSize();
        float userMaxSize = useLayoutRounding
            ? LayoutRound(def->GetUserMaxSize())
            : def->GetUserMaxSize();

        switch (def->GetUserSizeType())
        {
            case DirectUI::GridUnitType::Pixel:
                userSize = useLayoutRounding
                    ? LayoutRound(def->GetUserSizeValue())
                    : def->GetUserSizeValue();
                userMinSize = std::max(userMinSize, std::min(userSize, userMaxSize));
                def->SetEffectiveUnitType(DirectUI::GridUnitType::Pixel);
                break;
            case DirectUI::GridUnitType::Auto:
                def->SetEffectiveUnitType(DirectUI::GridUnitType::Auto);
                break;
            case DirectUI::GridUnitType::Star:
                if (treatStarAsAuto)
                {
                    def->SetEffectiveUnitType(DirectUI::GridUnitType::Auto);
                }
                else
                {
                    def->SetEffectiveUnitType(DirectUI::GridUnitType::Star);
                }
                break;
            default:
                ASSERT(false);
                break;
        }

        def->SetEffectiveMinSize(userMinSize);
        def->SetMeasureArrangeSize(std::max(userMinSize, std::min(userSize, userMaxSize)));
    }
}

// Organizes the grid cells into four groups, defining the order in which
// these should be measured.
CellGroups CGrid::ValidateCells(
    _In_ const CUIElementCollectionWrapper& children,
    _Inout_ CellCacheStackVector& cellCacheVector)
{
    m_gridFlags = GridFlags::None;

    CellGroups cellGroups;
    cellGroups.group1 = std::numeric_limits<unsigned int>::max();
    cellGroups.group2 = std::numeric_limits<unsigned int>::max();
    cellGroups.group3 = std::numeric_limits<unsigned int>::max();
    cellGroups.group4 = std::numeric_limits<unsigned int>::max();

    unsigned int childrenCount = children.GetCount();

    // Initialize the cells in the cell cache.
    cellCacheVector.m_vector.clear();
    cellCacheVector.m_vector.resize(childrenCount);

    unsigned int childIndex = childrenCount;
    while (childIndex-- > 0)
    {
        CUIElement* currentChild = children[childIndex];
        CellCache* cell = &cellCacheVector.m_vector[childIndex];

        cell->m_child = currentChild;
        cell->m_rowHeightTypes = GetLengthTypeForRange(
            m_pRows,
            GetRowIndex(currentChild),
            GetRowSpan(currentChild));
        cell->m_columnWidthTypes = GetLengthTypeForRange(
            m_pColumns,
            GetColumnIndex(currentChild),
            GetColumnSpan(currentChild));

        // Grid classifies cells into four groups based on their column/row
        // type. The following diagram depicts all the possible combinations
        // and their corresponding cell group:
        //
        //                  Px      Auto     Star
        //              +--------+--------+--------+
        //              |        |        |        |
        //           Px |    1   |    1   |    3   |
        //              |        |        |        |
        //              +--------+--------+--------+
        //              |        |        |        |
        //         Auto |    1   |    1   |    3   |
        //              |        |        |        |
        //              +--------+--------+--------+
        //              |        |        |        |
        //         Star |    4   |    2   |    4   |
        //              |        |        |        |
        //              +--------+--------+--------+

        if (!CellCache::IsStar(cell->m_rowHeightTypes))
        {
            if (!CellCache::IsStar(cell->m_columnWidthTypes))
            {
                cell->m_next = cellGroups.group1;
                cellGroups.group1 = childIndex;
            }
            else
            {
                cell->m_next = cellGroups.group3;
                cellGroups.group3 = childIndex;

                if (CellCache::IsAuto(cell->m_rowHeightTypes))
                {
                    // Remember that this Grid has at least one Auto row;
                    // useful for detecting cyclic dependency while measuring.
                    SetGridFlags(GridFlags::HasAutoRowsAndStarColumn);
                }
            }
        }
        else
        {
            SetGridFlags(GridFlags::HasStarRows);

            if (CellCache::IsAuto(cell->m_columnWidthTypes) && !CellCache::IsStar(cell->m_columnWidthTypes))
            {
                cell->m_next = cellGroups.group2;
                cellGroups.group2 = childIndex;
            }
            else
            {
                cell->m_next = cellGroups.group4;
                cellGroups.group4 = childIndex;
            }
        }

        if (CellCache::IsStar(cell->m_columnWidthTypes))
        {
            SetGridFlags(GridFlags::HasStarColumns);
        }
    }

    return cellGroups;
}

//------------------------------------------------------------------------
//
//  Method:   MeasureCellsGroup
//
//  Synopsis: Measure one group of cells
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CGrid::MeasureCellsGroup(
    unsigned int cellsHead, //cell group number
    unsigned int cellCount, //elements in the cell
    const float rowSpacing,
    const float columnSpacing,
    const bool ignoreColumnDesiredSize,
    const bool forceRowToInfinity,
    _Inout_ CellCacheStackVector& cellCacheVector)
{
    SpanStoreStackVector spanStore;
    
    if(cellsHead >= cellCount)
    {
        return S_OK;
    }

    do
    {
        CellCache* cell = &cellCacheVector.m_vector[cellsHead];
        CUIElement* pChild = cell->m_child;

        IFC_RETURN(MeasureCell(pChild, cell->m_rowHeightTypes, cell->m_columnWidthTypes, forceRowToInfinity, rowSpacing, columnSpacing));

        //If a span exists, add to span store for delayed processing. processing is done when
        //all the desired sizes for a given definition index and span value are known.

        if (!ignoreColumnDesiredSize)
        {
            XUINT32 columnSpan = GetColumnSpan(pChild);
            IFC_RETURN(pChild->EnsureLayoutStorage());
            if(columnSpan ==1 )
            {
                CDefinitionBase* pChildColumn = GetColumnNoRef(pChild);
                pChildColumn->UpdateEffectiveMinSize(pChild->DesiredSize.width);
            }
            else
            {
                RegisterSpan(
                    spanStore, 
                    GetColumnIndex(pChild), 
                    columnSpan, 
                    pChild->DesiredSize.width, 
                    true /* isColumnDefinition */);
            }
        }

        if (!forceRowToInfinity)
        {
            XUINT32 rowSpan = GetRowSpan(pChild);
            IFC_RETURN(pChild->EnsureLayoutStorage());
            if(rowSpan == 1)
            {
                CDefinitionBase* pChildRow = GetRowNoRef(pChild);
                pChildRow->UpdateEffectiveMinSize(pChild->DesiredSize.height);
            }
            else
            {
                RegisterSpan(
                    spanStore, 
                    GetRowIndex(pChild), 
                    rowSpan, 
                    pChild->DesiredSize.height, 
                    false /* isColumnDefinition */);
            }
        }
        cellsHead = cellCacheVector.m_vector[cellsHead].m_next;

    } while (cellsHead < cellCount);

    //Go through the spanned rows/columns allocating sizes.
    for (auto& entry : spanStore.m_vector)
    {
        if (entry.m_isColumnDefinition)
        {
            EnsureMinSizeInDefinitionRange(
                m_pColumns,
                entry.m_spanStart,
                entry.m_spanCount,
                columnSpacing,
                entry.m_desiredSize);
        }
        else
        {
            EnsureMinSizeInDefinitionRange(
                m_pRows,
                entry.m_spanStart,
                entry.m_spanCount,
                rowSpacing,
                entry.m_desiredSize);
        }
    }

    return S_OK;
}

// Measure a child of the Grid by taking in consideration the properties of
// the cell it belongs to.
_Check_return_ HRESULT CGrid::MeasureCell(
    _In_ CUIElement* const child,
    const CellUnitTypes rowHeightTypes,
    const CellUnitTypes columnWidthTypes,
    const bool forceRowToInfinity,
    const float rowSpacing,
    const float columnSpacing)
{
    XSIZEF availableSize = {};

    if (CellCache::IsAuto(columnWidthTypes) && !CellCache::IsStar(columnWidthTypes))
    {
        // If this cell belongs to at least one Auto column and not a single
        // Star column, then it should be measured freely to fit its content.
        // In other words, we must give it an infinite available width.
        availableSize.width = std::numeric_limits<float>::infinity();
    }
    else
    {
        availableSize.width = GetAvailableSizeForRange(
            m_pColumns,
            GetColumnIndex(child),
            GetColumnSpan(child),
            columnSpacing);
    }

    if (forceRowToInfinity
        || (CellCache::IsAuto(rowHeightTypes) && !CellCache::IsStar(rowHeightTypes)))
    {
        // If this cell belongs to at least one Auto row and not a single Star
        // row, then it should be measured freely to git its content. In other
        // words, we must give it an infinite available height.
        availableSize.height = std::numeric_limits<float>::infinity();
    }
    else
    {
        availableSize.height = GetAvailableSizeForRange(
            m_pRows,
            GetRowIndex(child),
            GetRowSpan(child),
            rowSpacing);
    }

    IFC_RETURN(child->Measure(availableSize));

    return S_OK;
}

// Adds a span entry to the list.
void CGrid::RegisterSpan(
    _Inout_ SpanStoreStackVector& spanStore,
    const unsigned int spanStart,
    const unsigned int spanCount,
    const float desiredSize,
    const bool isColumnDefinition)
{
    auto& spanStoreVector = spanStore.m_vector;
    // If an entry already exists with the same row/column index and span, 
    // then update the desired size stored in the entry.
    auto it = std::find_if(
            spanStoreVector.begin(),
            spanStoreVector.end(),
            [isColumnDefinition, spanStart, spanCount](const SpanStoreEntry &entry)
            {
                return  entry.m_isColumnDefinition == isColumnDefinition && entry.m_spanStart == spanStart && entry.m_spanCount == spanCount;
            });

    if (it != spanStoreVector.end())
    {
        if ((*it).m_desiredSize < desiredSize)
        {
            (*it).m_desiredSize = desiredSize;
        }
    }
    else
    {
        spanStoreVector.emplace_back(spanStart, spanCount, desiredSize, isColumnDefinition);
    }
}


//------------------------------------------------------------------------
//
//  Method:   EnsureMinSizeInDefinitionRange
//
//  Synopsis:  Distributes min size back to definition array's range.
//
//------------------------------------------------------------------------
void CGrid::EnsureMinSizeInDefinitionRange(
    _In_ CDOCollection* definitions,
    const unsigned int spanStart,
    const unsigned int spanCount,
    const float spacing,
    const float childDesiredSize)
{
    ASSERT((spanCount > 1) && (spanStart + spanCount) <= definitions->GetCount());

    // The spacing between definitions that this element spans through must not
    // be distributed.
    float requestedSize = std::max((childDesiredSize - spacing * (spanCount - 1)), 0.0f);

    //  No need to process if asked to distribute "zero".
    if (requestedSize <= REAL_EPSILON)
    {
        return;
    }

    const unsigned int spanEnd = spanStart + spanCount;
    unsigned int autoDefinitionsCount = 0;
    float rangeMinSize = 0.0f;
    float rangeMaxSize = 0.0f;
    float rangePreferredSize = 0.0f;
    float maxMaxSize = 0.0f;

    EnsureTempDefinitionsStorage(spanCount);

    // First, we need to obtain the necessary information:
    // a) Sum up the sizes in the range.
    // b) Cache the maximum size into SizeCache.
    // c) Obtain max of MaxSizes.
    // d) Count the number of auto definitions in the range.
    // e) Prepare indices.
    for (unsigned int i = spanStart; i < spanEnd; i++)
    {
        auto def = static_cast<CDefinitionBase*>((*definitions)[i]);
        float effectiveMinSize = def->GetEffectiveMinSize();
        float preferredSize = def->GetPreferredSize();
        float maxSize = std::max(def->GetUserMaxSize(), effectiveMinSize);
        rangeMinSize += effectiveMinSize;
        rangePreferredSize += preferredSize;
        rangeMaxSize += maxSize;

        // Sanity check: effectiveMinSize must always be the smallest value, maxSize
        // must be the largest one, and the preferredSize should fall in between.
        ASSERT(effectiveMinSize <= preferredSize
            && preferredSize <= maxSize
            && rangeMinSize <= rangePreferredSize
            && rangePreferredSize <= rangeMaxSize);

        def->SetSizeCache(maxSize);
        maxMaxSize = std::max(maxMaxSize, maxSize);

        if (def->GetUserSizeType() == DirectUI::GridUnitType::Auto)
        {
            autoDefinitionsCount++;
        }

        m_ppTempDefinitions[i - spanStart] = def;
    }

    if (requestedSize <= rangeMinSize)
    {
        // No need to process if the range is already big enough.
        return;
    }
    else if (requestedSize <= rangePreferredSize)
    {
        // If the requested size fits within the preferred size of the range,
        // we distribute the space following this logic:
        // - Do not distribute into Auto definitions; they should continue to
        //   stay "tight".
        // - For all non-Auto definitions, distribute to equi-size min sizes
        //   without exceeding the preferred size of the definition.
        //
        // In order to achieve this, the definitions are sorted in a way so
        // that all Auto definitions go first, then the other definitions
        // follow in ascending order of PreferredSize.
        float sizeToDistribute = requestedSize;
        SortDefinitionsForSpanPreferredDistribution(m_ppTempDefinitions, spanCount);

        // Process Auto definitions.
        for (unsigned int i = 0; i < autoDefinitionsCount; i++)
        {
            auto def = m_ppTempDefinitions[i];
            ASSERT(def->GetUserSizeType() == DirectUI::GridUnitType::Auto);

            sizeToDistribute -= def->GetEffectiveMinSize();
        }

        // Process the remaining, non-Auto definitions, distributing
        // the requested size among them.
        for (unsigned int i = autoDefinitionsCount; i < spanCount; i++)
        {
            auto def = m_ppTempDefinitions[i];
            ASSERT(def->GetUserSizeType() != DirectUI::GridUnitType::Auto);

            float newMinSize = std::min((sizeToDistribute / (spanCount - i)), def->GetPreferredSize());
            def->UpdateEffectiveMinSize(newMinSize);
            sizeToDistribute -= newMinSize;

            // Stop if there's no more space to distribute.
            if(sizeToDistribute < REAL_EPSILON)
            {
                break;
            }
        }
    }
    else if (requestedSize <= rangeMaxSize)
    {
        // If the requested size is larger than the preferred size of the range
        // but still fits within the max size of the range, we distribute the
        // space following this logic:
        // - Do not distribute into Auto definitions *if possible*; they should
        //   continue to stay "tight".
        // - For all non-Auto definitions, distribute to equi-size min sizes
        //   without exceeding the max size.
        //
        // In order to achieve this, the definitions are sorted in a way so
        // that all non-Auto definitions go first, followed by the Auto
        // definitions, and all of them in ascending order of MaxSize, which
        // is currently stored in the size cache of each definition.
        float sizeToDistribute = requestedSize - rangePreferredSize;
        SortDefinitionsForSpanMaxSizeDistribution(m_ppTempDefinitions, spanCount);

        unsigned int nonAutoDefinitionsCount = spanCount - autoDefinitionsCount;
        for (unsigned int i = 0; i < spanCount; i++)
        {
            auto def = m_ppTempDefinitions[i];
            float newMinSize = def->GetPreferredSize();

            if (i < nonAutoDefinitionsCount)
            {
                // Processing non-Auto definitions.
                ASSERT(def->GetUserSizeType() != DirectUI::GridUnitType::Auto);
                newMinSize += sizeToDistribute / (nonAutoDefinitionsCount - i);
            }
            else
            {
                // Processing the remaining, Auto definitions.
                ASSERT(def->GetUserSizeType() == DirectUI::GridUnitType::Auto);
                newMinSize += sizeToDistribute / (spanCount - i);
            }

            // Cache PreferredSize and update MinSize.
            float preferredSize = def->GetPreferredSize();
            newMinSize = std::min(newMinSize, def->GetSizeCache());
            def->UpdateEffectiveMinSize(newMinSize);

            sizeToDistribute -= def->GetEffectiveMinSize() - preferredSize;

            // Stop if there's no more space to distribute.
            if(sizeToDistribute < REAL_EPSILON)
            {
                break;
            }
        }
    }
    else
    {
        // If the requested size is larger than the max size of the range, we
        // distribute the space following this logic:
        // - For all definitions, distribute to equi-size min sizes.
        float equallyDistributedSize = requestedSize / spanCount;

        if ((equallyDistributedSize < maxMaxSize) && ((maxMaxSize - equallyDistributedSize) > REAL_EPSILON))
        {
            // If equi-size is less than the maximum of max sizes, then
            // we distribute space so that smaller definitions grow
            // faster than larger ones.
            float totalRemainingSize = maxMaxSize * spanCount - rangeMaxSize;
            float sizeToDistribute = requestedSize - rangeMaxSize;

            ASSERT(IsFiniteF(totalRemainingSize)
                && totalRemainingSize > 0
                && IsFiniteF(sizeToDistribute)
                && sizeToDistribute > 0);

            for (unsigned int i = 0; i < spanCount; i++)
            {
                auto def = m_ppTempDefinitions[i];
                float deltaSize = (maxMaxSize - def->GetSizeCache()) * sizeToDistribute / totalRemainingSize;
                def->UpdateEffectiveMinSize(def->GetSizeCache() + deltaSize);
            }
        }
        else
        {
            // If equi-size is greater or equal to the maximum of max sizes,
            // then all definitions receive equi-size as their min sizes.
            for (unsigned int i = 0; i < spanCount; i++)
            {
                m_ppTempDefinitions[i]->UpdateEffectiveMinSize(equallyDistributedSize);
            }
        }
    }
}

// Gets the union of the length types for a given range of definitions.
CellUnitTypes CGrid::GetLengthTypeForRange(
    _In_ const CDOCollection* const definitions,
    const unsigned int start,
    const unsigned int count) const
{
    ASSERT((count > 0) && ((start + count) <= definitions->GetCount()));

    CellUnitTypes unitTypes = CellUnitTypes::None;
    unsigned int index = start + count - 1;

    do
    {
        auto def = static_cast<CDefinitionBase*>((*definitions)[index]);
        switch (def->GetEffectiveUnitType())
        {
            case DirectUI::GridUnitType::Auto:
                unitTypes |= CellUnitTypes::Auto;
                break;
            case DirectUI::GridUnitType::Pixel:
                unitTypes |= CellUnitTypes::Pixel;
                break;
            case DirectUI::GridUnitType::Star:
                unitTypes |= CellUnitTypes::Star;
                break;
        }
    } while (index > 0 && --index >= start);

    return unitTypes;
}

// Accumulates available size information for a given range of definitions.
float CGrid::GetAvailableSizeForRange(
    _In_ const CDOCollection* const definitions,
    const unsigned int start,
    const unsigned int count,
    const float spacing) const
{
    ASSERT((count > 0) && ((start + count) <= definitions->GetCount()));

    float availableSize = 0.0f;
    unsigned int index = start + count - 1;

    do
    {
        auto def = static_cast<CDefinitionBase*>((*definitions)[index]);
        availableSize += (def->GetEffectiveUnitType() == DirectUI::GridUnitType::Auto)
                ? def->GetEffectiveMinSize()
                : def->GetMeasureArrangeSize();
    } while (index > 0 && --index >= start);

    availableSize += spacing * (count - 1);

    return availableSize;
}

// Accumulates final size information for a given range of definitions.
float CGrid::GetFinalSizeForRange(
    _In_ const CDOCollection* const definitions,
    const unsigned int start,
    const unsigned int count,
    const float spacing) const
{
    ASSERT((count > 0) && ((start + count) <= definitions->GetCount()));

    float finalSize = 0.0f;
    unsigned int index = start + count - 1;

    do
    {
        auto def = static_cast<CDefinitionBase*>((*definitions)[index]);
        finalSize += def->GetMeasureArrangeSize();
    } while (index > 0 && --index >= start);

    finalSize += spacing * (count - 1);

    return finalSize;
}

// Calculates the desired size of the Grid minus its BorderThickness and
// Padding assuming all the cells have already been measured.
float CGrid::GetDesiredInnerSize(_In_ const CDOCollection* const definitions) const
{
    float desiredSize = 0.0f;

    for (XUINT32 i = 0; i < definitions->GetCount(); ++i)
    {
        auto def = static_cast<CDefinitionBase*>((*definitions)[i]);
        desiredSize += def->GetEffectiveMinSize();
    }

    return desiredSize;
}

//------------------------------------------------------------------------
//
//  Method:   ResolveStar
//
//  Synopsis:  Resolves Star's for given array of definitions during measure pass
//
//------------------------------------------------------------------------
void CGrid::ResolveStar(
    _In_ CDefinitionCollectionBase* definitions, //the definitions collection
    XFLOAT availableSize //the total available size across this dimension
    )
{
    XUINT32 cStarDefinitions = 0;
    XFLOAT takenSize = 0.0f;
    XFLOAT effectiveAvailableSize = availableSize;

    EnsureTempDefinitionsStorage(definitions->GetCount());

    for(XUINT32 i=0; i < definitions->GetCount(); i++)
    {
        //if star definition, setup values for distribution calculation

        CDefinitionBase* pDef = static_cast<CDefinitionBase*>(definitions->GetItemImpl(i));

        if(pDef->GetEffectiveUnitType() == DirectUI::GridUnitType::Star)
        {
            m_ppTempDefinitions[cStarDefinitions++] = pDef;

            // Note that this user value is in star units and not pixel units,
            // and thus, there is no need to layout-round.
            XFLOAT starValue = pDef->GetUserSizeValue();

            if(starValue < REAL_EPSILON)
            {
                pDef->SetMeasureArrangeSize(0.0f);
                pDef->SetSizeCache(0.0f);
            }
            else
            {
                //clipping by a max to avoid overflow when all the star values are added up.
                starValue = MIN(starValue, GRID_STARVALUE_MAX);

                pDef->SetMeasureArrangeSize(starValue);

                // Note that this user value is used for a computation that is cached
                // and then used in the call to CGrid::DistributeStarSpace below for
                // further calculations where the final result is layout-rounded as
                // appropriate. In other words, it doesn't seem like we need to apply
                // layout-rounding just yet.
                XFLOAT maxSize = MIN(GRID_STARVALUE_MAX, MAX(pDef->GetEffectiveMinSize(), pDef->GetUserMaxSize()));
                pDef->SetSizeCache(maxSize / starValue);
            }
        }
        else
        {
            //if not star definition, reduce the size available to star definitions
            if(pDef->GetEffectiveUnitType() == DirectUI::GridUnitType::Pixel)
            {
                takenSize += pDef->GetMeasureArrangeSize();
            }
            else if(pDef->GetEffectiveUnitType() == DirectUI::GridUnitType::Auto)
            {
                takenSize += pDef->GetEffectiveMinSize();
            }
        }
    }

    if (GetUseLayoutRounding())
    {
        takenSize = LayoutRound(takenSize);

        // It is important to apply layout rounding to the available size too,
        // since we need to make sure that we are working with one or the other:
        // rounded values or non-rounded values only. Otherwise, the distribution
        // of star space will compute slightly different results for the star
        // definitions in the case were takenSize == 0 vs. takenSize != 0.
        effectiveAvailableSize = LayoutRound(effectiveAvailableSize);
    }

    DistributeStarSpace(m_ppTempDefinitions, cStarDefinitions, effectiveAvailableSize - takenSize, &takenSize);
}

//------------------------------------------------------------------------
//
//  Method:   DistributeStarSpace
//
//  Synopsis:  Distributes available space between star definitions.
//
//------------------------------------------------------------------------
void CGrid::DistributeStarSpace(
    _In_reads_(cStarDefinitions) CDefinitionBase** ppStarDefinitions,
    XUINT32 cStarDefinitions,
    XFLOAT availableSize,
    _Inout_ XFLOAT* pTotalResolvedSize)
{
    XFLOAT resolvedSize;
    XFLOAT starValue;
    XFLOAT totalStarResolvedSize = 0.0f;

    //sorting definitions for order of space allocation. definition with the lowest
    //maxSize to starValue ratio gets the size first.
    SortDefinitionsForStarSizeDistribution(ppStarDefinitions, cStarDefinitions);

    XFLOAT allStarWeights = 0.0f;
    XUINT32 i = cStarDefinitions ;

    while(i > 0)
    {
        i--;
        allStarWeights += ppStarDefinitions[i]->GetMeasureArrangeSize();
        //store partial sum of weights
        ppStarDefinitions[i]->SetSizeCache(allStarWeights);
    }

    i=0;
    while(i < cStarDefinitions)
    {
        resolvedSize = 0.0f;
        starValue = ppStarDefinitions[i]->GetMeasureArrangeSize();

        if(starValue == 0.0f)
        {
            resolvedSize = ppStarDefinitions[i]->GetEffectiveMinSize();
        }
        else
        {
            resolvedSize = MAX(availableSize - totalStarResolvedSize, 0.0f) * (starValue / ppStarDefinitions[i]->GetSizeCache());
            resolvedSize = MAX(ppStarDefinitions[i]->GetEffectiveMinSize(), MIN(resolvedSize, ppStarDefinitions[i]->GetUserMaxSize()));
        }

        if (GetUseLayoutRounding())
        {
            resolvedSize = LayoutRound(resolvedSize);
        }

        ppStarDefinitions[i]->SetMeasureArrangeSize(resolvedSize);
        totalStarResolvedSize += resolvedSize;

        i++;
    }
    *pTotalResolvedSize += totalStarResolvedSize;
}


//------------------------------------------------------------------------
//
//  Method:   EnsureTempDefinitionsStorage
//
//  Synopsis:  allocates memory for temporary definitions storage.
//
//------------------------------------------------------------------------
void CGrid::EnsureTempDefinitionsStorage(unsigned int minCount)
{
    if (m_ppTempDefinitions == nullptr || m_cTempDefinitions < minCount)
    {
        delete [] m_ppTempDefinitions;
        m_ppTempDefinitions = new CDefinitionBase*[minCount];
        m_cTempDefinitions = minCount;
    }
    memset(m_ppTempDefinitions, 0, minCount * sizeof(CDefinitionBase*));
}


//------------------------------------------------------------------------
//
//  Method:   CGrid::MeasureOverride
//
//  Synopsis:
//      Overriding CFrameworkElement virtual to add grid specific logic to measure pass
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CGrid::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    // Locking the row and columns definitions to prevent changes by user code
    // during the measure pass.
    LockDefinitions();
    auto scopeGuard = wil::scope_exit([&]
    {
        UnlockDefinitions();
    });

    const XSIZEF combinedThickness = CBorder::HelperGetCombinedThickness(this);
    XSIZEF innerAvailableSize = {
        availableSize.width - combinedThickness.width,
        availableSize.height - combinedThickness.height };

    desiredSize = { };

    if (IsWithoutRowAndColumnDefinitions())
    {
        // If this Grid has no user-defined rows or columns, it is possible
        // to shortcut this MeasureOverride.
        auto children = static_cast<CUIElementCollection*>(GetChildren());
        if (children)
        {
            for (auto& cdo : (*children))
            {
                auto currentChild = static_cast<CUIElement*>(cdo);
                ASSERT(currentChild);

                IFC_RETURN(currentChild->Measure(innerAvailableSize));
                IFC_RETURN(currentChild->EnsureLayoutStorage());

                XSIZEF& childDesiredSize = currentChild->GetLayoutStorage()->m_desiredSize;
                desiredSize.width = std::max(desiredSize.width, childDesiredSize.width);
                desiredSize.height = std::max(desiredSize.height, childDesiredSize.height);
            }
        }
    }
    else
    {
        if (HasGridFlags(GridFlags::DefinitionsChanged))
        {
            ClearGridFlags(GridFlags::DefinitionsChanged);
            IFC_RETURN(InitializeDefinitionStructure());
        }

        ValidateDefinitions(m_pRows, innerAvailableSize.height == std::numeric_limits<float>::infinity() /* treatStarAsAuto */);
        ValidateDefinitions(m_pColumns, innerAvailableSize.width == std::numeric_limits<float>::infinity() /* treatStarAsAuto */);

        const float rowSpacing = GetRowSpacing();
        const float columnSpacing = GetColumnSpacing();
        const float combinedRowSpacing = rowSpacing * (m_pRows->GetCount() - 1);
        const float combinedColumnSpacing = columnSpacing * (m_pColumns->GetCount() - 1);
        innerAvailableSize.width -= combinedColumnSpacing;
        innerAvailableSize.height -= combinedRowSpacing;

        auto children = GetUnsortedChildren();
        UINT32 childrenCount = children.GetCount();

        CellCacheStackVector cellCacheVector;
        CellGroups cellGroups = ValidateCells(children, cellCacheVector);

        // The group number of a cell indicates the order in which it will be
        // measured; a certain order is necessary to dynamically resolve star
        // definitions since there are cases when the topology of a Grid causes
        // cyclical dependencies. For example:
        //
        //
        //                         column width="Auto"      column width="*"
        //                      +----------------------+----------------------+
        //                      |                      |                      |
        //                      |                      |                      |
        //                      |                      |                      |
        //                      |                      |                      |
        //  row height="Auto"   |                      |      cell 1 2        |
        //                      |                      |                      |
        //                      |                      |                      |
        //                      |                      |                      |
        //                      |                      |                      |
        //                      +----------------------+----------------------+
        //                      |                      |                      |
        //                      |                      |                      |
        //                      |                      |                      |
        //                      |                      |                      |
        //  row height="*"      |       cell 2 1       |                      |
        //                      |                      |                      |
        //                      |                      |                      |
        //                      |                      |                      |
        //                      |                      |                      |
        //                      +----------------------+----------------------+
        //
        // In order to accurately calculate the constraining width for "cell 1 2"
        // (which corresponds to the remaining space of the grid's available width
        // and calculated value of the auto column), "cell 2 1" needs to be calculated
        // first, as it contributes to the calculated value of the auto column.
        // At the same time in order to accurately calculate the constraining
        // height for "cell 2 1", "cell 1 2" needs to be calculated first,
        // as it contributes to the calculated height of the auto row, which is used
        // in the computation of the resolved height of the star row.
        //
        // To break this cyclical dependency we are making the (arbitrary)
        // decision to treat cells like "cell 2 1" as if they were in auto
        // rows. We will recalculate them later once the heights of star rows
        // are resolved. In other words, the code below implements the
        // following logic:
        //
        //                       +---------+
        //                       |  enter  |
        //                       +---------+
        //                            |
        //                            V
        //                    +----------------+
        //                    | Measure Group1 |
        //                    +----------------+
        //                            |
        //                            V
        //                          / - \
        //                        /       \
        //                  Y   /    Can    \    N
        //            +--------|   Resolve   |-----------+
        //            |         \  StarsV?  /            |
        //            |           \       /              |
        //            |             \ - /                |
        //            V                                  V
        //    +----------------+                       / - \
        //    | Resolve StarsV |                     /       \
        //    +----------------+               Y   /    Can    \    N
        //            |                      +----|   Resolve   |------+
        //            V                      |     \  StarsU?  /       |
        //    +----------------+             |       \       /         |
        //    | Measure Group2 |             |         \ - /           |
        //    +----------------+             |                         V
        //            |                      |                 +-----------------+
        //            V                      |                 | Measure Group2' |
        //    +----------------+             |                 +-----------------+
        //    | Resolve StarsU |             |                         |
        //    +----------------+             V                         V
        //            |              +----------------+        +----------------+
        //            V              | Resolve StarsU |        | Resolve StarsU |
        //    +----------------+     +----------------+        +----------------+
        //    | Measure Group3 |             |                         |
        //    +----------------+             V                         V
        //            |              +----------------+        +----------------+
        //            |              | Measure Group3 |        | Measure Group3 |
        //            |              +----------------+        +----------------+
        //            |                      |                         |
        //            |                      V                         V
        //            |              +----------------+        +----------------+
        //            |              | Resolve StarsV |        | Resolve StarsV |
        //            |              +----------------+        +----------------+
        //            |                      |                         |
        //            |                      |                         V
        //            |                      |                +------------------+
        //            |                      |                | Measure Group2'' |
        //            |                      |                +------------------+
        //            |                      |                         |
        //            +----------------------+-------------------------+
        //                                   |
        //                                   V
        //                           +----------------+
        //                           | Measure Group4 |
        //                           +----------------+
        //                                   |
        //                                   V
        //                               +--------+
        //                               |  exit  |
        //                               +--------+
        //
        // Where:
        // *   StarsV = Stars in Rows
        // *   StarsU = Stars in Columns.
        // *   All [Measure GroupN] - regular children measure process -
        //     each cell is measured given contraint size as an input
        //     and each cell's desired size is accumulated on the
        //     corresponding column / row.
        // *   [Measure Group2'] - is when each cell is measured with
        //     infinite height as a constraint and a cell's desired
        //     height is ignored.
        // *   [Measure Groups''] - is when each cell is measured (second
        //     time during single Grid.MeasureOverride) regularly but its
        //     returned width is ignored.
        //
        // This algorithm is believed to be as close to ideal as possible.
        // It has the following drawbacks:
        // *   Cells belonging to Group2 could be measured twice.
        // *   Iff during the second measure, a cell belonging to Group2
        //     returns a desired width greater than desired width returned
        //     the first time, the cell is going to be clipped, even though
        //     it appears in an auto column.

        // Measure Group1. After Group1 is measured, only Group3 can have
        // cells belonging to auto rows.
        IFC_RETURN(MeasureCellsGroup(cellGroups.group1, childrenCount, rowSpacing, columnSpacing, FALSE, FALSE, cellCacheVector));

        // After Group1 is measured, only Group3 may have cells belonging to
        // Auto rows.
        if(!HasGridFlags(GridFlags::HasAutoRowsAndStarColumn))
        {
            // We have no cyclic dependency; resolve star row/auto column first.
            if (HasGridFlags(GridFlags::HasStarRows))
            {
                ResolveStar(m_pRows, innerAvailableSize.height);
            }

            // Measure Group2.
            IFC_RETURN(MeasureCellsGroup(cellGroups.group2, childrenCount, rowSpacing, columnSpacing, FALSE, FALSE, cellCacheVector));

            if (HasGridFlags(GridFlags::HasStarColumns))
            {
                ResolveStar(m_pColumns, innerAvailableSize.width);
            }

            // Measure Group3.
            IFC_RETURN(MeasureCellsGroup(cellGroups.group3, childrenCount, rowSpacing, columnSpacing, FALSE, FALSE, cellCacheVector));
        }
        else
        {
            // If at least one cell exists in Group2, it must be measured
            // before star columns can be resolved.
            if(cellGroups.group2 > childrenCount)
            {
                if (HasGridFlags(GridFlags::HasStarColumns))
                {
                    ResolveStar(m_pColumns, innerAvailableSize.width);
                }

                // Measure Group3.
                IFC_RETURN(MeasureCellsGroup(cellGroups.group3, childrenCount, rowSpacing, columnSpacing, FALSE, FALSE, cellCacheVector));

                if (HasGridFlags(GridFlags::HasStarRows))
                {
                    ResolveStar(m_pRows, innerAvailableSize.height);
                }
            }
            else
            {
                // We have a cyclic dependency; measure Group2 for their
                // widths, while setting the row heights to infinity.
                IFC_RETURN(MeasureCellsGroup(cellGroups.group2, childrenCount, rowSpacing, columnSpacing, FALSE, TRUE, cellCacheVector));

                if (HasGridFlags(GridFlags::HasStarColumns))
                {
                    ResolveStar(m_pColumns, innerAvailableSize.width);
                }

                // Measure Group3.
                IFC_RETURN(MeasureCellsGroup(cellGroups.group3, childrenCount, rowSpacing, columnSpacing, FALSE, FALSE, cellCacheVector));

                if (HasGridFlags(GridFlags::HasStarRows))
                {
                    ResolveStar(m_pRows, innerAvailableSize.height);
                }

                // Now, Measure Group2 again for their heights and ignore their widths.
                IFC_RETURN(MeasureCellsGroup(cellGroups.group2, childrenCount, rowSpacing, columnSpacing, TRUE, FALSE, cellCacheVector));
            }
        }

        // Finally, measure Group4.
        IFC_RETURN(MeasureCellsGroup(cellGroups.group4, childrenCount, rowSpacing, columnSpacing, FALSE, FALSE, cellCacheVector));

        desiredSize.width = GetDesiredInnerSize(m_pColumns) + combinedColumnSpacing;
        desiredSize.height = GetDesiredInnerSize(m_pRows) + combinedRowSpacing;
    }

    desiredSize.width += combinedThickness.width;
    desiredSize.height += combinedThickness.height;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CGrid::ArrangeOverride
//
//  Synopsis:
//      Overriding CFrameworkElement virtual to add grid specific logic to arrange pass
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CGrid::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)
{
    // Locking the row and columns definitions to prevent changes by user code
    // during the arrange pass.
    LockDefinitions();
    auto scopeGuard = wil::scope_exit([&]
    {
        delete[] m_ppTempDefinitions;
        m_ppTempDefinitions = nullptr;
        m_cTempDefinitions = 0;
        UnlockDefinitions();
    });

    XRECTF innerRect = CBorder::HelperGetInnerRect(this, finalSize);

    if (IsWithoutRowAndColumnDefinitions())
    {
        // If this Grid has no user-defined rows or columns, it is possible
        // to shortcut this ArrangeOverride.
        auto children = static_cast<CUIElementCollection*>(GetChildren());
        if (children)
        {
            for (auto& cdo : (*children))
            {
                auto currentChild = static_cast<CUIElement*>(cdo);
                ASSERT(currentChild);

                IFC_RETURN(currentChild->EnsureLayoutStorage());
                XSIZEF& childDesiredSize = currentChild->GetLayoutStorage()->m_desiredSize;
                innerRect.Width = std::max(innerRect.Width, childDesiredSize.width);
                innerRect.Height = std::max(innerRect.Height, childDesiredSize.height);
                IFC_RETURN(currentChild->Arrange(innerRect));
            }
        }
    }
    else
    {
        IFCEXPECT_RETURN(m_pRows && m_pColumns);

        const float rowSpacing = GetRowSpacing();
        const float columnSpacing = GetColumnSpacing();
        const float combinedRowSpacing = rowSpacing * (m_pRows->GetCount() - 1);
        const float combinedColumnSpacing = columnSpacing * (m_pColumns->GetCount() - 1);

        // Given an effective final size, compute the offsets and sizes of each
        // row and column, including the redistribution of Star sizes based on
        // the new width and height.
        SetFinalSize(m_pRows, innerRect.Height - combinedRowSpacing);
        SetFinalSize(m_pColumns, innerRect.Width - combinedColumnSpacing);

        auto children = GetUnsortedChildren();
        UINT32 count = children.GetCount();

        for (XUINT32 childIndex = 0; childIndex < count; childIndex++)
        {
            CUIElement* currentChild = children[childIndex];
            ASSERT(currentChild);

            CDefinitionBase* row = GetRowNoRef(currentChild);
            CDefinitionBase* column = GetColumnNoRef(currentChild);
            XUINT32 columnIndex = GetColumnIndex(currentChild);
            XUINT32 rowIndex = GetRowIndex(currentChild);

            XRECTF arrangeRect = { };
            arrangeRect.X = column->GetFinalOffset() + innerRect.X + (columnSpacing * columnIndex);
            arrangeRect.Y = row->GetFinalOffset() + innerRect.Y + (rowSpacing * rowIndex);
            arrangeRect.Width = GetFinalSizeForRange(m_pColumns, columnIndex, GetColumnSpan(currentChild), columnSpacing);
            arrangeRect.Height = GetFinalSizeForRange(m_pRows, rowIndex, GetRowSpan(currentChild), rowSpacing);

            IFC_RETURN(currentChild->Arrange(arrangeRect));
        }
    }

    newFinalSize = finalSize;

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CGrid::SetFinalSize
//
//  Synopsis:
//      Computes the offsets and sizes for each row and column
//
//------------------------------------------------------------------------
void CGrid::SetFinalSize(
    _In_ CDefinitionCollectionBase* definitions,
    XFLOAT finalSize
    )
{
    XFLOAT allPreferredArrangeSize = 0;
    CDefinitionBase* currDefinition = nullptr;
    CDefinitionBase* nextDefinition = nullptr;
    XUINT32 cStarDefinitions = 0;
    XUINT32 cNonStarDefinitions = definitions->GetCount();

    EnsureTempDefinitionsStorage(definitions->GetCount());

    for(XUINT32 i=0; i < definitions->GetCount(); i++)
    {
        CDefinitionBase* pDef = static_cast<CDefinitionBase*>(definitions->GetItemImpl(i));

        if (pDef->GetUserSizeType() == DirectUI::GridUnitType::Star)
        {
             //if star definition, setup values for distribution calculation

            m_ppTempDefinitions[cStarDefinitions++] = pDef;

            // Note that this user value is in star units and not pixel units,
            // and thus, there is no need to layout-round.
            XFLOAT starValue = pDef->GetUserSizeValue();

            if(starValue < REAL_EPSILON)
            {
                //cache normalized star value temporary into MeasureSize
                pDef->SetMeasureArrangeSize(0.0f);
                pDef->SetSizeCache(0.0f);
            }
            else
            {
                //clipping by a max to avoid overflow when all the star values are added up.
                starValue = MIN(starValue, GRID_STARVALUE_MAX);

                //cache normalized star value temporary into MeasureSize
                pDef->SetMeasureArrangeSize(starValue);

                // Note that this user value is used for a computation that is cached
                // and then used in the call to CGrid::DistributeStarSpace below for
                // further calculations where the final result is layout-rounded as
                // appropriate. In other words, it doesn't seem like we need to apply
                // layout-rounding just yet.
                XFLOAT maxSize = MIN(GRID_STARVALUE_MAX, MAX(pDef->GetEffectiveMinSize(), pDef->GetUserMaxSize()));
                pDef->SetSizeCache(maxSize / starValue);
            }
        }
        else
        {
            //if not star definition, reduce the size available to star definitions
            const bool useLayoutRounding = GetUseLayoutRounding();
            float userSize = 0.0f;
            float userMaxSize = useLayoutRounding
                ? LayoutRound(pDef->GetUserMaxSize())
                : pDef->GetUserMaxSize();

            m_ppTempDefinitions[--cNonStarDefinitions] = pDef;

            switch (pDef->GetUserSizeType())
            {
                case DirectUI::GridUnitType::Pixel:
                    userSize = useLayoutRounding
                        ? LayoutRound(pDef->GetUserSizeValue())
                        : pDef->GetUserSizeValue();
                    break;
                case DirectUI::GridUnitType::Auto:
                    userSize = pDef->GetEffectiveMinSize();
                    break;
            }

            pDef->SetMeasureArrangeSize(std::max(pDef->GetEffectiveMinSize(), std::min(userSize, userMaxSize)));
            allPreferredArrangeSize += pDef->GetMeasureArrangeSize();
        }
    }

    //distribute available space among star definitions.
    DistributeStarSpace(m_ppTempDefinitions, cStarDefinitions, finalSize - allPreferredArrangeSize, &allPreferredArrangeSize);

    //if the combined size of all definitions exceeds the finalSize, take the difference away.
    if((allPreferredArrangeSize > finalSize) && XcpAbsF(allPreferredArrangeSize - finalSize) > REAL_EPSILON)
    {
        //sort definitions to define an order for space distribution.
        SortDefinitionsForOverflowSizeDistribution(m_ppTempDefinitions, definitions->GetCount());
        XFLOAT sizeToDistribute = finalSize - allPreferredArrangeSize;

        for(XUINT32 i=0; i < definitions->GetCount(); i++)
        {
            XFLOAT finalMeasureArrangeSize = m_ppTempDefinitions[i]->GetMeasureArrangeSize() + (sizeToDistribute / (definitions->GetCount() - i));

            finalMeasureArrangeSize = MAX(finalMeasureArrangeSize, m_ppTempDefinitions[i]->GetEffectiveMinSize());
            finalMeasureArrangeSize = MIN(finalMeasureArrangeSize, m_ppTempDefinitions[i]->GetMeasureArrangeSize());
            sizeToDistribute -= (finalMeasureArrangeSize - m_ppTempDefinitions[i]->GetMeasureArrangeSize());
            m_ppTempDefinitions[i]->SetMeasureArrangeSize(finalMeasureArrangeSize);
        }
    }

    //Process definitions in original order to calculate offsets
    currDefinition = static_cast<CDefinitionBase*>(definitions->GetItemImpl(0));
    currDefinition->SetFinalOffset(0.0f);

    for(XUINT32 i=0; i<definitions->GetCount()-1; i++)
    {
        nextDefinition = static_cast<CDefinitionBase*>(definitions->GetItemImpl(i+1));
        nextDefinition->SetFinalOffset(currDefinition->GetFinalOffset() + currDefinition->GetMeasureArrangeSize());
        currDefinition = nextDefinition;
        nextDefinition = nullptr;
    }
}


//------------------------------------------------------------------------
//
//  Method:   SortDefinitionsForSpanPreferredDistribution
//
//  Synopsis: Sort definitions for span processing, for the case when the element
//                  desired Size is greater than rangeMinSize but less than rangePreferredSize.
//
//------------------------------------------------------------------------
void CGrid::SortDefinitionsForSpanPreferredDistribution(
    _In_reads_(cDefinitions) CDefinitionBase** ppDefinitions,
    XUINT32 cDefinitions
    )
{
    CDefinitionBase* pTemp;

    for (XUINT32 i = 1,j; i < cDefinitions; i++)
    {
        pTemp = ppDefinitions[i];
        for (j=i; j >0 ;j--)
        {
            if (pTemp->GetUserSizeType() == DirectUI::GridUnitType::Auto)
            {
                if (ppDefinitions[j - 1]->GetUserSizeType() == DirectUI::GridUnitType::Auto)
                {
                    if(pTemp->GetEffectiveMinSize() >= ppDefinitions[j-1]->GetEffectiveMinSize())
                    {
                        break;
                    }
                }
            }
            else
            {
                if (ppDefinitions[j - 1]->GetUserSizeType() != DirectUI::GridUnitType::Auto)
                {
                    if(pTemp->GetPreferredSize() >= ppDefinitions[j-1]->GetPreferredSize())
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }

            ppDefinitions[j] = ppDefinitions[j-1];
        }
        ppDefinitions[j] = pTemp;
    }
}


//------------------------------------------------------------------------
//
//  Method:   SortDefinitionsForSpanMaxSizeDistribution
//
//  Synopsis: Sort definitions for span processing, for the case when the element
//                  desired Size is greater than rangePreferredSize but less than rangeMaxSize.
//
//------------------------------------------------------------------------
void CGrid::SortDefinitionsForSpanMaxSizeDistribution(
    _In_reads_(cDefinitions)  CDefinitionBase** ppDefinitions,
    XUINT32 cDefinitions
    )
{
    CDefinitionBase* pTemp;

    for (XUINT32 i = 1,j; i < cDefinitions; i++)
    {
        pTemp = ppDefinitions[i];
        for (j=i; j >0 ;j--)
        {
            if (pTemp->GetUserSizeType() == DirectUI::GridUnitType::Auto)
            {
                if (ppDefinitions[j - 1]->GetUserSizeType() == DirectUI::GridUnitType::Auto)
                {
                    if(pTemp->GetSizeCache() >= ppDefinitions[j-1]->GetSizeCache())
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                if (ppDefinitions[j - 1]->GetUserSizeType() != DirectUI::GridUnitType::Auto)
                {
                    if(pTemp->GetSizeCache() >= ppDefinitions[j-1]->GetSizeCache())
                    {
                        break;
                    }
                }
            }

            ppDefinitions[j] = ppDefinitions[j-1];
        }
        ppDefinitions[j] = pTemp;
    }
}


//------------------------------------------------------------------------
//
//  Method: SortDefinitionsForOverflowSizeDistribution
//
//  Synopsis: Sort definitions for final size processing in ArrangeOverride, for the case
//                 when the combined size of all definitions across a dimension exceeds the
//                 finalSize in that dimension.
//
//------------------------------------------------------------------------
void CGrid::SortDefinitionsForOverflowSizeDistribution(
    _In_reads_(cDefinitions) CDefinitionBase** ppDefinitions,
    XUINT32 cDefinitions
    )
{
    CDefinitionBase* pTemp;

    // use insertion sort...it is stable...
    for (XUINT32 i = 1,j; i < cDefinitions; i++)
    {
        pTemp = ppDefinitions[i];
        for (j=i; j >0 ;j--)
        {
            if ((pTemp->GetMeasureArrangeSize() - pTemp->GetEffectiveMinSize())
                >= (ppDefinitions[j-1]->GetMeasureArrangeSize() - ppDefinitions[j-1]->GetEffectiveMinSize()))
            {
                break;
            }
            ppDefinitions[j] = ppDefinitions[j-1];
        }
        ppDefinitions[j] = pTemp;
    }
}


//------------------------------------------------------------------------
//
//  Method: SortDefinitionsForStarSizeDistribution
//
//  Synopsis: Sort definitions for distributing star space.
//
//------------------------------------------------------------------------
void
CGrid::SortDefinitionsForStarSizeDistribution(
    _In_reads_(cDefinitions) CDefinitionBase** ppDefinitions,
    XUINT32 cDefinitions
    )
{
    CDefinitionBase* pTemp;

    // use insertion sort...it is stable...
    for (XUINT32 i = 1,j; i < cDefinitions; i++)
    {
        pTemp = ppDefinitions[i];
        for (j=i; j >0 ;j--)
        {
            // Use >= instead of > to keep sort stable. If > is used,
            // sort will not be stable & size will distributed in a different
            // order than WPF.
            if (pTemp->GetSizeCache() >= ppDefinitions[j-1]->GetSizeCache())
            {
                break;
            }
            ppDefinitions[j] = ppDefinitions[j-1];
        }
        ppDefinitions[j] = pTemp;
    }
}

_Check_return_ HRESULT CGrid::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CPanel::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::Grid_RowDefinitions:
        case KnownPropertyIndex::Grid_ColumnDefinitions:
        {
           InvalidateDefinitions();
           break;
        }
    }
    RRETURN(S_OK);
}

xref_ptr<CBrush> CGrid::GetBorderBrush() const
{
    if(!IsPropertyDefaultByIndex(KnownPropertyIndex::Grid_BorderBrush))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::Grid_BorderBrush, &result));
        return static_sp_cast<CBrush>(result.DetachObject());
    }
    else
    {
        return CPanel::GetBorderBrush();
    }
}

XTHICKNESS CGrid::GetBorderThickness() const
{
    if(!IsPropertyDefaultByIndex(KnownPropertyIndex::Grid_BorderThickness))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::Grid_BorderThickness, &result));
        return *(result.AsThickness());
    }
    else
    {
        return CPanel::GetBorderThickness();
    }
}

XTHICKNESS CGrid::GetPadding() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Grid_Padding, &result));
    return *(result.AsThickness());
}

XCORNERRADIUS CGrid::GetCornerRadius() const
{
    if(!IsPropertyDefaultByIndex(KnownPropertyIndex::Grid_CornerRadius))
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::Grid_CornerRadius, &result));
        return *(result.AsCornerRadius());
    }
    else
    {
        return CPanel::GetCornerRadius();
    }
}

DirectUI::BackgroundSizing CGrid::GetBackgroundSizing() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Grid_BackgroundSizing, &result));
    return static_cast<DirectUI::BackgroundSizing>(result.AsEnum());
}

float CGrid::GetRowSpacing() const
{
    CValue result;
    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::Grid_RowSpacing, &result));
    return result.As<valueFloat>();
}

float CGrid::GetColumnSpacing() const
{
    CValue result;
    IFCFAILFAST(GetValueByIndex(KnownPropertyIndex::Grid_ColumnSpacing, &result));
    return result.As<valueFloat>();
}

bool CGrid::IsWithoutRowAndColumnDefinitions() const
{
    return (m_pRowDefinitions == nullptr || m_pRowDefinitions->size() == 0) && (m_pColumnDefinitions == nullptr || m_pColumnDefinitions->size() == 0);
}
