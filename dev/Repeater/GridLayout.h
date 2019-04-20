// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NonVirtualizingLayout.h"
#include "GridLayout.g.h"
#include "GridLayout.properties.h"

class GridLayout :
    public ReferenceTracker<GridLayout, winrt::implementation::GridLayoutT, NonVirtualizingLayout>,
    public GridLayoutProperties
{
public:
    GridLayout();

    winrt::IVector<winrt::GridTrackInfo> TemplateColumns();
    void TemplateColumns(winrt::IVector<winrt::GridTrackInfo> const& value);

    winrt::IVector<winrt::GridTrackInfo> TemplateRows();
    void TemplateRows(winrt::IVector<winrt::GridTrackInfo> const& value);

    double ColumnGap();
    void ColumnGap(double const& value);

    double RowGap();
    void RowGap(double const& value);

    winrt::GridJustifyItems JustifyItems();
    void JustifyItems(winrt::GridJustifyItems const& value);

    winrt::GridAlignItems AlignItems();
    void AlignItems(winrt::GridAlignItems const& value);

    winrt::GridJustifyContent JustifyContent();
    void JustifyContent(winrt::GridJustifyContent const& value);

    winrt::GridAlignContent AlignContent();
    void AlignContent(winrt::GridAlignContent const& value);

    winrt::IVector<winrt::GridTrackInfo> AutoColumns();
    void AutoColumns(winrt::IVector<winrt::GridTrackInfo> const& value);

    winrt::IVector<winrt::GridTrackInfo> AutoRows();
    void AutoRows(winrt::IVector<winrt::GridTrackInfo> const& value);

    winrt::GridAutoFlow AutoFlow();
    void AutoFlow(winrt::GridAutoFlow const& value);

#pragma region INonVirtualizingLayoutOverrides
    void InitializeForContextCore(winrt::LayoutContext const& context);
    void UninitializeForContextCore(winrt::LayoutContext const& context);

    winrt::Size MeasureOverride(winrt::LayoutContext const& context, winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::LayoutContext const& context, winrt::Size const& finalSize);
#pragma endregion

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    winrt::IVector<winrt::GridTrackInfo> m_templateColumns;
    winrt::IVector<winrt::GridTrackInfo> m_templateRows;
    double m_columnGap {};
    double m_rowGap {};
    winrt::GridJustifyItems m_justifyItems { winrt::GridJustifyItems::Start };
    winrt::GridAlignItems m_alignItems { winrt::GridAlignItems::Start };
    winrt::GridJustifyContent m_justifyContent { winrt::GridJustifyContent::Start };
    winrt::GridAlignContent m_alignContent { winrt::GridAlignContent::Start };
    winrt::IVector<winrt::GridTrackInfo> m_autoColumns;
    winrt::IVector<winrt::GridTrackInfo> m_autoRows;
    winrt::GridAutoFlow m_autoFlow { winrt::GridAutoFlow::Row };

    // PORT_TODO
    winrt::GridLocation GetColumnStart(winrt::UIElement const& element) { return nullptr; }
    winrt::GridLocation GetRowStart(winrt::UIElement const& element) { return nullptr; }
    winrt::GridLocation GetColumnEnd(winrt::UIElement const& element) { return nullptr; }
    winrt::GridLocation GetRowEnd(winrt::UIElement const& element) { return nullptr; }

    struct ResolvedGridReference
    {
    public:
        ResolvedGridReference() :
            ResolvedGridReference(-1, nullptr)
        {
        }
        ResolvedGridReference(int index, winrt::GridTrackInfo info) :
            Index(index),
            Info(info)
        {
        }

        bool IsValid();

        static ResolvedGridReference Invalid();

        int Index {-1};
        winrt::GridTrackInfo Info;
    };

    // Calculated info on one of the grid tracks, used to carry over calculations from Measure to Arrange
    // PORT_TODO: Should be a struct
    class MeasuredGridTrackInfo
    {
    public:
        float Size {};
        float Start {};
    };
    // TODO: Move to a State object
    std::unique_ptr<std::map<int, MeasuredGridTrackInfo>> m_columns;
    std::unique_ptr<std::map<int, MeasuredGridTrackInfo>> m_rows;

    // Tracks all the intermediate calculations of one direction (row or column) of the grid
    struct AxisInfo
    {
    public:
        AxisInfo(winrt::IVector<winrt::GridTrackInfo> templates, winrt::IVector<winrt::GridTrackInfo> autos, std::map<int, MeasuredGridTrackInfo>* calculated);

        std::vector<winrt::GridTrackInfo> Template;
        std::vector<winrt::GridTrackInfo> Auto;
        std::map<int, MeasuredGridTrackInfo>* Calculated;

        float Available {};
        float Remaining {};
        float TotalFixed {};
        float TotalFraction {};
        unsigned int TotalAutos {};
        float Gap {};

        int EnsureIndexAvailable(int index, bool clampIfOutOfBounds = true);
        ResolvedGridReference GetTrack(winrt::GridLocation const& location, ResolvedGridReference* previous = nullptr, bool allowOutOfRange = true);
        MeasuredGridTrackInfo GetMeasuredTrack(int index) const;
        MeasuredGridTrackInfo GetMeasuredTrackSafe(ResolvedGridReference track) const;
        void AddCalculated(int index, winrt::GridTrackInfo track, float size);
    };

    struct ChildGridLocations
    {
    public:
        ChildGridLocations() { }

        ResolvedGridReference ColStart {};
        ResolvedGridReference RowStart {};
        ResolvedGridReference ColEnd {};
        ResolvedGridReference RowEnd {};
    };

    struct GridCellIndex
    {
    public:
        int ColumnIndex {};
        int RowIndex {};

        bool operator <(const GridCellIndex& rhs) const
        {
            return
                (ColumnIndex < rhs.ColumnIndex) ||
                (RowIndex < rhs.RowIndex);
        }
    };

    void MarkOccupied(ChildGridLocations childLocation, std::map<GridCellIndex, bool> & occupied);
    static AxisInfo InitializeMeasure(winrt::IVector<winrt::GridTrackInfo> const& templates, winrt::IVector<winrt::GridTrackInfo> const& autos, std::map<int, GridLayout::MeasuredGridTrackInfo>* calculated, float gap, float available);
    static void ProcessFixedSizes(AxisInfo & measure);
    ChildGridLocations GetChildGridLocations(winrt::UIElement const& child, std::map<winrt::UIElement, ChildGridLocations> const& cache);
    bool TryGetChildGridLocations(winrt::UIElement const& child, AxisInfo & horizontal, AxisInfo & vertical, ChildGridLocations* outResult);
    bool ProcessAutoSizes(AxisInfo & measureHorizontal, AxisInfo & measureVertical, std::map<winrt::UIElement, ChildGridLocations> const& locationCache, winrt::NonVirtualizingLayoutContext const& context);
    static void UpdateAutoBasedOnMeasured(std::vector<winrt::GridTrackInfo> const& tracks, AxisInfo & measure, float childDesired);
    static bool ProcessFractionalSizes(AxisInfo & measure);
    static void ProcessAutoRemainingSize(AxisInfo & measure);
    float ProcessOffsets(AxisInfo & measure);
    void TraverseByColumn(AxisInfo & horizontal, AxisInfo & vertical, std::function<bool(GridCellIndex cell, AxisInfo & horizontal, AxisInfo & vertical)> predicate);
    void TraverseByRow(AxisInfo & horizontal, AxisInfo & vertical, std::function<bool(GridCellIndex cell, AxisInfo & horizontal, AxisInfo & vertical)> predicate);
    ChildGridLocations AssignUnoccupiedGridLocation(winrt::UIElement child, AxisInfo & horizontal, AxisInfo & vertical, winrt::GridAutoFlow autoFlow, std::map<GridCellIndex, bool> occupied);
    std::map<winrt::UIElement, ChildGridLocations> ResolveGridLocations(AxisInfo & horizontal, AxisInfo & vertical, winrt::NonVirtualizingLayoutContext const& context);
    static winrt::GridJustifyItems Convert(winrt::GridJustifySelf const& value);
    static winrt::GridAlignItems Convert(winrt::GridAlignSelf const& value);
};
