# Background
> This spec corresponds to [issue 913](https://github.com/microsoft/microsoft-ui-xaml/issues/913) on the WinUI repo.

Users should be informed about essential status changes that occur on an app level.
These status changes affect the app as a whole and can be either critical or informational.
Critical status changes like lost internet connectivity are directly impactful to app functionality while
informational status changes like an update has completed and been applied are indirectly impactful to app functionality.
These notifications and corresponding information should be presented in a consistent, predictable, and relevant
way to the user depending on the specific scenario.


For example the "Error while saving" message in this sample app:

![initial example](images/initial-example.jpg)

Currently, [TeachingTip](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.TeachingTip), 
[ContentDialog](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.ContentDialog), 
and customizations of other [Flyouts](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.Flyout) 
and dialogs exist as options to show these notifications but these controls were not specifically designed 
to handle app-wide status change notifications. 
Due to their visual layouts, inherent intrusiveness, or available features they are not
sufficient for displaying notifications at an app-wide level.


This spec introduces an InfoBar WinUI (Xaml) control that an app can use for these kinds of messages.


# InfoBar Class
Represents an informational control that is visible but does not interfere with the user. 


## Is this the right control?
Use an InfoBar control when a user should be informed of, acknowledge, or take action on a changed application state. 
By default the notification will remain in the content area until closed by the user 
but will not necessarily break user flow.

An InfoBar will take up space in your layout and behave like any other child elements. 
It will not cover up other content or float on top of it.

Do not use an InfoBar control to confirm or respond directly to a user action that doesn't change the state of the app, for time-sensitive alerts, 
or for non-essential messages.

## Remarks
Use an InfoBar that is closed by the user or when the status is resolved for scenarios 
that **directly** impact app perception or experience


Here are some examples:
- Internet connectivity lost
- Error while saving a document when triggered automatically, not related to specific user action
- No microphone plugged in when attempting to record
- Can't connect to your phone
- The subscription to the application is expired


Use an InfoBar that is closed by the user for scenarios that **indirectly** impact app perception or experience

Here are some examples:
- A call has begun recording
- Update applied with link to 'Release Notes'
- The terms of service have been updated and require acknowledgement
- An app-wide backup has successfully, asynchronously completed
- The subscription to the application is close to expiring

## When should a different control be used?

There are some scenarios where a [ContentDialog](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.ContentDialog), 
[Flyout](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.Flyout), or 
[TeachingTip](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.TeachingTip) may be more appropriate to use.

- For scenarios where a persistent notification is not needed, e.g. displaying information in context of 
a specific UI element, a 
[Flyout](https://docs.microsoft.com/en-us/windows/uwp/design/controls-and-patterns/dialogs-and-flyouts/flyouts) 
is a better option. 
- For scenarios where the application is confirming a user action, showing information the user ***must*** read, use a
[ContentDialog](https://docs.microsoft.com/en-us/windows/uwp/design/controls-and-patterns/dialogs-and-flyouts/dialogs).
  - Additionally, if a status change to the app is so severe that it needs to block all further ability for the user to interact with the app, use a ContentDialog.
- For scenarios where a notification is a transient teaching moment, a 
[TeachingTip](https://docs.microsoft.com/en-us/windows/uwp/design/controls-and-patterns/dialogs-and-flyouts/teaching-tip) 
is a better option.


For more info about choosing the right notification control, see the 
[Dialogs and Flyouts](https://docs.microsoft.com/en-us/windows/uwp/design/controls-and-patterns/dialogs-and-flyouts/)
article.


# Examples

## Create an InfoBar
The XAML below describes an inline InfoBar with the default styling for an informational notification. 
An info bar can be placed like any other element and will follow base layout behavior.
For example, in a vertical StackPanel, the InfoBar will horizontally expand to 
fill the available width. 


By default, the InfoBar will not be visible. Set the IsOpen property to true in the XAML or code behind
to display the InfoBar.


XAML
```xml
<controls:InfoBar x:Name="UpdateAvailableNotification"
    Title="Update available."
    Message="Restart the application to apply the latest update.">
</controls:InfoBar>
```
C#
```C#
public MainPage()
{
    this.InitializeComponent();

    if(IsUpdateAvailable())
    {
        UpdateAvailableNotification.IsOpen = true;
    }
}
```

![A mockup of an InfoBar in the default state with a close button and a message](images/Default_TitleMessage.png)

## Using pre-defined severity levels
The type of the info bar can be set via the Severity property to automatically set a consistent status color, icon, and assistive technology settings dependent on the criticality of the notification. If no Severity is set, the default informational styling is applied.


XAML
```xml
<controls:InfoBar x:Name="SubscriptionExpiringNotification"
    Severity="Warning"
    Title="Your subscription is expiring in 3 days."
    Message="Renew your subscription to keep all functionality" />
```
![A mockup of an InfoBar in a Warning state with a close button and a message](images/Warning_TitleMessage.png)

## Programmatic close in InfoBar
An InfoBar can be closed by the user via the close button or programmatically. 
If the notification is required to be in view until the status is resolved and you would like to remove 
the ability for the user to close the info bar, you can set the IsClosable property to false.


The default value of this property is true, in which case the close button is present and takes the form of an 'X'.


XAML
```xml
<controls:InfoBar x:Name="NoInternetNotification"
    Severity="Error"
    Title="No Internet"
    Message="Reconnect to continue working."
    IsClosable="False" />
```
![Mockup of an InfoBar in an Error state with no close button](images/Error_NoClose.png)

## Customization: Background color and icon
Outside of the pre-defined severity levels, the Background and IconSource properties can be set 
to customize the icon and background color. The InfoBar will retain the assistive technology settings of 
the severity defined, or default if none was defined.


A custom background color can be set via the standard Background property and will override the color set by Severity.
Please keep in mind content readability and accessibility when setting your own color.


A custom icon can be set via the IconSource property. By default, an icon will be visible
(assuming the control isn't collapsed).
This icon can be removed by setting the IsIconVisible property to false. 
For custom icons, the recommended icon size is 20px.


XAML
```xml
<controls:InfoBar x:Name="CallRecordingNotification"
    Title="Recording started"
    Message="Your call has begun recording."
    Background="{StaticResource LavenderBackgroundBrush}">
    <controls:InfoBar.IconSource>
        <controls:SymbolIconSource Symbol="Phone" />
    </controls:InfoBar.IconSource>
</controls:InfoBar>
```

![A mockup of an InfoBar in the default state with a custom background color, custom icon, and close button](images/Custom_IconColor.png)

## Add an action button

An additional action button can be added by defining your own button that inherits 
[ButtonBase](https://docs.microsoft.com/en-us/uwp/api/Windows.UI.Xaml.Controls.Primitives.ButtonBase) 
and setting it in the ActionButton property. Custom styling will be applied to action buttons of type 
[Button](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.Button) 
and [HyperlinkButton](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.HyperlinkButton) 
for consistency and accessibility. Aside from the ActionButton property, 
additional action buttons can be added via custom content and will appear below the message.


XAML
```xml
<controls:InfoBar x:Name="NoInternetNotification"
    Severity="Error"
    Title="No Internet"
    Message="Reconnect to continue working.">
    <controls:InfoBar.ActionButton>
        <Button Content="Network Settings"
            Click="InternetInfoBarButton_Click" />
    </controls:InfoBar.ActionButton>
</controls:InfoBar>
```

![A mockup of an InfoBar in the Error state with a single line message and an action button](images/Error_ActionButton.png)

XAML
```xml
<controls:InfoBar x:Name="TermsAndConditionsUpdatedNotification"
    Title="Terms and Conditions Updated"
    Message="Dismissal of this message implies agreement to the updated Terms and Conditions. Please view the changes on our website.">
    <controls:InfoBar.ActionButton>
        <HyperlinkButton Content="Terms and Conditions Sep 2020" 
            NavigateUri="https://www.example.com"
            Click="UpdateInfoBarHyperlinkButton_Click" />
    <controls:InfoBar.ActionButton/>
</controls:InfoBar>
```
![A mockup of an InfoBar with a message expanding multiple lines and a hyperlink](images/Default_Hyperlink.png)

## Content wrapping
If the InfoBar content, excluding custom content, is unable to fit on a single horizontal line 
they will be laid out vertically. The Title, Message, and ActionButton — if present — 
will each appear on separate lines.


XAML
```xml
<controls:InfoBar x:Name="BackupCompleteNotification"
    Severity="Success"
    Title="Backup complete: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."  
    Message="All documents were uploaded successfully. Ultrices sagittis orci a scelerisque. Aliquet risus feugiat in ante metus dictum at tempor commodo. Auctor augue mauris augue neque gravida.">
    <controls:InfoBar.ActionButton>
        <Button Content="Action"
            Click="BackupInfoBarButton_Click" />
    </controls:InfoBar.ActionButton>
</controls:InfoBar>
```

![A mockup of an InfoBar in the Success state multiline title and message InfoBar](images/Success_ContentWrapping.png)


## Custom content
XAML content can be added to an InfoBar using the Content property. 
It will appear in its own line below the rest of the control content. 
The InfoBar will expand to fit the content defined.


XAML
```xml
<controls:InfoBar x:Name="BackupInProgressNotification"
    Title="Backup in progress"  
    Message="Your documents are being saved to the cloud"
    IsClosable="False">
    <controls:InfoBar.Content>
        <ProgressBar IsIndeterminate="True" Margin="0,0,0,6"/>
    </controls:InfoBar.Content>
</controls:InfoBar>
```

![A mockup of an InfoBar in its default state with an indeterminate progress bar](images/Default_CustomContent.gif)

## Lightweight styling

You can modify the default Style and ControlTemplate to give the control a unique appearance. 
For more info, see the 
[Light-weight styling section](https://docs.microsoft.com/en-us/windows/uwp/design/controls-and-patterns/xaml-styles#lightweight-styling)
of the 
[Styling controls](https://docs.microsoft.com/en-us/windows/uwp/design/controls-and-patterns/xaml-styles)
article.

For example, the following causes the title bar font size to be 22pt on a Page's InfoBars:

```xml
<Page.Resources>
    <x:Double x:Key="InfoBarTitleFontSize">22</x:Double>
</Page.Resources>
```

# Inputs and Accessibility

## UI Automation Patterns 
InfoBar will use a Pane control pattern for inline notifications and will implement a custom "information" Landmark.


### Keyboard Navigation 
Set the IsOpen property to true in the XAML or code behind to display the InfoBar. There is no 
predefined gesture or keyboard shortcut to open an InfoBar.

Tab navigation into the InfoBar proceeds through the actionable items. The "actionable items" are the action button, focusable elements in the custom content, and the Close button. Not all elements may be present in an InfoBar.
  - A user can also use the left and right arrow keys to navigate between the actionable items.
- To close the InfoBar the close button needs to be pressed.
  - Note: Escape will not close the InfoBar and the control does not handle that input.

### Assistive Technologies
InfoBar will leverage the existing APIs used by Windows Notifications. 

The behavior of the InfoBar will change for assistive technologies like Narrator 
depending on the Severity set by the developer. As Error and Warning InfoBars are intended to be used 
for scenarios that directly impact app experience they should interrupt the user more than InfoBars 
that are informational. 
View [NotificationProcessing ](https://docs.microsoft.com/en-us/windows/win32/api/uiautomationcore/ne-uiautomationcore-notificationprocessing) 
docs for more information on the varied intended behavior.


| Severity |  NotificationProcessing | Behavior in Screen Readers|
|:--- | :---| :---|
| Error | NotificationProcessing_ImportantAll| Add new item to the end of the queue. It doesn’t matter the source, what is currently speaking or any other items in the queue queued with a higher priority than normal text. <br><br> This will cause it to get spoken after the current utterance/string finishes. Meaning it won't interrupt the speech but it will queue itself next to be spoken, ahead of anything already queued. <br><br> Focus and keyboard will NOT interrupt or flush, pressing control will silence and flush all |
| Warning | NotificationProcessing_ImportantAll| See above |
| Success | Processing_All| Add new item to the end of the queue. <br><br> Will be added to the end of the queue meaning all existing queued text will need to be spoken before this text will be spoken. <br><br>Focus, keyboard and control will silence/flush them all
| Default | Processing_All | See above

#### Screen readers
- Entry behavior for InfoBars based on Severity:
  - Error and Warning: The most recently alerted InfoBar will take priority over other queued content and screen readers will say "Click up to move to new information from" + App Name + Notification Contents. 
    - The InfoBar message will not be silenced via keyboarding or focus.
  - Default and Success: The InfoBar will appear to the user after the current queued content is iterated through and screen readers will then say "Click up to move to new information from" + App Name + Notification Contents. 
    - The InfoBar message can be easily silenced via keyboarding, focus change, or the control key.
- For all InfoBars, Ctrl + Narrator + Up arrow will move focus to the first actionable item in the InfoBar after the user is notified and read the element in focus.
- A user can press F6 to navigate to the first actionable item in the InfoBar.
- For touch screen devices, swiping will navigate through all actionable items, regardless of group, in order. When Swiping on the last element in the notification, focus will move to the screen readers's fullscreen invisible Close Button and the user may double tap the screen to close the window. Swiping again will move focus out of the notification.

## Enter and Exit Usability
### Flashing content
The InfoBar should not appear and disappear from view rapidly to prevent flashing on the screen. Avoid flashing visuals for people with photosensitivities and to improve the usability of your application.


For InfoBars that automatically enter and exit the view via an app status condition, 
we recommend you include logic in your application to prevent content from appearing or disappearing rapidly 
or multiple times in a row. However, in general, this control should be used for long-lived status messages.


### Updating the InfoBar
Once the control is open, any changes made to the various properties like updating the message 
or Severity will not raise a notification event. If you would like to inform users that use screen readers of 
the InfoBar's updated content we recommend you close and re-open the control to trigger the event.


### Inline messages offsetting content
For InfoBars that are inline with other UI content, keep in mind how the rest of the page will 
responsively react to the addition of the element.


InfoBars with a substantial height could dramatically alter the layout of the other elements on the page. 
If the InfoBar appears or disappears rapidly, especially in succession, 
the user may be confused with the changing visual state.


# Globalization and Localization

## Color and Icon
When customizing the color and icon outside of the preset Severity levels, 
keep in mind user expectations for the connotations from the set of standard icons and colors.


Additionally, the preset Severity colors have already been designed for theme changes, 
high-contrast mode, color confusion accessibility, and contrast with foreground colors. 
We recommend to use these colors when possible and to include custom logic in your application to
adapt to the various color states and accessibility features.


Please view the UX guidance for
[Standard Icons](https://docs.microsoft.com/en-us/windows/win32/uxguide/vis-std-icons) 
and [Color](https://docs.microsoft.com/en-us/windows/win32/uxguide/vis-color) 
to ensure your message is communicated clearly and accessible to users.


### Severity
 Avoid setting the Severity property for a notification that does not match the information communicated in 
 the Title, Message, or custom content.
 

 The accompanying information should aim to communicate the following to use that Severity.
 - Error: An error or problem that has occurred.
 - Warning: A condition that might cause a problem in the future.
 - Success: A long-running and/or background task has completed.
 - Default: General information that requires the user's attention.

Icons and color should not be the only UI components signifying meaning for your notification. 
Text in the notification's Title and/or Message should be included to display information.


## Message 

Text in your notification will not be a constant length in all languages. For the Title and Message property 
this may impact whether your notification will expand to a second line. 
We recommend you avoid positioning based on message length or other UI elements set to a specific language.

The notification will follow standard mirroring behavior when localized to/from languages that are right to left (RTL) or left to right (LTR). The icon will only mirror if there is directionality.

Please view the guidance for 
[Adjust layout and fonts, and support RTL](https://docs.microsoft.com/en-us/windows/uwp/design/globalizing/adjust-layout-and-fonts--and-support-rtl) 
for more information about text localization in your notification.


# InfoBarPanel class

A layout panel that positions its children horizontally if there’s available space,
otherwise positions them vertically. This panel is intended to only be used as part
of the ControlTemplate of the InfoBar control.

## InfoBar padding properties

The `HorizontalOrientationPadding` property gets/sets the distance between the edges
of the panel and its children, when the panel is laying out items horizontally.
The `VerticalOrientationPadding` property does likewise when the panel is laying out
items vertically.

[API note: these padding properties are analogous to ex the 
[StackPanel.Padding](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.StackPanel.Padding)
property.]

## InfoBar margin attached properties

The `HorizontalOrientationMargin` attached property can be set on child elements
of an InfoBarPanel, and gets/sets an extra margin on the element.
Similarly 
`VerticalOrientationMargin` for vertical layout.

These attached margin properties are applied on an element in addition to the
element's [Margin](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.FrameworkElement.Margin)
property. For example if a child element's Margin is 2 and its
InfoBarPanel.HorizontalOrientationMargin property is 3, it will have an effective margin
of 5 (when the panel is in its horizontal layout).

The leading and trailing margins are ignored  however.
For example, in horizontal layout, the HorizontalOrientationMargin.Left
is ignored on the first child, and the 
HorizontalOrientationMargin.Right is ignored on the last child.
This applies to the first/last child that are not collapsed (that take up layout space).
For example, if the first child is collapsed, it's the
HorizontalOrientationMargin.Left of the _second_ child that is ignored.

## InfoBarPanel example

This example shows an InfoBarPanel with 5px of padding above and below
its children when laying out horizontally, no padding otherwise. 

The children have spacing between each other that varies based on the child
and orientation. The second child (the green rectangle) won't have it its margin
applied if the first child (the red rectangle) is collapsed.

```xml
<Border BorderBrush="Black" BorderThickness="1" Margin="100" HorizontalAlignment="Left">
    <primitives:InfoBarPanel HorizontalOrientationPadding='0,5,0,5' >

        <Rectangle Width="100" Height="100" Fill="Red" />

        <Rectangle Width="100" Height="100" Fill="Green"
            primitives:InfoBarPanel.HorizontalOrientationMargin='16,0,0,0'
            primitives:InfoBarPanel.VerticalOrientationMargin='0,20,0,0'/>

        <Rectangle Width="100" Height="100" Fill="Blue"
            primitives:InfoBarPanel.HorizontalOrientationMargin='32,0,0,0'
            primitives:InfoBarPanel.VerticalOrientationMargin='0,20,0,0' />

    </primitives:InfoBarPanel>
</Border>
```

![InfoBarPanel example](images/InfoBarPanel-example.jpg)

# API Notes

### Notable Properties  
| Name | Description |
|:-:|:--|
| IsOpen| Gets or sets a value that determines the visibility of the InfoBar. By default, is set to false. |
| Severity | Gets or sets a value that indicates the  color and icon to style the InfoBar. It will also define the assistive technology settings. |
| IsClosable| Gets or sets a boolean that indicates whether the user will be able to close the InfoBar.


### Events  
| Name | Description |
|:-:|:--|
| CloseButtonClick | Occurs after the close button has been clicked. |
| Closed | Occurs after the info bar is closed. |
| Closing |Occurs just before the info bar begins to close. |


# API Details

## Microsoft.UI.Xaml.Controls

```c++
enum InfoBarCloseReason
{
    CloseButton = 0,
    Programmatic = 1,
};

enum InfoBarSeverity
{
    Informational = 0,
    Success = 1,
    Warning = 2,
    Error = 3,
}

runtimeclass InfoBarClosedEventArgs
{
    InfoBarCloseReason Reason{ get; };
};

runtimeclass InfoBarClosingEventArgs
{
    InfoBarCloseReason Reason{ get; };
    Boolean Cancel;
};

unsealed runtimeclass InfoBarTemplateSettings : Windows.UI.Xaml.DependencyObject
{
    InfoBarTemplateSettings();
    Windows.UI.Xaml.Controls.IconElement IconElement;

    static Windows.UI.Xaml.DependencyProperty IconElementProperty{ get; };
}

unsealed runtimeclass InfoBar : Windows.UI.Xaml.Controls.ContentControl
{
    InfoBar();

    String Title;
    String Message;

    Boolean IsOpen;
    Boolean IsClosable;
    Boolean IsIconVisible;

    Windows.UI.Xaml.Controls.Primitives.ButtonBase ActionButton;

    Windows.UI.Xaml.Style CloseButtonStyle;
    Windows.UI.Xaml.Input.ICommand CloseButtonCommand;
    Object CloseButtonCommandParameter;

    InfoBarSeverity Severity;
    IconSource IconSource;

    InfoBarTemplateSettings TemplateSettings{ get; };
    Object Content{ get; set; };
    Windows.UI.Xaml.DataTemplate ContentTemplate{ get; set; };

    event Windows.Foundation.TypedEventHandler<InfoBar, Object> CloseButtonClick;
    event Windows.Foundation.TypedEventHandler<InfoBar, InfoBarClosingEventArgs> Closing;
    event Windows.Foundation.TypedEventHandler<InfoBar, InfoBarClosedEventArgs> Closed;

    static Windows.UI.Xaml.DependencyProperty IsOpenProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty TitleProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty MessageProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty SeverityProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty IconSourceProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty IsIconVisibleProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty IsClosableProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty CloseButtonStyleProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty CloseButtonCommandProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty CloseButtonCommandParameterProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty ActionButton{ get; };
    static Windows.UI.Xaml.DependencyProperty ContentProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty ContentTemplateProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty TemplateSettingsProperty{ get; };
}
```

## Microsoft.UI.Xaml.Controls.Primitives

```cs
[webhosthidden]
unsealed runtimeclass InfoBarPanel : Windows.UI.Xaml.Controls.Panel
{
    InfoBarPanel();

    // HorizontalOrientationPadding property
    Windows.UI.Xaml.Thickness HorizontalOrientationPadding;
    static Windows.UI.Xaml.DependencyProperty HorizontalOrientationPaddingProperty{ get; };

    // VerticalOrientationPadding property
    Windows.UI.Xaml.Thickness VerticalOrientationPadding;
    static Windows.UI.Xaml.DependencyProperty VerticalOrientationPaddingProperty{ get; };

    // HorizontalOrientationMargin attached property
    static void SetHorizontalOrientationMargin(Windows.UI.Xaml.DependencyObject object, Windows.UI.Xaml.Thickness value);
    static Windows.UI.Xaml.Thickness GetHorizontalOrientationMargin(Windows.UI.Xaml.DependencyObject object);
    static Windows.UI.Xaml.DependencyProperty HorizontalOrientationMarginProperty{ get; };

    // VerticalOrientationMargin attached property
    static void SetVerticalOrientationMargin(Windows.UI.Xaml.DependencyObject object, Windows.UI.Xaml.Thickness value);
    static Windows.UI.Xaml.Thickness GetVerticalOrientationMargin(Windows.UI.Xaml.DependencyObject object);
    static Windows.UI.Xaml.DependencyProperty VerticalOrientationMarginProperty{ get; };
}
```

## Theme Resources

### Notable Theme Resources
| Name | Description |
|:-:|:--|
|InfoBarHyperlinkForeground | Sets the hyperlink button text color. <br> - Note: This is set to keep hyperlinks accessible on the variously colored backgrounds defined by the severity background colors.
|InfoBarCloseButtonStyle | Sets the close button style of the InfoBar and contains the following properties: <br> - Width <br> - Height <br> - VerticalAlignment <br> - Background <br> - Margin <br> - CornerRadius
|InfoBarIconFontSize | Sets the icon size of the InfoBar. <br> - Note: Custom icons set via the IconSource property will not resize to 20px unless they are in the standard symbol set.

### All Theme Resources
| Resource Key | Description |
|:-:|:--|
| InfoBarErrorSeverityBackgroundBrush | Error Severity background color 
| InfoBarWarningSeverityBackgroundBrush | Warning Severity background color 
| InfoBarSuccessSeverityBackgroundBrush | Success Severity background color 
| InfoBarInformationalSeverityBackgroundBrush | Informational Severity background color
| InfoBarErrorSeverityIconForeground | Error Severity icon color
| InfoBarWarningSeverityIconForeground | Warning Severity icon color
| InfoBarSuccessSeverityIconForeground | Success Severity icon color
| InfoBarInformationalSeverityIconForeground | Informational Severity icon color
| InfoBarTitleForeground | Title text color
| InfoBarMessageForeground | Message text color 
| InfoBarHyperlinkButtonForeground | Hyperlink button text color
| InfoBarBorderBrush | Border color
| InfoBarBorderThickness | Border thickness
| InfoBarTitleFontSize | Title text font size 
| InfoBarTitleFontWeight | Title text font weight
| InfoBarMessageFontSize | Message text font size
| InfoBarMessageFontWeight | Message text font weight
| InfoBarMinHeight | Minimum height of InfoBar
| InfoBarCloseButtonSize | Close button size
| InfoBarCloseButtonGlyphSize | Close button glyph size
| InfoBarInformationalIconGlyph | Informational Severity icon glyph
| InfoBarErrorIconGlyph | Error Severity icon glyph
| InfoBarWarningIconGlyph | Warning Severity icon glyph
| InfoBarSuccessIconGlyph | Success Severity icon glyph
| InfoBarContentRootPadding | Padding thickness for Content property
| InfoBarIconMargin | Icon margin thickness
| InfoBarIconFontSize | Icon font size
| InfoBarPanelMargin | Panel margin thickness
| InfoBarPanelHorizontalLayoutMargin | Horizontal, single-line, layout panel margin thickness
| InfoBarPanelVerticalLayoutMargin | Vertical, multiline, layout panel margin thickness
| InfoBarTitleHorizontalLayoutMargin | Horizontal, single-line, layout title margin thickness
| InfoBarTitleVerticalLayoutMargin | Vertical, multiline, layout title margin thickness
| InfoBarMessageHorizontalLayoutMargin | Horizontal, single-line, layout message margin thickness
| InfoBarMessageVerticalLayoutMargin | Vertical, multiline, layout message margin thickness
| InfoBarActionHorizontalLayoutMargin | Horizontal, single-line, layout action button margin thickness
| InfoBarActionVerticalLayoutMargin | Vertical, multiline, layout action button margin thickness
| InfoBarActionButtonMinWidth | Action button minimum width
| InfoBarActionButtonHeight | Action button height
| InfoBarActionButtonPadding | Action button padding thickness
| InfoBarActionButtonCornerRadius | Action button corner radius
| InfoBarHyperlinkButtonFontSize | Hyperlink button font size
| InfoBarHyperlinkButtonHeight | Hyperlink button height
| InfoBarHyperlinkButtonPadding | Hyperlink button padding thickness
| InfoBarCloseButtonSymbol | Close button symbol glyph
| InfoBarCloseButtonStyle | Close button style


# Appendix

## Design References
UI Elements for InfoBar


![Toolkit page for InfoBar UI elements](images/Docked_Toolkit.png)

### Visual Components
| Component |  Notes |
|:---:|:---|
| Container | - We recommend to place InfoBars in a layout control where the control can expand horizontally to the width of the content area.
| Icon | - Defined by either the Severity or by IconSource <br> - Can be removed by setting IsIconVisible to false
| Title | - Semi-bolded and appears right of the Icon <br> - Recommended to be 50 characters or less
| Message | - Will appear to the right of the Title in single-height notifications, otherwise will be on a new line <br> - Recommended to be 512 characters or less
| Action button |  - Will appear directly to the right of the Message in single height notifications, otherwise will be on its own new line. <br> - An action button can be any class that inherits ButtonBase however Button, HyperlinkButton, and Checkbox are our recommended options <br> - Additional action buttons may be added through custom XAML content
| Close button | - Will appear as 'X' by default <br> - Can be removed by setting IsClosable to false
| Content | - Can be customizable to include text, hyperlinks, and any other XAML content <br> - Appears in its own line underneath the other InfoBar content

## Behavioral Components
| Property | Notes |
|:---:|:---|
| Opening | - An info bar is shown by setting its IsOpen property to true. |
| Closing | There are two ways an info bar can close: <br>- The program sets the IsOpen property to false <br> - The user invokes the Close button. <br> Use the InfoBarCloseReason to determine which case has occurred. <br> Closing can be prevented by setting the Cancel property to true. You can use a deferral to respond asynchronously to the event. |

### Canceling close
The Closing event can be used to cancel and/or defer the close of an InfoBar. 
This can be used to keep the InfoBar open or allow time for a custom action to occur. 
When the closing of an InfoBar is canceled, IsOpen will go back to true. A programmatic close can also be canceled.


XAML
```xml
<controls:InfoBar x:Name="UpdateAvailable"
    Title="Update Available"
    Message="Please close this tip to apply required security updates to this application"
    Closing="InfoBar_Closing">
</controls:InfoBar>
```
C#
```C#
public void InfoBar_Closing(InfoBar sender, InfoBarClosingEventArgs args)
{
    if (args.Reason == InfoBarCloseReason.CloseButton) {
        if (!ApplyUpdates()) {
            // could not apply updates - do not close
            args.Cancel = true;
        }
    }
    
}
```

## Data and Intelligence Metrics
Feature has received enough feedback to exit preview
- Two or more preview or production applications are using InfoBar in prerelease and providing experience feedback.
- Multiple applications are using InfoBar, measured via the occurence of the InfoBar type via telemetry and user engagement.


The feature's guidance and intended usage is consistent with design expectations
- Evaluate how long an InfoBar is displayed before closed correlated to the set Severity level to influence future usage guidance. 
  - Measured by the time delta between InfoBar.IsOpen = true/false via telemetry
- Evaluate how often F6 is used to navigate to an InfoBar to track feature usage.
  - Measured by the number of times F6 is used to navigate to an InfoBar's action button or close button via telemetry
- Evaluate how often color and/or icon customization occurs to investigate other built-in Severity types.
  - Measured by the number of times the IconSource and/or Background property is set via telemetry.
- Evaluate how often multiple InfoBars are on-screen at once, correlated to the set Severity level to influence future usage guidance.
  - Measured by the number of InfoBar's defined in a single application and whether isOpen is set to true on both of them at the same time. (Stretch metric)

## Features in consideration for InfoBar v2
- Truncation option that allows the user to expand and collapse an InfoBar with multiple lines of content
- Potentially, an ActionContentArea to insert custom content in the same horizontal space as the other UI elements.
