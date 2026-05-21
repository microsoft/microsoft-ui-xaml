<!-- The purpose of this document is to describe the design and implementation of a new WinUI control.
     This document contains architectural and implementation details that do not appear in the functional spec. -->


# Description

<!-- Use this section to provide a brief description of the control. -->
The Segmented control will be used to configure and switch between different views or settings of an app. For example, in a photos gallery app, Segmented control can switch between viewing the photos in a grid or list view. 

The Segmented control is different from the NavigationView control in that the content itself remains the same but the view of the content changes. NavigationView and Segmented control are functionally and visually almost identical but serve different purposes. NavigationView transitions between pages (changes the content itself). Segmented control switches between the view of the content itself (the content stays the same). Having two different controls allows for better discoverability and ease of use for app developers.

In WinUI3, the Navigation View control has the capabilities to accomplish Segmented control's purpose; 
however, Navigation View is too heavy weight for customers who want a simple Segmented. So currently, there is a desire for a light-weight control that can switch between different views on an app like Pivot in WinUI 2.

# Functional spec link

<!-- Add a link to the functional spec which uses the control_functional_spec_template.md template -->
[Functional Spec](segmented-functional-spec.md)

# Architectural overview

## Key public components

<!-- List the key public components of the control and their relationships. Include an optional diagram. -->

Segmented (parent of SegmentedItem)
- has a horizontal collection of items that are Segmented items which are only selectable

SegmentedItem (child of Segmented)
- no interaction between children
  
## Key internal components

<!-- List the key internal components of the control and their relationships. Include an optional diagram. -->
Animation
- Pill animation
- Backplate animation

SegmentedItem (child of Segmented)
- OnHover
- OnLeave

# Performance considerations

<!-- List any performance-oriented implementation choices. -->
SegmentedItem consider using ItemsRepeater for large number of items even though it's not recommended

# Debugging and telemetry considerations

<!-- Specify which KEYWORD_xxx is used for debug logging, in \dev\Telemetry\TraceLogging.h.
     Example:  #define KEYWORD_TABVIEW 0x0000000000000040

     Consider using a test hooks for access to private data and notifications.
     Example:  \dev\ScrollPresenter\ScrollPresenterTestHooks.idl

     Specify the RuntimeProfiler ID assigned to the new control in \dev\Telemetry\RuntimeProfiler.h.
     Example:   ProfId_PersonPicture == 3
-->

## Gesture recognizer handling

<!-- List any use of UIElement.ManipulationXXX events or the gesture recognizer. -->

# Implementation details

<!-- List any particular algorithm, important data structures, asynchronous operations, timing considerations, etc... -->

Selection logic is very similar to Radio Buttons <br>
Pill animations (2 parts):
- pill done locally through template animation since no sliding between items is necessary
- backplate may require sliding animation between items (perhaps can leverage existing code in navigation view)
# Appendix
<!-- Anything else that you want to write down for posterity, but 
that isn't necessary to understand the functioning of the control. -->
