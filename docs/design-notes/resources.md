# Resources - Functional and Dev Design

## Table of Contents

- [Terminology](#terminology)
- [Resource dictionary locations](#resource-dictionary-locations)
- [Resource reference resolution](#resource-reference-resolution)
  - [Element tree vs markup tree during resource reference resolution](#element-tree-vs-markup-tree-during-resource-reference-resolution)
- [Special {ThemeResource} behavior (theme changes and styles)](#special-themeresource-behavior-theme-changes-and-styles)
  - [Note for RS1+](#note-for-rs1)
- [RequestedTheme](#requestedtheme)
- [Implicit styles](#implicit-styles)
- [Resolution exception to the rules](#resolution-exception-to-the-rules)
- [Evil ThemeResource tricks that no one should know](#evil-themeresource-tricks-that-no-one-should-know)
- [Magic keys](#magic-keys)
    - [System color/brush names](#system-colorbrush-names)
    - [Accent color names](#accent-color-names)
- [Other trivia](#other-trivia)
  - [StaticResource element](#staticresource-element)
  - [Xaml resources must be shareable](#xaml-resources-must-be-shareable)
  - [Common performance pitfall: app-shared resources defined in a local scope](#common-performance-pitfall-app-shared-resources-defined-in-a-local-scope)
- [Dev Design](#dev-design)

## Terminology

"**{StaticResource}**" and "**{ThemeResource}**" mean a reference to a resource, not the resource itself.
* So "Theme resource" doesn't mean a resource in a theme dictionary, at least not in this document.

Examples:
``` xml
<Rectangle Fill="{StaticResource RectFill}" Width="100" Height="100" />
<Rectangle Fill="{ThemeResource RectFill}" Width="100" Height="100" />
```

"**Theme dictionary**" is a resource dictionary that's an item in a `ResourceDictionary.ThemeDictionaries` property.

Example:
``` xml
<Page.Resources>
    <ResourceDictionary>
        <ResourceDictionary.ThemeDictionaries>
            <ResourceDictionary x:Key="Default">
                <SolidColorBrush x:Key="ThemeRectFill" Color="Blue" />
            </ResourceDictionary>
            <ResourceDictionary x:Key="HighContrast">
                <SolidColorBrush x:Key="ThemeRectFill" Color="Green" />
            </ResourceDictionary>
        </ResourceDictionary.ThemeDictionaries>
        <SolidColorBrush x:Key="RectFill" Color="Red" />
    </ResourceDictionary>
</Page.Resources>
```

"**System resources**" is a resource dictionary that ships with the framework.
* Sometimes these are called "theme resources", but that's easily confused with {ThemeResource} and theme dictionaries.
* At runtime, the location of the system resource dictionaries is not exposed. In fact the system dictionaries aren't necessarily even a dictionary, but they are accessible using {StaticResource} and {ThemeResource}.
* In the SDK, you can see the system resources in markup:
  * They're located in `Program Files (x86)\Windows Kits\8.0\Include\winrt\xaml\design\generic.xaml`
  * A subset of generic.xaml is also located in the same directory, in "themeresources.xaml". These files used to be used by Visual Studio, as originally they needed the theme dictionaries separated out. They no longer do, but we haven't gotten around to removing this SDK file and folks have found it useful to have it separate from the monolithic generic.xaml.

"**System colors**" are really user32 GetSysColor and UxTheme GetUserColorPreference/GetColorFromPreference
* But these too are exposed by {StaticResource} and {ThemeResource}
* These are listed below: [System color/brush names](#system-color%2Fbrush-names)

"**Accent colors**" (aka immersive colors) are the colors defined in the Settings app (light and dark accent colors)
* You can get to those in code with the `UISettings.GetColorValue` API
* These are listed below: [Accent color names](#accent-color-names)

**Confusion on the term "resources"**:
Resources is an overloaded term. It could mean:
* Xaml resources defined in a ResourceDictionary
* The assets used by an application, like an image or a video
* The values used to localize an application, like strings
* MRT resources
This document is talking about the first kind of resource - xaml resources defined in a ResourceDictionary.

## Resource dictionary locations

Places where a resource dictionary can live:

1. A standalone resource dictionary (a Xaml file with `<ResourceDictionary>` as the root)
2. An element in the tree (`FrameworkElement.Resources`)
3. The application (`Application.Resources`)
4. Internally in the framework (sometimes known as "theme resources"):
   * System resources (framework's generic.xaml)
	 * System colors (wrapper around user32)
	 * Accent colors (wrapper around uxtheme on desktop, UISettings on OneCore)
5. Merged dictionaries (`ResourceDictionary.MergedDictionaries`, of any resource dictionary)	
6. Theme dictionaries (`ResourceDictionary.ThemeDictionaries`, of any resource dictionary)

## Resource reference resolution

**Note**: Resource resolution is the same for {StaticResource} and {ThemeResource}.  
* They both look in ResourceDictionary and ResourceDictionary.ThemeDictionaries
* For both, the App resources override the system resources and colors
* The exception, where ThemeResource is different is on theme updates, described in the section below

If a resource is defined in multiple places, the highest priority resource dictionary wins.

Resource dictionary priority†:
* If the root of the markup file is a `<ResourceDictionary>` itself, resources in that dictionary
	* Otherwise, resources defined in the same markup file as the resource reference
* Application resources
* System resources
* System colors

†In general, when a ResourceDictionary is searched for a value, it will search not only its dictionary, but also any Merged and ThemeDictionaries as well. As can be imagined, the calls are recursive, and the order is:
* Current
* Merged
* ThemeDictionary

Once a matching key is found, the search stops, meaning if the key is defined in the current dictionary, neither merged nor ThemeDictionaries will be searched. MergedDictionary search starts with the last merged dictionary, so in the below markup, the Blue brush will always be chosen over the Red one:

``` xml
<ResourceDictionary.MergedDictionaries>
    <ResourceDictionary>
        <SolidColorBrush x:Key='MyBrush'>Red</SolidColorBrush>
    </ResourceDictionary>
    <ResourceDictionary>
     	  <SolidColorBrush x:Key='MyBrush'>Blue</SolidColorBrush>
    </ResourceDictionary>
</ResourceDictionary.MergedDictionaries>
```

When looking in a `ResourceDictionary.ThemeDictionaries`, only one  dictionary within the ThemeDictionaries collection is checked for the resource key. Selecting a dictionary out of ThemeDictionaries also has a process:
* If the system is in high contrast mode:
	* The dictionary with key HighContrastWhite or HighContrastBlack is used, depending on the theme.
	* Note:  if there's no RequestedTheme override set, depending on SKU, sometimes the dictionary must be keyed HighContrastCustom. 
* If in low contrast, or the above didn't find a dictionary yet, the dictionary with key HighContrast is used.
* If nothing found yet, the dictionary with key Light or Dark, based on theme settings, is used
* If nothing found yet, the dictionary with key Default is used.

### Element tree vs markup tree during resource reference resolution

Note the implication of this design, because it's a commonly misunderstood aspect of resource lookup. With one exception, {StaticResource} and {ThemeResource} references are resolved against the current markup tree, not the element tree.   (The exception is for {ThemeResource} references is described in the following section.)

For example, if the app's structure is:
```
App
  PageA
    UserControlB
```

If within UserControlB's markup is a {StaticResource} reference to a resource, that resource must be defined in either UserControlB's xaml file, or app.xaml, or the system resources. Even if PageA defines the resource, it will not be resolved. (This is confusing because asset "resources" like images do walk the tree to resolve relative resource references.)

## Special {ThemeResource} behavior (theme changes and styles)

Initially, {ThemeResource} and {StaticResource} behave the same.  Both look up resources the same way during Xaml loading. Note that this means a {ThemeResource} can be resolved to a key outside of a ThemeDictionary, if the key is also defined in a dictionary that is searched first. Likewise, StaticResources can be resolved to ThemeDictionaries.

But whereas a StaticResource reference is only resolved at Xaml load time, ThemeResource references can be reevaluated when:
* The user changes the system theme (`IContentWindow`'s `ThemeChanged`)
  * This triggers a tree walk from the root visual
* The application changes a `FrameworkElement.RequestedTheme` override property.
  * (Note that changing the `Application.RequestedTheme` is not supported)
  * This triggers a walk from the updated element
* New to RS1: when an element (or its ancestor) is added to the live tree.
  * Note that this includes any style setters applied to the element
  * This also includes VSM at or below the element
* When the shell accent colors change? **TODO**

Also, a ThemeResource on a Style Setter is re-evaluated in the context of each target to which the Style is applied. For example, the following implicit Button style makes buttons purple by default: 

``` xml
<Application>
    <SolidColorBrush x:Key="StyleBrush" Color="Purple" />
    <Application.Resources>

        <Style TargetType="Button">
            <Setter Property="Background" Value="{ThemeResource StyleBrush}" />
        </Style>

    </Application.Resources>
</Application>
```

… but that can be overridden:

``` xml
<StackPanel>
    <StackPanel.Resources>
        <SolidColorBrush x:Key="StyleBrush" Color="Green" />
    </StackPanel.Resources>
    <Button>I'm green</Button>
</StackPanel>
```

When a ThemeResource refresh is triggered by one of the above, the resource is revaluated (re-resolved), following the original algorithm, just like {StaticResource}, except:
* Pre-RS1:  There's no search for resource dictionaries in the tree. Instead, whatever dictionary was found initially is checked again directly
* RS1: The visual tree is walked (up)
* The application is still checked
* System resources and colors are still checked

### Note for RS1+

Prior to RS1, it wasn't possible to override theme resources on the page.  As of RS1, on a page you can override direct ThemeResource references from the template or style, but not indirect references.

For example, you can override the “ButtonBackground” brush, which is defined like this:

``` xml
<Style TargetType="Button">
    <Setter Property="Background" Value="{ThemeResource ButtonBackground}" />
```

But you can’t override the ButtonBackground’s reference to the SystemControlBackgroundBaseLowBrush, which is defined like this:

``` xml
<ResourceDictionary>
    …
    <StaticResource x:Key="ButtonBackground" ResourceKey="SystemControlBackgroundBaseLowBrush" />
</ResourceDictionary>
```

So basically, the way we have our brushes and colors set up, you can override brushes on the page or app, but you can only override colors on the app.

## RequestedTheme

The `Application.RequestedTheme` can't be changed after app-startup.  `FrameworkElement.RequestedTheme`, however, can be changed during the app's runtime.
Support for setting Application.RequestedTheme after loading app.xaml is a known limitation.

## Implicit styles

Like markup resource references, except it walks up the element tree rather than the markup tree
* If the element seeking a style is a template part (in a control template), stop walking at the root
  * Note: WPF also checked the templated parent
* For any resource dictionary found, check MergedDictionaries and ThemeDictionaries as usual
* Priority is the same:
  * ResourceDictionary in the element tree (walking up the logical tree)
  * ResourceDictionary in the Application
  * System resources

## Resolution exception to the rules

Above talks about the priority order when the desired resource key is in multiple dictionaries (tree, app, system).

There's one exception to this… The app and system resources aren't actually checked last; they're checked the first time a resource dictionary is found in the search order.

So if a resource key is set in the parent element's dictionary and the application dictionary, and the current element has a dictionary (without the key), the application will win over the parent.

At parse time, resources can be resolved even if it is defined in the same dictionary, but below the reference. This is different than how WPF worked, in which the markup resolution was a strict "upward" search. Meaning, this can work:

``` xml
<ResourceDictionary>
    <SolidColorBrush x:Key='MyBrush' Color='{StaticResource MyColor}' />
    <Color x:Key='MyColor'>Red</Color>
</ResourceDictionary>
```

And in this scenario, the Red brush wins out..

``` xml
<Page.Resources>
    <Color x:Key='MyColor'>Blue</Color>
</Page.Resources>
<Grid>
    <Grid.Resources>
        <SolidColorBrush x:Key='MyBrush' Color='{StaticResource MyColor}' />
        <Color x:Key='MyColor'>Red</Color>
    </Grid.Resources>
</Grid>
```

## Evil ThemeResource tricks that no one should know

You can force ThemeResource references to update (get updated resources values) for a Window at any time with:
```
  var windowContent = (Window.Current.Content as FrameworkElement);
  windowContent.RequestedTheme = ElementTheme.Default;
  windowContent.ClearValue(RequestedThemeProperty);
```

## Magic keys

System and Accent colors (and brushes). These ultimately come from the [GetSysColor](https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getsyscolor) (Win32) API and from uxtheme (the "accent" aka "immersive" colors, which mostly overlaps with the [UISettings.GetColorValue](https://docs.microsoft.com/en-us/uwp/api/windows.ui.viewmanagement.uisettings.getcolorvalu) API).

#### System color/brush names
"SystemColorActiveCaptionColor"  
"SystemColorBackgroundColor"  
"SystemColorButtonFaceColor"  
"SystemColorButtonTextColor"  
"SystemColorCaptionTextColor"  
"SystemColorGrayTextColor"  
"SystemColorHighlightColor"  
"SystemColorHighlightTextColor"  
"SystemColorHotlightColor"  
"SystemColorInactiveCaptionColor"  
"SystemColorInactiveCaptionTextColor"  
"SystemColorWindowColor"  
"SystemColorWindowTextColor"  
"SystemColorDisabledTextColor"  

"SystemColorActiveCaptionBrush"  
"SystemColorBackgroundBrush"  
"SystemColorButtonFaceBrush"  
"SystemColorButtonTextBrush"  
"SystemColorCaptionTextBrush"  
"SystemColorGrayTextBrush"  
"SystemColorHighlightBrush"  
"SystemColorHighlightTextBrush"  
"SystemColorHotlightBrush"  
"SystemColorInactiveCaptionBrush"  
"SystemColorInactiveCaptionTextBrush"  
"SystemColorWindowBrush"  
"SystemColorWindowTextBrush"  
"SystemColorDisabledTextBrush"  

"SystemColorControlAccentBrush"  

#### Accent color names
"SystemColorControlAccentColor"  
"SystemAccentColor"  

"SystemAccentColorDark1"  
"SystemAccentColorDark2"  
"SystemAccentColorDark3"  
"SystemAccentColorLight1"  
"SystemAccentColorLight2"  
"SystemAccentColorLight3"  

"SystemListAccentLowColor"  
"SystemListAccentMediumColor"  
"SystemListAccentHighColor"  

## Other trivia

### StaticResource element

Like WPF, xaml supports an element syntax for StaticResource references:
``` xml
<Rectangle Width="100" Height="100">
    <Rectangle.Fill>
        <StaticResource ResourceKey="RectFill" />
    </Rectangle.Fill>
</Rectangle>
```

This is not often used, but comes in handy once in a while. As far as I know, element syntax is not supported for ThemeResource references.

### Xaml resources must be shareable

Generally, shareable objects are data-like objects (colors, transforms, etc.). As opposed to elements - items in the tree.

### Common performance pitfall: app-shared resources defined in a local scope

One of the common things we see is apps defining resources at a local scope, but then creating multiple instances of the defining element. Example: the data template for a list contains a UserControl. That UserControl defines resources in a ResourceDictionary. Result: each container in the list will have a separate copy of the resources.

The advice we give in these situations is to move the resources to app.xaml, ensuring there will be only one copy of them at runtime.

## Dev Design

> Bolded references to a method indicate that there's a separate description of that method here.

**`CStaticResourceExtension::CThemeResourceExtension::ResolveInitialValueAndTargetDictionary`**
* (Following applies to both, the two code paths are nearly identical)
* If the markup is a `<ResourceDictionary>`, look there first (**`GetKeyNoRef`**)
* Next check the "ambient" dictionaries (**`GetKeyNoRef`**)
  * These are the dictionaries higher up in the same markup file
  * So this is effectively a markup tree walk, not an element tree walk
* Next check theme dictionaries (**`GetKeyFromGlobalThemeResourceNoRef`**)
  * Note that that checks for an app override if it found a theme resource
* Next check application (LookupApplicationResourceNoRef -> **`GetKeyNoRef`**)

**`CTemplateContent::ResolveReferenceAndAddToLocalDictionary`**
* Calls one of the above
  * `CStaticResourceExtension::LookupResourceNoRef`
  * `CThemeResourceExtension::LookupResource`, which calls **`CThemeResourceExtension::ResolveInitialValueAndTargetDictionary`**
* Except `bShouldCheckThemeResources` is false, so it never calls `LookupApplicationResourceNoRef` 
* And so it never checks for overrides in App.Xaml?

**`CResourceDictionary::GetKeyNoRef`**
* Check this store first (`FindResourceByKey`), undefer if necessary
  * If found, allow the designer to override
* If `!bLocalOnly`, check:
  * Check this dictionary's MergedDictionaries (walking backwards through the collection)
    * Recursive calls to **`GetKeyNoRef`**
    * Unconditionally set `bShouldCheckThemeResources` to false
  * Check this dictionary's theme dictionaries (**`this.GetKeyFromThemeDictionariesNoRef`**)
  * If still not found, and there were no merged dictionaries, check global theme dictionary (**`GetKeyFromGlobalThemeResourceNoRef`**)
    * IIRC, the "if no merged dictionaries" is because the check in theme dictionaries happened inside the check of merged dictionaries
* Who calls with `bLocalOnly` set true?:
  * `StyleCache::EnsureSubResourceDictionaryIsLoaded`
    * Called by `DefaultStyles::GetDefaultStyleByTypeInfo`/`GetDefaultStyleByTypeName`
  * `CResourceDictionary::Remove`
  * `CCoreServices::LookupApplicationResourceNoRef`, when `localOnly` is true
    * Which is never

**`CResourceDictionary::GetKeyFromThemeDictionariesNoRef`**
* Keeps track of active theme in `m_pActiveThemeDictionary`
  * Detects if the theme has changed since `m_pActiveThemeDictionary` was calculated last
* Find the correct theme dictionary:
  * Uses `m_pCore->GetThemeRequestedForSubTree`, which is a push/pop stack keeping track of `RequestedTheme` during a tree walk.
  * `m_pCore->GetFrameworkTheming` has the overall system theme (user setting)
    * Even if `RequestedTheme` is set as an override, you need to know this, because `RequestedTheme` provides light/dark and the overall system them tells you if you're in high or low contrast.
  * For high contrast, if there's a `RequestedTheme` override in the tree, the theme dictionaries must be named (depending on the value of `RequestedTheme`):
    * `HighContrastWhite`
    * `HighContrastBlack`
  * For high contrast where there's no `RequestedTheme` override in the tree, the theme dictionaries can be named (depending on the system theme):
    * `HighContrastBlack`
    * `HighContrastWhite`
    * `HighContrastCustom`
  * If in high contrast, and the above didn't find anything, check for a dictionary named "HighContrast".
  * If nothing found so far, look for a dictionary named "Light" or "Dark", based on the user setting or RequestedTheme override
  * If still nothing found, look for a dictionary named "Default".
  * If some step above found a dictionary:
    * Cache it in `m_pActiveThemeDictionary` so we don't have to look it up again
    * If this is a theme change, call `NotifyThemeChanged` on this dictionary
* If we have an active theme dictionary, look for the resource in it (**`GetKeyNoRef`**)
  * If we find it in there, still check for an app override
    * `GetKeyOverrideFromApplicationResourcesNoRef`, which calls `LookupApplicationResourceNoRef` , which calls **`GetKeyNoRef`**

**`CResourceDictionary::GetKeyFromGlobalThemeResourceNoRef`**
* Check for system colors
  * Look in system colors dictionary (`pCore->GetSystemColorsResources,` then **`GetKeyNoRef`**)
    * This gets loaded by `CCoreServices::UpdateColorAndBrushResources` (or is provided by the designer)
    * For the non-designer case, this ultimately comes from `FrameworkTheming::RebuildColorAndBrushResources`
    * Relies on User32 `GetSysColor` 
      * `COLOR_ACTIVECAPTION`, `COLOR_WINDOWTEXT`, `COLOR_HIGHLIGHT`, etc
    * Also relies on uxtheme's `GetUserColorPreference`/`GetColorFromPreference`, `UISettings` on OneCore
      * Aka "immersive colors"
      * `SystemAccentColorDark1`, `SystemAccentColorDark2`, `SystemAccentColorLight1`, etc
  * Try again if not found by `FrameworkCallbacks_EnsureImmersiveResource`
  * If found by one or the other, check `App.Resources` override (`GetKeyOverrideFromApplicationResourcesNoRef` -> **`GetKeyNoRef`**)
* Look in global theme resources (`pCore->GetThemeResources`)
  * Theme resources are ultimately a Win32 `FindResource`/`LoadResource` call to get the mapped buffer
  * If found, check `App.Resources` override (`GetKeyOverrideFromApplicationResourcesNoRef` -> **`GetKeyNoRef`**)
	
**`CDeferredKeys::ResolveAndSaveResource`**
* Calls **`GetKeyNoRef`** with false for `bShouldCheckThemeResources`

**`CThemeResource::RefreshValue`**
* This is called when the theme or `RequestedTheme` changes
  * Propagation 
    * Starts `CDependencyObject::NotifyThemeChanged,` which calls `NotifyThemeChangedCore`
    * `CFrameworkElement` overrides `NotifyThemeChangedCore` to walk inherited properties
    * `CUIElement` overrides to walk visual tree
  * For `RequestedTheme` case, it's detected in `CFrameworkElement::SetValue`
  * For system theme change, it's detected with the `IContentWindow` in `CXamlIslandRoot` (`ThemeChanged`), which eventually calls `FrameworkTheming::OnThemeChanged`, which starts the `NotifyThemeChanged` walk
* Calls back to the originally-found dictionary (`m_pTargetDictionaryWeakRef`, **`GetKeyNoRef`**)
* Since this calls **`GetKeyNoRef`**, it will re-check the theme and app
	
**`CFrameworkElement::EnsureImplicitStyle`**
* Walk up the (logical) element tree
  * For each one, call **`GetKeyNoRef`**
    * Thus it checks the system resources
  * In a control template, if the element seeking a style (the template part) isn't a Control, stop walking at the template root
    * WPF also checks the templated parent (the Control)
* Check the app resources

**Determining the system theme (light vs dark)**

* Look at HKCU, in the key `SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize`, in the value `AppsUseLightTheme`; 0 means dark and 1 means light.
* If it’s not there in HKCU, check the same place in HKLM.
* On older Windows Phone and Xbox, neither of those is set, and there’s a `GetThemeServices` API that’s used as a fallback.
* If none of the above work, then we check if the immersive color for `ApplicationBackground` is white; if so we’re light, dark otherwise.
* There's a comment in the code that there should really be a Shell API for this

**To do**

* Explain `CPopup::SetOverlayThemeBrush`
