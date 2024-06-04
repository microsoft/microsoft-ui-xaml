// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CustomXamlResourceLoaderWrapper.h"

using namespace DirectUI;
using namespace Microsoft::WRL;

CustomXamlResourceLoaderWrapper::CustomXamlResourceLoaderWrapper(_In_ xaml::Resources::ICustomXamlResourceLoader *pWinrtLoader)
{
    m_pWinrtLoader = pWinrtLoader;
    pWinrtLoader->AddRef();
}

CustomXamlResourceLoaderWrapper::~CustomXamlResourceLoaderWrapper()
{
    ReleaseInterface(m_pWinrtLoader);
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//      Translates the ICustomResourceLoader::GetResource call into a
//      xaml::Resources::ICustomResourceLoader::GetResource call, marshaling
//      arguments and return values.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CustomXamlResourceLoaderWrapper::GetResource(
    _In_ const xstring_ptr& resourceId,
    _In_ const xstring_ptr& objectType,
    _In_ const xstring_ptr& propertyName,
    _In_ const xstring_ptr& propertyType,
    _Out_ CValue *pValue
    )
{
    HRESULT hr = S_OK;
    xruntime_string_ptr strResourceId;
    xruntime_string_ptr strObjectType;
    xruntime_string_ptr strPropertyName;
    xruntime_string_ptr strPropertyType;
    IInspectable *pInspectable = NULL;
    BoxerBuffer buffer;
    CValue outValue;
    DependencyObject *pMOR = NULL;
    ComPtr<xaml::Resources::ICustomXamlResourceLoaderOverrides> pOverrides;
    ComPtr<xaml::Resources::ICustomXamlResourceLoader> pLoader(m_pWinrtLoader);

    IFC(resourceId.Promote(&strResourceId));
    IFC(objectType.Promote(&strObjectType));
    IFC(propertyName.Promote(&strPropertyName));
    IFC(propertyType.Promote(&strPropertyType));

    IFC(pLoader.As(&pOverrides));
    IFC(pOverrides->GetResource(strResourceId.GetHSTRING(),
                                strObjectType.GetHSTRING(),
                                strPropertyName.GetHSTRING(),
                                strPropertyType.GetHSTRING(),
                                &pInspectable));
    IFC(CValueBoxer::BoxObjectValue(&outValue, nullptr, pInspectable, &buffer, &pMOR));
    IFC(pValue->CopyConverted(outValue));

    //
    // since the object was not created by the Parser (the GetResource call may have created it for the first time),
    // we explicitly do a PegNoRef on the pMOR, so that it stays alive during parse.
    //
    // The PegNoRef will be removed when this gets added to the tree or
    // has an RCW or when the XQO destructor runs.
    //
    if (pMOR)
    {
        pMOR->PegNoRef();
    }

Cleanup:
    ReleaseInterface(pInspectable);
    ctl::release_interface(pMOR);
    RRETURN(hr);
}

xaml::Resources::ICustomXamlResourceLoader* CustomXamlResourceLoaderWrapper::GetLoader()
{
    return m_pWinrtLoader;
}
