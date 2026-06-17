ItemsRepeater overview
===

# Background

The ItemsRepeater is new-ish addition to the WinUI framework. It's a building block for creating controls that show item collections.
For details, please view the videos mentioned at the end of this document.
Six of our controls now use it, including the ItemsView control which is meant to replace the old ListView/GridView in the long term.

Components that come along with the ItemsRepeater: 
- ItemsSourceView
- StackLayout, UniformGridLayout, LinedFlowLayout as built-in layouts,
- ItemCollectionTransitionProvider for item animations (LinedFlowLayoutItemCollectionTransitionProvider is a built-in animator)

Still in preview:
- ElementFactory
- RecyclePool / RecyclingElementFactory
- FlowLayout (buggy)
- SelectionModel / IndexPath for selection management


# Checklist
- Scope of the area  
Large. The ItemsRepeater-related codebase is about 2.3 MB of code.

- Traffic/inflow in this area  
Medium. ItemsRepeater's usage has grown considerably these last few years.

- Dependencies  
None.

- Active Work / Bugs  
  * ItemsRepeater not adjusting RealizationRect to account for changes in LayoutOrigin
  * [GitHub] ItemsRepeater with custom VirtualizingLayout inside ScrollViewer crashes in MeasureOverride on scrolling from bottom to top
  * [GitHub] Unhandled exception when scrolling a ScrollViewer/ItemsRepeater after expanding two or more items that have ItemsRepeater inside
  * [GitHub] Scaling the ItemsRepeater crashes application
  * [GitHub] ItemsRepeater and ScrollView jumping back and forth
  * [GitHub] Expander Content With ItemsRepeater Rendering Bug
  * NonVirtualizingLayout does not reflect element changes in the ItemsRepeater.ItemsSource
  * ItemsRepeater: DataTemplate with ContextFlyout set to StaticResource results in flyout not showing correctly.
  * [Watson Failure] caused by STOWED_EXCEPTION_80131537_Microsoft.UI.Xaml.Controls.dll!ItemsRepeater::MeasureOverride
  * ItemsRepeater: ElementName binding used in its ItemTemplate does not work when using an ItemsRepeater sub-class
  * Lenovo:Think:NECPC:Win11:24H2_Upgrade: Items of ItemsRepeater might suddenly move when ItemsRepeater to coordinate with ScrollViewer
  * [Watson Failure] caused by STOWED_EXCEPTION_80070057_Microsoft.UI.Xaml.Controls.dll!ItemsRepeater::MeasureOverride
  * ItemsRepeater element reuse logic can reuse an unloaded element
  * [GitHub] Erratic Item Repositioning in ItemsRepeater with UniformGridLayout when Dragging Scroll Thumb (Introduced in 1.6-preview1)
  * [Coca-Cola] Using WinUI ListView and/or ItemsRepeater causes a substantial increase in unmanaged memory usage
  * [Watson Failure] caused by ACCESS_VIOLATION_c0000005_Microsoft.UI.Xaml.dll!ItemsRepeater::InvalidateMeasureForLayout
  * [Watson Failure] caused by APPLICATION_HANG_cfffffff_Microsoft.UI.Xaml.dll!ItemsRepeater::GetVirtualizationInfoProperty
  * [Watson Failure] caused by ACCESS_VIOLATION_1007_Microsoft.UI.Xaml.dll!ItemsRepeater::InvalidateMeasureForLayout
  * [Watson Failure] caused by STOWED_EXCEPTION_XAML_TEXT_The_parameter_is_incorrect_80070057_Microsoft.UI.Xaml.dll!ItemsRepeater::MeasureOverride
  * [Watson Failure] caused by STOWED_EXCEPTION_XAML_TEXT_Catastrophic_failure_8000ffff_Microsoft.UI.Xaml.dll!ItemsRepeater::MeasureOverride
  * [Watson Failure] caused by STOWED_EXCEPTION_XAML_TEXT_Invalid_pointer_80004003_Microsoft.UI.Xaml.dll!ItemsRepeater::MeasureOverride
  * [Watson Failure] caused by STOWED_EXCEPTION_XAML_TEXT_element_80070057_Microsoft.UI.Xaml.dll!ItemsRepeater::MeasureOverride
  * [Watson Failure] caused by STOWED_EXCEPTION_88985004_Microsoft.UI.Xaml.dll!ItemsRepeater::MeasureOverride
  * [Watson Failure] caused by APPLICATION_HANG_BusyHang_cfffffff_Microsoft.UI.Xaml.dll!ItemsRepeater::ArrangeOverride
  * [Watson Failure] caused by STOWED_EXCEPTION_80000013_Microsoft.UI.Xaml.dll!ItemsRepeater::MeasureOverride
  * [Watson Failure] caused by STOWED_EXCEPTION_802b000a_Microsoft.UI.Xaml.dll!ItemsRepeater::MeasureOverride
  * [Watson Failure] caused by ACCESS_VIOLATION_c0000005_Microsoft.UI.Xaml.dll!ItemsRepeater::GetVirtualizationInfo
  * [Watson Failure] caused by FATAL_USER_CALLBACK_EXCEPTION_c000041d_Microsoft.UI.Xaml.dll!ItemsRepeater::GetVirtualizationInfo
  * [Watson Failure] caused by STOWED_EXCEPTION_XAML_TEXT_The_paging_file_is_too_small_for_this_operation_to_complete_800705af_Microsoft.UI.Xaml.dll!ItemsRepeater::MeasureOverride
  * [Watson Failure] caused by STOWED_EXCEPTION_XAML_TEXT_ItemsRepeater_s_child_not_found_in_its_Children_collection_80004005_Microsoft.UI.Xaml.dll!ItemsRepeater::ArrangeOverride
  * [Watson Failure] caused by STOWED_EXCEPTION_8007007a_Microsoft.UI.Xaml.dll!ItemsRepeater::ArrangeOverride
  * [Watson Failure] caused by STOWED_EXCEPTION_80131502_Microsoft.UI.Xaml.dll!ItemsRepeater::MeasureOverride
  * [Watson Failure] caused by APPLICATION_HANG_BusyHang_cfffffff_Microsoft.UI.Xaml.dll!ItemsRepeater::GetLayoutContext
  * [Watson Failure] caused by STOWED_EXCEPTION_80131577_Microsoft.UI.Xaml.dll!ItemsRepeater::MeasureOverride
  * UniformGridLayout and ItemsRepeater doesn't correctly size for second item
  * ItemsRepeater should grow its realization window on ItemTemplate/Layout/ItemsSource changes
  * [GitHub] ItemsRepeater breaks when scrolling through variable height items
  * ScrollView: Shows one blank frame when jumping to disconnected offset

- Active Backlog / Enhancement / Committed work  
  * ItemsRepeater - slow scrolling performance
  * [Store] ItemsRepeater should support incremental loading
  * [Store] ItemsRepeater should have something similar to ListViewPersistenceHelper that helps restore scrolling position
  * [Store] ItemsRepeater should support methods like PrepareConnectedAnimation and TryStartConnectedAnimationAsync for connected animations
  * ItemsRepeater/ItemsView: Provide better way to turn off ItemTransitionProvider


- Best practices  
Do make use of the sprinkled debug outputs in a couple of related components.
Debug outputs can be turned on by recompiling Xaml controls with s_IsDebugOutputEnabled (for non-verbose) 
and s_IsVerboseDebugOutputEnabled (for verbose) flags set to true.

  * In ItemsRepeater.cpp:  
      // Change to 'true' to turn on debugging outputs in Output window  
      bool ItemsRepeaterTrace::s_IsDebugOutputEnabled{ false };  
      bool ItemsRepeaterTrace::s_IsVerboseDebugOutputEnabled{ false };  

  * In LinedFlowLayout.cpp:  
      // Change to 'true' to turn on debugging outputs in Output window  
      bool LinedFlowLayoutTrace::s_IsDebugOutputEnabled{ false };  
      bool LinedFlowLayoutTrace::s_IsVerboseDebugOutputEnabled{ false };  


# Key source code locations

These are paths for Microsoft.UI.Xaml.Controls.

- Product: \controls\dev\Repeater
- Test: \controls\dev\Repeater\APITest & InteractionTests
- MuxControlsTestApp test pages: \controls\dev\Repeater\TestUI


# Key methods when debugging

You may want to set breakpoints in these methods:  
- A majority of methods in \controls\dev\Repeater\ItemsRepeater.cpp are interesting, like ItemsRepeater::MeasureOverride / ItemsRepeater::ArrangeOverride.
- FlowLayoutAlgorithm's Measure, Generate and GetAnchorIndex for StackLayout & UniformGridLayout.
- ElementManager::EnsureElementRealized
- VirtualizationInfo::MoveOwnershipToXXXX
- ViewportManager::SetLayoutExtent
- Many APIs in ItemsRepeater.idl are still in Preview. Some will probably be made public when a new TableView control is made public.


# Factoids about the ItemsRepeater

- Some built-in layout instances are sharable in Xaml markup: StackLayout & UniformGridLayout (and FlowLayout). That means a single instance can be used
by multiple ItemsRepeater at the same time. The common fields are stored in StackLayout / UniformGridLayout classes, while the usage-specific fields are
stored in StackLayoutState / UniformGridLayoutState classes.
- The LinedFlowLayout layout does not do this. There is no separate LinedFlowLayoutState class.
- The LinedFlowLayout layout was created after the other layouts, specifically for the File Explorer Photos Gallery view.


# Controls that use the ItemsRepeater
- BreadcrumbBar
- ItemsView
- NavigationView
- PagerControl
- PipsPager
- RadioButtons


# Related docs

- [LinedFlowLayout](LinedFlowLayout_spec.md)
- [Layout updates for LinedFlowLayout](layout-updates-for-lfl.md)
- [ItemsView](ItemsView_spec.md)

- Very useful help topics: https://learn.microsoft.com/en-us/windows/apps/design/controls/items-repeater 
