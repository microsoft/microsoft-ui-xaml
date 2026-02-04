// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LiveReorderHelperUnitTests.h"

#include <XamlLogging.h>

using namespace DirectUI::Components::LiveReorderHelper;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace LiveReorderHelper {

    static bool AreRectsEqual(const wf::Rect& rect1, const wf::Rect& rect2)
    {
        return (rect1.X == rect2.X && rect1.Y == rect2.Y && rect1.Width == rect2.Width && rect1.Height == rect2.Height);
    }

    static void VerifyMovedItem(const MovedItem& item, const int sourceIndex, const wf::Rect& sourceRect)
    {
        VERIFY_ARE_EQUAL(item.sourceIndex, sourceIndex);
        VERIFY_IS_TRUE(AreRectsEqual(item.sourceRect, sourceRect));
    }

    static void VerifyMovedItem(const MovedItem& item, const int sourceIndex, const int destinationIndex, const wf::Rect& sourceRect, const wf::Rect& destinationRect)
    {
        LOG_OUTPUT(L"%f, %f, %f, %f", destinationRect.X, destinationRect.Y, destinationRect.Width, destinationRect.Height);
        LOG_OUTPUT(L"%f, %f, %f, %f", item.destinationRect.X, item.destinationRect.Y, item.destinationRect.Width, item.destinationRect.Height);

        VERIFY_ARE_EQUAL(item.sourceIndex, sourceIndex);
        VERIFY_ARE_EQUAL(item.destinationIndex, destinationIndex);
        VERIFY_IS_TRUE(AreRectsEqual(item.sourceRect, sourceRect));
        VERIFY_IS_TRUE(AreRectsEqual(item.destinationRect, destinationRect));
    }

    //
    // Test Cases
    //
    void LiveReorderHelperUnitTests::ValidateUpdateMovedItemsWithOneElement()
    {
        MovedItems movedItems;
        std::vector<MovedItem> newItems;
        std::vector<MovedItem> newItemsToMove;
        std::vector<MovedItem> oldItemsToMoveBack;

        wf::Rect rect0 = { 0, 0, 100, 100 };

        MovedItem item0(0, 0, rect0, rect0);

        newItems.push_back(item0);

        movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

        // no items to move
        VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(0));

        // no items to move back
        VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
    }

    void LiveReorderHelperUnitTests::ValidateUpdateMovedItemsWithFixedSizedItems()
    {
        MovedItems movedItems;
        std::vector<MovedItem> newItems;
        std::vector<MovedItem> newItemsToMove;
        std::vector<MovedItem> oldItemsToMoveBack;

        wf::Rect rect0 = { 0, 0, 100, 100 };
        wf::Rect rect1 = { 0, 100, 100, 100 };
        wf::Rect rect2 = { 0, 200, 100, 100 };
        wf::Rect rect3 = { 0, 300, 100, 100 };
        wf::Rect rect4 = { 0, 400, 100, 100 };

        MovedItem item0(0, -1, rect0, rect0);
        MovedItem item1(1, 0, rect1, rect0);
        MovedItem item2(2, 1, rect2, rect0);
        MovedItem item3(3, 2, rect3, rect0);
        MovedItem item4(4, 3, rect4, rect0);

        // scenario 1
        // initially items are as follows
        // 0
        // 1
        // 2
        // 3
        // 4
        {
            // starting from no moved items
            movedItems.clear();

            // dragging item 0 over item 1
            // 0 -> 1 : item 1 should move to item 0
            // 1 -> empty slot : item 1's location should be empty
            // 2 -> 2
            // 3 -> 3
            // 4 -> 4
            {
                newItems.clear();
                newItems.push_back(item0);
                newItems.push_back(item1);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 1 item to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(1));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 1, 0, rect1, rect0);

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // continue dragging item0 over item2
            // 0 -> 1 : item 1 should remain as is
            // 1 -> 2 : item 2 should move to item 1
            // 2 -> empty slot : item 2's location should be empty
            // 3 -> 3
            // 4 -> 4
            {
                newItems.clear();
                newItems.push_back(item0);
                newItems.push_back(item1);
                newItems.push_back(item2);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 1 item to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(1));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 2, 1, rect2, rect1);

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // drag item 0 back over item 2
            // 0 -> 1 : item 1 should move to item 0
            // 1 -> empty slot : item 1's location should be empty
            // 2 -> 2 : item 2 should move back
            // 3 -> 3
            // 4 -> 4
            {
                newItems.clear();
                newItems.push_back(item0);
                newItems.push_back(item1);


                LOG_OUTPUT(L"%i, %i", movedItems.size(), newItems.size());
                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);
                LOG_OUTPUT(L"%i, %i", movedItems.size(), newItems.size());

                // no items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(0));

                // 1 item to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(1));

                // items that moved back
                VerifyMovedItem(oldItemsToMoveBack[0], 2, rect2);
            }

            // drag item 0 over item 4 (case where dragged item could leave list and reenter over a new item)
            // 0 -> 1 : item 1 should remain as is
            // 1 -> 2 : item 2 should move to item 1
            // 2 -> 3 : item 3 should move to item 2
            // 3 -> 4 : item 4 should move to item 3
            // 4 -> empty slot : item 4's location should be empty
            {
                newItems.clear();
                newItems.push_back(item0);
                newItems.push_back(item1);
                newItems.push_back(item2);
                newItems.push_back(item3);
                newItems.push_back(item4);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 3 items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(3));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 2, 1, rect2, rect1);
                VerifyMovedItem(newItemsToMove[1], 3, 2, rect3, rect2);
                VerifyMovedItem(newItemsToMove[2], 4, 3, rect4, rect3);

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // drag item 0 back over item 1
            // 0 -> empty slot : item 0's location should be empty
            // 1 -> 1 : item 1 should move back
            // 2 -> 2 : item 2 should move back
            // 3 -> 3 : item 3 should move back
            // 4 -> 4 : item 4 should move back
            {
                newItems.clear();
                newItems.push_back(item0);

                LOG_OUTPUT(L"%i, %i", movedItems.size(), newItems.size());
                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);
                LOG_OUTPUT(L"%i, %i", movedItems.size(), newItems.size());

                // no items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(0));

                // 4 items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(4));

                // items that moved back
                VerifyMovedItem(oldItemsToMoveBack[0], 4, rect4);
                VerifyMovedItem(oldItemsToMoveBack[1], 3, rect3);
                VerifyMovedItem(oldItemsToMoveBack[2], 2, rect2);
                VerifyMovedItem(oldItemsToMoveBack[3], 1, rect1);
            }
        }

        // scenario 2
        // initially items are as follows
        // 0
        // 1
        // 2
        // 3
        // 4
        {
            // starting from no moved items
            movedItems.clear();

            item0.destinationIndex = 1;
            item1.destinationIndex = -1;
            item2.destinationIndex = 1;
            item3.destinationIndex = 2;
            item4.destinationIndex = 3;

            // dragging item 1 over item 0
            // 0 -> empty slot : item 0's location should be empty
            // 1 -> 1 : item 0 should move to item 1
            // 2 -> 2
            // 3 -> 3
            // 4 -> 4
            {
                newItems.clear();
                newItems.push_back(item1);
                newItems.push_back(item0);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 1 item to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(1));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 0, 1, rect0, rect1);

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // dragging item 1 over item 3
            // 0 -> 0 : item 0 should move back
            // 1 -> 2 : item 2 should move to item 1
            // 2 -> 3 : item 3 should move to item 2
            // 3 -> empty slot : item 3's location should be empty
            // 4 -> 4
            {
                newItems.clear();
                newItems.push_back(item1);
                newItems.push_back(item2);
                newItems.push_back(item3);


                LOG_OUTPUT(L"%i, %i", movedItems.size(), newItems.size());
                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);
                LOG_OUTPUT(L"%i, %i", movedItems.size(), newItems.size());

                // 2 items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(2));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 2, 1, rect2, rect1);
                VerifyMovedItem(newItemsToMove[1], 3, 2, rect3, rect2);

                // 1 item to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(1));

                // items that moved back
                VerifyMovedItem(oldItemsToMoveBack[0], 0, rect0);
            }
        }

        // scenario 3
        // initially items are as follows
        // 0
        // 1
        // 2
        // 3
        // 4
        {
            // starting from no moved items
            movedItems.clear();

            item0.destinationIndex = 1;
            item1.destinationIndex = 2;
            item2.destinationIndex = 3;
            item3.destinationIndex = 4;
            item4.destinationIndex = -1;

            // dragging item 4 over item 2
            // 0 -> 0
            // 1 -> 1
            // 2 -> empty slot : item 2's location should be empty
            // 3 -> 2 : item 2 should move to item 3
            // 4 -> 3 : item 3 should move to item 4
            {
                newItems.clear();
                newItems.push_back(item4);
                newItems.push_back(item3);
                newItems.push_back(item2);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 2 items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(2));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 3, 4, rect3, rect4);
                VerifyMovedItem(newItemsToMove[1], 2, 3, rect2, rect3);

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }
        }
    }

    void LiveReorderHelperUnitTests::ValidateUpdateMovedItemsWithVariableSizedItems()
    {
        MovedItems movedItems;
        std::vector<MovedItem> newItems;
        std::vector<MovedItem> newItemsToMove;
        std::vector<MovedItem> oldItemsToMoveBack;

        wf::Rect rect0 = { 0, 0, 100, 100 };
        wf::Rect rect1 = { 0, 100, 100, 200 };
        wf::Rect rect2 = { 0, 300, 100, 100 };
        wf::Rect rect3 = { 0, 400, 100, 300 };
        wf::Rect rect4 = { 0, 700, 100, 100 };

        MovedItem item0(0, -1, rect0, rect0);
        MovedItem item1(1, 0, rect1, rect0);
        MovedItem item2(2, 1, rect2, rect0);
        MovedItem item3(3, 2, rect3, rect0);
        MovedItem item4(4, 3, rect4, rect0);

        // scenario 1
        // initially items are as follows
        // 0
        // 1
        // 2
        // 3
        // 4
        {
            // starting from no moved items
            movedItems.clear();

            // dragging item 0 over item 1
            // 0 -> 1 : item 1 should move to item 0
            // 1 -> empty slot : item 1's location should be empty
            // 2 -> 2
            // 3 -> 3
            // 4 -> 4
            {
                newItems.clear();
                newItems.push_back(item0);
                newItems.push_back(item1);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 1 item to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(1));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 1, 0, rect1, { 0, 0, 100, 200 });

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // continue dragging item0 over item2
            // 0 -> 1 : item 1 should remain as is
            // 1 -> 2 : item 2 should move to item 1
            // 2 -> empty slot : item 2's location should be empty
            // 3 -> 3
            // 4 -> 4
            {
                newItems.clear();
                newItems.push_back(item0);
                newItems.push_back(item1);
                newItems.push_back(item2);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 1 item to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(1));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 2, 1, rect2, { 0, 200, 100, 100 });

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // drag item 0 back over item 2
            // 0 -> 1 : item 1 should move to item 0
            // 1 -> empty slot : item 1's location should be empty
            // 2 -> 2 : item 2 should move back
            // 3 -> 3
            // 4 -> 4
            {
                newItems.clear();
                newItems.push_back(item0);
                newItems.push_back(item1);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // no items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(0));

                // 1 item to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(1));

                // items that moved back
                VerifyMovedItem(oldItemsToMoveBack[0], 2, rect2);
            }

            // drag item 0 over item 4 (case where dragged item could leave list and reenter over a new item)
            // 0 -> 1 : item 1 should remain as is
            // 1 -> 2 : item 2 should move to item 1
            // 2 -> 3 : item 3 should move to item 2
            // 3 -> 4 : item 4 should move to item 3
            // 4 -> empty slot : item 4's location should be empty
            {
                newItems.clear();
                newItems.push_back(item0);
                newItems.push_back(item1);
                newItems.push_back(item2);
                newItems.push_back(item3);
                newItems.push_back(item4);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 3 items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(3));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 2, 1, rect2, { 0, 200, 100, 100 });
                VerifyMovedItem(newItemsToMove[1], 3, 2, rect3, { 0, 300, 100, 300 });
                VerifyMovedItem(newItemsToMove[2], 4, 3, rect4, { 0, 600, 100, 100 });

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // drag item 0 back over item 1
            // 0 -> empty slot : item 0's location should be empty
            // 1 -> 1 : item 1 should move back
            // 2 -> 2 : item 2 should move back
            // 3 -> 3 : item 3 should move back
            // 4 -> 4 : item 4 should move back
            {
                newItems.clear();
                newItems.push_back(item0);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // no items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(0));

                // 4 items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(4));

                // items that moved back
                VerifyMovedItem(oldItemsToMoveBack[0], 4, rect4);
                VerifyMovedItem(oldItemsToMoveBack[1], 3, rect3);
                VerifyMovedItem(oldItemsToMoveBack[2], 2, rect2);
                VerifyMovedItem(oldItemsToMoveBack[3], 1, rect1);
            }
        }

        // scenario 1
        // initially items are as follows
        // 0
        // 1
        // 2
        // 3
        // 4
        {
            // starting from no moved items
            movedItems.clear();

            item0.destinationIndex = 1;
            item1.destinationIndex = -1;
            item2.destinationIndex = 1;
            item3.destinationIndex = 2;
            item4.destinationIndex = 3;

            // dragging item 1 over item 0
            // 0 -> empty slot : item 0's location should be empty
            // 1 -> 1 : item 0 should move to item 1
            // 2 -> 2
            // 3 -> 3
            // 4 -> 4
            {
                newItems.clear();
                newItems.push_back(item1);
                newItems.push_back(item0);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 1 item to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(1));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 0, 1, rect0, { 0, 200, 100, 100 });

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // dragging item 1 over item 3
            // 0 -> 0 : item 0 should move back
            // 1 -> 2 : item 2 should move to item 1
            // 2 -> 3 : item 3 should move to item 2
            // 3 -> empty slot : item 3's location should be empty
            // 4 -> 4
            {
                newItems.clear();
                newItems.push_back(item1);
                newItems.push_back(item2);
                newItems.push_back(item3);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 2 items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(2));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 2, 1, rect2, { 0, 100, 100, 100 });
                VerifyMovedItem(newItemsToMove[1], 3, 2, rect3, { 0, 200, 100, 300 });

                // 1 item to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(1));

                // items that moved back
                VerifyMovedItem(oldItemsToMoveBack[0], 0, rect0);
            }
        }

        // scenario 3
        // initially items are as follows
        // 0
        // 1
        // 2
        // 3
        // 4
        {
            // starting from no moved items
            movedItems.clear();

            item0.destinationIndex = 1;
            item1.destinationIndex = 2;
            item2.destinationIndex = 3;
            item3.destinationIndex = 4;
            item4.destinationIndex = -1;

            // dragging item 4 over item 2
            // 0 -> 0
            // 1 -> 1
            // 2 -> empty slot : item 2's location should be empty
            // 3 -> 2 : item 2 should move to item 3
            // 4 -> 3 : item 3 should move to item 4
            {
                newItems.clear();
                newItems.push_back(item4);
                newItems.push_back(item3);
                newItems.push_back(item2);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 2 items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(2));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 3, 4, rect3, { 0, 500, 100, 300 });
                VerifyMovedItem(newItemsToMove[1], 2, 3, rect2, { 0, 400, 100, 100 });

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }
        }
    }

    void LiveReorderHelperUnitTests::ValidateUpdateMovedItemsWithGridViewScenario()
    {
        MovedItems movedItems;
        std::vector<MovedItem> newItems;
        std::vector<MovedItem> newItemsToMove;
        std::vector<MovedItem> oldItemsToMoveBack;

        // two rows of elements
        wf::Rect rect0 = { 0, 0, 100, 100 };
        wf::Rect rect1 = { 100, 0, 100, 100 };
        wf::Rect rect2 = { 200, 0, 100, 100 };
        wf::Rect rect3 = { 0, 100, 100, 100 };
        wf::Rect rect4 = { 100, 100, 100, 100 };
        wf::Rect rect5 = { 200, 100, 100, 100 };

        MovedItem item0(0, -1, rect0, rect0);
        MovedItem item1(1, 0, rect1, rect0);
        MovedItem item2(2, 1, rect2, rect0);
        MovedItem item3(3, 2, rect3, rect0);
        MovedItem item4(4, 3, rect4, rect0);
        MovedItem item5(5, 4, rect5, rect0);

        // scenario 1
        // initially items are as follows
        // 0 1 2
        // 3 4 5
        {
            // starting from no moved items
            movedItems.clear();

            // dragging item 0 over item 1
            // 0 -> 1 : item 1 should move to item 0
            // 1 -> empty slot : item 1's location should be empty
            // 2 -> 2
            // 3 -> 3
            // 4 -> 4
            // 5 -> 5
            {
                newItems.clear();
                newItems.push_back(item0);
                newItems.push_back(item1);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 1 item to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(1));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 1, 0, rect1, rect0);

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // continue dragging item0 over item2
            // 0 -> 1 : item 1 should remain as is
            // 1 -> 2 : item 2 should move to item 1
            // 2 -> empty slot : item 2's location should be empty
            // 3 -> 3
            // 4 -> 4
            // 5 -> 5
            {
                newItems.clear();
                newItems.push_back(item0);
                newItems.push_back(item1);
                newItems.push_back(item2);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 1 item to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(1));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 2, 1, rect2, rect1);

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // drag item 0 back over item 2
            // 0 -> 1 : item 1 should move to item 0
            // 1 -> empty slot : item 1's location should be empty
            // 2 -> 2 : item 2 should move back
            // 3 -> 3
            // 4 -> 4
            // 5 -> 5
            {
                newItems.clear();
                newItems.push_back(item0);
                newItems.push_back(item1);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // no items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(0));

                // 1 item to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(1));

                // items that moved back
                VerifyMovedItem(oldItemsToMoveBack[0], 2, rect2);
            }

            // drag item 0 over item 4 (case where dragged item could leave list and reenter over a new item)
            // 0 -> 1 : item 1 should remain as is
            // 1 -> 2 : item 2 should move to item 1
            // 2 -> 3 : item 3 should move to item 2
            // 3 -> 4 : item 4 should move to item 3
            // 4 -> empty slot : item 4's location should be empty
            // 5 -> 5
            {
                newItems.clear();
                newItems.push_back(item0);
                newItems.push_back(item1);
                newItems.push_back(item2);
                newItems.push_back(item3);
                newItems.push_back(item4);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 3 items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(3));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 2, 1, rect2, rect1);
                VerifyMovedItem(newItemsToMove[1], 3, 2, rect3, rect2);
                VerifyMovedItem(newItemsToMove[2], 4, 3, rect4, rect3);

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // drag item 0 back over item 1
            // 0 -> empty slot : item 0's location should be empty
            // 1 -> 1 : item 1 should move back
            // 2 -> 2 : item 2 should move back
            // 3 -> 3 : item 3 should move back
            // 4 -> 4 : item 4 should move back
            // 5 -> 5
            {
                newItems.clear();
                newItems.push_back(item0);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // no items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(0));

                // 4 items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(4));

                // items that moved back
                VerifyMovedItem(oldItemsToMoveBack[0], 4, rect4);
                VerifyMovedItem(oldItemsToMoveBack[1], 3, rect3);
                VerifyMovedItem(oldItemsToMoveBack[2], 2, rect2);
                VerifyMovedItem(oldItemsToMoveBack[3], 1, rect1);
            }
        }
    }

    void LiveReorderHelperUnitTests::ValidateUpdateMovedItemsWithOutsideDragWithFixedSizedItems()
    {
        MovedItems movedItems;
        std::vector<MovedItem> newItems;
        std::vector<MovedItem> newItemsToMove;
        std::vector<MovedItem> oldItemsToMoveBack;

        wf::Rect rect0 = { 0, 0, 100, 100 };
        wf::Rect rect1 = { 0, 100, 100, 100 };
        wf::Rect rect2 = { 0, 200, 100, 100 };
        wf::Rect rect3 = { 0, 300, 100, 100 };

        MovedItem outsideItem(4, -1, {}, {});
        MovedItem item0(0, 1, rect0, rect0);
        MovedItem item1(1, 2, rect1, rect0);
        MovedItem item2(2, 3, rect2, rect0);
        MovedItem item3(3, 4, rect3, rect0);

        // scenario 1
        // initially items are as follows
        // 0
        // 1
        // 2
        // 3
        {
            // starting from no moved items
            movedItems.clear();

            // dragging new item over item 0
            // 0 -> 1 : item 0 should move to item 1
            // 1 -> 2 : item 1 should move to item 2
            // 2 -> 3 : item 2 should move to item 3
            // 3 -> outside : item 3 should move to the outside
            {
                newItems.clear();
                newItems.push_back(outsideItem);
                newItems.push_back(item3);
                newItems.push_back(item2);
                newItems.push_back(item1);
                newItems.push_back(item0);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 4 items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(4));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 3, 4, rect3, { 0, 400, 100, 100 });
                VerifyMovedItem(newItemsToMove[1], 2, 3, rect2, rect3);
                VerifyMovedItem(newItemsToMove[2], 1, 2, rect1, rect2);
                VerifyMovedItem(newItemsToMove[3], 0, 1, rect0, rect1);

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // continue dragging new item over item 3
            // 0 -> 0 : item 0 should move back
            // 1 -> 1 : item 1 should move back
            // 2 -> 2 : item 2 should move back
            // 3 -> outside : item 3 should stay outside
            {
                newItems.clear();
                newItems.push_back(outsideItem);
                newItems.push_back(item3);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 4 items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(0));

                // 3 items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(3));

                // items that moved back
                VerifyMovedItem(oldItemsToMoveBack[0], 0, rect0);
                VerifyMovedItem(oldItemsToMoveBack[1], 1, rect1);
                VerifyMovedItem(oldItemsToMoveBack[2], 2, rect2);
            }
        }
    }

    void LiveReorderHelperUnitTests::ValidateUpdateMovedItemsWithOutsideDragWithVariableSizedItems()
    {
        MovedItems movedItems;
        std::vector<MovedItem> newItems;
        std::vector<MovedItem> newItemsToMove;
        std::vector<MovedItem> oldItemsToMoveBack;

        wf::Rect rect0 = { 0, 0, 100, 50 };
        wf::Rect rect1 = { 0, 50, 100, 100 };
        wf::Rect rect2 = { 0, 150, 100, 150 };
        wf::Rect rect3 = { 0, 300, 100, 200 };

        MovedItem outsideItem(4, -1, {}, {});
        MovedItem item0(0, 1, rect0, rect0);
        MovedItem item1(1, 2, rect1, rect0);
        MovedItem item2(2, 3, rect2, rect0);
        MovedItem item3(3, 4, rect3, rect0);

        // scenario 1
        // initially items are as follows
        // 0
        // 1
        // 2
        // 3
        {
            // starting from no moved items
            movedItems.clear();

            // dragging new item over item 0
            // 0 -> 1 : item 0 should move to item 1
            // 1 -> 2 : item 1 should move to item 2
            // 2 -> 3 : item 2 should move to item 3
            // 3 -> outside : item 3 should move to the outside
            {
                newItems.clear();
                newItems.push_back(outsideItem);
                newItems.push_back(item3);
                newItems.push_back(item2);
                newItems.push_back(item1);
                newItems.push_back(item0);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 4 items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(4));

                // items that moved
                VerifyMovedItem(newItemsToMove[0], 3, 4, rect3, { 0, 500, 100, 200 });
                VerifyMovedItem(newItemsToMove[1], 2, 3, rect2, { 0, 350, 100, 150 });
                VerifyMovedItem(newItemsToMove[2], 1, 2, rect1, { 0, 250, 100, 100});
                VerifyMovedItem(newItemsToMove[3], 0, 1, rect0, { 0, 200, 100, 50});

                // no items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(0));
            }

            // continue dragging new item over item 3
            // 0 -> 0 : item 0 should move back
            // 1 -> 1 : item 1 should move back
            // 2 -> 2 : item 2 should move back
            // 3 -> outside : item 3 should stay outside
            {
                newItems.clear();
                newItems.push_back(outsideItem);
                newItems.push_back(item3);

                movedItems.Update(true /* isOrientationVertical */, newItems, newItemsToMove, oldItemsToMoveBack);

                // 4 items to move
                VERIFY_ARE_EQUAL(newItemsToMove.size(), static_cast<unsigned int>(0));

                // 3 items to move back
                VERIFY_ARE_EQUAL(oldItemsToMoveBack.size(), static_cast<unsigned int>(3));

                // items that moved back
                VerifyMovedItem(oldItemsToMoveBack[0], 0, rect0);
                VerifyMovedItem(oldItemsToMoveBack[1], 1, rect1);
                VerifyMovedItem(oldItemsToMoveBack[2], 2, rect2);
            }
        }
    }

    void LiveReorderHelperUnitTests::ValidateGetDragOverIndex()
    {
        VERIFY_ARE_EQUAL(MovedItems::GetDragOverIndex(0, 0, 0), 0);
        VERIFY_ARE_EQUAL(MovedItems::GetDragOverIndex(0, 1, 0), 0);
        VERIFY_ARE_EQUAL(MovedItems::GetDragOverIndex(1, 1, 0), 0);
        VERIFY_ARE_EQUAL(MovedItems::GetDragOverIndex(1, 2, 0), 1);
        VERIFY_ARE_EQUAL(MovedItems::GetDragOverIndex(2, 2, 1), 1);
        VERIFY_ARE_EQUAL(MovedItems::GetDragOverIndex(2, 3, 1), 2);
        VERIFY_ARE_EQUAL(MovedItems::GetDragOverIndex(2, 2, 2), 2);
        VERIFY_ARE_EQUAL(MovedItems::GetDragOverIndex(1, 2, 2), 2);
        VERIFY_ARE_EQUAL(MovedItems::GetDragOverIndex(1, 1, 2), 1);
        VERIFY_ARE_EQUAL(MovedItems::GetDragOverIndex(0, 1, 1), 1);
        VERIFY_ARE_EQUAL(MovedItems::GetDragOverIndex(0, 0, 1), 0);
    }

} } } } }