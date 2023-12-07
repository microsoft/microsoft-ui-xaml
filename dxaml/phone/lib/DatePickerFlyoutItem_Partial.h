// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    class DatePickerFlyoutItem :
        public DatePickerFlyoutItemGenerated
    {

    public:
        DatePickerFlyoutItem() {}

        // ICustomPropertyProvider
        _Check_return_ HRESULT GetCustomPropertyImpl(_In_ HSTRING name, _Outptr_ xaml::Data::ICustomProperty** returnValue);
        _Check_return_ HRESULT GetIndexedPropertyImpl(_In_ HSTRING name, _In_ wxaml_interop::TypeName type, _Outptr_ xaml::Data::ICustomProperty** returnValue);
        _Check_return_ HRESULT GetStringRepresentationImpl(_Out_ HSTRING* plainText);

        _Check_return_ HRESULT get_TypeImpl(_Out_ wxaml_interop::TypeName* type);

    private:
        ~DatePickerFlyoutItem() {}

        _Check_return_ HRESULT InitializeImpl() override;
    };

    ActivatableClassWithFactory(DatePickerFlyoutItem, DatePickerFlyoutItemFactory);
} } } } XAML_ABI_NAMESPACE_END
