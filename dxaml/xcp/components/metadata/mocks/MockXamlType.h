// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TypeNamePtr.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Metadata {

        class MockXamlType : public Microsoft::WRL::RuntimeClass<xaml_markup::IXamlType>
        {
        public:
            static Microsoft::WRL::ComPtr<MockXamlType> Create()
            {
                return Microsoft::WRL::Make<MockXamlType>();
            }

            static Microsoft::WRL::ComPtr<MockXamlType> CreateMetadata(HSTRING fullName)
            {
                wrl_wrappers::HString strFullName;
                strFullName.Set(fullName);

                TypeNamePtr underlyingType(strFullName.Get(), wxaml_interop::TypeKind_Metadata);

                return Microsoft::WRL::Make<MockXamlType>()
                    ->WithFullName(std::move(strFullName))
                    ->WithUnderlyingType(underlyingType);
            }

            Microsoft::WRL::ComPtr<MockXamlType> WithIsBindable(bool value)
            {
                GetIsBindableCallback = [value](BOOLEAN* pValue) -> HRESULT
                {
                    *pValue = value;
                    return S_OK;
                };
                return this;
            }

            Microsoft::WRL::ComPtr<MockXamlType> WithBaseType(KnownTypeIndex typeIndex)
            {
                GetBaseTypeCallback = [typeIndex](xaml_markup::IXamlType** ppXamlType) -> HRESULT
                {
                    auto baseTypeClassInfo = DirectUI::MetadataAPI::GetClassInfoByIndex(typeIndex);
                    TypeNamePtr baseTypeName;
                    IFC_RETURN(DirectUI::MetadataAPI::GetTypeNameByClassInfo(baseTypeClassInfo, baseTypeName.ReleaseAndGetAddressOf()));
                    *ppXamlType = MockXamlType::Create()->WithUnderlyingType(baseTypeName).Detach();
                    return S_OK;
                };
                return this;
            }

            Microsoft::WRL::ComPtr<MockXamlType> WithFullName(wrl_wrappers::HString fullName)
            {
                GetFullNameCallback = [&fullName](HSTRING* value) -> HRESULT
                {
                    return fullName.CopyTo(value);
                };
                return this;
            }

            Microsoft::WRL::ComPtr<MockXamlType> WithUnderlyingType(TypeNamePtr& typeName)
            {
                GetUnderlyingTypeCallback = [typeName](wxaml_interop::TypeName* value) -> HRESULT
                {
                    return typeName.CopyTo(value);
                };
                return this;
            }

            IFACEMETHODIMP get_BaseType(_Outptr_ xaml_markup::IXamlType** value)
            {
                if (GetBaseTypeCallback != nullptr)
                {
                    return GetBaseTypeCallback(value);
                }
                return E_NOTIMPL;
            }

            IFACEMETHODIMP get_ContentProperty(_Outptr_ xaml_markup::IXamlMember** value)
            {
                if (GetContentPropertyCallback != nullptr)
                {
                    return GetContentPropertyCallback(value);
                }

                *value = nullptr;
                return S_OK;
            }

            IFACEMETHODIMP get_FullName(_Outptr_ HSTRING* value)
            {
                if (GetFullNameCallback != nullptr)
                {
                    return GetFullNameCallback(value);
                }
                return E_NOTIMPL;
            }

            IFACEMETHODIMP get_IsArray(_Out_ BOOLEAN* value)
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP get_IsBindable(_Out_ BOOLEAN* value)
            {
                if (GetIsBindableCallback != nullptr)
                {
                    return GetIsBindableCallback(value);
                }

                *value = false;
                return S_OK;
            }

            IFACEMETHODIMP get_IsCollection(_Out_ BOOLEAN* value)
            {
                if (GetIsCollectionCallback != nullptr)
                {
                    return GetIsCollectionCallback(value);
                }
                return E_NOTIMPL;
            }

            IFACEMETHODIMP get_IsConstructible(_Out_ BOOLEAN* value)
            {
                if (GetIsConstructibleCallback != nullptr)
                {
                    return GetIsConstructibleCallback(value);
                }
                return E_NOTIMPL;
            }

            IFACEMETHODIMP get_IsDictionary(_Out_ BOOLEAN* value)
            {
                if (GetIsDictionaryCallback != nullptr)
                {
                    return GetIsDictionaryCallback(value);
                }
                return E_NOTIMPL;
            }

            IFACEMETHODIMP get_IsMarkupExtension(_Out_ BOOLEAN* value)
            {
                if (GetIsMarkupExtensionCallback != nullptr)
                {
                    return GetIsMarkupExtensionCallback(value);
                }
                return E_NOTIMPL;
            }

            IFACEMETHODIMP get_ItemType(_Outptr_ xaml_markup::IXamlType** value)
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP get_KeyType(_Outptr_ xaml_markup::IXamlType** value)
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP get_BoxedType(_Outptr_ xaml_markup::IXamlType** value)
            {
                // Return null when the mock type does not represent a boxed type.
                *value = nullptr;
                return S_OK;
            }

            IFACEMETHODIMP get_Name(_Outptr_ HSTRING* value)
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP get_UnderlyingType(_Outptr_ wxaml_interop::TypeName* value)
            {
                if (GetUnderlyingTypeCallback != nullptr)
                {
                    return GetUnderlyingTypeCallback(value);
                }
                return E_NOTIMPL;
            }

            IFACEMETHODIMP ActivateInstance(_In_ IInspectable** instance)
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP RunInitializer()
            {
                if (RunInitializerCallback != nullptr)
                {
                    return RunInitializerCallback();
                }
                return E_NOTIMPL;
            }

            IFACEMETHODIMP CanAssignTo(_In_ xaml_markup::IXamlType* xamlType, _Out_ BOOLEAN* returnValue)
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP GetMember(_In_ HSTRING name, _Outptr_ xaml_markup::IXamlMember** xamlMember)
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP AddToVector(_In_ IInspectable *instance, _In_ IInspectable *value)
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP CreateFromString(_In_ HSTRING hValue, IInspectable **instance)
            {
                return E_NOTIMPL;
            }

            IFACEMETHODIMP AddToMap(_In_ IInspectable *instance, _In_ IInspectable *key, _In_ IInspectable *value)
            {
                return E_NOTIMPL;
            }

            std::function<HRESULT(xaml_markup::IXamlType**)> GetBaseTypeCallback;
            std::function<HRESULT(xaml_markup::IXamlMember**)> GetContentPropertyCallback;
            std::function<HRESULT(HSTRING*)> GetFullNameCallback;
            std::function<HRESULT(BOOLEAN*)> GetIsConstructibleCallback;
            std::function<HRESULT(BOOLEAN*)> GetIsCollectionCallback;
            std::function<HRESULT(BOOLEAN*)> GetIsDictionaryCallback;
            std::function<HRESULT(BOOLEAN*)> GetIsMarkupExtensionCallback;
            std::function<HRESULT(BOOLEAN*)> GetIsBindableCallback;
            std::function<HRESULT(wxaml_interop::TypeName*)> GetUnderlyingTypeCallback;
            std::function<HRESULT()> RunInitializerCallback;
        };

    }

} } } }
