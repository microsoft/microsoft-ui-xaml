# Authoring a new Control in Xaml

## Table of Contents

- [Intro](#intro)
- [UserControl vs Control](#usercontrol-vs-control)
- [Basics](#basics)
- [DependencyProperties](#dependencyproperties)
- [Template Parts and OnApplyTemplate](#template-parts-and-onapplytemplate)
- [Default Style](#default-style)
  - [Template-bind placeholder properties on Control](#template-bind-placeholder-properties-on-control)
- [Resources](#resources)
  - [Brush Resources and Theming](#brush-resources-and-theming)
  - [Light-weight Styling](#light-weight-styling)
  - [StaticResource vs ThemeResource](#staticresource-vs-themeresource)
  - [Switching between Light, Dark and High Contrast](#switching-between-light-dark-and-high-contrast)
- [Visual State Manager](#visual-state-manager)
- [Mouse, Touch, Keyboard](#mouse-touch-keyboard)
  - [Tab Stops and Keyboard Navigation](#tab-stops-and-keyboard-navigation)
  - [Focus Visuals](#focus-visuals)
- [UIAutomation and Accessibility](#uiautomation-and-accessibility)
  - [Automation Patterns](#automation-patterns)
  - [Automation Properties](#automation-properties)
  - [Inpect.exe and Accessibility Insights](#inpectexe-and-accessibility-insights)
  - [Narrator](#narrator)
- [Localization](#localization)

## Intro
This document describes how to author a new control in Xaml. It assumes that you are already familiar with general Xaml
application development and the various Xaml runtime systems. 

It covers not just the initial creation of the basic control but also what needs to be done to get the new control to 
parity with the existing first party controls. This includes considerations such as theming, localization, accessibility, etc.

This document applies to both implementing a new 'first-party' Xaml Control in the platform itself and to a third-party 
writing a custom Control. The high-level process is the same in both cases.

For a deeper dive on Xaml styling, theming, and best practices, refer to [Xaml Style Guide](.\design-notes\xaml-styling-guide.md) for more info.

## UserControl vs Control
In Xaml there are two types of custom controls: UserControl and "Templated Control". The first decision you will need to make
is to decide which type of custom control to create. What is the difference between these two types of control?

A UserControl derives from the type Microsoft.UI.Xaml.Controls.UserControl. It is a way to encapsulate some Xaml UI and
some code into a new type. The Xaml and the code are tightly bound and the Xaml Compiler will produce some generated code
from the .xaml file to make writing the code easier, e.g. for every element with an `x:Name` attribute, the Xaml Compiler
will generate a matching field in your class that allows you to access that element. In general, a UserControl is easier 
to create, but it is less flexible than a Templated Control.

A Templated Control is a type that derives from Microsoft.UI.Xaml.Controls.Control. All of the controls that are built
into the framework such as Button and NavigationView are Templated Controls. The major difference between a Templated 
Control and a UserControl is that a Templated Control can be re-templated by the app author by applying a custom Style
to the control. For example, if you don't like the way a default Button looks like, you can provide your own visuals. 
As a result of this flexibility, in a Templated Control the Xaml and the code are loosely bound which makes writing one 
a little bit more work. You also get less help from the Xaml Compiler as there is no generated code to support the creation
of a Templated Control.

So, which should you create, a UserControl or a Templated Control?

In general: if you are creating a custom control just for use in your own app, a UserControl is usually the best way to 
go due to being simpler to create. If you are creating a library of UI controls for use by third parties, a Templated 
Control is recommended due to the extra flexibility they provide to the consumer. All of the controls that ship in the
Xaml framework itself are Templated Controls.

This document is focused on the creation of Templated Controls not UserControl.

## Basics

At a high level, the approach you will take to creating a custom control is:

1. Create a new type deriving from Control (or a subclass of Control). 
2. Define your control's public API on this new type.
3. Create your control's default Style and Template. This will define the overall look of your control.
4. Implement the code for your control. This will determine the behavior of your control.

This approach to creating a new control is covered in these public docs:

* [Build XAML controls with C#](https://learn.microsoft.com/windows/apps/winui/winui3/xaml-templated-controls-csharp-winui-3)
* [Build XAML controls with C++/WinRT](https://learn.microsoft.com/windows/apps/winui/winui3/xaml-templated-controls-cppwinrt-winui-3)

These docs walk you through how to create a simple custom control consisting of its default `Style` and `ControlTemplate` 
along with the control's code.

This doc outlines how to go further and bring your control to a level that is a 'production ready' and in a state that
is ready to ship.

## DependencyProperties

In general, any publicly settable properties on your new control class should be implemented as dependency properties 
rather than as regular properties. Defining your properties as DependencyProperties allow them to take part in Xaml Runtime
systems such as Styling and Data-binding. It is a bit more work to define a property as a `DependencyProperty`. See
these docs for more information:

* [Dependency properties overview](https://learn.microsoft.com/windows/uwp/xaml-platform/dependency-properties-overview)
* [Custom dependency properties](https://learn.microsoft.com/windows/uwp/xaml-platform/custom-dependency-properties)

Note: In the Microsoft.UI.Xaml.Controls project, we use some custom code generation to automatically generate the boiler-plate 
code required for DependencyProperties. See [DependencyPropertyCodeGen.cs](controls\tools\CustomTasks\DependencyPropertyCodeGen.cs)
for more information and see the files under [controls\dev\Generated](controls\dev\Generated) for example output. This 
only applies to control development in the Microsoft.UI.Xaml.Controls project in this repo. For authoring controls in other
projects, this code will need to be hand-written.

## Template Parts and OnApplyTemplate

One thing to realize about a Templated Control is that the contents of the template are not immediately instantiated
when the control is first created. The template is lazily expanded when the control first goes through layout. As a result
of this you cannot assume that elements in the template are immediately available. This is a difference between a Templated 
Control and a UserControl.

The approach you should take is:

1. For every element in the template that you need to access from your Control's code you should set the x:Name attribute 
on that element in the template. 
2. Create a corresponding member variable of the appropriate type in the class of your control. 
3. Override the [OnApplyTemplate](https://learn.microsoft.com/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.frameworkelement.onapplytemplate?view=windows-app-sdk-1.1) method of Control. In this method, call GetTemplateChild to find the named element and 
assign it to the appropriate member variable.
4. Any time you access the member variable, check for null first to ensure that you don't try to use the variable before
it has been assigned.

When creating the member variable to hold a reference to the Control's template part, it is best practice to use a variable 
of the most generic type possible. If your Xaml template uses a Button but the code behind only uses some of its FrameworkElement 
properties, use a FrameworkElement variable. This allows more flexibility in the customers' custom templates.

Another thing to consider about template parts is that even though you have defined a default template for your Control,
the consumer of your Control might provide a custom template. This custom template is not guaranteed to contain every named
part that you expect. You should aim to have the functionality of your control apply graceful degradation in this case. 
At the very least, your control should not crash if it is missing named template parts.

## Default Style

For any properties on your control that you want to specify default values, use Setters in the default Style:

```xml
<Style TargetType="local:MyControl">
  <Setter Property="MinWidth" Value="100" />
  <Setter Property="MaxWidth" Value="500" />
</Style>
```

This is a better approach than setting these properties in your Control's constructor. By setting them in the Style it 
allows a consumer of your control to provide their own custom Style. If these default values were set in your Control's
constructor, that would stomp on any values coming from a custom Style.

### Template-bind placeholder properties on Control

The Control base class defines a number of properties that by default don't actually do anything, such as Background,
BorderBrush, BorderThickness, Foreground, HorizontalContentAlignment, Padding, VerticalContentAlignment. These properties
are defined on Control to act as placeholders (e.g. it saves you from having to define your own Background property on 
your custom control class). You can hook them up by TemplateBinding them to elements in your default ControlTemplate.

```xml
<Style TargetType="local:MyControl">
  <Setter Property="Background" Value="Red" />
  <Setter Property="Padding" Value="5" />
  <Setter Property="ControlTemplate">
    <Setter.Value>
      <ControlTemplate TargetType="local:MyControl">
        <Grid x:Name="RootGrid" Background="{TemplateBinding Background}" Margin="{TemplateBinding Padding}" />
      </ControlTemplate>
  </Setter>
</Style>
```

You can use TemplateBinding to bind any property of an element in your ControlTemplate to any property on your custom
Control.

Note: Some text related properties don't need to be explicitly template bound and will automatically propagate down the 
tree. These include: FontFamily, FontSize, FontStretch, FontStyle, FontWeight and also Foreground. If you don't set these
values explicitly on a template part such as a TextBlock or a ContentPresenter, the values will be automatically picked up
from the parent Control without an explicit TemplateBinding.

## Resources

When you create your default control template to define the overall look of the control you could hard-code all of the
values that you use directly in the template. For example:
```xml
<ControlTemplate>
  <Grid Background="#CC000000" Width="200" />
</ControlTemplate>
```
However, it is better to use resources in this case. There are two reasons for this:
* Theming
* Light-weight Styling

Theming: By hard-coding the background color you are preventing your control from supporting Light and Dark theme and 
High Contrast mode. This is a **must-fix issue** for the control.

Light-weight styling: A consumer of the control can provide a complete template to replace the default template. But if 
they only wish to make slight adjustments, it is more convenient if they can simply provide new theme resource values. 
For example, if they wished to change the Width of the Grid above it would be more convenient if they could override a 
single resource value rather than creating a full copy of the template which potentially could be very large. Supporting 
this kind of light-weight styling for your control is a **nice-to-have** rather than a strict requirement.

### Brush Resources and Theming
To enable theming in your control, you should avoid hard-coding any Color or Brush properties. Instead you should use a
ThemeResource to point to an appropriate Color or Brush value in a ThemeDictionary.


```xml
<Grid Background="{ThemeResource ControlFillColorDefaultBrush}" Width="200">
```
ControlFillColorDefaultBrush is a Brush resource that is built is defined in `common_themeresources_any.xaml` and resolves to a 
SolidColorBrush of #0FFFFFFF in dark theme, #B3FFFFFF in light theme and SystemColorButtonFaceColor in High Contrast theme.

You can view [common_themeresources_any.xaml](..\controls\dev\CommonStyles\Common_themeresources_any.xaml) to see the resources available to you. 
All new controls are to use brushes from this file. See [Xaml Style Guide](.\design-notes\xaml-styling-guide.md) for more info.

There are also resources with `System*` names that are built into the xaml platform. These are old resources from before the windows 11 rejuvenation.
You can view them at [generic.xaml](..\dxaml\xcp\dxaml\themes\generic.xaml).

### Light-weight Styling
To support light-weight styling in your control, you add an extra layer of indirection in the resource lookup. 

For example, in your control's template you would have:
```xml
<Grid Background="{ThemeResource MyControlGridBackground}" Width="{ThemeResource MyControlGridWidth}">
```
You would also define a set of ResourceDictionaries that look like:

```xml
<ResourceDictionary>
  <ResourceDictionary.ThemeDictionaries>
    <ResourceDictionary x:Key="Default">
      <StaticResource x:Key="MyControlGridBackground" ResourceKey="ControlFillColorDefaultBrush" />
    </ResourceDictionary>
    <ResourceDictionary x:Key="Light">
      <StaticResource x:Key="MyControlGridBackground" ResourceKey="ControlFillColorDefaultBrush" />
    </ResourceDictionary>
    <ResourceDictionary x:Key="HighContrast">
      <StaticResource x:Key="MyControlGridBackground" ResourceKey="ControlFillColorDefaultBrush" />
    </ResourceDictionary>
  </ResourceDictionary.ThemeDictionaries>
  <x:Double x:Key="MyControlGridWidth">200</x:Double>
</ResourceDictionary>
```
Pay attention to the layout of the resources here. Some notes:

1. MyControlGridWidth does not need to be defined in a ThemeDictionary. Its value does not change based on theme, so it
is ok for it to exist in the 'root' of the ResourceDictionary.
2. MyControlGridBackground must be defined in the three ThemeDictionaries. The multiple definitions appear redundant, but
they are not, since each resolves 'ControlFillColorDefaultBrush' to a different value.

[More information on Light-Weight Styling](https://learn.microsoft.com/en-us/windows/apps/design/style/xaml-styles#lightweight-styling).

### StaticResource vs ThemeResource
It is possible to use both StaticResource and ThemeResource to set properties to resource values. StaticResource is a 
one-time lookup that occurs at the time that the Xaml is parsed. A ThemeResource gets re-evaluated when the theme changes
between Light and Dark or when High Contrast mode is turned on or off. When either of these happens, the relevant 
ThemeDictionary in use gets switched out.

Note the subtlety in how the resources are defined in this example. For the brush example, from the Control, we use a
ThemeResource to point to MyControlGridBackground, which resolves to a entry in a ThemeDictionary that in turn uses a 
StaticResource to point to a Brush (ControlFillColorDefaultBrush). When the theme changes, the ThemeResource
gets updated, but the StaticResource does not. This explains why we would not want to have a single 
`<StaticResource x:Key="MyControlGridBackground" />` in the root ResourceDictionary.

More information on [Xaml Theme resources](https://learn.microsoft.com/en-us/windows/apps/design/style/xaml-theme-resources).

### Switching between Light, Dark and High Contrast
After you have set up your ThemeResources in your Control's default Template, you should try out your control and switch
between Light and Dark theme and turn High Contrast mode on and off. Make sure that your control looks good in all cases
and that it properly updates when the theme changes. If you see cases where a color does not update when the theme changes
it is likely that either a hard-coded value or a StaticResource is being used.

## Visual State Manager
In implementing your Control you will likely want to modify elements in the template. For example, in response to a 
pointer over event you may want to update a background color of an element. As outlined in 'Template Parts and OnApplyTemplate' 
above you have access to named template parts from your Control's code. So you can directly modify the required properties
that way. However, a preferred approach is to use VisualStateManager. VisualStateManager allows you to define a set of
states in your template and from code you can switch between these states. The advantage of this over modifying
the template parts directly from code is that because the VSM states are defined in Xaml in the template, a consumer of
the control can customize how they want the control to look in the various states. 

See the docs for more information on how to use VisualStateManager: 
[VisualStateManager Class](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.visualstatemanager)

Note: One thing to bear in mind is that a single property on a given element should only be updated from at most one 
VisualStateGroup. Otherwise you can end up in a situation where two different VisualStates from two different 
VisualStateGroups are attempting to set the property to different values. This can result in unexpected behavior, so you
should avoid this.

## Mouse, Touch, Keyboard
You should make sure that your Control works well with all input types, including Mouse, Pen, Touch and Keyboard.

### Tab Stops and Keyboard Navigation
By default a Control has IsTabStop=True. This means that a user can press tab and put focus on your control. In some 
cases a Control is mainly a container to hold its various template parts and does not need to be focusable itself. In 
this case you should set IsTabStop=False in the Control's default Style.

You should make sure that keyboard navigation works well in general with the Control and its various parts. If the 
default behavior does not work well, you can customize things by setting properties on your Control and its parts, including
[IsTabStop](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.uielement.istabstop), 
[TabIndex](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.uielement.TabIndex) and 
[TabNavigation](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.control.TabNavigation). 

You can also enable navigation using the arrow keys with [XYFocusKeyboardNavigation](https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.uielement.xyfocuskeyboardnavigation)

If you are unable to get the desired behavior by setting these properties you can subscribe to the GettingFocus and LosingFocus 
events to programmatically hook into the focus navigation system and redirect focus programmatically as desired. See 
[GettingFocusEventArgs.TrySetNewFocusedElement](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.input.gettingfocuseventargs.trysetnewfocusedelement)
for more information.

### Focus Visuals
You want the user to be able to visually see when your Control has focus. The most straight-forward way to enable this is
to set UseSystemFocusVisuals to true on your Control in its default Style:

```
<Setter Property="UseSystemFocusVisuals" Value="True" />
```

When your Control has focus, the Xaml runtime will draw a rectangle around your Control to visually indicate that it has 
focus. To customize the size and position of this focus visual you can use the [Control.IsTemplateFocusTarget](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.control.istemplatefocustarget)
attached property.

If you want to fully customize the focus visuals of your Control, set UseSystemFocusVisuals to false so that the system
does not draw its focus visual. Then add a new VisualStateGroup to your Control containing Focused, PointerFocused and 
Unfocused states:

```xml
<VisualStateGroup x:Name="FocusStates">
  <VisualState x:Name="Unfocused" />
  <VisualState x:Name="PointerFocused" />
  <VisualState x:Name="Focused">
    <VisualState.Setters>
      <!-- Add the necessary Setters here. -->
    </VisualState.Setters>
  </VisualState>
  <VisualState x:Name="PointerFocused" />
</VisualStateGroup>
```
Then update your Control's code so that it transitions to these states in response to the GotFocus and LostFocus events.
Use the value of Control.FocusState to determine which state to go to. If FocusState is Pointer you should go to the 
empty PointerFocused state since you usually don't want to visually show focus visuals when the user is interacting with
mouse or other pointer device.

## UIAutomation and Accessibility
Another major aspect of implementing a new Control is supporting UIAutomation. 

UIAutomation is a Microsoft technology that allows programmatic access to UI. It provides an API that is neutral with respect to 
the specific UI framework being targeted (e.g. Win32, WPF, Xaml, HTML, etc.). The two main uses of UIA are for test 
automation and for accessibility tools such as Narrator.

You can learn more about UIA here:
https://docs.microsoft.com/en-us/windows/win32/winauto/uiauto-uiautomationoverview

When you implement a new Control you will need to ensure that it works well with UIAutomation.

If your Control is just an aggregation of existing Controls then it is possible that you will not need to implement any
custom UIA code. In most cases however, you will need to implement custom UIA code.

UIA is based on [Properties](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-propertiesoverview) and
[Patterns](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-controlpatternsoverview). You should familiarize
yourself with these concepts.

In Xaml, the way a Control implements UIA patterns is by creating a custom AutomationPeer for your control. An AutomationPeer
is a Xaml concept that is used to implement the UIA properties, patterns and events. 

To create a custom AutomationPeer for your Control, define a new class that derrives from [FrameworkElementAutomationPeer](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.automation.peers.frameworkelementautomationpeer) 
(or a subclass of this). In your Control class, override [UIElement.OnCreateAutomationPeer](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.uielement.oncreateautomationpeer) 
so that your custom Control uses your custom AutomationPeer.

### Automation Patterns

In your custom AutomationPeer class, implement the appropriate interfaces corresponding to the desired UIA Patterns. E.g.
to implement the Invoke pattern your AutomationPeer should implement the [Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.automation.provider.iinvokeprovider).

Once you have the list of patterns that your AutomationPeer implements, override the [AutomationPeer.GetPatternCore](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.automation.peers.automationpeer.getpatterncore) 
method so that your automation peer responds correctly to queries about which patterns it supports. (GetPatternCore can 
be considered as the UIA analogue to QueryInterface in COM, UIA will query the AutomationPeer about which patterns it 
supports).

To help determine what set of Patterns your Control should support, you should first determine which UIA [Control Type](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-controltypesoverview) 
most closely matches your control. This will give an initial list of patterns to implement. Override [AutomationPeer.GetAutomationControlTypeCore](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.automation.peers.automationpeer.getautomationcontroltype) 
to return the correct AutomationControlType. You must implement all the Patterns that are required for a given ControlType. 
You should also implement any other patterns that apply.

### Automation Properties
You can also provide values for any of the UIA Properties by overriding the appropriate getter on AutomationPeer. One 
particularly important property is the Name property. This is the main thing that is read out by Narrator when focusing
the Control, so it should provide useful information to the end user about what the control is. For example, if the control
is a close button with a X icon, the UIA Name should be "Close" so that the user knows what the button does. 

In some cases, you will have UIElements in the tree that don't need to show up in UIA. For example if you have a Button
that contains a TextBlock to display an 'X' character to indicate the 'close' action, you don't need both the Button and
the TextBlock to show up in the UIA tree. You can resolve this by setting AutomationProperties.AccessibilityView="Raw" on
the TextBlock. It will still be available in the UIA tree, but most tools will ignore it. See [AccessibilityView ](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.automation.peers.accessibilityview?view=windows-app-sdk-1.2) 
and [UI Automation Tree Overview](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-treeoverview) for more info.

### Inpect.exe and Accessibility Insights
When working on implementing UIAutomation for your Control a very useful tool is [Inspect.exe](https://learn.microsoft.com/en-us/windows/win32/winauto/inspect-objects).
This tool is included in the Windows SDK and can be launched by typing `inspect.exe` on a Visual Studio command prompt.
This will allow you to view the UIA tree of the entire Windows desktop. You can view all the properties and patterns on
a given control and can interact with the control via the patterns that it implements.

If you want something more modern and user friendly you can also download and install [Accessibility Insights](https://accessibilityinsights.io/). 
This tool provides similar functionality to inspect and can also automatically detect accessibility issues in your app.

### Narrator
Narrator is an accessibility tool that ships in Windows. It allows blind and partially sighted users to interact with 
Windows and applications running on it. See [Complete guide to Narrator](https://support.microsoft.com/en-us/windows/complete-guide-to-narrator-e4397a0d-ef4f-b386-d8ae-c172f109bdb1)
for more information. Narrator uses UIAutomation to read and interact with the UI of an app. After implementing your 
Control's AutomationPeer, you should make sure that your Control works well with Narrator. Make sure that a Narrator user
can perform all the same actions that a fully sighted user can and make sure that all information that is available to
a fully sighted user is also available to a Narrator user.

## Localization
You should not hard-code any English language strings into your Control's UI. You should instead use localized string 
resources. See [App resources and the Resource Management System](https://learn.microsoft.com/en-us/windows/uwp/app-resources/)
for more information.

Note: 'resources' in this context are completely distinct from Xaml ThemeResources/StaticResources.

For more information on how we run the localization process itself in the WinUI repo, see [localization-process.md](localization-process.md).