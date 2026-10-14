
# Background

> This spec corresponds to [issue 2007](https://github.com/microsoft/microsoft-ui-xaml/issues/2007) on the WinUI repo.

The WinUI [TabView](https://docs.microsoft.com/uwp/api/Microsoft.UI.Xaml.Controls.TabView) control is a way to display a set of tabs and their respective content. There's also a TabView control in the [Windows Community Toolkit](https://docs.microsoft.com/en-us/windows/communitytoolkit/controls/tabview), which has two main features not found in the WinUI version.:
* **TabWidthMode: Compact** ([WCT API Link](https://docs.microsoft.com/en-us/dotnet/api/microsoft.toolkit.uwp.ui.controls.tabwidthmode?view=win-comm-toolkit-dotnet-stable)) 
* **Overlay close button** ([WCT API Link](https://docs.microsoft.com/en-us/dotnet/api/microsoft.toolkit.uwp.ui.controls.tabview.isclosebuttonoverlay?view=win-comm-toolkit-dotnet-stable#Microsoft_Toolkit_Uwp_UI_Controls_TabView_IsCloseButtonOverlay))

Once these feature gaps have been closed and the WinUI TabView is at parity with the Windows Community Toolkit version, we will be able to deprecate the WCT TabView control.

# Examples

## TabView.TabWidthMode

A TabView with two items, showing only the icons for the item tabs because TabWidthMode is set to Compact

```XML
<muxc:TabView TabWidthMode="Compact" x:Name="TabView1">
    <muxc:TabView.TabItems>
    
        <muxc:TabViewItem Header="Events" >
            <muxc:TabViewItem.IconSource>
                <muxc:SymbolIconSource Symbol="Calendar" />
            </muxc:TabViewItem.IconSource>
                    
            <!-- ... -->
                    
        </muxc:TabViewItem>

        <muxc:TabViewItem Header="Messages">
            <muxc:TabViewItem.IconSource>
                <muxc:SymbolIconSource Symbol="Message" />
            </muxc:TabViewItem.IconSource>

            <!-- ... -->

        </muxc:TabViewItem>
    </muxc:TabView.TabItems>
</muxc:TabView>
```

## TabView.CloseButtonOverlayMode

A TabView where the unselected tab items have a close button that only displays when the mouse is over it. (The selected tab item always has a close button, regardless of this property value.)

```XML
<muxc:TabView CloseButtonOverlayMode="OnPointerOver">
    <!-- ... -->
</muxc:TabView>
```

# API Notes

## TabViewWidthMode: Compact

This change introduces a new value to the existing `TabViewWidthMode` enum: `Compact`.

Setting `Compact` will cause unselected tab headers to display only the tab's icon. If an icon is not set, the tab will be the same size as if an icon was present but display an empty space where the icon would be.

The selected tab will render at its natural size (ie. the size it would render in `SizeToContent` mode.)

![Compact](./TabView_Width_Compact.png)

## CloseButtonOverlayMode

Describes the behavior of the x-to-close button found on each TabViewItem. 

The `CloseButtonOverlayMode` enum has three values: {`Auto`, `OnPointerOver`, `Always`}

> The `CloseButtonOverlayMode` will only affect tabs that are closeable - ie. the value of the `TabViewItem`'s `IsClosable` property is `TRUE`.

`OnPointerOver`: The x-to-close button only appears on the selected tab and any tabs that are being hovered.

![Hover](./TabView_Close_Hover.gif)

`Always`: The x-to-close button always appears on every tab. 

![Persistent](./TabView_Close_Persistent.png)

`Auto`: Currently maps to `Always`.

> Prior to this change, the behavior of the x-to-close button was `Always` with no option to change the behavior.

> See the [Appendix](#Appendix) section for why this value is an enum and not a boolean.

# API Details
<!-- The exact API, in MIDL3 format (https://docs.microsoft.com/en-us/uwp/midl-3/) -->

```
[WUXC_VERSION_MUXONLY]
[webhosthidden]
enum TabViewWidthMode
{
    Equal = 0,
    SizeToContent = 1,
    Compact = 2,
};

[WUXC_VERSION_MUXONLY]
[webhosthidden]
enum TabViewCloseButtonOverlayMode
{
    Auto = 0,
    OnPointerOver = 1,
    Always = 2,
};

[WUXC_VERSION_MUXONLY]
[webhosthidden]
[contentproperty("TabItems")]
unsealed runtimeclass TabView : Windows.UI.Xaml.Controls.Control
{

    [WUXC_VERSION_PREVIEW]
    [MUX_DEFAULT_VALUE("winrt::TabViewCloseButtonOverlayMode::Auto")]
    [MUX_PROPERTY_CHANGED_CALLBACK(TRUE)]
    TabViewCloseButtonOverlayMode CloseButtonOverlayMode{ get; set; };

    [WUXC_VERSION_PREVIEW]
    static Windows.UI.Xaml.DependencyProperty CloseButtonOverlayModeProperty{ get; };
}
```

# Appendix
<!-- Anything else that you want to write down for posterity, but 
that isn't necessary to understand the purpose and usage of the API.
For example, implementation details. -->

**Why is CloseButtonOverlayMode an enum and not a boolean?**

1. The corresponding visual effect might change over time

The CloseButtonOverlayMode enum describes a visual effect that is likely to change over time. For example, Spartan Edge used the OnPointerOver model, whereas chromium Edge uses a Always model. An enum allows that platform to have an opinion (via `Auto`) and an option to change the behavior over time in a way that a boolean does not. 

2. Per-device input support

The original spec had discussion around the desired default (Always vs. pointer over) both in the context of user experience as well as touch-friendliness. An enum value provides the platform an option to set per-device or per-input behavior if desired.

3. Futureproofing

An enum gives the platform an option to introduce other modes in the future. Although there haven't been specific modes proposed yet, an enum leaves the door open for changes.
