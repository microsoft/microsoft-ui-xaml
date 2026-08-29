// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NamespaceAliases.h>
#include <Microsoft.UI.Xaml.coretypes.h>
#include <vector>
#include <ExternalDependency.h>

namespace Private
{
    struct XamlTypeName;
    struct UserTypeInfo;
    struct UserMemberInfo;
    struct UserEnumInfo;
    struct UserEnumValueInfo;
    class UserMetadataCallbacks;

    class XamlTypeInfoProvider
    {
    public:
        XamlTypeInfoProvider(PCWSTR moduleName);
        XamlTypeInfoProvider(const XamlTypeInfoProvider& other) = delete;
        XamlTypeInfoProvider(const XamlTypeInfoProvider&& other) noexcept :
            m_cXamlTypeNameTableSize(other.m_cXamlTypeNameTableSize),
            m_cXamlMemberTableSize(other.m_cXamlMemberTableSize),
            m_aspXamlTypeDataCache(std::move(other.m_aspXamlTypeDataCache)),
            m_aspXamlMemberDataCache(std::move(other.m_aspXamlMemberDataCache)),
            m_aXamlTypeNames(other.m_aXamlTypeNames),
            m_aUserTypes(other.m_aUserTypes),
            m_pMemberIndices(other.m_pMemberIndices),
            m_aUserMembers(other.m_aUserMembers),
            m_aUserEnums(other.m_aUserEnums),
            m_pStrings(other.m_pStrings),
            m_hResourceModule(other.m_hResourceModule),
            m_moduleName(other.m_moduleName),
            m_pfnTelemetryProc(other.m_pfnTelemetryProc)
        {
        }

#if _MSC_VER >= 1900
        XamlTypeInfoProvider& operator=(XamlTypeInfoProvider&&) = default;
#endif

        _Check_return_ HRESULT Initialize(_In_ PFNTELEMETRYPROC pfnTelemetryProc);

        ~XamlTypeInfoProvider();

    public:
        HRESULT FindXamlType_FromHSTRING(
            _In_ HSTRING hTypeName,
            _Out_ wrl::ComPtr<xaml_markup::IXamlType>* pspXamlType);

        HRESULT FindXamlType_FromInt32(
            INT32 iTypeName,
            _Out_ wrl::ComPtr<xaml_markup::IXamlType>* pspXamlType);

        HRESULT FindXamlMember_FromInt32(
            INT32 iTypeName,
            _Out_ wrl::ComPtr<xaml_markup::IXamlMember>* pspXamlType);

        const Private::XamlTypeName*    GetXamlTypeName(INT32 iTypeName);
        const Private::UserTypeInfo*    GetUserTypeInfo(INT32 iUserInfo);
        const Private::UserMemberInfo*  GetUserMemberInfo(INT32 iMember);
        const Private::UserEnumInfo*    GetUserEnumInfo(INT32 iEnumIndex);

        const INT32* GetMemberIndexTable();

        _Check_return_ HRESULT GetString(_In_ UINT16 ids, _Outptr_ PCWSTR* string, _Out_ size_t* size) const;

        _Check_return_ HRESULT GetString(_In_ UINT16 ids, _Outptr_ PCWSTR* string) const
        {
            size_t size;
            return GetString(ids, string, &size);
        }

        _Check_return_ HRESULT GetHString(_In_ UINT16 ids, _Outptr_ HSTRING* string) const
        {
            wrl_wrappers::HString name;

            IFCPTRRC_RETURN(string, E_INVALIDARG);

            PCWSTR buffer;
            IFC_RETURN(GetString(ids, &buffer));
            IFC_RETURN(name.Set(buffer));

            (*string) = name.Detach();

            return S_OK;
        }

        _Check_return_ HRESULT ActivateInstance(_In_ UINT16 typeId, _Outptr_ IInspectable **instance) const;

        _Check_return_ HRESULT GetValue(_In_ UINT16 getFuncId, _In_ IInspectable* instance, _Outptr_ IInspectable **value) const;

        _Check_return_ HRESULT SetValue(_In_ UINT16 setFuncId, _In_ IInspectable* instance, _In_ IInspectable *value) const;

        _Check_return_ HRESULT BoxEnum(_In_ UINT16 enumBoxerId, _In_ UINT32 enumValue, _Outptr_ IInspectable **ppBoxedEnum) const;

        _Check_return_ HRESULT AddToVector(_In_ UINT16 addToVectorId, _In_ IInspectable* instance, _In_ IInspectable* value) const;

        _Check_return_ HRESULT AddToMap(_In_ UINT16 addToMapId, _In_ IInspectable* instance, _In_ IInspectable* key, _In_ IInspectable* value) const;

        _Check_return_ HRESULT GetUserEnumValues(_In_ const Private::UserEnumInfo& userEnumInfo, _Outptr_ const Private::UserEnumValueInfo** ppValues) const;
        
        _Check_return_ HRESULT EnsureDependencyProperties(_In_ UINT16 typeId) const;

        void ResetDependencyProperties() const;

    private:

        template<class T>
        _Check_return_ HRESULT LoadFromResource(_In_ UINT resId, _In_ UINT typeId, _Outptr_ const T** ppData, _Out_ size_t* size) const;

        template<class T>
        _Check_return_ HRESULT LoadFromResource(_In_ UINT resId, _In_ UINT typeId, _Outptr_ const T** ppData) const
        {
            size_t size;
            return LoadFromResource<T>(resId, typeId, ppData, &size);
        }

        size_t m_cXamlTypeNameTableSize{};
        size_t m_cXamlMemberTableSize{};

        std::vector<wrl::ComPtr<xaml_markup::IXamlType>> m_aspXamlTypeDataCache;
        std::vector<wrl::ComPtr<xaml_markup::IXamlMember>> m_aspXamlMemberDataCache;

        const Private::XamlTypeName *m_aXamlTypeNames;
        const Private::UserTypeInfo *m_aUserTypes;
        const INT32 *m_pMemberIndices;
        const Private::UserMemberInfo *m_aUserMembers;
        const Private::UserEnumInfo *m_aUserEnums;
        const UINT16 *m_pStrings;
        HINSTANCE m_hResourceModule;
        PCWSTR m_moduleName;
        PFNTELEMETRYPROC m_pfnTelemetryProc;
    };
}

