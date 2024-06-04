// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    class PivotItemEventArgs :
        public ::Microsoft::WRL::RuntimeClass<
            xaml_controls::IPivotItemEventArgs,
            wrl::ComposableBase<xaml::IDependencyObjectFactory>
            >
    {
        InspectableClass(RuntimeClass_Microsoft_UI_Xaml_Controls_PivotItemEventArgs, TrustLevel::BaseTrust);

    public:
        PivotItemEventArgs();

        HRESULT RuntimeClassInitialize();

        IFACEMETHOD(get_Item)(
            _Out_ xaml_controls::IPivotItem **value);

        IFACEMETHOD(put_Item)(
            _In_ xaml_controls::IPivotItem *value);

    private:
        ~PivotItemEventArgs();

        wrl::WeakRef m_wpPivotItem;
    };

    ActivatableClass(PivotItemEventArgs);

} } } } XAML_ABI_NAMESPACE_END
