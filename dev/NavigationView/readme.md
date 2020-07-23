# NavigationView Control

NavigationView is a powerful control makes it easy to integrate navigation into apps.
This document outlines overall structure and behaviors.
Information related to the NavigationViewItem, or the rendering of the control see the documents in the /NavigationView/docs folder.

## Architecture

NavigationView is a complex control, which has a lot of internal states to store.

Rendering of the menu items is being done through two ItemsRepeaters, one for the NavigationView in left mode, and one for the NavigationView in top mode.
For more on this, see [the rendering document](./docs/rendering.md).

The selection is being tracked through a SelectionModel, which uses IndexPath tracking for hierarchical NavigationViewItems.
Every NavigationViewItem can be selected by either setting the `IsSelected` property of a NavigationViewItem, or setting the selected item.
To allow this, every NavigationViewItem is aware of its parent NavigationView.

Keyboard navigation is done by the NavigationView, however handling of ItemInvokation is being handled by both the NavigationView and the NavigationViewItem.

## Flows

### NavigationViewItemInvoked Selection

1. NavigationViewItem gets invoked
2. NavigationView gets informed that an item was invoked
3. NavigationView raises ItemInvoked
4. NavigationView updates selection
5. NavigationView notifies the invoked NavigationViewItem about its new selection state
6. NavigationView raises SelectionChanged

If the NavigationViewItem has "SelectOnInvoke" set to false, the selection will not be updated:

1. NavigationViewItem gets invoked
2. NavigationView gets informed that an item was invoked
3. NavigationView raises ItemInvoked

### Selection update from API (SelectedItem property)

1. SelectedItem gets set
2. SelectionModel updates internal selection
3. Previously selected NavigationViewItem gets unselected
3. New selected NavigationViewItem gets selected
4. Update UI (SelectionIndicator, Flyouts)


### Compact pane length change
1. The NavigationView gets notified it compact pane length changes
2. Adjusts the pane length buttons widths
3. Tells all of the NavigationViewItems to adjust their icon column width


### Accessibility

To ensure accessibility, both the NavigationView, and the NavigationViewItem provide their own AutomationPeers and implement the needed pattern.

Keyboard accessible navigation is being handled by the NavigationView.