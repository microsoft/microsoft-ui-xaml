// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "GridLayout.h"
#include "RuntimeProfiler.h"
#include "Vector.h"
#include "GridLayoutState.h"

static winrt::GridTrackInfo s_lastInTrack;

GridLayout::GridLayout()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_GridLayout);

    m_templateColumns = winrt::make<Vector<winrt::GridTrackInfo>>();
    m_templateRows = winrt::make<Vector<winrt::GridTrackInfo>>();
    m_autoColumns = winrt::make<Vector<winrt::GridTrackInfo>>();
    m_autoRows = winrt::make<Vector<winrt::GridTrackInfo>>();

    // TODO: Initialize static
    if (s_lastInTrack != nullptr)
    {
        s_lastInTrack = winrt::GridTrackInfo();
    }
}

winrt::IVector<winrt::GridTrackInfo> GridLayout::TemplateColumns()
{
    return m_templateColumns;
}

void GridLayout::TemplateColumns(winrt::IVector<winrt::GridTrackInfo> const& value)
{
    m_templateColumns = value;
}

winrt::IVector<winrt::GridTrackInfo> GridLayout::TemplateRows()
{
    return m_templateRows;
}

void GridLayout::TemplateRows(winrt::IVector<winrt::GridTrackInfo> const& value)
{
    m_templateRows = value;
}

double GridLayout::ColumnGap()
{
    return m_columnGap;
}

void GridLayout::ColumnGap(double const& value)
{
    m_columnGap = value;
}

double GridLayout::RowGap()
{
    return m_rowGap;
}

void GridLayout::RowGap(double const& value)
{
    m_rowGap = value;
}

winrt::GridJustifyItems GridLayout::JustifyItems()
{
    return m_justifyItems;
}

void GridLayout::JustifyItems(winrt::GridJustifyItems const& value)
{
    m_justifyItems = value;
}

winrt::GridAlignItems GridLayout::AlignItems()
{
    return m_alignItems;
}

void GridLayout::AlignItems(winrt::GridAlignItems const& value)
{
    m_alignItems = value;
}

winrt::GridJustifyContent GridLayout::JustifyContent()
{
    return m_justifyContent;
}

void GridLayout::JustifyContent(winrt::GridJustifyContent const& value)
{
    m_justifyContent = value;
}

winrt::GridAlignContent GridLayout::AlignContent()
{
    return m_alignContent;
}

void GridLayout::AlignContent(winrt::GridAlignContent const& value)
{
    m_alignContent = value;
}

winrt::IVector<winrt::GridTrackInfo> GridLayout::AutoColumns()
{
    return m_autoColumns;
}

void GridLayout::AutoColumns(winrt::IVector<winrt::GridTrackInfo> const& value)
{
    m_autoColumns = value;
}

winrt::IVector<winrt::GridTrackInfo> GridLayout::AutoRows()
{
    return m_autoRows;
}

void GridLayout::AutoRows(winrt::IVector<winrt::GridTrackInfo> const& value)
{
    m_autoRows = value;
}

winrt::GridAutoFlow GridLayout::AutoFlow()
{
    return m_autoFlow;
}

void GridLayout::AutoFlow(winrt::GridAutoFlow const& value)
{
    m_autoFlow = value;
}


bool GridLayout::ResolvedGridReference::IsValid()
{
    return (Index != -1);
}

GridLayout::ResolvedGridReference GridLayout::ResolvedGridReference::Invalid()
{
    return GridLayout::ResolvedGridReference();
}

GridLayout::AxisInfo::AxisInfo(winrt::IVector<winrt::GridTrackInfo> templates, winrt::IVector<winrt::GridTrackInfo> autos, std::map<int, GridLayoutState::MeasuredGridTrackInfo>* calculated)
{
    // Add all the items from the markup defined template, plus one more grid line for 
    // the end (unless we have auto tracks, in which case they will define what happens 
    // when we go out of bounds)
    for (auto item : templates)
    {
        Template.push_back(item);
    }
    if (autos.Size() == 0)
    {
        Template.push_back(s_lastInTrack);
    }

    for (auto item : autos)
    {
        Auto.push_back(item);
    }

    Calculated = calculated;

    Available = 0.0f;
    Remaining = 0.0f;
    TotalFixed = 0.0f;
    TotalFraction = 0.0f;
    TotalAutos = 0;
    Gap = 0.0f;
}

int GridLayout::AxisInfo::EnsureIndexAvailable(int index, bool clampIfOutOfBounds)
{
    assert(index >= 0);

    if (static_cast<unsigned int>(index) < Template.size())
    {
        return index;
    }

    if (Auto.size() == 0u)
    {
        if (clampIfOutOfBounds)
        {
            // Clamp to the known set of indices
            return Template.size() - 1;
        }
        else
        {
            // Or if that's disallowed, return an invalid index
            return -1;
        }
    }

    // Grow the list of Templates to include this new index
    // FUTURE: Filling each of these in is obviously not the best for virtualization
    while (Template.size() <= static_cast<unsigned int>(index))
    {
        for (auto const& track : Auto)
        {
            //DumpBegin($"Adding auto track at index {Template.Count}");
            //DumpGridTrackInfo(track);
            //DumpEnd();
            Template.push_back(track);
        }
    }

    return index;
}

GridLayout::ResolvedGridReference GridLayout::AxisInfo::GetTrack(winrt::GridLocation const& location, GridLayout::ResolvedGridReference *previous, bool allowOutOfRange)
{
    if (location != nullptr)
    {
        // Exact track index
        int index = location.Index();
        if (index >= 0)
        {
            index = EnsureIndexAvailable(index, allowOutOfRange);
            if (index >= 0)
            {
                return GridLayout::ResolvedGridReference(index, Template[index]);
            }
        }

        // Friendly track name
        // PORT_TODO
        //if (!String.IsNullOrEmpty(location.LineName))
        {
            for (unsigned int i = 0u; i < Template.size(); i++)
            {
                winrt::GridTrackInfo track = Template[i];
                if (track.LineName() == location.LineName())
                {
                    return GridLayout::ResolvedGridReference(i, track);
                }
            }
        }
    }

    // Span relative to previous track
    if (previous != nullptr)
    {
        // By default go 1 beyond the previous one
        int span = 1;
        if ((location != nullptr) && (location.Span() > 0))
        {
            span = location.Span();
        }

        int previousIndex = previous->Index;
        if (previousIndex >= 0)
        {
            int spanIndex = previousIndex + span;
            spanIndex = EnsureIndexAvailable(spanIndex, allowOutOfRange);
            if (spanIndex >= 0)
            {
                return ResolvedGridReference(spanIndex, Template[spanIndex]);
            }
        }
    }

    return GridLayout::ResolvedGridReference::Invalid();
}

GridLayoutState::MeasuredGridTrackInfo GridLayout::AxisInfo::GetMeasuredTrack(int index) const
{
    auto result = Calculated->find(index);
    return result->second;
}

GridLayoutState::MeasuredGridTrackInfo GridLayout::AxisInfo::GetMeasuredTrackSafe(GridLayout::ResolvedGridReference track) const
{
    if (!track.IsValid())
    {
        // TODO: Is this a programming error?
        return GridLayoutState::MeasuredGridTrackInfo();
    }

    auto result = Calculated->find(track.Index);
    if (result != Calculated->end())
    {
        return result->second;
    }

    // TODO: Is this a programming error?
    return GridLayoutState::MeasuredGridTrackInfo();
}

void GridLayout::AxisInfo::AddCalculated(int index, winrt::GridTrackInfo track, float size)
{
    Calculated->insert(std::make_pair(index, GridLayoutState::MeasuredGridTrackInfo{ size, 0.0f }));
}

void GridLayout::MarkOccupied(GridLayout::ChildGridLocations childLocation, std::map<GridLayout::GridCellIndex, bool> & occupied)
{
    if (!childLocation.ColStart.IsValid() || !childLocation.RowStart.IsValid())
    {
        return;
    }

    for (int column = childLocation.ColStart.Index; column < childLocation.ColEnd.Index; column++)
    {
        for (int row = childLocation.RowStart.Index; row < childLocation.RowEnd.Index; row++)
        {
            //DumpInfo($"Mark occupied {{{column},{row}}}");
            // PORT_TODO
            auto index = GridLayout::GridCellIndex{ column, row };
            occupied[index] = true;
        }
    }
}

GridLayout::AxisInfo GridLayout::InitializeMeasure(winrt::IVector<winrt::GridTrackInfo> const& templates, winrt::IVector<winrt::GridTrackInfo> const& autos, std::map<int, GridLayoutState::MeasuredGridTrackInfo>* calculated, float gap, float available)
{
    int numberOfGaps = (templates.Size() - 1);
    if ((gap > 0.0f) && (numberOfGaps > 0))
    {
        available -= (gap * numberOfGaps);
    }

    GridLayout::AxisInfo measure = GridLayout::AxisInfo(templates, autos, calculated);
    measure.Available = available;
    measure.Gap = gap;

    return measure;
}

void GridLayout::ProcessFixedSizes(GridLayout::AxisInfo & measure)
{
    for (unsigned int i = 0u; i < measure.Template.size(); i++)
    {
        winrt::GridTrackInfo track = measure.Template[i];

        float fixedSize = static_cast<float>(track.Length());

        // Percentage is effectively a fixed size in that it needs to be applied before any
        // of the more relative sizes (fraction, auto, etc.)
        if ((fixedSize == 0.0f) && (track.Percentage() != 0.0f))
        {
            fixedSize = (static_cast<float>(track.Percentage()) * measure.Available);
        }

        measure.TotalFixed += fixedSize;

        // Accumulate the fractional sizes now so we know how many pieces of pie to dish out
        measure.TotalFraction += static_cast<float>(track.Fraction());

        if (track.Auto())
        {
            measure.TotalAutos++;
        }

        measure.AddCalculated(i, track, fixedSize);
    }

    measure.Remaining = std::max(measure.Available - measure.TotalFixed, 0.0f);
}

GridLayout::ChildGridLocations GridLayout::GetChildGridLocations(winrt::UIElement const& child, std::map<winrt::UIElement, GridLayout::ChildGridLocations> const& cache)
{
    auto result = cache.find(child);
    if (result != end(cache))
    {
        return result->second;
    }

    assert("All children should be processed into the cache before using this method");
    return GridLayout::ChildGridLocations();
}

bool GridLayout::TryGetChildGridLocations(winrt::UIElement const& child, GridLayout::AxisInfo & horizontal, GridLayout::AxisInfo & vertical, GridLayout::ChildGridLocations* outResult)
{
    GridLayout::ChildGridLocations result;

    // Read preferences off the child
    winrt::GridLocation colStart = GetColumnStart(child);
    winrt::GridLocation rowStart = GetRowStart(child);

    // We need a starting point in order to resolve the location. Save this item for the
    // second pass (AutoFlow)
    if ((colStart == nullptr) || (rowStart == nullptr))
    {
        return false;
    }

    winrt::GridLocation colEnd = GetColumnEnd(child);
    winrt::GridLocation rowEnd = GetRowEnd(child);

    // Map the preferences to actual grid lines
    result.ColStart = horizontal.GetTrack(colStart);
    result.RowStart = vertical.GetTrack(rowStart);
    result.ColEnd = horizontal.GetTrack(colEnd, &result.ColStart);
    result.RowEnd = vertical.GetTrack(rowEnd, &result.RowStart);

    *outResult = result;
    return true;
}

bool GridLayout::ProcessAutoSizes(GridLayout::AxisInfo & measureHorizontal, GridLayout::AxisInfo & measureVertical, std::map<winrt::UIElement, GridLayout::ChildGridLocations> const& locationCache, winrt::NonVirtualizingLayoutContext const& context)
{
    if ((measureHorizontal.TotalAutos == 0) && (measureVertical.TotalAutos == 0))
    {
        return false;
    }

    for (winrt::UIElement const& child : context.try_as<winrt::NonVirtualizingLayoutContext>().Children())
    {
        ChildGridLocations childLocation = GetChildGridLocations(child, locationCache);

        std::vector<winrt::GridTrackInfo> autoHorizontal;
        std::vector<winrt::GridTrackInfo> autoVertical;

        auto getAutoTracks = [](GridLayout::ResolvedGridReference start, GridLayout::ResolvedGridReference end, GridLayout::AxisInfo & measure, std::vector<winrt::GridTrackInfo> & autoTracks)
        {
            if (!start.IsValid() || !end.IsValid())
            {
                return false;
            }

            int startIndex = start.Index;
            int endIndex = end.Index;

            for (int i = startIndex; i <= endIndex; i++)
            {
                auto track = measure.Template[i];
                if (track.Auto())
                {
                    autoTracks.push_back(track);
                }
            }
            return (autoTracks.size() > 0);
        };

        bool affectsHorizontalAuto = getAutoTracks(childLocation.ColStart, childLocation.ColEnd, measureHorizontal, autoHorizontal);
        bool affectsVerticalAuto = getAutoTracks(childLocation.RowStart, childLocation.RowEnd, measureVertical, autoVertical);

        // If none of the grid tracks are Auto then we can skip this item
        if (!affectsHorizontalAuto && !affectsVerticalAuto)
        {
            continue;
        }

        // Measure this child to see how much of the available space they would take.
        // Note that even if there are multiple Autos we don't attempt to preemptively split 
        // the space between them. They are all given a crack at being greedy.
        winrt::Size measureSize = winrt::Size{ measureHorizontal.Remaining, measureVertical.Remaining };
        child.Measure(measureSize);
        //DumpInfo($"Child inside Auto range measured to {child.DesiredSize}");

        // Update that row/column with the dimensions
        UpdateAutoBasedOnMeasured(autoHorizontal, measureHorizontal, child.DesiredSize().Width);
        UpdateAutoBasedOnMeasured(autoVertical, measureVertical, child.DesiredSize().Height);
    }

    return true;
}

// NOTE: Can't do as an anonymous inline Action above because we need to declare the struct AxisInfo as ref (and Actions don't support ref parameters)
void GridLayout::UpdateAutoBasedOnMeasured(std::vector<winrt::GridTrackInfo> const& tracks, GridLayout::AxisInfo & measure, float childDesired)
{
    if (tracks.size() == 0)
    {
        return;
    }

    float autoSlice = (childDesired / tracks.size());

    for (unsigned int i = 0; i < tracks.size(); i++)
    {
        GridLayoutState::MeasuredGridTrackInfo info = measure.GetMeasuredTrack(i);
        float moreSize = (autoSlice - info.Size);
        if (moreSize > 0)
        {
            //DumpInfo($"Increasing Auto size track {i} by {moreSize}");
            info.Size += moreSize;
            measure.Remaining = std::max(measure.Remaining - moreSize, 0.0f);
        }
    }
}

bool GridLayout::ProcessFractionalSizes(GridLayout::AxisInfo & measure)
{
    if (measure.TotalFraction <= 0.0f)
    {
        return false;
    }

    // What is the size of each fraction?
    float fractionSlice = measure.Remaining / measure.TotalFraction;

    for (unsigned int i = 0; i < measure.Template.size(); i++)
    {
        winrt::GridTrackInfo track = measure.Template[i];
        if (track.Fraction() == 0.0f)
        {
            continue;
        }

        // We only apply the fraction if the item didn't also have a fixed size
        GridLayoutState::MeasuredGridTrackInfo info = measure.GetMeasuredTrack(i);
        if (info.Size != 0.0f)
        {
            continue;
        }

        float myFractionSlice = static_cast<float>(track.Fraction()) * fractionSlice;
        info.Size = myFractionSlice;
    }

    // Fractions consume all that's left
    measure.Remaining = 0.0f;

    return true;
}

// Allow any Auto tracks to take the remaining space
void GridLayout::ProcessAutoRemainingSize(GridLayout::AxisInfo & measure)
{
    if (measure.TotalAutos <= 0)
    {
        return;
    }

    // If there were fractional elements they would have already taken up all the space
    if (measure.Remaining <= 0.0f)
    {
        return;
    }

    // How much extra space do we give up to each Auto element?
    float autoSlice = measure.Remaining / (float)measure.TotalAutos;

    for (unsigned int i = 0; i < measure.Template.size(); i++)
    {
        winrt::GridTrackInfo track = measure.Template[i];
        if (!track.Auto())
        {
            continue;
        }

        GridLayoutState::MeasuredGridTrackInfo info = measure.GetMeasuredTrack(i);
        info.Size += autoSlice;
    }

    // We've consumed all that's left
    measure.Remaining = 0.0f;
}


float GridLayout::ProcessOffsets(GridLayout::AxisInfo & measure)
{
    float offset = 0.0f;
    // PORT_TODO: Backport to C# version
    for (unsigned int i = 0; i < measure.Template.size(); i++)
    {
        winrt::GridTrackInfo track = measure.Template[i];
        auto measurement = measure.Calculated->at(i);
        measurement.Start = offset;
        // PORT_TODO: Running into some equality issues preventing this from being overridden. Using an erase() until I figure this back
        measure.Calculated->erase(i);
        measure.Calculated->insert(std::make_pair(i, measurement));

        offset += measurement.Size;
        offset += measure.Gap;
    }

    return offset;
}

void GridLayout::TraverseByColumn(GridLayout::AxisInfo & horizontal, GridLayout::AxisInfo & vertical, std::function<bool(GridLayout::GridCellIndex cell, GridLayout::AxisInfo & horizontal, GridLayout::AxisInfo & vertical)> predicate)
{
    for (unsigned int column = 0; column < horizontal.Template.size(); column++)
    {
        // Don't consider the last grid line an option (it's meant to be an upper bound, not a starter).
        if (horizontal.Template[column] == s_lastInTrack)
        {
            break;
        }

        for (unsigned int row = 0; row < vertical.Template.size(); row++)
        {
            // Don't consider the last grid line an option (it's meant to be an upper bound, not a starter).
            if (vertical.Template[row] == s_lastInTrack)
            {
                break;
            }

            if (predicate(GridCellIndex{ static_cast<int>(column), static_cast<int>(row) }, horizontal, vertical))
            {
                return;
            }

            // Ping the next track to potentially fault it in if AutoRows are specified
            int nextRow = vertical.EnsureIndexAvailable(row + 1);
        }

    // Ping the next track to potentially fault it in if AutoColumns are specified
    int nextColumn = horizontal.EnsureIndexAvailable(column + 1);
    }
}

// TODO: This code can be more smartly shared with TraverseByColumn
void GridLayout::TraverseByRow(GridLayout::AxisInfo & horizontal, GridLayout::AxisInfo & vertical, std::function<bool(GridLayout::GridCellIndex cell, GridLayout::AxisInfo & horizontal, GridLayout::AxisInfo & vertical)> predicate)
{
    for (unsigned int row = 0; row < vertical.Template.size(); row++)
    {
        // Don't consider the last grid line an option (it's meant to be an upper bound, not a starter).
        if (vertical.Template[row] == s_lastInTrack)
        {
            break;
        }

        for (unsigned int column = 0; column < horizontal.Template.size(); column++)
        {
            // Don't consider the last grid line an option (it's meant to be an upper bound, not a starter).
            if (horizontal.Template[column] == s_lastInTrack)
            {
                break;
            }

            if (predicate(GridCellIndex{ static_cast<int>(column), static_cast<int>(row) }, horizontal, vertical))
            {
                return;
            }

            // Ping the next track to potentially fault it in if AutoColumns are specified
            int nextColumn = horizontal.EnsureIndexAvailable(column + 1);
        }

        // Ping the next track to potentially fault it in if AutoRows are specified
        int nextRow = vertical.EnsureIndexAvailable(row + 1);
    }
}

GridLayout::ChildGridLocations GridLayout::AssignUnoccupiedGridLocation(winrt::UIElement child, GridLayout::AxisInfo & horizontal, GridLayout::AxisInfo & vertical, winrt::GridAutoFlow autoFlow, std::map<GridLayout::GridCellIndex, bool> occupied)
{
    ChildGridLocations result;

    // Read preferences off the child
    winrt::GridLocation colStart = GetColumnStart(child);
    winrt::GridLocation rowStart = GetRowStart(child);
    winrt::GridLocation colEnd = GetColumnEnd(child);
    winrt::GridLocation rowEnd = GetRowEnd(child);

    assert((colStart == nullptr) || (rowStart == nullptr));

    auto checkUnoccupied = [&colStart, &rowStart, colEnd, rowEnd, &occupied](GridCellIndex topLeft, AxisInfo & horizontalAxis, AxisInfo & verticalAxis)
    {
        GridCellIndex bottomRight = GridCellIndex{ topLeft.ColumnIndex + 1, topLeft.RowIndex + 1 };

        bool const allowOutOfRange = false;
        // Handle a specified End paired with our unspecified Start
        if (colEnd != nullptr)
        {
            auto left = GridLayout::ResolvedGridReference(topLeft.ColumnIndex, nullptr);
            ResolvedGridReference right = horizontalAxis.GetTrack(colEnd, &left, allowOutOfRange);
            if (!right.IsValid())
            {
                return false;
            }
            bottomRight.ColumnIndex = right.Index;
        }
        if (rowEnd != nullptr)
        {
            auto top = GridLayout::ResolvedGridReference(topLeft.RowIndex, nullptr);
            ResolvedGridReference bottom = verticalAxis.GetTrack(rowEnd, &top, allowOutOfRange);
            if (!bottom.IsValid())
            {
                return false;
            }
            bottomRight.RowIndex = bottom.Index;
        }

        // Make sure each individual cell is unoccupied in the whole span
        //DumpInfo($"Testing {{{topLeft.ColumnIndex},{topLeft.RowIndex}}} to {{{bottomRight.ColumnIndex},{bottomRight.RowIndex}}}");
        for (int col = topLeft.ColumnIndex; col < bottomRight.ColumnIndex; col++)
        {
            for (int row = topLeft.RowIndex; row < bottomRight.RowIndex; row++)
            {
                GridCellIndex testCoordinate = GridCellIndex{ col, row };
                if (occupied.find(testCoordinate) != occupied.end())
                {
                    return false;
                }
            }
        }

        //DumpInfo($"Found unoccupied {{{topLeft.ColumnIndex},{topLeft.RowIndex}}} to {{{bottomRight.ColumnIndex},{bottomRight.RowIndex}}}");
        colStart = winrt::GridLocation();
        colStart.Index(topLeft.ColumnIndex);
        rowStart = winrt::GridLocation();
        rowStart.Index(topLeft.RowIndex);

        return true;
    };

    // The child has no preference. Find them the first available spot according to the 
    // AutoFlow policy.
    // TODO: Implement difference between Dense and not Dense
    // TODO: Should Dense be a different enum value or a separate property (bool AutoFlowDense)
    switch (autoFlow)
    {
    case winrt::GridAutoFlow::Column:
        TraverseByColumn(horizontal, vertical, checkUnoccupied);
        break;

    case winrt::GridAutoFlow::Row:
        TraverseByRow(horizontal, vertical, checkUnoccupied);
        break;
    }

    if (colStart != nullptr)
    {
        //DumpGridTrackInfo(horizontal.Template[colStart.Index]);
    }
    else
    {
        //DumpInfo("Unable to find column for child");
    }

    if (rowStart != nullptr)
    {
        //DumpGridTrackInfo(vertical.Template[rowStart.Index]);
    }
    else
    {
        //DumpInfo("Unable to find row for child");
    }

    // Map the preferences to actual grid lines
    result.ColStart = horizontal.GetTrack(colStart);
    result.RowStart = vertical.GetTrack(rowStart);
    result.ColEnd = horizontal.GetTrack(colEnd, &result.ColStart);
    result.RowEnd = vertical.GetTrack(rowEnd, &result.RowStart);

    return result;
}

std::map<winrt::UIElement, GridLayout::ChildGridLocations> GridLayout::ResolveGridLocations(GridLayout::AxisInfo & horizontal, GridLayout::AxisInfo & vertical, winrt::NonVirtualizingLayoutContext const& context)
{
    std::map<winrt::UIElement, GridLayout::ChildGridLocations> locationCache;
    std::map<GridLayout::GridCellIndex, bool> occupied;

    // Mark any known grid coordinates as occupied
    for (winrt::UIElement const& child : context.Children())
    {
        GridLayout::ChildGridLocations childLocation;
        if (TryGetChildGridLocations(child, horizontal, vertical, &childLocation))
        {
            MarkOccupied(childLocation, occupied);
            locationCache[child] = childLocation;
        }
    }

    // Go find places for all the unspecified items
    for (unsigned int i = 0; i < context.Children().Size(); i++)
    {
        auto child = context.Children().GetAt(i);
        auto cache = locationCache.find(child);
        if (cache != locationCache.end())
        {
            continue;
        }

        //DumpBegin($"Finding space for unspecified child {i} {child.GetType().Name} (according to AutoFlow {_autoFlow})");
        ChildGridLocations childLocation = AssignUnoccupiedGridLocation(child, horizontal, vertical, m_autoFlow, occupied);
        MarkOccupied(childLocation, occupied);
        locationCache[child] = childLocation;
        //DumpEnd();
    }

    return locationCache;
}

winrt::GridJustifyItems GridLayout::Convert(winrt::GridJustifySelf const& value)
{
    switch (value)
    {
    case winrt::GridJustifySelf::Start: return winrt::GridJustifyItems::Start;
    case winrt::GridJustifySelf::Center: return winrt::GridJustifyItems::Center;
    case winrt::GridJustifySelf::End: return winrt::GridJustifyItems::End;
    case winrt::GridJustifySelf::Stretch: return winrt::GridJustifyItems::Stretch;
    default: assert(false); return winrt::GridJustifyItems::Start;
    }
}

winrt::GridAlignItems GridLayout::Convert(winrt::GridAlignSelf const& value)
{
    switch (value)
    {
    case winrt::GridAlignSelf::Start: return winrt::GridAlignItems::Start;
    case winrt::GridAlignSelf::Center: return winrt::GridAlignItems::Center;
    case winrt::GridAlignSelf::End: return winrt::GridAlignItems::End;
    case winrt::GridAlignSelf::Stretch: return winrt::GridAlignItems::Stretch;
    default: assert(false); return winrt::GridAlignItems::Start;
    }
}

void GridLayout::InitializeForContextCore(winrt::LayoutContext const& context)
{
    auto state = context.LayoutState();
    winrt::com_ptr<GridLayoutState> gridState = nullptr;
    if (state)
    {
        gridState = state.as<GridLayoutState>();
    }

    if (!gridState)
    {
        if (state)
        {
            throw winrt::hresult_error(E_FAIL, L"LayoutState must derive from GridLayoutState.");
        }

        gridState = winrt::make_self<GridLayoutState>();
    }

    context.LayoutStateCore(*gridState);
}

void GridLayout::UninitializeForContextCore(winrt::LayoutContext const& context)
{
}

winrt::Size GridLayout::MeasureOverride(
    winrt::LayoutContext const& context,
    winrt::Size const& availableSize)
{
    //DumpBegin(availableSize, "Measure");

    auto state = context.LayoutState().as<GridLayoutState>();

    //DumpTemplates();
    //DumpInfo($"ColumnGap={_columnGap}, RowGap={_rowGap}");
    //DumpInfo($"AutoFlow={_autoFlow}");

    state->Columns->clear();
    state->Rows->clear();

    GridLayout::AxisInfo measureHorizontal = InitializeMeasure(m_templateColumns, m_autoColumns, state->Columns.get(), static_cast<float>(m_columnGap), availableSize.Width);
    GridLayout::AxisInfo measureVertical = InitializeMeasure(m_templateRows, m_autoRows, state->Rows.get(), static_cast<float>(m_rowGap), availableSize.Height);
    //DumpChildren(measureHorizontal, measureVertical);

    // Resolve all grid references
    std::map<winrt::UIElement, ChildGridLocations> locationCache = ResolveGridLocations(measureHorizontal, measureVertical, context.try_as<winrt::NonVirtualizingLayoutContext>());

    // First process any fixed sizes
    ProcessFixedSizes(measureHorizontal);
    ProcessFixedSizes(measureVertical);
    //DumpMeasureInfo(measureHorizontal, measureVertical, "Fixed");

    // Next we need to know how large the auto sizes are
    bool anyAuto = ProcessAutoSizes(measureHorizontal, measureVertical, locationCache, context.try_as<winrt::NonVirtualizingLayoutContext>());
    if (anyAuto)
    {
        //DumpMeasureInfo(measureHorizontal, measureVertical, "Auto");
    }

    // Then we can figure out how large the fractional sizes should be
    bool anyFractional = false;
    anyFractional |= ProcessFractionalSizes(measureHorizontal);
    anyFractional |= ProcessFractionalSizes(measureVertical);
    if (anyFractional)
    {
        //DumpMeasureInfo(measureHorizontal, measureVertical, "Fractional");
    }

    // And then the auto elements can claim any remaining sizes
    ProcessAutoRemainingSize(measureHorizontal);
    ProcessAutoRemainingSize(measureVertical);
    if (anyAuto)
    {
        //DumpMeasureInfo(measureHorizontal, measureVertical, "Auto remainder");
    }

    for (winrt::UIElement const& child : context.try_as<winrt::NonVirtualizingLayoutContext>().Children())
    {
        ChildGridLocations childLocation = GetChildGridLocations(child, locationCache);

        GridLayoutState::MeasuredGridTrackInfo colMeasure = measureHorizontal.GetMeasuredTrackSafe(childLocation.ColStart);
        GridLayoutState::MeasuredGridTrackInfo rowMeasure = measureVertical.GetMeasuredTrackSafe(childLocation.RowStart);

        // TODO: This isn't measuring them against their entire span
        winrt::Size measureSize = winrt::Size{ colMeasure.Size, rowMeasure.Size };
        child.Measure(measureSize);
    }

    // Now that the sizes are known we can calculate the offsets for the grid tracks
    float width = ProcessOffsets(measureHorizontal);
    float height = ProcessOffsets(measureVertical);
    //DumpMeasureInfo(measureHorizontal, measureVertical, "Calculate offsets", includeOffset: true);

    // If there's no entry for columns/rows use the minimal size, otherwise use the whole space.
    // TODO: This should be derived from our calculated Remaining numbers
    //if (_templateColumns.Count > 0)
    //{
    //    width = availableSize.Width;
    //}
    //if (_templateRows.Count > 0)
    //{
    //    height = availableSize.Height;
    //}

    //DumpEnd();
    return winrt::Size{ width, height };
}

winrt::Size GridLayout::ArrangeOverride(
    winrt::LayoutContext const& context,
    winrt::Size const& finalSize)
{
    //DumpBegin(finalSize, "Arrange");

    auto state = context.LayoutState().as<GridLayoutState>();

    auto nonVirtualizingContext = context.try_as<winrt::NonVirtualizingLayoutContext>();

    // PORT_TODO: How to get attached element's DesiredSize inside arrange?
    //float extraWidth = finalSize.Width - nonVirtualizingContext.DesiredSize().Width;
    float extraWidth = 0.0f;
    float rootOffsetX = 0.0f;

    switch (m_justifyContent)
    {
    case winrt::GridJustifyContent::Start:
        break;

    case winrt::GridJustifyContent::End:
        rootOffsetX = extraWidth;
        break;

    case winrt::GridJustifyContent::Center:
        rootOffsetX = extraWidth * 0.5f;
        break;

    case winrt::GridJustifyContent::SpaceAround:
    case winrt::GridJustifyContent::SpaceBetween:
    case winrt::GridJustifyContent::SpaceEvenly:
        // TODO: Implement
        break;
    }

    // PORT_TODO: How to get attached element's DesiredSize inside arrange?
    //float extraHeight = finalSize.Height - nonVirtualizingContext.DesiredSize().Height;
    float extraHeight = 0.0f;
    float rootOffsetY = 0.0f;

    switch (m_alignContent)
    {
    case winrt::GridAlignContent::Start:
        break;

    case winrt::GridAlignContent::End:
        rootOffsetY = extraHeight;
        break;

    case winrt::GridAlignContent::Center:
        rootOffsetY = extraHeight * 0.5f;
        break;

    case winrt::GridAlignContent::SpaceAround:
    case winrt::GridAlignContent::SpaceBetween:
    case winrt::GridAlignContent::SpaceEvenly:
        // TODO: Implement
        break;
    }

    // TODO: Avoid recreating these lists
    GridLayout::AxisInfo measureHorizontal = InitializeMeasure(m_templateColumns, m_autoColumns, state->Columns.get(), static_cast<float>(m_columnGap), finalSize.Width);
    GridLayout::AxisInfo measureVertical = InitializeMeasure(m_templateRows, m_autoRows, state->Rows.get(), static_cast<float>(m_rowGap), finalSize.Height);

    // Resolve all grid references
    std::map<winrt::UIElement, GridLayout::ChildGridLocations> locationCache = ResolveGridLocations(measureHorizontal, measureVertical, nonVirtualizingContext);

    for (winrt::UIElement const& child : nonVirtualizingContext.Children())
    {
        GridLayout::ChildGridLocations childLocation = GetChildGridLocations(child, locationCache);

        if (!childLocation.ColStart.IsValid() || !childLocation.RowStart.IsValid())
        {
            child.Arrange(winrt::Rect{ 0.0f, 0.0f, 0.0f, 0.0f });
            continue;
        }

        GridLayoutState::MeasuredGridTrackInfo colMeasure = measureHorizontal.GetMeasuredTrackSafe(childLocation.ColStart);
        GridLayoutState::MeasuredGridTrackInfo rowMeasure = measureVertical.GetMeasuredTrackSafe(childLocation.RowStart);
        GridLayoutState::MeasuredGridTrackInfo colEndMesure = measureHorizontal.GetMeasuredTrackSafe(childLocation.ColEnd);
        GridLayoutState::MeasuredGridTrackInfo rowEndMesure = measureVertical.GetMeasuredTrackSafe(childLocation.RowEnd);

        float left = colMeasure.Start;
        float top = rowMeasure.Start;
        float right = colEndMesure.Start;
        float bottom = rowEndMesure.Start;

        // The left edge of a grid line includes the gap amount. As long as we have any
        // coordinates at all we should be trimming that off.
        if (right > 0.0f)
        {
            right -= static_cast<float>(m_columnGap);
        }
        if (bottom > 0.0f)
        {
            bottom -= static_cast<float>(m_rowGap);
        }

        // They might have specified grid references that were inverted. If so, collapse
        // them and essentially zero out the child's arrange size.
        if (right < left)
        {
            right = left;
        }
        if (bottom < top)
        {
            bottom = top;
        }

        float width = (right - left);
        float height = (bottom - top);

        float desiredWidth = std::min(child.DesiredSize().Width, width);
        float unusedWidth = (width - desiredWidth);
        winrt::GridJustifyItems justify = m_justifyItems;
        // PORT_TODO
#if FALSE
        winrt::GridJustifySelf? justifySelf = TryGetJustifySelf(child);
        if (justifySelf.HasValue)
        {
            justify = Convert(justifySelf.Value);
        }
#endif
        switch (justify)
        {
        case winrt::GridJustifyItems::Start:
            width = desiredWidth;
            break;
        case winrt::GridJustifyItems::End:
            left += unusedWidth;
            width = desiredWidth;
            break;
        case winrt::GridJustifyItems::Center:
            left += unusedWidth * 0.5f;
            width = desiredWidth;
            break;
        case winrt::GridJustifyItems::Stretch:
            break;
        }

        float desiredHeight = std::min(child.DesiredSize().Height, height);
        float unusedHeight = (height - desiredHeight);
        winrt::GridAlignItems align = m_alignItems;
        // PORT_TODO
#if FALSE
        winrt::GridAlignSelf? alignSelf = TryGetAlignSelf(child);
        if (alignSelf.HasValue)
        {
            align = Convert(alignSelf.Value);
        }
#endif
        switch (align)
        {
        case winrt::GridAlignItems::Start:
            height = desiredHeight;
            break;
        case winrt::GridAlignItems::End:
            top += unusedHeight;
            height = desiredHeight;
            break;
        case winrt::GridAlignItems::Center:
            top += unusedHeight * 0.5f;
            height = desiredHeight;
            break;
        case winrt::GridAlignItems::Stretch:
            break;
        }

        //DumpBegin(child.GetType().Name);
        //DumpInfo("leftTrack=" + childLocation.ColStart.Index);
        //DumpInfo("topTrack=" + childLocation.RowStart.Index);
        //DumpInfo("rightTrack=" + childLocation.ColEnd.Index);
        //DumpInfo("bottomTrack=" + childLocation.RowEnd.Index);
        //DumpInfo($"left={left}, top={top}, right={right}, bottom={bottom}");
        //DumpEnd();

        winrt::Rect arrangeRect = winrt::Rect{ rootOffsetX + left, rootOffsetY + top, width, height };
        child.Arrange(arrangeRect);
    }

    //DumpEnd();
    return finalSize;
}

void GridLayout::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
}
