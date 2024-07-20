// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "UriValidator.h"

namespace UriValidator
{
    _Check_return_
    HRESULT Validate(
        _In_ CCoreServices *pCore,
        _In_ const CValue &value
        )
    {
        xstring_ptr uriString = value.AsString();

        if (value.IsNullOrUnset())
        {
            return S_OK;
        }

        if (!uriString.IsNullOrEmpty())
        {
            IFC_RETURN(Validate(pCore, uriString));
            return S_OK;
        }

        if (value.AsObject())
        {
            CString *pString = NULL;
            IFC_RETURN(DoPointerCast(pString, value.AsObject()));
            IFC_RETURN(Validate(pCore, pString->m_strString));
        } 

        return S_OK;
    }

    _Check_return_
    HRESULT Validate(
        _In_ CCoreServices *pCore,
        _In_ const xstring_ptr& uriString
        )
    {
        IPALUri *pBaseUri = pCore->GetBaseUriNoRef();
        IFCEXPECT_ASSERT_RETURN(pBaseUri != NULL);
        
        // To validate the URI, we attempt to combine it with the base URI. If it fails,
        // it's invalid.
        wrl::ComPtr<IPALUri> pCombinedUri;
        IFC_RETURN(pBaseUri->Combine(uriString.GetCount(), uriString.GetBuffer(), &pCombinedUri));

        return S_OK;
    }
}
