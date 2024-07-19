// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ElevationHelper.h"
#include <windows.globalization.datetimeformatting.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

// This is July 15th, 2011 as our sentinel date. There are no known
//  daylight savings transitions that happened on that date.
#define TIMEPICKER_SENTINELDATE_YEAR 2011
#define TIMEPICKER_SENTINELDATE_MONTH 7
#define TIMEPICKER_SENTINELDATE_DAY 15
#define TIMEPICKER_SENTINELDATE_TIMEFIELDS 0
#define TIMEPICKER_SENTINELDATE_HOUR12 12
#define TIMEPICKER_SENTINELDATE_HOUR24 0
#define TIMEPICKER_COERCION_INDEX 0
#define TIMEPICKER_AM_INDEX 0
#define TIMEPICKER_PM_INDEX 1
#define TIMEPICKER_RTL_CHARACTER_CODE 8207
#define TIMEPICKER_MINUTEINCREMENT_MIN 0
#define TIMEPICKER_MINUTEINCREMENT_MAX 59

// When the minute increment is set to 0, we want to only have 00 at the minute picker. This
// can be easily obtained by treating 0 as 60 with our existing logic. So during our logic, if we see
// that minute increment is zero we will use 60 in our calculations.
#define TIMEPICKER_MINUTEINCREMENT_ZERO_REPLACEMENT 60

const WCHAR TimePickerFlyoutPresenter::_hourLoopingSelectorAutomationId[] = L"HourLoopingSelector";
const WCHAR TimePickerFlyoutPresenter::_minuteLoopingSelectorAutomationId[] = L"MinuteLoopingSelector";
const WCHAR TimePickerFlyoutPresenter::_periodLoopingSelectorAutomationId[] = L"PeriodLoopingSelector";

const WCHAR* TimePickerFlyoutPresenter::_strTwelveHourClock = L"12HourClock";
const WCHAR* TimePickerFlyoutPresenter::_strHourFormat = L"{hour.integer(1)}";
const WCHAR* TimePickerFlyoutPresenter::_strMinuteFormat = L"{minute.integer(2)}";
const WCHAR* TimePickerFlyoutPresenter::_strPeriodFormat = L"{period.abbreviated(2)}";
const INT64 TimePickerFlyoutPresenter::_timeSpanTicksPerMinute = 600000000;
const INT64 TimePickerFlyoutPresenter::_timeSpanTicksPerHour = 36000000000;
const INT64 TimePickerFlyoutPresenter::_timeSpanTicksPerDay = 864000000000;
const INT32 TimePickerFlyoutPresenter::_periodCoercionOffset = 12;
const WCHAR TimePickerFlyoutPresenter::_firstPickerHostName[] = L"FirstPickerHost";
const WCHAR TimePickerFlyoutPresenter::_secondPickerHostName[] = L"SecondPickerHost";
const WCHAR TimePickerFlyoutPresenter::_thirdPickerHostName[] = L"ThirdPickerHost";
const WCHAR TimePickerFlyoutPresenter::_backgroundName[] = L"Background";
const WCHAR TimePickerFlyoutPresenter::_contentPanelName[] = L"ContentPanel";
const WCHAR TimePickerFlyoutPresenter::_titlePresenterName[] = L"TitlePresenter";

TimePickerFlyoutPresenter::TimePickerFlyoutPresenter()
    :_is12HourClock(FALSE),
     _reactionToSelectionChangeAllowed(FALSE),
     _minuteIncrement(0),
     _time(wf::TimeSpan()),
    _acceptDismissButtonsVisible(true)
{}

_Check_return_
HRESULT
TimePickerFlyoutPresenter::InitializeImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IControlFactory> spInnerFactory;
    wrl::ComPtr<xaml_controls::IControl> spInnerInstance;
    wrl::ComPtr<IInspectable> spInnerInspectable;

    IFC(TimePickerFlyoutPresenterGenerated::InitializeImpl());

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Control).Get(),
        &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IInspectable*>(static_cast<ITimePickerFlyoutPresenter*>(this)),
        &spInnerInspectable,
        &spInnerInstance));

    IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

    IFC(Private::SetDefaultStyleKey(
            spInnerInspectable.Get(),
            L"Microsoft.UI.Xaml.Controls.TimePickerFlyoutPresenter"));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
TimePickerFlyoutPresenter::OnApplyTemplateImpl()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml_controls::IControlProtected> spControlProtected;

    wrl::ComPtr<xaml_controls::ITextBlock> spTitlePresenter;
    wrl::ComPtr<xaml_controls::IBorder> spFirstPickerHost;
    wrl::ComPtr<xaml_controls::IBorder> spSecondPickerHost;
    wrl::ComPtr<xaml_controls::IBorder> spThirdPickerHost;
    wrl::ComPtr<xaml_controls::IBorder> spBackgroundBorder;
    wrl::ComPtr<IFrameworkElement> spContentPanel;
    wrl::ComPtr<IColumnDefinition> spFirstPickerHostColumn;
    wrl::ComPtr<IColumnDefinition> spSecondPickerHostColumn;
    wrl::ComPtr<IColumnDefinition> spThirdPickerHostColumn;
    wrl::ComPtr<IUIElement> spFirstPickerSpacing;
    wrl::ComPtr<IUIElement> spSecondPickerSpacing;
    wrl::ComPtr<IUIElement> spAcceptDismissHostGrid;
    wrl::ComPtr<IUIElement> spAcceptButton;
    wrl::ComPtr<IUIElement> spDismissButton;
    xaml::Thickness itemPadding;

    if (_tpMinutePicker)
    {
        IFC(_tpMinutePicker->remove_SelectionChanged(_minuteSelectionChangedToken));
    }

    if (_tpHourPicker)
    {
        IFC(_tpHourPicker->remove_SelectionChanged(_hourSelectionChangedToken));
    }

    if (_tpPeriodPicker)
    {
        IFC(_tpPeriodPicker->remove_SelectionChanged(_periodSelectionChangedToken));
    }

    _tpTitlePresenter.Clear();
    _tpMinutePicker.Clear();
    _tpHourPicker.Clear();
    _tpPeriodPicker.Clear();
    _tpFirstPickerHost.Clear();
    _tpSecondPickerHost.Clear();
    _tpThirdPickerHost.Clear();
    _tpBackgroundBorder.Clear();
    _tpContentPanel.Clear();
    _tpAcceptDismissHostGrid.Clear();
    _tpAcceptButton.Clear();
    _tpDismissButton.Clear();

    IFC(QueryInterface(__uuidof(xaml_controls::IControlProtected), &spControlProtected));
    IFC(TimePickerFlyoutPresenterGenerated::OnApplyTemplateImpl());

    IFC(Private::AttachTemplatePart<xaml_controls::IBorder>(
        spControlProtected.Get(),
        _backgroundName,
        &spBackgroundBorder));
    IFC(SetPtrValue(_tpBackgroundBorder, spBackgroundBorder.Get()));

    IFC(Private::AttachTemplatePart<xaml_controls::ITextBlock>(
        spControlProtected.Get(),
        _titlePresenterName,
        &spTitlePresenter));

    IFC(SetPtrValue(_tpTitlePresenter, spTitlePresenter.Get()));
    if (_tpTitlePresenter)
    {
        wrl::ComPtr<xaml::IUIElement> spPresenterAsUI;
        IFC(_tpTitlePresenter.As(&spPresenterAsUI));
        IFC(spPresenterAsUI->put_Visibility(_title.IsEmpty() ? xaml::Visibility_Collapsed : xaml::Visibility_Visible));
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
        L"FirstPickerHostColumn",
        &spFirstPickerHostColumn));
    IFC(SetPtrValue(_tpFirstPickerHostColumn, spFirstPickerHostColumn.Get()));

    IFC(Private::AttachTemplatePart<xaml_controls::IColumnDefinition>(
        spControlProtected.Get(),
        L"SecondPickerHostColumn",
        &spSecondPickerHostColumn));
    IFC(SetPtrValue(_tpSecondPickerHostColumn, spSecondPickerHostColumn.Get()));

    IFC(Private::AttachTemplatePart<xaml_controls::IColumnDefinition>(
        spControlProtected.Get(),
        L"ThirdPickerHostColumn",
        &spThirdPickerHostColumn));
    IFC(SetPtrValue(_tpThirdPickerHostColumn, spThirdPickerHostColumn.Get()));

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

    INT32 itemHeight;
    double itemHeightFromMarkup;
    if (SUCCEEDED(Private::ApplicationResourceHelpers::GetApplicationResource(
        wrl_wrappers::HStringReference(L"TimePickerFlyoutPresenterItemHeight").Get(),
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
        wrl_wrappers::HStringReference(L"TimePickerFlyoutPresenterItemPadding").Get(),
        &itemPadding)))
    {
        itemPadding = { 0, 3, 0, 5 };
    }

    if (_tpFirstPickerHost)
    {
        wrl::ComPtr<xaml::IUIElement> spLSAsUI;
        wrl::ComPtr<xaml::Controls::IControl> spLSAsControl;
        wrl::ComPtr<xaml_primitives::ILoopingSelector> spHourPicker;

        IFC(wrl::MakeAndInitialize<xaml_primitives::LoopingSelector>(&spHourPicker));
        IFC(SetPtrValue(_tpHourPicker, spHourPicker.Get()));

        IFC(spHourPicker.As(&spLSAsUI));
        IFC(spHourPicker.As(&spLSAsControl));

        //Don't set ItemWidth. We want the item to size to the width of its parent.
        IFC(spHourPicker->put_ItemHeight(itemHeight));
        IFC(spLSAsControl->put_Padding(itemPadding));
        IFC(spLSAsControl->put_HorizontalContentAlignment(xaml::HorizontalAlignment_Center));

        IFC(_tpFirstPickerHost->put_Child(spLSAsUI.Get()));
    }

    if (_tpSecondPickerHost)
    {
        wrl::ComPtr<xaml::IUIElement> spLSAsUI;
        wrl::ComPtr<xaml::Controls::IControl> spLSAsControl;
        wrl::ComPtr<xaml_primitives::ILoopingSelector> spMinutePicker;

        IFC(wrl::MakeAndInitialize<xaml_primitives::LoopingSelector>(&spMinutePicker));
        IFC(SetPtrValue(_tpMinutePicker, spMinutePicker.Get()));

        IFC(spMinutePicker.As(&spLSAsUI));
        IFC(spMinutePicker.As(&spLSAsControl));

        //Don't set ItemWidth. We want the item to size to the width of its parent.
        IFC(spMinutePicker->put_ItemHeight(itemHeight));
        IFC(spLSAsControl->put_Padding(itemPadding));
        IFC(spLSAsControl->put_HorizontalContentAlignment(xaml::HorizontalAlignment_Center));

        IFC(_tpSecondPickerHost->put_Child(spLSAsUI.Get()));
    }

    if (_tpThirdPickerHost)
    {
        wrl::ComPtr<xaml::IUIElement> spLSAsUI;
        wrl::ComPtr<xaml::Controls::IControl> spLSAsControl;
        wrl::ComPtr<xaml_primitives::ILoopingSelector> spPeriodPicker;

        IFC(wrl::MakeAndInitialize<xaml_primitives::LoopingSelector>(&spPeriodPicker));
        IFC(SetPtrValue(_tpPeriodPicker, spPeriodPicker.Get()));

        IFC(spPeriodPicker.As(&spLSAsUI));
        IFC(spPeriodPicker.As(&spLSAsControl));

        IFC(spPeriodPicker->put_ShouldLoop(false));

        IFC(spPeriodPicker->put_ItemHeight(itemHeight));
        IFC(spLSAsControl->put_Padding(itemPadding));
        IFC(spLSAsControl->put_HorizontalContentAlignment(xaml::HorizontalAlignment_Center));

        IFC(_tpThirdPickerHost->put_Child(spLSAsUI.Get()));
    }

    if (_tpHourPicker)
    {
        wrl::ComPtr<xaml::IDependencyObject> spHourPickerAsDO;
        wrl_wrappers::HString localizedName;

        IFC(_tpHourPicker->add_SelectionChanged(
            wrl::Callback<xaml_controls::ISelectionChangedEventHandler>
            (this, &TimePickerFlyoutPresenter::OnSelectorSelectionChanged).Get(),
            &_hourSelectionChangedToken));

        IFC(_tpHourPicker.As(&spHourPickerAsDO));

        IFC(Private::FindStringResource(
            UIA_AP_TIMEPICKER_HOURNAME,
            localizedName.ReleaseAndGetAddressOf()));
        IFC(Private::AutomationHelper::SetElementAutomationName(
            spHourPickerAsDO.Get(),
            localizedName.Get()));

        IFC(Private::AutomationHelper::SetElementAutomationId(
            spHourPickerAsDO.Get(),
            wrl_wrappers::HStringReference(_hourLoopingSelectorAutomationId).Get()));
    }
    if (_tpMinutePicker)
    {
        wrl::ComPtr<xaml::IDependencyObject> spMinutePickerAsDO;
        wrl_wrappers::HString localizedName;

        IFC(_tpMinutePicker->add_SelectionChanged(
            wrl::Callback<xaml_controls::ISelectionChangedEventHandler>
            (this, &TimePickerFlyoutPresenter::OnSelectorSelectionChanged).Get(),
            &_minuteSelectionChangedToken));

        IFC(_tpMinutePicker.As(&spMinutePickerAsDO));

        IFC(Private::FindStringResource(
            UIA_AP_TIMEPICKER_MINUTENAME,
            localizedName.ReleaseAndGetAddressOf()));
        IFC(Private::AutomationHelper::SetElementAutomationName(
            spMinutePickerAsDO.Get(),
            localizedName.Get()));

        IFC(Private::AutomationHelper::SetElementAutomationId(
            spMinutePickerAsDO.Get(),
            wrl_wrappers::HStringReference(_minuteLoopingSelectorAutomationId).Get()));
    }
    if (_tpPeriodPicker)
    {
        wrl::ComPtr<xaml::IDependencyObject> spPeriodPickerAsDO;
        wrl_wrappers::HString localizedName;

        IFC(_tpPeriodPicker->add_SelectionChanged(
            wrl::Callback<xaml_controls::ISelectionChangedEventHandler>
            (this, &TimePickerFlyoutPresenter::OnSelectorSelectionChanged).Get(),
            &_periodSelectionChangedToken));

        IFC(_tpPeriodPicker.As(&spPeriodPickerAsDO));

        IFC(Private::FindStringResource(
            UIA_AP_TIMEPICKER_PERIODNAME,
            localizedName.ReleaseAndGetAddressOf()));
        IFC(Private::AutomationHelper::SetElementAutomationName(
            spPeriodPickerAsDO.Get(),
            localizedName.Get()));

        IFC(Private::AutomationHelper::SetElementAutomationId(
            spPeriodPickerAsDO.Get(),
            wrl_wrappers::HStringReference(_periodLoopingSelectorAutomationId).Get()));
    }

    IFC(Initialize());
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
    RRETURN(hr);
}

_Check_return_ HRESULT
TimePickerFlyoutPresenter::OnCreateAutomationPeerImpl(
    _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::TimePickerFlyoutPresenter> spThis(this);
    wrl::ComPtr<xaml_controls::ITimePickerFlyoutPresenter> spThisAsITimePickerFlyoutPresenter;
    wrl::ComPtr<xaml_automation_peers::TimePickerFlyoutPresenterAutomationPeer> spTimePickerFlyoutPresenterAutomationPeer;

    IFC(spThis.As(&spThisAsITimePickerFlyoutPresenter));
    IFC(wrl::MakeAndInitialize<xaml_automation_peers::TimePickerFlyoutPresenterAutomationPeer>
            (&spTimePickerFlyoutPresenterAutomationPeer, spThisAsITimePickerFlyoutPresenter.Get()));

    IFC(spTimePickerFlyoutPresenterAutomationPeer.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TimePickerFlyoutPresenter::PullPropertiesFromOwner(
    _In_ xaml_controls::ITimePickerFlyout* pOwner)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<ITimePickerFlyout> spOwner(pOwner);
    wrl::ComPtr<IDependencyObject> spOwnerAsDO;
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBaseStatics> spPickerFlyoutBaseStatics;

    wrl_wrappers::HString clockIdentifier;
    wrl_wrappers::HString title;
    INT32 minuteIncrement = 0;
    wf::TimeSpan time = {};

    INT32 oldMinuteIncrement = _minuteIncrement;
    wrl_wrappers::HString oldClockID;
    INT32 clockIDCompareResult = 0;
    IFC(oldClockID.Duplicate(_clockIdentifier));


    IFC(spOwner.As(&spOwnerAsDO));
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_PickerFlyoutBase).Get(),
        &spPickerFlyoutBaseStatics));

    // Pull properties from owner
    IFC(spOwner->get_ClockIdentifier(clockIdentifier.GetAddressOf()));
    IFC(spPickerFlyoutBaseStatics->GetTitle(spOwnerAsDO.Get(), title.GetAddressOf()));
    IFC(spOwner->get_MinuteIncrement(&minuteIncrement));
    IFC(spOwner->get_Time(&time));

    // Check which values have changed
    IFC(WindowsCompareStringOrdinal(oldClockID.Get(), clockIdentifier.Get(), &clockIDCompareResult));

    // Store new values
    IFC(clockIdentifier.CopyTo(_clockIdentifier.ReleaseAndGetAddressOf()));
    IFC(title.CopyTo(_title.ReleaseAndGetAddressOf()));
    if (_tpTitlePresenter)
    {
        wrl::ComPtr<xaml::IUIElement> spPresenterAsUI;
        IFC(_tpTitlePresenter.As(&spPresenterAsUI));
        IFC(spPresenterAsUI->put_Visibility(_title.IsEmpty() ? xaml::Visibility_Collapsed : xaml::Visibility_Visible));
        IFC(_tpTitlePresenter->put_Text(_title.Get()));
    }
    _minuteIncrement = minuteIncrement;

    // Perform updates
    if (clockIDCompareResult != 0)
    {
        IFC(OnClockIdentifierChanged(oldClockID.Get()));
    }

    if (oldMinuteIncrement != _minuteIncrement)
    {
        IFC(OnMinuteIncrementChanged(oldMinuteIncrement, _minuteIncrement));
    }

    // Time has its own handler since it can be set through multiple codepaths
    IFC(SetTime(time));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TimePickerFlyoutPresenter::SetAcceptDismissButtonsVisibility(_In_ bool isVisible)
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

_Check_return_ HRESULT TimePickerFlyoutPresenter::GetTime(_Out_ wf::TimeSpan* pTime)
{
    *pTime = _time;
    RRETURN(S_OK);
}

_Check_return_ HRESULT TimePickerFlyoutPresenter::SetTime(_In_ wf::TimeSpan newTime)
{
    // If we're setting the time to the null sentinel value,
    // we'll instead set it to the current time for the purposes
    // of where to place the user's position in the looping selectors.
    if (newTime.Duration == -1)
    {
        wf::DateTime dateTime = {};
        wrl::ComPtr<wg::ICalendar> calendar;

        IFC_RETURN(CreateNewCalendar(_clockIdentifier.Get(), &calendar));
        IFC_RETURN(calendar->SetToNow());
        IFC_RETURN(calendar->GetDateTime(&dateTime));
        IFC_RETURN(GetTimeSpanFromDateTime(dateTime, &newTime));
    }

    if (_time.Duration != newTime.Duration)
    {
        _time = newTime;
        IFC_RETURN(OnTimeChanged());
    }

    return S_OK;
}

_Check_return_ HRESULT TimePickerFlyoutPresenter::OnKeyDownImpl(_In_ xaml_input::IKeyRoutedEventArgs* pEventArgs)
{
    return DateTimePickerFlyoutHelper::OnKeyDownImpl(pEventArgs, _tpFirstPickerAsControl.Get(), _tpSecondPickerAsControl.Get(), _tpThirdPickerAsControl.Get(), _tpContentPanel.Get());
}

_Check_return_ HRESULT TimePickerFlyoutPresenter::Initialize()
{
    HRESULT hr = S_OK;

    if (!(_tpMinuteSource && _tpHourSource && _tpPeriodSource))
    {
        wrl::ComPtr<wfci_::Vector<IInspectable*>> spCollection;
        wrl::ComPtr<wfc::IVector<IInspectable*>> spCollectionAsInterface;
        IFC(wfci_::Vector<IInspectable*>::Make(&spCollection));
        IFC(spCollection.As(&spCollectionAsInterface));
        IFC(SetPtrValue(_tpMinuteSource, spCollectionAsInterface.Get()));

        IFC(wfci_::Vector<IInspectable*>::Make(&spCollection));
        IFC(spCollection.As(&spCollectionAsInterface));
        IFC(SetPtrValue(_tpHourSource, spCollectionAsInterface.Get()));

        IFC(wfci_::Vector<IInspectable*>::Make(&spCollection));
        IFC(spCollection.As(&spCollectionAsInterface))
        IFC(SetPtrValue(_tpPeriodSource, spCollectionAsInterface.Get()));
    }

    IFC(RefreshSetup());

Cleanup:
    RRETURN(hr);
}


// Reacts to change in MinuteIncrement property.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::OnMinuteIncrementChanged(
    _In_ INT32 oldValue,
    _In_ INT32 newValue)
{
    HRESULT hr = S_OK;

    if (newValue < TIMEPICKER_MINUTEINCREMENT_MIN || newValue > TIMEPICKER_MINUTEINCREMENT_MAX)
    {
        // revert the change
        _minuteIncrement = oldValue;
        IFC(E_INVALIDARG);
    }

    IFC(RefreshSetup());

Cleanup:
    RRETURN(hr);
}

// Reacts to change in ClockIdentifier property.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::OnClockIdentifierChanged(
    _In_ HSTRING oldValue)
{
    HRESULT hr = S_OK;

    IFC(RefreshSetup());

Cleanup:
    if (FAILED(hr))
    {
        // revert the change
        _clockIdentifier.Release();
        _clockIdentifier.Attach(oldValue);
        IGNOREHR(RefreshSetup());
    }
    RRETURN(hr);
}


// Reacts to changes in Time property. Time may have changed programmatically or end user may have changed the
// selection of one of our selectors causing a change in Time.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::OnTimeChanged()
{
    HRESULT hr = S_OK;
    wf::TimeSpan coercedTime = {};

    IFC(CheckAndCoerceTime(_time, &coercedTime));

    // We are checking to see if new value is different from the current one. This is because even if they are same,
    // calling put_Time will break any Binding on Time (if there is any) that this TimePicker is target of.
    if (_time.Duration != coercedTime.Duration)
    {
        // We are coercing the time. The new property change will execute the necessary logic so
        // we will just go to cleanup after this call.
        IFC(SetTime(coercedTime));
        goto Cleanup;
    }

    IFC(UpdateDisplay());

Cleanup:
    RRETURN(hr);
}


// Checks whether the given time is in our acceptable range, coerces it or raises exception when necessary.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::CheckAndCoerceTime(
    _In_ wf::TimeSpan time,
    _Out_ wf::TimeSpan* pCoercedTime)
{
    wf::TimeSpan coercedTime = {};
    wf::DateTime dateTime = {};
    INT32 minute = 0;
    INT32 minuteIncrement = 0;

    // Check the value of time, we do not accept negative timespan values
    if (time.Duration < 0)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // If the time's duration is greater than 24 hours, we coerce it down to 24 hour range
    // by taking mod of it.
    coercedTime.Duration = time.Duration % _timeSpanTicksPerDay;

    // Finally we coerce the minutes to a factor of MinuteIncrement
    IFC_RETURN(GetAdjustedMinuteIncrement(&minuteIncrement));
    IFC_RETURN(GetDateTimeFromTimeSpan(coercedTime, &dateTime));
    IFC_RETURN(_tpCalendar->SetDateTime(dateTime));
    IFC_RETURN(_tpCalendar->get_Minute(&minute));
    IFC_RETURN(_tpCalendar->put_Minute(minute - (minute % minuteIncrement)));
    IFC_RETURN(_tpCalendar->GetDateTime(&dateTime));
    IFC_RETURN(GetTimeSpanFromDateTime(dateTime, pCoercedTime));

    return S_OK;
}


// Reacts to change in selection of our selectors. Calculates the new date represented by the selected indices and updates the
// Date property.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::OnSelectorSelectionChanged(
    _In_ IInspectable* pSender,
    _In_ xaml_controls::ISelectionChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(pArgs);
    UNREFERENCED_PARAMETER(pSender);

    if (IsReactionToSelectionChangeAllowed())
    {
        IFC(UpdateTime());
    }

Cleanup:
    RRETURN(hr);
}

// Updates the Time property according to the selected indices of the selectors.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::UpdateTime()
{
    HRESULT hr = S_OK;
    INT32 hourIndex = 0;
    INT32 minuteIndex = 0;
    INT32 minuteIncrement = 0;
    INT32 periodIndex = 0;
    wf::DateTime dateTime = {};
    wf::TimeSpan timeSpan = {};

    if (_tpHourPicker)
    {
        IFC(_tpHourPicker->get_SelectedIndex(&hourIndex));
    }

    if (_tpMinutePicker)
    {
        IFC(_tpMinutePicker->get_SelectedIndex(&minuteIndex));
    }

    if (_tpPeriodPicker)
    {
        IFC(_tpPeriodPicker->get_SelectedIndex(&periodIndex));
    }

    IFC(SetSentinelDate(_tpCalendar.Get()));

    if (_is12HourClock)
    {
        INT32 firstPeriodInThisDay = 0;

        IFC(_tpCalendar->get_FirstPeriodInThisDay(&firstPeriodInThisDay));
        IFC(_tpCalendar->put_Period(periodIndex + firstPeriodInThisDay));
        // 12 hour clock time flow is 12, 1, 2, 3 ... 11 for both am and pm times. So if the index is 0 we need
        // to put hour 12 into hour calendar.
        if (hourIndex == TIMEPICKER_COERCION_INDEX)
        {
            IFC(_tpCalendar->put_Hour(_periodCoercionOffset));
        }
        else
        {
            IFC(_tpCalendar->put_Hour(hourIndex));
        }
    }
    else
    {
        IFC(_tpCalendar->put_Hour(hourIndex));
    }

    IFC(GetAdjustedMinuteIncrement(&minuteIncrement));
    IFC(_tpCalendar->put_Minute(minuteIncrement * minuteIndex));

    IFC(_tpCalendar->GetDateTime(&dateTime));
    IFC(GetTimeSpanFromDateTime(dateTime, &timeSpan));

    // If there is not any pickers changing time will not mean anything semantically.
    if (_tpHourPicker.Get() || _tpMinutePicker.Get() || _tpPeriodPicker.Get())
    {
        // We are checking to see if new value is different from the current one. This is because even if they are same,
        // calling put_Time will break any Binding on Time (if there is any) that this TimePicker is target of.
        if (_time.Duration != timeSpan.Duration)
        {
            IFC(SetTime(timeSpan));
        }
    }

Cleanup:
    RRETURN(hr);
}


// Creates a new wg::Calendar, taking into account the ClockIdentifier
_Check_return_
HRESULT
TimePickerFlyoutPresenter::CreateNewCalendar(
    _In_ HSTRING strClockIdentifier,
    _Outptr_ wg::ICalendar** ppCalendar)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wg::ICalendarFactory> spCalendarFactory;
    wrl::ComPtr<wg::ICalendar> spCalendar;
    wrl::ComPtr<wfc::IVectorView<HSTRING>> spLanguages;
    wrl::ComPtr<wfc::IIterable<HSTRING>> spLanguagesAsIterable;

    *ppCalendar = nullptr;

    IFC(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Calendar).Get(),
        spCalendar.ReleaseAndGetAddressOf()));

    IFC(spCalendar->get_Languages(&spLanguages));
    IFC(spLanguages.As(&spLanguagesAsIterable));

    //Create the calendar
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Calendar).Get(),
        &spCalendarFactory));

    IFC(spCalendarFactory->CreateCalendar(
        spLanguagesAsIterable.Get(), /* Languages*/
        wrl_wrappers::HStringReference(STR_LEN_PAIR(L"GregorianCalendar")).Get(), /* Calendar */
        strClockIdentifier, /* Clock */
        spCalendar.ReleaseAndGetAddressOf()));

    *ppCalendar = spCalendar.Detach();

Cleanup:
    RRETURN(hr);
}

// Creates a new DateTimeFormatter with the given format and clock identifier.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::CreateNewFormatterWithClock(
    _In_ HSTRING strFormat,
    _In_ HSTRING strClockIdentifier,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter** ppDateTimeFormatter)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatterFactory> spFormatterFactory;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    wrl::ComPtr<wfc::IVectorView<HSTRING>> spLanguages;
    wrl::ComPtr<wfc::IIterable<HSTRING>> spLanguagesAsIterable;
    wrl_wrappers::HString strGeographicRegion;
    wrl_wrappers::HString strCalendarSystem;

    *ppDateTimeFormatter = nullptr;

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_DateTimeFormatting_DateTimeFormatter).Get(),
        &spFormatterFactory));
    IFCPTR(spFormatterFactory.Get());

    IFC(spFormatterFactory->CreateDateTimeFormatter(strFormat, &spFormatter));

    IFC(spFormatter->get_GeographicRegion(strGeographicRegion.GetAddressOf()));
    IFC(spFormatter->get_Languages(&spLanguages));
    IFC(spFormatter->get_Calendar(strCalendarSystem.GetAddressOf()));
    IFC(spLanguages.As(&spLanguagesAsIterable));

    IFC(spFormatterFactory->CreateDateTimeFormatterContext(
            strFormat,/* Format string */
            spLanguagesAsIterable.Get(), /* Languages*/
            strGeographicRegion.Get(), /* Geographic region */
            strCalendarSystem.Get(), /* Calendar */
            strClockIdentifier, /* Clock */
            spFormatter.ReleaseAndGetAddressOf()));

    *ppDateTimeFormatter = spFormatter.Detach();

Cleanup:
    RRETURN(hr);
}

// Creates a new DateTimeFormatter with the given format and calendar identifier.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::CreateNewFormatterWithCalendar(
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

    *ppDateTimeFormatter = nullptr;

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_DateTimeFormatting_DateTimeFormatter).Get(),
        &spFormatterFactory));
    IFCPTR(spFormatterFactory.Get());

    IFC(spFormatterFactory->CreateDateTimeFormatter(strFormat, &spFormatter));

    IFC(spFormatter->get_GeographicRegion(strGeographicRegion.GetAddressOf()));
    IFC(spFormatter->get_Languages(&spLanguages));
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

_Check_return_ HRESULT TimePickerFlyoutPresenter::GetTimeFormatter(
    _In_ HSTRING strClockIdentifier,
    _Outptr_ wg::DateTimeFormatting::IDateTimeFormatter ** ppDateFormatter)
{
    HRESULT hr = S_OK;

    IFCPTR(ppDateFormatter);
    // We can only use the cached formatter if there is a cached formatter, cached formatter's format is the same as the new one's
    // and cached formatter's clock identifier is the same as the new one's.
    if (!(_tpTimeFormatter.Get()
        && strClockIdentifier == _strTimeFormatterClockIdentifier))
    {
        // We either do not have a cached formatter or it is stale. We need a create a new one and cache it along
        // with its identifying info.
        wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;

        _tpTimeFormatter.Clear();
        IFC(CreateNewFormatterWithClock(
            wrl_wrappers::HStringReference(STR_LEN_PAIR(L"shorttime")).Get(),
            strClockIdentifier,
            &spFormatter));
        IFC(SetPtrValue(_tpTimeFormatter, spFormatter.Get()));
        IFC(::WindowsDuplicateString(strClockIdentifier, _strTimeFormatterClockIdentifier.ReleaseAndGetAddressOf()));
    }

    _tpTimeFormatter.CopyTo(ppDateFormatter);

Cleanup:
    RRETURN(hr);
}

// Sets our sentinel date to the given calendar. This date is 21st of July 2011 midnight.
// On this day there are no known daylight saving transitions.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::SetSentinelDate(
    _In_ wg::ICalendar* pCalendar)
{
    HRESULT hr = S_OK;

    IFC(pCalendar->put_Year(TIMEPICKER_SENTINELDATE_YEAR));
    IFC(pCalendar->put_Month(TIMEPICKER_SENTINELDATE_MONTH));
    IFC(pCalendar->put_Day(TIMEPICKER_SENTINELDATE_DAY));

    if (_is12HourClock)
    {
        INT32 firstPeriodInThisDay = 0;

        IFC(pCalendar->get_FirstPeriodInThisDay(&firstPeriodInThisDay));
        IFC(pCalendar->put_Period(firstPeriodInThisDay));
        IFC(pCalendar->put_Hour(TIMEPICKER_SENTINELDATE_HOUR12));
    }
    else
    {
        IFC(pCalendar->put_Hour(TIMEPICKER_SENTINELDATE_HOUR24));
    }
    IFC(pCalendar->put_Minute(TIMEPICKER_SENTINELDATE_TIMEFIELDS));
    IFC(pCalendar->put_Second(TIMEPICKER_SENTINELDATE_TIMEFIELDS));
    IFC(pCalendar->put_Nanosecond(TIMEPICKER_SENTINELDATE_TIMEFIELDS));

Cleanup:
    RRETURN(hr);
}

// Generate the collection that we will populate our hour picker with.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::GenerateHours()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    wrl_wrappers::HString strHour;
    wf::DateTime dateTime = {};
    INT32 firstHourInThisPeriod = 0;
    INT32 numberOfHours = 0;

    IFC(CreateNewFormatterWithClock(wrl_wrappers::HStringReference(_strHourFormat).Get(),
                            _clockIdentifier.Get(), &spFormatter));

    IFC(SetSentinelDate(_tpCalendar.Get()));
    IFC(_tpCalendar->get_NumberOfHoursInThisPeriod(&numberOfHours));
    IFC(_tpCalendar->get_FirstHourInThisPeriod(&firstHourInThisPeriod));
    IFC(_tpCalendar->put_Hour(firstHourInThisPeriod));

    IFC(_tpHourSource->Clear());

    for (INT32 hourOffset = 0; hourOffset < numberOfHours; hourOffset++)
    {
        wrl::ComPtr<IDatePickerFlyoutItem> spItem;
        wrl::ComPtr<IInspectable> spInspectable;
        IFC(wrl::MakeAndInitialize<DatePickerFlyoutItem>(&spItem));

        IFC(_tpCalendar->GetDateTime(&dateTime));
        IFC(spFormatter->Format(dateTime, strHour.ReleaseAndGetAddressOf()));

        IFC(spItem->put_PrimaryText(strHour.Get()));
        IFC(spItem.As(&spInspectable));

        IFC(_tpHourSource->Append(spInspectable.Get()));

        IFC(_tpCalendar->AddHours(1));
    }

Cleanup:
    RRETURN(hr);
}

// Generate the collection that we will populate our minute picker with.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::GenerateMinutes()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    wrl_wrappers::HString strMinute;
    wf::DateTime dateTime = {};
    INT32 minuteIncrement = 0;
    INT32 lastMinute = 0;
    INT32 firstMinuteInThisHour = 0;

    IFC(CreateNewFormatterWithClock(wrl_wrappers::HStringReference(_strMinuteFormat).Get(),
                            _clockIdentifier.Get(), &spFormatter));
    IFC(SetSentinelDate(_tpCalendar.Get()));
    IFC(GetAdjustedMinuteIncrement(&minuteIncrement));
    IFC(_tpCalendar->get_LastMinuteInThisHour(&lastMinute));
    IFC(_tpCalendar->get_FirstMinuteInThisHour(&firstMinuteInThisHour));

    IFC(_tpMinuteSource->Clear())

    for (INT32 i = firstMinuteInThisHour; i <= lastMinute / minuteIncrement; i++)
    {
        wrl::ComPtr<IDatePickerFlyoutItem> spItem;
        wrl::ComPtr<IInspectable> spInspectable;
        IFC(wrl::MakeAndInitialize<DatePickerFlyoutItem>(&spItem));

        IFC(_tpCalendar->put_Minute(i * minuteIncrement));

        IFC(_tpCalendar->GetDateTime(&dateTime));
        IFC(spFormatter->Format(dateTime, strMinute.ReleaseAndGetAddressOf()));

        IFC(spItem->put_PrimaryText(strMinute.Get()));
        IFC(spItem.As(&spInspectable));

        IFC(_tpMinuteSource->Append(spInspectable.Get()));
    }

Cleanup:
    RRETURN(hr);
}

// Generate the collection that we will populate our period picker with.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::GeneratePeriods()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatter;
    wrl_wrappers::HString strPeriod;
    wrl::ComPtr<IDatePickerFlyoutItem> spItem;
    wrl::ComPtr<IInspectable> spInspectable;
    INT32 firstPeriodInThisDay = 0;
    wf::DateTime dateTime = {};

    IFC(CreateNewFormatterWithClock(wrl_wrappers::HStringReference(_strPeriodFormat).Get(),
                            _clockIdentifier.Get(), &spFormatter));
    IFC(SetSentinelDate(_tpCalendar.Get()));

    IFC(_tpPeriodSource->Clear());

    IFC(_tpCalendar->get_FirstPeriodInThisDay(&firstPeriodInThisDay));
    IFC(_tpCalendar->put_Period(firstPeriodInThisDay));
    IFC(_tpCalendar->GetDateTime(&dateTime));
    IFC(spFormatter->Format(dateTime, strPeriod.ReleaseAndGetAddressOf()));

    IFC(wrl::MakeAndInitialize<DatePickerFlyoutItem>(&spItem));
    IFC(spItem->put_PrimaryText(strPeriod.Get()));
    IFC(spItem.As(&spInspectable));
    IFC(_tpPeriodSource->Append(spInspectable.Get()));

    IFC(_tpCalendar->AddPeriods(1));
    IFC(_tpCalendar->GetDateTime(&dateTime));
    IFC(spFormatter->Format(dateTime, strPeriod.ReleaseAndGetAddressOf()));

    IFC(wrl::MakeAndInitialize<DatePickerFlyoutItem>(spItem.ReleaseAndGetAddressOf()));
    IFC(spItem->put_PrimaryText(strPeriod.Get()));
    IFC(spItem.As(&spInspectable));
    IFC(_tpPeriodSource->Append(spInspectable.Get()));

Cleanup:
    RRETURN(hr);
}

// Clears the ItemsSource  properties of the selectors.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::ClearSelectors()
{
    HRESULT hr = S_OK;

    if (_tpHourPicker.Get())
    {
        IFC(_tpHourPicker->put_Items(nullptr));
    }

    if (_tpMinutePicker.Get())
    {
        IFC(_tpMinutePicker->put_Items(nullptr));
    }

    if (_tpPeriodPicker.Get())
    {
        IFC(_tpPeriodPicker->put_Items(nullptr));
    }

Cleanup:
    RRETURN(hr);
}

// Gets the layout ordering of the selectors.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::GetOrder(
    _Out_ INT32* hourOrder,
    _Out_ INT32* minuteOrder,
    _Out_ INT32* periodOrder,
    _Out_ BOOLEAN* isRTL)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatterWithClock;
    wrl::ComPtr<wg::DateTimeFormatting::IDateTimeFormatter> spFormatterWithCalendar;
    wrl::ComPtr<__FIVectorView_1_HSTRING> spPatterns;
    wrl_wrappers::HString strPattern;
    UINT32 length = 0;
    LPCWSTR szPattern;

    // Default ordering
    *hourOrder = 0;
    *minuteOrder = 1;
    *periodOrder = 2;

    IFC(CreateNewFormatterWithCalendar(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"month.full")).Get(),
        wrl_wrappers::HStringReference(STR_LEN_PAIR(L"GregorianCalendar")).Get(), &spFormatterWithCalendar));
    IFC(spFormatterWithCalendar->get_Patterns(&spPatterns));
    IFC(spPatterns->GetAt(0, strPattern.ReleaseAndGetAddressOf()));

    szPattern = strPattern.GetRawBuffer(&length);

    *isRTL = szPattern[0] == TIMEPICKER_RTL_CHARACTER_CODE;

    IFC(CreateNewFormatterWithClock(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"hour minute")).Get(), _clockIdentifier.Get(), &spFormatterWithClock));
    IFC(spFormatterWithClock->get_Patterns(&spPatterns));
    IFC(spPatterns->GetAt(0, strPattern.ReleaseAndGetAddressOf()));

    if (strPattern.Get())
    {
        LPCWSTR hourOccurence = nullptr;
        LPCWSTR minuteOccurence = nullptr;
        LPCWSTR periodOccurence = nullptr;

        szPattern = strPattern.GetRawBuffer(&length);

        // We do string search to determine the order of the fields.
        hourOccurence = wcsstr(szPattern, L"{hour");
        minuteOccurence = wcsstr(szPattern, L"{minute");
        periodOccurence = wcsstr(szPattern, L"{period");

        if (hourOccurence < minuteOccurence)
        {
            if (hourOccurence < periodOccurence)
            {
                *hourOrder = 0;
                if (minuteOccurence < periodOccurence)
                {
                    *minuteOrder = 1;
                    *periodOrder = 2;
                }
                else
                {
                    *minuteOrder = 2;
                    *periodOrder = 1;
                }
            }
            else
            {
                *hourOrder = 1;
                *minuteOrder = 2;
                *periodOrder = 0;
            }
        }
        else
        {
            if (hourOccurence < periodOccurence)
            {
                *hourOrder = 1;
                *minuteOrder = 0;
                *periodOrder = 2;
            }
            else
            {
                *hourOrder = 2;
                if (minuteOccurence < periodOccurence)
                {
                    *minuteOrder = 0;
                    *periodOrder = 1;
                }
                else
                {

                    *minuteOrder = 1;
                    *periodOrder = 0;
                }
            }
        }

        // Thi is a trick we are mimicking from from our js counterpart. In rtl languages if we just naively lay out our pickers right-to-left
        // it will not be correct. Say our LTR layout is HH MM PP, when we just lay it out RTL it will be PP MM HH, however we actually want
        // our pickers to be laid out as PP HH MM as hour and minute fields are english numerals and they should be laid out LTR. So we are
        // preempting our lay out mechanism and swapping hour and minute pickers, thus the final lay out will be correct.
        if (*isRTL)
        {
            std::swap(*hourOrder, *minuteOrder);
        }
    }

Cleanup:
    RRETURN(hr);
}

// Updates the order of selectors in our layout. Also takes care of hiding/showing the selectors and related spacing depending our
// public properties set by the user.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::UpdateOrderAndLayout()
{
    HRESULT hr = S_OK;
    INT32 hourOrder = 0;
    INT32 minuteOrder = 0;
    INT32 periodOrder = 0;
    BOOLEAN firstHostPopulated = FALSE;
    BOOLEAN secondHostPopulated = FALSE;
    BOOLEAN thirdHostPopulated = FALSE;
    BOOLEAN isRTL = FALSE;
    xaml::GridLength starGridLength = {};
    xaml::GridLength zeroGridLength = {};
    wrl::ComPtr<xaml::IUIElement> spUIElement;
    wrl::ComPtr<xaml_controls::IControl> firstPickerAsControl;
    wrl::ComPtr<xaml_controls::IControl> secondPickerAsControl;
    wrl::ComPtr<xaml_controls::IControl> thirdPickerAsControl;

    _tpFirstPickerAsControl.Clear();
    _tpSecondPickerAsControl.Clear();
    _tpThirdPickerAsControl.Clear();

    zeroGridLength.GridUnitType = xaml::GridUnitType_Pixel;
    zeroGridLength.Value = 0.0;
    starGridLength.GridUnitType = xaml::GridUnitType_Star;
    starGridLength.Value = 1.0;

    IFC(GetOrder(&hourOrder, &minuteOrder, &periodOrder, &isRTL));

    if (_tpContentPanel)
    {
        IFC(_tpContentPanel->put_FlowDirection(isRTL ?
            xaml::FlowDirection_RightToLeft : xaml::FlowDirection_LeftToRight));
    }

    // Clear the children of hosts first, so we never risk putting one picker in two hosts and failing.
    if (_tpFirstPickerHost)
    {
        IFC(_tpFirstPickerHost->put_Child(nullptr));
    }
    if (_tpSecondPickerHost)
    {
        IFC(_tpSecondPickerHost->put_Child(nullptr));
    }
    if (_tpThirdPickerHost)
    {
        IFC(_tpThirdPickerHost->put_Child(nullptr));
    }

    // Assign the selectors to the hosts.
    switch (hourOrder)
    {
        case 0:
            if (_tpFirstPickerHost && _tpHourPicker)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpHourPicker.As(&spPickerAsUI));
                IFC(_tpFirstPickerHost->put_Child(spPickerAsUI.Get()));
                IFC(_tpHourPicker.As(&firstPickerAsControl));
                firstHostPopulated = TRUE;
            }
            break;
        case 1:
            if (_tpSecondPickerHost && _tpHourPicker)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpHourPicker.As(&spPickerAsUI));
                IFC(_tpSecondPickerHost->put_Child(spPickerAsUI.Get()));
                IFC(_tpHourPicker.As(&secondPickerAsControl));
                secondHostPopulated = TRUE;
            }
            break;
        case 2:
            if (_tpThirdPickerHost && _tpHourPicker)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpHourPicker.As(&spPickerAsUI));
                IFC(_tpThirdPickerHost->put_Child(spPickerAsUI.Get()));
                IFC(_tpHourPicker.As(&thirdPickerAsControl));
                thirdHostPopulated = TRUE;
            }
            break;
    }

    switch (minuteOrder)
    {
        case 0:
            if (_tpFirstPickerHost && _tpMinutePicker)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpMinutePicker.As(&spPickerAsUI));
                IFC(_tpFirstPickerHost->put_Child(spPickerAsUI.Get()));
                IFC(_tpMinutePicker.As(&firstPickerAsControl));
                firstHostPopulated = TRUE;
            }
            break;
        case 1:
            if (_tpSecondPickerHost && _tpMinutePicker)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpMinutePicker.As(&spPickerAsUI));
                IFC(_tpSecondPickerHost->put_Child(spPickerAsUI.Get()));
                IFC(_tpMinutePicker.As(&secondPickerAsControl));
                secondHostPopulated = TRUE;
            }
            break;
        case 2:
            if (_tpThirdPickerHost && _tpMinutePicker)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpMinutePicker.As(&spPickerAsUI));
                IFC(_tpThirdPickerHost->put_Child(spPickerAsUI.Get()));
                IFC(_tpMinutePicker.As(&thirdPickerAsControl));
                thirdHostPopulated = TRUE;
            }
            break;
    }

    switch (periodOrder)
    {
        case 0:
            if (_tpFirstPickerHost && _tpPeriodPicker && _is12HourClock)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpPeriodPicker.As(&spPickerAsUI));
                IFC(_tpFirstPickerHost->put_Child(spPickerAsUI.Get()));
                IFC(_tpPeriodPicker.As(&firstPickerAsControl));
                firstHostPopulated = TRUE;
            }
            break;
        case 1:
            if (_tpSecondPickerHost && _tpPeriodPicker && _is12HourClock)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpPeriodPicker.As(&spPickerAsUI));
                IFC(_tpSecondPickerHost->put_Child(spPickerAsUI.Get()));
                IFC(_tpPeriodPicker.As(&secondPickerAsControl));
                secondHostPopulated = TRUE;
            }
            break;
        case 2:
            if (_tpThirdPickerHost && _tpPeriodPicker && _is12HourClock)
            {
                wrl::ComPtr<xaml::IUIElement> spPickerAsUI;
                IFC(_tpPeriodPicker.As(&spPickerAsUI));
                IFC(_tpThirdPickerHost->put_Child(spPickerAsUI.Get()));
                IFC(_tpPeriodPicker.As(&thirdPickerAsControl));
                thirdHostPopulated = TRUE;
            }
            break;
    }

    IFC(SetPtrValue(_tpFirstPickerAsControl, firstPickerAsControl.Get()));
    IFC(SetPtrValue(_tpSecondPickerAsControl, secondPickerAsControl.Get()));
    IFC(SetPtrValue(_tpThirdPickerAsControl, thirdPickerAsControl.Get()));

    if (_tpFirstPickerHost)
    {
        wrl::ComPtr<xaml::IUIElement> spHostAsUI;
        IFC(_tpFirstPickerHost.As(&spHostAsUI));
        IFC(spHostAsUI->put_Visibility(firstHostPopulated ? xaml::Visibility_Visible
            : xaml::Visibility_Collapsed));
        if (_tpFirstPickerHostColumn)
        {
            IFC(_tpFirstPickerHostColumn->put_Width(firstHostPopulated ? starGridLength : zeroGridLength));
        }

    }
    if (_tpSecondPickerHost)
    {
        wrl::ComPtr<xaml::IUIElement> spHostAsUI;
        IFC(_tpSecondPickerHost.As(&spHostAsUI));
        IFC(spHostAsUI->put_Visibility(secondHostPopulated ? xaml::Visibility_Visible
            : xaml::Visibility_Collapsed));
        if (_tpSecondPickerHostColumn)
        {
            IFC(_tpSecondPickerHostColumn->put_Width(secondHostPopulated ? starGridLength : zeroGridLength));
        }
    }
    if (_tpThirdPickerHost)
    {
        wrl::ComPtr<xaml::IUIElement> spHostAsUI;
        IFC(_tpThirdPickerHost.As(&spHostAsUI));
        IFC(spHostAsUI->put_Visibility(thirdHostPopulated ? xaml::Visibility_Visible
            : xaml::Visibility_Collapsed));
        if (_tpThirdPickerHostColumn)
        {
            IFC(_tpThirdPickerHostColumn->put_Width(thirdHostPopulated ? starGridLength : zeroGridLength));
        }
    }

    if (_tpHourPicker)
    {
        IFC(_tpHourPicker.As(&spUIElement));
        IFC(spUIElement->put_TabIndex(hourOrder));
    }
    if (_tpMinutePicker)
    {
        IFC(_tpMinutePicker.As(&spUIElement));
        IFC(spUIElement->put_TabIndex(minuteOrder));
    }
    if (_tpPeriodPicker)
    {
        IFC(_tpPeriodPicker.As(&spUIElement));
        IFC(spUIElement->put_TabIndex(periodOrder));
    }

    // Determine if we will show the dividers and assign visibilities to them. We will determine if the dividers
    // are shown by looking at which borders are populated.
    if (_tpFirstPickerSpacing)
    {
        wrl::ComPtr<xaml::IUIElement> spSpacerAsUI;
        IFC(_tpFirstPickerSpacing.As(&spSpacerAsUI));
        IFC(spSpacerAsUI->put_Visibility(
            firstHostPopulated && (secondHostPopulated || thirdHostPopulated) ?
            xaml::Visibility_Visible : xaml::Visibility_Collapsed));
    }
    if (_tpSecondPickerSpacing)
    {
        wrl::ComPtr<xaml::IUIElement> spSpacerAsUI;
        IFC(_tpSecondPickerSpacing.As(&spSpacerAsUI));
        IFC(spSpacerAsUI->put_Visibility(
            secondHostPopulated && thirdHostPopulated ?
            xaml::Visibility_Visible : xaml::Visibility_Collapsed));
    }

Cleanup:
    RRETURN(hr);
}

// Updates the selector selected indices to display our Time property.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::UpdateDisplay()
{
    HRESULT hr = S_OK;
    wf::DateTime dateTime = {};
    wrl_wrappers::HString strClockIdentifier;
    INT32 hour = 0;
    INT32 minuteIncrement = 0;
    INT32 minute;
    INT32 period = 0;
    INT32 firstPeriodInThisDay = 0;
    INT32 firstMinuteInThisHour = 0;
    INT32 firstHourInThisPeriod = 0;

    PreventReactionToSelectionChange();

    IFC(GetDateTimeFromTimeSpan(_time, &dateTime));
    IFC(_tpCalendar->SetDateTime(dateTime));

    // Calculate the period index and set it
    if (_is12HourClock)
    {
        IFC(_tpCalendar->get_Period(&period));
        IFC(_tpCalendar->get_FirstPeriodInThisDay(&firstPeriodInThisDay));
        if (_tpPeriodPicker.Get())
        {
            IFC(_tpPeriodPicker->put_SelectedIndex(period - firstPeriodInThisDay));
        }
    }

    // Calculate the hour index and set it
    IFC(_tpCalendar->get_Hour(&hour));
    IFC(_tpCalendar->get_FirstHourInThisPeriod(&firstHourInThisPeriod));
    if (_is12HourClock)
    {
        // For 12 hour clock 12 am and 12 pm are always the first element (index 0) in hour picker.
        // Other hours translate directly to indices. So it is sufficient to make a mod operation while translating
        // hour to index.
        if (_tpHourPicker.Get())
        {
            IFC(_tpHourPicker->put_SelectedIndex(hour % _periodCoercionOffset));
        }
    }
    else
    {
        // For 24 hour clock, Hour translates exactly to the hour picker's selected index.
        if (_tpHourPicker.Get())
        {
            IFC(_tpHourPicker->put_SelectedIndex(hour));
        }
    }

    // Calculate the minute index and set it
    IFC(GetAdjustedMinuteIncrement(&minuteIncrement));
    IFC(_tpCalendar->get_Minute(&minute));
    IFC(_tpCalendar->get_FirstMinuteInThisHour(&firstMinuteInThisHour));
    if (_tpMinutePicker)
    {
        IFC(_tpMinutePicker->put_SelectedIndex((minute / minuteIncrement) - firstMinuteInThisHour));
    }

Cleanup:
    AllowReactionToSelectionChange();
    RRETURN(hr);
}

// Clears everything, generates and sets the itemssources to selectors.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::RefreshSetup()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wg::ICalendar> spCalendar;

    PreventReactionToSelectionChange();

    _is12HourClock = _clockIdentifier == wrl_wrappers::HStringReference(_strTwelveHourClock);

    IFC(CreateNewCalendar(_clockIdentifier.Get(), &spCalendar));
    IFC(SetSentinelDate(spCalendar.Get()));

    // Clock identifier change may have rendered _tpCalendar stale.
    IFC(CreateNewCalendar(_clockIdentifier.Get(), &spCalendar));
    IFC(SetPtrValue(_tpCalendar, spCalendar.Get()));

    IFC(ClearSelectors());
    IFC(UpdateOrderAndLayout());

    if (_tpHourPicker.Get())
    {
        IFC(GenerateHours());
        IFC(_tpHourPicker->put_Items(_tpHourSource.Get()));
    }

    if (_tpMinutePicker.Get())
    {
        IFC(GenerateMinutes());
        IFC(_tpMinutePicker->put_Items(_tpMinuteSource.Get()));
        // If MinuteIncrement is zero, we don't want the minutes column to loop.
        IFC(_tpMinutePicker->put_ShouldLoop(_minuteIncrement != 0));
    }

    if (_tpPeriodPicker.Get())
    {
        IFC(GeneratePeriods());
        IFC(_tpPeriodPicker->put_Items(_tpPeriodSource.Get()))
    }

    IFC(UpdateDisplay());
    IFC(UpdateTime());

Cleanup:
    AllowReactionToSelectionChange();
    RRETURN(hr);
}

// Translates ONLY the hour and minute fields of DateTime into TimeSpan.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::GetTimeSpanFromDateTime(
    _In_ wf::DateTime dateTime,
    _Out_ wf::TimeSpan* pTimeSpan)
{
    HRESULT hr = S_OK;
    wf::TimeSpan timeSpan = {};
    INT32 hour = 0;
    INT32 minute = 0;

    IFC(_tpCalendar->SetDateTime(dateTime));

    IFC(_tpCalendar->get_Minute(&minute));
    timeSpan.Duration += minute * _timeSpanTicksPerMinute;

    IFC(_tpCalendar->get_Hour(&hour));
    if (_is12HourClock)
    {
        INT32 period = 0;
        INT32 firstPeriodInThisDay = 0;

        IFC(_tpCalendar->get_Period(&period));
        IFC(_tpCalendar->get_FirstPeriodInThisDay(&firstPeriodInThisDay));

        if (period == firstPeriodInThisDay)
        {
            if (hour == _periodCoercionOffset)
            {
                hour = 0;
            }
        }
        else
        {
            if (hour != _periodCoercionOffset)
            {
                hour += _periodCoercionOffset;
            }
        }
    }
    timeSpan.Duration += hour * _timeSpanTicksPerHour;

    *pTimeSpan = timeSpan;

Cleanup:
    RRETURN(hr);
}

// Translates a timespan to datetime. Note that, unrelated fields of datetime (year, day etc.)
// are set to our sentinel values.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::GetDateTimeFromTimeSpan(
    _In_ wf::TimeSpan timeSpan,
    _Out_ wf::DateTime* pDateTime)
{
    HRESULT hr = S_OK;
    wf::DateTime dateTime = {};

    IFC(SetSentinelDate(_tpCalendar.Get()));
    IFC(_tpCalendar->GetDateTime(&dateTime));

    dateTime.UniversalTime += timeSpan.Duration;

    *pDateTime = dateTime;

Cleanup:
    RRETURN(hr);
}

// Gets the minute increment and if it is 0, adjusts it to 60 so we will handle the 0
// case correctly.
_Check_return_
HRESULT
TimePickerFlyoutPresenter::GetAdjustedMinuteIncrement(
    _Out_ INT32* minuteIncrement)

{
    *minuteIncrement = _minuteIncrement;
    if (*minuteIncrement == 0)
    {
        *minuteIncrement = TIMEPICKER_MINUTEINCREMENT_ZERO_REPLACEMENT;
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT TimePickerFlyoutPresenter::GetDefaultIsDefaultShadowEnabled(_Outptr_ IInspectable** ppIsDefaultShadowEnabledValue)
{
    HRESULT hr = S_OK;

    IFC(Private::ValueBoxer::CreateBoolean(true, ppIsDefaultShadowEnabledValue));

Cleanup:
    RRETURN(hr);
}

} } } } XAML_ABI_NAMESPACE_END