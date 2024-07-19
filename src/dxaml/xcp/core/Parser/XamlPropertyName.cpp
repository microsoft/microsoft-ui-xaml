// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"


HRESULT XamlPropertyName::Parse(
    _In_ const xstring_ptr_view& spPrefix,
    _In_ const xstring_ptr_view& spLongName,
    _Out_ std::shared_ptr<XamlPropertyName>& spPropertyName)
{
    XamlPropertyNameParser parser(spLongName);
    IFC_RETURN(parser.ParseXamlPropertyName(spPrefix, spPropertyName));

    return S_OK;
}

HRESULT XamlPropertyName::get_ScopedOwnerName(_Out_ xstring_ptr* pstrOutScopedOwnerName)
{
    if (m_Owner)
    {
        IFC_RETURN(m_Owner->get_ScopedName(pstrOutScopedOwnerName));
    }
    else
    {
        *pstrOutScopedOwnerName = xstring_ptr::EmptyString();
    }

    return S_OK;
}

HRESULT XamlPropertyName::get_FullName(_Out_ xstring_ptr* pstrOutFullName)
{
    HRESULT hr = S_OK;

    if (m_ssFullName.IsNull())
    {
        if (PrivateIsDotted())
        {
            // OwnersName.Name
            XStringBuilder fullNameBuilder;

            IFC(fullNameBuilder.Initialize(m_Owner->get_Name().GetCount() + 1 + m_ssName.GetCount()));
            IFC(fullNameBuilder.Append(m_Owner->get_Name()));
            IFC(fullNameBuilder.AppendChar(L'.'));
            IFC(fullNameBuilder.Append(m_ssName));
            IFC(fullNameBuilder.DetachString(&m_ssFullName));
        }
        else
        {
            m_ssFullName = m_ssName;
        }
    }

    *pstrOutFullName = m_ssFullName;

Cleanup:
    if (FAILED(hr))
    {
        m_ssFullName.Reset();
    }

    RRETURN(hr);
}

HRESULT XamlPropertyName::get_ScopedName(_Out_ xstring_ptr* pstrOutScopedName)
{
    HRESULT hr = S_OK;

    if (m_ssScopedName.IsNull())
    {
        xstring_ptr ssScopedName;

        if (PrivateIsDotted())
        {
            IFC(m_Owner->get_ScopedName(&ssScopedName));

            // OwnersScopedName.Name
            XStringBuilder scopedNameBuilder;

            IFC(scopedNameBuilder.Initialize(ssScopedName.GetCount() + 1 + m_ssName.GetCount()));
            IFC(scopedNameBuilder.Append(ssScopedName));
            IFC(scopedNameBuilder.AppendChar(L'.'));
            IFC(scopedNameBuilder.Append(m_ssName));
            IFC(scopedNameBuilder.DetachString(&m_ssScopedName));
        }
        else
        {
            // Name
            m_ssScopedName = m_ssName;
        }
    }

    *pstrOutScopedName = m_ssScopedName;

Cleanup:
    if (FAILED(hr))
    {
        m_ssScopedName.Reset();
    }

    RRETURN(hr);
}

HRESULT XamlPropertyName::get_OwnerName(_Out_ xstring_ptr* pstrOutOwnerName)
{
    // TODO: Can m_Owner be forced to empty somewhere else and return const ref here?
    if (m_Owner)
    {
        *pstrOutOwnerName = m_Owner->get_Name();
    }
    else
    {
        *pstrOutOwnerName = xstring_ptr::EmptyString();
    }

    RRETURN(S_OK);
}

HRESULT XamlPropertyName::get_OwnerPrefix(_Out_ xstring_ptr* pstrOutOwnerPrefix)
{
    // TODO: Can m_Owner be forced to empty somewhere else and return const ref here?
    if (m_Owner)
    {
        *pstrOutOwnerPrefix = m_Owner->get_Prefix();
    }
    else
    {
        *pstrOutOwnerPrefix = xstring_ptr::EmptyString();
    }

    RRETURN(S_OK);
}

HRESULT XamlPropertyName::get_Owner(std::shared_ptr<XamlName>& outOwner)
{
    if (m_Owner)
    {
        outOwner = m_Owner;
    }

    RRETURN(S_OK);
}

HRESULT XamlPropertyName::get_IsDotted(bool& isDotted)
{
    // If there's an owner (there's an implicit cast of smart_ptr to BOOL)
    isDotted = !!m_Owner;
    RRETURN(S_OK);
}


//private:
XamlPropertyName::XamlPropertyName(_In_ const xstring_ptr& inPrefix, _In_ const xstring_ptr& inName)
{
    std::shared_ptr<XamlTypeName> nullOwner;
    Init(nullOwner, inPrefix, inName);
}

XamlPropertyName::XamlPropertyName(const std::shared_ptr<XamlName>& inOwner, _In_ const xstring_ptr& inName)
{
    Init(inOwner, inOwner->get_Prefix(), inName);
}

XamlPropertyName::XamlPropertyName(_In_ const xstring_ptr& inName)
{
    xstring_ptr emptyPrefix(xstring_ptr::EmptyString());
    std::shared_ptr<XamlTypeName> nullOwner;
    
    // This will be verified in the static method.
    Init(nullOwner, emptyPrefix, inName);
}

XamlPropertyName::XamlPropertyName(const std::shared_ptr<XamlName>& inOwner, _In_ const xstring_ptr& inPrefix, _In_ const xstring_ptr& inName)
{
    Init(inOwner, inPrefix, inName);
}

void XamlPropertyName::Init(const std::shared_ptr<XamlName>& inOwner, _In_ const xstring_ptr& inPrefix, _In_ const xstring_ptr& inName)
{
    m_Owner = inOwner;
    m_ssPrefix = inPrefix;
    m_ssName = inName;
}


// ---------------------------------------------------------

HRESULT XamlPropertyNameParser::ParseXamlPropertyName(_In_ const xstring_ptr_view& inPrefix, std::shared_ptr<XamlPropertyName>& outPropertyName)
{
    xephemeral_string_ptr ssPrefix;
    xephemeral_string_ptr ssOwner;
    xephemeral_string_ptr ssProperty;
    XUINT32 startIndex = m_uIndex;

    // Our parser can take the prefix separately itself. So pass it in.
    if(
            ((Seq(startIndex, ParseProperty(&ssOwner, &ssProperty)))
        ||  (Seq(startIndex, inPrefix.IsNullOrEmpty() && ParsePrefix(&ssPrefix) && ParseProperty(&ssOwner, &ssProperty))))
        &&  (ParseEmpty()))
    {
        // Use the supplied index if there is one.
        if (!inPrefix.IsNullOrEmpty())
        {
            // Shouldn't have been able to parse
            // a value into here if we supplied the prefix.
            ASSERT(ssPrefix.IsNull());
            inPrefix.Demote(&ssPrefix);
        }

        if (ssPrefix.IsNull())
        {
            xstring_ptr::EmptyString().Demote(&ssPrefix);
        }

        xstring_ptr strPrefix;
        xstring_ptr strProperty;

        IFC_RETURN(ssPrefix.Promote(&strPrefix));
        IFC_RETURN(ssProperty.Promote(&strProperty));

        if (ssOwner.GetCount() == 0)
        {
            outPropertyName = std::make_shared<XamlPropertyName>(strPrefix, strProperty);
        }
        else
        {
            xstring_ptr strOwner;
            IFC_RETURN(ssOwner.Promote(&strOwner));
            auto qualifiedTypeName = std::make_shared<XamlQualifiedName>(strPrefix, strOwner);
            outPropertyName = std::make_shared<XamlPropertyName>(qualifiedTypeName, strProperty);
        }
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}


bool XamlPropertyNameParser::ParseProperty(_Out_ xephemeral_string_ptr* pstrOutssOwner, _Out_ xephemeral_string_ptr* pstrOutssProperty)
{
    return ParseUndottedName(pstrOutssOwner, pstrOutssProperty) 
                || ParseSingleDottedName(pstrOutssOwner, pstrOutssProperty);
}


