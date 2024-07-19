// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{

class ListPickerFlyoutPresenterAutomationPeer :
    public ListPickerFlyoutPresenterAutomationPeerGenerated
{

public:
    ListPickerFlyoutPresenterAutomationPeer() {}

protected:

    // IAutomationPeerOverrides
    _Check_return_ HRESULT
    GetAutomationControlTypeCoreImpl(
        _Out_ xaml::Automation::Peers::AutomationControlType *returnValue) override;

    _Check_return_ HRESULT
    GetClassNameCoreImpl(
        _Out_ HSTRING *returnValue) override;

    _Check_return_ HRESULT
    GetNameCoreImpl(
        _Out_ HSTRING *returnValue) override;

private:
    ~ListPickerFlyoutPresenterAutomationPeer() {}

    _Check_return_ HRESULT InitializeImpl(
        _In_ xaml_controls::IListPickerFlyoutPresenter* pOwner) override;
};

} } } } } XAML_ABI_NAMESPACE_END

