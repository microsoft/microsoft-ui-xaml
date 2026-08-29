# Xaml Style Guide

## Overview

This guide provides best practices and guidelines for writing XAML code, with a focus on authoring WinUI controls. 
Following these guidelines will help ensure consistency, maintainability, and readability in your XAML code. 
This will also ensure that the control can easily be customized or re-templated by developers.

For a higher-level overview on authoring a control, refer to [How to Author a Xaml Control](../how-to-author-a-xaml-control.md).

## Xaml Template

A WinUI control's template consists of a `[ControlName].xaml` and `[ControlName]_themeresources.xaml` file. 
Generally, the default template style is stored in the `.xaml` file, while everything else, including themeresources 
and template variations, are stored in the themeresources file.

Logic changes to the control happen in the codebehind `.cpp` and `.h` files. They interact with the templated 
parts of the control.

Below sections are elements of note in the `*.xaml` files.

### Default[Control]Style and ControlTemplate

```xml
<ResourceDictionary
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:controls="using:Microsoft.UI.Xaml.Controls"
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">

    <Style TargetType="controls:TitleBar" BasedOn="{StaticResource DefaultTitleBarStyle}"/>

    <Style x:Key="DefaultTitleBarStyle" TargetType="controls:TitleBar">
        <Setter Property="HorizontalAlignment" Value="Stretch" />
        <Setter Property="Foreground" Value="{ThemeResource TitleBarForegroundBrush}" />
        <Setter Property="Background" Value="Transparent" />
        <Setter Property="Template">
            <Setter.Value>
                <ControlTemplate TargetType="controls:TitleBar">
                    <Grid x:Name="PART_LayoutRoot" Background="{TemplateBinding Background}">
                    ...
                    </Grid>
            </Setter.Value>
        </Setter>
    </Style>
</ResourceDictionary>
```

The above syntax defines the visuals of the control. 
- Giving the style a key and referencing it with `BasedOn` syntax is imperative to allow for re-templating. 
- `controls:` namespace references the WinUI Xaml controls resource.
- `x:` namespace references the winfx resource, used for properties such as `x:Name`.
- `Template` is an attached property of the control element, just like `Foreground` and `Background`.
We set the template property with an object of type `ControlTemplate` to define the control's visual structure.
- See [ControlTemplate Class](https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.controls.controltemplate?view=winrt-26100)
for more info.

### TemplateBind Control Properties

The properties inherited from `Control` are generally expected to modify the corresponding elements in the control. 
These properties are designed as placeholders and doesn't modify anything by default. 
You can hook them up by TemplateBinding them to elements in your default ControlTemplate.

```xml
<Style x:Key="DefaultTitleBarStyle" TargetType="controls:TitleBar">
    <Setter Property="HorizontalAlignment" Value="Stretch" />
    <Setter Property="Foreground" Value="{ThemeResource TitleBarForegroundBrush}" />
    <Setter Property="Background" Value="{ThemeResource TitleBarBackgroundBrush}" />
    <Setter Property="Template">
        <Setter.Value>
            <ControlTemplate TargetType="controls:TitleBar">
                <Grid x:Name="PART_LayoutRoot" Background="{TemplateBinding Background}">
                ...
                    <TextBlock
                        x:Name="PART_TitleText"
                        Text="{TemplateBinding Title}"
                        Foreground="{TemplateBinding Foreground}" />
                ...
                </Grid>
        </Setter.Value>
    </Setter>
</Style>
```

Here, default `Foreground` and `Background` properties are set through Setter properties on the control, and is linked to inner control elements via `TemplateBinding` syntax.
Note that default properties in the setters are not hard-coded and reference themeresources defined in the ResourceDictionary.

These control properties are important for easy styling capabilities for the end developer. For example:

```xml
<controls:TitleBar x:Name="MyTitleBar" Title="MyTitleBarTitle" Foreground="White" Background="Blue" />
```

Numerous properties are expected by developers to modify something. Hence when authoring controls, it is important to go through this list and consider 
each property's desired behavior and template bind them to the appropriate element if needed.

Relevant properties from `Control` to consider:
- `Background`
- `BorderBrush`
- `BorderThickness`
- `CornerRadius`
- `Foreground`
- `Height`
- `HorizontalAlignment`
- `HorizontalContentAlignment`
- `Margin`
- `MaxHeight`
- `MaxWidth`
- `MinHeight`
- `MinWidth`
- `Padding`
- `VerticalAlignment`
- `VerticalContentAlignment`
- `Width`

### VisualStateManager

VisualStateManager class manages a control's visual states and transitions between each state. 
The control must be one of the visual states within a `VisualStateGroup`. 

For example, the most generic `VisualStateGroup` is `CommonStates`and contains visual states such as `Normal`, 
`PointerOver`, `Pressed`, `Disabled`.
- The control is either one of those states at all times. 
- The first state should generally be a self closing tag (ie. `<VisualState x:Name="Normal" />`), with its 
definition defined in the template.
- Property values should always reference a defined ThemeResource or StaticResource. 
See ThemeResource section for more info.

```xml
<ControlTemplate TargetType="controls:MenuBarItem">
    <Grid x:Name="ContentRoot" Background="{TemplateBinding Background}" CornerRadius="{TemplateBinding CornerRadius}">
        <VisualStateManager.VisualStateGroups>
            <VisualStateGroup x:Name="CommonStates">
            <VisualState x:Name="Normal" />
            <VisualState x:Name="PointerOver">
                <VisualState.Setters>
                <Setter Target="Background.Background" Value="{ThemeResource MenuBarItemBackgroundPointerOver}" />
                <Setter Target="Background.BorderBrush" Value="{ThemeResource MenuBarItemBorderBrushPointerOver}" />
                </VisualState.Setters>
            </VisualState>
            <VisualState x:Name="Pressed">
                <VisualState.Setters>
                <Setter Target="Background.Background" Value="{ThemeResource MenuBarItemBackgroundPressed}" />
                <Setter Target="Background.BorderBrush" Value="{ThemeResource MenuBarItemBorderBrushPressed}" />
                </VisualState.Setters>
            </VisualState>
            <VisualState x:Name="Selected">
                <VisualState.Setters>
                <Setter Target="Background.Background" Value="{ThemeResource MenuBarItemBackgroundSelected}" />
                <Setter Target="Background.BorderBrush" Value="{ThemeResource MenuBarItemBorderBrushSelected}" />
                </VisualState.Setters>
            </VisualState>
            </VisualStateGroup>
        </VisualStateManager.VisualStateGroups>      
        ...
```

Once these visual states are defined, logic is added to the `.cpp` codebehind file to toggle between these states.

```c#
void MenuBarItem::UpdateVisualStates()
{
    if (auto button = m_button.get())
    {
        if (button.IsPressed())
        {
            winrt::VisualStateManager::GoToState(*this, s_pressedVisualStateName, false);
        }
        else if (button.IsPointerOver())
        {
            winrt::VisualStateManager::GoToState(*this, s_pointerOverVisualStateName, false);
        }
        else
        {
            if (m_isFlyoutOpen)
            {
                winrt::VisualStateManager::GoToState(*this, s_selectedVisualStateName, false);
            }
            else
            {
                winrt::VisualStateManager::GoToState(*this, s_normalVisualStateName, false);
            }
        }
    }
}

// Stored in the corresponding header file.
static constexpr std::wstring_view s_pressedVisualStateName{ L"Pressed"sv };
static constexpr std::wstring_view s_pointerOverVisualStateName{ L"PointerOver"sv };
static constexpr std::wstring_view s_selectedVisualStateName{ L"Selected"sv };
static constexpr std::wstring_view s_normalVisualStateName{ L"Normal"sv };
```

Note that the visual state names should be stored as `satic constexpr` in the header file.

Changes to separate element parts can be defined in a separate VisualStateGroup. Here, RadioMenuFlyout has a 
`KeyboardAcceleratorTextBlock` that has its visibility toggled.

```xml
<VisualStateGroup x:Name="KeyboardAcceleratorTextVisibility">
    <VisualState x:Name="KeyboardAcceleratorTextCollapsed" />
    <VisualState x:Name="KeyboardAcceleratorTextVisible">
        <VisualState.Setters>
        <Setter Target="KeyboardAcceleratorTextBlock.Visibility" Value="Visible" />
        </VisualState.Setters>
    </VisualState>
</VisualStateGroup>
```

Note that different visual state groups **cannot** modify the same element. Modifying the same element can cause
unpredictable behaviour and bugs. 

For example, in AnimatedIcon, both checked and common pointer states modify the `Icon` and `StateTextBlock`
element. The solution is to merge what usually would be two visual state groups like so:

```xml
<VisualStateGroup x:Name="CommonStates">
    <VisualState x:Name="CheckedNormal">
        <VisualState.Setters>
            <Setter Target="Icon.(controls:AnimatedIcon.State)" Value="NormalOn"/>
            <Setter Target="StateTextBlock.Text" Value="NormalOn"/>
        </VisualState.Setters>
    </VisualState>
    <VisualState x:Name="CheckedPointerOver">
        <VisualState.Setters>
            <Setter Target="Icon.(controls:AnimatedIcon.State)" Value="PointerOverOn"/>
            <Setter Target="StateTextBlock.Text" Value="PointerOverOn"/>
        </VisualState.Setters>
    </VisualState>
    <VisualState x:Name="CheckedPressed">
        <VisualState.Setters>
            <Setter Target="Icon.(controls:AnimatedIcon.State)" Value="PressedOn"/>
            <Setter Target="StateTextBlock.Text" Value="PressedOn"/>
        </VisualState.Setters>
    </VisualState>
    <VisualState x:Name="CheckedDisabled">
        <VisualState.Setters>
            <Setter Target="Icon.(controls:AnimatedIcon.State)" Value="DisabledOn"/>
            <Setter Target="StateTextBlock.Text" Value="DisabledOn"/>
        </VisualState.Setters>
    </VisualState>
    <VisualState x:Name="UncheckedNormal">
        <VisualState.Setters>
            <Setter Target="Icon.(controls:AnimatedIcon.State)" Value="NormalOff"/>
            <Setter Target="StateTextBlock.Text" Value="NormalOff"/>
        </VisualState.Setters>
    </VisualState>
    <VisualState x:Name="UncheckedPointerOver">
        <VisualState.Setters>
            <Setter Target="Icon.(controls:AnimatedIcon.State)" Value="PointerOverOff"/>
            <Setter Target="StateTextBlock.Text" Value="PointerOverOff"/>
        </VisualState.Setters>
    </VisualState>
    <VisualState x:Name="UncheckedPressed">
        <VisualState.Setters>
            <Setter Target="Icon.(controls:AnimatedIcon.State)" Value="PressedOff"/>
            <Setter Target="StateTextBlock.Text" Value="PressedOff"/>
        </VisualState.Setters>
    </VisualState>
    <VisualState x:Name="UncheckedDisabled">
        <VisualState.Setters>
            <Setter Target="Icon.(controls:AnimatedIcon.State)" Value="DisabledOff"/>
            <Setter Target="StateTextBlock.Text" Value="DisabledOff"/>
        </VisualState.Setters>
    </VisualState>
</VisualStateGroup>
```

_Note that the default state (ChekcedNormal) has definitions defined due to AnimatedIcon's implementation constraints._


#### Animations with VisualStateManager

`<VisualState.Setters>` is the most fundamental way to toggle between different element properties as outlined above.
For more sophisticated animations, `Storyboard` and `VisualTransition` can be used.

ContentDialog uses `VisualTransition` to define animations that will be played when switching to the corresponding visual states.

```xml
<VisualStateGroup x:Name="DialogShowingStates">
    <VisualStateGroup.Transitions>
        <VisualTransition To="DialogHidden">
        <Storyboard>
            ...
            <ObjectAnimationUsingKeyFrames Storyboard.TargetName="LayoutRoot" Storyboard.TargetProperty="Visibility">
                <DiscreteObjectKeyFrame KeyTime="0:0:0" Value="Visible" />
            </ObjectAnimationUsingKeyFrames>
            <DoubleAnimationUsingKeyFrames Storyboard.TargetName="LayoutRoot" Storyboard.TargetProperty="Opacity">
                <DiscreteDoubleKeyFrame KeyTime="0:0:0" Value="1.0" />
                <LinearDoubleKeyFrame KeyTime="{StaticResource ControlFasterAnimationDuration}" Value="0.0" />
            </DoubleAnimationUsingKeyFrames>
        </Storyboard>
        </VisualTransition>
        <VisualTransition To="DialogShowing">
        <Storyboard>
            ...
            <ObjectAnimationUsingKeyFrames Storyboard.TargetName="LayoutRoot" Storyboard.TargetProperty="Visibility">
                <DiscreteObjectKeyFrame KeyTime="0:0:0" Value="Visible" />
            </ObjectAnimationUsingKeyFrames>
            <DoubleAnimationUsingKeyFrames Storyboard.TargetName="LayoutRoot" Storyboard.TargetProperty="Opacity">
                <DiscreteDoubleKeyFrame KeyTime="0:0:0" Value="0.0" />
                <LinearDoubleKeyFrame KeyTime="{StaticResource ControlFasterAnimationDuration}" Value="1.0" />
            </DoubleAnimationUsingKeyFrames>
        </Storyboard>
        </VisualTransition>
    </VisualStateGroup.Transitions>
    <VisualState x:Name="DialogHidden" />
    <VisualState x:Name="DialogShowing">
        ...
    </VisualState>
</VisualStateGroup>
```

Everytime the element switches to the `DialogHidden` visual state, the storyboard defined by 
`<VisualTransition To="DialogHidden">` is played.

```xml
<ObjectAnimationUsingKeyFrames Storyboard.TargetName="LayoutRoot" Storyboard.TargetProperty="Visibility">
    <DiscreteObjectKeyFrame KeyTime="0:0:0" Value="Visible" />
</ObjectAnimationUsingKeyFrames>
```

- `ObjectAnimationUsingKeyFrames` animates the value of an **Object property** along a set of `KeyFrame`s over a duration.
In this case, the object property is `LayoutRoot`'s `Visibility` property.
- There is only one keyframe, so `LayoutRoot` animates to visible at time 0.

```xml
<DoubleAnimationUsingKeyFrames Storyboard.TargetName="LayoutRoot" Storyboard.TargetProperty="Opacity">
    <DiscreteDoubleKeyFrame KeyTime="0:0:0" Value="1.0" />
    <LinearDoubleKeyFrame KeyTime="{StaticResource ControlFasterAnimationDuration}" Value="0.0" />
</DoubleAnimationUsingKeyFrames>
```

- `DoubleAnimationUsingKeyFrames` animates the value of a **Double property** along a set of `KeyFrame`s over a duration.
In this case, the double property is `LayoutRoot`'s `Opacity` property.
- The `Opacity` property starts out with a value of 1 at time 0.
- The `Opacity` property **linearly** animates to 0 at time `ControlFasterAnimationDuration`.
- See Common Themeresources section for more info on `ControlFasterAnimationDuration`.
- See Animation section in WinUI Gallery or [Animations in XAML](https://learn.microsoft.com/en-us/windows/apps/design/motion/xaml-animation)
for more info.

### DataTemplate

`DataTemplate` describes the visual structure of a data object. It is similar to a `ContentTemplate`
but it is typically used for data binding scenarios, or when an element is repeated (with some data variations).
It is typically set as the value of an `ItemTemplate` Object but can also be used for the following properties

- **ItemsControl.ItemTemplate** (which is inherited by various items controls such as ListView, GridView, ListBox)
- **ContentControl.ContentTemplate** (which is inherited by various content controls such as Button, Frame, SettingsFlyout)
- **HeaderTemplate** and **FooterTemplate** properties of various items control classes
- **ItemsPresenter.HeaderTemplate** and **ItemsPresenter.FooterTemplate**
- **HeaderTemplate** and **FooterTemplate** properties of text controls such as RichEditBox, TextBox
- **HeaderTemplate** property of controls such as ComboBox, DatePicker, Hub, HubSection, Pivot, Slider, TimePicker, ToggleSwitch; some of these also have **FooterTemplate**

- See [DataTemplate Class](https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.datatemplate?view=winrt-26100) for more info

We use `DataTemplate` less frequently when authoring controls, since we are generally creating a single, re-usable "element" 
that gives end developers the option to data bind. We generally design the control to have `DataTemplate` 
as a settable property value.

For example: 

- `NavigationView.MenuItemTemplate` is of type `DataTemplate` so takes a `DataTemplate` type as a settable property value:

[NavigationView.idl](..\..\controls\dev\NavigationView\NavigationView.idl)
```c#
Microsoft.UI.Xaml.DataTemplate MenuItemTemplate { get; set; };
   
```

In some scenarios, you can write a DataTemplateSelector to add logic to switch between different `DataTemplates`. 
See [Data Binding in Depth](https://learn.microsoft.com/en-us/windows/uwp/data-binding/data-binding-in-depth) for more info.

```c#
Microsoft.UI.Xaml.Controls.DataTemplateSelector MenuItemTemplateSelector { get; set; };
```

[NavigationViewPageDataContext.xaml](..\..\controls\dev\NavigationView\TestUI\Common\NavigationViewPageDataContext.xaml)

```xml
<muxcontrols:NavigationView
    Width="400" PaneDisplayMode="Left"
    MenuItemsSource="{x:Bind Pages, Mode=OneWay}"
    ItemInvoked="NavigationView_ItemInvoked">
    <muxcontrols:NavigationView.MenuItemTemplate>
        <DataTemplate>
            <muxcontrols:NavigationViewItem Content="{Binding Name}" AutomationProperties.Name="{Binding Name}"/>
        </DataTemplate>
    </muxcontrols:NavigationView.MenuItemTemplate>
    ...
```

Some control examples with repeated elements are `PipsPager` and `RatingControl`. They use DataTemplates to implement these elements.

[PipsPager.xaml](..\..\controls\dev\PipsPager\PipsPager.xaml)
```xml
<ScrollViewer x:Name="PipsPagerScrollViewer" VerticalScrollBarVisibility="Hidden" VerticalScrollMode="Disabled" HorizontalScrollBarVisibility="Hidden" HorizontalScrollMode="Disabled" IsHorizontalScrollChainingEnabled="False" IsVerticalScrollChainingEnabled="False" HorizontalAlignment="Center" VerticalAlignment="Center">
    <controls:ItemsRepeater x:Name="PipsPagerItemsRepeater" ItemsSource="{Binding RelativeSource={RelativeSource TemplatedParent}, Path=TemplateSettings.PipsPagerItems}">
    <controls:ItemsRepeater.Layout>
        <controls:StackLayout Orientation="{Binding RelativeSource={RelativeSource TemplatedParent}, Path=Orientation}" />
    </controls:ItemsRepeater.Layout>
    <controls:ItemsRepeater.ItemTemplate>
        <DataTemplate>
        <Button />
        </DataTemplate>
    </controls:ItemsRepeater.ItemTemplate>
    </controls:ItemsRepeater>
</ScrollViewer>
```

[RatingControl_themeresources.xaml](..\..\controls\dev\RatingControl\RatingControl_themeresources.xaml)
```xml
<DataTemplate x:Key="BackgroundGlyphDefaultTemplate">
    <TextBlock Foreground="{ThemeResource RatingControlUnselectedForeground}" Margin="-8,-8,0,0" FontSize="{StaticResource RatingControlFontSizeForRendering}" Text="&#xE734;" AutomationProperties.AccessibilityView="Raw" FontFamily="{ThemeResource SymbolThemeFontFamily}" />
</DataTemplate>
```


### TemplateSettings

TemplateSettings class is used to expose calculated values so that it can be consumed by the template. 
It is best practice to never modify a control's visual with codebehind. 
By using binding to the exposed values from TemplateSettings, we ensure that the control
remains customizable and re-templatable. 

An example from [Expander.xaml](..\..controls\dev\InfoBadge\InfoBadge_themeresources.xaml), 
where `ContentHeight` is the calculated value of ExpanderContent's height. 

```xml
<DoubleAnimationUsingKeyFrames Storyboard.TargetName="ExpanderContent" Storyboard.TargetProperty="(UIElement.RenderTransform).(CompositeTransform.TranslateY)">
    <DiscreteDoubleKeyFrame KeyTime="0" Value="{Binding RelativeSource={RelativeSource TemplatedParent}, Path=TemplateSettings.ContentHeight}" />
    <SplineDoubleKeyFrame KeyTime="0:0:0.333" Value="0" KeySpline="0.0, 0.0, 0.0, 1.0" />
</DoubleAnimationUsingKeyFrames>
```

Instead of animating `ExpanderContent` to the its calculated height value in codebehind,
the animation is kept in the template, leveraging `ExpanderTemplateSettings.ContentHeight`.

TemplateSettings properties are APIs defined in a control's `*.idl` file. 
They also need to undergo API reviews, much like overridable themeresources.

[Expander.idl](../../controls\dev\Expander\Expander.idl)
```c#
runtimeclass ExpanderTemplateSettings : Microsoft.UI.Xaml.DependencyObject
{
    Double ContentHeight{ get; };
    Double NegativeContentHeight{ get; };
}
```

See [TemplateSettings page](https://learn.microsoft.com/en-us/windows/uwp/xaml-platform/template-settings-classes) 
for more info.

### Theming with ResourceDictionaries

All controls must support theming in Light, Dark, and Contrast themes. This is achieved by referencing **all** colors 
as a brush in the resource dictionary. The brushes are also set for each theme.

Below is an example for TitleBar. 

```xml
<!-- Style for DefaultTitleBar in TitleBar.xaml-->
<Style x:Key="DefaultTitleBarStyle" TargetType="controls:TitleBar">
    <Setter Property="Foreground" Value="{ThemeResource TitleBarForegroundBrush}" />
    ...
```

- The `Foreground` property is set as a `ThemeResource` with the key `TitleBarForegroundBrush`.
- `ThemeResource` is used instead of a `StaticResource`. `ThemeResource` allows for brushes to be re-evaluated in xaml, 
while `StaticResource` is evaluated once OnApplyTemplate only. 
We **almost always** use `ThemeResource` to reference brush keys.
- See [Xaml Theme Resources](https://learn.microsoft.com/en-us/windows/apps/develop/platform/xaml/xaml-theme-resources)
for more info.
- Brush key names should follow the naming convention `[ControlName][Description]`. 

```xml
<!-- Associated Resource for `TitleBarForegroundBrush` in TitleBar_themeresources.xaml -->
<ResourceDictionary.ThemeDictionaries>
    <ResourceDictionary x:Key="Default">
        <StaticResource x:Key="TitleBarForegroundBrush" ResourceKey="TextFillColorPrimaryBrush" />
    </ResourceDictionary>
    <ResourceDictionary x:Key="Light">
        <StaticResource x:Key="TitleBarForegroundBrush" ResourceKey="TextFillColorPrimaryBrush" />
    </ResourceDictionary>
    <ResourceDictionary x:Key="HighContrast">
        <StaticResource x:Key="TitleBarForegroundBrush" ResourceKey="SystemControlForegroundBaseHighBrush" />
    </ResourceDictionary>
</ResourceDictionary.ThemeDictionaries>
```

- `TitleBarForegroundBrush` is set for `Default` (dark), `Light`, and `HighContrast` resource dictionaries.
- `Default` and `Light` references resource keys defined in [Common_themeresources_any.xaml](..\..\controls\dev\CommonStyles\Common_themeresources_any.xaml).
- `HighContrast` resources are defined in System Xaml. 

- Setting a `TitleBarForegroundBrush` in the resource dictionary also **exposes it as an overridable resource for the end developers.** 
This is important to maintain the customizability of the xaml framework. 
Even though `TextFillColorPrimaryBrush` has all themes set in the `Common_themeresources_any.xaml` file and would theme correctly, 
we should **never reference it directly in the control**.

The same applies to other settable resources such as:

```xml
<!-- TitleBar.xaml -->
<Grid x:Name="PART_LayoutRoot" Height="{ThemeResource TitleBarCompactHeight}" Background="{TemplateBinding Background}">
                    
<!-- TitleBar_themeresources.xaml -->
<ResourceDictionary.ThemeDictionaries>
    <ResourceDictionary x:Key="Default">
        <x:Double x:Key="TitleBarCompactHeight">32</x:Double>
        <x:Double x:Key="TitleBarExpandedHeight">48</x:Double>
    </ResourceDictionary>
    <ResourceDictionary x:Key="Light">
        <x:Double x:Key="TitleBarCompactHeight">32</x:Double>
        <x:Double x:Key="TitleBarExpandedHeight">48</x:Double>
    </ResourceDictionary>
    <ResourceDictionary x:Key="HighContrast">
        <x:Double x:Key="TitleBarCompactHeight">32</x:Double>
        <x:Double x:Key="TitleBarExpandedHeight">48</x:Double>
    </ResourceDictionary>
</ResourceDictionary.ThemeDictionaries>                   
```

- `TitleBarCompactHeight` and `TitleBarExpandedHeight` are defined in the ResourceDictionary and referenced in the template. 
- This exposes them as overridable resources. 
- Even though double resources are the same for each theme dictionary, it is generally set individually as above.
Sometimes the values can vary in contrast themes (ie. thicker borders).
    - _Note: The above is somewhat inconsistent. Perhaps this can be an opportunity to clean up the xaml._

### Common_themeresources_any.xaml

Common_themeresources_any.xaml contains all the brushes set by design during the Windows 10 to Windows 11 rejuvenation.
- There is a 1-1 mapping of each color in the color ramp to resource key in the file.
- See the [Figma Windows UI Kit](https://www.figma.com/community/file/1440832812269040007/windows-ui-kit),
[Color in Windows Page](https://learn.microsoft.com/en-us/windows/apps/design/signature-experiences/color),
or Color page in WinUI 3 Gallery for more info.

For example, looking at `TextFillColor*` in [Common_themeresources_any.xaml](..\..\controls\dev\CommonStyles\Common_themeresources_any.xaml):

```xml
<ResourceDictionary.ThemeDictionaries>
    <ResourceDictionary x:Key="Default">
        <Color x:Key="TextFillColorPrimary">#FFFFFF</Color>
        <Color x:Key="TextFillColorSecondary">#C5FFFFFF</Color>
        <Color x:Key="TextFillColorTertiary">#87FFFFFF</Color>
        <Color x:Key="TextFillColorDisabled">#5DFFFFFF</Color>
        <SolidColorBrush x:Key="TextFillColorPrimaryBrush" Color="{StaticResource TextFillColorPrimary}" />
        <SolidColorBrush x:Key="TextFillColorSecondaryBrush" Color="{StaticResource TextFillColorSecondary}" />
        <SolidColorBrush x:Key="TextFillColorTertiaryBrush" Color="{StaticResource TextFillColorTertiary}" />
        <SolidColorBrush x:Key="TextFillColorDisabledBrush" Color="{StaticResource TextFillColorDisabled}" />
    </ResourceDictionary>
    <ResourceDictionary x:Key="Light">
        <Color x:Key="TextFillColorPrimary">#E4000000</Color>
        <Color x:Key="TextFillColorSecondary">#9E000000</Color>
        <Color x:Key="TextFillColorTertiary">#72000000</Color>
        <Color x:Key="TextFillColorDisabled">#5C000000</Color>
        <SolidColorBrush x:Key="TextFillColorPrimaryBrush" Color="{StaticResource TextFillColorPrimary}" />
        <SolidColorBrush x:Key="TextFillColorSecondaryBrush" Color="{StaticResource TextFillColorSecondary}" />
        <SolidColorBrush x:Key="TextFillColorTertiaryBrush" Color="{StaticResource TextFillColorTertiary}" />
        <SolidColorBrush x:Key="TextFillColorDisabledBrush" Color="{StaticResource TextFillColorDisabled}" />
    </ResourceDictionary>
    <ResourceDictionary x:Key="HighContrast">
        <SolidColorBrush x:Key="TextFillColorPrimaryBrush" Color="{ThemeResource SystemColorWindowTextColor}" />
        <SolidColorBrush x:Key="TextFillColorSecondaryBrush" Color="{ThemeResource SystemColorWindowTextColor}" />
        <SolidColorBrush x:Key="TextFillColorTertiaryBrush" Color="{ThemeResource SystemColorWindowTextColor}" />
        <SolidColorBrush x:Key="TextFillColorDisabledBrush" Color="{ThemeResource SystemColorGrayTextColor}" />
        <Color x:Key="TextFillColorPrimary">#FF0000</Color>
        <Color x:Key="TextFillColorSecondary">#FF0000</Color>
        <Color x:Key="TextFillColorTertiary">#FF0000</Color>
        <Color x:Key="TextFillColorDisabled">#FF0000</Color>
    </ResourceDictionary>
</ResourceDictionary.ThemeDictionaries>
```

- Each color is set as a `Color` object. The `SolidColorBrush` object references this `Color` object.
- It is best practice to only reference a `SolidColorBrush` object when authoring controls.
- HighContrast brushes references `System*` contrast colours instead. These colors are defined in the OS system. 
See Contrast Themes section for more info.
- Note that all colors are `#FF0000` (red) for the HighContrast resource dictionary. This is because these colors are not
to be used, but still included in the resource dictionary to not cause a crash.

When given Figma design specs to implement, one can simply inspect the Figma element for the correct brush to use.
However, the resource names usually infer its usage. With time, one can generally grasp the correct brush usage. 

- In this case, `TextFillColorPrimary` points to this brush being used for primary text.
- Generally `*Secondary` points to the _PointerOver_ variation of the primary.
- Generally `*Tertiary` points to the _Pressed_ variation of the primary.
- `*Disabled` points to the _Disabled_ variation of the primary.

#### Accent Colors

Accent colors change according to System theme color settings. As such, they reference `SystemAccent*` colors 
defined in at the dxaml level.
- See [SystemThemingInterop.cpp](../../dxaml/xcp/components/theminginterop/SystemThemingInterop.cpp)
and [FrameworkTheming.cpp](../../dxaml/xcp/components/theming/FrameworkTheming.cpp) for more info.

```xml
<SolidColorBrush x:Key="AccentTextFillColorPrimaryBrush" Color="{ThemeResource SystemAccentColorDark2}" />
<SolidColorBrush x:Key="AccentTextFillColorSecondaryBrush" Color="{ThemeResource SystemAccentColorDark3}" />
<SolidColorBrush x:Key="AccentTextFillColorTertiaryBrush" Color="{ThemeResource SystemAccentColorDark1}" />
```

#### Elevation Border Brushes

```xml
<LinearGradientBrush x:Key="ControlElevationBorderBrush" MappingMode="Absolute" StartPoint="0,0" EndPoint="0,3">
    <LinearGradientBrush.RelativeTransform>
        <ScaleTransform ScaleY="-1" CenterY="0.5" />
    </LinearGradientBrush.RelativeTransform>
    <LinearGradientBrush.GradientStops>
        <GradientStop Offset="0.33" Color="{StaticResource ControlStrokeColorSecondary}" />
        <GradientStop Offset="1.0" Color="{StaticResource ControlStrokeColorDefault}" />
    </LinearGradientBrush.GradientStops>
</LinearGradientBrush>
<LinearGradientBrush x:Key="AccentControlElevationBorderBrush" MappingMode="Absolute" StartPoint="0,0" EndPoint="0,3">
    <LinearGradientBrush.RelativeTransform>
        <ScaleTransform ScaleY="-1" CenterY="0.5" />
    </LinearGradientBrush.RelativeTransform>
    <LinearGradientBrush.GradientStops>
        <GradientStop Offset="0.33" Color="{StaticResource ControlStrokeColorOnAccentSecondary}" />
        <GradientStop Offset="1.0" Color="{StaticResource ControlStrokeColorOnAccentDefault}" />
    </LinearGradientBrush.GradientStops>
</LinearGradientBrush>
```

`ControlElevationBorderBrush` and `AccentControlElevationBorderBrush` are defined `LinearGradientBrush` to create
a drop shadow effect on controls such as buttons.

For example, the default button uses `ControlElevationBorderBrush` and the 
accent button uses `AccentControlElevationBorderBrush`.

```xml
<LinearGradientBrush x:Key="CircleElevationBorderBrush" MappingMode="RelativeToBoundingBox" StartPoint="0,0" EndPoint="0,1">
    <LinearGradientBrush.GradientStops>
        <GradientStop Offset="0.50" Color="{StaticResource ControlStrokeColorDefault}" />
        <GradientStop Offset="0.70" Color="{StaticResource ControlStrokeColorSecondary}" />
    </LinearGradientBrush.GradientStops>
</LinearGradientBrush>
```

`CircleElevationBorderBrush` achieves the same effect but for circle shapes, such as `RadioButton` and `ToggleSwitch`.

Note that above elevation border brushes map to solid System contrast colors for HighContrast:
```xml
<SolidColorBrush x:Key="CircleElevationBorderBrush" Color="{ThemeResource SystemColorWindowTextColor}" />
<SolidColorBrush x:Key="AccentControlElevationBorderBrush" Color="{ThemeResource SystemColorWindowTextColor}" />
<SolidColorBrush x:Key="SystemColorWindowTextColorBrush" Color="{ThemeResource SystemColorWindowTextColor}" />
```
#### Animation Speeds

```xml
<x:String x:Key="ControlFastOutSlowInKeySpline">0,0,0,1</x:String>
<x:String x:Key="ControlNormalAnimationDuration">00:00:00.250</x:String>
<x:String x:Key="ControlFastAnimationDuration">00:00:00.167</x:String>
<x:String x:Key="ControlFastAnimationAfterDuration">00:00:00.168</x:String>
<x:String x:Key="ControlFasterAnimationDuration">00:00:00.083</x:String>
```

Animation speeds for Animation Keytimes are also stored as a reusable resource in the common themeresrouce file.
It is best practice to reference the above resources rather than hard-coding the values.

For more info on usage, reference the [Timing and Easing](https://learn.microsoft.com/en-us/windows/apps/design/motion/timing-and-easing)
page.

### Contrast Themes

Even though each Win11 rejuvenation brush has a HighContrast equivalent, generally the `SystemColor*` contrast brushes
are referenced directly for each control (I am unsure why_).

For full list of contrast brushes and usage, see [High-Contrast Themes](https://learn.microsoft.com/en-us/windows/apps/design/accessibility/high-contrast-themes) 
page for more info.

### Figma to Xaml

To know what themeresource to use, refer to the [Token Mapper](https://www.figma.com/community/plugin/889923752072774330/token-mapper) Figma plugin.

## Misc.

### Quick Start (Authoring a New Control)

WinUI controls live under the `controls` directory. These scripts help set-up scaffolding for a new blank 
control in the repo:
- [GenerateNewControlInProject.cmd](./controls/tools/GenerateNewControlInProject.cmd)
- [GenerateNewControlProject.cmd](./controls/tools/GenerateNewControlProject.cmd)

_Note: They are quite outdated, and as such a lot of set-up needs to be done manually after running these scripts._

