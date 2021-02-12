# NavigationView rendering

This document talks about the UI of the NavigationView, and how it is achieved.

Since the rendering is different based on the displaymode, those will be looked at independently.

## ItemTemplates and NavigationViewItems

The NavigationView supports using a MenuItemsSource and using an ItemTemplate.

If the provided MenuItems are NavigationViewItems, and there is no ItemTemplate,the NavigationViewItems will be used as is.
In case of an ItemTemplate, the NavigationViewItem acts based on the provided template.
If the template returns a NavigationViewItem, the template will be used without any wrapper.
If the template does not return a NavigationViewItem (e.g. a Button), the returned items will be wrapped inside a NavigationViewItem.

In the next three sections, the template parts of the NavigationView will be explained.
The first section talks about items that are used regardless of the displaymode,
the second section is for items only needed in displaymode left,
while the third section is for displaymode top.

Note that the settings item will be rendered as an element in the FooterMenu items list.

## ControlTemplate

These are the necessary control templates used for both displaymodes and how they are used in code behind.

* `RootGrid`: Template root, used for keyboard navigation
* `PaneToggleButtonGrid`: Not used
* `TogglePaneTopPadding`: Referenced to adjust padding
* `ButtonHolderGrid`: Not used
* `NavigationViewBackButton`: Allows the BackRequested event
* `NavigationViewBackButtonToolTip`: Localization of back button ToolTip
* `NavigationViewCloseButton`: Close button functionality
* `NavigationViewCloseButtonToolTip`: Localization of close button ToolTip
* `TogglePaneButton`: Used for close/open pane functionality
* `PaneTitleTextBlock`: Pane title binding and rendering
* `PaneTitleHolder`: Holds pane title presenter
* `PaneTitlePresenter`: Renders pane title

When switching between top and left displaymode, the NavigationView "moves" certain items such as the PaneHeader and the AutoSuggestBox,
since UI items can only be added the VisualTree once, and we need them to be present in the area that is currently visible.

## DisplayMode Left

For left mode, the following control template parts are used:

* `RootSplitView`: Renders the content and left pane
* `PaneContentGrid`: Used for layouting of left pane, also needed for the animation
* `ItemsContainerRow`: Used for sizing of menu items/footer items
* `ContentPaneTopPadding`: Used for height adjustments
* `PaneHeaderContentBorderRow`: Referenced for sizing adjustments
* `PaneHeaderCloseButtonColumn`: Used for CompactPaneLength adjusting of pane close buttons
* `PaneHeaderToggleButtonColumn`: Used for CompactPaneLength adjusting of pane toggle buttons
* `PaneHeaderContentBorder`: Manual sizing for pane header
* `AutoSuggestArea`: Not referenced
* `PaneAutoSuggestBoxPresenter`: Used to check if there is content in autosuggest area
* `PaneAutoSuggestButton`: Used to open pane when button gets invoked
* `PaneCustomContentBorder`: Used for pane header rendering
* `ItemsContainerGrid`: Used for size calculations of menu and footer items views
* `MenuItemsScrollViewer`: Used to limit sizing of menu items host
* `MenuItemsHost`: Used for rendering of the NavigationViewItems
* `VisualItemsSeparator`: Referenced to dynamically show/hide the separator when needed
* `FooterContentBorder`: Used for rendering of pane footer
* `FooterMenuItemsHost`: Renders the footer menu items
* `ContentGrid`: Shadow handling and focus behavior
* `ContentTopPadding`: Not referenced
* `ContentLeftPadding`: Used for padding of the content
* `HeaderContent`: Not referenced

In DisplayMode left, the SplitView is used to show/hide the pane based on the displaymode and the width of the NavigationView's width.

In PaneDisplayMode `Auto`, the pane gets hidden based on its width.

## DisplayMode Top

For displaymode Top the following parts are used:
* `TopNavArea`: Not referenced
* `TopNavLeftPadding`: Not referenced
* `TopNavGrid`: Used for down level support
* `BackButtonPlaceholderOnTopNav`: Not referenced
* `PaneHeaderOnTopPane`: Used for rendering of header
* `PaneTitleOnTopPane`: Used for rendering of pane title
* `TopNavMenuItemsHost`: Renders the NavigationViewItems in top mode
* `TopNavOverflowButton`: Opens an overflow menu when there is not enough space to show all items
* `TopNavMenuItemsOverflowHost`: Renders the items in the overflow flyout
* `PaneCustomContentOnTopPane`: Renders the custom pane content
* `TopPaneAutoSuggestArea`: Takes up space for the search box
* `TopPaneAutoSuggestBoxPresenter`: Renders the AutoSuggestBox
* `TopFooterMenuItemsHost`: Renders the footer menu items
* `PaneFooterOnTopPane`: Shows the panefooter
* `TopNavContentOverlayAreaGrid`: Used for content overlays
