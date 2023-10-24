// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define GRID_STARVALUE_MAX  (XFLOAT_MAX/XUINT32_MAX -1)

class CColumnDefinitionCollection;
class CDefinitionBase;
class CDefinitionCollectionBase;
class CDOCollection;
class CRowDefinitionCollection;

#include "panel.h"
#include "GridCells.h"

enum class GridFlags : uint8_t
{
    None = 0x00,
    HasStarRows = 0x01,
    HasStarColumns = 0x02,
    HasAutoRowsAndStarColumn = 0x04,
    DefinitionsChanged = 0x08,
};
DEFINE_ENUM_FLAG_OPERATORS(GridFlags);

class CGrid : public CPanel
{
private:
    // Stores RowSpan or ColumnSpan information.
    struct SpanStoreEntry
    {
        SpanStoreEntry(unsigned int spanStart, unsigned int spanCount, float desiredSize, bool isColumnDefinition) :
            m_spanStart(spanStart), m_spanCount(spanCount), m_desiredSize(desiredSize), m_isColumnDefinition(isColumnDefinition) {};

        // Starting index of the cell.
        unsigned int m_spanStart;

        // Span value of the cell.
        unsigned int m_spanCount;

        // DesiredSize of the element in the cell.
        float m_desiredSize;

        bool m_isColumnDefinition;
    };

    static constexpr size_t c_spanStoreStackVectorSize = 16;
    static constexpr size_t c_cellCacheStackVectorSize = 16;

    typedef Jupiter::stack_vector<CellCache, c_cellCacheStackVectorSize> CellCacheStackVector;
    typedef Jupiter::stack_vector<SpanStoreEntry, c_spanStoreStackVectorSize> SpanStoreStackVector;

    _Check_return_ HRESULT InitializeDefinitionStructure();

    void ValidateDefinitions(
        _In_ const CDefinitionCollectionBase* const definitions,
        const bool treatStarAsAuto);

    CellGroups ValidateCells(
        _In_ const CUIElementCollectionWrapper& children,
        _Inout_ CellCacheStackVector& cellCacheVector);

    unsigned int GetRowIndex(_In_ const CUIElement* const child) const;

    unsigned int GetColumnIndex(_In_ const CUIElement* const child) const;

    unsigned int GetRowSpan(_In_ const CUIElement* const child) const;

    unsigned int GetColumnSpan(_In_ const CUIElement* const child) const;

    CDefinitionBase* GetRowNoRef(_In_ CUIElement* pChild);

    CDefinitionBase* GetColumnNoRef(_In_ CUIElement* pChild);

    void SetGridFlags(const GridFlags mask)
    {
        m_gridFlags |= mask;
    }

    void ClearGridFlags(const GridFlags mask)
    {
        m_gridFlags &= ~mask;
    }

    bool HasGridFlags(const GridFlags mask) const
    {
        return (m_gridFlags & mask) == mask;
    }

    void LockDefinitions();

    void UnlockDefinitions();

    _Check_return_ HRESULT MeasureCellsGroup(
        unsigned int cellsHead,
        unsigned int cellCount,
        const float rowSpacing,
        const float columnSpacing,
        const bool ignoreColumnDesiredSize,
        const bool forceRowToInfinity,
        _Inout_ CellCacheStackVector& cellCacheVector);

    _Check_return_ HRESULT MeasureCell(
        _In_ CUIElement* const child,
        const CellUnitTypes rowHeightTypes,
        const CellUnitTypes columnWidthTypes,
        const bool forceRowToInfinity,
        const float rowSpacing,
        const float columnSpacing);

    CellUnitTypes GetLengthTypeForRange(
        _In_ const CDOCollection* const definitions,
        const unsigned int start,
        const unsigned int count) const;

    float GetAvailableSizeForRange(
        _In_ const CDOCollection* const definitions,
        const unsigned int start,
        const unsigned int count,
        const float spacing) const;

    float GetFinalSizeForRange(
        _In_ const CDOCollection* const definitions,
        const unsigned int start,
        const unsigned int count,
        const float spacing) const;

    float GetDesiredInnerSize(_In_ const CDOCollection* const definitions) const;

    void SortDefinitionsForOverflowSizeDistribution(
        _In_reads_(cDefinitions) CDefinitionBase** ppDefinitions,
        XUINT32 cDefinitions);

    void SortDefinitionsForStarSizeDistribution(
        _In_reads_(cDefinitions) CDefinitionBase** ppDefinitions,
        XUINT32 cDefinitions);

    void SortDefinitionsForSpanPreferredDistribution(
        _In_reads_(cDefinitions) CDefinitionBase** ppDefinitions,
        XUINT32 cDefinitions);

    void SortDefinitionsForSpanMaxSizeDistribution(
        _In_reads_(cDefinitions) CDefinitionBase** ppDefinitions,
        XUINT32 cDefinitions);

    void SetFinalSize(
        _In_ CDefinitionCollectionBase* definitions,
        XFLOAT finalSize);

    void ResolveStar(
        _In_ CDefinitionCollectionBase* definitions,
        XFLOAT availableSize);

    void EnsureTempDefinitionsStorage(unsigned int minCount);

    void DistributeStarSpace(
        _In_reads_(cStarDefinitions) CDefinitionBase** ppStarDefinitions,
        XUINT32 cStarDefinitions,
        XFLOAT availableSize,
        _Inout_ XFLOAT *pTotalResolvedSize);

    void RegisterSpan(
        _Inout_ SpanStoreStackVector& spanStore,
        const unsigned int spanStart,
        const unsigned int spanCount,
        const float desiredtSize,
        const bool isColumnDefinition);

    void EnsureMinSizeInDefinitionRange(
        _In_ CDOCollection* definitions,
        const unsigned int spanStart,
        const unsigned int spanCount,
        const float spacing,
        const float childDesiredSize);

    bool IsWithoutRowAndColumnDefinitions() const;

    xref_ptr<CBrush> GetBorderBrush() const final;

    XTHICKNESS GetBorderThickness() const final;

    XTHICKNESS GetPadding() const final;

    XCORNERRADIUS GetCornerRadius() const final;

    DirectUI::BackgroundSizing GetBackgroundSizing() const final;

    float GetRowSpacing() const;

    float GetColumnSpacing() const;

protected:
    CGrid(_In_ CCoreServices *pCore)
        : CPanel(pCore)
    {
    }

    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) final;

    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) override;

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) final;

public:
    ~CGrid() override;

    DECLARE_CREATE(CGrid);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CGrid>::Index;
    }

    void InvalidateDefinitions()
    {
        SetGridFlags(GridFlags::DefinitionsChanged);
    }

public:
    CRowDefinitionCollection* m_pRowDefinitions = nullptr;          // User specified rows.
    CColumnDefinitionCollection* m_pColumnDefinitions = nullptr;    // User specified columns.

private:
    CRowDefinitionCollection* m_pRows = nullptr;                    // Effective row collection.
    CColumnDefinitionCollection* m_pColumns = nullptr;              // Effective column collection.

                                                                    // This is a temporary storage that is released after arrange.
    // Note the ScopeExit in ArrangeOveride
    CDefinitionBase** m_ppTempDefinitions = nullptr;                // Temporary definitions storage.
    XUINT32 m_cTempDefinitions = 0;                                 // Size in elements of temporary definitions storage


    GridFlags m_gridFlags = GridFlags::None;                        // Internal grid flags used for layout processing. Should have enough bits to fit all flags set by SetGridFlag.
};
