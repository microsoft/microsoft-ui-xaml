# PagerControl

PagerControl allows developers to show items by providing an easy way to handle the user interface of pagination.

## Architecture

The PagerControl is structured into different multiple areas:
- Handling of control events such as button clicks
- Property changes
- Updating NumberPanel buttons

Both the ComboBox and NumberPanel need collections to render that are not easily created through XAML as they require some logic.
To pass those collections to the visual layer, we use TemplateSettings. 
For performance reasons, the collections are stored inside the PagerControl and are not modified through the TemplateSettings object but rather through the reference inside the PagerControl.
Since the collections on the TemplateSettings are readonly, this way of accessing is always modifying the correct items.

## Template

Below are all the named template parts of the PagerControl template. Next to them is their use case inside the control.

- RootGrid: Referenced to listen to key events to allow accessible keyboarding behavior.
- FirstPageButton: Referenced to listen to click event and to add the localized UIA name.
- PreviousPageButton: Referenced to listen to click event and to add the localized UIA name.
- BoxPanels: Only referenced inside template to quickly hide/show non ButtonPanel controls.
- NumberBoxDisplay: Referenced to listen to value changed, add localized UIA name and update maximum value.
- ComboBoxDisplay: Referenced to listen to selected index changed and add localized UIA name.
- SuffixTextLabel: Only referenced inside the template to hide in infinity mode.
- TotalNumberOfPagesLabel: Only referenced inside the template to hide in infinity mode.
- NumberPanelItemsRepeater: Referenced to force layout updating in time and allow correct placement of selection indicator.
- NumberPanelCurrentPageIndicator: Referenced to update position based on the selected item.
- NextPageButton: Referenced to listen to click event and to add the localized UIA name.
- LastPageButton: Referenced to listen to click event and to add the localized UIA name.


## Flows

There are two areas of flows, 1. property changes and 2. user interactions.
Every user interaction essentially only modifies a single property (generally SelectedPageIndex).
Because of that, we will only discuss property change flows.

*Note:* Updating the TemplateSettings is done lazily, that means, we only update the collection that get's currently rendered.
If we are displaying the ComboBox, we only update the ComboBox collection and if we are showing the NumberPanel, we only update the NumberPanel item collection.
That's why we are updating the TemplateSettings not only when the NumberOfPages changes but also when the DisplayMode changes.

#### First/Previous/Next/Last-ButtonVisibility property
1. Reevaluate the visibility of the button in question

#### DisplayMode property
1. Switch to correct control to show
2. Update TemplateSettings collections

#### NumberOfPages property
1. Adjust SelectedPageIndex if it would be larger then NumberOfPages
2. Update TemplateSettings collections
3. Update DisplayMode Auto rendering if needed
4. Update NumberBox maximum
5. Reevaluate whether to show edge buttons (first/previous/next/last page buttons)

*Note:* If `NumberOfPages` is below 0, we enter "infinity mode".
Users can enter numbers as large as they want for the case of NumberBox and the NumberPanel will always allow getting the next number.
ComboBox will either show 100 items or show the last number of items when the previous value of NumberOfPages was greater than 100.
DisplayMode Auto will show a NumberBox. In "infinity mode", we do not show the last page button.

#### SelectedPageIndex property
1. Adjust SelectedPageIndex to be between 0 and NumberOfPages. In infinity mode we only ensure that the value is 0 or larger.
2. Adjust the selection of ComboBox or NumberBox respectively
3. Update edge buttons visibility
4. Update TemplateSettings collections
5. Move the selection indicator if necessary
6. Raise SelectedIndexChanged event on control and automation peer

#### ButtonPanelAlwaysShowFirstLastPage property
1. Update TemplateSettings collections for NumberPanel

## Accessibility
The PagerControlAutomationPeer implements the ISelectionProvider interface and raises the `SelectionPatternIdentifiers::SelectionProperty` changed event.
NumberBox, ComboBox and the buttons of the NumberPanel have localized UIA names provided.
The first,previous,next and last page buttons also have a localized UIA name set.

Users can use the left and right arrow keys to navigate the control.