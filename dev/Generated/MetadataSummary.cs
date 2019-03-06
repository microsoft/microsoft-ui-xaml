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
            NeedsPropChangedCallbackMetadata["AcrylicBrush.BackgroundSource"] = true;
            NeedsPropChangedCallbackMetadata["AcrylicBrush.TintColor"] = true;
            NeedsPropChangedCallbackMetadata["AcrylicBrush.TintLuminosityOpacity"] = true;
            PropValidationCallbackMetadata["AcrylicBrush.TintLuminosityOpacity"] = "CoerceToZeroOneRange_Nullable";
            NeedsPropChangedCallbackMetadata["AcrylicBrush.TintOpacity"] = true;
            PropValidationCallbackMetadata["AcrylicBrush.TintOpacity"] = "CoerceToZeroOneRange";
            NeedsPropChangedCallbackMetadata["AcrylicBrush.TintTransitionDuration"] = true;
            // AcrylicBrush -- DefaultValueMetadata
            DefaultValueMetadata["AcrylicBrush.BackgroundSource"] = @"winrt::AcrylicBackgroundSource::Backdrop";
            DefaultValueMetadata["AcrylicBrush.TintColor"] = @"AcrylicBrush::sc_defaultTintColor";
            DefaultValueMetadata["AcrylicBrush.TintOpacity"] = @"AcrylicBrush::sc_defaultTintOpacity";
            DefaultValueMetadata["AcrylicBrush.TintTransitionDuration"] = @"AcrylicBrush::sc_defaultTintTransitionDuration";

            IncludedTypesMetadata["AnimatedVisualPlayer"] = true;
            // AnimatedVisualPlayer -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["AnimatedVisualPlayer.AutoPlay"] = true;
            PropChangedCallbackMethodNameMetadata["AnimatedVisualPlayer.AutoPlay"] = "OnAutoPlayPropertyChanged";
            NeedsPropChangedCallbackMetadata["AnimatedVisualPlayer.FallbackContent"] = true;
            PropChangedCallbackMethodNameMetadata["AnimatedVisualPlayer.FallbackContent"] = "OnFallbackContentPropertyChanged";
            NeedsPropChangedCallbackMetadata["AnimatedVisualPlayer.PlaybackRate"] = true;
            PropChangedCallbackMethodNameMetadata["AnimatedVisualPlayer.PlaybackRate"] = "OnPlaybackRatePropertyChanged";
            NeedsPropChangedCallbackMetadata["AnimatedVisualPlayer.Source"] = true;
            PropChangedCallbackMethodNameMetadata["AnimatedVisualPlayer.Source"] = "OnSourcePropertyChanged";
            NeedsPropChangedCallbackMetadata["AnimatedVisualPlayer.Stretch"] = true;
            PropChangedCallbackMethodNameMetadata["AnimatedVisualPlayer.Stretch"] = "OnStretchPropertyChanged";
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
            NeedsPropChangedCallbackMetadata["ColorPicker.ColorSpectrumComponents"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.ColorSpectrumShape"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.IsAlphaEnabled"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.IsAlphaSliderVisible"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.IsAlphaTextInputVisible"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.IsColorChannelTextInputVisible"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.IsColorPreviewVisible"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.IsColorSliderVisible"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.IsColorSpectrumVisible"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.IsHexInputVisible"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.IsMoreButtonVisible"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.MaxHue"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.MaxSaturation"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.MaxValue"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.MinHue"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.MinSaturation"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.MinValue"] = true;
            NeedsPropChangedCallbackMetadata["ColorPicker.PreviousColor"] = true;
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
            NeedsPropChangedCallbackMetadata["ColorSpectrum.Components"] = true;
            NeedsPropChangedCallbackMetadata["ColorSpectrum.HsvColor"] = true;
            NeedsPropChangedCallbackMetadata["ColorSpectrum.MaxHue"] = true;
            NeedsPropChangedCallbackMetadata["ColorSpectrum.MaxSaturation"] = true;
            NeedsPropChangedCallbackMetadata["ColorSpectrum.MaxValue"] = true;
            NeedsPropChangedCallbackMetadata["ColorSpectrum.MinHue"] = true;
            NeedsPropChangedCallbackMetadata["ColorSpectrum.MinSaturation"] = true;
            NeedsPropChangedCallbackMetadata["ColorSpectrum.MinValue"] = true;
            NeedsPropChangedCallbackMetadata["ColorSpectrum.Shape"] = true;
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
            // FlowLayout -- DefaultValueMetadata

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
            // ItemsRepeater -- DefaultValueMetadata

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
            NeedsPropChangedCallbackMetadata["NavigationView.AutoSuggestBox"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.CompactModeThresholdWidth"] = true;
            PropValidationCallbackMetadata["NavigationView.CompactModeThresholdWidth"] = "CoerceToGreaterThanZero";
            NeedsPropChangedCallbackMetadata["NavigationView.CompactPaneLength"] = true;
            PropValidationCallbackMetadata["NavigationView.CompactPaneLength"] = "CoerceToGreaterThanZero";
            NeedsPropChangedCallbackMetadata["NavigationView.DisplayMode"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.ExpandedModeThresholdWidth"] = true;
            PropValidationCallbackMetadata["NavigationView.ExpandedModeThresholdWidth"] = "CoerceToGreaterThanZero";
            NeedsPropChangedCallbackMetadata["NavigationView.Header"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.HeaderTemplate"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.IsBackButtonVisible"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.IsBackEnabled"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.IsPaneOpen"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.IsPaneToggleButtonVisible"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.IsPaneVisible"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.IsSettingsVisible"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.MenuItemContainerStyle"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.MenuItemContainerStyleSelector"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.MenuItems"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.MenuItemsSource"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.MenuItemTemplate"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.MenuItemTemplateSelector"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.OpenPaneLength"] = true;
            PropValidationCallbackMetadata["NavigationView.OpenPaneLength"] = "CoerceToGreaterThanZero";
            NeedsPropChangedCallbackMetadata["NavigationView.OverflowLabelMode"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.PaneDisplayMode"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.PaneFooter"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.PaneTitle"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.PaneToggleButtonStyle"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.SelectedItem"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.SelectionFollowsFocus"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.SettingsItem"] = true;
            NeedsPropChangedCallbackMetadata["NavigationView.ShoulderNavigationEnabled"] = true;
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
            NeedsPropChangedCallbackMetadata["ParallaxView.HorizontalShift"] = true;
            NeedsPropChangedCallbackMetadata["ParallaxView.HorizontalSourceEndOffset"] = true;
            NeedsPropChangedCallbackMetadata["ParallaxView.HorizontalSourceOffsetKind"] = true;
            NeedsPropChangedCallbackMetadata["ParallaxView.HorizontalSourceStartOffset"] = true;
            NeedsPropChangedCallbackMetadata["ParallaxView.IsHorizontalShiftClamped"] = true;
            NeedsPropChangedCallbackMetadata["ParallaxView.IsVerticalShiftClamped"] = true;
            NeedsPropChangedCallbackMetadata["ParallaxView.MaxHorizontalShiftRatio"] = true;
            NeedsPropChangedCallbackMetadata["ParallaxView.MaxVerticalShiftRatio"] = true;
            NeedsPropChangedCallbackMetadata["ParallaxView.Source"] = true;
            NeedsPropChangedCallbackMetadata["ParallaxView.VerticalShift"] = true;
            NeedsPropChangedCallbackMetadata["ParallaxView.VerticalSourceEndOffset"] = true;
            NeedsPropChangedCallbackMetadata["ParallaxView.VerticalSourceOffsetKind"] = true;
            NeedsPropChangedCallbackMetadata["ParallaxView.VerticalSourceStartOffset"] = true;
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
            NeedsPropChangedCallbackMetadata["PersonPicture.BadgeImageSource"] = true;
            NeedsPropChangedCallbackMetadata["PersonPicture.BadgeNumber"] = true;
            NeedsPropChangedCallbackMetadata["PersonPicture.BadgeText"] = true;
            NeedsPropChangedCallbackMetadata["PersonPicture.Contact"] = true;
            NeedsPropChangedCallbackMetadata["PersonPicture.DisplayName"] = true;
            NeedsPropChangedCallbackMetadata["PersonPicture.Initials"] = true;
            NeedsPropChangedCallbackMetadata["PersonPicture.IsGroup"] = true;
            NeedsPropChangedCallbackMetadata["PersonPicture.PreferSmallImage"] = true;
            NeedsPropChangedCallbackMetadata["PersonPicture.ProfilePicture"] = true;
            // PersonPicture -- DefaultValueMetadata

            IncludedTypesMetadata["RadialGradientBrush"] = true;
            // RadialGradientBrush -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["RadialGradientBrush.Placeholder"] = true;
            // RadialGradientBrush -- DefaultValueMetadata

            IncludedTypesMetadata["RadioButtons"] = true;
            // RadioButtons -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["RadioButtons.Header"] = true;
            NeedsPropChangedCallbackMetadata["RadioButtons.Items"] = true;
            NeedsPropChangedCallbackMetadata["RadioButtons.ItemsSource"] = true;
            NeedsPropChangedCallbackMetadata["RadioButtons.ItemTemplate"] = true;
            NeedsPropChangedCallbackMetadata["RadioButtons.MaximumColumns"] = true;
            NeedsPropChangedCallbackMetadata["RadioButtons.SelectedIndex"] = true;
            NeedsPropChangedCallbackMetadata["RadioButtons.SelectedItem"] = true;
            // RadioButtons -- DefaultValueMetadata
            DefaultValueMetadata["RadioButtons.MaximumColumns"] = @"1";
            DefaultValueMetadata["RadioButtons.SelectedIndex"] = @"-1";

            IncludedTypesMetadata["RadioMenuFlyoutItem"] = true;
            // RadioMenuFlyoutItem -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["RadioMenuFlyoutItem.GroupName"] = true;
            NeedsPropChangedCallbackMetadata["RadioMenuFlyoutItem.IsChecked"] = true;
            // RadioMenuFlyoutItem -- DefaultValueMetadata

            IncludedTypesMetadata["RatingControl"] = true;
            // RatingControl -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["RatingControl.Caption"] = true;
            NeedsPropChangedCallbackMetadata["RatingControl.InitialSetValue"] = true;
            NeedsPropChangedCallbackMetadata["RatingControl.IsClearEnabled"] = true;
            NeedsPropChangedCallbackMetadata["RatingControl.IsReadOnly"] = true;
            NeedsPropChangedCallbackMetadata["RatingControl.ItemInfo"] = true;
            NeedsPropChangedCallbackMetadata["RatingControl.MaxRating"] = true;
            NeedsPropChangedCallbackMetadata["RatingControl.PlaceholderValue"] = true;
            NeedsPropChangedCallbackMetadata["RatingControl.Value"] = true;
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
            NeedsPropChangedCallbackMetadata["RefreshContainer.Visualizer"] = true;
            // RefreshContainer -- DefaultValueMetadata
            DefaultValueMetadata["RefreshContainer.PullDirection"] = @"winrt::RefreshPullDirection::TopToBottom";

            IncludedTypesMetadata["RefreshVisualizer"] = true;
            // RefreshVisualizer -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["RefreshVisualizer.Content"] = true;
            NeedsPropChangedCallbackMetadata["RefreshVisualizer.InfoProvider"] = true;
            PropertyTypeOverrideMetadata["RefreshVisualizer.InfoProvider"] = "winrt::IInspectable";
            NeedsPropChangedCallbackMetadata["RefreshVisualizer.Orientation"] = true;
            NeedsPropChangedCallbackMetadata["RefreshVisualizer.State"] = true;
            // RefreshVisualizer -- DefaultValueMetadata
            DefaultValueMetadata["RefreshVisualizer.Orientation"] = @"winrt::RefreshVisualizerOrientation::Auto";
            DefaultValueMetadata["RefreshVisualizer.State"] = @"winrt::RefreshVisualizerState::Idle";

            IncludedTypesMetadata["RepeaterTestHooks"] = true;
            // RepeaterTestHooks -- NeedsPropChangedCallbackMetadata
            // RepeaterTestHooks -- DefaultValueMetadata

            IncludedTypesMetadata["RevealBrush"] = true;
            // RevealBrush -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["RevealBrush.AlwaysUseFallback"] = true;
            NeedsPropChangedCallbackMetadata["RevealBrush.Color"] = true;
            NeedsPropChangedCallbackMetadata["RevealBrush.State"] = true;
            PropChangedCallbackMethodNameMetadata["RevealBrush.State"] = "OnStatePropertyChanged";
            NeedsPropChangedCallbackMetadata["RevealBrush.TargetTheme"] = true;
            // RevealBrush -- DefaultValueMetadata
            DefaultValueMetadata["RevealBrush.Color"] = @"RevealBrush::sc_defaultColor";
            DefaultValueMetadata["RevealBrush.State"] = @"winrt::RevealBrushState::Normal";
            DefaultValueMetadata["RevealBrush.TargetTheme"] = @"winrt::ApplicationTheme::Light";

            IncludedTypesMetadata["Scroller"] = true;
            // Scroller -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["Scroller.Background"] = true;
            NeedsPropChangedCallbackMetadata["Scroller.Content"] = true;
            NeedsPropChangedCallbackMetadata["Scroller.ContentOrientation"] = true;
            NeedsPropChangedCallbackMetadata["Scroller.HorizontalAnchorRatio"] = true;
            PropValidationCallbackMetadata["Scroller.HorizontalAnchorRatio"] = "ValidateAnchorRatio";
            NeedsPropChangedCallbackMetadata["Scroller.HorizontalScrollChainingMode"] = true;
            NeedsPropChangedCallbackMetadata["Scroller.HorizontalScrollMode"] = true;
            NeedsPropChangedCallbackMetadata["Scroller.HorizontalScrollRailingMode"] = true;
            NeedsPropChangedCallbackMetadata["Scroller.IgnoredInputKind"] = true;
            NeedsPropChangedCallbackMetadata["Scroller.MaxZoomFactor"] = true;
            PropValidationCallbackMetadata["Scroller.MaxZoomFactor"] = "ValidateZoomFactoryBoundary";
            NeedsPropChangedCallbackMetadata["Scroller.MinZoomFactor"] = true;
            PropValidationCallbackMetadata["Scroller.MinZoomFactor"] = "ValidateZoomFactoryBoundary";
            NeedsPropChangedCallbackMetadata["Scroller.VerticalAnchorRatio"] = true;
            PropValidationCallbackMetadata["Scroller.VerticalAnchorRatio"] = "ValidateAnchorRatio";
            NeedsPropChangedCallbackMetadata["Scroller.VerticalScrollChainingMode"] = true;
            NeedsPropChangedCallbackMetadata["Scroller.VerticalScrollMode"] = true;
            NeedsPropChangedCallbackMetadata["Scroller.VerticalScrollRailingMode"] = true;
            NeedsPropChangedCallbackMetadata["Scroller.ZoomChainingMode"] = true;
            NeedsPropChangedCallbackMetadata["Scroller.ZoomMode"] = true;
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
            NeedsPropChangedCallbackMetadata["ScrollViewer.ComputedVerticalScrollBarVisibility"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.Content"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.ContentOrientation"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.HorizontalAnchorRatio"] = true;
            PropValidationCallbackMetadata["ScrollViewer.HorizontalAnchorRatio"] = "ValidateAnchorRatio";
            NeedsPropChangedCallbackMetadata["ScrollViewer.HorizontalScrollBarVisibility"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.HorizontalScrollChainingMode"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.HorizontalScrollController"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.HorizontalScrollMode"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.HorizontalScrollRailingMode"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.IgnoredInputKind"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.MaxZoomFactor"] = true;
            PropValidationCallbackMetadata["ScrollViewer.MaxZoomFactor"] = "ValidateZoomFactoryBoundary";
            NeedsPropChangedCallbackMetadata["ScrollViewer.MinZoomFactor"] = true;
            PropValidationCallbackMetadata["ScrollViewer.MinZoomFactor"] = "ValidateZoomFactoryBoundary";
            NeedsPropChangedCallbackMetadata["ScrollViewer.Scroller"] = true;
            PropertyTypeOverrideMetadata["ScrollViewer.Scroller"] = "winrt::Scroller";
            NeedsPropChangedCallbackMetadata["ScrollViewer.VerticalAnchorRatio"] = true;
            PropValidationCallbackMetadata["ScrollViewer.VerticalAnchorRatio"] = "ValidateAnchorRatio";
            NeedsPropChangedCallbackMetadata["ScrollViewer.VerticalScrollBarVisibility"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.VerticalScrollChainingMode"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.VerticalScrollController"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.VerticalScrollMode"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.VerticalScrollRailingMode"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.ZoomChainingMode"] = true;
            NeedsPropChangedCallbackMetadata["ScrollViewer.ZoomMode"] = true;
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
            NeedsPropChangedCallbackMetadata["SpectrumBrush.MaxSurfaceOpacity"] = true;
            NeedsPropChangedCallbackMetadata["SpectrumBrush.MinSurface"] = true;
            // SpectrumBrush -- DefaultValueMetadata
            DefaultValueMetadata["SpectrumBrush.MaxSurfaceOpacity"] = @"1.0";

            IncludedTypesMetadata["SplitButton"] = true;
            // SplitButton -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["SplitButton.Command"] = true;
            NeedsPropChangedCallbackMetadata["SplitButton.CommandParameter"] = true;
            NeedsPropChangedCallbackMetadata["SplitButton.Flyout"] = true;
            // SplitButton -- DefaultValueMetadata

            IncludedTypesMetadata["StackLayout"] = true;
            // StackLayout -- NeedsPropChangedCallbackMetadata
            // StackLayout -- DefaultValueMetadata

            IncludedTypesMetadata["SwipeControl"] = true;
            // SwipeControl -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["SwipeControl.BottomItems"] = true;
            NeedsPropChangedCallbackMetadata["SwipeControl.LeftItems"] = true;
            NeedsPropChangedCallbackMetadata["SwipeControl.RightItems"] = true;
            NeedsPropChangedCallbackMetadata["SwipeControl.TopItems"] = true;
            // SwipeControl -- DefaultValueMetadata

            IncludedTypesMetadata["SwipeItem"] = true;
            // SwipeItem -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["SwipeItem.Background"] = true;
            NeedsPropChangedCallbackMetadata["SwipeItem.BehaviorOnInvoked"] = true;
            NeedsPropChangedCallbackMetadata["SwipeItem.Command"] = true;
            NeedsPropChangedCallbackMetadata["SwipeItem.CommandParameter"] = true;
            NeedsPropChangedCallbackMetadata["SwipeItem.Foreground"] = true;
            NeedsPropChangedCallbackMetadata["SwipeItem.IconSource"] = true;
            NeedsPropChangedCallbackMetadata["SwipeItem.Text"] = true;
            // SwipeItem -- DefaultValueMetadata
            DefaultValueMetadata["SwipeItem.BehaviorOnInvoked"] = @"winrt::SwipeBehaviorOnInvoked::Auto";

            IncludedTypesMetadata["SwipeItems"] = true;
            // SwipeItems -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["SwipeItems.Mode"] = true;
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
            NeedsPropChangedCallbackMetadata["TeachingTip.ActionButtonCommandParameter"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.ActionButtonStyle"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.ActionButtonText"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.Attach"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.BleedingImageContent"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.BleedingImagePlacement"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.CloseButtonCommand"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.CloseButtonCommandParameter"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.CloseButtonKind"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.CloseButtonStyle"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.CloseButtonText"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.IconSource"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.IsLightDismissEnabled"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.IsOpen"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.Placement"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.Subtext"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.TargetOffset"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.TemplateSettings"] = true;
            NeedsPropChangedCallbackMetadata["TeachingTip.Title"] = true;
            // TeachingTip -- DefaultValueMetadata
            DefaultValueMetadata["TeachingTip.BleedingImagePlacement"] = @"winrt::TeachingTipBleedingImagePlacementMode::Auto";
            DefaultValueMetadata["TeachingTip.CloseButtonKind"] = @"winrt::TeachingTipCloseButtonKind::Auto";
            DefaultValueMetadata["TeachingTip.IsLightDismissEnabled"] = @"false";
            DefaultValueMetadata["TeachingTip.IsOpen"] = @"false";
            DefaultValueMetadata["TeachingTip.Placement"] = @"winrt::TeachingTipPlacementMode::Auto";

            IncludedTypesMetadata["TeachingTipTemplateSettings"] = true;
            // TeachingTipTemplateSettings -- NeedsPropChangedCallbackMetadata
            // TeachingTipTemplateSettings -- DefaultValueMetadata

            IncludedTypesMetadata["TeachingTipTestHooks"] = true;
            // TeachingTipTestHooks -- NeedsPropChangedCallbackMetadata
            // TeachingTipTestHooks -- DefaultValueMetadata

            IncludedTypesMetadata["ToggleSplitButton"] = true;
            // ToggleSplitButton -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["ToggleSplitButton.IsChecked"] = true;
            NeedsDependencyPropertyFieldMetadata["ToggleSplitButton.IsChecked"] = true;
            // ToggleSplitButton -- DefaultValueMetadata

            IncludedTypesMetadata["TreeView"] = true;
            // TreeView -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["TreeView.ItemsSource"] = true;
            NeedsPropChangedCallbackMetadata["TreeView.SelectionMode"] = true;
            // TreeView -- DefaultValueMetadata
            DefaultValueMetadata["TreeView.CanDragItems"] = @"true";
            DefaultValueMetadata["TreeView.CanReorderItems"] = @"true";
            DefaultValueMetadata["TreeView.SelectionMode"] = @"winrt::TreeViewSelectionMode::Single";

            IncludedTypesMetadata["TreeViewItem"] = true;
            // TreeViewItem -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["TreeViewItem.HasUnrealizedChildren"] = true;
            NeedsPropChangedCallbackMetadata["TreeViewItem.IsExpanded"] = true;
            NeedsPropChangedCallbackMetadata["TreeViewItem.ItemsSource"] = true;
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
            NeedsPropChangedCallbackMetadata["TreeViewNode.HasChildren"] = true;
            NeedsPropChangedCallbackMetadata["TreeViewNode.IsExpanded"] = true;
            // TreeViewNode -- DefaultValueMetadata
            DefaultValueMetadata["TreeViewNode.Depth"] = @"-1";

            IncludedTypesMetadata["TwoPaneView"] = true;
            // TwoPaneView -- NeedsPropChangedCallbackMetadata
            NeedsPropChangedCallbackMetadata["TwoPaneView.MinTallModeHeight"] = true;
            NeedsPropChangedCallbackMetadata["TwoPaneView.MinWideModeWidth"] = true;
            NeedsPropChangedCallbackMetadata["TwoPaneView.Pane1"] = true;
            NeedsPropChangedCallbackMetadata["TwoPaneView.Pane1Length"] = true;
            NeedsPropChangedCallbackMetadata["TwoPaneView.Pane2"] = true;
            NeedsPropChangedCallbackMetadata["TwoPaneView.Pane2Length"] = true;
            NeedsPropChangedCallbackMetadata["TwoPaneView.PanePriority"] = true;
            NeedsPropChangedCallbackMetadata["TwoPaneView.TallModeConfiguration"] = true;
            NeedsPropChangedCallbackMetadata["TwoPaneView.WideModeConfiguration"] = true;
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
            // UniformGridLayout -- DefaultValueMetadata

            IncludedTypesMetadata["XamlAmbientLight"] = true;
            // XamlAmbientLight -- NeedsPropChangedCallbackMetadata
            PropChangedCallbackMethodNameMetadata["XamlAmbientLight.Color"] = "OnPropertyChanged";
            PropChangedCallbackMethodNameMetadata["XamlAmbientLight.IsTarget"] = "OnPropertyChanged";
            // XamlAmbientLight -- DefaultValueMetadata
            DefaultValueMetadata["XamlAmbientLight.Color"] = @"{ 255, 255, 255, 255 }";


        }
    }
}
