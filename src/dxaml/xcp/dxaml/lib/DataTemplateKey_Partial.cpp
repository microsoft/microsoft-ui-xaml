// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DataTemplateKey.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT DataTemplateKey::get_DataTypeImpl(
    _Outptr_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = m_pDataType;
    AddRefInterface(m_pDataType);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DataTemplateKey::put_DataTypeImpl(
    _In_ IInspectable* pValue)
{
    HRESULT hr = S_OK;
    wf::IReference<wxaml_interop::TypeName>* pType = NULL;

    ReleaseInterface(m_pDataType);

    if (pValue != NULL)
    {
        // Verify the value is a type.
        IFC(ctl::do_query_interface(pType, pValue));

        m_pDataType = pType;
        pType = NULL;
    }

Cleanup:
    ReleaseInterface(pType);
    RRETURN(hr);
}

_Check_return_ HRESULT DataTemplateKeyFactory::CreateInstanceWithTypeImpl(
    _In_ IInspectable* pDataType,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml::IDataTemplateKey** ppInstance)
{
    HRESULT hr = S_OK;
    IInspectable* pInner = NULL;
    IDataTemplateKey* pInstance = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);

    IFC(ctl::AggregableActivationFactory<DataTemplateKey>::ActivateInstance(pOuter, &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(pInstance->put_DataType(pDataType));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(pInner);
    ReleaseInterface(pInstance);
    RRETURN(hr);
}
