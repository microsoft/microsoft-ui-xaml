// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CustomXamlResourceLoader.g.h"
#include "CustomXamlResourceLoaderWrapper.h"

namespace DirectUI
{
    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Default implementation of overridable GetResource--fails with E_NOTIMPL.
    //
    //---------------------------------------------------------------------------
    _Check_return_ HRESULT CustomXamlResourceLoader::GetResourceImpl(
        _In_ HSTRING resourceId,
        _In_ HSTRING objectType,
        _In_ HSTRING propertyName,
        _In_ HSTRING propertyType,
        _Outptr_ IInspectable** returnValue
        )
    {
        RRETURN(E_NOTIMPL);
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Retrieves the current CustomXamlResourceLoader, if any.
    //
    //---------------------------------------------------------------------------
    _Check_return_ HRESULT CustomXamlResourceLoaderFactory::get_CurrentImpl(_Outptr_ xaml::Resources::ICustomXamlResourceLoader **ppValue)
    {
        HRESULT hr = S_OK;
        ICustomResourceLoader *pInternalLoader = NULL;
        CustomXamlResourceLoaderWrapper *pLoaderWrapper = NULL;

        IFCPTR(ppValue);
        *ppValue = NULL;

        IFC(CoreImports::CoreServices_GetCustomResourceLoader(DXamlCore::GetCurrent()->GetHandle(), &pInternalLoader));
        if (pInternalLoader)
        {
            pLoaderWrapper = static_cast<CustomXamlResourceLoaderWrapper*>(pInternalLoader);

            *ppValue = pLoaderWrapper->GetLoader();
            AddRefInterface(*ppValue);
        }

    Cleanup:
        ReleaseInterface(pLoaderWrapper);
        RRETURN(hr);
    }

    //---------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Sets the current CustomXamlResourceLoader.
    //
    //---------------------------------------------------------------------------
    _Check_return_ HRESULT CustomXamlResourceLoaderFactory::put_CurrentImpl(_In_ xaml::Resources::ICustomXamlResourceLoader *pValue)
    {
        HRESULT hr = S_OK;
        CustomXamlResourceLoaderWrapper *pLoaderWrapper = NULL;

        if (pValue)
        {
            pLoaderWrapper = new CustomXamlResourceLoaderWrapper(pValue);
        }

        IFC(CoreImports::CoreServices_SetCustomResourceLoader(DXamlCore::GetCurrent()->GetHandle(), pLoaderWrapper));

    Cleanup:
        ReleaseInterface(pLoaderWrapper);
        RRETURN(hr);
    }
}

