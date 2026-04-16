// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <MocoHelper.h>
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace MoCo {

    class MoCoGroupingIntegrationTests : public WEX::TestClass<MoCoGroupingIntegrationTests>
    {
    public:

        BEGIN_TEST_CLASS(MoCoGroupingIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;a69ddfa4-5142-4bed-887d-6d0ca14a3473;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //

        BEGIN_TEST_METHOD(CanCreateBasicGroupedListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates we can create grouped ListView from a grouped CVS")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCreateBasicGroupedGridView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates we can create grouped GridView from a grouped CVS")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGenerateGroupHeaderItemListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that group headers are generated correctly for ListView.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGenerateGroupHeaderItemGridView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that group headers are generated correctly for GridView")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FirstVisibleItemIsRetainedOnResetListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates whether resetting the items source causes the first visible item to be retained for ListView.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RemoveOnlyItemInOnlyNonEmptyGroup)
            TEST_METHOD_PROPERTY(L"Description", L"ListView-Crash when removing only item in the only non-empty group when hidesifEmpty=true" \
                                                 L"When hidesifempty is true, if we are hiding a group, the group is still in the datasource but the header in"
                                                 L"containermapper should be null.This worked for cases where there were still non empty groups, since when doing" \
                                                 L"a generate pass, we realize that the headers need to be turned to null.The problem with removing the only item" \
                                                 L"in the non empty group is that we are hitting issues before the generate pass(during window determination) where" \
                                                 L"the assumption is that if we have valid headers(which we do, because we have not removed them in the generation" \
                                                 L"pass yet), then we should have stuff to display on screen." \
                                                 L"The fix is to check if we are removing the only item from the only non - empty group during OnItemRemoved() call," \
                                                 L"and force recycle the header as well, so that the next time we measure, we are not looking at that header.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MakeMultipleGroupsEmptyIncludingLastOne)
            TEST_METHOD_PROPERTY(L"Description", L"Make sure that making multiple groups empty including the last on does not cause the listview to crash.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetGroupHeaderContainerFromItemContainer)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the GroupHeaderContainerFromItemContainer api.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MaintainViewportOnHeaderChanged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates converged behavior for maintain viewport on header change")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyChoosingGroupHeaderContainerEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ChoosingGroupHeaderContainer event")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyMaxGroupHeadersInGarbageSection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates no more than 16 group headers are moved into the garbage section of the ItemsPanel children.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyEmptyGroupsDoNotContributeToExtent)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that empty groups dont contribute to extent when hidesifempty is true")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGroupHeaderPlacement)
            TEST_METHOD_PROPERTY(L"Description", L"Validates group header placement")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RemoveLastNonEmptyGroup)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we dont crash on removing last non empty group")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRecycleTrackedGroupHeader)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a tracked group header can get recycled without the world falling apart.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(KeyboardNavigationWithStickyHeaders)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that keyboard up and down keys dont skip items when using sticky headers")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StickyHeadersWorkAfterItemsChange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that sticky headers are in accurate position and visible after inserting and removing items")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AddToEmptyGroupAtBegining)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"Description", L"With hidesIfEmpty adding to empty group at the begining of the list shows the list")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyNoReorderWithGrouping)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that reordering does not work with grouping")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCanAssignCVSSourceWhenCollapsed)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"Ignore", L"True")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that cvs.source can be set and collection changes can happen when listview is collapsed")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //


        //
        // Platform:Phone
        //
    protected:

        void PerformGenerateGroupHeaderTest(
            xaml_controls::ListViewBase^ list,
            MocoHelper::ListControlType listControlType,
            int expectedGroupHeaders);
        void PerformGenerateGroupItemTest(xaml_controls::ListViewBase^ list, int expectedGroups, int expectedItems);
        void PerformBasicGroupingTests(xaml_controls::ListViewBase^ list);
    };

} } } } } }

