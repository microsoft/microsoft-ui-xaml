// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef SNAPPING_ASSERT
#define SNAPPING_ASSERT ASSERT
#endif

namespace TouchTextSelection {

namespace Details
{
    template <bool condition, typename T = void>
    struct enable_if { };
    template <typename T>
    struct enable_if<true, T> { typedef T type;};

    template <typename T>
    struct remove_const { typedef T type;};
    template <typename T>
    struct remove_const<const T> {typedef T type;};

    template <typename T>
    struct is_integral { const static bool value = false;};
    template <> struct is_integral<__int32> { const static bool value = true;};
    template <> struct is_integral<unsigned __int32> { const static bool value = true;};
    template <> struct is_integral<__int64> { const static bool value = true;};
    template <> struct is_integral<unsigned __int64> { const static bool value = true;};
    template <> struct is_integral<long> { const static bool value = true;};
    template <> struct is_integral<unsigned long> { const static bool value = true;};
} /* namespace Details */

enum class SnappingMode
{
    Char = 0,
    Word
};

enum class GripperSide
{
    Undefined = 0,
    Left,
    Right,
};

// SelectionRange is enabled via an integral Position type - it is undefined by default
template <typename Position, typename Enable = void>
struct SelectionRange;

template <typename Position>
struct SelectionRange<Position, typename Details::enable_if<Details::is_integral<typename Details::remove_const<Position>::type>::value>::type>
{
    SelectionRange()
        : begin(0)
        , end(0)
    {}

    SelectionRange(Position begin, Position end)
        : begin(begin)
        , end(end)
    {
        SNAPPING_ASSERT(begin <= end);
    }

    bool operator==(const SelectionRange& other) const
    {
        return (begin == other.begin) && (end == other.end);
    }

    bool operator!=(const SelectionRange& other) const
    {
        return !(*this == other);
    }

    bool ContainsInside(Position position) const
    {
        return (begin <= position) && (position < end);
    }

    Position begin;
    Position end;
};

// Position is a value type (an integer-type offset)
// SelectionFrameworkClass is the framework-specific enclosing class which has knowledge of the text being selected and its units (words/blocks)

// The selection range here for our algorithm is the range encompassed by the anchor position and the moving position
// (gripperPosition in ComputeSnappingMode)
// If snapping happens, this is not the same as the selection box range shown in the client
// (because it will extend one or both ends to snap an entity)

template <typename Position, typename SelectionFrameworkClass>
class CSnappingCalculator
{

public:
    typedef bool (SelectionFrameworkClass::*PositionsInSameWordFunc)(Position, Position);

public:
    CSnappingCalculator(_In_ SelectionFrameworkClass& selectionFramework,
                        _In_ const PositionsInSameWordFunc& arePositionsInSameWord) :
        _selectionFramework(selectionFramework),
        _ArePositionsInSameWord(arePositionsInSameWord)
    {}

    void InitiateSelection(_In_ const SelectionRange<Position>& range, _In_ GripperSide gripperSide)
    {
        // Must be called with a defined GripperSide
        SNAPPING_ASSERT(gripperSide != GripperSide::Undefined);
        _selectionRange = range;
        _lastShrinkingPosition = (gripperSide == GripperSide::Left) ? _selectionRange.begin : _selectionRange.end - 1;
        _lastActiveGripperSide = gripperSide;
    }

    // gripperPosition is the text position immediately after the gripper's drawn post
    // For example, in the following text where the gripper is being drawn at the position of the vertical bar:
    //
    // ABCDE|FGHIJ
    //
    // gripperPosition is the text position of the character F
    SnappingMode ComputeSnappingMode(_In_ GripperSide gripperSide, _In_ Position gripperPosition)
    {
        // Must be called with a defined GripperSide
        SNAPPING_ASSERT(gripperSide != GripperSide::Undefined);
        SNAPPING_ASSERT(_lastActiveGripperSide != GripperSide::Undefined);
        const bool isSelectionRangeShrinking = _IsSelectionRangeShrinking(gripperSide, gripperPosition);
        _UpdateSelectionRange(gripperSide, gripperPosition);

        bool isSelectionSizeMoreThanOne = (_selectionRange.end - _selectionRange.begin) > 1;
        bool shouldDoWordSnapping = isSelectionSizeMoreThanOne && !isSelectionRangeShrinking && !(_selectionFramework.*_ArePositionsInSameWord)(gripperPosition, _lastShrinkingPosition);
        auto snappingMode = shouldDoWordSnapping ? SnappingMode::Word : SnappingMode::Char;

        _lastActiveGripperSide = gripperSide;
        if (isSelectionRangeShrinking)
        {
            _lastShrinkingPosition = gripperPosition;
        }

        return snappingMode;
    }

#ifdef UNIT_TESTING
    void SetLastShrinkingPosition(_In_ Position lastShrinkingPosition)
    {
        _lastShrinkingPosition = lastShrinkingPosition;
    }

    Position GetLastShrinkingPosition() const
    {
        return _lastShrinkingPosition;
    }

    SelectionRange<Position> GetSelectionRange() const
    {
        return _selectionRange;
    }
#endif

private:
    CSnappingCalculator(const CSnappingCalculator&); // prohibit copy (and move) constructor
    CSnappingCalculator& operator=(const CSnappingCalculator&); // prohibit assignment (and move assignment)

    bool _IsSelectionRangeShrinking(_In_ GripperSide gripperSide, _In_ Position newPosition) const
    {
        // Must be called with a defined GripperSide
        SNAPPING_ASSERT(gripperSide != GripperSide::Undefined);

        // If the grabbed gripper didn't move, the selection has not shrank
        if (((gripperSide == GripperSide::Left) && (newPosition == _selectionRange.begin)) ||
            ((gripperSide == GripperSide::Right) && (newPosition == _selectionRange.end)))
        {
            return false;
        }

        return _selectionRange.ContainsInside(newPosition);
    }

    void _UpdateSelectionRange(_In_ GripperSide gripperSide, _In_ Position gripperPosition)
    {
        // Must be called with a defined GripperSide
        SNAPPING_ASSERT(gripperSide != GripperSide::Undefined);
        SNAPPING_ASSERT(_lastActiveGripperSide != GripperSide::Undefined);
        if (gripperSide == GripperSide::Left)
        {
            if (_lastActiveGripperSide == GripperSide::Right)
            {
                _selectionRange.end = _selectionRange.begin;
            }
            _selectionRange.begin = gripperPosition;
        }
        else
        {
            if (_lastActiveGripperSide == GripperSide::Left)
            {
                _selectionRange.begin = _selectionRange.end;
            }
            _selectionRange.end = gripperPosition;
        }
    }

private:
    SelectionRange<Position> _selectionRange;
    Position _lastShrinkingPosition;
    GripperSide _lastActiveGripperSide;

    // The framework class must outlive this class, as this class references but does not own it
    // This can be ensured by making this a member variable of the framework class, as it isn't not copyable or movable
    SelectionFrameworkClass& _selectionFramework;
    PositionsInSameWordFunc _ArePositionsInSameWord;
};

} /* namespace TouchTextSelection */
