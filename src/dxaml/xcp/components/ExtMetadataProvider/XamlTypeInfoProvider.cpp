// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlTypeInfoProvider.h"
#include <XamlTypeInfo.h>
#include "XamlType.h"
#include "XamlMember.h"
#include "LoadLibraryAbs.h"

namespace Private
{
    XamlTypeInfoProvider::XamlTypeInfoProvider(PCWSTR moduleName) :
        m_aXamlTypeNames(nullptr),
        m_aUserTypes(nullptr),
        m_pMemberIndices(nullptr),
        m_aUserMembers(nullptr),
        m_aUserEnums(nullptr),
        m_pStrings(nullptr),
        m_hResourceModule(nullptr),
        m_moduleName(moduleName),
        m_pfnTelemetryProc(nullptr)
    {
    }

    XamlTypeInfoProvider::~XamlTypeInfoProvider()
    {
    }

    _Check_return_ HRESULT XamlTypeInfoProvider::GetString(_In_ UINT16 ids, _Outptr_ PCWSTR* string, _Out_ size_t* size) const
    {
        auto data = m_pStrings + ids;
        IFCPTR_RETURN(data);

        (*size) = data[0];
        (*string) = reinterpret_cast<PCWSTR>(data + 1);

        return S_OK;
    }

    template<class T>
    _Check_return_ HRESULT
    XamlTypeInfoProvider::LoadFromResource(
        _In_ UINT resId,
        _In_ UINT typeId,
        _Outptr_ const T** ppData,
        _Out_ size_t* size) const
    {
        auto resInfo = FindResource(m_hResourceModule, MAKEINTRESOURCE(resId), MAKEINTRESOURCE(typeId));
        IFCHNDL_RETURN(resInfo);

        auto resSize = SizeofResource(m_hResourceModule, resInfo);

        auto res = LoadResource(m_hResourceModule, resInfo);
        IFCHNDL_RETURN(res);

        auto data = LockResource(res);
        IFCPTR_RETURN(data);

        (*ppData) = reinterpret_cast<const T*>(data);
        (*size) = resSize / sizeof(T);

        return S_OK;
    }

    _Check_return_ HRESULT
    XamlTypeInfoProvider::GetUserEnumValues(
        _In_ const Private::UserEnumInfo& userEnumInfo,
        _Outptr_ const Private::UserEnumValueInfo** ppValues) const
    {
        return LoadFromResource<Private::UserEnumValueInfo>(userEnumInfo.valuesId, USERENUMVALUEINFO, ppValues);
    }

    _Check_return_ HRESULT
    XamlTypeInfoProvider::Initialize(_In_ PFNTELEMETRYPROC pfnTelemetryProc)
    {
        m_pfnTelemetryProc = pfnTelemetryProc;
    
        HRESULT hr = S_OK;

        if ( m_hResourceModule != nullptr )
        {
            goto Cleanup;
        }

        m_hResourceModule = LoadLibraryExWAbs(
            m_moduleName,
            nullptr,
            LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
        IFCHNDL(m_hResourceModule);

        IFC(LoadFromResource<Private::XamlTypeName>(XAMLTYPENAME, XAMLTYPENAMES, &m_aXamlTypeNames, &m_cXamlTypeNameTableSize));
        IFC(LoadFromResource<Private::UserTypeInfo>(USERTYPEINFO, USERTYPES, &m_aUserTypes));
        IFC(LoadFromResource<Private::UserMemberInfo>(USERMEMBERINFO, USERMEMBERS, &m_aUserMembers, &m_cXamlMemberTableSize));
        IFC(LoadFromResource<INT32>(MEMBERINDICE, MEMBERINDICES, &m_pMemberIndices));
        IFC(LoadFromResource<Private::UserEnumInfo>(USERENUMINFO, USERENUMS, &m_aUserEnums));
        IFC(LoadFromResource<UINT16>(XAMLSTRING, XAMLSTRINGS, &m_pStrings));

        m_aspXamlTypeDataCache.resize(m_cXamlTypeNameTableSize);
        m_aspXamlMemberDataCache.resize(m_cXamlMemberTableSize);

    Cleanup:
        if ( FAILED(hr) )
        {
            m_hResourceModule = nullptr;
        }

        return hr;
    }

    HRESULT
    XamlTypeInfoProvider::FindXamlType_FromHSTRING(
        _In_ HSTRING hTypeName,
        _Out_ wrl::ComPtr<xaml_markup::IXamlType>* pspXamlType)
    {
        wrl::ComPtr<xaml_markup::IXamlType> spType;
        PCWSTR pszTypeName = WindowsGetStringRawBuffer(hTypeName, NULL);

        INT32 iType = -1;

        IFC_RETURN(Initialize(m_pfnTelemetryProc));

        for (UINT32 i=0; i < m_cXamlTypeNameTableSize; i++)
        {
            PCWSTR pszFullName = nullptr;
            IFC_RETURN(GetString(m_aXamlTypeNames[i].idsName, &pszFullName));

            if (0 == wcscmp(pszTypeName, pszFullName))
            {
                iType = i;
                break;
            }
        }

        // If we couldn't find the type, we'll end up returning null;
        // however, this is not a failure.
        if (-1 != iType)
        {
            IFC_NOTRACE_RETURN(FindXamlType_FromInt32(iType, &spType));
        }

        *pspXamlType = spType;

        return S_OK;
    }

    HRESULT
    XamlTypeInfoProvider::FindXamlType_FromInt32(
        INT32 iType,
        _Out_ wrl::ComPtr<xaml_markup::IXamlType>* pspXamlType)
    {
        wrl::ComPtr<xaml_markup::IXamlType> spType;

        IFC_RETURN(Initialize(m_pfnTelemetryProc));

        // return the cached copy or cache the new one.
        spType = m_aspXamlTypeDataCache[iType];

        if (!spType)
        {
            spType = wrl::Make<Internal::XamlType>(iType, this);


            m_aspXamlTypeDataCache[iType] = spType;
        }

        *pspXamlType = spType;

        return S_OK;
    }

    HRESULT
    XamlTypeInfoProvider::FindXamlMember_FromInt32(
        INT32 iMember,
        _Out_ wrl::ComPtr<xaml_markup::IXamlMember>* pspXamlMember)
    {
        wrl::ComPtr<xaml_markup::IXamlMember> spMember;

        IFC_RETURN(Initialize(m_pfnTelemetryProc));

        // return the cached copy or cache the new one.
        spMember = m_aspXamlMemberDataCache[iMember];

        if (!spMember)
        {
            spMember = wrl::Make<Internal::XamlMember>(iMember, this);


            m_aspXamlMemberDataCache[iMember] = spMember;
        }

        *pspXamlMember = spMember;

        return S_OK;
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
        return &m_aUserEnums[iEnumIndex];
    }

    const INT32*
    XamlTypeInfoProvider::GetMemberIndexTable()
    {
        return m_pMemberIndices;
    }

    _Check_return_ HRESULT
    XamlTypeInfoProvider::ActivateInstance(_In_ UINT16 typeId, _Outptr_ IInspectable **instance) const
    {
        DependencyLocator::ExternalDependency<XamlRuntimeType> metadata(m_moduleName, m_pfnTelemetryProc);
        return metadata->ActivateInstance(typeId, instance);
    }

    _Check_return_ HRESULT
    XamlTypeInfoProvider::GetValue(_In_ UINT16 getFuncId, _In_ IInspectable* instance, _Outptr_ IInspectable **value) const
    {
        DependencyLocator::ExternalDependency<XamlRuntimeType> metadata(m_moduleName, m_pfnTelemetryProc);
        return metadata->GetValue(getFuncId, instance, value);
    }

    _Check_return_ HRESULT
    XamlTypeInfoProvider::SetValue(_In_ UINT16 setFuncId, _In_ IInspectable* instance, _In_ IInspectable *value) const
    {
        DependencyLocator::ExternalDependency<XamlRuntimeType> metadata(m_moduleName, m_pfnTelemetryProc);
        return metadata->SetValue(setFuncId, instance, value);
    }

    _Check_return_ HRESULT
    XamlTypeInfoProvider::BoxEnum(_In_ UINT16 enumBoxerId, _In_ UINT32 enumValue, _Outptr_ IInspectable **ppBoxedEnum) const
    {
        DependencyLocator::ExternalDependency<XamlRuntimeType> metadata(m_moduleName, m_pfnTelemetryProc);
        return metadata->BoxEnum(enumBoxerId, enumValue, ppBoxedEnum);
    }

    _Check_return_ HRESULT
    XamlTypeInfoProvider::AddToVector(_In_ UINT16 addToVectorId, _In_ IInspectable* instance, _In_ IInspectable* value) const
    {
        DependencyLocator::ExternalDependency<XamlRuntimeType> metadata(m_moduleName, m_pfnTelemetryProc);
        return metadata->AddToVector(addToVectorId, instance, value);
    }

    _Check_return_ HRESULT
    XamlTypeInfoProvider::AddToMap(_In_ UINT16 addToMapId, _In_ IInspectable* instance, _In_ IInspectable* key, _In_ IInspectable* value) const
    {
        DependencyLocator::ExternalDependency<XamlRuntimeType> metadata(m_moduleName, m_pfnTelemetryProc);
        return metadata->AddToMap(addToMapId, instance, key, value);
    }

    _Check_return_ HRESULT
    XamlTypeInfoProvider::EnsureDependencyProperties(_In_ UINT16 typeId) const
    {
        DependencyLocator::ExternalDependency<XamlRuntimeType> metadata(m_moduleName, m_pfnTelemetryProc);
        return metadata->EnsureDependencyProperties(typeId);
    }

    void
    XamlTypeInfoProvider::ResetDependencyProperties() const
    {
        DependencyLocator::ExternalDependency<XamlRuntimeType> metadata(m_moduleName, m_pfnTelemetryProc);

        if (metadata.IsInitialized())
        {
            metadata->ResetDependencyProperties();
        }
    }
}

