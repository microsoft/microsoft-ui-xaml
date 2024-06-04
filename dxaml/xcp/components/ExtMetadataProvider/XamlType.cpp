// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlType.h"
#include "XamlTypeInfoProvider.h"
#include "palexports.h"

#include <XamlTypeInfo.h>
#include <regex>
#include "XcpAllocationDebug.h"

namespace Internal
{
    XamlType::XamlType(
        UINT typeIndex,
        _In_ Private::XamlTypeInfoProvider* typeInfoProvider)
        : m_pUserTypeData(nullptr)
    {
        m_pTypeInfoProvider = typeInfoProvider;
        m_pTypeName = m_pTypeInfoProvider->GetXamlTypeName(typeIndex);

        if (m_pTypeName->iUserType >= 0)
        {
            m_pUserTypeData = m_pTypeInfoProvider->GetUserTypeInfo(m_pTypeName->iUserType);

            if (m_pUserTypeData->iMembers >= 0)
            {
                const INT32* memberIndices = m_pTypeInfoProvider->GetMemberIndexTable();

                m_piMemberIndices = &(memberIndices[m_pUserTypeData->iMembers]);
            }
        }
    }

    IFACEMETHODIMP
    XamlType::get_BaseType(
        _COM_Outptr_ xaml_markup::IXamlType** value)
    {
        wrl::ComPtr<xaml_markup::IXamlType> xamlType;

        IFCPTRRC_RETURN(value, E_INVALIDARG);

        // Enums and structs don't have a base type.
        // The metadatastore will assign them INDEX_OBJECT.
        if(!IsSystemType() && m_pUserTypeData->iBaseType >= 0)
        {
            IFC_RETURN(m_pTypeInfoProvider->FindXamlType_FromInt32(
                m_pUserTypeData->iBaseType,
                &xamlType));
        }

        *value = xamlType.Detach();

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::get_ContentProperty(
        _COM_Outptr_result_maybenull_ xaml_markup::IXamlMember** value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_markup::IXamlMember> xamlMember;

        IFCPTRRC(value, E_INVALIDARG);

        if (!IsSystemType() && m_pUserTypeData->iContentProperty >= 0)
        {
            IFC(m_pTypeInfoProvider->FindXamlMember_FromInt32(
                m_pUserTypeData->iContentProperty,
                &xamlMember));
        }
        else
        {
            //
            // From MSDN: IXamlMember information for the XAML content property
            // of the IXamlType. May be null if no XAML content property exists.
            //
            ASSERT(hr == S_OK && !xamlMember);
        }

        *value = xamlMember.Detach();

    Cleanup:
        return hr;
    }

    IFACEMETHODIMP
    XamlType::get_FullName(
        _Outptr_ HSTRING* value)
    {
        HRESULT hr = S_OK;

        IFCPTRRC(value, E_INVALIDARG);

        IFC(m_pTypeInfoProvider->GetHString(m_pTypeName->idsName, value))

    Cleanup:
        return hr;
    }

    IFACEMETHODIMP
    XamlType::get_IsArray(
        _Out_ BOOLEAN* value)
    {
        IFCPTRRC_RETURN(value, E_INVALIDARG);

        *value = IsSystemType() ? FALSE : static_cast<BOOLEAN>(m_pUserTypeData->isArray);

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::get_IsCollection(
        _Out_ BOOLEAN* value)
    {
        IFCPTRRC_RETURN(value, E_INVALIDARG);

        *value = IsSystemType() ? FALSE : static_cast<BOOLEAN>(m_pUserTypeData->IsCollection);

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::get_IsConstructible(
        _Out_ BOOLEAN* value)
    {
        IFCPTRRC_RETURN(value, E_INVALIDARG);

        *value = IsSystemType() ? FALSE : static_cast<BOOLEAN>(m_pUserTypeData->isConstructible);

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::get_IsDictionary(
        _Out_ BOOLEAN* value)
    {
        IFCPTRRC_RETURN(value, E_INVALIDARG);

        *value = IsSystemType() ? FALSE : static_cast<BOOLEAN>(m_pUserTypeData->isDictionary);

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::get_IsMarkupExtension(
        _Out_ BOOLEAN* value)
    {
        IFCPTRRC_RETURN(value, E_INVALIDARG);

        *value = IsSystemType() ? FALSE : static_cast<BOOLEAN>(m_pUserTypeData->isMarkupExtension);

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::get_IsSystemType(
        _Out_ BOOLEAN* value)
    {
        IFCPTRRC_RETURN(value, E_INVALIDARG);

        *value = IsSystemType() ? TRUE : FALSE;

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::get_IsBindable(
        _Out_ BOOLEAN* value)
    {
        IFCPTRRC_RETURN(value, E_INVALIDARG);

        *value = FALSE;

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::get_ItemType(
        _COM_Outptr_result_maybenull_ xaml_markup::IXamlType** value)
    {
        IFCPTRRC_RETURN(value, E_INVALIDARG);

        *value = nullptr;

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::get_KeyType(
        _COM_Outptr_result_maybenull_ xaml_markup::IXamlType** value)
    {
        IFCPTRRC_RETURN(value, E_INVALIDARG);

        *value = nullptr;

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::get_BoxedType(
        _COM_Outptr_result_maybenull_ xaml_markup::IXamlType** value)
    {
        IFCPTRRC_RETURN(value, E_INVALIDARG);

        *value = nullptr;

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::get_Name(
        _Outptr_ HSTRING* value)
    {
        HRESULT hr = S_OK;

        IFCPTRRC(value, E_INVALIDARG);
        IFCEXPECT(m_pUserTypeData != nullptr);

        IFC(m_pTypeInfoProvider->GetHString(m_pUserTypeData->idsName, value))

    Cleanup:
        return hr;
    }

    IFACEMETHODIMP
    XamlType::get_UnderlyingType(
        _Out_ wxaml_interop::TypeName* value)
    {
        IFCPTRRC_RETURN(value, E_INVALIDARG);

        IFC_RETURN(get_FullName(
            &value->Name));

        value->Kind = wxaml_interop::TypeKind::TypeKind_Metadata;

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::ActivateInstance(
        _Outptr_ IInspectable** instance)
    {
        IFCPTRRC_RETURN(instance, E_INVALIDARG);
        IFCEXPECT_RETURN(m_pUserTypeData != nullptr);

        IFC_RETURN(m_pTypeInfoProvider->ActivateInstance(m_pUserTypeData->activatorId, instance));

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::RunInitializer()
    {
        if (IsSystemType() || !IsValueType())
        {
            // Force the Regitration of Custom DP's before loading the Type.
            IFC_RETURN(m_pTypeInfoProvider->EnsureDependencyProperties(m_pUserTypeData->activatorId));
        }
        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::GetMember(
        _In_ HSTRING memberName,
        _COM_Outptr_result_maybenull_ xaml_markup::IXamlMember** value)
    {
        wrl::ComPtr<xaml_markup::IXamlMember> xamlMember;
        PCWSTR pszName = WindowsGetStringRawBuffer(memberName, nullptr);
        INT32* piMemIdxs = const_cast<INT32*>(m_piMemberIndices);

        IFCPTRRC_RETURN(value, E_INVALIDARG);

        if (nullptr != piMemIdxs)
        {
            INT32 iMember = *piMemIdxs;

            while (-1 != iMember)
            {
                const Private::UserMemberInfo *memberinfo =
                    m_pTypeInfoProvider->GetUserMemberInfo(iMember);
                PCWSTR memberNameFromProvider = nullptr;
                IFC_RETURN(m_pTypeInfoProvider->GetString(memberinfo->idsName, &memberNameFromProvider));

                if (0 == wcscmp(memberNameFromProvider, pszName))
                {
                    IFC_RETURN(m_pTypeInfoProvider->FindXamlMember_FromInt32(
                        iMember,
                        &xamlMember));

                    break;
                }

                iMember = *(++piMemIdxs);
            }
        }

        *value = xamlMember.Detach();

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::CreateFromString(
        _In_ HSTRING hValue,
        _Outptr_ IInspectable** ppInstance)
    {
        // Support (multiple) named constants as well as numeric values.

        IFCEXPECT_RETURN(ppInstance);
        *ppInstance = nullptr;

        if(IsEnum())
        {
#if XCP_MONITOR
            // std::wregex allocates a global locale object that lives for the lifetime of the application. Not much
            // we can do other than ignore it
            auto stopIgnoringLeaks = XcpDebugStartIgnoringLeaks();
#endif
            // Regex used by C++/CX, it will basically split the string using the ',' delimiter and trim spaces.
            const std::wregex regex(L"^\\s+|\\s*,\\s*|\\s+$");

#if XCP_MONITOR
            stopIgnoringLeaks.reset();
#endif

            UINT32 enumValue = 0;
            UINT32 inputLength = 0;
            PCWSTR inputBuffer = WindowsGetStringRawBuffer(hValue, &inputLength);

            IFCEXPECT_RETURN(inputBuffer);
            IFCEXPECT_RETURN(inputLength > 0);

            for (std::wcregex_token_iterator it(inputBuffer, inputBuffer + inputLength, regex, -1), end; it != end; ++it)
            {
                BOOLEAN found = FALSE;
                const std::wcsub_match& subMatch = *it;

                if (subMatch.length() == 0 )
                {
                    continue;
                }

                const Private::UserEnumInfo& enumInfo = *m_pTypeInfoProvider->GetUserEnumInfo(m_pUserTypeData->iEnumIndex);
                const Private::UserEnumValueInfo* pValues = nullptr;
                IFC_RETURN(m_pTypeInfoProvider->GetUserEnumValues(enumInfo, &pValues));

                for(UINT32 i = 0; i < enumInfo.cValues && !found; ++i)
                {
                    const Private::UserEnumValueInfo enumValueInfo = pValues[i];
                    size_t cText = 0;
                    PCWSTR pText = nullptr;
                    IFC_RETURN(m_pTypeInfoProvider->GetString(enumValueInfo.idsName, &pText, &cText));

                    if(cText == static_cast<UINT32>(subMatch.length()) &&
                        !_wcsnicmp(pText, subMatch.first, cText))
                    {
                        enumValue |= enumValueInfo.iValue;
                        found = TRUE;
                    }
                }

                if(!found)
                {
                    // std::stoi stops at the first non-numeric character when converting. So if there are no integers, it will
                    // throw an exception that we are unable to catch, resulting in a failfast. This is bad for the designer because
                    // users can enter the wrong value by accident, and we really shouldn't crash. It's also possible someone could
                    // pass in a number greater 2^32 which will also throw, but realistically that won't happen.
                    // This check should be enough for real world scenarios.
                    if (iswdigit(subMatch.str()[0]))
                    {
                        enumValue |= std::stoi(subMatch);
                    }
                    else
                    {
                        IFC_RETURN(E_INVALIDARG);
                    }
                }
            }

            IFC_RETURN(m_pTypeInfoProvider->BoxEnum(m_pUserTypeData->enumBoxerId, enumValue, ppInstance));
        }

        return S_OK;
    }

    IFACEMETHODIMP
    XamlType::AddToVector(
        _In_ IInspectable* instance,
        _In_ IInspectable* value)
    {
        return m_pTypeInfoProvider->AddToVector(m_pUserTypeData->addToVectorId, instance, value);
    }

    IFACEMETHODIMP
    XamlType::AddToMap(
        _In_ IInspectable* instance,
        _In_ IInspectable* key,
        _In_ IInspectable* value)
    {
        return m_pTypeInfoProvider->AddToMap(m_pUserTypeData->addToMapId, instance, key, value);
    }
}

