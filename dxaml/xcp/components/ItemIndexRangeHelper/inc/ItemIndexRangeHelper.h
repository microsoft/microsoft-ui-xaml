// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI { namespace Components { namespace ItemIndexRangeHelper {

    enum class IndexLocation
    {
        Before,
        After,
        Inside
    };

#pragma region helper functions
    // used to check if an index is within range defined by a start and an end
    bool IndexInRange(
        int startIndex,
        int endIndex,
        int index);

    // used to check if two ranges are equal
    bool AreRangesEqual(
        int startIndex1,
        unsigned int length1,
        int startIndex2,
        unsigned int length2);

    // returns the length of the continuous indices starting at the given index
    unsigned int GetContinousIndicesLengthStartingAtIndex(
        const std::vector<unsigned int>& indices,
        int startIndex);

    // used to check the location of an index with respect to a range defined by a start and an end
    IndexLocation IndexWithRespectToRange(
        int startIndex,
        int endIndex,
        int index);

    // used to check if range1 is inside range2
    bool IsFirstRangeInsideSecondRange(
        int startIndex1,
        unsigned int length1,
        int startIndex2,
        unsigned int length2);

    // used to check if range1 is adjacent to range2
    // i.e. end of range1 is start of range2 - 1
    bool IsFirstRangeAdjacentToSecondRange(
        int startIndex1,
        unsigned int length1,
        int startIndex2);
#pragma endregion

#pragma region range struct
    struct Range
    {
        int firstIndex;
        unsigned int length;
        __declspec(property(get = getlastIndex)) int lastIndex;

        Range(int FirstIndex, unsigned int Length)
            : firstIndex(FirstIndex)
            , length(Length)
        {
        }

        int getlastIndex()
        {
            return firstIndex + length - 1;
        }
    };
#pragma endregion

#pragma region range selection
    class RangeSelection
    {
    public:
        void ItemInsertedAt(
            _In_ int index);

        void ItemRemovedAt(
            _In_ int index);

        void ItemChangedAt(
            _In_ int index);

        void SelectAll(
            _In_ unsigned int length,
            _Inout_ std::vector<Range>& addedRanges);

        void SelectRange(
            _In_ int firstIndex,
            _In_ unsigned int length,
            _Inout_ std::vector<Range>& addedRanges);

        void DeselectRange(
            _In_ int firstIndex,
            _In_ unsigned int length,
            _Inout_ std::vector<Range>& removedRanges);

        void DeselectAll(
            _Out_ std::vector<Range>& removedRanges);

        // used to access the private selected ranges vector
        std::vector<Range>::iterator begin()
        {
            return m_selectedRanges.begin();
        }

        std::vector<Range>::iterator end()
        {
            return m_selectedRanges.end();
        }

        unsigned int size()
        {
            return static_cast<unsigned int>(m_selectedRanges.size());
        }

    private:
        // returns true if the index is inside a range inside the selected ranges
        // in case of true, it returns the index of the range it is contained in
        // in case of false, it returns the index of the last range it lies after
        void FindIndexInSelectedRanges(
            _In_ int index,
            _Out_ int* pRangeIndex,
            _Out_ bool* pFound);

        // Used to split a range into two ranges (if possible) using the index passed
        // in the case of adding a new range, rangeIndex is incremented to point to the new range
        void SplitRangeAt(
            _In_ int index,
            _In_ bool forInsertion,
            _Inout_ int* rangeIndex);

        // used when select range happens to overlap two or more selected ranges
        // this function goes through the overlapped ranges and appends to the AddedRanges the ranges in between them
        void AppendGapsToAddedRanges(
            _In_ int startRangeIndex,
            _In_ int endRangeIndex,
            _Inout_ std::vector<Range>& addedRanges);

        // a helper to find the first intersecting or adjacent range from the front or to find the last intersecting or adjacent range from the end
        void SelectRangeFindRangeHelper(
            _In_ bool front,
            _In_ int index,
            _Out_ int* pRangeIndex);

        // a helper to split the range if the deselected range lies inside the current range
        void DeselectRangeInsideHelper(
            _In_ int firstIndex,
            _In_ unsigned int length,
            _In_ int currentRangeIndex,
            _Inout_ std::vector<Range>& removedRanges);

        // a helper if the deselected range is interesecting with the current range from the front or end
        void DeselectRangeIntersectionHelper(
            _In_ int firstIndex,
            _In_ unsigned int length,
            _In_ bool frontIntersection,
            _Inout_ int& currentRangeIndex,
            _Inout_ std::vector<Range>& removedRanges);

        std::vector<Range> m_selectedRanges;
    };
#pragma endregion

} } }