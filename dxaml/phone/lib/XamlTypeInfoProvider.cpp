// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

namespace Private
{
    XamlTypeInfoProvider::XamlTypeInfoProvider()
    {
    }

    XamlTypeInfoProvider::~XamlTypeInfoProvider()
    {
    }

    /* static */ HRESULT
    XamlTypeInfoProvider::Create(
        _Outptr_ XamlTypeInfoProvider** initializedProvider)
    {
        HRESULT hr = S_OK;

        auto provider = new XamlTypeInfoProvider();

        IFCOOM(provider);

        IFC(provider->Initialize());

        *initializedProvider = provider;
        provider = nullptr;

    Cleanup:
        delete provider;

        return hr;
    }

    HRESULT
    XamlTypeInfoProvider::Initialize()
    {
        HRESULT hr = S_OK;

        m_cXamlTypeNameTableSize = Private::Generated::XamlTypeNameTableSize;
        m_cXamlMemberTableSize = Private::Generated::UserMemberTableSize;

        m_aspXamlTypeDataCache =
            new wrl::ComPtr<xaml_markup::IXamlType>[
                m_cXamlTypeNameTableSize];

        IFCOOM(m_aspXamlTypeDataCache);

        m_aspXamlMemberDataCache =
            new wrl::ComPtr<xaml_markup::IXamlMember>[
                m_cXamlMemberTableSize];

        IFCOOM(m_aspXamlMemberDataCache);

        m_aXamlTypeNames = &Private::Generated::XamlTypeNames[0];
        m_aUserTypes = &Private::Generated::UserTypes[0];
        m_pMemberIndices = &Private::Generated::MemberIndices[0];
        m_aUserMembers = &Private::Generated::UserMembers[0];
        m_aUserEnums = &Private::Generated::UserEnums[0];

    Cleanup:
        return hr;
    }

    HRESULT
    XamlTypeInfoProvider::FindXamlType_FromHSTRING(
        _In_ HSTRING hTypeName,
        _Out_ wrl::ComPtr<xaml_markup::IXamlType>* pspXamlType)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_markup::IXamlType> spType;
        PCWSTR pszTypeName = WindowsGetStringRawBuffer(hTypeName, NULL);

        INT32 iType = -1;
        for (INT32 i=0; i < m_cXamlTypeNameTableSize; i++)
        {
            if (0 == wcscmp(pszTypeName, m_aXamlTypeNames[i].pszFullName))
            {
                iType = i;
                break;
            }
        }

        // If we couldn't find the type, we'll end up returning null;
        // however, this is not a failure.
        if (-1 != iType)
        {
            IFC_NOTRACE(FindXamlType_FromInt32(iType, &spType));
        }

        *pspXamlType = spType;

    Cleanup:
        return hr;
    }

    HRESULT
    XamlTypeInfoProvider::FindXamlType_FromInt32(
        INT32 iType,
        _Out_ wrl::ComPtr<xaml_markup::IXamlType>* pspXamlType)
    {
        HRESULT hr = S_OK;

        // return the cached copy or cache the new one.
        wrl::ComPtr<xaml_markup::IXamlType> spType =
            m_aspXamlTypeDataCache[iType];

        if (!spType)
        {
            spType = wrl::Make<Internal::XamlType>(iType, this);

            IFCOOM(spType);

            m_aspXamlTypeDataCache[iType] = spType;
        }

        *pspXamlType = spType;

    Cleanup:
        return hr;
    }

    HRESULT
    XamlTypeInfoProvider::FindXamlMember_FromInt32(
        INT32 iMember,
        _Out_ wrl::ComPtr<xaml_markup::IXamlMember>* pspXamlMember)
    {
        HRESULT hr = S_OK;

        // return the cached copy or cache the new one.
        wrl::ComPtr<xaml_markup::IXamlMember> spMember =
            m_aspXamlMemberDataCache[iMember];

        if (!spMember)
        {
            spMember = wrl::Make<Internal::XamlMember>(iMember, this);

            IFCOOM(spMember);

            m_aspXamlMemberDataCache[iMember] = spMember;
        }

        *pspXamlMember = spMember;

    Cleanup:
        return hr;
    }

    const Private::XamlTypeName*
    XamlTypeInfoProvider::GetXamlTypeName(INT32 iType)
    {
        return &m_aXamlTypeNames[iType];
    }

    const Private::UserTypeInfo*
    XamlTypeInfoProvider::GetUserTypeInfo(INT32 iUserType)
    {
        return &m_aUserTypes[iUserType];
    }

    const Private::UserMemberInfo*
    XamlTypeInfoProvider::GetUserMemberInfo(INT32 iMember)
    {
        return &m_aUserMembers[iMember];
    }

    const Private::UserEnumInfo*
    XamlTypeInfoProvider::GetUserEnumInfo(INT32 iEnumIndex)
    {
        ASSERT(iEnumIndex >= 0 && iEnumIndex < Private::Generated::UserEnumTableSize);
        return &m_aUserEnums[iEnumIndex];
    }

    const INT32*
    XamlTypeInfoProvider::GetMemberIndexTable()
    {
        return m_pMemberIndices;
    }
}

