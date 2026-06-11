// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

_Check_return_ HRESULT XamlSchemaContext::GetRuntime(
    _In_ XamlFactoryKind fk,
    _Out_ std::shared_ptr<XamlRuntime>& rspXamlRuntime
    )
{
    switch (fk)
    {
    case tfkNative:
        if (!m_spNativeRuntime)
        {
            auto runtime = std::make_shared<XamlNativeRuntime>(shared_from_this());
            m_spNativeRuntime = std::static_pointer_cast<XamlNativeRuntime>(runtime);
        }
        rspXamlRuntime = m_spNativeRuntime;
        break;
    case tfkManaged:
        if (!m_spManagedRuntime)
        {
            auto runtime = std::make_shared<XamlManagedRuntime>(shared_from_this());
            m_spManagedRuntime = std::static_pointer_cast<XamlManagedRuntime>(runtime);
        }
        rspXamlRuntime = m_spManagedRuntime;
        break;
    default:
        IFC_RETURN(E_FAIL);
    }


    return S_OK;
}
