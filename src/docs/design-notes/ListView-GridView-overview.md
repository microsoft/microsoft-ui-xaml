ListView / GridView overview
===

# Background

The ListView and GridView controls are used to display item collections. They have a common ancestor: the Selector which itself is an ItemsControl.
The Selector adds the notion of selection to the ItemsControl, while the ListView/GridView add the notion of layout.
They are old, complex and widly used. We have been working on building blocks for creating modern collection controls: ItemsRepeater with Layout, 
SelectionModel, ItemContainer, ScrollPresenter, ScrollView and ItemsView.


# Checklist
- Scope of the area
Very large. If you take into account the ItemsControl & Selector base classes, the various panels (VirtualizingPanel, OrientedVirtualizingPanel, 
VirtualizingStakPanel, ModernCollectionBasePanel, ItemsStackPanel, ItemsWrapGrid) and numerous satellite files, we are talking about ~3.5 MB of code.

- Traffic/inflow in this area
Minimal as the investments have been put into a new generation of collection controls in recent years.

- Dependencies
None

- Active Work / Bugs / Active Backlog

Listing active bugs I found, just to give you an idea of the scope:
  * av in Windows.UI.Xaml.dll!DirectUI::ItemsControl::AddContainers in XAML designer
  * [GitHub] ListViewBase.SelectedItems.Add() performs poorly
  * Changing ListViewItem.Background in code doesn't take effect until pointer over
  * ListviewItem leak in System Xaml leaks TabViewItem which in turn leaks XAML core window Windows_UI!Windows::UI::Core::WindowServer in Explorer process with Tabbed File Explorer.
  * [GitHub] ListView bound to ISupportIncrementalLoading collection causes application crash.
  * [GitHub] ProgressRing is not visible on ListView 1st item, after adding and then removing all items, then adding new items
  * Update ItemsView api spec with note about comparison between interfaces supported by ItemsView.ItemsSource and ItemsControl.ItemsSource so that this can be documented publicly
  * [GitHub] ListView with KeepLastItemInView attribute sometimes results in a blank space at the bottom
  * [GitHub] SetImplicitShowAnimation not working in ItemsControl with ItemsStackPanel
  * [GitHub] BorderThickness and BorderBrush properties of ListViewItem are not working when setting CornerRadius for items in ListView control.
  * [GitHub] UnhandledException when ListView doubletapped event should hide ContentDialog
  * [GitHub] Margins of ListViewItem top Margin visibly changes if it ListViewItem shrinks or grows.
  * [GitHub] ListView allows Multiple Selections while in Single Selection Mode
  * [GitHub] Inheriting from ListView(Base) breaks SelectedIndex selection
  * [GitHub] Theme keys customization not working properly in WinUI ComboBox and ListView control
  * [GitHub] ListView reorder items doesn't trigger VectorChanged with winrt
  * [GitHub] Inconsistent GridView behaviour when setting uninvolved property
  * [GitHub] [GridView] Not enough contrast for 'Selected' & 'Disabled' items on Light mode
  * [GitHub] ListView items stop responding as Drag and Drop targets
  * [Apple TV #322] Moving tab focus to a listview scrolled to a selected item the first time causes the listview to scroll.
  * [GitHub] Using groups in a ListView causes high CPU usage
  * [Apple] Tabbing into ListView for the first time resets its scroll offset even when ICollectionView had set a current item
  * [GitHub] ListView adds duplicate items to SelectedItems
  * [GitHub] ListView Crashes if GetContainerForItemOverride Returns a Container Not Derived from SelectorItem
  * [GitHub] ListView doesn't handle NotifyCollectionChangedAction.Replace with different number of items in old and new.
  * [GitHub] When dragging and dropping ListView items, use `e.Data.SetDataProvider (StandardDataFormats. StorageItems, MyDataProviderHandle)` ; Unable to implement delayed data provision.
  * [Watson Failure] caused by FAIL_FAST_GUARD_ICALL_CHECK_FAILURE_c0000409_Windows.UI.Xaml.dll!DirectUI::ItemsControl::InitializeItemContainerGenerator
  * [Watson Failure] caused by STOWED_EXCEPTION_80131515_Microsoft.UI.Xaml.dll!DirectUI::Selector::SelectRangeInternal
  * [Watson Failure] caused by STOWED_EXCEPTION_80070057_Microsoft.UI.Xaml.dll!DirectUI::ListViewBase::OnDragGesture
  * [Watson Failure] caused by FATAL_USER_CALLBACK_EXCEPTION_c000041d_Windows.UI.Xaml.dll!DirectUI::ItemsControl::SetItemCollection
  * [GitHub] ItemsStackPanel with ListView does not layout items properly when horizontal scroll mode is enabled
  * [GitHub] ListView navigation keys are ignoring keyboard modifiers
  * [GitHub] Unable to open Split-Button flyout within a ListView using ALT + Down on the keyboard
  * [Watson Failure] caused by MOAPPLICATION_HANG_cfffffff_Windows.UI.Xaml.dll!DirectUI::ItemsPresenter::MeasureOverride
  * Using WinUI ListView and/or ItemsRepeater causes a substantial increase in unmanaged memory usage
  * [Watson Failure] caused by STOWED_EXCEPTION_80070057_Windows.UI.Xaml.dll!DirectUI::Selector::OnSelectedIndexChanged
  * [Watson Failure] caused by ACCESS_VIOLATION_c0000005_Microsoft.UI.Xaml.dll!DirectUI::ItemsControl::get_ItemsHost
  * [Watson Failure] caused by MOAPPLICATION_HANG_cfffffff_Windows.UI.Xaml.dll!DirectUI::Selector::UpdateSelectedRanges
  * [Watson Failure] caused by FAIL_FAST_GUARD_ICALL_CHECK_FAILURE_c0000409_Windows.UI.Xaml.dll!DirectUI::ItemsControl::OnItemsChangedImpl
  * [Watson Failure] caused by MOAPPLICATION_HANG_cfffffff_Windows.UI.Xaml.dll!DirectUI::ListView::IsItemItsOwnContainerOverride
  * [Watson Failure] caused by STOWED_EXCEPTION_80131502_Windows.UI.Xaml.dll!DirectUI::Selector::InvokeSelectionChanged
  * [Watson Failure] caused by ACCESS_VIOLATION_1007_Windows.UI.Xaml.dll!DirectUI::Selector::OnSelectionChanged
  * [Watson Failure] caused by MOAPPLICATION_HANG_cfffffff_Windows.UI.Xaml.dll!DirectUI::ItemsControl::GetItemCount
  * [Watson Failure] caused by STACK_OVERFLOW_c00000fd_Windows.UI.Xaml.dll!DirectUI::ListViewBase::OnGettingFocus
  * [Watson Failure] caused by ACCESS_VIOLATION_c0000005_Windows.UI.Xaml.dll!DirectUI::ItemsPresenter::WantsScrollViewerToObscureAvailableSizeBasedOnScrollBarVisibility
  * [Watson Failure] caused by STOWED_EXCEPTION_XAML_TEXT_Parametre_hatal?_80070057_Windows.UI.Xaml.dll!DirectUI::Selector::OnPropertyChanged2
  * [Watson Failure] caused by ILLEGAL_INSTRUCTION_c000001d_Windows.UI.Xaml.dll!DirectUI::Selector::UpdatePublicSelectionProperties
  * [Watson Failure] caused by STOWED_EXCEPTION_XAML_TEXT_??????????????_80070057_Windows.UI.Xaml.dll!DirectUI::ListViewBase::OnDragGesture
  * [Watson Failure] caused by APPLICATION_HANG_BusyHang_cfffffff_Windows.UI.Xaml.dll!DirectUI::ItemsControl::QueryInterfaceImpl
  * [Watson Failure] caused by STOWED_EXCEPTION_80070057_Windows.UI.Xaml.dll!DirectUI::ListViewBase::SetupContainerContentChangingAfterPrepare
  * [Watson Failure] caused by STOWED_EXCEPTION_XAML_TEXT_????????_?????_???????_80070057_Windows.UI.Xaml.dll!DirectUI::Selector::OnPropertyChanged2
  * [Watson Failure] caused by HEAP_CORRUPTION_c0000374_Windows.UI.Xaml.dll!DirectUI::Selector::Initialize
  * [Watson Failure] caused by APPLICATION_HANG_BusyHang_cfffffff_Microsoft.UI.Xaml.dll!DirectUI::ItemsControl::ContainerFromItemImpl
  * [Watson Failure] caused by STOWED_EXCEPTION_XAML_TEXT_A_param_ter_nem_megfelel?_80070057_Windows.UI.Xaml.dll!DirectUI::Selector::OnSelectedIndexChanged
  * [Watson Failure] caused by STOWED_EXCEPTION_8000ffff_Microsoft.UI.Xaml.dll!DirectUI::ListViewBase::MakeRangeSelection
  * [Watson Failure] caused by MOAPPLICATION_HANG_cfffffff_Windows.UI.Xaml.dll!DirectUI::ItemsControl::OnPropertyChanged2
  * [Watson Failure] caused by MOAPPLICATION_HANG_cfffffff_Windows.UI.Xaml.dll!DirectUI::ItemsPresenter::get_PhysicalOrientation
  * [Watson Failure] caused by FAIL_FAST_GUARD_ICALL_CHECK_FAILURE_c0000409_Windows.UI.Xaml.dll!DirectUI::ItemsControl::_ItemsControl
  * [Watson Failure] caused by STOWED_EXCEPTION_80004002_Microsoft.UI.Xaml.dll!DirectUI::ListViewBase::SetupContainerContentChangingAfterPrepare
  * [Watson Failure] caused by STOWED_EXCEPTION_XAML_TEXT_????_80070057_Windows.UI.Xaml.dll!DirectUI::Selector::OnSelectedIndexChanged
  * [Watson Failure] caused by ACCESS_VIOLATION_c0000005_Windows.UI.Xaml.dll!DirectUI::ItemsPresenter::GetInnerPanelOffset
  * [Watson Failure] caused by ACCESS_VIOLATION_c0000005_Windows.UI.Xaml.dll!DirectUI::ListViewBase::BuildTreeImpl
  * [Watson Failure] caused by STOWED_EXCEPTION_XAML_TEXT_El_par_metro_no_es_correcto_80070057_Windows.UI.Xaml.dll!DirectUI::Selector::OnSelectedIndexChanged
  * You can Shift-Tab out of an open SplitView pane when its contents are a ListView
  * Setting IsSelectionRequired propert to true for a listview control
  * [XBOX] ListView.Header does not work properly with gamepand and remote auto-focus model
  * HyperLinkButton in a hovered ListViewItem is not visible in HighContrast mode
  * Swiping down a listview sometimes results in pivots bouncing in an undeterministic way
  * ListView animations don’t always account for the alignment on the panel or ItemsPresenter when the list is not scrollable
  * ListView ItemTemplate fired ContainerContentChanging on AccentColor change
  * Add test coverage for Drag and Drop Narrator scenarios (ListView and other controls)
  * ListView and GridView navigation : when navigating quickly, the animation seems to be choppy
  * On Xbox, ListView does not select the first item the first time the control is focused
  * Accessibility feedback: Narrator should state whether or not there is a context menu associated with a ListView item when it has focus (i.e. "2 finger tap to open menu")
  * Press & Hold on draggable ListView with Transforms causes drag visual to be offset
  * [Keyboard Navigation] When there is a single row in GridView, up/down arrow keys should behave as left/right arrows
  * GridView with ItemsWrapGrid with vertical scrolling and custom focus rects does not bring into view as expected on xbox
  * Binding ListView to a collection that implements IReadOnlyList<T> results in the entire collection being enumerated. To avoid this, the collection must implement IList. Only IList::this[int] needs to be implemented (the rest can throw NotImplementedExcept
  * Empty ListView steals focus from any controls in the ListView's header
  * ListView::ContainerContentChanging::add crashes when called using ->*
  * Dragging on ListView covers up elements in the Footer
  * Visuals disappear when you press and hold on a ListViewItem if the ListView is in a Popup
  * Outlook Mail uses a custom virtualizing list control to display email list and folders [microsoft.windowscommunicationsapps_8wekyb3d8bbwe:ListView]
  * [DocGap] Add guidance to convert from SplitView/ListView to NavigationView - RS4
  * [ListView] - Unable to drop to the middle of list when scrolling is eanbled
  * ListViewBase IsSwipeEnabled needs to be updated in API docs to indicate that it is no longer supported
  * UWP Word uses a custom virtualizing list control to display Recent list and New document grid of templates [microsoft.office.word_8wekyb3d8bbwe:ListView]
  * UWP Excel uses a custom virtualizing list control to display Recent list and New document grid of templates [microsoft.office.excel_8wekyb3d8bbwe:ListView]
  * UWP PowerPoint uses a custom virtualizing list control to display Recent list and New document grid of templates [microsoft.office.powerpoint_8wekyb3d8bbwe:ListView]
  * ListView: Narrator says "Selected" when an entry is selected, but doesn't say anything when it is unselected.
  * ListView.IsSwipeEnabled causes developer confusion in relation to Swipe Control
  * Viewbox causes Textbox to clip when displayed in a ListView with a large data source (baseline pixel snapping issue?)
  * AutoSuggestBox's ItemsControl.Items developer experience is broken (doesn’t have a default collection) and it's causing extra work on XAML platform tests
  * [ATHEN] Win10 - Windows Settings: Categories are arranged in a grid -- GridView should implement IGridProvider
  * Elevation: ListView/GridView touch press+hold animation needs to scale back down after the scale-up
  * Knowing which ListView Item has called the RelayCommand method
  * MultiTest Failure: Controls::ListViewBase Failures - 3 Tests
  * StackPanel->ScrollViewer->GridView leads to Layout Cycle Detected
  * Catastrophic failure (Exception from HRESULT: 0x8000FFFF (E_UNEXPECTED))} from ItemsControl if both DisplayMemberPath and ItemTemplate are set
  * ListViewItem gets stuck in the PointerOver state after dropping an item on it
  * [RFC] Adding drag and drop to grouped ListView causes application to crash
  * Cannnot derive from Selector (and thus cannot cleanly replicate controls like ComboBox)
  * Test Failure: Controls.ListViewBase.FocusVisualTests.LVICNoSystemFocusVisualsBehaviorWithMultipleSelection1
  * Narrator does not provide any imformation about image in ListViewItem
  * ListViewItem haven't name property value if there is image instead of text
  * Test Failure: Enterprise::ListView::ListViewIntegrationTests::AnimateZoomableItemIntoView
  * Test Failure: Enterprise::GridView::GridViewIntegrationTests::ValidateNavigationDoesNotHorizontallyWrapWithGamepad
  * [BUG] UWP Grouped ListView Scroll Bug on Windows 1803 and later
  * ListView control should help users understand how to collapse list items
  * Narrator Desktop : XAML Controls Gallery : Editable text boxes under "GridView with layout customization" header does not have name property.
  * Narrator Desktop : XAML Controls Gallery : List items under "GridView with layout customization" header has wrong name property.
  * ListView Scrolling Randomly Breaks Down
  * Desktop : XAML Controls Gallery : Sent messages are not clearly visible in ListView.
  * Keyboard Desktop : XAML Controls Gallery : After activating "Listview" keyboard focus is going to selection mode combobox.
  * Accessibility Insights: ListView does not report IsOffscreen correctly
  * [ListView] Keyboard reordering (Alt+Shift+arrow keys) does not match visual direction when RTL is enabled
  * [Move to GitHub] Using a StackPanel as the ItemsPanel for a ListView regresses drag-drop announcements in Narrator
  * Color audit: ListViewItem [Regis's change might fix this] (remainder)
  * Color audit: ListViewItem
  * Test Failure: Enterprise::ListView::ListViewIntegrationTests::AnimateZoomableItemIntoView
  * Design Review: ListViewItem/GridViewItem multiselect checks are too small
  * Cannot customize drag visual in ListViewBase.DragItemsStarting
  * GridView with Images has indistinguishable/invisible states for hover + press
  * RS4 XCG ListView page crashes on load (SystemControlErrorTextForegroundBrush used in XCG not found)
  * [Shell request] GridView has no drag delay when using pen
  * When first engaging into a listview with IsFocusEngagementEnabled, the first item gets focus even though KeepLastItemInView is set
  * GridView selection: GridView.SelectRange or setting GridView.SelectedIndex does not work when collection implements ISelectionInfo
  * GridView Ctrl+Click Selection is enabled even in SelectionMode=Single and can lead to crash when ISelectionInfo is implemented
  * ListViewItem Background doesn't always update when changed
  * Every item is cut by GridView, according to the first item's width.
  * [ ListView ] [ HC ] Style updates (remainder x2)
  * [GitHub] WinUI 3 ListView using Wrong Style
  * WinUI 3 crashes on Frame.Navigate() while ListViewItem has capture
  * `ListViewBase.ItemClick` is triggered with a null `ItemClickEventArgs.ClickedItem`
  * [Win10][UWP][NoteApp] Once Crash for pointer point is NULL during ListView Item Drag and Drop Operation
  * [GitHub] Frame.Navigate throws when clicking unselected item in GridView
  * [GitHub] Cannot subclass Selector control
  * Frame.Navigate throws when clicking unselected item in GridView
  * ListView/GridView multi-select elements (MultiSelectSquare and MultiArrangeOverlayTextBorder + their children) should not exist unless selection is used
  * ListView crashes on scrolling with many items
  * [WinAppSDK 1.1] ListView crashes on scrolling with many items
  * GridView bounds to CollectionViewSource with IsSourceGrouped="True" does not show group elements in WinUI3
  * ListView styles regressed in WinAppSDK 1.2
  * [WinAppSDK 1.2-preview1] ListView styles regressed in WinAppSDK 1.2
  * ListViewItem, GridViewItem, and CalendarView styles regressed with latest WinUI2 port
  * [WinAppSDK 1.2] ListViewItem, GridViewItem, and CalendarView styles regressed with latest WinUI2 port
  * Grouped ListView not displaying child elements(WinAppSDK
  * [Watson Failure] caused by STOWED_EXCEPTION_80070002_Microsoft.ui.xaml.dll!DirectUI::Selector::OnSelectionChanged
  * [1.2 Servicing] ListView leaks when DataTemplate has ListViewItem as root element (feature to use the datatemplate's ListViewItem as Container)
  * Add test coverage for ListView with ListViewItem for DataTemplate
  * Navigating between Pages containing a ListView results in memory leaks
  * [Watson Failure] caused by NULL_CLASS_PTR_READ_c0000005_Microsoft.UI.Xaml.dll!_DirectUI::ListViewBase::OnDragGesture_::_163_::_lambda_1_::operator
  * [1.4 Servicing] [Apple] Apple Music/Movies: Enable Listview to optionally navigate between items via TAB instead of Arrows
  * ItemsPresenter: ItemsPresenter's IScrollSnapPointInfo implementation should expose additional snap point(s) to avoid need for ItemsPanel Margin in order to be able to view edge items while using Mandatory snap points
  * [GitHub] SwipeControl (WUX/MUX) causes random crashes in ListView
  * [GitHub] BorderBrush does not apply to ListViewItem
  * [GitHub] UWP ListView Touch Unhandled Win32 Exception
  * [GitHub] ListView creates two containers for one item
  * [GitHub] ContextFlyout throw exception in a GridView with Template in v1.4
  * Apple Music/Movies: Enable Listview to optionally navigate between items via TAB instead of Arrows
  * Grouped Headers don't update layout height when ListView.GroupStyle collection changes - is there a local work-around?
  * ListViewBase TabNavigation=Local broken with virtualized items
  * [1.4 Servicing] ListViewBase TabNavigation=Local broken with virtualized items
  * ListView content gets truncated with text scaling
  * [GitHub] UnhandledException when ListView doubletapped event should hide ContentDialog
  * [GitHub] ListView CanReorderItems not working with custom ItemsPanel
  * [WhatsApp] ItemsStackPanel.ItemsUpdatingScrollMode=KeepLastItemInView is broken when shrinking the ListView size
  * [GitHub] ListView virtualization breaks when using different item templates
  * [Apple TV #396] Window A steals focus from Window B when it contains a ListView with a modified source collection
  * [GitHub] Question: How to reorder item in ListView/GridView when it itself is clickable
  * [GitHub] ListView throws ArgumentOutOfRangeException when CollectionChanged is handled with Action = Reset
  * [GitHub] Unexpected items were obtained in a subclass of ItemsControl
  * [GitHub] How to nest ItemsControl?
  * [File Explorer][ListView] File Explorer sometimes crashes after opening many tabs and selecting Home node in address bar
  * Lenovo:Think:NECPC:Win10、Win11: In the ListView, deleting the first item in the current window results in filling from top to bottom, while deleting other items results in filling from bottom to top. The two behaviors are inconsistent.
  * WinAppSDK 1.6.2 Servicing: Using WinUI ListView and/or ItemsRepeater causes a substantial increase in unmanaged memory usage
  * Connected animations always appear to animate to the first listview item after page navigation instead of the desired listview item
  * [GitHub] Layout cycle detected (crash) for some list view sizes when scrolling to end of listview
  * [GitHub] App freezes with ItemsPresenter + UniformGridLayout + Binding in ItemTemplate
  * [Watson Failure] caused by NULL_CLASS_PTR_READ_c0000005_Microsoft.UI.Xaml.dll!DirectUI::ItemsControl::get_ItemsHost
  * [GitHub] Bug: Should not be able to add same item to ListView.SelectedItems more than once
  * [GitHub] Dropping element on ListViewItem leaves drop target in bad VSM state
  * [GitHub] Groups in ListView and GridView are read incorrectly by the narrator
  * [GitHub] Change `ListViewItem.BackGround` in code doesn't take effect until pointer over
  * [GitHub] Controls in ListView.Header may end up in unload state after changing the ListView.ItemsPanel
  * [GitHub] Flyout in ListView doesn't open for selected item
  * [GitHub] Loaded event for the controls is trigger when the item remove from the listview
  * [GitHub] ListView Default Keyboard Accelerators
  * [GitHub] Selected ListView items especially textbox are shown differently in high constrast theme impacting accessibility.
  * [GitHub] Show wrong select indicator on ListView after reordered
  * [GitHub] `ListView` does not apply DataContext to items correctly
  * [GitHub] Loaded event is fired for a removed ListView item
  * [GitHub] clearing observable collection bound to ListView crashes with ArgumentOutOfRangeException
  * [GitHub] ListView reorder produces two SelectedItems
  * [GitHub] ListView does not trigger "Loaded" events of items when Animations are turned off
  * [GitHub] 100 threads are created when reordering ListView with 2 items in WinUI3
  * [GitHub] UnhandledException when ListView doubletapped event should hide ContentDialog
  * [GitHub] [GridView] Narrator won't announce type of item in GridView
  * [GitHub] [ListView] The app crash when try to use the right click during a reorder operation.
  * [GitHub] ListView doesn't respect Visibility of contents
  * [GitHub] ListView issue
  * [GitHub] ListView Width="*" in header and item not equal on different resolutions.
  * [GitHub] ListView control overflows when it contains a large number of items
  * Using the Community Toolkit DataGrid, Syncfusion DataGrid, or ListView within a StackPanel or NavigationView significantly increases memory usage
  * [Watson Failure] caused by ACCESS_VIOLATION_AVRF_c0000005_Windows.UI.Xaml.dll!DirectUI::ItemsControl::get_ItemsHost

- Active Enhancement / Committed work
  * None

- Proposed work
  * WinUI 3 Reunion 0.5 Bug: Listview does not maintain multi-selection after drag and drop.
  * Cleanup Swipe related dead code from ListView
  * Fix reentrancy bug in ListView Test
  * Improve performance of CDependencyProperty::GetDefaultValue() for several ListView/GridView properties
  * BorderThickness and BorderBrush properties of ListViewItem are not working when setting CornerRadius for items in ListView control.
  * [XAML bug] In GridView, when animation effects are turned off, removing an item drops focus out of the GridView
  * [GitHub] TreeView should expose Header/Footer properties and make use of underlying ListView
  * [GitHub] ItemTemplateSelector is broken in all Collection Views (ListView, GridView, TreeView...) that has their default containers specified in the DataTemplate
  * Grouped ListView drag drop support placeholder task
  * [GitHub] ListView reorder item triggers ObservableCollection Remove and Add instead of Move


# Key control parts

Inside the ListView / GridView control templates, we have four key nested elements:
- a Border,
- a ScrollViewer (the old DirectManipulation-based scrolling/zooming control),
- an ItemsPresenter,
- an ItemsPanel (which is an ItemsStackPanel by default)


# Key source code locations

This list is not exhaustive.

Core control code:
- dxaml/xcp/core/inc/GridViewItemChrome.h
- dxaml/xcp/core/inc/ItemsControl.h
- dxaml/xcp/core/inc/ItemsPresenter.h
- dxaml/xcp/core/inc/ItemsStackPanel.h
- dxaml/xcp/core/inc/ListViewBaseItem.h
- dxaml/xcp/core/inc/ListViewBaseItemChrome.h
- dxaml/xcp/core/inc/ListViewBaseItemSecondaryChrome.h
- dxaml/xcp/core/inc/ListViewItemChrome.h
- dxaml/xcp/core/core/elements/ItemsControl.cpp
- dxaml/xcp/core/core/elements/ItemsPresenter.cpp
- dxaml/xcp/core/core/elements/ItemsStackPanel.cpp
- dxaml/xcp/core/core/elements/ListViewBaseItem.cpp
- dxaml/xcp/core/core/elements/ListViewBaseItemChrome.cpp
- dxaml/xcp/core/core/elements/ListViewBaseItemSecondaryChrome.cpp

DirectUI code:
- dxaml/xcp/dxaml/lib/ListView*.h / .cpp
- dxaml/xcp/dxaml/lib/GridView*.h / .cpp
- dxaml/xcp/dxaml/lib/ItemsControl*.h / .cpp
- dxaml/xcp/dxaml/lib/ItemsPresenter*.h / .cpp
- dxaml/xcp/dxaml/lib/Selector*.h / .cpp
- dxaml/xcp/dxaml/lib/*CollectionView*.h / .cpp
- dxaml/xcp/dxaml/lib/ItemContainerGenerator_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/ItemCollection_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/ItemsStackPanel_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/ItemsWrapGrid_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/ModernCollectionBasePanel_*.h / .cpp
- dxaml/xcp/dxaml/lib/*VirtualizingPanel*.h / .cpp
- dxaml/xcp/dxaml/lib/StackingLayoutStrategy_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/WrappingLayoutStrategy_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/CurrentChangingEventArgs_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/ContainerContentChangingEventArgs_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/FaceplateContentPresenterAutomationPeer_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/Group*.h / *.cpp
- dxaml/xcp/dxaml/lib/Item*.h / *.cpp
- dxaml/xcp/dxaml/lib/StickyHeaderWrapper.h / .cpp
- dxaml/xcp/dxaml/lib/WrapGrid_Partial.h / .cpp
- dxaml/xcp/dxaml/lib/BudgetManager_Partial.h / .cpp
- UI Automation:
  * dxaml/xcp/dxaml/lib/ItemAutomationPeer_partial.h / .cpp
  * dxaml/xcp/dxaml/lib/ItemsControlAutomationPeer_Partial.h / .cpp
  * dxaml/xcp/dxaml/lib/FaceplateContentPresenterAutomationPeer_partial.h / .cpp
  * dxaml/xcp/dxaml/lib/GridViewAutomationPeer_Partial.h / .cpp
  * dxaml/xcp/dxaml/lib/GridViewHeaderItemAutomationPeer_Partial.h / .cpp
  * dxaml/xcp/dxaml/lib/GridViewItemAutomationPeer_partial.h / .cpp
  * dxaml/xcp/dxaml/lib/GridViewItemDataAutomationPeer_partial.h / .cpp
  * dxaml/xcp/dxaml/lib/GroupItemAutomationPeer_Partial.h / .cpp
  * dxaml/xcp/dxaml/lib/ListViewAutomationPeer_Partial.h / .cpp
  * dxaml/xcp/dxaml/lib/ListViewBaseAutomationPeer_Partial.h / .cpp
  * dxaml/xcp/dxaml/lib/ListViewBaseHeaderItemAutomationPeer_Partial.h / .cpp
  * dxaml/xcp/dxaml/lib/ListViewBaseItemAutomationPeer_partial.h / .cpp
  * dxaml/xcp/dxaml/lib/ListViewBaseItemDataAutomationPeer_partial.h / .cpp
  * dxaml/xcp/dxaml/lib/ListViewHeaderItemAutomationPeer_Partial.h / .cpp
  * dxaml/xcp/dxaml/lib/ListViewItemAutomationPeer_partial.h / .cpp
  * dxaml/xcp/dxaml/lib/ListViewItemDataAutomationPeer_partial.h / .cpp
  * dxaml/xcp/dxaml/lib/SelectorAutomationPeer_Partial.h / .cpp
  * dxaml/xcp/dxaml/lib/SelectorItemAutomationPeer_partial.h / .cpp

Test code:
- dxaml/test/native/external/controls/itemscontrol
- dxaml/test/native/external/controls/itemsstackpanel
- dxaml/test/native/external/controls/listviewbaseconnectedanimations
- dxaml/test/native/external/controls/listviewbaseheaderitem
- dxaml/test/native/external/controls/listviewbaseitem
- dxaml/test/native/external/controls/variablesizedwrapgrid
- dxaml/test/native/external/controls/wrapgrid
- dxaml/test/native/external/enterprise/datavirtualization
- dxaml/test/native/external/enterprise/GridView
- dxaml/test/native/external/enterprise/ItemsControl
- dxaml/test/native/external/enterprise/ListView
- dxaml/test/native/external/enterprise/SemanticZoom
- dxaml/test/native/external/enterprise/StickyHeaders

- dxaml/test/managed/enterprise/ItemsPresenter
- dxaml/test/managed/enterprise/Moco

A few files in MUXC as well:
- Resources:
  * \controls\dev\CommonStyles\GridViewItem_themeresources.xaml
  * \controls\dev\CommonStyles\ListViewItem_themeresources

- MuxControlsTestApp test pages:
  * \controls\dev\CommonStyles\TestUI\GridViewPage.*
  * \controls\dev\CommonStyles\TestUI\GroupedItemsControlPage.*
  * \controls\dev\CommonStyles\TestUI\GroupedListViewBasePage.*
  * \controls\dev\CommonStyles\TestUI\ItemsControlPage.*
  * \controls\dev\CommonStyles\TestUI\ListViewAnchoringPage.*
  * \controls\dev\CommonStyles\TestUI\ListViewBasePage.*
  * \controls\dev\CommonStyles\TestUI\ListViewElementNameBindingPage.*
  * \controls\dev\CommonStyles\TestUI\ListViewPage.*
  * \controls\dev\CommonStyles\TestUI\NestedGridViewsPage.*
  * \controls\dev\CommonStyles\TestUI\NestedItemsControlsPage.*
  * \controls\dev\CommonStyles\TestUI\NestedListViewsPage.*


# XAML markup resources

Control templates are defined in `dxaml/xcp/dxaml/themes/generic.xaml`, while item styles are in `controls/dev/CommonStyles` for WinUI 3.


# Key methods when debugging

You may want to set breakpoints in these methods:
- Selector::ScrollIntoViewInternal - for bring-item-into-view operations,

- ListViewBase::BuildTreeImpl - use of BudgetManager for progressive realizations,
- ListViewBase::PrepareContainerForItemOverride/ClearContainerForItemOverride
- ListViewBase::ProcessTabStopInternal/OnKeyDown - for keyboard handling
- ListViewBase::ScrollWithVelocity - for edge scrolling
- ListViewBase::SetLastFocusedIndex - for focus management

- ModernCollectionBasePanel::GenerateContainerAtIndexImpl - for modern item generation

- CListViewBaseItemChrome::MeasureNewStyle/ArrangeNewStyle/GoToChromedStateNewStyl - for chrome rendering


# Factoids about the ListView/GridView

- These complex controls support:
  * ItemsControl.ItemsSource can be set to an object enumeration:
    myListView.ItemsSource = Enumerable.Range(0, 500);    
    or ItemsControl.Items can be filled with UI items like:
    <ListView x:Name="myListView">
      <ListViewItem Content="Woodinville" FontSize="15"/>
      <ListViewItem Content="Yarrow Point" FontSize="15"/>
    </ListView>
  * progressive data consumption through the ISupportIncrementalLoading standard interface,
  * ICollectionView data sources, from the old WPF & Silverlight worlds. With data shaping: filtering, sorting, grouping!
  * ICollectionViewSource is also supported, for collection declarations in markup,
  * UI virtualization through the use of virtualizing panels, with progressive buffer filling, up to ListViewBase.DataFetchSize or 
  ItemsStackPanel/ItemsWrapGrid.CacheLength,
  * item recycling, with a recycling pool, to improve perf in general and scrolling perf in particular (see ItemContainerGenerator),
  * header and footer,
  * group headers,
  * sticky group headers through the use of DirectManipulation parametric curves, and ItemsStackPanel/ItemsWrapGrid.AreStickyGroupHeadersEnabled,
  * item anchoring through the use of the ItemsStackPanel.ItemsUpdatingScrollMode property, to preserve the first or last item in view for example,
  * item bring-into-view operation through the use of the ListViewBase.ScrollIntoView method,
  * edge-scrolling during drag & drop operations,
  * drag & drop:
      * item reordering
      * item tear off
      * item creation from external source,
  * layout transitions for header, footer and items, through the use of TransitionCollection:
      * ListViewBase.HeaderTransitions
      * ListViewBase.FooterTransitions
      * ItemsControl.ItemContainerTransitions,
  * multiple selection modes through the ListViewBase.SelectionMode property,
  * header, footer, group header & item templates,
  * scrolling AND zooming,
  * optional and mandatory scrolling snap points,
  * connected animations

- The items use a 'hard-coded' visuals called chrome defined in `dxaml/xcp/core/inc/ListViewBaseItemChrome.h` &
`dxaml/xcp/core/core/elements/ListViewBaseItemChrome.cpp`. 
This was done to minimize the visuals/weight of the GridView and ListView items, for better performance.
A CListViewBaseItemChrome is a light-weight CContentPresenter sub-class with a pre-defined fixed number of children. This limits items' customizability
but improves perf.

- The controls use a BudgetManager, defined in `dxaml/xcp/dxaml/lib/BudgetManager_Partial.cpp`, which allow optimum item realizations in virtualized
panels, based on the UI thread busyness. This allows the right amount of items to be realized in between two consecutive UI thread ticks, and not
over/underwhelm the thread when scrolling the list for example.

- You can turn on debug outputs by defining LVBIC_DBG and LVBIC_DEBUG in `dxaml/xcp/core/core/elements/ListViewBaseItemChrome.cpp`:

  // Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get ListViewBaseItemChrome debug outputs, and 0 otherwise
  #define LVBIC_DBG 1
  #define LVBIC_DEBUG

- Originally the controls used ItemsPanel like StackPanel, VirtualizingStackPanel and WrapGrid. Then more modern/performant panels were implemented: 
ItemsStackPanel and ItemsWrapGrid.

- These ItemsPanel implementations support the IScrollSnapPointsInfo interface for scrolling snap points:
   * OrientedVirtualizingPanel base class of VirtualizingStackPanel and WrapGrid,
   * ModernCollectionBasePanel base class of ItemsStackPanel and ItemsWrapGrid.

- The controls use a public ItemContainerGenerator which is in charge of the items realization.

- Placing a ListView/GridView into a StackPanel turns off its potential virtualization, since the StackPanel uses an infinite available size for its
stacking direction. It's a recurring performance trap for our customers. 


# Other ItemsControl sub-classes
- AutoSuggestBox
- CommandBarOverflowPresenter
- MenuFlyoutPresenter
- Pivot
- ListBox
- FlipView
- ComboBox

- SemanticZoom


# Somewhat related doc

The [ScrollView](ScrollView-spec.md) document compares the new ScrollView usages with the old ScrollViewer ones.
