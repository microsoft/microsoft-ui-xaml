// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DataTemplateKey.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DataTemplateKey)
    {
    public:
        DataTemplateKey(): m_pDataType(NULL)
        {
        }

        ~DataTemplateKey() override
        {
            ReleaseInterface(m_pDataType);
        }

        _Check_return_ HRESULT get_DataTypeImpl(_Outptr_ IInspectable** pValue);
        _Check_return_ HRESULT put_DataTypeImpl(_In_ IInspectable* value);

    private:
        wf::IReference<wxaml_interop::TypeName>* m_pDataType;
    };
}
