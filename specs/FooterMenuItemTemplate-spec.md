
<!-- The purpose of this spec is to describe a new feature and
its APIs that make up a new feature in WinUI. -->

<!-- There are two audiences for the spec. The first are people
that want to evaluate and give feedback on the API, as part of
the submission process.  When it's complete
it will be incorporated into the public documentation at
docs.microsoft.com (http://docs.microsoft.com/uwp/toolkits/winui/).
Hopefully we'll be able to copy it mostly verbatim.
So the second audience is everyone that reads there to learn how
and why to use this API. -->

# Background
<!-- Use this section to provide background context for the new API(s) 
in this spec. -->

<!-- This section and the appendix are the only sections that likely
do not get copied to docs.microsoft.com; they're just an aid to reading this spec. -->

<!-- If you're modifying an existing API, included a link here to the
existing page(s) -->

<!-- For example, this section is a place to explain why you're adding this API rather than
modifying an existing API. -->

<!-- For example, this is a place to provide a brief explanation of some dependent
area, just explanation enough to understand this new API, rather than telling
the reader "go read 100 pages of background information posted at ...". -->

The [`NavigationView.FooterMenuItems`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.controls.navigationview.footermenuitems?view=winui-2.5) property provides a way to place NavigationView items into a bottom-aligned (or right-aligned, if your NavigationView is in top mode) section of a NavigationView. This is distinct from the [`NavigationView.PaneFooter`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.controls.navigationview.panefooter?view=winui-2.5) property, as only Navigation items should be placed in the Footer menu, while any kind of content can be placed in the PaneFooter.

<img src="images/footermenu-example.png" height="400" alt="Image of a NavigationView with labels pointing to main menu items and footer menu items"/>

Currently, the DataTemplate that's assigned to the [`NavigationView.MenuItemTemplate`](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.controls.navigationview.menuitemtemplate?view=winui-2.5) property is applied to all Navigation items in the NavigationView. This means that if you add both regular NavigationView menu items and NavigationView footer menu items, they will automatically have the same data template applied. This is an issue as there's no current way to provide a standardized data template for all footer menu items that's separate/different from the data template used for all main menu items.

# Description
<!-- Use this section to provide a brief description of the feature.
For an example, see the introduction to the PasswordBox control 
(http://docs.microsoft.com/windows/uwp/design/controls-and-patterns/password-box). -->

In order to apply a data template to all items in the footer menu of your NavigationView, use the `NavigationView.FooterMenuItemTemplate` property. This property is similar to [NavigationView.MenuItemTemplate](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.controls.navigationview.menuitemtemplate?view=winui-2.5), but only applies to items placed in NavigationView's footer menu. It takes in a `DataTemplate` and applies that template to any Navigation item placed in the footer menu.

To apply different data templates to individual Navigation items within your footer menu based on certain criteria, use the `NavigationView.FooterMenuItemTemplateSelector`. This property is similar to the [NavigationView.MenuItemTemplateSelector](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.controls.navigationview.menuitemtemplateselector?view=winui-2.5), but only applies to items placed in NavigationView's footer menu.

The [NavigationView.MenuItemTemplate](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.controls.navigationview.menuitemtemplate?view=winui-2.5) will only apply to Navigation items that are placed in the main menu of a NavigationView. 


# Examples
<!-- Use this section to explain the features of the API, showing
example code with each description. The general format is: 
  feature explanation,
  example code
  feature explanation,
  example code
  etc.-->
  
<!-- Code samples should be in C# and/or C++/WinRT -->

<!-- As an example of this section, see the Examples section for the PasswordBox control 
(https://docs.microsoft.com/windows/uwp/design/controls-and-patterns/password-box#examples). -->

Example showing how to set `NavigationView.FooterMenuItemTemplate` in markup:

```xml
<muxc:NavigationView x:Name="NavView" SelectionChanged="NavView_SelectionChanged">
    <muxc:NavigationView.MenuItemTemplate>
        <DataTemplate>
            <muxc:NavigationViewItem Icon="Play" AutomationProperties.Name="MenuItemFromTemplate" Content="{Binding Content}"/>
        </DataTemplate>
    </muxc:NavigationView.MenuItemTemplate>
    <muxc:NavigationView.FooterMenuItemTemplate>
        <DataTemplate>
            <muxc:NavigationViewItem Icon="Stop" AutomationProperties.Name="FooterMenuItemFromTemplate" Content="{Binding Content}"/>
        </DataTemplate>
    </muxc:NavigationView.FooterMenuItemTemplate>
    <StackPanel>
        <TextBlock x:Name="SelectedItem"/>
    </StackPanel>
</muxc:NavigationView>

```

# Remarks
<!-- Explanation and guidance that doesn't fit into the Examples section. -->

<!-- APIs should only throw exceptions in exceptional conditions; basically,
only when there's a bug in the caller, such as argument exception.  But if for some
reason it's necessary for a caller to catch an exception from an API, call that
out with an explanation either here or in the Examples -->

# API Notes
<!-- Option 1: Give a one or two line description of each API (type
and member), or at least the ones that aren't obvious
from their name.  These descriptions are what show up
in IntelliSense. For properties, specify the default value of the property if it
isn't the type's default (for example an int-typed property that doesn't default to zero.) -->

<!-- Option 2: Put these descriptions in the below API Details section,
with a "///" comment above the member or type. -->

The following APIs are members of `NavigationView`.

| Name | Description |
| - | - |
| FooterMenuItemTemplate | Gets or sets the [DataTemplate](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.datatemplate) used to display each footer menu item. |
| FooterMenuItemTemplateSelector | Gets or sets a reference to a custom [DataTemplateSelector](https://docs.microsoft.com/uwp/api/windows.ui.xaml.controls.datatemplateselector) logic class. The DataTemplateSelector referenced by this property returns a template to apply to Navigation items placed in the footer menu.


# API Details
<!-- The exact API, in MIDL3 format (https://docs.microsoft.com/en-us/uwp/midl-3/) -->

```csharp
[MUX_PREVIEW]
    {
        Windows.UI.Xaml.DataTemplate FooterMenuItemTemplate { get; set; };
        Windows.UI.Xaml.Controls.DataTemplateSelector FooterMenuItemTemplateSelector { get; set; };
    }
```
# Appendix
<!-- Anything else that you want to write down for posterity, but 
that isn't necessary to understand the purpose and usage of the API.
For example, implementation details. -->
