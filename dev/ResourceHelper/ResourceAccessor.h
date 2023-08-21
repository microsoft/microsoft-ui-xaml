﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

/// <summary>
/// Resource Accessor
/// </summary>
class ResourceAccessor sealed
{
private:
    /// <summary>
    /// Declare a private constructor to prevent instance creation.
    /// </summary>
    ResourceAccessor() = delete;

    /// <summary>
    /// String containing the resource location
    /// </summary>
    static PCWSTR c_resourceLoc;
    static PCWSTR c_resourceLocWinUI;

    static winrt::ResourceMap GetResourceMap();
public:
    static winrt::hstring GetLocalizedStringResource(const wstring_view &resourceName);

#ifdef MUX_EXPERIMENTAL
    static winrt::hstring GetLocalizedStringResourceFromWinUI(const wstring_view& resourceName);
#endif

    static winrt::LoadedImageSurface GetImageSurface(const wstring_view &assetName, winrt::Size imageSize);
    static winrt::IInspectable ResourceLookup(const winrt::Control& control, const winrt::IInspectable& key);

    static bool IsResourceIdNull(ResourceIdType resourceId)
    {
        return resourceId.size() == 0;
    }
};

#define SR_BasicRatingString L"BasicRatingString"
#define SR_CommunityRatingString L"CommunityRatingString"
#define SR_RatingsControlName L"RatingsControlName"
#define SR_RatingControlName L"RatingControlName"
#define SR_RatingUnset L"RatingUnset"
#define SR_NavigationButtonClosedName L"NavigationButtonClosedName"
#define SR_NavigationButtonOpenName L"NavigationButtonOpenName"
#define SR_NavigationViewItemDefaultControlName L"NavigationViewItemDefaultControlName"
#define SR_NavigationBackButtonName L"NavigationBackButtonName"
#define SR_NavigationBackButtonToolTip L"NavigationBackButtonToolTip"
#define SR_NavigationCloseButtonName L"NavigationCloseButtonName"
#define SR_NavigationOverflowButtonName L"NavigationOverflowButtonName"
#define SR_NavigationOverflowButtonText L"NavigationOverflowButtonText"
#define SR_NavigationOverflowButtonToolTip L"NavigationOverflowButtonToolTip"
#define SR_SettingsButtonName L"SettingsButtonName"
#define SR_NavigationViewSearchButtonName L"NavigationViewSearchButtonName"
#define SR_TextAlphaLabel L"TextAlphaLabel"
#define SR_AutomationNameAlphaSlider L"AutomationNameAlphaSlider"
#define SR_AutomationNameAlphaTextBox L"AutomationNameAlphaTextBox"
#define SR_AutomationNameHueSlider L"AutomationNameHueSlider"
#define SR_AutomationNameSaturationSlider L"AutomationNameSaturationSlider"
#define SR_AutomationNameValueSlider L"AutomationNameValueSlider"
#define SR_TextBlueLabel L"TextBlueLabel"
#define SR_AutomationNameBlueTextBox L"AutomationNameBlueTextBox"
#define SR_AutomationNameColorModelComboBox L"AutomationNameColorModelComboBox"
#define SR_AutomationNameColorSpectrum L"AutomationNameColorSpectrum"
#define SR_TextGreenLabel L"TextGreenLabel"
#define SR_AutomationNameGreenTextBox L"AutomationNameGreenTextBox"
#define SR_HelpTextColorSpectrum L"HelpTextColorSpectrum"
#define SR_AutomationNameHexTextBox L"AutomationNameHexTextBox"
#define SR_ContentHSVComboBoxItem L"ContentHSVComboBoxItem"
#define SR_TextHueLabel L"TextHueLabel"
#define SR_AutomationNameHueTextBox L"AutomationNameHueTextBox"
#define SR_LocalizedControlTypeColorSpectrum L"LocalizedControlTypeColorSpectrum"
#define SR_TextRedLabel L"TextRedLabel"
#define SR_AutomationNameRedTextBox L"AutomationNameRedTextBox"
#define SR_ContentRGBComboBoxItem L"ContentRGBComboBoxItem"
#define SR_TextSaturationLabel L"TextSaturationLabel"
#define SR_AutomationNameSaturationTextBox L"AutomationNameSaturationTextBox"
#define SR_TextValueLabel L"TextValueLabel"
#define SR_ValueStringColorSpectrumWithColorName L"ValueStringColorSpectrumWithColorName"
#define SR_ValueStringColorSpectrumWithoutColorName L"ValueStringColorSpectrumWithoutColorName"
#define SR_ValueStringHueSliderWithColorName L"ValueStringHueSliderWithColorName"
#define SR_ValueStringHueSliderWithoutColorName L"ValueStringHueSliderWithoutColorName"
#define SR_ValueStringSaturationSliderWithColorName L"ValueStringSaturationSliderWithColorName"
#define SR_ValueStringSaturationSliderWithoutColorName L"ValueStringSaturationSliderWithoutColorName"
#define SR_ValueStringValueSliderWithColorName L"ValueStringValueSliderWithColorName"
#define SR_ValueStringValueSliderWithoutColorName L"ValueStringValueSliderWithoutColorName"
#define SR_AutomationNameValueTextBox L"AutomationNameValueTextBox"
#define SR_ToolTipStringAlphaSlider L"ToolTipStringAlphaSlider"
#define SR_ToolTipStringHueSliderWithColorName L"ToolTipStringHueSliderWithColorName"
#define SR_ToolTipStringHueSliderWithoutColorName L"ToolTipStringHueSliderWithoutColorName"
#define SR_ToolTipStringSaturationSliderWithColorName L"ToolTipStringSaturationSliderWithColorName"
#define SR_ToolTipStringSaturationSliderWithoutColorName L"ToolTipStringSaturationSliderWithoutColorName"
#define SR_ToolTipStringValueSliderWithColorName L"ToolTipStringValueSliderWithColorName"
#define SR_ToolTipStringValueSliderWithoutColorName L"ToolTipStringValueSliderWithoutColorName"
#define SR_AutomationNameMoreButtonCollapsed L"AutomationNameMoreButtonCollapsed"
#define SR_AutomationNameMoreButtonExpanded L"AutomationNameMoreButtonExpanded"
#define SR_HelpTextMoreButton L"HelpTextMoreButton"
#define SR_TextMoreButtonLabelCollapsed L"TextMoreButtonLabelCollapsed"
#define SR_TextMoreButtonLabelExpanded L"TextMoreButtonLabelExpanded"
#define SR_BadgeItemPlural1 L"BadgeItemPlural1"
#define SR_BadgeItemPlural2 L"BadgeItemPlural2"
#define SR_BadgeItemPlural3 L"BadgeItemPlural3"
#define SR_BadgeItemPlural4 L"BadgeItemPlural4"
#define SR_BadgeItemPlural5 L"BadgeItemPlural5"
#define SR_BadgeItemPlural6 L"BadgeItemPlural6"
#define SR_BadgeItemPlural7 L"BadgeItemPlural7"
#define SR_BadgeItemSingular L"BadgeItemSingular"
#define SR_BadgeItemTextOverride L"BadgeItemTextOverride"
#define SR_BadgeIcon L"BadgeIcon"
#define SR_BadgeIconTextOverride L"BadgeIconTextOverride"
#define SR_PersonName L"PersonName"
#define SR_GroupName L"GroupName"
#define SR_CancelDraggingString L"CancelDraggingString"
#define SR_DefaultItemString L"DefaultItemString"
#define SR_DropIntoNodeString L"DropIntoNodeString"
#define SR_FallBackPlaceString L"FallBackPlaceString"
#define SR_PagerControlPageTextName L"PagerControlPageText"
#define SR_PagerControlPrefixTextName L"PagerControlPrefixText"
#define SR_PagerControlSuffixTextName L"PagerControlSuffixText"
#define SR_PagerControlFirstPageButtonTextName L"PagerControlFirstPageButtonText"
#define SR_PagerControlPreviousPageButtonTextName L"PagerControlPreviousPageButtonText"
#define SR_PagerControlNextPageButtonTextName L"PagerControlNextPageButtonText"
#define SR_PagerControlLastPageButtonTextName L"PagerControlLastPageButtonText"
#define SR_PipsPagerNameText L"PipsPagerNameText"
#define SR_PipsPagerNextPageButtonText L"PipsPagerNextPageButtonText"
#define SR_PipsPagerPreviousPageButtonText L"PipsPagerPreviousPageButtonText"
#define SR_PipsPagerPageText L"PipsPagerPageText"
#define SR_PlaceAfterString L"PlaceAfterString"
#define SR_PlaceBeforeString L"PlaceBeforeString"
#define SR_PlaceBetweenString L"PlaceBetweenString"
#define SR_ProgressRingName L"ProgressRingName"
#define SR_ProgressRingIndeterminateStatus L"ProgressRingIndeterminateStatus"
#define SR_ProgressBarIndeterminateStatus L"ProgressBarIndeterminateStatus"
#define SR_ProgressBarPausedStatus L"ProgressBarPausedStatus"
#define SR_ProgressBarErrorStatus L"ProgressBarErrorStatus"
#define SR_RatingLocalizedControlType L"RatingLocalizedControlType"
#define SR_BreadcrumbBarItemLocalizedControlType L"BreadcrumbBarItemLocalizedControlType"
#define SR_SplitButtonSecondaryButtonName L"SplitButtonSecondaryButtonName"
#define SR_ProofingMenuItemLabel L"ProofingMenuItemLabel"
#define SR_TextCommandLabelCut L"TextCommandLabelCut"
#define SR_TextCommandLabelCopy L"TextCommandLabelCopy"
#define SR_TextCommandLabelPaste L"TextCommandLabelPaste"
#define SR_TextCommandLabelSelectAll L"TextCommandLabelSelectAll"
#define SR_TextCommandLabelBold L"TextCommandLabelBold"
#define SR_TextCommandLabelItalic L"TextCommandLabelItalic"
#define SR_TextCommandLabelUnderline L"TextCommandLabelUnderline"
#define SR_TextCommandLabelUndo L"TextCommandLabelUndo"
#define SR_TextCommandLabelRedo L"TextCommandLabelRedo"
#define SR_TextCommandDescriptionCut L"TextCommandDescriptionCut"
#define SR_TextCommandDescriptionCopy L"TextCommandDescriptionCopy"
#define SR_TextCommandDescriptionPaste L"TextCommandDescriptionPaste"
#define SR_TextCommandDescriptionSelectAll L"TextCommandDescriptionSelectAll"
#define SR_TextCommandDescriptionBold L"TextCommandDescriptionBold"
#define SR_TextCommandDescriptionItalic L"TextCommandDescriptionItalic"
#define SR_TextCommandDescriptionUnderline L"TextCommandDescriptionUnderline"
#define SR_TextCommandDescriptionUndo L"TextCommandDescriptionUndo"
#define SR_TextCommandDescriptionRedo L"TextCommandDescriptionRedo"
#define SR_TextCommandKeyboardAcceleratorKeyCut L"TextCommandKeyboardAcceleratorKeyCut"
#define SR_TextCommandKeyboardAcceleratorKeyCopy L"TextCommandKeyboardAcceleratorKeyCopy"
#define SR_TextCommandKeyboardAcceleratorKeyPaste L"TextCommandKeyboardAcceleratorKeyPaste"
#define SR_TextCommandKeyboardAcceleratorKeySelectAll L"TextCommandKeyboardAcceleratorKeySelectAll"
#define SR_TextCommandKeyboardAcceleratorKeyBold L"TextCommandKeyboardAcceleratorKeyBold"
#define SR_TextCommandKeyboardAcceleratorKeyItalic L"TextCommandKeyboardAcceleratorKeyItalic"
#define SR_TextCommandKeyboardAcceleratorKeyUnderline L"TextCommandKeyboardAcceleratorKeyUnderline"
#define SR_TextCommandKeyboardAcceleratorKeyUndo L"TextCommandKeyboardAcceleratorKeyUndo"
#define SR_TextCommandKeyboardAcceleratorKeyRedo L"TextCommandKeyboardAcceleratorKeyRedo"
#define SR_TeachingTipAlternateCloseButtonName L"TeachingTipAlternateCloseButtonName"
#define SR_TeachingTipAlternateCloseButtonTooltip L"TeachingTipAlternateCloseButtonTooltip"
#define SR_TeachingTipCustomLandmarkName L"TeachingTipCustomLandmarkName"
#define SR_TeachingTipNotification L"TeachingTipNotification"
#define SR_TeachingTipNotificationWithoutAppName L"TeachingTipNotificationWithoutAppName"
#define SR_TabViewAddButtonName L"TabViewAddButtonName"
#define SR_TabViewAddButtonTooltip L"TabViewAddButtonTooltip"
#define SR_TabViewCloseButtonName L"TabViewCloseButtonName"
#define SR_TabViewCloseButtonTooltip L"TabViewCloseButtonTooltip"
#define SR_TabViewCloseButtonTooltipWithKA L"TabViewCloseButtonTooltipWithKA"
#define SR_TabViewScrollDecreaseButtonTooltip L"TabViewScrollDecreaseButtonTooltip"
#define SR_TabViewScrollIncreaseButtonTooltip L"TabViewScrollIncreaseButtonTooltip"
#define SR_NumberBoxUpSpinButtonName L"NumberBoxUpSpinButtonName"
#define SR_NumberBoxDownSpinButtonName L"NumberBoxDownSpinButtonName"
#define SR_NumberBoxMaximumValueStatus L"NumberBoxMaximumValueStatus"
#define SR_NumberBoxMinimumValueStatus L"NumberBoxMinimumValueStatus"
#define SR_ExpanderDefaultControlName L"ExpanderDefaultControlName"

#define SR_InfoBarCloseButtonName L"InfoBarCloseButtonName"
#define SR_InfoBarOpenedNotification L"InfoBarOpenedNotification"
#define SR_InfoBarClosedNotification L"InfoBarClosedNotification"
#define SR_InfoBarCustomLandmarkName L"InfoBarCustomLandmarkName"
#define SR_InfoBarCloseButtonTooltip L"InfoBarCloseButtonTooltip"
#define SR_InfoBarSeverityInformationalName L"InfoBarSeverityInformationalName"
#define SR_InfoBarSeveritySuccessName L"InfoBarSeveritySuccessName"
#define SR_InfoBarSeverityWarningName L"InfoBarSeverityWarningName"
#define SR_InfoBarSeverityErrorName L"InfoBarSeverityErrorName"
#define SR_InfoBarIconSeverityInformationalName L"InfoBarIconSeverityInformationalName"
#define SR_InfoBarIconSeveritySuccessName L"InfoBarIconSeveritySuccessName"
#define SR_InfoBarIconSeverityWarningName L"InfoBarIconSeverityWarningName"
#define SR_InfoBarIconSeverityErrorName L"InfoBarIconSeverityErrorName"

#define SR_AutomationNameEllipsisBreadcrumbBarItem L"AutomationNameEllipsisBreadcrumbBarItem"

#define SR_CommandBarFlyoutCommandBarLocalizedControlType L"CommandBarFlyoutCommandBarLocalizedControlType"
#define SR_CommandBarFlyoutAppBarButtonLocalizedControlType L"CommandBarFlyoutAppBarButtonLocalizedControlType"
#define SR_CommandBarFlyoutAppBarToggleButtonLocalizedControlType L"CommandBarFlyoutAppBarToggleButtonLocalizedControlType"

#define SR_WarningSuitableWebView2NotFound L"WarningSuitableWebView2NotFound"
#define SR_DownloadWebView2Runtime L"DownloadWebView2Runtime"

#define IR_NoiseAsset_256X256_PNG L"NoiseAsset_256X256_PNG"
