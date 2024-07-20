// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InternalDebugInterop.h"

#include "BindingExpression.g.h"
#include "Binding.g.h"
#include "CollectionViewSource.g.h"
#include "xamlbindinghelper.g.h"
#include "DefaultValueConverter.h"

namespace DirectUI
{
    class XamlBindingHelperFactory: public XamlBindingHelperFactoryGenerated
    {

    public:
        _Check_return_ HRESULT ConvertValueImpl(_In_ wxaml_interop::TypeName type, _In_ IInspectable* pValue, _Outptr_ IInspectable** ppReturnValue);

        // Override Close on debug builds only so we can release m_valueConverter.
#if XCP_MONITOR
        IFACEMETHOD(Close()) override;
#endif
    private:
        ctl::ComPtr<IValueConverterInternal> m_valueConverter;
    };
}
