// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DoCollection.h>
#include "CollectionUnitTests.h"
#include <XamlLogging.h>
#include <CollectionMoveView.h>
#include <numeric>
#include <vector>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Collection {

    namespace
    {
        using DirectUI::Components::CollectionMoveView;

        void VerifyMoveView(
            const CollectionMoveView& view,
            const std::vector<std::uint32_t>& source,
            const std::vector<std::uint32_t>& expected)
        {
            VERIFY_ARE_EQUAL(static_cast<std::uint32_t>(expected.size()), view.GetSize());
            for (std::uint32_t index = 0; index < view.GetSize(); ++index)
            {
                const auto sourceIndex = view.GetSourceIndex(index);
                VERIFY_IS_TRUE(sourceIndex < source.size());
                VERIFY_ARE_EQUAL(expected.at(index), source.at(sourceIndex));
            }
        }

        void VerifyMove(std::uint32_t size, std::uint32_t oldIndex, std::uint32_t newIndex, std::uint32_t count)
        {
            std::vector<std::uint32_t> expected(size);
            std::iota(expected.begin(), expected.end(), 0u);
            std::vector<std::uint32_t> moved(expected.begin() + oldIndex, expected.begin() + oldIndex + count);
            auto source = expected;
            source.erase(source.begin() + oldIndex, source.begin() + oldIndex + count);
            source.insert(source.begin() + newIndex, moved.begin(), moved.end());

            VERIFY_IS_TRUE(CollectionMoveView::IsValid(size, oldIndex, newIndex, count, count));
            CollectionMoveView view(size, oldIndex, newIndex, count);
            VerifyMoveView(view, source, expected);

            for (std::uint32_t removed = 0; removed < count; ++removed)
            {
                expected.erase(expected.begin() + oldIndex);
                view.RemoveNext();
                VerifyMoveView(view, source, expected);
            }
            for (std::uint32_t inserted = 0; inserted < count; ++inserted)
            {
                expected.insert(expected.begin() + newIndex + inserted, moved[inserted]);
                view.InsertNext();
                VerifyMoveView(view, source, expected);
            }
            VerifyMoveView(view, source, source);
        }
    }

    void CollectionUnitTests::ValidateCCollectionReserve()
    {
        auto pDoCollection = make_xref<CDOCollection>();
        auto pCollection = static_cast<CCollection*>(pDoCollection);

        VERIFY_ARE_EQUAL(pCollection->Reserve(33), S_OK);
    }

    void CollectionUnitTests::MoveViewMapsSingleItems()
    {
        for (std::uint32_t size = 1; size <= 10; ++size)
        {
            for (std::uint32_t oldIndex = 0; oldIndex < size; ++oldIndex)
            {
                for (std::uint32_t newIndex = 0; newIndex < size; ++newIndex)
                {
                    VerifyMove(size, oldIndex, newIndex, 1);
                }
            }
        }
    }

    void CollectionUnitTests::MoveViewMapsRanges()
    {
        for (std::uint32_t size = 1; size <= 10; ++size)
        {
            for (std::uint32_t count = 1; count <= size; ++count)
            {
                for (std::uint32_t oldIndex = 0; oldIndex <= size - count; ++oldIndex)
                {
                    for (std::uint32_t newIndex = 0; newIndex <= size - count; ++newIndex)
                    {
                        VerifyMove(size, oldIndex, newIndex, count);
                    }
                }
            }
        }
    }

    void CollectionUnitTests::MoveViewValidatesRanges()
    {
        VERIFY_IS_TRUE(CollectionMoveView::IsValid(4, 0, 3, 1, 1));
        VERIFY_IS_TRUE(CollectionMoveView::IsValid(4, 2, 0, 2, 2));
        VERIFY_IS_TRUE(CollectionMoveView::IsValid(4, 0, 0, 4, 4));
        VERIFY_IS_FALSE(CollectionMoveView::IsValid(0, 0, 0, 0, 0));
        VERIFY_IS_FALSE(CollectionMoveView::IsValid(4, 0, 0, 0, 0));
        VERIFY_IS_FALSE(CollectionMoveView::IsValid(4, 0, 0, 1, 2));
        VERIFY_IS_FALSE(CollectionMoveView::IsValid(4, -1, 0, 1, 1));
        VERIFY_IS_FALSE(CollectionMoveView::IsValid(4, 0, -1, 1, 1));
        VERIFY_IS_FALSE(CollectionMoveView::IsValid(4, 3, 0, 2, 2));
        VERIFY_IS_FALSE(CollectionMoveView::IsValid(4, 0, 3, 2, 2));
        VERIFY_IS_FALSE(CollectionMoveView::IsValid(4, 0, 0, 5, 5));
        VERIFY_IS_FALSE(CollectionMoveView::IsValid(UINT32_MAX, 1, 0, UINT32_MAX, UINT32_MAX));
    }

    void CollectionUnitTests::MoveViewHandlesLargeIndices()
    {
        VERIFY_IS_TRUE(CollectionMoveView::IsValid(UINT32_MAX, 0, INT32_MAX, 2, 2));
        CollectionMoveView view(UINT32_MAX, 0, INT32_MAX, 2);

        view.RemoveNext();
        VERIFY_ARE_EQUAL(UINT32_MAX - 1u, view.GetSize());
        VERIFY_ARE_EQUAL(static_cast<std::uint32_t>(INT32_MAX) + 1u, view.GetSourceIndex(0));
        VERIFY_ARE_EQUAL(UINT32_MAX - 1u, view.GetSourceIndex(view.GetSize() - 1u));

        view.RemoveNext();
        VERIFY_ARE_EQUAL(UINT32_MAX - 2u, view.GetSize());
        VERIFY_ARE_EQUAL(0u, view.GetSourceIndex(0));

        view.InsertNext();
        VERIFY_ARE_EQUAL(static_cast<std::uint32_t>(INT32_MAX), view.GetSourceIndex(INT32_MAX));
        VERIFY_ARE_EQUAL(static_cast<std::uint32_t>(INT32_MAX) + 2u, view.GetSourceIndex(static_cast<std::uint32_t>(INT32_MAX) + 1u));

        view.InsertNext();
        VERIFY_ARE_EQUAL(UINT32_MAX, view.GetSize());
        VERIFY_ARE_EQUAL(UINT32_MAX - 1u, view.GetSourceIndex(view.GetSize() - 1u));
    }

} } } } }