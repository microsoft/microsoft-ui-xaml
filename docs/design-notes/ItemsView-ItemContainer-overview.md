ItemsView/ItemContainer overviews
===

# Background

The ItemsView is a new control dedicated to representing item collections. Each item is hosted in the new light-weight ItemContainer control.
This is a new modern offering replacing the aging ListView/GridView collection controls. It is built on top of new building blocks: ItemsRepeater,
SelectionModel and [ScrollView](ScrollView-overview.md).

The ItemsView and ListView/GridView feature sets overlap but each has missing capabilities compared to the other.

Examples: 
- Mostly because the ItemsView is new, it has no built-in support for:
    * grouping, 
    * sticky group headers, 
    * progressive data consumption through the ISupportIncrementalLoading,
    * ICollectionView data sources,
    * header and footer,
    * edge-scrolling,
    * drag & drop,
    * attached properties for its inner ScrollView part.

- The ListView & GridView do not support:
    * flow layouts (like the new [LinedFlowLayout](LinedFlowLayout_spec.md)),
    * pluggable layouts,
    * pluggable scrolling controllers (like the new [AnnotatedScrollBar](annotatedscrollbar_spec.md)),
    * custom recycling pools,
    * programmatic control of scrolling/zooming curves,
    * custom collection change animations.
    

# Checklist
- Scope of the area  
Medium. The overall codebase involved, excluding the underlying ScrollView/ItemsRepeater components which are much larger, is:
    * ItemContainer: 630KB.
    * ItemsView: 1 MB.

- Traffic/inflow in this area  
Minimal. Not many controls make use of the ItemsView yet.

- Dependencies  
Built on top of major internal components: ItemsRepeater & ScrollView (which depends on external InteractionTracker).

- Active Work / Bugs
    * ItemContainer: Content bleeds beyond selection border
    * Deleted ItemContainer, with active animator, causes blank space after swapping Layout in ItemsView
    * ItemContainer visual state gets stucked in "pressed"

    * ItemsView: Try to avoid the need for LinedFlowLayout::m_anchorIndexRetentionCountdown during ItemsView.StartBringItemIntoView calls
    * UniformGridLayout: Application hangs when hitting End key in ItemsView
    * ItemsView: Calling myItemsView.SelectionModel.SelectAll() in None and Single SelectionMode selects all items	 
    * ItemsView: test failures, Selection="None" verify ItemInvoked raised
    * Deleted ItemContainer, with active animator, causes blank space after swapping Layout in ItemsView 
    * [GitHub] Calling ItemsView.StartBringItemIntoView() Freezes The WinUI3 App
    * [GitHub] If the ItemsView scrolls the scrollbar, the memory of the item will not be released.


- Active Enhancement / Committed work  
    * ItemContainer: ItemContainer does not implement the IHostItemContainer / IHostItemContainer2 interfaces as spec'ed

    * ItemsRepeater/ItemsView: Provide better way to turn off ItemTransitionProvider

- Active Backlog  
    * ItemContainer - Focus Visuals are not consistent with other locations in FileExplorer	 
    * Update ItemsView api spec with note about comparison between interfaces supported by ItemsView.ItemsSource and ItemsControl.ItemsSource so that this can be documented publicly

    * [Proposal] Interaction design for checkboxes in Gallery ItemsView should align with other File Explorer views
    * ItemsView: consider adding some SingleSelectionFollowsFocus property
    * ItemsView: Should ItemsViewSelectionMode.Single be split into two?
    * [Group] [Phase Next] FE - Build ItemsView Layouts for File Explorer’s gallery view
    * OWL - ItemsView: Photos App Scenarios
    * OWL - ItemsView: Third Party Scenarios 
    * ADEPT - File Explorer: ItemsView support for expandable/collapsible groups of items
    * ADEPT - File Explorer: ItemsView support for customization of the template used for the groups and the items within those groups
    * ItemsRepeater should grow its realization window on ItemTemplate/Layout/ItemsSource changes
    * Proposal: [WinUI] Create shimmer loader control to support AI + shell features
    * [Proposal] : Implement “Lasso” support for selection model in ItemsView
    * [Proposal] Implement pinch to zoom (dynamic scaling based on pinch to zoom or Ctrl+scroll wheel) in ItemsView
    * [Proposal] Include ItemsRangeInfo support in ItemsView
    * FE - Add advanced features to ItemsView control in SV4+
    * Build the Masonry Layout View
    * Add Drag and Drop Support


- Best practices  
Do make use of the sprinkled debug outputs in most ItemsView-related components.
A ton of debug outputs can be turned on by recompiling Xaml controls with s_IsDebugOutputEnabled (for non-verbose) 
and s_IsVerboseDebugOutputEnabled (for verbose) flags set to true.

  Examples:
    * In ItemContainer.cpp:
        // Change to 'true' to turn on debugging outputs in Output window
        bool ItemContainerTrace::s_IsDebugOutputEnabled{ false };
        bool ItemContainerTrace::s_IsVerboseDebugOutputEnabled{ false };

    * In ItemsView.cpp:
        // Change to 'true' to turn on debugging outputs in Output window
        bool ItemsViewTrace::s_IsDebugOutputEnabled{ false };
        bool ItemsViewTrace::s_IsVerboseDebugOutputEnabled{ false };

  ItemsView::***Dbg are methods dedicated to debugging only. It's a pattern that should be followed generally (The ItemContainer does not but it should).


# Key control parts

Inside the ItemsView control template, we have two key elements:
- a ScrollView, named PART_ScrollView, which allows scrolling and zooming of the collection items ScrollView.Content,
- an ItemsRepeater, PART_ItemsRepeater named, (as the Content of the above ScrollView) which hosts the collection items.

Inside the ItemContainer control template, we have the following key elements:
- a Grid, named PART_ContainerRoot, parent of all other elements and the Child element,
- a Grid, named PART_SelectionVisual, responsible for the selection cue,
- a Rectangle, named PART_CommonVisual, also shows selection cues and background,
- a CheckBox, named PART_SelectionCheckbox, shows a selection checkbox when the MultiSelectMode property is ItemContainerMultiSelectMode.Multiple.


# Key source code locations

These are paths for Microsoft.UI.Xaml.Controls.

## ItemContainer

- Product: \controls\dev\ItemContainer
- Test: \controls\dev\ItemContainer\APITest & InteractionTests


## ItemsView

- Product: \controls\dev\ItemsView
- Test: \controls\dev\ItemsView\APITest & InteractionTests


# XAML markup resources

## ItemContainer

Style resources, including the control template, are located under \controls\dev\ItemContainer:
- \controls\dev\ItemContainerw\ItemContainer_themeresources.xaml
- \controls\dev\ItemContainer\ItemContainer.xaml


## ItemsView

Style resources, including the control template, are located under \controls\dev\ItemsView:
- \controls\dev\ItemsView\ItemsView_themeresources.xaml
- \controls\dev\ItemsView\ItemsView.xaml


# MuxControlsTestApp test pages

- ItemContainer

Examples:  
![Top level test page](images\ItemContainerTestPage1.png)  
![APIs test page](images\ItemContainerTestPage2.png)

- ItemsView

Examples:  
![Top level test page](images\ItemsViewTestPage1.png)  
![APIs test page](images\ItemsViewTestPage2.png)


# Key methods when debugging

You may want to set breakpoints in these methods:
- ItemContainer::GoToState
- ItemsView::StartBringItemIntoViewInternal - for bring-into-view operations
- ItemsView::GetItemInternal/SetFocusElementIndex - for keyboarding / focusing
- ItemsView::OnItemsRepeaterElementPrepared/OnItemsRepeaterElementClearing - for item setup and recycling
- ItemsView::ApplySelectionModelSelectionChange - for selection tracking


# Key fields when debugging

You may want to keep track of these important fields:
- std::shared_ptr<PointerInfo<ItemContainer>> ItemContainer::m_pointerInfo{ nullptr };
- winrt::SelectionModel ItemsView::m_selectionModel{}; - for selection tracking
- winrt::SelectionModel ItemsView::m_currentElementSelectionModel{}; - for currency tracking


# Factoids about the ItemContainer

- ItemContainer.Child is hosted in a Grid named PART_ContainerRoot. It is inserted as the first child of that Grid.
- It was designed to be a building block for other high level controls.
- Its IDL defines internal APIs that were meant for more baking time and then shipped.
- In future releases, it should implement [IHostItemContainer / IHostItemContainer2](itemcontainer-functional-spec.md) interfaces for much improved reusability.
  ItemContainer does not implement the IHostItemContainer / IHostItemContainer2 interfaces as spec'ed.
- A couple of control template elements use DeferLoadStrategy in ItemContainer.xaml for improved performance.
- Consider adding another control template element to represent currency (which is something File Explorer would use for example).


# Factoids about the ItemsView

- The default ItemTemplate uses an ItemContainer at its root.
- Any custom ItemTemplate must also use an ItemContainer at its root (for now, until ItemsView.GetItemContainerOverride is implemented).
- Customers can use any built-in layout: StackLayout (the default), UniformGridLayout or LinedFlowLayout (via the ItemsView.Layout property).
- Customers can plug in their own custom animator (via the ItemsView.ItemTransitionProvider property).
- Customers can plug in their own custom layout implementations (via the ItemsView.Layout property).
- Customers can plug in their own custom vertical scroll controller (via the ItemsView.VerticalScrollController property).
- It supports the same selection models as the old Selector control.
- ItemsView.ItemsSource <=> inner ItemsRepeater.ItemsSource.
- ItemsView.ItemTemplate <=> inner ItemsRepeater.ItemTemplate.
- ItemsView.VerticalScrollController <=> inner ScrollView.ScrollPresenter.VerticalScrollController.
- It exposes APIs from its inner SelectionModel.
- Goal (unrealilzed at the moment): ItemsView should have no dependency on ItemContainer type.
- Supports bring-into-view operation to unrealized item.
- Supports index-based and location-based keyboard navigations, for any layout.


# Features that make use of ItemsView

Recent File Explorer Photos Gallery UI.


# Related docs

The [ItemsView](ItemsView_spec.md), [ItemContainer](itemcontainer-functional-spec), [ItemCollectionTransitionProvider](ItemCollectionTransitionProvider-spec),
[LinedFlowLayout](LinedFlowLayout_spec), [Layout updates for LinedFlowLayout](layout-updates-for-lfl) documents provide lots of information about the expected
behaviors.
