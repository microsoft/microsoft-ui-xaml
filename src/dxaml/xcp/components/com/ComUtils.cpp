// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComUtils.h"

_Check_return_ HRESULT ctl::do_get_property_type(_In_ IInspectable *pIn, _Out_ wf::PropertyType* pType)
{
    HRESULT hr = S_OK;
    wf::IPropertyValue* pValue = NULL;

    IFCPTR(pType);

    pValue = get_property_value(pIn);
    if (pValue)
    {
        IFC(pValue->get_Type(pType));
    }
    else
    {
        IFC(E_FAIL);
    }

Cleanup:
    ReleaseInterface(pValue);
    RRETURN(hr);
}

wf::IPropertyValue* ctl::get_property_value(IInspectable *pIn)
{
   if (pIn)
   {
       return query_interface<wf::IPropertyValue>(pIn);
   }
   return NULL;
}

_Check_return_ HRESULT ctl::do_get_value(_Out_ HSTRING& pOut, _In_ IInspectable *pIn)
{
    HRESULT hr = S_OK;
    wf::PropertyType type = wf::PropertyType_Empty;
    wf::IPropertyValue* pValue = get_property_value(pIn);

    pOut = NULL;
    if (pValue)
    {
        IFC(pValue->get_Type(&type));
        if (type == wf::PropertyType_String)
        {
            IFC(pValue->GetString(&pOut));
        }
    }

Cleanup:       
    ReleaseInterface(pValue);
    return hr;
}

_Check_return_ HRESULT ctl::do_get_value(_Out_ BOOLEAN& pOut, _In_ IInspectable *pIn)
{
    HRESULT hr = S_OK;
    wf::IPropertyValue* pValue = get_property_value(pIn);

    IFCPTR(pValue);

    IFC(pValue->GetBoolean(&pOut));

Cleanup:       
    ReleaseInterface(pValue);
    return hr;
}

_Check_return_ HRESULT ctl::do_get_value(_Out_ INT& pOut, _In_ IInspectable *pIn)
{
    HRESULT hr = S_OK;
    wf::IPropertyValue* pValue = get_property_value(pIn);

    IFCPTR(pValue);

    IFC(pValue->GetInt32(&pOut));

Cleanup:       
    ReleaseInterface(pValue);        
    return hr;
}

_Check_return_ HRESULT ctl::do_get_value(_Out_ UINT& pOut, _In_ IInspectable *pIn)
{
    HRESULT hr = S_OK;
    wf::IPropertyValue* pValue = get_property_value(pIn);

    IFCPTR(pValue);

    IFC(pValue->GetUInt32(&pOut));

Cleanup:       
    ReleaseInterface(pValue);        
    return hr;
}

_Check_return_ HRESULT ctl::do_get_value(_Out_ INT64& pOut, _In_ IInspectable *pIn)
{
    HRESULT hr = S_OK;
    wf::IPropertyValue* pValue = get_property_value(pIn);

    IFCPTR(pValue);

    IFC(pValue->GetInt64(&pOut));

Cleanup:
    ReleaseInterface(pValue);
    return hr;
}

_Check_return_ HRESULT ctl::do_get_value(_Out_ UINT64& pOut, _In_ IInspectable *pIn)
{
    HRESULT hr = S_OK;
    wf::IPropertyValue* pValue = get_property_value(pIn);

    IFCPTR(pValue);

    IFC(pValue->GetUInt64(&pOut));

Cleanup:
    ReleaseInterface(pValue);
    return hr;
}

_Check_return_ HRESULT ctl::do_get_value(_Out_ FLOAT& pOut, _In_ IInspectable *pIn)
{
    HRESULT hr = S_OK;
    wf::IPropertyValue* pValue = get_property_value(pIn);

    IFCPTR(pValue);

    IFC(pValue->GetSingle(&pOut));

Cleanup:       
    ReleaseInterface(pValue);        
    return hr;
}

_Check_return_ HRESULT ctl::do_get_value(_Out_ DOUBLE& pOut, _In_ IInspectable *pIn)
{
    HRESULT hr = S_OK;
    wf::IPropertyValue* pValue = get_property_value(pIn);

    IFCPTR(pValue);

    IFC(pValue->GetDouble(&pOut));

Cleanup:       
    ReleaseInterface(pValue);        
    return hr;
}

_Check_return_ HRESULT ctl::do_get_value(_Out_ wf::TimeSpan& pOut, _In_ IInspectable *pIn)
{
    HRESULT hr = S_OK;
    wf::IPropertyValue* pValue = get_property_value(pIn);

    IFCPTR(pValue);

    IFC(pValue->GetTimeSpan(&pOut));

Cleanup:       
    ReleaseInterface(pValue);        
    return hr;
}

_Check_return_ HRESULT ctl::do_get_value(_Out_ wf::DateTime& pOut, _In_ IInspectable *pIn)
{
    HRESULT hr = S_OK;
    wf::IPropertyValue* pValue = get_property_value(pIn);

    IFCPTR(pValue);

    IFC(pValue->GetDateTime(&pOut));

Cleanup:       
    ReleaseInterface(pValue);        
    return hr;
}

_Check_return_ HRESULT ctl::are_equal(
    _In_ IUnknown *pFirst,
    _In_ IUnknown *pSecond,
    _Out_ bool *pAreEqual
    )
{
    *pAreEqual = are_equal(pFirst, pSecond);
    RRETURN(S_OK);
}

_Check_return_ bool ctl::are_equal(_In_ IUnknown* pFirst, _In_ IUnknown* pSecond)
{
    auto spFirstIdentity = ctl::query_interface_cast<IUnknown>(pFirst);
    auto spSecondIdentity = ctl::query_interface_cast<IUnknown>(pSecond);

    // QI from IUnknown to IUnknown should never fail.
    ASSERT(!pFirst || spFirstIdentity);
    ASSERT(!pSecond || spSecondIdentity);

    return spFirstIdentity.Get() == spSecondIdentity.Get();
}