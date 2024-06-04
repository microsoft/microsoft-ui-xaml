# NavigationViewItem

This doc covers the areas the individual NavigationViewItems are handling.

## Architecture

There is the `NavigationViewItemBase` class, which is the base of all NavigationViewItems.
The base NavigationViewItem handles basic events and properties that are needed for the NavigationViewItem.

## Rendering
In addition to inheriting from NavigationViewItemBase, the NavigationViewItem renders itself throught the NavigationViewItemPresenter.

For each of the different displaymodes of the NavigationView, the NavigationViewItemPresenter has its own specific style, which gets applied depending on the displaymode of the NavigationView.

Each individual NavigationViewItem is responsible for rendering its own menu items, in the case of hierarchical NavigationView.

In the case of PaneDisplayMode `Top` and `LeftCompact`, the hierarchical NavigationViewItems will get rendered inside a MenuFlyout.
In the case of display mode left, the items are being rendered through an ItemsRepeater.

The template of the NavigationViewItem consist of the following parts:

* `NVIRootGrid`: Root grid
* `NavigationViewItemPresenter`: This handes the actual rendering of the item itself
* `NavigationViewItemMenuItemsHost`: Used for rendering of child items
* `ChildrenFlyout`: Flyout used for children when in top NavigationView
