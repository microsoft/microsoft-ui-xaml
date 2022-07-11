NavigationViewTemplateSettings.OpenPaneLength
===

# Background

*This spec adds a OpenPaneLength property to the Xaml [NavigationViewTemplateSettings](https://docs.microsoft.com//windows/winui/api/microsoft.ui.xaml.controls.navigationviewtemplatesettings) class*

The OpenPaneLength TemplateSettings property defines the width of the pane of the NavigationView control when it is open and expanded to its full width. It takes the `min` between `NavigationView.OpenPaneLength` and the app's width. This was necessary to fix a bug where the NavigationView pane contents were being cropped when `OpenPaneLength` surpasses the app's width. `NavigationViewTemplateSettings.OpenPaneLength` is set in `SplitView` and `ShadowCaster` template; it was previously a simple template binding to `NavigationView.OpenPaneLength` property.

TemplateSettings properties are not commonly used by app developers, however, in situations when they copy a WinUI control's template into their own project, there is a Xaml error when the TemplateSettings property is not public.
See [issue #6682](https://github.com/microsoft/microsoft-ui-xaml/issues/6682) for more information on the error caused by non-public TemplateSettings.

# API Pages

## NavigationViewTemplateSettings.OpenPaneLength

Gets the `min` between `OpenPaneLength` and the app's width. This is the calculated value of the width of the pane when opened and fully expanded. Defaults to 320.0.

(Type: Double)

*_Spec note: Usage available to view in this PR: [#7341](https://github.com/microsoft/microsoft-ui-xaml/pull/7341)*

# API Details

```c# (but really MIDL3)
[webhosthidden]
unsealed runtimeclass NavigationViewTemplateSettings : Windows.UI.Xaml.DependencyObject
{
    // ...

    [MUX_DEFAULT_VALUE("320.0")]
    Double OpenPaneLength{ get; };

    static Windows.UI.Xaml.DependencyProperty OpenPaneLengthProperty{ get; };
}
```

# Appendix

The NavigationView pane is open when it is expanded to its full width. It is visible when the pane is shown - even in compact mode. See [NavigationView.IsPaneOpen](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.controls.navigationview.ispaneopen) and [NavigationView.IsPaneVisible](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.controls.navigationview.ispanevisible) for additional information on the distinction.
