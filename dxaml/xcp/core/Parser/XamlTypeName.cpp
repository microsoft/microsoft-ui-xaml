// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

HRESULT XamlTypeName::Parse(
            _In_ const xstring_ptr_view& inLongName, 
            std::shared_ptr<XamlTypeName>& outTypeName)
{
    return Parse(xstring_ptr::NullString(), inLongName, outTypeName);
}

HRESULT XamlTypeName::Parse(
            _In_ const xstring_ptr_view& inPrefix,
            _In_ const xstring_ptr_view& inName, 
            std::shared_ptr<XamlTypeName>& outTypeName)
{
    XamlTypeNameParser parser(inName);

    IFC_RETURN(parser.ParseXamlTypeName(inPrefix, outTypeName));
    return S_OK;
}

HRESULT XamlTypeName::get_ScopedName(_Out_ xstring_ptr* pstrOutPrefix)
{
    HRESULT hr = S_OK;

    if (m_ssScopedName.IsNull())
    {
        // Cache it on first access
        if (m_ssPrefix.GetCount() == 0)
        {
            // Name
            m_ssScopedName = m_ssName;
        }
        else
        {
            // Prefix:Name
            XStringBuilder scopedNameBuilder;

            IFC(scopedNameBuilder.Initialize(m_ssPrefix.GetCount() + 1 + m_ssName.GetCount()));
            IFC(scopedNameBuilder.Append(m_ssPrefix));
            IFC(scopedNameBuilder.AppendChar(L':'));
            IFC(scopedNameBuilder.Append(m_ssName));

            IFC(scopedNameBuilder.DetachString(&m_ssScopedName));
        }
    }

    *pstrOutPrefix = m_ssScopedName;

Cleanup:
    if (FAILED(hr))
    {
        m_ssScopedName.Reset();
    }

    RRETURN(hr);
}

HRESULT XamlTypeNameParser::ParseXamlTypeName(
            _In_ const xstring_ptr_view& inPrefix, 
            std::shared_ptr<XamlTypeName>& outTypeName)
{
    XUINT32 uStartIndex = m_uIndex;
    xephemeral_string_ptr ssPrefix;
    xephemeral_string_ptr ssName;
    xstring_ptr strPrefix;
    xstring_ptr strName;

    if (!Seq(uStartIndex, (inPrefix.IsNullOrEmpty() && ParsePrefix(&ssPrefix) && ParseUndottedName(&ssName))
                ||    (ParseUndottedName(&ssName))))
    {
        IFC_RETURN(E_FAIL);
    }

    if (ssPrefix.IsNull())
    {
        if (!inPrefix.IsNull())
        {
            IFC_RETURN(inPrefix.Promote(&strPrefix));
        }
        else
        {
            strPrefix = xstring_ptr::EmptyString();
        }
    }
    else
    {
        IFC_RETURN(ssPrefix.Promote(&strPrefix));
    }

    IFC_RETURN(ssName.Promote(&strName));

    outTypeName = std::make_shared<XamlTypeName>(strPrefix, strName);

    return S_OK;
}



    

