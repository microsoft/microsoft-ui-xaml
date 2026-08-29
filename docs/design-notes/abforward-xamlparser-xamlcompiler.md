# A/B Forward Compatibility - XAML Parser and XamlCompiler

## Background

A/B Forward Compatibility is a capability that allows big apps to safely test and slowly move users forward to the next
version of Windows App SDK. In order to support this capability, both the XAML Parser and XamlCompiler will need to be
modified to output XBF files and generated code, respectively, that is compatible with Windows App SDK \[N-1, N\].

Fundamentally, this is a limited version of the "light-up" problem that we solved for UWP apps and so the same techniques
can be employed, namely Conditional XAML for markup and version checks guarding new API usage sites in code-behind.

### Conditional XAML

Conditional XAML is a XAML language extension that allows for version adaptive markup to be written by selectively parsing
elements or attributes to determine whether they will be available at runtime. As this is an existing feature, this 
document will not discuss its syntax or general usage; instead, the [MSDN article](https://learn.microsoft.com/en-us/windows/uwp/debug-test-perf/conditional-xaml) 
should be consulted for that information.

Unfortunately, there are currently some limitations on where and how Conditional XAML can be used. Specifically, 
`Style` _`Setters`_ cannot have a Conditional XAML annotation; thus, if a `Style` wishes to set a new property, then it
must be duplicated (one copy sets the property and the other does not) and Conditional XAML employed at the usage site
to select the correct Style. It should also be noted that `VisualStateManager` is slightly less performant if Conditional
XAML is used on a descendant of a `VisualStateGroupCollection`. Additionally, none of the extant Conditional XAML predicates
are suitable for use with A/B Forward Compatibility so a new one will need to be created, mirroring the new Windows 
App SDK API for detecting current runtime version.

### XBF

Thanks to earlier investments during UWP XAML development, XBF does not require any changes to support A/B Forward 
Compatibiltiy. It also already has versioning capabilities if changes are needed in the future.

### Generated Code

XamlCompiler generates code during app compilation that supports both the runtime itself (e.g. `IXamlMetadataProvider`
implementation) and various features (e.g. x:Bind). Currently, this generated code does not rely on any APIs that would
render it forward incompatible. If in the future XamlCompiler is modified to output generated code that relies on a new API,
then that code will need to be appropriately guarded by a version check. For now, nothing needs to be done in this area.

## Proposal

* **(Required)** Add support for Conditional XAML to `Style`

  Although the limitation noted above can be worked around, it is suboptimal from a usability standpoint and so it would be
  better if it didn't exist. It should also be noted that if we punt this work to some future version N, then developers would
  be unable to take advantage of it until version N+1. We thus should do this work in 1.7

* **(Optional, punt to 1.8)** Implement new Conditional XAML predicate

  This new predicate would call the Windows App SDK runtime version check API [`Microsoft.Windows.ApplicationModel.WindowsAppRuntime.ReleaseInfo`](https://github.com/microsoft/WindowsAppSDK/blob/80e530d6692167e2927b4010b067b4ef79b8aae6/dev/VersionInfo/VersionInfo.idl#L11-L27) 
  as a straight passhthrough and be named accordingly: `IsWindowsAppSDKReleaseMajorMinorPresent(<major>,<minor>)`. 
  Note that there would also be a negated version of this predicate, `IsWindowsAppSDKReleaseMajorMinorNotPresent(<major>,<minor>)`.

  Strictly speaking this is not required as the existing ApiInformation-based predicates are functional but it's useful to have
  in case those are considered too slow despite their inherent result caching. However, the fact that the version check API queries
  for a specific version of the runtime, and XAML does not have a preprocessor to allow for a constant to be defined in a single
  easy-to-update place that is then used by all appropriate Conditional XAML predicates, a predicate mirroring the API would be
  onerious to use as upgrading the version of WinAppSDK used would require updating *every* usage site of that predicate. As such,
  it would be better to only implement this new predicate if it's absolutely required for performance reasons.

  It should be noted that if this proposed predicate is implemented in 1.8 (N), then there are two options:
  1. Backport to 1.7 (N-1) simultaneously with the release in 1.8 (N). However, this can only be done safely if an app targeting
     1.8 (N) and using A/B Forward can *guarantee* that the version of 1.7 (N-1) installed on the customer's machine has the minimum
     required patch level.
  2. Include in 1.8 (N) a block that prevents an app targeting 1.8 from *compiling* if the new predicate is used in markup (i.e. XBF
     generation fails) and then remove that block in 1.9 (N+1) so that apps targeting 1.9 (N+1) or later can use the new predicate even
     if run against 1.8 (N).

  Because of the technical dependency present in Option 1, it is not possible at this time to recommend an approach without further
  investigation.

## Example
```xml
<StackPanel
 xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
 xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
 xmlns:winui17="http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,8)"
 xmlns:winui18="http://schemas.microsoft.com/winfx/2006/xaml/presentation?IsApiContractPresent(Microsoft.UI.Xaml.WinUIContract,9)">
 <TextBlock>
   <TextBlock.Style>
     <Style TargetType="TextBlock">
       <Style.Setters>
         <winui17:Setter Property="SelectionHighlightColor" Value="Blue" />
         <winui18:Setter Property="SelectionHighlightColor" Value="Gold" />
         <winui18:Setter Property="SomeNewTextBlockProperty" Value="lorem ipsum" />
       </Style.Setters>
     </Style>
   </TextBlock.Style>
 </TextBlock>
</StackPanel>;
```

## Discussion

* What about `VisualStateManager`?

  The performance impact (loss of XBFv2 optimization for that particular `VisualStateManager` instance) is unfortunate but also
  expected to be minor. A developer wishing to avoid the performance impact could duplicate the `VisualStateManager` instance
  and annotate each instance appropriate at the top level, similar to what a developer would need to do for `Style`.
  Due to the complexity of the XBFv2 optimization for `VisualStateManager`, adding proper support for Conditional XAML to `
  VisualStateManager` is expected to be very expensive. Given the uncertain ROI and limited available resources, it would be best to tackle this at some later point (if ever).
