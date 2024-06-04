# NavigationView Control

NavigationView is a powerful control that makes it easy to integrate a rich navigation experience into apps.
This document outlines overall structure and behaviors.
Information related to the NavigationViewItem, or the rendering of the control see the documents in the /NavigationView/docs folder.

## Architecture

NavigationView is a complex control, which has a lot of internal states to store.

Rendering of the menu items is being done through two different types of ItemsRepeaters.
There are 5 root ItemRepeaters, one for the MenuItems in left mode, one for the footer items in left mode,
one for the MenuItems in top mode, one for the footer items in top mode,
and one for the overflow menu in top mode.

In addition to this, in the case of hierarchical NavigationView, every NavigationViewItem with children has it's own ItemsRepeater
which is used to render the child items of that NavigationViewItem.

For more on this, see [the rendering document](./docs/rendering.md).

The selection is being tracked through a SelectionModel, which uses IndexPath tracking for NavigationViewItems.

Selection can be changed through 3 different ways:
- NavigationViewItem invoke (user interaction)
- NavigationViewItem.IsSelected property (API)
- NavigationView.SelectedItem property (API)

To allow this, every NavigationViewItem is aware of its parent NavigationView.

Keyboard navigation and item invocation are both handled by the NavigationView.

## Flows

### NavigationViewItemInvoked Selection

1. NavigationViewItem gets invoked
2. NavigationView gets informed that an item was invoked and raises ItemInvoked
2.5. NavigationViewChecks the Item's SelectOnInvoke property, if false we are done, if true:
3. NavigationView updates selection
4. NavigationView notifies the invoked NavigationViewItem about its new selection state
5. NavigationView raises SelectionChanged

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