// Workaround for Deliverable 18767852: WinMD Feature Request: Custom attributes (for codegen hints) which are stripped out of SDK metadata
// In the OS repo we can't use IDL attributes as they would be part of public metadata. So instead
// we write this summary file which in the dep.controls/MUX build is an output file. But in the OS
// build it can be a 'sidecar' file that carries the metadata that we can't store in the IDL.
// So the truth is in the IDL but in the OS repo if you need to RunWUXCCodeGen without round-tripping
// through the dep.controls build then you can manually edit MetadataSummary.cs in a pinch.
using System;
using System.Collections.Generic;

namespace CustomTasks
{
    public class MetadataSummary
    {
        public Dictionary<string, bool> IncludedTypesMetadata { get; set; }
        public Dictionary<string, bool> HasCustomActivationFactoryMetadata { get; set; }
        public Dictionary<string, bool> NeedsDependencyPropertyFieldMetadata { get; set; }
        public Dictionary<string, bool?> NeedsPropChangedCallbackMetadata { get; set; }
        public Dictionary<string, string> PropChangedCallbackMethodNameMetadata { get; set; }
        public Dictionary<string, string> PropValidationCallbackMetadata { get; set; }
        public Dictionary<string, string> PropertyTypeOverrideMetadata { get; set; }
        public Dictionary<string, string> DefaultValueMetadata { get; set; }

        public MetadataSummary()
        {
            IncludedTypesMetadata = new Dictionary<string, bool>();
            HasCustomActivationFactoryMetadata = new Dictionary<string, bool>();
            NeedsDependencyPropertyFieldMetadata = new Dictionary<string, bool>();
            NeedsPropChangedCallbackMetadata = new Dictionary<string, bool?>();
            PropChangedCallbackMethodNameMetadata = new Dictionary<string, string>();
            PropValidationCallbackMetadata = new Dictionary<string, string>();
            PropertyTypeOverrideMetadata = new Dictionary<string, string>();
            DefaultValueMetadata = new Dictionary<string, string>();


            IncludedTypesMetadata["AcrylicBrush"] = true;
            HasCustomActivationFactoryMetadata["AcrylicBrush"] = true;
            // AcrylicBrush -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["AcrylicBrush.AlwaysUseFallback"] = true;
            PropChangedCallbackMethodNameMetadata["AcrylicBrush.AlwaysUseFallback"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["AcrylicBrush.BackgroundSource"] = true;
            PropChangedCallbackMethodNameMetadata["AcrylicBrush.BackgroundSource"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["AcrylicBrush.TintColor"] = true;
            PropChangedCallbackMethodNameMetadata["AcrylicBrush.TintColor"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["AcrylicBrush.TintLuminosityOpacity"] = true;
            PropChangedCallbackMethodNameMetadata["AcrylicBrush.TintLuminosityOpacity"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["AcrylicBrush.TintLuminosityOpacity"] = "CoerceToZeroOneRange_Nullable";
            NeedsPropChangedCallbackMetadata["AcrylicBrush.TintOpacity"] = true;
            PropChangedCallbackMethodNameMetadata["AcrylicBrush.TintOpacity"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["AcrylicBrush.TintOpacity"] = "CoerceToZeroOneRange";
            NeedsPropChangedCallbackMetadata["AcrylicBrush.TintTransitionDuration"] = true;
            PropChangedCallbackMethodNameMetadata["AcrylicBrush.TintTransitionDuration"] = "OnPropertyChanged";
            // AcrylicBrush -- DefaultValueMetadata
            DefaultValueMetadata["AcrylicBrush.BackgroundSource"] = @"winrt::AcrylicBackgroundSource::Backdrop";
            DefaultValueMetadata["AcrylicBrush.TintColor"] = @"AcrylicBrush::sc_defaultTintColor";
            DefaultValueMetadata["AcrylicBrush.TintOpacity"] = @"AcrylicBrush::sc_defaultTintOpacity";
            DefaultValueMetadata["AcrylicBrush.TintTransitionDuration"] = @"AcrylicBrush::sc_defaultTintTransitionDuration";

            IncludedTypesMetadata["AnimatedVisualPlayer"] = true;
            // AnimatedVisualPlayer -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["AnimatedVisualPlayer.AutoPlay"] = true;
            NeedsPropChangedCallbackMetadata["AnimatedVisualPlayer.FallbackContent"] = true;
            NeedsPropChangedCallbackMetadata["AnimatedVisualPlayer.PlaybackRate"] = true;
            NeedsPropChangedCallbackMetadata["AnimatedVisualPlayer.Source"] = true;
            NeedsPropChangedCallbackMetadata["AnimatedVisualPlayer.Stretch"] = true;
            // AnimatedVisualPlayer -- DefaultValueMetadata
            DefaultValueMetadata["AnimatedVisualPlayer.AutoPlay"] = @"true";
            DefaultValueMetadata["AnimatedVisualPlayer.PlaybackRate"] = @"1";
            DefaultValueMetadata["AnimatedVisualPlayer.Stretch"] = @"winrt::Stretch::Uniform";

            IncludedTypesMetadata["BitmapIconSource"] = true;
            // BitmapIconSource -- NeedsPropChangedCallbackMetadata
            // BitmapIconSource -- DefaultValueMetadata
            DefaultValueMetadata["BitmapIconSource.ShowAsMonochrome"] = @"true";

            IncludedTypesMetadata["ButtonInteraction"] = true;
            // ButtonInteraction -- NeedsPropChangedCallbackMetadata
            // ButtonInteraction -- DefaultValueMetadata

            IncludedTypesMetadata["ColorPicker"] = true;
            // ColorPicker -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["ColorPicker.Color"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.Color"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.ColorSpectrumComponents"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.ColorSpectrumComponents"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.ColorSpectrumShape"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.ColorSpectrumShape"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.IsAlphaEnabled"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.IsAlphaEnabled"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.IsAlphaSliderVisible"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.IsAlphaSliderVisible"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.IsAlphaTextInputVisible"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.IsAlphaTextInputVisible"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.IsColorChannelTextInputVisible"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.IsColorChannelTextInputVisible"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.IsColorPreviewVisible"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.IsColorPreviewVisible"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.IsColorSliderVisible"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.IsColorSliderVisible"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.IsColorSpectrumVisible"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.IsColorSpectrumVisible"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.IsHexInputVisible"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.IsHexInputVisible"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.IsMoreButtonVisible"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.IsMoreButtonVisible"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.MaxHue"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.MaxHue"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.MaxSaturation"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.MaxSaturation"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.MaxValue"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.MaxValue"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.MinHue"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.MinHue"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.MinSaturation"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.MinSaturation"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.MinValue"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.MinValue"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorPicker.PreviousColor"] = true;
            PropChangedCallbackMethodNameMetadata["ColorPicker.PreviousColor"] = "OnPropertyChanged";
            // ColorPicker -- DefaultValueMetadata
            DefaultValueMetadata["ColorPicker.Color"] = @"{ 255, 255, 255, 255 }";
            DefaultValueMetadata["ColorPicker.ColorSpectrumComponents"] = @"winrt::ColorSpectrumComponents::HueSaturation";
            DefaultValueMetadata["ColorPicker.ColorSpectrumShape"] = @"winrt::ColorSpectrumShape::Box";
            DefaultValueMetadata["ColorPicker.IsAlphaSliderVisible"] = @"true";
            DefaultValueMetadata["ColorPicker.IsAlphaTextInputVisible"] = @"true";
            DefaultValueMetadata["ColorPicker.IsColorChannelTextInputVisible"] = @"true";
            DefaultValueMetadata["ColorPicker.IsColorPreviewVisible"] = @"true";
            DefaultValueMetadata["ColorPicker.IsColorSliderVisible"] = @"true";
            DefaultValueMetadata["ColorPicker.IsColorSpectrumVisible"] = @"true";
            DefaultValueMetadata["ColorPicker.IsHexInputVisible"] = @"true";
            DefaultValueMetadata["ColorPicker.MaxHue"] = @"359";
            DefaultValueMetadata["ColorPicker.MaxSaturation"] = @"100";
            DefaultValueMetadata["ColorPicker.MaxValue"] = @"100";
            DefaultValueMetadata["ColorPicker.MinHue"] = @"0";
            DefaultValueMetadata["ColorPicker.MinSaturation"] = @"0";
            DefaultValueMetadata["ColorPicker.MinValue"] = @"0";

            IncludedTypesMetadata["ColorPickerSlider"] = true;
            // ColorPickerSlider -- NeedsPropChangedCallbackMetadata
            // ColorPickerSlider -- DefaultValueMetadata
            DefaultValueMetadata["ColorPickerSlider.ColorChannel"] = @"winrt::ColorPickerHsvChannel::Value";

            IncludedTypesMetadata["ColorSpectrum"] = true;
            // ColorSpectrum -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["ColorSpectrum.Color"] = true;
            PropChangedCallbackMethodNameMetadata["ColorSpectrum.Color"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorSpectrum.Components"] = true;
            PropChangedCallbackMethodNameMetadata["ColorSpectrum.Components"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorSpectrum.HsvColor"] = true;
            PropChangedCallbackMethodNameMetadata["ColorSpectrum.HsvColor"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorSpectrum.MaxHue"] = true;
            PropChangedCallbackMethodNameMetadata["ColorSpectrum.MaxHue"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorSpectrum.MaxSaturation"] = true;
            PropChangedCallbackMethodNameMetadata["ColorSpectrum.MaxSaturation"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorSpectrum.MaxValue"] = true;
            PropChangedCallbackMethodNameMetadata["ColorSpectrum.MaxValue"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorSpectrum.MinHue"] = true;
            PropChangedCallbackMethodNameMetadata["ColorSpectrum.MinHue"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorSpectrum.MinSaturation"] = true;
            PropChangedCallbackMethodNameMetadata["ColorSpectrum.MinSaturation"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorSpectrum.MinValue"] = true;
            PropChangedCallbackMethodNameMetadata["ColorSpectrum.MinValue"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ColorSpectrum.Shape"] = true;
            PropChangedCallbackMethodNameMetadata["ColorSpectrum.Shape"] = "OnPropertyChanged";
            // ColorSpectrum -- DefaultValueMetadata
            DefaultValueMetadata["ColorSpectrum.Color"] = @"{ 255, 255, 255, 255 }";
            DefaultValueMetadata["ColorSpectrum.Components"] = @"winrt::ColorSpectrumComponents::HueSaturation";
            DefaultValueMetadata["ColorSpectrum.HsvColor"] = @"{ 0, 0, 1, 1 }";
            DefaultValueMetadata["ColorSpectrum.MaxHue"] = @"359";
            DefaultValueMetadata["ColorSpectrum.MaxSaturation"] = @"100";
            DefaultValueMetadata["ColorSpectrum.MaxValue"] = @"100";
            DefaultValueMetadata["ColorSpectrum.MinHue"] = @"0";
            DefaultValueMetadata["ColorSpectrum.MinSaturation"] = @"0";
            DefaultValueMetadata["ColorSpectrum.MinValue"] = @"0";
            DefaultValueMetadata["ColorSpectrum.Shape"] = @"winrt::ColorSpectrumShape::Box";

            IncludedTypesMetadata["CommandBarFlyoutCommandBar"] = true;
            // CommandBarFlyoutCommandBar -- NeedsPropChangedCallbackMetadata
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBar.FlyoutTemplateSettings"] = true;
            // CommandBarFlyoutCommandBar -- DefaultValueMetadata

            IncludedTypesMetadata["CommandBarFlyoutCommandBarTemplateSettings"] = true;
            // CommandBarFlyoutCommandBarTemplateSettings -- NeedsPropChangedCallbackMetadata
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.CloseAnimationEndPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.ContentClipRect"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.CurrentWidth"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.Dispatcher"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.ExpandDownAnimationEndPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.ExpandDownAnimationHoldPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.ExpandDownAnimationStartPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.ExpandDownOverflowVerticalPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.ExpandedWidth"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.ExpandUpAnimationEndPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.ExpandUpAnimationHoldPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.ExpandUpAnimationStartPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.ExpandUpOverflowVerticalPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.OpenAnimationEndPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.OpenAnimationStartPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.OverflowContentClipRect"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.WidthExpansionAnimationEndPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.WidthExpansionAnimationStartPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.WidthExpansionDelta"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.WidthExpansionMoreButtonAnimationEndPosition"] = true;
            NeedsDependencyPropertyFieldMetadata["CommandBarFlyoutCommandBarTemplateSettings.WidthExpansionMoreButtonAnimationStartPosition"] = true;
            // CommandBarFlyoutCommandBarTemplateSettings -- DefaultValueMetadata

            IncludedTypesMetadata["ElementAnimator"] = true;
            // ElementAnimator -- NeedsPropChangedCallbackMetadata
            // ElementAnimator -- DefaultValueMetadata

            IncludedTypesMetadata["FlowLayout"] = true;
            // FlowLayout -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["FlowLayout.LineAlignment"] = true;
            PropChangedCallbackMethodNameMetadata["FlowLayout.LineAlignment"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["FlowLayout.MinColumnSpacing"] = true;
            PropChangedCallbackMethodNameMetadata["FlowLayout.MinColumnSpacing"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["FlowLayout.MinRowSpacing"] = true;
            PropChangedCallbackMethodNameMetadata["FlowLayout.MinRowSpacing"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["FlowLayout.Orientation"] = true;
            PropChangedCallbackMethodNameMetadata["FlowLayout.Orientation"] = "OnPropertyChanged";
            // FlowLayout -- DefaultValueMetadata
            DefaultValueMetadata["FlowLayout.LineAlignment"] = @"winrt::FlowLayoutLineAlignment::Start";
            DefaultValueMetadata["FlowLayout.MinColumnSpacing"] = @"0.0";
            DefaultValueMetadata["FlowLayout.MinRowSpacing"] = @"0.0";
            DefaultValueMetadata["FlowLayout.Orientation"] = @"winrt::Orientation::Horizontal";

            IncludedTypesMetadata["FontIconSource"] = true;
            // FontIconSource -- NeedsPropChangedCallbackMetadata
            // FontIconSource -- DefaultValueMetadata
            DefaultValueMetadata["FontIconSource.FontFamily"] = @"{ c_fontIconSourceDefaultFontFamily }";
            DefaultValueMetadata["FontIconSource.FontSize"] = @"20.0";
            DefaultValueMetadata["FontIconSource.FontStyle"] = @"winrt::FontStyle::Normal";
            DefaultValueMetadata["FontIconSource.FontWeight"] = @"{ 400 }";
            DefaultValueMetadata["FontIconSource.IsTextScaleFactorEnabled"] = @"true";

            IncludedTypesMetadata["IconSource"] = true;
            // IconSource -- NeedsPropChangedCallbackMetadata
            // IconSource -- DefaultValueMetadata

            IncludedTypesMetadata["IDynamicAnimatedVisualSource"] = true;
            // IDynamicAnimatedVisualSource -- NeedsPropChangedCallbackMetadata
            // IDynamicAnimatedVisualSource -- DefaultValueMetadata

            IncludedTypesMetadata["IRefreshInfoProvider"] = true;
            // IRefreshInfoProvider -- NeedsPropChangedCallbackMetadata
            // IRefreshInfoProvider -- DefaultValueMetadata

            IncludedTypesMetadata["IRepeaterScrollingSurface"] = true;
            // IRepeaterScrollingSurface -- NeedsPropChangedCallbackMetadata
            // IRepeaterScrollingSurface -- DefaultValueMetadata

            IncludedTypesMetadata["IScrollController"] = true;
            // IScrollController -- NeedsPropChangedCallbackMetadata
            // IScrollController -- DefaultValueMetadata

            IncludedTypesMetadata["ItemsRepeater"] = true;
            // ItemsRepeater -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["ItemsRepeater.Animator"] = true;
            PropChangedCallbackMethodNameMetadata["ItemsRepeater.Animator"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["ItemsRepeater.Background"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ItemsRepeater.HorizontalCacheLength"] = true;
            PropChangedCallbackMethodNameMetadata["ItemsRepeater.HorizontalCacheLength"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ItemsRepeater.ItemsSource"] = true;
            PropChangedCallbackMethodNameMetadata["ItemsRepeater.ItemsSource"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ItemsRepeater.ItemTemplate"] = true;
            PropChangedCallbackMethodNameMetadata["ItemsRepeater.ItemTemplate"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ItemsRepeater.Layout"] = true;
            PropChangedCallbackMethodNameMetadata["ItemsRepeater.Layout"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ItemsRepeater.VerticalCacheLength"] = true;
            PropChangedCallbackMethodNameMetadata["ItemsRepeater.VerticalCacheLength"] = "OnPropertyChanged";
            // ItemsRepeater -- DefaultValueMetadata
            DefaultValueMetadata["ItemsRepeater.HorizontalCacheLength"] = @"2.0";
            DefaultValueMetadata["ItemsRepeater.Layout"] = @"winrt::StackLayout()";
            DefaultValueMetadata["ItemsRepeater.VerticalCacheLength"] = @"2.0";

            IncludedTypesMetadata["Layout"] = true;
            // Layout -- NeedsPropChangedCallbackMetadata
            // Layout -- DefaultValueMetadata

            IncludedTypesMetadata["LayoutPanel"] = true;
            // LayoutPanel -- NeedsPropChangedCallbackMetadata
            // LayoutPanel -- DefaultValueMetadata

            IncludedTypesMetadata["MenuBar"] = true;
            // MenuBar -- NeedsPropChangedCallbackMetadata
            // MenuBar -- DefaultValueMetadata

            IncludedTypesMetadata["MenuBarItem"] = true;
            // MenuBarItem -- NeedsPropChangedCallbackMetadata
            // MenuBarItem -- DefaultValueMetadata

            IncludedTypesMetadata["MUXControlsTestHooks"] = true;
            // MUXControlsTestHooks -- NeedsPropChangedCallbackMetadata
            // MUXControlsTestHooks -- DefaultValueMetadata

            IncludedTypesMetadata["NavigationView"] = true;
            // NavigationView -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["NavigationView.AlwaysShowHeader"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.AlwaysShowHeader"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.AutoSuggestBox"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.AutoSuggestBox"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.CompactModeThresholdWidth"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.CompactModeThresholdWidth"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["NavigationView.CompactModeThresholdWidth"] = "CoerceToGreaterThanZero";
            NeedsPropChangedCallbackMetadata["NavigationView.CompactPaneLength"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.CompactPaneLength"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["NavigationView.CompactPaneLength"] = "CoerceToGreaterThanZero";
            PropChangedCallbackMethodNameMetadata["NavigationView.ContentOverlay"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.DisplayMode"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.DisplayMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.ExpandedModeThresholdWidth"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.ExpandedModeThresholdWidth"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["NavigationView.ExpandedModeThresholdWidth"] = "CoerceToGreaterThanZero";
            NeedsPropChangedCallbackMetadata["NavigationView.Header"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.Header"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.HeaderTemplate"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.HeaderTemplate"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.IsBackButtonVisible"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.IsBackButtonVisible"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.IsBackEnabled"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.IsBackEnabled"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.IsPaneOpen"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.IsPaneOpen"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.IsPaneToggleButtonVisible"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.IsPaneToggleButtonVisible"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.IsPaneVisible"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.IsPaneVisible"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.IsSettingsVisible"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.IsSettingsVisible"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.MenuItemContainerStyle"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.MenuItemContainerStyle"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.MenuItemContainerStyleSelector"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.MenuItemContainerStyleSelector"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.MenuItems"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.MenuItems"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.MenuItemsSource"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.MenuItemsSource"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.MenuItemTemplate"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.MenuItemTemplate"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.MenuItemTemplateSelector"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.MenuItemTemplateSelector"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.OpenPaneLength"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.OpenPaneLength"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["NavigationView.OpenPaneLength"] = "CoerceToGreaterThanZero";
            NeedsPropChangedCallbackMetadata["NavigationView.OverflowLabelMode"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.OverflowLabelMode"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["NavigationView.PaneCustomContent"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.PaneDisplayMode"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.PaneDisplayMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.PaneFooter"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.PaneFooter"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["NavigationView.PaneHeader"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.PaneTitle"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.PaneTitle"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.PaneToggleButtonStyle"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.PaneToggleButtonStyle"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.SelectedItem"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.SelectedItem"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.SelectionFollowsFocus"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.SelectionFollowsFocus"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.SettingsItem"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.SettingsItem"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["NavigationView.ShoulderNavigationEnabled"] = true;
            PropChangedCallbackMethodNameMetadata["NavigationView.ShoulderNavigationEnabled"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["NavigationView.TemplateSettings"] = "OnPropertyChanged";
            // NavigationView -- DefaultValueMetadata
            DefaultValueMetadata["NavigationView.AlwaysShowHeader"] = @"true";
            DefaultValueMetadata["NavigationView.CompactModeThresholdWidth"] = @"641.0";
            DefaultValueMetadata["NavigationView.CompactPaneLength"] = @"48.0";
            DefaultValueMetadata["NavigationView.DisplayMode"] = @"winrt::NavigationViewDisplayMode::Minimal";
            DefaultValueMetadata["NavigationView.ExpandedModeThresholdWidth"] = @"1008.0";
            DefaultValueMetadata["NavigationView.IsBackButtonVisible"] = @"winrt::NavigationViewBackButtonVisible::Auto";
            DefaultValueMetadata["NavigationView.IsPaneOpen"] = @"true";
            DefaultValueMetadata["NavigationView.IsPaneToggleButtonVisible"] = @"true";
            DefaultValueMetadata["NavigationView.IsPaneVisible"] = @"true";
            DefaultValueMetadata["NavigationView.IsSettingsVisible"] = @"true";
            DefaultValueMetadata["NavigationView.OpenPaneLength"] = @"320.0";
            DefaultValueMetadata["NavigationView.OverflowLabelMode"] = @"winrt::NavigationViewOverflowLabelMode::MoreLabel";
            DefaultValueMetadata["NavigationView.PaneDisplayMode"] = @"winrt::NavigationViewPaneDisplayMode::Auto";
            DefaultValueMetadata["NavigationView.SelectionFollowsFocus"] = @"winrt::NavigationViewSelectionFollowsFocus::Disabled";
            DefaultValueMetadata["NavigationView.ShoulderNavigationEnabled"] = @"winrt::NavigationViewShoulderNavigationEnabled::Never";

            IncludedTypesMetadata["NavigationViewItem"] = true;
            // NavigationViewItem -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["NavigationViewItem.Icon"] = true;
            // NavigationViewItem -- DefaultValueMetadata
            DefaultValueMetadata["NavigationViewItem.CompactPaneLength"] = @"48.0";
            DefaultValueMetadata["NavigationViewItem.SelectsOnInvoked"] = @"true";

            IncludedTypesMetadata["NavigationViewItemPresenter"] = true;
            // NavigationViewItemPresenter -- NeedsPropChangedCallbackMetadata
            // NavigationViewItemPresenter -- DefaultValueMetadata

            IncludedTypesMetadata["NavigationViewTemplateSettings"] = true;
            // NavigationViewTemplateSettings -- NeedsPropChangedCallbackMetadata
            // NavigationViewTemplateSettings -- DefaultValueMetadata
            DefaultValueMetadata["NavigationViewTemplateSettings.BackButtonVisibility"] = @"winrt::Visibility::Collapsed";
            DefaultValueMetadata["NavigationViewTemplateSettings.LeftPaneVisibility"] = @"winrt::Visibility::Visible";
            DefaultValueMetadata["NavigationViewTemplateSettings.OverflowButtonVisibility"] = @"winrt::Visibility::Collapsed";
            DefaultValueMetadata["NavigationViewTemplateSettings.PaneToggleButtonVisibility"] = @"winrt::Visibility::Visible";
            DefaultValueMetadata["NavigationViewTemplateSettings.TopPadding"] = @"0.0";
            DefaultValueMetadata["NavigationViewTemplateSettings.TopPaneVisibility"] = @"winrt::Visibility::Collapsed";

            IncludedTypesMetadata["ParallaxView"] = true;
            // ParallaxView -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["ParallaxView.Child"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.Child"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.HorizontalShift"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.HorizontalShift"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.HorizontalSourceEndOffset"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.HorizontalSourceEndOffset"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.HorizontalSourceOffsetKind"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.HorizontalSourceOffsetKind"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.HorizontalSourceStartOffset"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.HorizontalSourceStartOffset"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.IsHorizontalShiftClamped"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.IsHorizontalShiftClamped"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.IsVerticalShiftClamped"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.IsVerticalShiftClamped"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.MaxHorizontalShiftRatio"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.MaxHorizontalShiftRatio"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.MaxVerticalShiftRatio"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.MaxVerticalShiftRatio"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.Source"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.Source"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.VerticalShift"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.VerticalShift"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.VerticalSourceEndOffset"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.VerticalSourceEndOffset"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.VerticalSourceOffsetKind"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.VerticalSourceOffsetKind"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ParallaxView.VerticalSourceStartOffset"] = true;
            PropChangedCallbackMethodNameMetadata["ParallaxView.VerticalSourceStartOffset"] = "OnPropertyChanged";
            // ParallaxView -- DefaultValueMetadata
            DefaultValueMetadata["ParallaxView.HorizontalSourceOffsetKind"] = @"winrt::ParallaxSourceOffsetKind::Relative";
            DefaultValueMetadata["ParallaxView.IsHorizontalShiftClamped"] = @"true";
            DefaultValueMetadata["ParallaxView.IsVerticalShiftClamped"] = @"true";
            DefaultValueMetadata["ParallaxView.MaxHorizontalShiftRatio"] = @"1.0";
            DefaultValueMetadata["ParallaxView.MaxVerticalShiftRatio"] = @"1.0";
            DefaultValueMetadata["ParallaxView.VerticalSourceOffsetKind"] = @"winrt::ParallaxSourceOffsetKind::Relative";

            IncludedTypesMetadata["PathIconSource"] = true;
            // PathIconSource -- NeedsPropChangedCallbackMetadata
            // PathIconSource -- DefaultValueMetadata

            IncludedTypesMetadata["PersonPicture"] = true;
            // PersonPicture -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["PersonPicture.BadgeGlyph"] = true;
            PropChangedCallbackMethodNameMetadata["PersonPicture.BadgeGlyph"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["PersonPicture.BadgeImageSource"] = true;
            PropChangedCallbackMethodNameMetadata["PersonPicture.BadgeImageSource"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["PersonPicture.BadgeNumber"] = true;
            PropChangedCallbackMethodNameMetadata["PersonPicture.BadgeNumber"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["PersonPicture.BadgeText"] = true;
            PropChangedCallbackMethodNameMetadata["PersonPicture.BadgeText"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["PersonPicture.Contact"] = true;
            PropChangedCallbackMethodNameMetadata["PersonPicture.Contact"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["PersonPicture.DisplayName"] = true;
            PropChangedCallbackMethodNameMetadata["PersonPicture.DisplayName"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["PersonPicture.Initials"] = true;
            PropChangedCallbackMethodNameMetadata["PersonPicture.Initials"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["PersonPicture.IsGroup"] = true;
            PropChangedCallbackMethodNameMetadata["PersonPicture.IsGroup"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["PersonPicture.PreferSmallImage"] = true;
            PropChangedCallbackMethodNameMetadata["PersonPicture.PreferSmallImage"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["PersonPicture.ProfilePicture"] = true;
            PropChangedCallbackMethodNameMetadata["PersonPicture.ProfilePicture"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["PersonPicture.TemplateSettings"] = true;
            PropChangedCallbackMethodNameMetadata["PersonPicture.TemplateSettings"] = "OnPropertyChanged";
            NeedsDependencyPropertyFieldMetadata["PersonPicture.TemplateSettings"] = true;
            // PersonPicture -- DefaultValueMetadata

            IncludedTypesMetadata["PersonPictureTemplateSettings"] = true;
            // PersonPictureTemplateSettings -- NeedsPropChangedCallbackMetadata
            NeedsDependencyPropertyFieldMetadata["PersonPictureTemplateSettings.ActualImageBrush"] = true;
            NeedsDependencyPropertyFieldMetadata["PersonPictureTemplateSettings.ActualInitials"] = true;
            NeedsDependencyPropertyFieldMetadata["PersonPictureTemplateSettings.Dispatcher"] = true;
            // PersonPictureTemplateSettings -- DefaultValueMetadata

            IncludedTypesMetadata["RadioButtons"] = true;
            // RadioButtons -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["RadioButtons.Header"] = true;
            PropChangedCallbackMethodNameMetadata["RadioButtons.Header"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RadioButtons.Items"] = true;
            PropChangedCallbackMethodNameMetadata["RadioButtons.Items"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RadioButtons.ItemsSource"] = true;
            PropChangedCallbackMethodNameMetadata["RadioButtons.ItemsSource"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RadioButtons.ItemTemplate"] = true;
            PropChangedCallbackMethodNameMetadata["RadioButtons.ItemTemplate"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RadioButtons.MaximumColumns"] = true;
            PropChangedCallbackMethodNameMetadata["RadioButtons.MaximumColumns"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RadioButtons.SelectedIndex"] = true;
            PropChangedCallbackMethodNameMetadata["RadioButtons.SelectedIndex"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RadioButtons.SelectedItem"] = true;
            PropChangedCallbackMethodNameMetadata["RadioButtons.SelectedItem"] = "OnPropertyChanged";
            // RadioButtons -- DefaultValueMetadata
            DefaultValueMetadata["RadioButtons.MaximumColumns"] = @"1";
            DefaultValueMetadata["RadioButtons.SelectedIndex"] = @"-1";

            IncludedTypesMetadata["RadioMenuFlyoutItem"] = true;
            // RadioMenuFlyoutItem -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["RadioMenuFlyoutItem.GroupName"] = true;
            PropChangedCallbackMethodNameMetadata["RadioMenuFlyoutItem.GroupName"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RadioMenuFlyoutItem.IsChecked"] = true;
            PropChangedCallbackMethodNameMetadata["RadioMenuFlyoutItem.IsChecked"] = "OnPropertyChanged";
            // RadioMenuFlyoutItem -- DefaultValueMetadata

            IncludedTypesMetadata["RatingControl"] = true;
            // RatingControl -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["RatingControl.Caption"] = true;
            PropChangedCallbackMethodNameMetadata["RatingControl.Caption"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RatingControl.InitialSetValue"] = true;
            PropChangedCallbackMethodNameMetadata["RatingControl.InitialSetValue"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RatingControl.IsClearEnabled"] = true;
            PropChangedCallbackMethodNameMetadata["RatingControl.IsClearEnabled"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RatingControl.IsReadOnly"] = true;
            PropChangedCallbackMethodNameMetadata["RatingControl.IsReadOnly"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RatingControl.ItemInfo"] = true;
            PropChangedCallbackMethodNameMetadata["RatingControl.ItemInfo"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RatingControl.MaxRating"] = true;
            PropChangedCallbackMethodNameMetadata["RatingControl.MaxRating"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RatingControl.PlaceholderValue"] = true;
            PropChangedCallbackMethodNameMetadata["RatingControl.PlaceholderValue"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RatingControl.Value"] = true;
            PropChangedCallbackMethodNameMetadata["RatingControl.Value"] = "OnPropertyChanged";
            // RatingControl -- DefaultValueMetadata
            DefaultValueMetadata["RatingControl.InitialSetValue"] = @"1";
            DefaultValueMetadata["RatingControl.IsClearEnabled"] = @"true";
            DefaultValueMetadata["RatingControl.MaxRating"] = @"5";
            DefaultValueMetadata["RatingControl.PlaceholderValue"] = @"-1";
            DefaultValueMetadata["RatingControl.Value"] = @"-1";

            IncludedTypesMetadata["RatingItemFontInfo"] = true;
            // RatingItemFontInfo -- NeedsPropChangedCallbackMetadata
            // RatingItemFontInfo -- DefaultValueMetadata

            IncludedTypesMetadata["RatingItemImageInfo"] = true;
            // RatingItemImageInfo -- NeedsPropChangedCallbackMetadata
            // RatingItemImageInfo -- DefaultValueMetadata

            IncludedTypesMetadata["RecyclePool"] = true;
            // RecyclePool -- NeedsPropChangedCallbackMetadata
            // RecyclePool -- DefaultValueMetadata

            IncludedTypesMetadata["RecyclingElementFactory"] = true;
            // RecyclingElementFactory -- NeedsPropChangedCallbackMetadata
            // RecyclingElementFactory -- DefaultValueMetadata

            IncludedTypesMetadata["RefreshContainer"] = true;
            // RefreshContainer -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["RefreshContainer.PullDirection"] = true;
            PropChangedCallbackMethodNameMetadata["RefreshContainer.PullDirection"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RefreshContainer.Visualizer"] = true;
            PropChangedCallbackMethodNameMetadata["RefreshContainer.Visualizer"] = "OnPropertyChanged";
            // RefreshContainer -- DefaultValueMetadata
            DefaultValueMetadata["RefreshContainer.PullDirection"] = @"winrt::RefreshPullDirection::TopToBottom";

            IncludedTypesMetadata["RefreshVisualizer"] = true;
            // RefreshVisualizer -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["RefreshVisualizer.Content"] = true;
            PropChangedCallbackMethodNameMetadata["RefreshVisualizer.Content"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RefreshVisualizer.InfoProvider"] = true;
            PropChangedCallbackMethodNameMetadata["RefreshVisualizer.InfoProvider"] = "OnPropertyChanged";
            PropertyTypeOverrideMetadata["RefreshVisualizer.InfoProvider"] = "winrt::IInspectable";
            NeedsPropChangedCallbackMetadata["RefreshVisualizer.Orientation"] = true;
            PropChangedCallbackMethodNameMetadata["RefreshVisualizer.Orientation"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RefreshVisualizer.State"] = true;
            PropChangedCallbackMethodNameMetadata["RefreshVisualizer.State"] = "OnPropertyChanged";
            // RefreshVisualizer -- DefaultValueMetadata
            DefaultValueMetadata["RefreshVisualizer.Orientation"] = @"winrt::RefreshVisualizerOrientation::Auto";
            DefaultValueMetadata["RefreshVisualizer.State"] = @"winrt::RefreshVisualizerState::Idle";

            IncludedTypesMetadata["RepeaterTestHooks"] = true;
            // RepeaterTestHooks -- NeedsPropChangedCallbackMetadata
            // RepeaterTestHooks -- DefaultValueMetadata

            IncludedTypesMetadata["RevealBrush"] = true;
            // RevealBrush -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["RevealBrush.AlwaysUseFallback"] = true;
            PropChangedCallbackMethodNameMetadata["RevealBrush.AlwaysUseFallback"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RevealBrush.Color"] = true;
            PropChangedCallbackMethodNameMetadata["RevealBrush.Color"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["RevealBrush.State"] = true;
            PropChangedCallbackMethodNameMetadata["RevealBrush.State"] = "OnStatePropertyChanged";
            NeedsPropChangedCallbackMetadata["RevealBrush.TargetTheme"] = true;
            PropChangedCallbackMethodNameMetadata["RevealBrush.TargetTheme"] = "OnPropertyChanged";
            // RevealBrush -- DefaultValueMetadata
            DefaultValueMetadata["RevealBrush.Color"] = @"RevealBrush::sc_defaultColor";
            DefaultValueMetadata["RevealBrush.State"] = @"winrt::RevealBrushState::Normal";
            DefaultValueMetadata["RevealBrush.TargetTheme"] = @"winrt::ApplicationTheme::Light";

            IncludedTypesMetadata["Scroller"] = true;
            // Scroller -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["Scroller.Background"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.Background"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["Scroller.Content"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.Content"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["Scroller.ContentOrientation"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.ContentOrientation"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["Scroller.HorizontalAnchorRatio"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.HorizontalAnchorRatio"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["Scroller.HorizontalAnchorRatio"] = "ValidateAnchorRatio";
            NeedsPropChangedCallbackMetadata["Scroller.HorizontalScrollChainingMode"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.HorizontalScrollChainingMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["Scroller.HorizontalScrollMode"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.HorizontalScrollMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["Scroller.HorizontalScrollRailingMode"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.HorizontalScrollRailingMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["Scroller.IgnoredInputKind"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.IgnoredInputKind"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["Scroller.MaxZoomFactor"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.MaxZoomFactor"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["Scroller.MaxZoomFactor"] = "ValidateZoomFactoryBoundary";
            NeedsPropChangedCallbackMetadata["Scroller.MinZoomFactor"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.MinZoomFactor"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["Scroller.MinZoomFactor"] = "ValidateZoomFactoryBoundary";
            NeedsPropChangedCallbackMetadata["Scroller.VerticalAnchorRatio"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.VerticalAnchorRatio"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["Scroller.VerticalAnchorRatio"] = "ValidateAnchorRatio";
            NeedsPropChangedCallbackMetadata["Scroller.VerticalScrollChainingMode"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.VerticalScrollChainingMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["Scroller.VerticalScrollMode"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.VerticalScrollMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["Scroller.VerticalScrollRailingMode"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.VerticalScrollRailingMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["Scroller.ZoomChainingMode"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.ZoomChainingMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["Scroller.ZoomMode"] = true;
            PropChangedCallbackMethodNameMetadata["Scroller.ZoomMode"] = "OnPropertyChanged";
            // Scroller -- DefaultValueMetadata
            DefaultValueMetadata["Scroller.ContentOrientation"] = @"Scroller::s_defaultContentOrientation";
            DefaultValueMetadata["Scroller.HorizontalAnchorRatio"] = @"Scroller::s_defaultAnchorRatio";
            DefaultValueMetadata["Scroller.HorizontalScrollChainingMode"] = @"Scroller::s_defaultHorizontalScrollChainingMode";
            DefaultValueMetadata["Scroller.HorizontalScrollMode"] = @"Scroller::s_defaultHorizontalScrollMode";
            DefaultValueMetadata["Scroller.HorizontalScrollRailingMode"] = @"Scroller::s_defaultHorizontalScrollRailingMode";
            DefaultValueMetadata["Scroller.IgnoredInputKind"] = @"Scroller::s_defaultIgnoredInputKind";
            DefaultValueMetadata["Scroller.MaxZoomFactor"] = @"Scroller::s_defaultMaxZoomFactor";
            DefaultValueMetadata["Scroller.MinZoomFactor"] = @"Scroller::s_defaultMinZoomFactor";
            DefaultValueMetadata["Scroller.VerticalAnchorRatio"] = @"Scroller::s_defaultAnchorRatio";
            DefaultValueMetadata["Scroller.VerticalScrollChainingMode"] = @"Scroller::s_defaultVerticalScrollChainingMode";
            DefaultValueMetadata["Scroller.VerticalScrollMode"] = @"Scroller::s_defaultVerticalScrollMode";
            DefaultValueMetadata["Scroller.VerticalScrollRailingMode"] = @"Scroller::s_defaultVerticalScrollRailingMode";
            DefaultValueMetadata["Scroller.ZoomChainingMode"] = @"Scroller::s_defaultZoomChainingMode";
            DefaultValueMetadata["Scroller.ZoomMode"] = @"Scroller::s_defaultZoomMode";

            IncludedTypesMetadata["ScrollerTestHooks"] = true;
            // ScrollerTestHooks -- NeedsPropChangedCallbackMetadata
            // ScrollerTestHooks -- DefaultValueMetadata

            IncludedTypesMetadata["ScrollViewer"] = true;
            // ScrollViewer -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["ScrollViewer.ComputedHorizontalScrollBarVisibility"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.ComputedHorizontalScrollBarVisibility"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.ComputedVerticalScrollBarVisibility"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.ComputedVerticalScrollBarVisibility"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.Content"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.Content"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.ContentOrientation"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.ContentOrientation"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.HorizontalAnchorRatio"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.HorizontalAnchorRatio"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["ScrollViewer.HorizontalAnchorRatio"] = "ValidateAnchorRatio";
            NeedsPropChangedCallbackMetadata["ScrollViewer.HorizontalScrollBarVisibility"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.HorizontalScrollBarVisibility"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.HorizontalScrollChainingMode"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.HorizontalScrollChainingMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.HorizontalScrollController"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.HorizontalScrollController"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.HorizontalScrollMode"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.HorizontalScrollMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.HorizontalScrollRailingMode"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.HorizontalScrollRailingMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.IgnoredInputKind"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.IgnoredInputKind"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.MaxZoomFactor"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.MaxZoomFactor"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["ScrollViewer.MaxZoomFactor"] = "ValidateZoomFactoryBoundary";
            NeedsPropChangedCallbackMetadata["ScrollViewer.MinZoomFactor"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.MinZoomFactor"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["ScrollViewer.MinZoomFactor"] = "ValidateZoomFactoryBoundary";
            NeedsPropChangedCallbackMetadata["ScrollViewer.Scroller"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.Scroller"] = "OnPropertyChanged";
            PropertyTypeOverrideMetadata["ScrollViewer.Scroller"] = "winrt::Scroller";
            NeedsPropChangedCallbackMetadata["ScrollViewer.VerticalAnchorRatio"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.VerticalAnchorRatio"] = "OnPropertyChanged";
            PropValidationCallbackMetadata["ScrollViewer.VerticalAnchorRatio"] = "ValidateAnchorRatio";
            NeedsPropChangedCallbackMetadata["ScrollViewer.VerticalScrollBarVisibility"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.VerticalScrollBarVisibility"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.VerticalScrollChainingMode"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.VerticalScrollChainingMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.VerticalScrollController"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.VerticalScrollController"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.VerticalScrollMode"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.VerticalScrollMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.VerticalScrollRailingMode"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.VerticalScrollRailingMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.ZoomChainingMode"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.ZoomChainingMode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["ScrollViewer.ZoomMode"] = true;
            PropChangedCallbackMethodNameMetadata["ScrollViewer.ZoomMode"] = "OnPropertyChanged";
            // ScrollViewer -- DefaultValueMetadata
            DefaultValueMetadata["ScrollViewer.ComputedHorizontalScrollBarVisibility"] = @"ScrollViewer::s_defaultComputedHorizontalScrollBarVisibility";
            DefaultValueMetadata["ScrollViewer.ComputedVerticalScrollBarVisibility"] = @"ScrollViewer::s_defaultComputedVerticalScrollBarVisibility";
            DefaultValueMetadata["ScrollViewer.ContentOrientation"] = @"ScrollViewer::s_defaultContentOrientation";
            DefaultValueMetadata["ScrollViewer.HorizontalAnchorRatio"] = @"ScrollViewer::s_defaultAnchorRatio";
            DefaultValueMetadata["ScrollViewer.HorizontalScrollBarVisibility"] = @"ScrollViewer::s_defaultHorizontalScrollBarVisibility";
            DefaultValueMetadata["ScrollViewer.HorizontalScrollChainingMode"] = @"ScrollViewer::s_defaultHorizontalScrollChainingMode";
            DefaultValueMetadata["ScrollViewer.HorizontalScrollMode"] = @"ScrollViewer::s_defaultHorizontalScrollMode";
            DefaultValueMetadata["ScrollViewer.HorizontalScrollRailingMode"] = @"ScrollViewer::s_defaultHorizontalScrollRailingMode";
            DefaultValueMetadata["ScrollViewer.IgnoredInputKind"] = @"ScrollViewer::s_defaultIgnoredInputKind";
            DefaultValueMetadata["ScrollViewer.MaxZoomFactor"] = @"ScrollViewer::s_defaultMaxZoomFactor";
            DefaultValueMetadata["ScrollViewer.MinZoomFactor"] = @"ScrollViewer::s_defaultMinZoomFactor";
            DefaultValueMetadata["ScrollViewer.VerticalAnchorRatio"] = @"ScrollViewer::s_defaultAnchorRatio";
            DefaultValueMetadata["ScrollViewer.VerticalScrollBarVisibility"] = @"ScrollViewer::s_defaultVerticalScrollBarVisibility";
            DefaultValueMetadata["ScrollViewer.VerticalScrollChainingMode"] = @"ScrollViewer::s_defaultVerticalScrollChainingMode";
            DefaultValueMetadata["ScrollViewer.VerticalScrollMode"] = @"ScrollViewer::s_defaultVerticalScrollMode";
            DefaultValueMetadata["ScrollViewer.VerticalScrollRailingMode"] = @"ScrollViewer::s_defaultVerticalScrollRailingMode";
            DefaultValueMetadata["ScrollViewer.ZoomChainingMode"] = @"ScrollViewer::s_defaultZoomChainingMode";
            DefaultValueMetadata["ScrollViewer.ZoomMode"] = @"ScrollViewer::s_defaultZoomMode";

            IncludedTypesMetadata["SelectionModel"] = true;
            // SelectionModel -- NeedsPropChangedCallbackMetadata
            // SelectionModel -- DefaultValueMetadata

            IncludedTypesMetadata["SpectrumBrush"] = true;
            // SpectrumBrush -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["SpectrumBrush.MaxSurface"] = true;
            PropChangedCallbackMethodNameMetadata["SpectrumBrush.MaxSurface"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SpectrumBrush.MaxSurfaceOpacity"] = true;
            PropChangedCallbackMethodNameMetadata["SpectrumBrush.MaxSurfaceOpacity"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SpectrumBrush.MinSurface"] = true;
            PropChangedCallbackMethodNameMetadata["SpectrumBrush.MinSurface"] = "OnPropertyChanged";
            // SpectrumBrush -- DefaultValueMetadata
            DefaultValueMetadata["SpectrumBrush.MaxSurfaceOpacity"] = @"1.0";

            IncludedTypesMetadata["SplitButton"] = true;
            // SplitButton -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["SplitButton.Command"] = true;
            PropChangedCallbackMethodNameMetadata["SplitButton.Command"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SplitButton.CommandParameter"] = true;
            PropChangedCallbackMethodNameMetadata["SplitButton.CommandParameter"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SplitButton.Flyout"] = true;
            PropChangedCallbackMethodNameMetadata["SplitButton.Flyout"] = "OnPropertyChanged";
            // SplitButton -- DefaultValueMetadata

            IncludedTypesMetadata["StackLayout"] = true;
            // StackLayout -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["StackLayout.Orientation"] = true;
            PropChangedCallbackMethodNameMetadata["StackLayout.Orientation"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["StackLayout.Spacing"] = true;
            PropChangedCallbackMethodNameMetadata["StackLayout.Spacing"] = "OnPropertyChanged";
            // StackLayout -- DefaultValueMetadata
            DefaultValueMetadata["StackLayout.Orientation"] = @"winrt::Orientation::Vertical";
            DefaultValueMetadata["StackLayout.Spacing"] = @"0.0";

            IncludedTypesMetadata["SwipeControl"] = true;
            // SwipeControl -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["SwipeControl.BottomItems"] = true;
            PropChangedCallbackMethodNameMetadata["SwipeControl.BottomItems"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SwipeControl.LeftItems"] = true;
            PropChangedCallbackMethodNameMetadata["SwipeControl.LeftItems"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SwipeControl.RightItems"] = true;
            PropChangedCallbackMethodNameMetadata["SwipeControl.RightItems"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SwipeControl.TopItems"] = true;
            PropChangedCallbackMethodNameMetadata["SwipeControl.TopItems"] = "OnPropertyChanged";
            // SwipeControl -- DefaultValueMetadata

            IncludedTypesMetadata["SwipeItem"] = true;
            // SwipeItem -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["SwipeItem.Background"] = true;
            PropChangedCallbackMethodNameMetadata["SwipeItem.Background"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SwipeItem.BehaviorOnInvoked"] = true;
            PropChangedCallbackMethodNameMetadata["SwipeItem.BehaviorOnInvoked"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SwipeItem.Command"] = true;
            PropChangedCallbackMethodNameMetadata["SwipeItem.Command"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SwipeItem.CommandParameter"] = true;
            PropChangedCallbackMethodNameMetadata["SwipeItem.CommandParameter"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SwipeItem.Foreground"] = true;
            PropChangedCallbackMethodNameMetadata["SwipeItem.Foreground"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SwipeItem.IconSource"] = true;
            PropChangedCallbackMethodNameMetadata["SwipeItem.IconSource"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["SwipeItem.Text"] = true;
            PropChangedCallbackMethodNameMetadata["SwipeItem.Text"] = "OnPropertyChanged";
            // SwipeItem -- DefaultValueMetadata
            DefaultValueMetadata["SwipeItem.BehaviorOnInvoked"] = @"winrt::SwipeBehaviorOnInvoked::Auto";

            IncludedTypesMetadata["SwipeItems"] = true;
            // SwipeItems -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["SwipeItems.Mode"] = true;
            PropChangedCallbackMethodNameMetadata["SwipeItems.Mode"] = "OnPropertyChanged";
            // SwipeItems -- DefaultValueMetadata
            DefaultValueMetadata["SwipeItems.Mode"] = @"winrt::SwipeMode::Reveal";

            IncludedTypesMetadata["SwipeTestHooks"] = true;
            // SwipeTestHooks -- NeedsPropChangedCallbackMetadata
            // SwipeTestHooks -- DefaultValueMetadata

            IncludedTypesMetadata["SymbolIconSource"] = true;
            // SymbolIconSource -- NeedsPropChangedCallbackMetadata
            // SymbolIconSource -- DefaultValueMetadata
            DefaultValueMetadata["SymbolIconSource.Symbol"] = @"winrt::Symbol::Emoji";

            IncludedTypesMetadata["TeachingTip"] = true;
            // TeachingTip -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["TeachingTip.ActionButtonCommand"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.ActionButtonCommand"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.ActionButtonCommandParameter"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.ActionButtonCommandParameter"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.ActionButtonContent"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.ActionButtonContent"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.ActionButtonStyle"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.ActionButtonStyle"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.CloseButtonCommand"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.CloseButtonCommand"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.CloseButtonCommandParameter"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.CloseButtonCommandParameter"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.CloseButtonContent"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.CloseButtonContent"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.CloseButtonStyle"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.CloseButtonStyle"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.HeroContent"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.HeroContent"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.HeroContentPlacement"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.HeroContentPlacement"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.IconSource"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.IconSource"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.IsLightDismissEnabled"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.IsLightDismissEnabled"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.IsOpen"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.IsOpen"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.PlacementMargin"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.PlacementMargin"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.PreferredPlacement"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.PreferredPlacement"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.Subtitle"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.Subtitle"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.TailVisibility"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.TailVisibility"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.Target"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.Target"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.TemplateSettings"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.TemplateSettings"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TeachingTip.Title"] = true;
            PropChangedCallbackMethodNameMetadata["TeachingTip.Title"] = "OnPropertyChanged";
            // TeachingTip -- DefaultValueMetadata
            DefaultValueMetadata["TeachingTip.HeroContentPlacement"] = @"winrt::TeachingTipHeroContentPlacementMode::Auto";
            DefaultValueMetadata["TeachingTip.IsLightDismissEnabled"] = @"false";
            DefaultValueMetadata["TeachingTip.IsOpen"] = @"false";
            DefaultValueMetadata["TeachingTip.PreferredPlacement"] = @"winrt::TeachingTipPlacementMode::Auto";
            DefaultValueMetadata["TeachingTip.TailVisibility"] = @"winrt::TeachingTipTailVisibility::Auto";

            IncludedTypesMetadata["TeachingTipTemplateSettings"] = true;
            // TeachingTipTemplateSettings -- NeedsPropChangedCallbackMetadata
            // TeachingTipTemplateSettings -- DefaultValueMetadata

            IncludedTypesMetadata["TeachingTipTestHooks"] = true;
            // TeachingTipTestHooks -- NeedsPropChangedCallbackMetadata
            // TeachingTipTestHooks -- DefaultValueMetadata

            IncludedTypesMetadata["ToggleSplitButton"] = true;
            // ToggleSplitButton -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["ToggleSplitButton.IsChecked"] = true;
            PropChangedCallbackMethodNameMetadata["ToggleSplitButton.IsChecked"] = "OnPropertyChanged";
            NeedsDependencyPropertyFieldMetadata["ToggleSplitButton.IsChecked"] = true;
            // ToggleSplitButton -- DefaultValueMetadata

            IncludedTypesMetadata["TreeView"] = true;
            // TreeView -- NeedsPropChangedCallbackMetadata
            PropChangedCallbackMethodNameMetadata["TreeView.CanDragItems"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TreeView.CanReorderItems"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TreeView.ItemContainerStyle"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TreeView.ItemContainerStyleSelector"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TreeView.ItemContainerTransitions"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TreeView.ItemsSource"] = true;
            PropChangedCallbackMethodNameMetadata["TreeView.ItemsSource"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TreeView.ItemTemplate"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TreeView.ItemTemplateSelector"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TreeView.SelectionMode"] = true;
            PropChangedCallbackMethodNameMetadata["TreeView.SelectionMode"] = "OnPropertyChanged";
            // TreeView -- DefaultValueMetadata
            DefaultValueMetadata["TreeView.CanDragItems"] = @"true";
            DefaultValueMetadata["TreeView.CanReorderItems"] = @"true";
            DefaultValueMetadata["TreeView.SelectionMode"] = @"winrt::TreeViewSelectionMode::Single";

            IncludedTypesMetadata["TreeViewItem"] = true;
            // TreeViewItem -- NeedsPropChangedCallbackMetadata
            PropChangedCallbackMethodNameMetadata["TreeViewItem.CollapsedGlyph"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TreeViewItem.ExpandedGlyph"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TreeViewItem.GlyphBrush"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TreeViewItem.GlyphOpacity"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TreeViewItem.GlyphSize"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TreeViewItem.HasUnrealizedChildren"] = true;
            PropChangedCallbackMethodNameMetadata["TreeViewItem.HasUnrealizedChildren"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TreeViewItem.IsExpanded"] = true;
            PropChangedCallbackMethodNameMetadata["TreeViewItem.IsExpanded"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TreeViewItem.ItemsSource"] = true;
            PropChangedCallbackMethodNameMetadata["TreeViewItem.ItemsSource"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TreeViewItem.TreeViewItemTemplateSettings"] = "OnPropertyChanged";
            // TreeViewItem -- DefaultValueMetadata
            DefaultValueMetadata["TreeViewItem.CollapsedGlyph"] = @"\uE76C";
            DefaultValueMetadata["TreeViewItem.ExpandedGlyph"] = @"\uE70D";
            DefaultValueMetadata["TreeViewItem.GlyphOpacity"] = @"1.0";
            DefaultValueMetadata["TreeViewItem.GlyphSize"] = @"12.0";

            IncludedTypesMetadata["TreeViewItemTemplateSettings"] = true;
            // TreeViewItemTemplateSettings -- NeedsPropChangedCallbackMetadata
            // TreeViewItemTemplateSettings -- DefaultValueMetadata
            DefaultValueMetadata["TreeViewItemTemplateSettings.CollapsedGlyphVisibility"] = @"winrt::Visibility::Collapsed";
            DefaultValueMetadata["TreeViewItemTemplateSettings.ExpandedGlyphVisibility"] = @"winrt::Visibility::Collapsed";

            IncludedTypesMetadata["TreeViewNode"] = true;
            // TreeViewNode -- NeedsPropChangedCallbackMetadata
            PropChangedCallbackMethodNameMetadata["TreeViewNode.Content"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TreeViewNode.Depth"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TreeViewNode.HasChildren"] = true;
            PropChangedCallbackMethodNameMetadata["TreeViewNode.HasChildren"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TreeViewNode.IsExpanded"] = true;
            PropChangedCallbackMethodNameMetadata["TreeViewNode.IsExpanded"] = "OnPropertyChanged";
            // TreeViewNode -- DefaultValueMetadata
            DefaultValueMetadata["TreeViewNode.Depth"] = @"-1";

            IncludedTypesMetadata["TwoPaneView"] = true;
            // TwoPaneView -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["TwoPaneView.MinTallModeHeight"] = true;
            PropChangedCallbackMethodNameMetadata["TwoPaneView.MinTallModeHeight"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TwoPaneView.MinWideModeWidth"] = true;
            PropChangedCallbackMethodNameMetadata["TwoPaneView.MinWideModeWidth"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["TwoPaneView.Mode"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TwoPaneView.Pane1"] = true;
            PropChangedCallbackMethodNameMetadata["TwoPaneView.Pane1"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TwoPaneView.Pane1Length"] = true;
            PropChangedCallbackMethodNameMetadata["TwoPaneView.Pane1Length"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TwoPaneView.Pane2"] = true;
            PropChangedCallbackMethodNameMetadata["TwoPaneView.Pane2"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TwoPaneView.Pane2Length"] = true;
            PropChangedCallbackMethodNameMetadata["TwoPaneView.Pane2Length"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TwoPaneView.PanePriority"] = true;
            PropChangedCallbackMethodNameMetadata["TwoPaneView.PanePriority"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TwoPaneView.TallModeConfiguration"] = true;
            PropChangedCallbackMethodNameMetadata["TwoPaneView.TallModeConfiguration"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["TwoPaneView.WideModeConfiguration"] = true;
            PropChangedCallbackMethodNameMetadata["TwoPaneView.WideModeConfiguration"] = "OnPropertyChanged";
            // TwoPaneView -- DefaultValueMetadata
            DefaultValueMetadata["TwoPaneView.MinTallModeHeight"] = @"c_defaultMinTallModeHeight";
            DefaultValueMetadata["TwoPaneView.MinWideModeWidth"] = @"c_defaultMinWideModeWidth";
            DefaultValueMetadata["TwoPaneView.Mode"] = @"winrt::TwoPaneViewMode::SinglePane";
            DefaultValueMetadata["TwoPaneView.Pane1Length"] = @"c_pane1LengthDefault";
            DefaultValueMetadata["TwoPaneView.Pane2Length"] = @"c_pane2LengthDefault";
            DefaultValueMetadata["TwoPaneView.PanePriority"] = @"winrt::TwoPaneViewPriority::Pane1";
            DefaultValueMetadata["TwoPaneView.TallModeConfiguration"] = @"winrt::TwoPaneViewTallModeConfiguration::TopBottom";
            DefaultValueMetadata["TwoPaneView.WideModeConfiguration"] = @"winrt::TwoPaneViewWideModeConfiguration::LeftRight";

            IncludedTypesMetadata["UniformGridLayout"] = true;
            // UniformGridLayout -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["UniformGridLayout.ItemsJustification"] = true;
            PropChangedCallbackMethodNameMetadata["UniformGridLayout.ItemsJustification"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["UniformGridLayout.ItemsStretch"] = true;
            PropChangedCallbackMethodNameMetadata["UniformGridLayout.ItemsStretch"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["UniformGridLayout.MinColumnSpacing"] = true;
            PropChangedCallbackMethodNameMetadata["UniformGridLayout.MinColumnSpacing"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["UniformGridLayout.MinItemHeight"] = true;
            PropChangedCallbackMethodNameMetadata["UniformGridLayout.MinItemHeight"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["UniformGridLayout.MinItemWidth"] = true;
            PropChangedCallbackMethodNameMetadata["UniformGridLayout.MinItemWidth"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["UniformGridLayout.MinRowSpacing"] = true;
            PropChangedCallbackMethodNameMetadata["UniformGridLayout.MinRowSpacing"] = "OnPropertyChanged";
            NeedsPropChangedCallbackMetadata["UniformGridLayout.Orientation"] = true;
            PropChangedCallbackMethodNameMetadata["UniformGridLayout.Orientation"] = "OnPropertyChanged";
            // UniformGridLayout -- DefaultValueMetadata
            DefaultValueMetadata["UniformGridLayout.ItemsJustification"] = @"winrt::UniformGridLayoutItemsJustification::Start";
            DefaultValueMetadata["UniformGridLayout.ItemsStretch"] = @"winrt::UniformGridLayoutItemsStretch::None";
            DefaultValueMetadata["UniformGridLayout.MinColumnSpacing"] = @"0.0";
            DefaultValueMetadata["UniformGridLayout.MinItemHeight"] = @"0.0";
            DefaultValueMetadata["UniformGridLayout.MinItemWidth"] = @"0.0";
            DefaultValueMetadata["UniformGridLayout.MinRowSpacing"] = @"0.0";
            DefaultValueMetadata["UniformGridLayout.Orientation"] = @"winrt::Orientation::Horizontal";

            IncludedTypesMetadata["XamlAmbientLight"] = true;
            // XamlAmbientLight -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["XamlAmbientLight.Color"] = true;
            PropChangedCallbackMethodNameMetadata["XamlAmbientLight.IsTarget"] = "OnIsTargetPropertyChanged";
            // XamlAmbientLight -- DefaultValueMetadata
            DefaultValueMetadata["XamlAmbientLight.Color"] = @"{ 255, 255, 255, 255 }";


        }
    }
}
