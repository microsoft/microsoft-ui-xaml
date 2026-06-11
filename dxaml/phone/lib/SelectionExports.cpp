// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PlatformHelpers.h"

extern "C"
HRESULT
XamlControlsGetDatePickerSelection(
    _In_ IInspectable *pSourceDatePicker,
    _In_ IInspectable *pPlacementTarget,
    _Outptr_ IInspectable **ppSelectionResultOperation
    )
{
    return XamlControls_GetDatePickerSelectionImpl(pSourceDatePicker, pPlacementTarget, ppSelectionResultOperation);
}

extern "C"
HRESULT
XamlControlsGetTimePickerSelection(
    _In_ IInspectable *pSourceTimePicker,
    _In_ IInspectable *pPlacementTarget,
    _Outptr_ IInspectable **ppSelectionResultOperation
    )
{
    return XamlControls_GetTimePickerSelectionImpl(pSourceTimePicker, pPlacementTarget, ppSelectionResultOperation);
}


HRESULT
XamlControls_GetDatePickerSelectionImpl(
    _In_ IInspectable *pSourceDatePicker,
    _In_ IInspectable *pPlacementTarget,
    _Outptr_ IInspectable **ppSelectionResultOperation
    )
{
    HRESULT hr = S_OK;

    // Pointer to the operation that this function will return to
    // the caller (a on-page DatePicker)
    wrl::ComPtr<wf::IAsyncOperation<wf::IReference<wf::DateTime>*>> spGetSelectionAsyncOperation;

    wrl::ComPtr<xaml_controls::IDatePicker> spDatePicker;
    wrl::ComPtr<xaml::IFrameworkElement> spPlacementTargetAsFE;

    // The DatePickerFlyout we will create and invoke
    wrl::ComPtr<xaml_controls::IDatePickerFlyout> spDatePickerFlyout;
    wrl::ComPtr<xaml::IDependencyObject> spDatePickerFlyoutAsDO;
    wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase;

    // Collection of properties needed to be copied from
    // the DatePicker into the DatePickerFlyout
    wrl_wrappers::HString calendarIdentifier;
    wf::DateTime dt = {};
    BOOLEAN monthVisible = FALSE;
    BOOLEAN yearVisible = FALSE;
    BOOLEAN dayVisible = FALSE;
    wf::DateTime minYear = {};
    wf::DateTime maxYear = {};
    wrl_wrappers::HString title;
    wrl_wrappers::HString dayFormat;
    wrl_wrappers::HString monthFormat;
    wrl_wrappers::HString yearFormat;
    auto overlayMode = xaml_controls::LightDismissOverlayMode_Off;
    auto soundMode = xaml::ElementSoundMode_Default;

    // PickerFlyoutBase statics, used to retrieve title attached property.
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBaseStatics> spPickerFlyoutBaseStatics;
    wrl::ComPtr<xaml::IDependencyObject> spDatePickerAsDO;

    IFC(pSourceDatePicker->QueryInterface<xaml_controls::IDatePicker>(
        &spDatePicker));
    IFCPTR(spDatePicker.Get());
    IFC(spDatePicker.As(&spDatePickerAsDO));

    IFC(pPlacementTarget->QueryInterface<xaml::IFrameworkElement>(&spPlacementTargetAsFE));
    IFCPTR(spPlacementTargetAsFE.Get());

    IFC(wrl::MakeAndInitialize<xaml_controls::DatePickerFlyout>(
        &spDatePickerFlyout));
    IFC(spDatePickerFlyout.As(&spDatePickerFlyoutAsDO));
    IFC(spDatePickerFlyout.As(&spFlyoutBase));

    // Copy over all the relevant properties to show the
    // Flyout with.
    IFC(spDatePicker->get_CalendarIdentifier(calendarIdentifier.GetAddressOf()));
    IFC(spDatePicker->get_MonthVisible(&monthVisible));
    IFC(spDatePicker->get_YearVisible(&yearVisible));
    IFC(spDatePicker->get_DayVisible(&dayVisible));
    IFC(spDatePicker->get_MinYear(&minYear));
    IFC(spDatePicker->get_MaxYear(&maxYear));
    IFC(spDatePicker->get_Date(&dt));
    IFC(spDatePicker->get_DayFormat(dayFormat.GetAddressOf()));
    IFC(spDatePicker->get_MonthFormat(monthFormat.GetAddressOf()));
    IFC(spDatePicker->get_YearFormat(yearFormat.GetAddressOf()));
    IFC(spDatePicker->get_LightDismissOverlayMode(&overlayMode));
    IFC(xaml_controls::PlatformHelpers::GetEffectiveSoundMode(spDatePickerAsDO.Get(), &soundMode));

    // See if the calling DatePicker has an attached Title, if not
    // retrieve the default phrase.
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_PickerFlyoutBase).Get(),
        &spPickerFlyoutBaseStatics));
    IFC(spPickerFlyoutBaseStatics->GetTitle(spDatePickerAsDO.Get(), title.GetAddressOf()));

    if (title == nullptr)
    {
        IFC(Private::FindStringResource(
            TEXT_DATEPICKERFLYOUT_TITLE,
            title.GetAddressOf()));
    }

    IFC(spDatePickerFlyout->put_CalendarIdentifier(calendarIdentifier.Get()));
    IFC(spPickerFlyoutBaseStatics->SetTitle(spDatePickerFlyoutAsDO.Get(), title.Get()));
    IFC(spDatePickerFlyout->put_MinYear(minYear));
    IFC(spDatePickerFlyout->put_MaxYear(maxYear));
    IFC(spDatePickerFlyout->put_MonthVisible(monthVisible));
    IFC(spDatePickerFlyout->put_YearVisible(yearVisible));
    IFC(spDatePickerFlyout->put_DayVisible(dayVisible));
    IFC(spDatePickerFlyout->put_Date(dt));
    IFC(spDatePickerFlyout->put_DayFormat(dayFormat.Get()));
    IFC(spDatePickerFlyout->put_MonthFormat(monthFormat.Get()));
    IFC(spDatePickerFlyout->put_YearFormat(yearFormat.Get()));
    IFC(spFlyoutBase->put_LightDismissOverlayMode(overlayMode));
    IFC(spFlyoutBase->put_ElementSoundMode(soundMode));

    // Actually cause the window to popup. This is the async operation that
    // will return with the Flyout is dismissed
    IFC(spDatePickerFlyout->ShowAtAsync(spPlacementTargetAsFE.Get(), &spGetSelectionAsyncOperation));
    IFC(spGetSelectionAsyncOperation.CopyTo(ppSelectionResultOperation));

Cleanup:
    RRETURN(hr);
}

HRESULT
XamlControls_GetTimePickerSelectionImpl(
    _In_ IInspectable *pSourceTimePicker,
    _In_ IInspectable *pPlacementTarget,
    _Outptr_ IInspectable **ppSelectionResultOperation
    )
{
    HRESULT hr = S_OK;

    // Pointer to the operation that this function will return to
    // the caller (a on-page TimePicker)
    wrl::ComPtr<wf::IAsyncOperation<wf::IReference<wf::TimeSpan>*>> spGetSelectionAsyncOperation;

    wrl::ComPtr<xaml_controls::ITimePicker> spTimePicker;
    wrl::ComPtr<xaml::IFrameworkElement> spPlacementTargetAsFE;

    // The DatePickerFlyout we will create and invoke, and the
    // IAsyncAction it will return.
    wrl::ComPtr<xaml_controls::ITimePickerFlyout> spTimePickerFlyout;
    wrl::ComPtr<xaml::IDependencyObject> spTimePickerFlyoutAsDO;
    wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase;

    // Collection of properties needed to be copied from
    // the DatePicker into the DatePickerFlyout
    wrl_wrappers::HString clockIdentifier;
    wf::TimeSpan ts = {};
    INT32 minuteInterval = 0;
    wrl_wrappers::HString title;
    auto overlayMode = xaml_controls::LightDismissOverlayMode_Off;
    auto soundMode = xaml::ElementSoundMode_Default;

    // PickerFlyoutBase statics, used to retrieve title attached property.
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBaseStatics> spPickerFlyoutBaseStatics;
    wrl::ComPtr<xaml::IDependencyObject> spTimePickerAsDO;

    IFC(pSourceTimePicker->QueryInterface<xaml_controls::ITimePicker>(
        &spTimePicker));
    IFCPTR(spTimePicker.Get());
    IFC(spTimePicker.As(&spTimePickerAsDO));

    IFC(pPlacementTarget->QueryInterface<xaml::IFrameworkElement>(&spPlacementTargetAsFE));
    IFCPTR(spPlacementTargetAsFE.Get());

    IFC(wrl::MakeAndInitialize<xaml_controls::TimePickerFlyout>(
        &spTimePickerFlyout));
    IFC(spTimePickerFlyout.As(&spTimePickerFlyoutAsDO));
    IFC(spTimePickerFlyout.As(&spFlyoutBase));

    // Copy over all the relevant properties to show the
    // Flyout with.
    IFC(spTimePicker->get_ClockIdentifier(clockIdentifier.GetAddressOf()));
    IFC(spTimePicker->get_MinuteIncrement(&minuteInterval));
    IFC(spTimePicker->get_Time(&ts));
    IFC(spTimePicker->get_LightDismissOverlayMode(&overlayMode));
    IFC(xaml_controls::PlatformHelpers::GetEffectiveSoundMode(spTimePickerAsDO.Get(), &soundMode));

    // See if the calling TimePicker has an attached Title, if not
    // retrieve the default phrase.
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_PickerFlyoutBase).Get(),
        &spPickerFlyoutBaseStatics));
    IFC(spPickerFlyoutBaseStatics->GetTitle(spTimePickerAsDO.Get(), title.GetAddressOf()));

    if (title == nullptr)
    {
        IFC(Private::FindStringResource(
            TEXT_TIMEPICKERFLYOUT_TITLE,
            title.GetAddressOf()));
    }

    IFC(spPickerFlyoutBaseStatics->SetTitle(spTimePickerFlyoutAsDO.Get(), title));
    IFC(spTimePickerFlyout->put_ClockIdentifier(clockIdentifier.Get()));
    IFC(spTimePickerFlyout->put_MinuteIncrement(minuteInterval));
    IFC(spTimePickerFlyout->put_Time(ts));
    IFC(spFlyoutBase->put_LightDismissOverlayMode(overlayMode));
    IFC(spFlyoutBase->put_ElementSoundMode(soundMode));

    // Actually cause the window to popup. This is the async operation that
    // will return with the Flyout is dismissed
    IFC(spTimePickerFlyout->ShowAtAsync(spPlacementTargetAsFE.Get(), &spGetSelectionAsyncOperation));
    IFC(spGetSelectionAsyncOperation.CopyTo(ppSelectionResultOperation));

Cleanup:
    RRETURN(hr);
}

extern "C"
HRESULT
XamlControlsGetListPickerSelection(
    _In_ IInspectable *pSourceComboBox,
    _Outptr_ IInspectable **ppSelectionResultOperation
    )
{
    return XamlControls_GetListPickerSelectionImpl(pSourceComboBox, ppSelectionResultOperation);
}

HRESULT
XamlControls_GetListPickerSelectionImpl(
    _In_ IInspectable *pSourceComboBox,
    _Outptr_ IInspectable **ppSelectionResultOperation
    )
{
    HRESULT hr = S_OK;

    wrl::ComPtr<wf::IAsyncOperation<wfc::IVectorView<IInspectable*>*>> spGetSelectionAsyncOperation;
    wrl::ComPtr<xaml_controls::ListPickerGetSelectedIndexAsyncOperation> spGetSelectedIndexAsyncOperation;
    wrl::ComPtr<IInspectable> spSourceComboBoxAsI(pSourceComboBox);
    wrl::ComPtr<xaml_controls::IComboBox> spSourceComboBox;
    wrl::ComPtr<xaml::IFrameworkElement> spSourceComboBoxAsFE;
    wrl::ComPtr<xaml_controls::IListPickerFlyout> spListPickerFlyout;

    IFC(spSourceComboBoxAsI.As(&spSourceComboBox));
    IFC(spSourceComboBox.As(&spSourceComboBoxAsFE));
    IFC(wrl::MakeAndInitialize<xaml_controls::ListPickerFlyout>(&spListPickerFlyout));
    IFC(CopyPropertiesFromComboBoxToListPickerFlyout(spSourceComboBox.Get(), spListPickerFlyout.Get()));

    // Show the flyout and set up the wrapper operation
    IFC(wrl::MakeAndInitialize<xaml_controls::ListPickerGetSelectedIndexAsyncOperation>(
        &spGetSelectedIndexAsyncOperation));
    IFC(spListPickerFlyout->ShowAtAsync(spSourceComboBoxAsFE.Get(), &spGetSelectionAsyncOperation));
    IFC(spGetSelectedIndexAsyncOperation->StartOperation(spListPickerFlyout.Get(), spGetSelectionAsyncOperation.Get()));
    IFC(spGetSelectedIndexAsyncOperation.CopyTo(ppSelectionResultOperation));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CopyPropertiesFromComboBoxToListPickerFlyout(
    _In_ xaml_controls::IComboBox* pSourceComboBox,
    _In_ xaml_controls::IListPickerFlyout* pDestLPF)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IDependencyObject> spSourceComboBoxAsDO;
    wrl::ComPtr<xaml_controls::IComboBox> spSourceComboBox(pSourceComboBox);
    wrl::ComPtr<xaml_primitives::ISelector> spSourceComboBoxAsSelector;
    wrl::ComPtr<xaml_controls::IItemsControl> spSourceComboBoxAsIC;
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBaseStatics> spPickerFlyoutBaseStatics;
    wrl::ComPtr<xaml_controls::IListPickerFlyout> spListPickerFlyout(pDestLPF);
    xaml_controls::ListPickerFlyout* pListPickerFlyoutNoRef = static_cast<xaml_controls::ListPickerFlyout*>(pDestLPF);
    wrl::ComPtr<xaml::IDependencyObject> spListPickerFlyoutAsDO;
    wrl::ComPtr<wfci_::Vector<IInspectable*>> spVector;
    wrl::ComPtr<wfc::IVector<IInspectable*>> spShadowItemsSource;
    wrl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    wrl::ComPtr<wfc::IVectorView<IInspectable*>> spItemsAsVectorView;
    wrl::ComPtr<xaml::IDataTemplate> spItemTemplate;
    UINT32 itemsCount = 0;
    INT32 selectedIndex = -1;
    wrl_wrappers::HString displayMemberPath;
    wrl_wrappers::HString selectedValuePath;
    wrl_wrappers::HString title;

    IFC(spSourceComboBox.As(&spSourceComboBoxAsDO));
    IFC(spSourceComboBox.As(&spSourceComboBoxAsSelector));
    IFC(spSourceComboBox.As(&spSourceComboBoxAsIC));

    IFC(wfci_::Vector<IInspectable*>::Make(&spVector));
    IFC(spVector.As(&spShadowItemsSource));
    IFC(spSourceComboBoxAsIC->get_Items(&spItems));
    ASSERT(spItems, "ComboBox Items property is NULL.");
    IFC(spItems.As(&spItemsAsVectorView));
    IFC(spItemsAsVectorView->get_Size(&itemsCount));
    for (UINT32 i = 0; i < itemsCount; i++)
    {
        wrl::ComPtr<IInspectable> spItem;
        wrl::ComPtr<xaml_controls::IComboBoxItem> spItemAsCBI;

        IFC(spItemsAsVectorView->GetAt(i, &spItem));

        if (SUCCEEDED(spItem.As(&spItemAsCBI)))
        {
            // If the item is a ComboBoxItem we want to pull out the content
            // and use that as the ListPickerFlyout item so that we don't pick
            // up the ComboBoxItem visuals.
            wrl::ComPtr<xaml_controls::IContentControl> spItemAsCC;
            wrl::ComPtr<IInspectable> spContent;

            IFC(spItemAsCBI.As(&spItemAsCC));
            IFC(spItemAsCC->get_Content(&spContent));
            IFC(spShadowItemsSource->Append(spContent.Get()));
        }
        else
        {
            IFC(spShadowItemsSource->Append(spItem.Get()));
        }
    }

    // Set original items owner and shadow items source
    IFC(pListPickerFlyoutNoRef->SetItemsOwner(spSourceComboBoxAsIC.Get()));
    IFC(spListPickerFlyout->put_ItemsSource(spShadowItemsSource.Get()));

    // ItemTemplate
    IFC(spSourceComboBoxAsIC->get_ItemTemplate(&spItemTemplate));
    IFC(spListPickerFlyout->put_ItemTemplate(spItemTemplate.Get()));

    // DisplayMemberPath
    IFC(spSourceComboBoxAsIC->get_DisplayMemberPath(displayMemberPath.GetAddressOf()));
    IFC(spListPickerFlyout->put_DisplayMemberPath(displayMemberPath.Get()));

    // SelectedValuePath
    IFC(spSourceComboBoxAsSelector->get_SelectedValuePath(selectedValuePath.GetAddressOf()));
    IFC(spListPickerFlyout->put_SelectedValuePath(selectedValuePath.Get()));

    // Selection
    IFC(spListPickerFlyout->put_SelectionMode(xaml_controls::ListPickerFlyoutSelectionMode_Single));
    IFC(spSourceComboBoxAsSelector->get_SelectedIndex(&selectedIndex));
    IFC(spListPickerFlyout->put_SelectedIndex(selectedIndex));

    // Title
    // See if the calling ComboBox has an attached Title. If not leave the default value in place.
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_PickerFlyoutBase).Get(),
        &spPickerFlyoutBaseStatics));
    IFC(spPickerFlyoutBaseStatics->GetTitle(spSourceComboBoxAsDO.Get(), title.GetAddressOf()));

    if (!title.IsEmpty())
    {
        IFC(spListPickerFlyout.As(&spListPickerFlyoutAsDO));
        IFC(spPickerFlyoutBaseStatics->SetTitle(spListPickerFlyoutAsDO.Get(), title));
    }

Cleanup:
    RRETURN(hr);
}

extern "C"
HRESULT
XamlControlsTestHookCreateLoopingSelector(
    _Outptr_ IInspectable **ppLoopingSelector
)
{
    return XamlControls_TestHookCreateLoopingSelectorImpl(ppLoopingSelector);
}

HRESULT
XamlControls_TestHookCreateLoopingSelectorImpl(
    _Outptr_ IInspectable **ppLoopingSelector
)
{
    HRESULT hr = S_OK;

    IFCPTR(ppLoopingSelector);
    IFC(wrl::MakeAndInitialize<xaml_primitives::LoopingSelector>(ppLoopingSelector));

Cleanup:
    return hr;
}
