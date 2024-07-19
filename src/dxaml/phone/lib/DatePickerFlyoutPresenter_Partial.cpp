// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ElevationHelper.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

#define DATEPICKER_RTL_CHARACTER_CODE 8207
#define DATEPICKER_MIN_MAX_YEAR_DEAFULT_OFFSET 100
#define DATEPICKER_SENTINELTIME_HOUR 12
#define DATEPICKER_SENTINELTIME_MINUTE 0
#define DATEPICKER_SENTINELTIME_SECOND 0
#define DATEPICKER_WRAP_AROUND_MONTHS_FIRST_INDEX 1

const WCHAR DatePickerFlyoutPresenter::_dayLoopingSelectorAutomationId[] = L"DayLoopingSelector";
const WCHAR DatePickerFlyoutPresenter::_monthLoopingSelectorAutomationId[] = L"MonthLoopingSelector";
const WCHAR DatePickerFlyoutPresenter::_yearLoopingSelectorAutomationId[] = L"YearLoopingSelector";

const WCHAR DatePickerFlyoutPresenter::_firstPickerHostName[] = L"FirstPickerHost";
const WCHAR DatePickerFlyoutPresenter::_secondPickerHostName[] = L"SecondPickerHost";
const WCHAR DatePickerFlyoutPresenter::_thirdPickerHostName[] = L"ThirdPickerHost";
const WCHAR DatePickerFlyoutPresenter::_backgroundName[] = L"Background";
const WCHAR DatePickerFlyoutPresenter::_contentPanelName[] = L"ContentPanel";
const WCHAR DatePickerFlyoutPresenter::_titlePresenterName[] = L"TitlePresenter";

DatePickerFlyoutPresenter::DatePickerFlyoutPresenter() :
    _isInitializing(TRUE),
    _dayVisible(TRUE),
    _monthVisible(TRUE),
    _yearVisible(TRUE),
    _minYear(wf::DateTime()),
    _maxYear(wf::DateTime()),
    _acceptDismissButtonsVisible(true)
{}

_Check_return_ HRESULT
DatePickerFlyoutPresenter::InitializeImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IControlFactory> spInnerFactory;
    wrl::ComPtr<xaml_controls::IControl> spInnerInstance;
    wrl::ComPtr<IInspectable> spInnerInspectable;

    IFC(DatePickerFlyoutPresenterGenerated::InitializeImpl());

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Control).Get(),
        &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IInspectable*>(static_cast<IDatePickerFlyoutPresenter*>(this)),
        &spInnerInspectable,
        &spInnerInstance));

    IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

    IFC(Private::SetDefaultStyleKey(
            spInnerInspectable.Get(),
            L"Microsoft.UI.Xaml.Controls.DatePickerFlyoutPresenter"));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
DatePickerFlyoutPresenter::OnApplyTemplateImpl()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml_controls::IControlProtected> spControlProtected;
    wrl::ComPtr<xaml_controls::IBorder> spBackgroundBorder;
    wrl::ComPtr<xaml_controls::ITextBlock> spTitlePresenter;
    wrl::ComPtr<xaml_controls::IBorder> spFirstPickerHost;
    wrl::ComPtr<xaml_controls::IBorder> spSecondPickerHost;
    wrl::ComPtr<xaml_controls::IBorder> spThirdPickerHost;
    wrl::ComPtr<IFrameworkElement> spContentPanel;
    wrl::ComPtr<IColumnDefinition> spDayColumn;
    wrl::ComPtr<IColumnDefinition> spMonthColumn;
    wrl::ComPtr<IColumnDefinition> spYearColumn;
    wrl::ComPtr<IColumnDefinition> spFirstSpacerColumn;
    wrl::ComPtr<IColumnDefinition> spSecondSpacerColumn;
    wrl::ComPtr<IGrid> spPickerHostGrid;
    wrl::ComPtr<IUIElement> spFirstPickerSpacing;
    wrl::ComPtr<IUIElement> spSecondPickerSpacing;
    wrl::ComPtr<IPanel> spPickerHostGridAsPanel;
    wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spPickerHostGridChildren;
    wrl::ComPtr<IUIElement> spAcceptDismissHostGrid;
    wrl::ComPtr<IUIElement> spAcceptButton;
    wrl::ComPtr<IUIElement> spDismissButton;
    xaml::Thickness itemPadding;
    xaml::Thickness monthPadding;

    if (_tpDayPicker)
    {
        IFC(_tpDayPicker->remove_SelectionChanged(_daySelectionChangedToken));
    }

    if (_tpMonthPicker)
    {
        IFC(_tpMonthPicker->remove_SelectionChanged(_monthSelectionChangedToken));
    }

    if (_tpYearPicker)
    {
        IFC(_tpYearPicker->remove_SelectionChanged(_yearSelectionChangedToken));
    }

    _tpBackgroundBorder.Clear();
    _tpTitlePresenter.Clear();
    _tpDayPicker.Clear();
    _tpMonthPicker.Clear();
    _tpYearPicker.Clear();
    _tpFirstPickerHost.Clear();
    _tpSecondPickerHost.Clear();
    _tpThirdPickerHost.Clear();
    _tpContentPanel.Clear();
    _tpAcceptDismissHostGrid.Clear();
    _tpAcceptButton.Clear();
    _tpDismissButton.Clear();

    IFC(QueryInterface(__uuidof(xaml_controls::IControlProtected), &spControlProtected));

    IFC(DatePickerFlyoutPresenterGenerated::OnApplyTemplateImpl());

    IFC(Private::AttachTemplatePart<xaml_controls::IBorder>(
        spControlProtected.Get(),
        _backgroundName,
        &spBackgroundBorder));
    IFC(SetPtrValue(_tpBackgroundBorder, spBackgroundBorder.Get()));

    IFC(Private::AttachTemplatePart<xaml_controls::ITextBlock>(
        spControlProtected.Get(),
        _titlePresenterName,
        &spTitlePresenter));
    IFC(SetPtrValue(_tpTitlePresenter, spTitlePresenter.Get()))

    if (spTitlePresenter)
    {
        wrl::ComPtr<xaml::IUIElement> spPresenterAsUI;
        IFC(_tpTitlePresenter.As(&spPresenterAsUI));
        IFC(spPresenterAsUI->put_Visibility(WindowsIsStringEmpty(_title.Get()) ? xaml::Visibility_Collapsed : xaml::Visibility_Visible));
        IFC(_tpTitlePresenter->put_Text(_title.Get()));
    }

    IFC(Private::AttachTemplatePart<xaml_controls::IBorder>(
        spControlProtected.Get(),
        _firstPickerHostName,
        &spFirstPickerHost));
    IFC(SetPtrValue(_tpFirstPickerHost, spFirstPickerHost.Get()));

    IFC(Private::AttachTemplatePart<xaml_controls::IBorder>(
        spControlProtected.Get(),
        _secondPickerHostName,
        &spSecondPickerHost));
    IFC(SetPtrValue(_tpSecondPickerHost, spSecondPickerHost.Get()));

    IFC(Private::AttachTemplatePart<xaml_controls::IBorder>(
        spControlProtected.Get(),
        _thirdPickerHostName,
        &spThirdPickerHost));
    IFC(SetPtrValue(_tpThirdPickerHost, spThirdPickerHost.Get()));

    IFC(Private::AttachTemplatePart<xaml::IFrameworkElement>(
        spControlProtected.Get(),
        _contentPanelName,
        &spContentPanel));
    IFC(SetPtrValue(_tpContentPanel, spContentPanel.Get()));

    IFC(Private::AttachTemplatePart<xaml_controls::IColumnDefinition>(
        spControlProtected.Get(),
        L"DayColumn",
        &spDayColumn));
    IFC(SetPtrValue(_tpDayColumn, spDayColumn.Get()));

    IFC(Private::AttachTemplatePart<xaml_controls::IColumnDefinition>(
        spControlProtected.Get(),
        L"MonthColumn",
        &spMonthColumn));
    IFC(SetPtrValue(_tpMonthColumn, spMonthColumn.Get()));

    IFC(Private::AttachTemplatePart<xaml_controls::IColumnDefinition>(
        spControlProtected.Get(),
        L"YearColumn",
        &spYearColumn));
    IFC(SetPtrValue(_tpYearColumn, spYearColumn.Get()));

    IFC(Private::AttachTemplatePart<xaml_controls::IColumnDefinition>(
        spControlProtected.Get(),
        L"FirstSpacerColumn",
        &spFirstSpacerColumn));
    IFC(SetPtrValue(_tpFirstSpacerColumn, spFirstSpacerColumn.Get()));

    IFC(Private::AttachTemplatePart<xaml_controls::IColumnDefinition>(
        spControlProtected.Get(),
        L"SecondSpacerColumn",
        &spSecondSpacerColumn));
    IFC(SetPtrValue(_tpSecondSpacerColumn, spSecondSpacerColumn.Get()));

    IFC(Private::AttachTemplatePart<IUIElement>(
        spControlProtected.Get(),
        L"FirstPickerSpacing",
        &spFirstPickerSpacing));
    IFC(SetPtrValue(_tpFirstPickerSpacing, spFirstPickerSpacing.Get()));

    IFC(Private::AttachTemplatePart<IUIElement>(
        spControlProtected.Get(),
        L"SecondPickerSpacing",
        &spSecondPickerSpacing));
    IFC(SetPtrValue(_tpSecondPickerSpacing, spSecondPickerSpacing.Get()));

    IFC(Private::AttachTemplatePart<xaml_controls::IGrid>(
        spControlProtected.Get(),
        L"PickerHostGrid",
        &spPickerHostGrid));
    IFC(SetPtrValue(_tpPickerHostGrid, spPickerHostGrid.Get()));

    IFC(Private::AttachTemplatePart<IUIElement>(
        spControlProtected.Get(),
        L"AcceptDismissHostGrid",
        &spAcceptDismissHostGrid));
    IFC(SetPtrValue(_tpAcceptDismissHostGrid, spAcceptDismissHostGrid.Get()));

    IFC(Private::AttachTemplatePart<IUIElement>(
        spControlProtected.Get(),
        L"AcceptButton",
        &spAcceptButton));
    IFC(SetPtrValue(_tpAcceptButton, spAcceptButton.Get()));

    IFC(Private::AttachTemplatePart<IUIElement>(
        spControlProtected.Get(),
        L"DismissButton",
        &spDismissButton));
    IFC(SetPtrValue(_tpDismissButton, spDismissButton.Get()));

    if (_tpPickerHostGrid)
    {
        IFC(_tpPickerHostGrid.As(&spPickerHostGridAsPanel));
        IFC(spPickerHostGridAsPanel->get_Children(&spPickerHostGridChildren));
        ASSERT(spPickerHostGridChildren);
    }

    INT32 itemHeight;
    double itemHeightFromMarkup;
    if (SUCCEEDED(Private::ApplicationResourceHelpers::GetApplicationResource(
        wrl_wrappers::HStringReference(L"DatePickerFlyoutPresenterItemHeight").Get(),
        &itemHeightFromMarkup)))
    {
        itemHeight = static_cast<INT32>(itemHeightFromMarkup);
    }
    else
    {
        // Value for RS4. Used if resource values not found
        itemHeight = 44;
    }

    if (FAILED(Private::ApplicationResourceHelpers::GetApplicationResource(
        wrl_wrappers::HStringReference(L"DatePickerFlyoutPresenterItemPadding").Get(),
        &itemPadding)))
    {
        itemPadding = { 0, 3, 0, 5 };
    }

    if (FAILED(Private::ApplicationResourceHelpers::GetApplicationResource(
        wrl_wrappers::HStringReference(L"DatePickerFlyoutPresenterMonthPadding").Get(),
        &monthPadding)))
    {
        monthPadding = { 9, 3, 0, 5 };
    }

    //The Template uses a single host Grid for the 3 LoopingSelectors.
    if (_tpFirstPickerHost || _tpPickerHostGrid)
    {
        wrl::ComPtr<xaml::IUIElement> spLSAsUI;
        wrl::ComPtr<xaml::IFrameworkElement> spLSAsFE;
        wrl::ComPtr<xaml::Controls::IControl> spLSAsControl;
        wrl::ComPtr<xaml_primitives::ILoopingSelector> spMonthPicker;

        IFC(wrl::MakeAndInitialize<xaml_primitives::LoopingSelector>(&spMonthPicker));
        IFC(SetPtrValue(_tpMonthPicker, spMonthPicker.Get()));

        IFC(spMonthPicker.As(&spLSAsUI));
        IFC(spMonthPicker.As(&spLSAsFE));
        IFC(spMonthPicker.As(&spLSAsControl));

        //Don't set ItemWidth. We want the item to size to the width of its parent.
        IFC(spMonthPicker->put_ItemHeight(itemHeight));
        IFC(spLSAsControl->put_HorizontalContentAlignment(xaml::HorizontalAlignment_Left));
        IFC(spLSAsControl->put_Padding(monthPadding));
        IFC(spLSAsFE->put_Name(wrl_wrappers::HStringReference(L"MonthLoopingSelector").Get()));

        if (_tpFirstPickerHost)
        {
            IFC(_tpFirstPickerHost->put_Child(spLSAsUI.Get()));
        }
        else //_tpPickerHostGrid != null
        {
            IFC(spPickerHostGridChildren->Append(spLSAsUI.Get()));
        }
    }

    if (_tpSecondPickerHost || _tpPickerHostGrid)
    {
        wrl::ComPtr<xaml::IUIElement> spLSAsUI;
        wrl::ComPtr<xaml::IFrameworkElement> spLSAsFE;
        wrl::ComPtr<xaml::Controls::IControl> spLSAsControl;
        wrl::ComPtr<xaml_primitives::ILoopingSelector> spDayPicker;

        IFC(wrl::MakeAndInitialize<xaml_primitives::LoopingSelector>(&spDayPicker));
        IFC(SetPtrValue(_tpDayPicker, spDayPicker.Get()));

        IFC(spDayPicker.As(&spLSAsUI));
        IFC(spDayPicker.As(&spLSAsFE));
        IFC(spDayPicker.As(&spLSAsControl));

        //Don't set ItemWidth. We want the item to size to the width of its parent.
        IFC(spDayPicker->put_ItemHeight(itemHeight));
        IFC(spLSAsControl->put_HorizontalContentAlignment(xaml::HorizontalAlignment_Center));
        IFC(spLSAsControl->put_Padding(itemPadding));
        IFC(spLSAsFE->put_Name(wrl_wrappers::HStringReference(L"DayLoopingSelector").Get()));

        if (_tpSecondPickerHost)
        {
            IFC(_tpSecondPickerHost->put_Child(spLSAsUI.Get()));
        }
        else //_tpPickerHostGrid != null
        {
            IFC(spPickerHostGridChildren->Append(spLSAsUI.Get()));
        }
    }

    if (_tpThirdPickerHost || _tpPickerHostGrid)
    {
        wrl::ComPtr<xaml::IUIElement> spLSAsUI;
        wrl::ComPtr<xaml::IFrameworkElement> spLSAsFE;
        wrl::ComPtr<xaml::Controls::IControl> spLSAsControl;
        wrl::ComPtr<xaml_primitives::ILoopingSelector> spYearPicker;

        IFC(wrl::MakeAndInitialize<xaml_primitives::LoopingSelector>(&spYearPicker));
        IFC(SetPtrValue(_tpYearPicker, spYearPicker.Get()));

        IFC(spYearPicker.As(&spLSAsUI));
        IFC(spYearPicker.As(&spLSAsFE));
        IFC(spYearPicker.As(&spLSAsControl));

        IFC(spYearPicker->put_ShouldLoop(false));

        //Don't set ItemWidth. We want the item to size to the width of its parent.
        IFC(spYearPicker->put_ItemHeight(itemHeight));
        IFC(spLSAsControl->put_HorizontalContentAlignment(xaml::HorizontalAlignment_Center));
        IFC(spLSAsControl->put_Padding(itemPadding));
        IFC(spLSAsFE->put_Name(wrl_wrappers::HStringReference(L"YearLoopingSelector").Get()));

        if (_tpSecondPickerHost)
        {
            IFC(_tpThirdPickerHost->put_Child(spLSAsUI.Get()));
        }
        else //_tpPickerHostGrid != null
        {
            IFC(spPickerHostGridChildren->Append(spLSAsUI.Get()));
        }
    }

    if (_tpDayPicker)
    {
        wrl::ComPtr<xaml::IDependencyObject> spDayPickerAsDO;
        wrl_wrappers::HString localizedName;

        IFC(_tpDayPicker->add_SelectionChanged(
            wrl::Callback<xaml_controls::ISelectionChangedEventHandler>
            (this, &DatePickerFlyoutPresenter::OnSelectorSelectionChanged).Get(),
            &_daySelectionChangedToken));

        IFC(_tpDayPicker.As(&spDayPickerAsDO));

        IFC(Private::FindStringResource(
            UIA_AP_DATEPICKER_DAYNAME,
            localizedName.ReleaseAndGetAddressOf()));
        IFC(Private::AutomationHelper::SetElementAutomationName(
            spDayPickerAsDO.Get(),
            localizedName.Get()));

        IFC(Private::AutomationHelper::SetElementAutomationId(
            spDayPickerAsDO.Get(),
            wrl_wrappers::HStringReference(_dayLoopingSelectorAutomationId).Get()));
    }
    if (_tpMonthPicker)
    {
        wrl::ComPtr<xaml::IDependencyObject> spMonthPickerAsDO;
        wrl_wrappers::HString localizedName;

        IFC(_tpMonthPicker->add_SelectionChanged(
            wrl::Callback<xaml_controls::ISelectionChangedEventHandler>
            (this, &DatePickerFlyoutPresenter::OnSelectorSelectionChanged).Get(),
            &_monthSelectionChangedToken));

        IFC(_tpMonthPicker.As(&spMonthPickerAsDO));

        IFC(Private::FindStringResource(
            UIA_AP_DATEPICKER_MONTHNAME,
            localizedName.ReleaseAndGetAddressOf()));
        IFC(Private::AutomationHelper::SetElementAutomationName(
            spMonthPickerAsDO.Get(),
            localizedName.Get()));

        IFC(Private::AutomationHelper::SetElementAutomationId(
            spMonthPickerAsDO.Get(),
            wrl_wrappers::HStringReference(_monthLoopingSelectorAutomationId).Get()));

    }
    if (_tpYearPicker)
    {
        wrl::ComPtr<xaml::IDependencyObject> spYearPickerAsDO;
        wrl_wrappers::HString localizedName;

        IFC(_tpYearPicker->add_SelectionChanged(
            wrl::Callback<xaml_controls::ISelectionChangedEventHandler>
            (this, &DatePickerFlyoutPresenter::OnSelectorSelectionChanged).Get(),
            &_yearSelectionChangedToken));

        IFC(_tpYearPicker.As(&spYearPickerAsDO));

        IFC(Private::FindStringResource(
            UIA_AP_DATEPICKER_YEARNAME,
            localizedName.ReleaseAndGetAddressOf()));
        IFC(Private::AutomationHelper::SetElementAutomationName(
            spYearPickerAsDO.Get(),
            localizedName.Get()));

        IFC(Private::AutomationHelper::SetElementAutomationId(
            spYearPickerAsDO.Get(),
            wrl_wrappers::HStringReference(_yearLoopingSelectorAutomationId).Get()));
    }

    if (!(_tpYearSource && _tpMonthSource && _tpDaySource))
    {
        wrl::ComPtr<wfci_::Vector<IInspectable*>> spCollection;
        wrl::ComPtr<wfc::IVector<IInspectable*>> spCollectionAsInterface;

        IFC(wfci_::Vector<IInspectable*>::Make(&spCollection));
        IFC(spCollection.As(&spCollectionAsInterface));
        IFC(SetPtrValue(_tpDaySource, spCollectionAsInterface.Get()));

        IFC(wfci_::Vector<IInspectable*>::Make(&spCollection));
        IFC(spCollection.As(&spCollectionAsInterface));
        IFC(SetPtrValue(_tpMonthSource, spCollectionAsInterface.Get()));

        IFC(wfci_::Vector<IInspectable*>::Make(&spCollection));
        IFC(spCollection.As(&spCollectionAsInterface));
        IFC(SetPtrValue(_tpYearSource, spCollectionAsInterface.Get()));
    }

    IFC(RefreshSetup());
    IFC(SetAcceptDismissButtonsVisibility(_acceptDismissButtonsVisible));

    // Apply a shadow
    BOOLEAN isDefaultShadowEnabled;
    IFC(get_IsDefaultShadowEnabled(&isDefaultShadowEnabled));
    if (isDefaultShadowEnabled)
    {
        wrl::ComPtr<Microsoft::UI::Xaml::Media::IThemeShadowStaticsPrivate> themeShadowStatics;
        IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(
            RuntimeClass_Microsoft_UI_Xaml_Media_ThemeShadow).Get(),
            &themeShadowStatics));
        BOOLEAN isDropShadowMode;
        IFC(themeShadowStatics->get_IsDropShadowMode(&isDropShadowMode));
        wrl::ComPtr<xaml::IUIElement> spShadowTarget;
        if (isDropShadowMode)
        {
            IFC(QueryInterface(__uuidof(xaml::IUIElement), &spShadowTarget));
        }
        else
        {
            IFC(_tpBackgroundBorder.As(&spShadowTarget));
        }

        IFC(ApplyElevationEffect(spShadowTarget.Get()));
    }

Cleanup:
    _isInitializing = FALSE;
    RRETURN(hr);
}

_Check_return_ HRESULT
DatePickerFlyoutPresenter::OnCreateAutomationPeerImpl(
    _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::DatePickerFlyoutPresenter> spThis(this);
    wrl::ComPtr<xaml_controls::IDatePickerFlyoutPresenter> spThisAsIDatePickerFlyoutPresenter;
    wrl::ComPtr<xaml_automation_peers::DatePickerFlyoutPresenterAutomationPeer> spDatePickerFlyoutPresenterAutomationPeer;

    IFC(spThis.As(&spThisAsIDatePickerFlyoutPresenter));
    IFC(wrl::MakeAndInitialize<xaml_automation_peers::DatePickerFlyoutPresenterAutomationPeer>
            (&spDatePickerFlyoutPresenterAutomationPeer, spThisAsIDatePickerFlyoutPresenter.Get()));

    IFC(spDatePickerFlyoutPresenterAutomationPeer.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DatePickerFlyoutPresenter::PullPropertiesFromOwner(
    _In_ xaml_controls::IDatePickerFlyout* pOwner)
{
    wrl::ComPtr<IDatePickerFlyout> spOwner(pOwner);
    wrl::ComPtr<IDependencyObject> spOwnerAsDO;
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBaseStatics> spPickerFlyoutBaseStatics;

    wrl_wrappers::HString calendarIdentifier;
    wrl_wrappers::HString title;
    wf::DateTime date = {};
    BOOLEAN monthVisible = FALSE;
    BOOLEAN yearVisible = FALSE;
    BOOLEAN dayVisible = FALSE;
    wf::DateTime minYear = {};
    wf::DateTime maxYear = {};
    wrl_wrappers::HString dayFormat;
    wrl_wrappers::HString monthFormat;
    wrl_wrappers::HString yearFormat;

    INT32 dayFormatCompareResult = 0;
    INT32 monthFormatCompareResult = 0;
    INT32 yearFormatCompareResult = 0;

    INT32 calendarIDCompareResult = 0;
    wrl_wrappers::HString oldCalendarID;
    IFC_RETURN(oldCalendarID.Set(_calendarIdentifier.Get())); // copies the string

    IFC_RETURN(spOwner.As(&spOwnerAsDO));
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_PickerFlyoutBase).Get(),
        &spPickerFlyoutBaseStatics));

    // Pull properties from owner
    IFC_RETURN(spOwner->get_CalendarIdentifier(calendarIdentifier.GetAddressOf()));
    IFC_RETURN(spPickerFlyoutBaseStatics->GetTitle(spOwnerAsDO.Get(), title.GetAddressOf()));
    IFC_RETURN(spOwner->get_MonthVisible(&monthVisible));
    IFC_RETURN(spOwner->get_YearVisible(&yearVisible));
    IFC_RETURN(spOwner->get_DayVisible(&dayVisible));
    IFC_RETURN(spOwner->get_MinYear(&minYear));
    IFC_RETURN(spOwner->get_MaxYear(&maxYear));
    IFC_RETURN(spOwner->get_Date(&date));
    IFC_RETURN(spOwner->get_DayFormat(dayFormat.GetAddressOf()));
    IFC_RETURN(spOwner->get_MonthFormat(monthFormat.GetAddressOf()));
    IFC_RETURN(spOwner->get_YearFormat(yearFormat.GetAddressOf()));

    // Check which values have changed
    IFC_RETURN(WindowsCompareStringOrdinal(oldCalendarID.Get(), calendarIdentifier.Get(), &calendarIDCompareResult));
    IFC_RETURN(WindowsCompareStringOrdinal(_dayFormat.Get(), dayFormat.Get(), &dayFormatCompareResult));
    IFC_RETURN(WindowsCompareStringOrdinal(_monthFormat.Get(), monthFormat.Get(), &monthFormatCompareResult));
    IFC_RETURN(WindowsCompareStringOrdinal(_yearFormat.Get(), yearFormat.Get(), &yearFormatCompareResult));

    const bool dayFormatChanged = dayFormatCompareResult != 0;
    const bool monthFormatChanged = monthFormatCompareResult != 0;
    const bool yearFormatChanged = yearFormatCompareResult != 0;

    const bool haveFieldVisibilitiesChanged = dayVisible != _dayVisible ||
                                   monthVisible != _monthVisible ||
                                   yearVisible != _yearVisible;

    const bool haveYearLimitsChanged = maxYear.UniversalTime != _maxYear.UniversalTime ||
                            minYear.UniversalTime != _minYear.UniversalTime;

    // Store new values

    IFC_RETURN(calendarIdentifier.CopyTo(_calendarIdentifier.ReleaseAndGetAddressOf()));
    IFC_RETURN(title.CopyTo(_title.ReleaseAndGetAddressOf()));
    if (_tpTitlePresenter)
    {
        wrl::ComPtr<xaml::IUIElement> spPresenterAsUI;
        IFC_RETURN(_tpTitlePresenter.As(&spPresenterAsUI));
        IFC_RETURN(spPresenterAsUI->put_Visibility(WindowsIsStringEmpty(_title.Get()) ? xaml::Visibility_Collapsed : xaml::Visibility_Visible));
        IFC_RETURN(_tpTitlePresenter->put_Text(_title.Get()));
    }
    _dayVisible = dayVisible;
    _monthVisible = monthVisible;
    _yearVisible = yearVisible;
    _minYear = minYear;
    _maxYear = maxYear;

    IFC_RETURN(dayFormat.CopyTo(_dayFormat.ReleaseAndGetAddressOf()));
    IFC_RETURN(monthFormat.CopyTo(_monthFormat.ReleaseAndGetAddressOf()));
    IFC_RETURN(yearFormat.CopyTo(_yearFormat.ReleaseAndGetAddressOf()));

    // Perform updates
    if (calendarIDCompareResult != 0)
    {
        IFC_RETURN(OnCalendarIdentifierPropertyChanged(oldCalendarID.Get()));
    }
    if (dayFormatChanged)
    {
        // The cached formatters are no longer valid. They will be regenerated via RefreshSetup
        _tpPrimaryDayFormatter.Clear();
    }
    if (monthFormatChanged)
    {
        // The cached formatters are no longer valid. They will be regenerated via RefreshSetup
        _tpPrimaryMonthFormatter.Clear();
    }
    if (yearFormatChanged)
    {
        // The cached formatters are no longer valid. They will be regenerated via RefreshSetup
        _tpPrimaryYearFormatter.Clear();
    }
    if (haveYearLimitsChanged || dayFormatChanged || monthFormatChanged || yearFormatChanged)
    {
        IFC_RETURN(RefreshSetup());
    }
    if (haveFieldVisibilitiesChanged)
    {
        IFC_RETURN(UpdateOrderAndLayout());
    }

    // Date has its own handler since it can be set through multiple codepaths
    IFC_RETURN(SetDate(date));

    return S_OK;
}

_Check_return_ HRESULT DatePickerFlyoutPresenter::SetAcceptDismissButtonsVisibility(_In_ bool isVisible)
{
    // If we have a named host grid for the buttons, we'll hide that.
    // Otherwise, we'll just hide the buttons, since we shouldn't
    // assume anything about the surrounding visual tree.
    if (_tpAcceptDismissHostGrid)
    {
        IFC_RETURN(_tpAcceptDismissHostGrid->put_Visibility(isVisible ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
    }
    else if (_tpAcceptButton && _tpDismissButton)
    {
        IFC_RETURN(_tpAcceptButton->put_Visibility(isVisible ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
        IFC_RETURN(_tpDismissButton->put_Visibility(isVisible ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
    }

    _acceptDismissButtonsVisible = isVisible;

    return S_OK;
}

_Check_return_ HRESULT DatePickerFlyoutPresenter::GetDate(_Out_ wf::DateTime* pDate)
{
    *pDate = _date;
    RRETURN(S_OK);
}

_Check_return_ HRESULT DatePickerFlyoutPresenter::SetDate(_In_ wf::DateTime newDate)
{
    // If we're setting the date to the null sentinel value,
    // we'll instead set it to the current date for the purposes
    // of where to place the user's position in the looping selectors.
    if (newDate.UniversalTime == 0)
    {
        wf::DateTime dateTime = {};
        wrl::ComPtr<wg::ICalendar> calendar;

        IFC_RETURN(CreateNewCalendar(_calendarIdentifier.Get(), &calendar));
        IFC_RETURN(calendar->SetToNow());
        IFC_RETURN(calendar->GetDateTime(&newDate));
    }

    if (newDate.UniversalTime != _date.UniversalTime)
    {
        wf::DateTime oldDate = _date;
        _date = newDate;
        IFC_RETURN(OnDateChanged(oldDate, _date));
    }

    return S_OK;
}

_Check_return_ HRESULT DatePickerFlyoutPresenter::OnKeyDownImpl(_In_ xaml_input::IKeyRoutedEventArgs* pEventArgs)
{
    return DateTimePickerFlyoutHelper::OnKeyDownImpl(pEventArgs, _tpFirstPickerAsControl.Get(), _tpSecondPickerAsControl.Get(), _tpThirdPickerAsControl.Get(), _tpContentPanel.Get());
}

_Check_return_ HRESULT DatePickerFlyoutPresenter::GetDefaultIsDefaultShadowEnabled(_Outptr_ IInspectable** ppIsDefaultShadowEnabledValue)
{
    HRESULT hr = S_OK;

    IFC(Private::ValueBoxer::CreateBoolean(true, ppIsDefaultShadowEnabledValue));

Cleanup:
    RRETURN(hr);
}

#pragma region Logical Functionality
// Clears the ItemsSource and SelectedItem properties of the selectors.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::ClearSelectors(
    _In_ BOOLEAN clearDay,
    _In_ BOOLEAN clearMonth,
    _In_ BOOLEAN clearYear)
{
    HRESULT hr = S_OK;

    if (_tpDayPicker.Get() && clearDay)
    {
        IFC(_tpDayPicker->put_Items(nullptr));
    }

    if (_tpMonthPicker.Get() && clearMonth)
    {
        IFC(_tpMonthPicker->put_Items(nullptr));
    }

    if (_tpYearPicker.Get() && clearYear)
    {
        IFC(_tpYearPicker->put_Items(nullptr));
    }

Cleanup:
    RRETURN(hr);
}

// Get indices of related fields of current Date for generated itemsources.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::GetIndices(
    _Inout_ INT32& yearIndex,
    _Inout_ INT32& monthIndex,
    _Inout_ INT32& dayIndex)
{
    HRESULT hr = S_OK;
    INT32 currentIndex = 0;
    INT32 firstIndex = 0;
    INT32 monthsInThisYear = 0;

    // We will need the second calendar for calculating the year difference
    IFC(_tpBaselineCalendar->SetDateTime(ClampDate(_date, _startDate, _endDate)));
    IFC(_tpCalendar->SetDateTime(_startDate));
    IFC(GetYearDifference(_tpCalendar.Get(), _tpBaselineCalendar.Get(), yearIndex));

    IFC(_tpBaselineCalendar->get_FirstMonthInThisYear(&firstIndex));
    IFC(_tpBaselineCalendar->get_Month(&currentIndex));
    IFC(_tpBaselineCalendar->get_NumberOfMonthsInThisYear(&monthsInThisYear));
    if (currentIndex - firstIndex >= 0)
    {
        monthIndex = currentIndex - firstIndex;
    }
    else
    {
        // A special case is in some ThaiCalendar years first month
        // of the year is April, last month is March and month flow is wrap-around
        // style; April, March .... November, December, January, February, March. So the first index
        // will be 4 and last index will be 3. We are handling the case to convert this wraparound behavior
        // into selected index.
        monthIndex = currentIndex - firstIndex + monthsInThisYear;
    }
    IFC(_tpBaselineCalendar->get_FirstDayInThisMonth(&firstIndex));
    IFC(_tpBaselineCalendar->get_Day(&currentIndex));
    dayIndex = currentIndex - firstIndex;

Cleanup:
    RRETURN(hr);
}

// Clears everything and refreshes the helper objects. After that, generates and
// sets the itemssources to selectors.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::RefreshSetup()
{
    HRESULT hr = S_OK;

    // Since we will be clearing itemssources / selecteditems and putting new ones, selection changes will be fired from the
    // selectors. However, we do not want to process them as if the end user has purposefully changed the selection on a selector.
    PreventReactionToSelectionChange();

    // This will recalculate the startyear/endyear etc and will tell us if we have a valid range to generate sources.
    IFC(UpdateState());

    if (_hasValidYearRange)
    {
        INT32 yearIndex = 0;
        INT32 monthIndex = 0;
        INT32 dayIndex = 0;
        wf::DateTime date = {};

        // When we are refreshing all our setup, year selector should
        // also be refreshed.
        IFC(RefreshSourcesAndSetSelectedIndices(TRUE /*Refresh day */, TRUE /* Refresh month*/,TRUE /* Refresh year */ ));

        // If we refreshed our set up due to a property change, this may have caused us to coerce and change the current displayed date. For example
        // min/max year changes may have force us to coerce the current datetime to the nearest value inside the valid range.
        // So, we should update our DateTime property. If there is a change, we will end up firing the event as desired, if there isn't a change
        // we will just no_op.
        IFC(GetIndices(yearIndex, monthIndex, dayIndex));
        IFC(GetDateFromIndices(yearIndex, monthIndex, dayIndex, &date));
        IFC(SetDate(date));
    }

Cleanup:
    AllowReactionToSelectionChange();
    RRETURN(hr);
}

// Regenerate the itemssource for the day/month/yearpickers and select the appropriate indices that represent the current DateTime.
 // Depending on which field changes we might not need to refresh some of the sources.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::RefreshSourcesAndSetSelectedIndices(
    _In_ BOOLEAN refreshDay,
    _In_ BOOLEAN refreshMonth,
    _In_ BOOLEAN refreshYear)
{
    HRESULT hr = S_OK;
    INT32 yearIndex = 0;
    INT32 monthIndex = 0;
    INT32 dayIndex = 0;

    PreventReactionToSelectionChange();

    IFC(GetIndices(yearIndex, monthIndex, dayIndex));

    IFC(ClearSelectors(refreshDay, refreshMonth, refreshYear));

    if (_tpYearPicker.Get())
    {
        if (refreshYear)
        {
            IFC(GenerateYears());
            IFC(_tpYearPicker->put_Items(_tpYearSource.Get()));
        }
        IFC(_tpYearPicker->put_SelectedIndex(yearIndex));
    }

    if (_tpMonthPicker.Get())
    {
        if (refreshMonth)
        {
            IFC(GenerateMonths(yearIndex));
            IFC(_tpMonthPicker->put_Items(_tpMonthSource.Get()));
        }
        IFC(_tpMonthPicker->put_SelectedIndex(monthIndex));
    }

    if (_tpDayPicker.Get())
    {
        if(refreshDay)
        {
            IFC(GenerateDays(yearIndex, monthIndex));
            IFC(_tpDayPicker->put_Items(_tpDaySource.Get()));
        }
        IFC(_tpDayPicker->put_SelectedIndex(dayIndex));
    }

Cleanup:
    AllowReactionToSelectionChange();
    RRETURN(hr);
}

// Generate the collection that we will populate our year picker with.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::GenerateYears()
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strYear;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spPrimaryFormatter;
    wf::DateTime dateTime;

    IFC(GetYearFormatter(_calendarIdentifier.Get(), &spPrimaryFormatter));

    IFC(_tpYearSource->Clear());

    for (INT32 yearOffset = 0; yearOffset < _numberOfYears; yearOffset++)
    {
        wrl::ComPtr<IDatePickerFlyoutItem> spItem;
        wrl::ComPtr<IInspectable> spInspectable;

        IFC(_tpCalendar->SetDateTime(_startDate));
        IFC(_tpCalendar->AddYears(yearOffset));

        IFC(_tpCalendar->GetDateTime(&dateTime));

        IFC(wrl::MakeAndInitialize<DatePickerFlyoutItem>(&spItem));

        IFC(spPrimaryFormatter->Format(dateTime, strYear.ReleaseAndGetAddressOf()));
        IFC(spItem->put_PrimaryText(strYear.Get()));
        IFC(spItem->put_SecondaryText(wrl_wrappers::HStringReference(L"").Get()));

        IFC(spItem.As(&spInspectable));

        IFC(_tpYearSource->Append(spInspectable.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Generate the collection that we will populate our month picker with.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::GenerateMonths(
    _In_ INT32 yearOffset)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strMonth;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spPrimaryFormatter;
    wf::DateTime dateTime;
    INT32 monthOffset = 0;
    INT32 numberOfMonths = 0;
    INT32 firstMonthInThisYear = 0;

    IFC(GetMonthFormatter(_calendarIdentifier.Get(), &spPrimaryFormatter));

    IFC(_tpCalendar->SetDateTime(_startDate));
    IFC(_tpCalendar->AddYears(yearOffset));
    IFC(_tpCalendar->get_NumberOfMonthsInThisYear(&numberOfMonths));
    IFC(_tpCalendar->get_FirstMonthInThisYear(&firstMonthInThisYear));

    IFC(_tpMonthSource->Clear());

    for (monthOffset = 0; monthOffset < numberOfMonths; monthOffset++)
    {
        wrl::ComPtr<IDatePickerFlyoutItem> spItem;
        wrl::ComPtr<IInspectable> spInspectable;

        IFC(_tpCalendar->put_Month(firstMonthInThisYear));
        IFC(_tpCalendar->AddMonths(monthOffset));
        IFC(_tpCalendar->GetDateTime(&dateTime));

        IFC(wrl::MakeAndInitialize<DatePickerFlyoutItem>(&spItem));

        IFC(spPrimaryFormatter->Format(dateTime, strMonth.ReleaseAndGetAddressOf()));
        IFC(spItem->put_PrimaryText(strMonth.Get()));
        IFC(spItem.As(&spInspectable));

        IFC(_tpMonthSource->Append(spInspectable.Get()));
    }

Cleanup:
    RRETURN(hr);
}


// Generate the collection that we will populate our day picker with.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::GenerateDays(
    _In_ INT32 yearOffset,
    _In_ INT32 monthOffset)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strDay;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spPrimaryFormatter;
    wf::DateTime dateTime;
    INT32 dayOffset = 0;
    INT32 numberOfDays = 0;
    INT32 firstDayInThisMonth = 0;
    INT32 firstMonthInThisYear = 0;

    IFC(GetDayFormatter(_calendarIdentifier.Get(), &spPrimaryFormatter));

    IFC(_tpCalendar->SetDateTime(_startDate));
    IFC(_tpCalendar->AddYears(yearOffset));
    IFC(_tpCalendar->get_FirstMonthInThisYear(&firstMonthInThisYear));
    IFC(_tpCalendar->put_Month(firstMonthInThisYear));
    IFC(_tpCalendar->AddMonths(monthOffset));
    IFC(_tpCalendar->get_NumberOfDaysInThisMonth(&numberOfDays));
    IFC(_tpCalendar->get_FirstDayInThisMonth(&firstDayInThisMonth));

    IFC(_tpDaySource->Clear());

    for (dayOffset = 0; dayOffset < numberOfDays; dayOffset++)
    {
        wrl::ComPtr<IDatePickerFlyoutItem> spItem;
        wrl::ComPtr<IInspectable> spInspectable;

        IFC(_tpCalendar->put_Day(firstDayInThisMonth + dayOffset));
        IFC(_tpCalendar->GetDateTime(&dateTime));

        IFC(wrl::MakeAndInitialize<DatePickerFlyoutItem>(&spItem));

        IFC(spPrimaryFormatter->Format(dateTime, strDay.ReleaseAndGetAddressOf()));
        IFC(spItem->put_PrimaryText(strDay.Get()));

        IFC(spItem.As(&spInspectable));

        IFC(_tpDaySource->Append(spInspectable.Get()));
    }

Cleanup:
   RRETURN(hr);
}

// Reacts to change in selection of our selectors. Calculates the new date represented by the selected indices and updates the
// Date property.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::OnSelectorSelectionChanged(
    _In_ IInspectable* pSender,
    _In_ xaml_controls::ISelectionChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pArgs);
    if (IsReactionToSelectionChangeAllowed())
    {
        INT32 yearIndex = 0;
        INT32 monthIndex = 0;
        INT32 dayIndex = 0;
        wf::DateTime date = {};

        if (_tpYearPicker)
        {
            IFC(_tpYearPicker->get_SelectedIndex(&yearIndex));
        }

        if (_tpMonthPicker)
        {
            IFC(_tpMonthPicker->get_SelectedIndex(&monthIndex));
        }

        if (_tpDayPicker)
        {
            IFC(_tpDayPicker->get_SelectedIndex(&dayIndex));
        }

        IFC(GetDateFromIndices(yearIndex, monthIndex, dayIndex, &date));
        IFC(SetDate(date));
    }

Cleanup:
    RRETURN(hr);
}

// Interprets the selected indices of the selectors and creates and returns a DateTime corresponding to the date represented by these
// indices.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::GetDateFromIndices(
    _In_ INT32 yearIndex,
    _In_ INT32 monthIndex,
    _In_ INT32 dayIndex,
    _Out_ wf::DateTime* date)
{
    HRESULT hr = S_OK;
    wf::DateTime current = {};
    INT32 safeIndex = 0;
    INT32 firstIndex = 0;
    INT32 totalNumber = 0;
    INT32 period = 0;
    INT32 hour = 0;
    INT32 minute = 0;
    INT32 second = 0;
    INT32 nanosecond = 0;
    INT32 newYear = 0;
    INT32 newMonth = 0;
    INT32 previousYear = 0;
    INT32 previousMonth = 0;
    INT32 previousDay = 0;
    INT32 lastIndex = 0;

    current = ClampDate(_date, _startDate, _endDate);
    IFC(_tpCalendar->SetDateTime(current));
    // We want to preserve the time information. So we keep them around in order to prevent them overwritten by our sentinel time.
    IFC(_tpCalendar->get_Period(&period));
    IFC(_tpCalendar->get_Hour(&hour));
    IFC(_tpCalendar->get_Minute(&minute));
    IFC(_tpCalendar->get_Second(&second));
    IFC(_tpCalendar->get_Nanosecond(&nanosecond));
    IFC(_tpCalendar->get_Year(&previousYear));
    IFC(_tpCalendar->get_Month(&previousMonth));
    IFC(_tpCalendar->get_Day(&previousDay));

    IFC(_tpCalendar->SetDateTime(_startDate));
    IFC(_tpCalendar->put_Period(period));
    IFC(_tpCalendar->put_Hour(hour));
    IFC(_tpCalendar->put_Minute(minute));
    IFC(_tpCalendar->put_Second(second));
    IFC(_tpCalendar->put_Nanosecond(nanosecond));

    IFC(_tpCalendar->AddYears(yearIndex));
    IFC(_tpCalendar->get_Year(&newYear));

    IFC(_tpCalendar->get_FirstMonthInThisYear(&firstIndex));
    IFC(_tpCalendar->get_NumberOfMonthsInThisYear(&totalNumber));
    IFC(_tpCalendar->get_LastMonthInThisYear(&lastIndex));

    if (firstIndex > lastIndex)
    {
        if (monthIndex + firstIndex > totalNumber)
        {
            safeIndex = monthIndex + firstIndex - totalNumber;
        }
        else
        {
            safeIndex = monthIndex + firstIndex;
        }

        if (previousYear != newYear)
        {
            // Year has changed in some transitions in Thai Calendar, this will change the first month, and last month indices of the year.
            safeIndex = max(min(previousMonth, totalNumber), DATEPICKER_WRAP_AROUND_MONTHS_FIRST_INDEX);
        }
    }
    else
    {
        if (previousYear == newYear)
        {
            safeIndex = max(min(monthIndex + firstIndex, firstIndex + totalNumber - 1), firstIndex);
        }
        else
        {
            // Year has changed in some transitions in Thai Calendar, this will change the first month, and last month indices of the year.
            safeIndex = max(min(previousMonth, firstIndex + totalNumber - 1), firstIndex);
        }
    }

    IFC(_tpCalendar->put_Month(safeIndex));
    IFC(_tpCalendar->get_Month(&newMonth));

    IFC(_tpCalendar->get_FirstDayInThisMonth(&firstIndex));
    IFC(_tpCalendar->get_NumberOfDaysInThisMonth(&totalNumber));
    // We also need to coerce the day index into the safe range because a change in month or year may have changed the number of days
    // rendering our previous index invalid.
    safeIndex = max(min(dayIndex + firstIndex, firstIndex + totalNumber - 1), firstIndex);
    if(previousYear != newYear || previousMonth != newMonth)
    {
        safeIndex = max(min(previousDay, firstIndex + totalNumber - 1), firstIndex);
    }
    IFC(_tpCalendar->put_Day(safeIndex));

    IFC(_tpCalendar->GetDateTime(date));

Cleanup:
    RRETURN(hr);
}

// Reacts to the changes in string typed properties. Reverts the property value to the last valid value,
// if property change causes an exception.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::OnCalendarIdentifierPropertyChanged(
    _In_ HSTRING oldValue)
{
    HRESULT hr = S_OK;

    IFC(RefreshSetup());

Cleanup:
    if (FAILED(hr))
    {
        // revert the change
        _calendarIdentifier.Release();
        _calendarIdentifier.Attach(oldValue);
        IGNOREHR(SUCCEEDED(RefreshSetup()));
    }
    RRETURN(hr);
}

// Reacts to changes in Date property. Day may have changed programmatically or end user may have changed the
// selection of one of our selectors causing a change in Date.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::OnDateChanged(
    _In_ wf::DateTime oldValue,
    _In_ wf::DateTime newValue)
{
    HRESULT hr = S_OK;
    wf::DateTime clampedNewDate = {};
    wf::DateTime clampedOldDate = {};

    if (_hasValidYearRange)
    {
        INT32 newYear = 0;
        INT32 oldYear = 0;
        INT32 newMonth = 0;
        INT32 oldMonth = 0;
        BOOLEAN refreshMonth = FALSE;
        BOOLEAN refreshDay = FALSE;

        // The DateTime value set may be out of our acceptable range.
        clampedNewDate = ClampDate(newValue, _startDate, _endDate);
        clampedOldDate = ClampDate(oldValue, _startDate, _endDate);
        if (clampedNewDate.UniversalTime != newValue.UniversalTime)
        {
            // We need to coerce the date into the acceptable range. This will trigger another OnDateChanged which
            // will take care of executing the logic needed.
            IFC(SetDate(clampedNewDate));
            goto Cleanup;
        }

        if (clampedNewDate.UniversalTime == clampedOldDate.UniversalTime)
        {
            // It looks like we clamped an invalid date into an acceptable one, we need to refresh the sources.
            refreshMonth = TRUE;
            refreshDay = TRUE;
        }
        else
        {
            IFC(_tpCalendar->SetDateTime(clampedOldDate));
            IFC(_tpCalendar->get_Year(&oldYear));
            IFC(_tpCalendar->get_Month(&oldMonth));

            IFC(_tpCalendar->SetDateTime(clampedNewDate));
            IFC(_tpCalendar->get_Year(&newYear));
            IFC(_tpCalendar->get_Month(&newMonth));

            // Change in year will invalidate month and days.
            if (oldYear != newYear)
            {
                refreshMonth = TRUE;
                refreshDay = TRUE;
            }
            // Change in month will invalidate days.
            else if (oldMonth != newMonth)
            {
                refreshDay = TRUE;
            }
        }

        IFC(RefreshSourcesAndSetSelectedIndices(refreshDay, refreshMonth, FALSE));
    }

Cleanup:
    RRETURN(hr);
}

// Returns the cached DateTimeFormatter for the given Calendar - Format pair for generating the strings
// representing the years in our date range. If there isn't a cached DateTimeFormatter instance,
// creates one and caches it to be returned for the following calls with the same pair.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::GetYearFormatter(
    _In_ HSTRING strCalendarIdentifier,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppPrimaryDateTimeFormatter)
{
    HRESULT hr = S_OK;

    // We can only use the cached formatter if there is a cached formatter, cached formatter's format is the same as the new one's
    // and cached formatter's calendar identifier is the same as the new one's.
    if (!(_tpPrimaryYearFormatter.Get() &&
         strCalendarIdentifier  == _strYearCalendarIdentifier))
    {
        // We either do not have a cached formatter or it is stale. We need a create a new one and cache it along
        // with its identifying info.
        wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;

        _tpPrimaryYearFormatter.Clear();

        IFC(CreateNewFormatter(_yearFormat.Get(), strCalendarIdentifier, &spFormatter));
        IFC(SetPtrValue(_tpPrimaryYearFormatter, spFormatter.Get()));

        IFC(_strYearCalendarIdentifier.Set(strCalendarIdentifier));
    }

    _tpPrimaryYearFormatter.CopyTo(ppPrimaryDateTimeFormatter);

Cleanup:
    RRETURN(hr);
}

// Returns the cached DateTimeFormatter for the given Calendar - Format pair for generating the strings
// representing the months in our date range. If there isn't a cached DateTimeFormatter instance,
// creates one and caches it to be returned for the following calls with the same pair.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::GetMonthFormatter(
    _In_ HSTRING strCalendarIdentifier,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppPrimaryDateTimeFormatter)
{
    HRESULT hr = S_OK;

    // We can only use the cached formatter if there is a cached formatter, cached formatter's format is the same as the new one's
    // and cached formatter's calendar identifier is the same as the new one's.
    if (!(_tpPrimaryMonthFormatter.Get() && strCalendarIdentifier == _strMonthCalendarIdentifier))
    {
        // We either do not have a cached formatter or it is stale. We need a create a new one and cache it along
        // with its identifying info.
        wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;

        _tpPrimaryMonthFormatter.Clear();

        IFC(CreateNewFormatter(_monthFormat.Get(), strCalendarIdentifier, &spFormatter));
        IFC(SetPtrValue(_tpPrimaryMonthFormatter, spFormatter.Get()));

        IFC(_strMonthCalendarIdentifier.Set(strCalendarIdentifier));
    }

    _tpPrimaryMonthFormatter.CopyTo(ppPrimaryDateTimeFormatter);

Cleanup:
    RRETURN(hr);
}

// Returns the cached DateTimeFormatter for the given Calendar - Format pair for generating the strings
// representing the days in our date range. If there isn't a cached DateTimeFormatter instance,
// creates one and caches it to be returned for the following calls with the same pair.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::GetDayFormatter(
    _In_ HSTRING strCalendarIdentifier,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppPrimaryDateTimeFormatter)
{
    HRESULT hr = S_OK;

    // We can only use the cached formatter if there is a cached formatter, cached formatter's format is the same as the new one's
    // and cached formatter's calendar identifier is the same as the new one's.
    if (!(_tpPrimaryDayFormatter.Get() && strCalendarIdentifier == _strDayCalendarIdentifier))
    {
        // We either do not have a cached formatter or it is stale. We need a create a new one and cache it along
        // with its identifying info.
        wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;

        _tpPrimaryDayFormatter.Clear();

        IFC(CreateNewFormatter(_dayFormat.Get(), strCalendarIdentifier, &spFormatter));
        IFC(SetPtrValue(_tpPrimaryDayFormatter, spFormatter.Get()));
        IFC(_strDayCalendarIdentifier.Set(strCalendarIdentifier));
    }

    _tpPrimaryDayFormatter.CopyTo(ppPrimaryDateTimeFormatter);

Cleanup:
    RRETURN(hr);
}

// Creates a new DateTimeFormatter with the given parameters.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::CreateNewFormatter(
    _In_ HSTRING strFormat,
    _In_ HSTRING strCalendarIdentifier,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatterFactory> spFormatterFactory;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    wrl::ComPtr<wfc::IVectorView<HSTRING>> spLanguages;
    wrl::ComPtr<wfc::IIterable<HSTRING>> spLanguagesAsIterable;
    wrl_wrappers::HString strGeographicRegion;
    wrl_wrappers::HString strClock;

    IFCPTR(ppDateTimeFormatter);
    *ppDateTimeFormatter = nullptr;

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_DateTimeFormatting_DateTimeFormatter).Get(),
        &spFormatterFactory));
    IFCPTR(spFormatterFactory.Get());

    IFC(spFormatterFactory->CreateDateTimeFormatter(strFormat, spFormatter.ReleaseAndGetAddressOf()));

    IFC(spFormatter->get_GeographicRegion(strGeographicRegion.GetAddressOf()));
    IFC(spFormatter->get_Languages(spLanguages.ReleaseAndGetAddressOf()));
    IFC(spFormatter->get_Clock(strClock.GetAddressOf()));

    IFC(spLanguages.As(&spLanguagesAsIterable));

    IFC(spFormatterFactory->CreateDateTimeFormatterContext(
            strFormat,/* Format string */
            spLanguagesAsIterable.Get(), /* Languages*/
            strGeographicRegion.Get(), /* Geographic region */
            strCalendarIdentifier, /* Calendar */
            strClock.Get(), /* Clock */
            spFormatter.ReleaseAndGetAddressOf()));

    *ppDateTimeFormatter = spFormatter.Detach();

Cleanup:
    RRETURN(hr);
}

// Creates a new wg::Calendar, taking into account the Calendar Identifier
// represented by our public "Calendar" property.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::CreateNewCalendar(
    _In_ HSTRING strCalendarIdentifier,
    _Inout_ wrl::ComPtr<wg::ICalendar>* pspCalendar)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<wg::ICalendarFactory> spCalendarFactory;
    wrl::ComPtr<wg::ICalendar> spTemporaryCalendar;
    wrl::ComPtr<wfc::IVectorView<HSTRING>> spLanguages;
    wrl::ComPtr<wfc::IIterable<HSTRING>> spLanguagesAsIterable;
    wrl_wrappers::HString strClock;

    IFC(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Calendar).Get(),
        spTemporaryCalendar.ReleaseAndGetAddressOf()));

    IFC(spTemporaryCalendar->get_Languages(&spLanguages));
    IFC(spTemporaryCalendar->GetClock(strClock.GetAddressOf()));
    IFC(spLanguages.As(&spLanguagesAsIterable));

    //Create the calendar
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Calendar).Get(),
        &spCalendarFactory));

    IFC(spCalendarFactory->CreateCalendar(
        spLanguagesAsIterable.Get(), /* Languages*/
        strCalendarIdentifier, /* Calendar */
        strClock.Get(), /* Clock */
        pspCalendar->ReleaseAndGetAddressOf()));

Cleanup:
    RRETURN(hr);
}

// Given two calendars, finds the difference of years between them. Note that we are counting on the two
// calendars will have the same system.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::GetYearDifference(
    _In_ wg::ICalendar* pStartCalendar,
    _In_ wg::ICalendar* pEndCalendar,
    _Inout_ INT32& difference)
{
    HRESULT hr = S_OK;
    INT32 startEra = 0;
    INT32 endEra = 0;
    INT32 startYear = 0;
    INT32 endYear = 0;
    wrl_wrappers::HString strStartCalendarSystem;
    wrl_wrappers::HString strEndCalendarSystem;

    IFC(pStartCalendar->GetCalendarSystem(strStartCalendarSystem.GetAddressOf()));
    IFC(pEndCalendar->GetCalendarSystem(strEndCalendarSystem.GetAddressOf()));
    if(strStartCalendarSystem != strEndCalendarSystem)
    {
        IFC(E_FAIL);
    }

    difference = 0;

    // Get the eras and years of the calendars.
    IFC(pStartCalendar->get_Era(&startEra));
    IFC(pEndCalendar->get_Era(&endEra));

    IFC(pStartCalendar->get_Year(&startYear));
    IFC(pEndCalendar->get_Year(&endYear));

    while (startEra != endEra || startYear != endYear)
    {
        // Add years to start calendar until their eras and years both match.
        IFC(pStartCalendar->AddYears(1));
        difference++;
        IFC(pStartCalendar->get_Era(&startEra));
        IFC(pStartCalendar->get_Year(&startYear));
    }

Cleanup:
    RRETURN(hr);
}

// Clamps the given date within the range defined by the min and max dates. Note that it is caller's responsibility
// to feed appropriate min/max values that defines a valid date range.
wf::DateTime
DatePickerFlyoutPresenter::ClampDate(
    _In_ wf::DateTime date,
    _In_ wf::DateTime minDate,
    _In_ wf::DateTime maxDate)
{
    if (date.UniversalTime < minDate.UniversalTime)
    {
        return minDate;
    }
    else if (date.UniversalTime > maxDate.UniversalTime)
    {
        return maxDate;
    }
    return date;
}

// The order of date fields vary depending on geographic region, calendar type etc. This function determines the M/D/Y order using
// globalization APIs. It also determines whether the fields should be laid RTL.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::GetOrder(
    _Out_ INT32* yearOrder,
    _Out_ INT32* monthOrder,
    _Out_ INT32* dayOrder,
    _Out_ BOOLEAN* isRTL)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    wrl::ComPtr<wfc::IVectorView<HSTRING>> spPatterns;
    wrl_wrappers::HString strDate;

    // Default orderings.
    *yearOrder = 2;
    *monthOrder = 0;
    *dayOrder = 1;
    *isRTL = FALSE;

    IFC(CreateNewFormatter(
        wrl_wrappers::HStringReference(STR_LEN_PAIR(L"day month.full year")).Get(),
        _calendarIdentifier.Get(),
        &spFormatter));
    IFC(spFormatter->get_Patterns(&spPatterns));
    IFC(spPatterns->GetAt(0, strDate.GetAddressOf()));

    if (strDate.Get())
    {
        LPCWSTR szDate;
        UINT32 length = 0;
        LPCWSTR dayOccurence = nullptr;
        LPCWSTR monthOccurence = nullptr;
        LPCWSTR yearOccurence = nullptr;

        szDate = strDate.GetRawBuffer(&length);

        //The calendar is right-to-left if the first character of the pattern string is the rtl character
        *isRTL = szDate[0] == DATEPICKER_RTL_CHARACTER_CODE;

        // We do string search to determine the order of the fields.
        dayOccurence = wcsstr(szDate, L"{day");
        monthOccurence = wcsstr(szDate, L"{month");
        yearOccurence = wcsstr(szDate, L"{year");

        if (dayOccurence < monthOccurence)
        {
            if (dayOccurence < yearOccurence)
            {
                *dayOrder = 0;
                if (monthOccurence < yearOccurence)
                {
                    *monthOrder = 1;
                    *yearOrder = 2;
                }
                else
                {
                    *monthOrder = 2;
                    *yearOrder = 1;
                }
            }
            else
            {
                *dayOrder = 1;
                *monthOrder = 2;
                *yearOrder = 0;
            }
        }
        else
        {
            if (dayOccurence < yearOccurence)
            {
                *dayOrder = 1;
                *monthOrder = 0;
                *yearOrder = 2;
            }
            else
            {
                *dayOrder = 2;
                if (monthOccurence < yearOccurence)
                {
                    *monthOrder = 0;
                    *yearOrder = 1;
                }
                else
                {

                    *monthOrder = 1;
                    *yearOrder = 0;
                }
            }
        }

    }

Cleanup:
    RRETURN(hr);
}

// Updates the order of selectors in our layout. Also takes care of hiding/showing the comboboxes and related spacing depending our
// public properties set by the user.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::UpdateOrderAndLayout()
{
    HRESULT hr = S_OK;
    INT32 yearOrder = 0;
    INT32 monthOrder = 0;
    INT32 dayOrder = 0;
    BOOLEAN isRTL = FALSE;
    BOOLEAN firstHostPopulated = FALSE;
    BOOLEAN secondHostPopulated = FALSE;
    BOOLEAN thirdHostPopulated = FALSE;
    BOOLEAN columnIsFound = FALSE;
    UINT32 columnIndex = 0;
    wrl::ComPtr<wfc::IVector<xaml_controls::ColumnDefinition*>> spColumns;
    wrl::ComPtr<xaml_controls::IColumnDefinition> firstPickerHostColumn = nullptr;
    wrl::ComPtr<xaml_controls::IColumnDefinition> secondPickerHostColumn = nullptr;
    wrl::ComPtr<xaml_controls::IColumnDefinition> thirdPickerHostColumn = nullptr;
    wrl::ComPtr<xaml_controls::IGridStatics> spGridStatics;
    wrl::ComPtr<xaml::IFrameworkElement> spFrameworkElement;
    wrl::ComPtr<xaml::IUIElement> spUIElement;
    wrl::ComPtr<xaml_controls::IControl> firstPickerAsControl;
    wrl::ComPtr<xaml_controls::IControl> secondPickerAsControl;
    wrl::ComPtr<xaml_controls::IControl> thirdPickerAsControl;

    _tpFirstPickerAsControl.Clear();
    _tpSecondPickerAsControl.Clear();
    _tpThirdPickerAsControl.Clear();

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Grid).Get(),
        &spGridStatics));

    IFC(GetOrder(&yearOrder, &monthOrder, &dayOrder, &isRTL));

    // Some of the Calendars are RTL (Hebrew, Um Al Qura) we need to change the flow direction of DatePicker to accommodate these
    // calendars.
    if (_tpContentPanel.Get())
    {
        IFC(_tpContentPanel->put_FlowDirection(isRTL ?
            xaml::FlowDirection_RightToLeft : xaml::FlowDirection_LeftToRight));
    }

    // Clear the children of hosts first, so we never risk putting one picker in two hosts and failing.
    if (_tpFirstPickerHost.Get())
    {
        IFC(_tpFirstPickerHost->put_Child(nullptr));
    }
    if (_tpSecondPickerHost.Get())
    {
        IFC(_tpSecondPickerHost->put_Child(nullptr));
    }
    if (_tpThirdPickerHost.Get())
    {
        IFC(_tpThirdPickerHost->put_Child(nullptr));
    }


    // Clear the columns of the grid first. We will re-add the columns that we need further down.
    if (_tpPickerHostGrid)
    {
        IFC(_tpPickerHostGrid->get_ColumnDefinitions(&spColumns));
        IFC(spColumns->Clear());
    }

    // Assign the selectors to the hosts, if the selector is not shown, we will not put the selector inside the related hosts. Note that we
    // could have just collapsed selector or its host to accomplish hiding, however, we decided not to put the hidden fields to already
    // crowded visual tree.
    switch (yearOrder)
    {
        case 0:
            if (_tpFirstPickerHost.Get() && _tpYearPicker.Get() && _yearVisible)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpYearPicker.As(&spPickerAsUI));
                IFC(_tpFirstPickerHost->put_Child(spPickerAsUI.Get()));
                firstHostPopulated = TRUE;
            }
            else if (_tpYearColumn && _tpYearPicker && _yearVisible)
            {
                firstHostPopulated = TRUE;
                firstPickerHostColumn = _tpYearColumn.Get();
            }

            if (firstHostPopulated)
            {
                _tpYearPicker.As(&firstPickerAsControl);
            }
            break;
        case 1:
            if (_tpSecondPickerHost.Get() && _tpYearPicker.Get() && _yearVisible)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpYearPicker.As(&spPickerAsUI));
                IFC(_tpSecondPickerHost->put_Child(spPickerAsUI.Get()));
                secondHostPopulated = TRUE;
            }
            else if (_tpYearColumn && _tpYearPicker && _yearVisible)
            {
                secondHostPopulated = TRUE;
                secondPickerHostColumn = _tpYearColumn.Get();
            }

            if (secondHostPopulated)
            {
                IFC(_tpYearPicker.As(&secondPickerAsControl));
            }
            break;
        case 2:
            if (_tpThirdPickerHost.Get() && _tpYearPicker.Get() && _yearVisible)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpYearPicker.As(&spPickerAsUI));
                IFC(_tpThirdPickerHost->put_Child(spPickerAsUI.Get()));
                thirdHostPopulated = TRUE;
            }
            else if (_tpYearColumn && _tpYearPicker && _yearVisible)
            {
                thirdHostPopulated = TRUE;
                thirdPickerHostColumn = _tpYearColumn.Get();
            }

            if (thirdHostPopulated)
            {
                IFC(_tpYearPicker.As(&thirdPickerAsControl));
            }
            break;
    }

    switch (monthOrder)
    {
        case 0:
            if (_tpFirstPickerHost.Get() && _tpMonthPicker.Get() && _monthVisible)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpMonthPicker.As(&spPickerAsUI));
                IFC(_tpFirstPickerHost->put_Child(spPickerAsUI.Get()));
                firstHostPopulated = TRUE;
            }
            else if (_tpMonthColumn && _tpMonthPicker && _monthVisible)
            {
                firstHostPopulated = TRUE;
                firstPickerHostColumn = _tpMonthColumn.Get();
            }

            if (firstHostPopulated)
            {
                IFC(_tpMonthPicker.As(&firstPickerAsControl));
            }
            break;
        case 1:
            if (_tpSecondPickerHost.Get() && _tpMonthPicker.Get() && _monthVisible)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpMonthPicker.As(&spPickerAsUI));
                IFC(_tpSecondPickerHost->put_Child(spPickerAsUI.Get()));
                secondHostPopulated = TRUE;
            }
            else if (_tpMonthColumn && _tpMonthPicker && _monthVisible)
            {
                secondHostPopulated = TRUE;
                secondPickerHostColumn = _tpMonthColumn.Get();
            }

            if (secondHostPopulated)
            {
                IFC(_tpMonthPicker.As(&secondPickerAsControl));
            }
            break;
        case 2:
            if (_tpThirdPickerHost.Get() && _tpMonthPicker.Get() && _monthVisible)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpMonthPicker.As(&spPickerAsUI));
                IFC(_tpThirdPickerHost->put_Child(spPickerAsUI.Get()));
                thirdHostPopulated = TRUE;
            }
            else if (_tpMonthColumn && _tpMonthPicker && _monthVisible)
            {
                thirdHostPopulated = TRUE;
                thirdPickerHostColumn = _tpMonthColumn.Get();
            }

            if (thirdHostPopulated)
            {
                IFC(_tpMonthPicker.As(&thirdPickerAsControl));
            }
            break;
    }

    switch (dayOrder)
    {
        case 0:
            if (_tpFirstPickerHost.Get() && _tpDayPicker.Get() && _dayVisible)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpDayPicker.As(&spPickerAsUI));
                IFC(_tpFirstPickerHost->put_Child(spPickerAsUI.Get()));
                firstHostPopulated = TRUE;
            }
            else if (_tpDayColumn && _tpDayPicker && _dayVisible)
            {
                firstHostPopulated = TRUE;
                firstPickerHostColumn = _tpDayColumn.Get();
            }

            if (firstHostPopulated)
            {
                IFC(_tpDayPicker.As(&firstPickerAsControl));
            }
            break;
        case 1:
            if (_tpSecondPickerHost.Get() && _tpDayPicker.Get() && _dayVisible)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpDayPicker.As(&spPickerAsUI));
                IFC(_tpSecondPickerHost->put_Child(spPickerAsUI.Get()));
                secondHostPopulated = TRUE;
            }
            else if (_tpDayColumn && _tpDayPicker && _dayVisible)
            {
                secondHostPopulated = TRUE;
                secondPickerHostColumn = _tpDayColumn.Get();
            }

            if (secondHostPopulated)
            {
                IFC(_tpDayPicker.As(&secondPickerAsControl));
            }
            break;
        case 2:
            if (_tpThirdPickerHost.Get() && _tpDayPicker.Get() && _dayVisible)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpDayPicker.As(&spPickerAsUI));
                IFC(_tpThirdPickerHost->put_Child(spPickerAsUI.Get()));
                thirdHostPopulated = TRUE;
            }
            else if (_tpDayColumn && _tpDayPicker && _dayVisible)
            {
                thirdHostPopulated = TRUE;
                thirdPickerHostColumn = _tpDayColumn.Get();
            }

            if (thirdHostPopulated)
            {
                IFC(_tpDayPicker.As(&thirdPickerAsControl));
            }
            break;
    }

    IFC(SetPtrValue(_tpFirstPickerAsControl, firstPickerAsControl.Get()));
    IFC(SetPtrValue(_tpSecondPickerAsControl, secondPickerAsControl.Get()));
    IFC(SetPtrValue(_tpThirdPickerAsControl, thirdPickerAsControl.Get()));

    // Add the columns to the grid in the correct order (as computed in the switch statement above).
    if (spColumns)
    {
        if (firstPickerHostColumn)
        {
            spColumns->Append(firstPickerHostColumn.Get());
        }

        if (_tpFirstSpacerColumn)
        {
            spColumns->Append(_tpFirstSpacerColumn.Get());
        }

        if (secondPickerHostColumn)
        {
            spColumns->Append(secondPickerHostColumn.Get());
        }

        if (_tpSecondSpacerColumn)
        {
            spColumns->Append(_tpSecondSpacerColumn.Get());
        }

        if (thirdPickerHostColumn)
        {
            spColumns->Append(thirdPickerHostColumn.Get());
        }
    }

    // Set the Grid.Column property on the Day/Month/Year TextBlocks to the index of the matching ColumnDefinition
    // e.g. YearTextBlock Grid.Column = columns.IndexOf(YearColumn)
    if (_tpYearPicker && _tpYearColumn && _yearVisible && spColumns)
    {
        IFC(spColumns->IndexOf(_tpYearColumn.Get(), &columnIndex, &columnIsFound));
        ASSERT(columnIsFound);
        IFC(_tpYearPicker.As(&spFrameworkElement));
        IFC(spGridStatics->SetColumn(spFrameworkElement.Get(), columnIndex));
    }
    if (_tpMonthPicker && _tpMonthColumn && _monthVisible && spColumns)
    {
        IFC(spColumns->IndexOf(_tpMonthColumn.Get(), &columnIndex, &columnIsFound));
        ASSERT(columnIsFound);
        IFC(_tpMonthPicker.As(&spFrameworkElement));
        IFC(spGridStatics->SetColumn(spFrameworkElement.Get(), columnIndex));
    }
    if (_tpDayPicker && _tpDayColumn && _dayVisible && spColumns)
    {
        IFC(spColumns->IndexOf(_tpDayColumn.Get(), &columnIndex, &columnIsFound));
        ASSERT(columnIsFound);
        IFC(_tpDayPicker.As(&spFrameworkElement));
        IFC(spGridStatics->SetColumn(spFrameworkElement.Get(), columnIndex));
    }

    // Collapse the Day/Month/Year LoopingSelectors if DayVisible/MonthVisible/YearVisible are false.
    // Set the TabIndex property on the LoopingSelectors to match the day/month/year order.
    if (_tpDayPicker)
    {
        IFC(_tpDayPicker.As(&spUIElement));
        IFC(spUIElement->put_Visibility(_dayVisible ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
        IFC(spUIElement->put_TabIndex(dayOrder));
    }
    if (_tpMonthPicker)
    {
        IFC(_tpMonthPicker.As(&spUIElement));
        IFC(spUIElement->put_Visibility(_monthVisible ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
        IFC(spUIElement->put_TabIndex(monthOrder));
    }
    if (_tpYearPicker)
    {
        IFC(_tpYearPicker.As(&spUIElement));
        IFC(spUIElement->put_Visibility(_yearVisible ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
        IFC(spUIElement->put_TabIndex(yearOrder));
    }

    // Determine if we will show the spacings and assign visibilities to spacing holders. We will determine if the spacings
    // are shown by looking at which borders/columns are populated.
    // Also move the spacers to the correct column.
    if (_tpFirstPickerSpacing.Get())
    {
        IFC(_tpFirstPickerSpacing.As(&spFrameworkElement));
        IFC(_tpFirstPickerSpacing.As(&spUIElement));
        IFC(spUIElement->put_Visibility(
            firstHostPopulated && (secondHostPopulated || thirdHostPopulated) ?
            xaml::Visibility_Visible : xaml::Visibility_Collapsed));
        if (_tpFirstSpacerColumn && spColumns)
        {
            IFC(spColumns->IndexOf(_tpFirstSpacerColumn.Get(), &columnIndex, &columnIsFound));
            ASSERT(columnIsFound);
            IFC(spGridStatics->SetColumn(spFrameworkElement.Get(), columnIndex));
        }
    }
    if (_tpSecondPickerSpacing.Get())
    {
        IFC(_tpSecondPickerSpacing.As(&spFrameworkElement));
        IFC(_tpSecondPickerSpacing.As(&spUIElement));
        IFC(spUIElement->put_Visibility(
            secondHostPopulated && thirdHostPopulated ?
            xaml::Visibility_Visible : xaml::Visibility_Collapsed));
        if (_tpSecondSpacerColumn && spColumns)
        {
            IFC(spColumns->IndexOf(_tpSecondSpacerColumn.Get(), &columnIndex, &columnIsFound));
            ASSERT(columnIsFound);
            IFC(spGridStatics->SetColumn(spFrameworkElement.Get(), columnIndex));
        }
    }

Cleanup:
    RRETURN(hr);
}

// We execute our logic depending on some state information such as start date, end date, number of years etc. These state
// variables need to be updated whenever a public property change occurs which affects them.
_Check_return_
HRESULT
DatePickerFlyoutPresenter::UpdateState()
{
    HRESULT hr = S_OK;
    INT32 month = 0;
    INT32 day = 0;
    wf::DateTime minYearDate = {};
    wf::DateTime maxYearDate = {};
    wf::DateTime maxCalendarDate = {};
    wf::DateTime minCalendarDate = {};

    wrl::ComPtr<wg::ICalendar> spCalendar;
    wrl::ComPtr<wg::ICalendar> spBaselineCalendar;

    // Create a calendar with the the current CalendarIdentifier
    IFC(CreateNewCalendar(_calendarIdentifier.Get(), &spCalendar));
    IFC(CreateNewCalendar(_calendarIdentifier.Get(), &spBaselineCalendar));

    IFC(SetPtrValue(_tpCalendar, spCalendar.Get()));
    IFC(SetPtrValue(_tpBaselineCalendar, spBaselineCalendar.Get()));

    // We do not have a valid range if our MinYear is later than our MaxYear
    _hasValidYearRange = _minYear.UniversalTime <= _maxYear.UniversalTime;

    if (_hasValidYearRange)
    {
        // Find the earliest and latest dates available for this calendar.
        IFC(_tpCalendar->SetToMin());
        IFC(_tpCalendar->GetDateTime(&minCalendarDate));

        //Find the latest date available for this calendar.
        IFC(_tpCalendar->SetToMax());
        IFC(_tpCalendar->GetDateTime(&maxCalendarDate));

        minYearDate = ClampDate(_minYear, minCalendarDate, maxCalendarDate);
        maxYearDate = ClampDate(_maxYear, minCalendarDate, maxCalendarDate);

        // Since we only care about the year field of minYearDate and maxYearDate we will change other fields into first day and last day
        // of the year respectively.
        IFC(_tpCalendar->SetDateTime(minYearDate));
        IFC(_tpCalendar->get_FirstMonthInThisYear(&month));
        IFC(_tpCalendar->put_Month(month));
        IFC(_tpCalendar->get_FirstDayInThisMonth(&day));
        IFC(_tpCalendar->put_Day(day));
        IFC(_tpCalendar->GetDateTime(&minYearDate));

        IFC(_tpCalendar->SetDateTime(maxYearDate));
        IFC(_tpCalendar->get_LastMonthInThisYear(&month));
        IFC(_tpCalendar->put_Month(month));
        IFC(_tpCalendar->get_LastDayInThisMonth(&day));
        IFC(_tpCalendar->put_Day(day));
        IFC(_tpCalendar->GetDateTime(&maxYearDate));

        IFC(_tpCalendar->SetDateTime(minYearDate));
        //Set our sentinel time to the start date as we will be using it while generating item sources, we do not need to do this for end date
        IFC(_tpCalendar->put_Hour(DATEPICKER_SENTINELTIME_HOUR));
        IFC(_tpCalendar->put_Minute(DATEPICKER_SENTINELTIME_MINUTE));
        IFC(_tpCalendar->put_Second(DATEPICKER_SENTINELTIME_SECOND));
        IFC(_tpCalendar->GetDateTime(&_startDate));
        _endDate = maxYearDate;

        // Find the number of years in our range
        IFC(_tpCalendar->SetDateTime(_startDate));
        IFC(_tpBaselineCalendar->SetDateTime(_endDate));

        IFC(GetYearDifference(_tpCalendar.Get(), _tpBaselineCalendar.Get(), _numberOfYears));
        _numberOfYears++; //since we should include both start and end years
    }
    else
    {
        // We do not want to display anything if we do not have a valid year range
        IFC(ClearSelectors(TRUE /*Clear day*/, TRUE /*Clear month*/, TRUE/*Clear year*/));
    }

    IFC(UpdateOrderAndLayout());

Cleanup:
    RRETURN(hr);
}

#pragma endregion

} } } } XAML_ABI_NAMESPACE_END