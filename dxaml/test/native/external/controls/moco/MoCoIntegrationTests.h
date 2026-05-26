// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <string>
#include <collection.h>
#include <MocoHelper.h>
#include <Versioning.h>
#include <CustomMetadataRegistrar.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace MoCo {

    class MoCoIntegrationTests : public WEX::TestClass<MoCoIntegrationTests>
    {
    public:

        BEGIN_TEST_CLASS(MoCoIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"2f2374e7-40ca-495b-b878-9c1acf349dee;e7de4cca-1436-4030-80b9-56ef01aa1cae")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanSelectItemListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully select an item using ListView.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"True")            
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectItemGridView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully select an item using GridView.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanInstantiateListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a ListView.")
        END_TEST_METHOD()

       BEGIN_TEST_METHOD(CanInstantiateGridView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a GridView.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTreeListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove ListView and GridView from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTreeGridView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove ListView and GridView from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanVirtualizeAndRealizeListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully realize and virtualize items in ListView")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanVirtualizeAndRealizeGridView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully realize and virtualize items in GridView")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseCICAndReturnAListViewItemWithCustomItemTemplate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can pass our own itemtemplate and it is being used as expected.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanUseCICAsADataTemplateSelectorReplacement)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can effectively use CIC as a way to select containers.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ContainerContentChangingTestListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ContainerContentChanging event is raised with correct parameters for ListView.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ContainerContentChangingTestGridView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ContainerContentChanging event is raised with correct parameters for GridView.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FirstVisibleItemIsRetainedOnResetListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates whether resetting the items source causes the first visible item to be retained for ListView.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ListViewItemsHaveAReasonableSuggestionPattern)
            TEST_METHOD_PROPERTY(L"Description", L"Validates whether resetting the items source causes ListViewItems to be reset as well without ChoosingItemContainer.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ListViewItemsAreTheSameAfterResetWithChoosingItemContainer)
            TEST_METHOD_PROPERTY(L"Description", L"Validates whether resetting the items source with ChoosingItemContainer causes us to correctly use the same ListViewItem.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateListViewVisualTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ListViewItemPresenter is in the visual tree.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateGridViewVisualTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that GridViewItemPresenter is in the visual tree.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanAccessAndEditConvergedPropertiesGridView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can get and set all of the converged properties on a GridViewItemPresenter from both phone and desktop.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanAccessAndEditConvergedPropertiesListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can get and set all of the converged properties on a ListViewItemPresenter from both phone and desktop.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ManuallyCreatedListViewItemsAreUsedAsContainers)
            TEST_METHOD_PROPERTY(L"Description", L"Validates whether directly giving ListViewItems to a ListView causes them to be used as the containers in the ListView.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanReuseOldListViewItemsOnReset)
             TEST_METHOD_PROPERTY(L"Description", L"Validates whether ContainerContentChanging and ChoosingItemContainer can be used in tandem to reuse old ListViewItems on reset.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanReuseContainersFromRecycleQueue)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that using a container from the RQ works.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCallGetGroupBoundsOnRecycledHeader)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we do not crash if we call GetGroupBounds on ListViewBaseHeaderItem when the header is recycled.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDisablePlaceholdersConvergedBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"verify that we do not show placeholders during sezo view change")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollIntoViewDuringStartup)
            TEST_METHOD_PROPERTY(L"Description", L"verify that we can scroll an item into view after load")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollIntoViewDuringStartupHorizontally)
            TEST_METHOD_PROPERTY(L"Description", L"verify that we can scroll an item into view after load")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanFocusWhileInPopupWithoutStackOverflow)
            TEST_METHOD_PROPERTY(L"Description", L"Regression test for stackoverflow during focus")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateIsSwipeEnabledDeprecated)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that swiping does not invoke selection when IsSwipeEnabled is true")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNewSelectionAndItemClickBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the new selection and itemclick behavior is functioning properly")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(BasicScrollOnGridView)
            TEST_METHOD_PROPERTY(L"Description", L"validates scrolling on default gridview")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCacheExpansion)
            TEST_METHOD_PROPERTY(L"Description", L"validates that the cache is expanded")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePhasingWithIDataTemplateExtensions)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we properly call phasing on datatemplates that have x:bind in them.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePhasingWithIDataTemplateExtensionsAndRegularCCC)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we properly call phasing on datatemplates that have x:bind in them and also CCC usage.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidatePhasingWithRegularCCC)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we properly call phasing with CCC usage.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSizingOfListViewItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we allow for smaller and bigger items in a listview and properly respect alignment.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanDragMulitpleItemsInListView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can drag multiple items in a ListView uinsg the ListViewItemExpanded style")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateNoSelectionWithCVSAndNone)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the list view does not have any selection when using a CVS and selection mode set to none")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateQuicklySwitchingSelectionModes)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can can quickly switch between SelectionMode multiple and single")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCacheRenewal)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we the caches are correct after mutations")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateListItemRightTap)
            TEST_METHOD_PROPERTY(L"Description", L"Validates right tap functionality for pen and touch")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLVWithIWGAndInlineLVI)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that using a list view with an ItemsWrapGrid and an inline ListViewItem takes up the desired space")
        END_TEST_METHOD()
            
        BEGIN_TEST_METHOD(ValidateNavigationAndSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a list view can be navigated, selected and clicked with a gamepad")
        END_TEST_METHOD()
            
        BEGIN_TEST_METHOD(ValidateKeyboardTabFocusWithFirstItemDisabled)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that tabbing to the ListView with the first item disabled sets focus on the next available element")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardTabNavigationWrapsInGroupingScenarioWithLocalNavigationForward)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that tabbing will wrap from the LV to outside if LV is the last control on page")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardTabNavigationWrapsInGroupingScenarioWithLocalNavigationBackward)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that shift-tabbing will wrap from the LV to outside if LV is the first control on page")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(SuspendListViewBuildTreeWhileCollapsed)
            TEST_METHOD_PROPERTY(L"Description", L"Suspend buildtree work when BuildTree controls are collapsed")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(SuspendMCBPBuildTreeWhileCollapsed)
            TEST_METHOD_PROPERTY(L"Description", L"Suspend buildtree work when BuildTree controls are collapsed")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOutOfListViewToAppBar)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can tab out of a ListView and into the AppBar when the ListView is the only item on the page")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPlaceholders)
            TEST_METHOD_PROPERTY(L"Description", L"Validates placeholders are displayed correctly when items are null (haven't been loaded yet)")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyListViewKeyboardBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"Validates keyboard behavior in ListView")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGridViewKeyboardBehavior)
            TEST_METHOD_PROPERTY(L"Description", L"Validates keyboard behavior in GridView")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGridViewKeyboardBehaviorWrapGrid)
            TEST_METHOD_PROPERTY(L"Description", L"Validates keyboard behavior in GridView using a WrapGrid")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyEdgeScrollingWithReorder)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that dragging to the bottom of the listview causes the listview to scroll")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanCalculateFutureRealizationWindowCorrectly)
            TEST_METHOD_PROPERTY(L"Description", L"In order to maintain the first visible element in the viewport, we need to correctly calculate the future realization window that encompass it.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanInsertMultipleItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can insert multiple items in the middle of the collection without some items not showing up.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLargeFooterDoesNotRecycleElements)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that having a large footer that covers the full Window does not recycle the elements and keeps some in cache.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyListViewKeyboardReorder)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that keyboard reordering works in ListView")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGridViewKeyboardReorder)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that keyboard reordering works in GridView")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGridViewKeyboardReorderWrapGrid)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that keyboard reordering works in GridView using a WrapGrid")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyRemoveSelectedDraggedItemFromItemsList)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that removing the (selected) dragged item from the collection does not crash the application")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDropIntoFolder)
            TEST_METHOD_PROPERTY(L"Description", L"Validates dropping into folder scenario does not reorder items")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyLiveReorderStoryboards)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that live reorder plays the right storyboards")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateRangeSelectionAfterSelectingProgrammatically)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that selecting ranges through Ctrl/Shift+Click still works after setting SelectedIndex value or using SelectRange")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateScrollingWithLiveReorder)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that scrolling the mouse wheel during reorder resets containers' location correctly")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"Ignore", L"True") // ValidateScrollingWithLiveReorder is failing
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateItemsAfterCancellingDragWithTouch)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that is the developer cancels the Drag in DragItemsStarting, items go back to their normal state")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReadOnlyMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that Read-Only mode is supported when SelectionMode is none and IsItemClickEnabled is false")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateArrowKeysDontAffectListViewBaseItemsWhenInHeaderFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that pressing the arrow keys does not get treated by the ListViewBase as an item interaction")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFocusCanBeChangedInListViewItemClickEventHandler)
            TEST_METHOD_PROPERTY(L"Description", L"If app code moves focus in ListView.ItemClick event handler, we should not overwrite that.")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //
        BEGIN_TEST_METHOD(ValidateMultipleSelectionModeRangeSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the new Multiple selection supports the new keyboard range selection behavior")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSourceClearDuringSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates we can clear a collection during itemclick and not crash")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDraggedItemsContentNotNullWithCCC)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the Dragged items returned are not NULL when using CCC")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateGlyphAndCaptionVisibilityDuringReorder)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the glyph and caption visibility during a Reorder")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateInsertFromOutsideAfterLastItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can drag an item from the outside the ListView and insert at the end of the ListView")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

protected:
        void PerformSelectionTests(xaml_controls::ListViewBase^ list);
        void PerformVirtualizationAndRealizationTests(xaml_controls::ListViewBase^ list);
        void PerformContainerContentChangingTests(xaml_controls::ListViewBase^ list);
        void PerformArrowKeysInHeaderFooterFocusTest();

private:        
        // This member variable stores the MetadataProvider we'll use to inform Juptier about
        // the custom types in this test.

        void PerformSelectionAndItemClickFalseTests(
            Microsoft::UI::Xaml::Controls::ListView^ listView,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& selectionChangedEvent,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& itemClickEvent);

        void PerformSelectionAndItemClickTrueTests(
            Microsoft::UI::Xaml::Controls::ListView^ listView,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& selectionChangedEvent,
            std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& itemClickEvent,
            bool singleSelectionFollowsFocus);

        void PerformKeyboardReorderTest(
            Microsoft::UI::Xaml::Controls::ListViewBase^ listViewBase,
            int initialSelectedIndex,
            int resultingSelectedIndex);
    };

    ref class MoCoDataTemplateExtensionMock sealed : public IDataTemplateExtension
    {
    internal:
        MoCoDataTemplateExtensionMock(Platform::Collections::Vector<uint32>^ PhasesWeWantCallbacksFor)
        {
            numberOfResetTemplateCalls = 0;
            phasesCalled = ref new Platform::Collections::Vector<uint32>();
            phasesWeWantCallbacksFor = PhasesWeWantCallbacksFor;
            inRecycle = false;
        }

    public:
        virtual void ResetTemplate() { numberOfResetTemplateCalls++; }

        virtual bool ProcessBinding(unsigned int phase) { phase++; throw ref new Platform::FailureException; }

        virtual int ProcessBindings(Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args);

        uint16 GetNumberOfResetTemplateCalls() { return numberOfResetTemplateCalls; }

        bool VerifyCalledForAllPhases();

        bool ContainerIsInRecycleQueue() { return inRecycle; }

    private:
        uint16 numberOfResetTemplateCalls;
        Platform::Collections::Vector<uint32>^ phasesCalled;
        Platform::Collections::Vector<uint32>^ phasesWeWantCallbacksFor;
        bool inRecycle;
    };

    ref class ListViewItemMock sealed : public Microsoft::UI::Xaml::Controls::ListViewItem
    {
    public:
        ::Windows::Foundation::Size GetPassedInAvailableSize() { return passedInAvailableSize; }
        ::Windows::Foundation::Size GetPassedInFinalSize() { return passedInFinalSize; }

    protected:
        virtual ::Windows::Foundation::Size MeasureOverride(::Windows::Foundation::Size availableSize) override
        {
            passedInAvailableSize = availableSize;
            return __super::MeasureOverride(availableSize);
        }
        virtual ::Windows::Foundation::Size ArrangeOverride(::Windows::Foundation::Size finalSize) override
        {
            passedInFinalSize = finalSize;
            return __super::ArrangeOverride(finalSize);
        }

    private:
        ::Windows::Foundation::Size passedInAvailableSize;
        ::Windows::Foundation::Size passedInFinalSize;
    };
} } } } } }

