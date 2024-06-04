// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{

const WCHAR JumpListItemForegroundConverter::c_EnabledBrushName[] = L"JumpListDefaultEnabledForeground";
const WCHAR JumpListItemForegroundConverter::c_DisabledBrushName[] = L"JumpListDefaultDisabledForeground";

_Check_return_ HRESULT
JumpListItemForegroundConverter::InitializeImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IDependencyObjectFactory> spInnerFactory;
    wrl::ComPtr<xaml::IDependencyObject> spInnerInstance;
    wrl::ComPtr<IInspectable> spInnerInspectable;


    IFC(JumpListItemForegroundConverterGenerated::InitializeImpl());
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_DependencyObject).Get(),
        &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IJumpListItemForegroundConverter*>(this),
        &spInnerInspectable,
        &spInnerInstance));

    IFC(SetComposableBasePointers(
        spInnerInspectable.Get(),
        spInnerFactory.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
JumpListItemForegroundConverter::GetDefaultEnabled(_Outptr_ IInspectable** ppDefaultEnabledBrush)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<IInspectable> spEnabledAsII;

    ARG_VALIDRETURNPOINTER(ppDefaultEnabledBrush);
    IFC(PlatformHelpers::LookupThemeResource(wrl_wrappers::HStringReference(c_EnabledBrushName).Get(), &spEnabledAsII));
    *ppDefaultEnabledBrush = spEnabledAsII.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
JumpListItemForegroundConverter::GetDefaultDisabled(_Outptr_ IInspectable** ppDefaultDisabledBrush)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<IInspectable> spDisabledAsII;

    ARG_VALIDRETURNPOINTER(ppDefaultDisabledBrush);
    IFC(PlatformHelpers::LookupThemeResource(wrl_wrappers::HStringReference(c_DisabledBrushName).Get(), &spDisabledAsII));
    *ppDefaultDisabledBrush = spDisabledAsII.Detach();

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT
JumpListItemForegroundConverter::ConvertImpl(
    _In_ IInspectable* value,
    _In_ wxaml_interop::TypeName,
    _In_ IInspectable*,
    _In_ HSTRING,
    _Outptr_ IInspectable** returnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN hasItems = FALSE;
    wrl::ComPtr<xaml_media::IBrush> spBrush;

    ARG_VALIDRETURNPOINTER(returnValue);

    IFC(Private::JumpListHelper::HasItems(value, &hasItems));

    if (hasItems)
    {
        IFC(get_Enabled(&spBrush));
    }
    else
    {
        IFC(get_Disabled(&spBrush));
    }

    *returnValue = spBrush.Detach();
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
JumpListItemForegroundConverter::ConvertBackImpl(
    _In_ IInspectable*,
    _In_ wxaml_interop::TypeName,
    _In_ IInspectable*,
    _In_ HSTRING,
    _Outptr_ IInspectable**)
{
    RRETURN(E_NOTIMPL);
}

} } } } } XAML_ABI_NAMESPACE_END
