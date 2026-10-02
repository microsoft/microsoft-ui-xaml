# Teaching tip

A teaching tip is a semi-persistent and content-rich flyout that provides contextual information. It is often used for informing, reminding, and teaching users about important and new features that may enhance their experience.

**Important APIs:** [TeachingTip class](https://docs.microsoft.com/en-us/uwp/api/microsoft.ui.xaml.controls.teachingtip)

A teaching tip may be light-dismiss or require explicit action to close. A teaching tip can target a specific UI element with its tail and also be used without a tail or target.

## Is this the right control? 

Use a **TeachingTip** control to focus a user's attention on new or important updates and features, remind a user of nonessential options that would improve their experience, or teach a user how a task should be completed. 

Because teaching tip is transient, it would not be the recommended control for prompting users about errors or important status changes.


## Examples

<table>
<th align="left">XAML Controls Gallery<th>
<tr>
<td><img src="images/xaml-controls-gallery-sm.png" alt="XAML controls gallery"></img></td>
<td>
    <p>If you have the <strong style="font-weight: semi-bold">XAML Controls Gallery</strong> app installed, click here to <a href="xamlcontrolsgallery:/item/TeachingTip">open the app and see the TeachingTip in action</a>.</p>
    <ul>
    <li><a href="https://www.microsoft.com/store/productId/9MSVH128X2ZT">Get the XAML Controls Gallery app (Microsoft Store)</a></li>
    <li><a href="https://github.com/Microsoft/Xaml-Controls-Gallery">Get the source code (GitHub)</a></li>
    </ul>
</td>
</tr>
</table>

A teaching tip can have several configurations, including these notable ones.

A teaching tip can target a specific UI element with its tail to enhance contextual clarity of the information it is presenting. 

![A sample app with a teaching tip targeting the save button. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to." There is a close button on the top right corner of the teaching tip.](images/teaching-tip-targeted.png)

When the information presented does not pertain to a particular UI element, a nontargeted teaching tip can be created by removing the tail.

![A sample app with a teaching tip in the bottom right corner. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to." There is a close button on the top right corner of the teaching tip.](images/teaching-tip-non-targeted.png)

A teaching tip can require the user to dismiss it via an "X" button in a top corner or a "Close" button at the bottom. A teaching tip may also be light-dismiss enabled in which case there is no dismiss button and the teaching tip will instead dismiss when a user scrolls or interacts with other elements of the application. Because of this behavior, light-dismiss tips are the best solution when a tip needs to be placed in a scrollable area. 

![A sample app with a light-dismiss teaching tip in the bottom right corner. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to."](images/teaching-tip-light-dismiss.png)


### Create a teaching tip

Here's the XAML for a targeted teaching tip control that demonstrates the default look of the TeachingTip with a title and subtitle. 
Note that the teaching tip can appear anywhere in the element tree or code behind. In this example below, it's located in a ResourceDictionary.

XAML
```XAML
<Button x:Name="SaveButton" Content="Save">
    <Button.Resources>
        <controls:TeachingTip x:Name="AutoSaveTip"
            Target="{x:Bind SaveButton}"
            Title="Save automatically"
            Subtitle="When you save your file to OneDrive, we save your changes as you go - so you never have to.">
        </controls:TeachingTip>
    </Button.Resources>
</Button>
```

C#
```C#
public MainPage()
{
    this.InitializeComponent();

    if(!HaveExplainedAutoSave())
    {
        AutoSaveTip.IsOpen = true;
        SetHaveExplainedAutoSave();
    }
}
```

Here's the result when the Page containing the button and teaching tip is shown:

![A sample app with a teaching tip targeting the save button. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to." There is a close button on the top right corner of the teaching tip.](images/teaching-tip-targeted.png)

### Non-targeted tips

Not all tips relate to an element onscreen. For these scenarios, do not set the Target property and the teaching tip will instead display relative to the edges of the xaml root. However, a teaching tip can have the tail removed while retaining placement relative to a UI element by setting the TailVisibility property to "Collapsed". The following example is of a non-targeted teaching tip.

XAML
```XAML
<Button x:Name="SaveButton" Content="Save" />

<controls:TeachingTip x:Name="AutoSaveTip"
    Title="Saving automatically"
    Subtitle="We save your changes as you go - so you never have to.">
</controls:TeachingTip>
```

Note that in this example the TeachingTip is in the element tree rather than in a ResourceDictionary or in code behind. This has no effect on behavior; the TeachingTip only displays when opened, and takes up no layout space.

![A sample app with a teaching tip in the bottom right corner. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to." There is a close button on the top right corner of the teaching tip.](images/teaching-tip-non-targeted.png)

### Preferred placement

Teaching tip replicates Flyout's [FlyoutPlacementMode](https://docs.microsoft.com/uwp/api/Windows.UI.Xaml.Controls.Primitives.FlyoutPlacementMode) placement behavior with the TeachingTipPlacementMode property. The default placement mode will try to place a targeted teaching tip above its target and a non-targeted teaching tip centered at the bottom of the xaml root. As with Flyout, if the preferred placement mode would not leave room for the teaching tip to show, another placement mode will be automatically chosen. 

For applications that predict gamepad input, please see [gamepad and remote control interactions]( https://docs.microsoft.com/en-us/windows/uwp/design/input/gamepad-and-remote-interactions#xy-focus-navigation-and-interaction). It is encouraged to test gamepad accessibility of each teaching tip using all possible configurations of an app's UI.

A targeted teaching tip with its PreferredPlacement set to "BottomLeft" will appear with the tail centered at the bottom of its target with the teaching tip's body shifted toward the left.

XAML
```XAML
<Button x:Name="SaveButton" Content="Save">
    <Button.Resources>
        <controls:TeachingTip x:Name="AutoSaveTip"
            Target="{x:Bind SaveButton}"
            Title="Saving automatically"
            Subtitle="We save your changes as you go - so you never have to."
            PreferredPlacement="BottomLeft">
        </controls:TeachingTip>
    </Button.Resources>
</Button>
```

![A sample app with a "Save" button that is being targeted by a teaching tip underneath its left corner. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to." There is a close button on the top right corner of the teaching tip.](images/teaching-tip-targeted-preferred-placement.png)


A non-targeted teaching tip with its PreferredPlacement set to "BottomLeft" will appear in the bottom left corner of the xaml root.

XAML
```XAML
<Button x:Name="SaveButton" Content="Save" />

<controls:TeachingTip x:Name="AutoSaveTip"
    Title="Saving automatically"
    Subtitle="We save your changes as you go - so you never have to."
    PreferredPlacement="BottomLeft">
</controls:TeachingTip>
```

![A sample app with a teaching tip in the bottom left corner. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to." There is a close button on the top right corner of the teaching tip.](images/teaching-tip-non-targeted-preferred-placement.png)

The diagram below depicts the result of all 13 PreferredPlacement modes that can be set for targeted teaching tips.
![Illustration containing 13 teaching tips, each demonstrating a different targeted placement mode. Each teaching tip is labeled with the mode it represents. The first word of a placement mode indicates the side of the target that the teaching tip will appear centered on. The tail of the teaching tip will always be located at the center of that side of the target and will point towards the target. If there is a second word in the placement mode, the body of the teaching tip will not be centered, but will instead be shifted in the specified direction. For example, the placement mode "TopRight" will make the teaching tip appear above the target and shifted right, with its tail pointing down at the center of the target's top edge. Because the body is shifted right, the tail is almost on the leftmost edge of the body of the teaching tip, and the teaching tip extends past the right edge of the target. The placement mode "Center" in unique and will make the teaching tip's tail point at the center of the target and the teaching tip will be centered over the top half of the target.](images/teaching-tip-targeted-preferred-placement-modes.png)

The diagram below depicts the result of all 13 PreferredPlacement modes that can be set for non-targeted teaching tips.
![Illustration containing nine teaching tips, each demonstrating a different non-targeted placement mode. Each teaching tip is labeled with the mode it represents. The first word of a placement mode indicates the side of the xaml root that the teaching tip will appear centered on. If there is a second word in the placement mode, the teaching tip will position itself toward that specified corner of the xaml root. For example, the placement mode "TopRight" will make the teaching tip appear in the top right corner of the xaml root. For non-targeted placement modes, the order of the two words does not affect the placement. TopRight is equivalent to RightTop. The placement mode "Center" in unique and will make the teaching tip appear in the vertical and horizontal center of the xaml root.](images/teaching-tip-non-targeted-preferred-placement-modes.png)

### Add a placement margin  

You can control how far a targeted teaching tip is set apart from its target and how far a non-targeted teaching tip is set apart from the edges of the xaml root by using the PlacementMargin property. Like [Margin](https://docs.microsoft.com/en-us/uwp/api/windows.ui.xaml.frameworkelement.margin), PlacementMargin has four values – left, right, top, and bottom – so only the relevant values are used. For example, PlacementMargin.Left applies when the tip is left of the target or on the left edge of the xaml root.

The following example shows a non-targeted tip with the PlacementMargin’s Left/Top/Right/Bottom all set to 80.

XAML
```XAML
<Button x:Name="SaveButton" Content="Save" />

<controls:TeachingTip x:Name="AutoSaveTip"
    Title="Saving automatically"
    Subtitle="We save your changes as you go - so you never have to."
    PreferredPlacement="BottomLeft"
    PlacementMargin="80">
</controls:TeachingTip>
```

![A sample app with a teaching tip positioned toward, but not fully against, the bottom right corner. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to." There is a close button on the top right corner of the teaching tip.](images/teaching-tip-placement-margin.png)


### Add content

Content can be added to a teaching tip using the Content property. If there is more content to show than what the size of a teaching tip will allow, a scrollbar will be automatically enabled to allow a user to scroll the content area. 

XAML
```XAML
<Button x:Name="SaveButton" Content="Save">
    <Button.Resources>
        <controls:TeachingTip x:Name="AutoSaveTip"
            Target="{x:Bind SaveButton}"
            Title="Saving automatically"
            Subtitle="We save your changes as you go - so you never have to.">
                <StackPanel>
                    <CheckBox x:Name="HideTipsCheckBox" Content="Don't show tips at start up" IsChecked="{x:Bind HidingTips, Mode=TwoWay}" />
                    <TextBlock>You can change your tip preferences in <Hyperlink NavigateUri="app:/item/SettingsPage">Settings</Hyperlink> if you change your mind.</TextBlock>
                </StackPanel>
        </controls:TeachingTip>
    </Button.Resources>
</Button>
```

![A sample app with a teaching tip targeting the save button. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to." In the content area of the teaching tip is a CheckBox labeled "Don't show tips at startup" and underneath is text that reads "You can change your tip preferences in Settings if you change your mind" where "Settings" is a link to the app's settings page. There is a close button on the top right corner of the teaching tip.](images/teaching-tip-content.png)

### Add buttons

By default, a standard "X" close button is shown next to the title of a teaching tip. The Close button can be customized with the CloseButtonContent property, in which case the button is moved to the bottom of the teaching tip.

**Note: No close button will appear on light-dismiss enabled tips**

A custom action button can be added by setting ActionButtonContent property (and optionally the ActionButtonCommand and the ActionButtonCommandParameter properties).

XAML
```XAML
<Button x:Name="SaveButton" Content="Save">
    <Button.Resources> 
        <controls:TeachingTip x:Name="AutoSaveTip"
            Target="{x:Bind SaveButton}"
            Title="Saving automatically"
            Subtitle="We save your changes as you go - so you never have to."
            ActionButtonContent="Disable"
            ActionButtonCommand="DisableAutoSave"
            CloseButtonContent="Got it!">
                <StackPanel>
                    <CheckBox x:Name="HideTipsCheckBox" Content="Don't show tips at start up" IsChecked="{x:Bind HidingTips, Mode=TwoWay}" />
                    <TextBlock>You can change your tip preferences in <Hyperlink NavigateUri="app:/item/SettingsPage">Settings</Hyperlink> if you change your mind.</TextBlock>
                </StackPanel>
        </controls:TeachingTip>
    </Button.Resources>
</Button>
```

![A sample app with a teaching tip targeting the save button. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to." In the content area of the teaching tip is a CheckBox labeled "Don't show tips at startup" and underneath is text that reads "You can change your tip preferences in Settings if you change your mind" where "Settings" is a link to the app's settings page. At the bottom of the teaching are two buttons, a gray one on the left that reads "Disable" and a blue one on the right that reads "Got it!"](images/teaching-tip-buttons.png)

### Hero content

Edge to edge content can be added to a teaching tip by setting the HeroContent property. The location of hero content can be set to the top or bottom of a teaching tip by setting the HeroContentPlacement property.

XAML
```XAML
<Button x:Name="SaveButton" Content="Save">
    <Button.Resources> 
        <controls:TeachingTip x:Name="AutoSaveTip"
            Target="{x:Bind SaveButton}"
            Title="Saving automatically"
            Subtitle="We save your changes as you go - so you never have to.">
            <controls:TeachingTip.HeroContent>
                <Image Source="Assets/cloud.png" />
            </controls:TeachingTip.HeroContent>
        </controls:TeachingTip>
    </Button.Resources>
</Button>
```

![A sample app with a teaching tip targeting the save button. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to." At the bottom of the teaching tip is a border-to-border image of a cartoon man putting files in the cloud. There is a close button on the top right corner of the teaching tip.](images/teaching-tip-hero-content.png)

### Add an icon

An icon can be added beside the title and subtitle using the IconSource property. Recommended icon sizes include 16px, 24px, and 32px. 

XAML
```XAML
<Button x:Name="SaveButton" Content="Save">
    <Button.Resources>
        <controls:TeachingTip x:Name="AutoSaveTip"
            Target="{x:Bind SaveButton}"
            Title="Saving automatically"
            Subtitle="We save your changes as you go - so you never have to."
            <controls:TeachingTip.IconSource>
                <controls:SymbolIconSource Symbol="Save" />
            </controls:TeachingTip.IconSource>
        </controls:TeachingTip>
    </Button.Resources>
</Button>
```

![A sample app with a teaching tip targeting the save button. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to." To the left of the title and subtitle is a floppy disk icon. There is a close button on the top right corner of the teaching tip.](images/teaching-tip-icon.png)

### Enable light-dismiss

Light-dismiss functionality is disabled by default but it can enabled so that a teaching tip will dismiss, for example, when a user scrolls or interacts with other elements of the application. Because of this behavior, light-dismiss tips are the best solution when a tip needs to be placed in a scrollable area. 

The close button will be automatically removed from a light-dismiss enabled teaching tip to identify its light-dismiss behavior to users. 

XAML
```XAML
<Button x:Name="SaveButton" Content="Save" />

<controls:TeachingTip x:Name="AutoSaveTip"
    Title="Saving automatically"
    Subtitle="We save your changes as you go - so you never have to."
    IsLightDismissEnabled="True">
</controls:TeachingTip>
```

![A sample app with a light-dismiss teaching tip in the bottom right corner. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to."](images/teaching-tip-light-dismiss.png)

### Escaping the xaml root bounds

On Windows version 19H1 and above, a teaching tip can escape the bounds of the xaml root and the screen by setting the ShouldConstrainToRootBounds property. When this property is enabled, a teaching tip will not attempt to stay in the bounds of the xaml root nor the screen and will always position at the set PreferredPlacement mode. It is encouraged to enable the IsLightDismissEnabled property and set the PreferredPlacement mode nearest to the center of the xaml root to ensure the best experience for users.

On earlier versions of Windows, this property is ignored and the teaching tip always stays within the bounds of the xaml root.

XAML
```XAML
<Button x:Name="SaveButton" Content="Save" />

<controls:TeachingTip x:Name="AutoSaveTip"
    Title="Saving automatically"
    Subtitle="We save your changes as you go - so you never have to."
    PreferredPlacement="BottomRight"
    PlacementMargin="-80,-50,0,0"
    ShouldConstrainToRootBounds="False">
</controls:TeachingTip>
```

![A sample app with a teaching tip outside of the app's bottom right corner. The tip title reads "Saving automatically" and the subtitle reads "We save your changes as you go - so you never have to." There is a close button on the top right corner of the teaching tip.](images/teaching-tip-escape-xaml-root.png)

### Canceling and deferring close

The Closing event can be used to cancel and/or defer the close of a teaching tip. This can be used to keep the teaching tip open or allow time for an action or custom animation to occur. When the closing of a teaching tip is canceled, IsOpen will go back to true, however, it will stay false during the deferral. A programmatic close can also be canceled. 

**Note: If no placement option would allow a teaching tip to fully show, teaching tip will iterate through its event lifecycle to force a close rather than display without an accessible close button. If the app cancels the Closing event, the teaching tip may remain open without an accessible Close button.**

XAML
```XAML
<controls:TeachingTip x:Name="EnableNewSettingsTip"
    Title="New ways to protect your privacy!"
    Subtitle="Please close this tip and review our updated privacy policy and privacy settings."
    Closing="OnTipClosing">
</controls:TeachingTip>
```

C#
```C#
public void OnTipClosing(object sender, TeachingTipClosingEventArgs args)
{
    if (args.Reason == TeachingTipCloseReason.CloseButton)
    {
        using(args.GetDeferral())
        {
            bool success = await UpdateUserSettings(User thisUsersID);
            if(!success)
            {
                //We were not able to update the settings! Don't close the tip and display the reason why.
                args.Cancel = true;
                ShowLastErrorMessage();
            }
        }
    }
}
```

## Remarks

### Related articles 

* [Dialogs and flyouts](https://docs.microsoft.com/en-us/windows/uwp/design/controls-and-patterns/dialogs-and-flyouts/index)

### Recommendations
* Tips are impermanent and should not contain information or options that are critical to the experience of an application. 
* Try to avoid showing teaching tips too often. Teaching tips are most likely to each recieve individual attention when they are staggered throughout long sessions or across multiple sessions.    
* Keep tips succinct and their topic clear. Research shows users, on average, only read 3-5 words and only comprehend 2-3 words before deciding whether to interact with a tip.
* Gamepad accessibility of a teaching tip is not guaranteed. For applications that predict gamepad input, please see [gamepad and remote control interactions]( https://docs.microsoft.com/en-us/windows/uwp/design/input/gamepad-and-remote-interactions#xy-focus-navigation-and-interaction). It is encouraged to test gamepad accessibility of each teaching tip using all possible configurations of an app's UI.
* When enabling a teaching tip to escape the xaml root, it is encouraged to also enable the IsLightDismissEnabled property and set the PreferredPlacement mode nearest to the center of the xaml root. 

### Reconfiguring an open teaching tip

Some content and properties can be reconfigured while the teaching tip is open and will take effect immediately. Other content and properties, such as the icon property, the Action and Close buttons, and reconfiguring between light-dismiss and explicit-dismiss will all require the teaching tip to be closed and reopened for changes to these properties to take affect. Note that changing dismissal behavior from manual-dismiss to light-dismiss while a teaching tip is open will cause the teaching tip to have its Close button removed before the light-dismiss behavior is enabled and the tip can remain stuck on-screen.


## API Notes

### Notable Properties  

| Name | Description |
|:-:|:--|
| TailVisibility | Gets or sets a value that indicates whether to collapse the tail of a teaching tip or leave it visible. |
| PreferredPlacement  | Gets or sets the default placement to be used for the teaching tip. |
| ShouldConstrainToRootBounds | Gets or sets a value that indicates whether the teaching tip will constrain to the bounds of its root. |

### Events    

| Name | Description |
|:-:|:--|
| ActionButtonClick | Occurs after the action button has been tapped. |
| CloseButtonClick | Occurs after the close button has been tapped. |
| Closed | Occurs after the tip is closed. |
| Closing |Occurs just before the tip begins to close. |


## API Details 

```c++ 
enum TeachingTipCloseReason
{
    CloseButton,
    LightDismiss,
    Programmatic,
};

enum TeachingTipPlacementMode
{
    Auto,
    Top,
    TopRight,
    TopLeft,
    Right, 
    RightTop,
    RightBottom,
    Bottom,
    BottomRight,
    BottomLeft,
    Left, 
    LeftTop,
    TopBottom,
    Center,
};

enum TeachingTipHeroContentPlacementMode
{
    Auto,
    Top,
    Bottom,
};

enum TeachingTipTailVisibility
{
	Auto,
	Visible,
	Collapsed,
};

runtimeclass TeachingTipClosedEventArgs
{
    TeachingTipCloseReason Reason{ get; };
};

runtimeclass TeachingTipClosingEventArgs
{
    TeachingTipCloseReason Reason{ get; };
    Boolean Cancel;
    Windows.Foundation.Deferral GetDeferral();
};

unsealed runtimeclass TeachingTipTemplateSettings : Windows.UI.Xaml.DependencyObject
{
    TeachingTipTemplateSettings();

    Windows.UI.Xaml.Thickness TopRightHighlightMargin;
    Windows.UI.Xaml.Thickness TopLeftHighlightMargin;

    Windows.UI.Xaml.Controls.IconElement IconElement;

    static Windows.UI.Xaml.DependencyProperty TopRightHighlightMarginProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty TopLeftHighlightMarginProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty IconElementProperty{ get; };
}

unsealed runtimeclass TeachingTip : Windows.UI.Xaml.Controls.ContentControl
{
    TeachingTip();

    String Title;
    String Subtitle;

    Boolean IsOpen;

    Windows.UI.Xaml.FrameworkElement Target;

    Object ActionButtonContent;
    Windows.UI.Xaml.Style ActionButtonStyle;
    Windows.UI.Xaml.Input.ICommand ActionButtonCommand;
    Object ActionButtonCommandParameter;

    Object CloseButtonContent;
    Windows.UI.Xaml.Style CloseButtonStyle;
    Windows.UI.Xaml.Input.ICommand CloseButtonCommand;
    Object CloseButtonCommandParameter;

    Windows.UI.Xaml.Thickness PlacementMargin;
    Boolean IsLightDismissEnabled;
    Boolean ShouldConstrainToRootBounds;
    TeachingTipPlacementMode PreferredPlacement;
    TeachingTipHeroContentPlacementMode HeroContentPlacement;
    TeachingTip
    Mode TailVisibility;

    Windows.UI.Xaml.UIElement HeroContent;
    IconSource IconSource;

    TeachingTipTemplateSettings TemplateSettings{ get; };

    event Windows.Foundation.TypedEventHandler<TeachingTip, Object> ActionButtonClick;
    event Windows.Foundation.TypedEventHandler<TeachingTip, Object> CloseButtonClick;
    event Windows.Foundation.TypedEventHandler<TeachingTip, TeachingTipClosingEventArgs> Closing;
    event Windows.Foundation.TypedEventHandler<TeachingTip, TeachingTipClosedEventArgs> Closed;

    static Windows.UI.Xaml.DependencyProperty IsOpenProperty{ get; };

    static Windows.UI.Xaml.DependencyProperty TargetProperty{ get; };

    static Windows.UI.Xaml.DependencyProperty TitleProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty SubtitleProperty{ get; };

    static Windows.UI.Xaml.DependencyProperty ActionButtonContentProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty ActionButtonStyleProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty ActionButtonCommandProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty ActionButtonCommandParameterProperty{ get; };

    static Windows.UI.Xaml.DependencyProperty CloseButtonContentProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty CloseButtonStyleProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty CloseButtonCommandProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty CloseButtonCommandParameterProperty{ get; };

    static Windows.UI.Xaml.DependencyProperty PlacementMarginProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty IsLightDismissEnabledProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty ShouldConstrainToRootBounds{ get; };
    static Windows.UI.Xaml.DependencyProperty PreferredPlacementProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty HeroContentPlacementProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty TailVisibilityProperty{ get; };

    static Windows.UI.Xaml.DependencyProperty HeroContentProperty{ get; };
    static Windows.UI.Xaml.DependencyProperty IconSourceProperty{ get; };

    static Windows.UI.Xaml.DependencyProperty TemplateSettingsProperty{ get; };
}
```


## Appendix

### Visual Components

 | Component |  Notes |
|:---:|:---|
| Container | * The container is the body of the tip and encapsulates all the tip components. <br> * Nonmodal. <br> * If content height exceeds max height or width, vertical scrolling will be enabled. See Scroll. <br> * For visibility concerns, the container has a border around the outer edge, which adheres to the tail if present. See Tail. <br> * For visibility concerns, the top edge of the container has a 1px highlight which also adheres to the tail if present. See Tail. <br><br> ![An empty teaching tip.](images/Container.PNG) |
| Title| * Semi-bolded. <br> * Text wraps at Close (X) Button and Container border. <br><br> ![A teaching tip with a title and subtitle populated with sample text.](images/Title.PNG) |
| Subtitle | * Text wraps at Close (X) Button and Container border. <br><br> ![A teaching tip with a subtitle that reads "Body text in a minimum height tip.](images/Subtitle.PNG) |
| Content | * Can be customized to include text, images, videos, animation, checkboxes, hyperlinks, and any other XAML content. <br> * Will scroll if there is more content to show than tip height allows. See Scroll Bar. <br> * Placed below Subtitle and above Close/Action Button. <br><br> ![A teaching tip with a sample title and subtitle. The teaching tip has a picture of a mountain in the content area.](images/Content.PNG) |
| Close Button | * Will appear as an X Button in the top right corner by default and in the top left corner automatically for RTL languages. The close button may also be set to appear in the bottom right corner of the tip as a traditional button or be set to not show at all so that a custom close option may be implemented in the Content Area. <br> * If a tip is set to light-dismiss, no close button will appear at all. <br><br> ![A teaching tip with a sample title and subtitle. There is a close button in the top right corner.](images/HeaderClose.PNG) &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; ![A teaching tip with a sample title and subtitle. In the teaching tip's content area is a CheckBox labeled "Don't show tips at startup. At the bottom of the teaching tip is a button that reads "Got it".](images/FooterClose.PNG) |
| Action Button | * Allows the user to invoke a custom event. <br> * This is the only non-Close button provided out of the box. Other buttons may be implemented in the Content Area. <br><br> ![A teaching tip with a sample title and subtitle. At the bottom of the tip are two buttons, a blue button on the left that reads "Default" and a gray button on the right that reads "Secondary".](images/ActionButton.PNG) |
| Tail | * Triangular extension of the tip body that can be used to indicate that the tip is referring to on-screen UI element. <br> * When TailVisibility is set to auto, the tail will automatically appear on tips that are attached to a target and off for tips that are not attached to a target. <br> * Prefers to center on target. <br> * Maintains a 12px margin from edges of the tip. <br> * Not animated. <br> * Will not have a shadow as shadows cannot yet be added to nonrectangular surfaces. ![An empty teaching tip with a triangular pointer centered outward on its bottom edge.](images/Pointer.PNG) |
| Icon | * Added to the left of the Title and Subtitle by default and automatically moved to the right for RTL languages. <br><br> ![A teaching tip with a sample title and subtitle. To the left of the title is a sample icon showing a graph of connected dots.](images/Icon.PNG) |
| Hero Content | * Hero Content is media that stretches to the edges of a tip. <br> * Can be placed at the top or bottom of a tip. <br><br> ![A teaching tip with a sample title and subtitle. Above the title and subtitle is a large border-to-border picture of a giraffe.](images/HeroContent.PNG) |
| Scroll Bar | * If the tip's contents are large enough to require scrolling, a scrollbar which will not intersect the Close (X) Button will be added to the content area. <br><br> ![A tall teaching tip with a scrollbar on the right a long string of "Lorem ipsum" text in the body.](images/ScrollBar.PNG) |

### Behavioral Components

 | Property | Notes |
|:---:|:---|
| Opening | * A tip is shown by setting its IsOpen property to true. <br> * Tips will animate on opening. <br> * When a tip does not have enough available window space to fully show in any location [see Placement], it will not open and will instead overwrite IsOpen to false. |
| Closing | There are three ways a tip can close: The program sets the IsOpen property to false, the user invokes the Close button, or the tip closes due to light dismiss. Use the TeachingTipCloseReason to determine which case has occurred. Closing can be prevented by setting the Cancel property to true. You can use a deferral to respond asynchronously to the event. |
| Placement | * Placement modes for targeted teaching tips will follow the precedent of Flyout. Full placement mode will be replaced by Center which positions Tail at the center of the element. <br> * Placement modes for non-targeted tips will include each side, corner, and center of the application window. <br> * The following properties are not preferred in tip placement: <br> &nbsp;&nbsp;&nbsp;&nbsp; * There is not enough space for the tip to show without clipping. <br> &nbsp;&nbsp;&nbsp;&nbsp; * The target is not large enough to maintain the tip's alignment and the Tail's 12px margin from the edge of the tip. <br> &nbsp;&nbsp;&nbsp;&nbsp; * The target element is too large to maintain edge alignment while keeping the Tail centered on the target. |
| Light-dismiss | * Allows a tip to be dismissed when the user scrolls or clicks elsewhere within the application. |
| Persistent Tip Location | * Once a tip is open, it will not move even if its target does. The exception to this is when the window is resized. |
| Motion | * Tips have built in open and close animations that can be customizable using Storyboards.|
| Tail/Hero Content Avoidance | * To avoid the visual oddity of the Tail emerging from Hero Content, the Tail and Hero Content will attempt to avoid intersecting using the following rules: <br> &nbsp;&nbsp;&nbsp;&nbsp; * Move the hero content to Top or Bottom (Disabled when HeroContentPlacement is not auto). <br> &nbsp;&nbsp;&nbsp;&nbsp; * Shift the beak along the edge of the tip (Disabled when the placement of the tip is edge aligned). <br> &nbsp;&nbsp;&nbsp;&nbsp; * Change the placement of the tip (Disabled when the tip placement is not auto).  |
| Out of Window Bounds | * Tips can escape window bounds on newer OS versions via the ShouldConstrainToRootBounds property.  When this property is set to false, the tip uses screen boundaries instead of window boundaries during its placement algorithm. |

### Inputs and Accessibility

#### UI Automation Patterns 

TeachingTip will alternate between Pane for persistent TeachingTips and Window for light-dismiss enabled TeachingTips with IScrollProvider for the (conditionally) scrollable content area within TeachingTip. 

TeachingTip will implement a custom "tip" Landmark. 

#### Keyboard Navigation 
| State | Action |
|:---|:---|
| Tip appears | No action is needed invoke the tip. |
| Tip recieves focus | F6: <br> Tip will be added to F6 region navigation stops so that F6 may be used to focus into and out of the tip. <br><br> Tab: <br> If Narrator is active, TeachingTip will automatically be added to the top of Narrator navigation stops thanks to its UI Automation Pattern(s), similar to popups or ContentDialog, and can be accessed via tabbing. |
| Tip is tabbed through | Tab Button: <br> Will go through all actionable items, regardless of group, in order. When tab is pressed on the last element in the teaching tip, focus will cycle to the first element in the teaching tip.  <br> <br> Left + Right Arrow Keys: <br> Can be used to navigate between the footer Action and Close buttons if both are present. <br><br> Escape: <br> Will result in closing the tip. |
| Tip is dismissed | 1. X Button is pressed. <br> 2. Close Button is pressed. <br> 3. Action Button is pressed. <br><br> * Tab increments focus to the next element but does not close the tip. |

#### Narrator

TeachingTip will leverage the existing APIs used by Windows Notifications.

To provide targeted tip context for Narrator users, a targeted TeachingTip will inject the name of its target before the tip's title to be read together by Narrator. 

| State | Action |
|:---|:---|
| Tip appears | Narrator will say "Click Up to move to new notifcation from" + App Name + Tip Contents | 
| Tip recieves focus | Ctrl + Narrator + Up arrow: <br> Will move focus to tip and Narrator will read the element in focus. |
| Tip is tabbed through | Tab Button: <br> Will navigate through all actionable items, regardless of group, in order. When tab is pressed on the last element in the teaching tip, focus will cycle to the first element in the teaching tip.  <br> <br> Swipe (for touch screen devices): <br> Will navigate through all actionable items, regardless of group, in order. When Swiping on the last element in the teaching tip, focus will move to Narrator's fullscreen invisible Close Button and the user may double tap the screen to close the window. Swiping again will move focus out of the tip. <br><br> Left + Right Arrow Keys: <br> Can be used to navigate between the footer Action and Close buttons if both are present. <br><br> Escape: <br> Will result in closing the tip. |
| Tip is dismissed | 1. Header Close Button is invoked. <br> 2. Footer Close Button is invoked. <br> 3. Action Button is invoked. <br> 4. Swipe (for touch screen devices) moves focus to Narrator's fullscreen invisible Close Button and the user double taps the screen to close the window. * Tab increments focus to the next element but does not close the tip. |

#### Gamepad  
| State | Action |
|:---|:---|
| Tip appears | No action is needed invoke the tip. |
| Tip recieves focus | Spatial navigation: <br> Spatial navigation may be used to access tip. Guidance will be added to advise proper design consideration for tip accessibility and testing for gamepad. |
| Tip is navigated | Spatial navigation: <br> Will spatially navigate focus across actionable items (without respect to group).  <br> <br> A Button: <br> Will interact with the item in focus, such as "press" the action or close button. <br><br> B Button: <br> Will result in closing the tip. |
| Tip is dismissed | 1. Header "X" Close Button is pressed. <br> 2. Footer Close Button is pressed. <br> 3. Action Button is pressed. <br> 4. B Button returns focus to the element previously in focus. |

### Data and Intelligence Metrics

##### P0: Feature Key Performance Indicators

Feature has received enough app/developer validation and feedback for it to exit preview.

* KPI: Three or more first party applications are validating the feature in prerelease.
    * Measurement: Occurrence of TeachingTip type in apps on developer devices via telemetry (running under debugger). **Note for telemetry sampling considerations:** Need to be cautious for WinUI as it does not work the same way as with OS SDK. If the solution is not carefully considered, it will result in too little dev data (at 2% sampling) or too much end-user data (at 100% sampling) and hence throttling (telemetry data loss). Requires further investigation. 
    * Measurement: Count of users providing feedback on GitHub/UserVoice/Feedback Hub/SIUF.
    
Developers feel delighted and satisfied with the feature. 

* KPI: Dev NSAT is positive (min), >20 (aspirational) as we exit 2019.
    * Measurement: Developer survey via SIUF, e.g.: "How useful is the feature?" 
    
Control is accessible to end users. 

* KPI: F6 is discoverable to keyboard navigation users.  
    * Measurement: What percentage of the instances of accessing teaching tip are through F6?
    * Measurement: What percentage of instances of using teaching tip through F6 are done by unique users?

* KPI: Instances of teaching tip consistently surface to end users. 
    * Measurement: Number of times a control has immediately iterated through its lifecycle to force a close because it could not find room to show. 
    
##### P1: Feature Performance Indicators

Ratio of apps applying teaching tip should be comparable across all Windows experiences.

* KPI: Ratio of apps using teaching tip per Windows experience and form factor is comparable as we exit 2019. 
    * Measurement: Occurrence of TeachingTip type in apps via static analysis and telemetry.

* KPI: Average time before teaching tip is dismissed by end user per Windows experience and form factor is comparable.
    * Measurement: Time delta between TeachingTip.IsOpen = true/false via telemetry.

Uses of teaching tip are perceived useful and are not an annoyance for end users.

* KPI: Average display time for teaching tip is between 3 and 30 seconds, excluding instances containing video media.
    * Measurement: Time delta between TeachingTip.IsOpen = true/false via telemetry for tips not containing video media. 
  
##### P2+: Trailing Indicators
 
N/A
