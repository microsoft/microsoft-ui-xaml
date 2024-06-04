// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{
    class JumpListItemBackgroundConverter :
        public JumpListItemBackgroundConverterGenerated
    {
    public:

        static _Check_return_ HRESULT GetDefaultEnabled(_Outptr_ IInspectable** ppDefaultEnabledBrush);
        static _Check_return_ HRESULT GetDefaultDisabled(_Outptr_ IInspectable** ppDefaultDisabledBrush);

        // IValueConverter methods
        _Check_return_ HRESULT ConvertImpl(
            _In_ IInspectable* value,
            _In_ wxaml_interop::TypeName targetType,
            _In_ IInspectable* parameter,
            _In_ HSTRING language,
            _Outptr_ IInspectable** returnValue);

        _Check_return_ HRESULT ConvertBackImpl(
            _In_ IInspectable* value,
            _In_ wxaml_interop::TypeName targetType,
            _In_ IInspectable* parameter,
            _In_ HSTRING language,
            _Outptr_ IInspectable** returnValue);

    private:
         _Check_return_ HRESULT InitializeImpl() override;

        static const WCHAR c_EnabledBrushName[];
        static const WCHAR c_DisabledBrushName[];
    };

    ActivatableClassWithFactory(JumpListItemBackgroundConverter, JumpListItemBackgroundConverterFactory);

} } } } } XAML_ABI_NAMESPACE_END
