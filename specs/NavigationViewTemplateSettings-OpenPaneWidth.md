NavigationViewTemplateSettings.OpenPaneWidth
===

# Background

*This spec adds a OpenPaneWidth property to the Xaml [NavigationViewTemplateSettings](https://docs.microsoft.com//windows/winui/api/microsoft.ui.xaml.controls.navigationviewtemplatesettings) class*

<!-- Explanation for why we introduced this: To follow best API design practices to allow for better customization with the control template it was added to NavigationView's template as a preview API -->
The OpenPaneWidth property defines the width of the pane of the NavigationView control when it is open and expanded to its full width. It takes the `min` between `OpenPaneLength` and the app's width. This was necessary to fix a bug where the NavView pane contents were being cropped when `OpenPaneLength` surpasses the app's width. `TemplateSettings.OpenPaneWidth` is set in `SplitView` and `ShadowCaster` template; it was previously a simple templatebinding to NavigationView's OpenPaneLength property.

TemplateSetting properties are not commonly used by app developers, however, in situations when they copy a WinUI control's template into their own project, there is a Xaml error when the TemplateSettings property is not public.
See [issue #6682](https://github.com/microsoft/microsoft-ui-xaml/issues/6682) for more information on the error caused by non-public TemplateSettings.

# API Pages

## NavigationViewTemplateSettings.OpenPaneWidth

Gets the `min` between `OpenPaneLength` and the app's width. This is the calculated value of the width of the pane when opened and fully expanded. Defaults to 320.0.

(Type: Double)

*_Spec note: Usage available to view in NavigationView.xaml today on line [472](https://github.com/microsoft/microsoft-ui-xaml/blob/d3fef08fdf2b3e86386928097216fdfbedfda02c/dev/NavigationView/NavigationView.xaml#L472) & [686](https://github.com/microsoft/microsoft-ui-xaml/blob/d3fef08fdf2b3e86386928097216fdfbedfda02c/dev/NavigationView/NavigationView.xaml#L686)*

<!-- Note: Why is OpenPaneLength=OpenPaneWidth in the RootSplitView but Width=OpenPaneWidth in the ShadowCaster? -->

# API Details

```c# (but really MIDL3)
[webhosthidden]
unsealed runtimeclass NavigationViewTemplateSettings : Windows.UI.Xaml.DependencyObject
{
    // ...

    [MUX_DEFAULT_VALUE("320.0")]
    Double OpenPaneWidth{ get; };

    static Windows.UI.Xaml.DependencyProperty OpenPaneWidthProperty{ get; };
}
```

# Appendix

The NavigationView pane is open when it is expanded to its full width. It is visible when the pane is shown - even in compact mode. See [NavigationView.IsPaneOpen](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.controls.navigationview.ispaneopen) and [NavigationView.IsPaneVisible](https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.controls.navigationview.ispanevisible) for additional information on the distinction.
