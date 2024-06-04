// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "TypeTableStructs.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace DependencyObject {
    class PropertySystemUnitTests;
} } } } } }

class CCustomDependencyProperty : public CDependencyProperty
{
    friend class CPropertyBase;

public:
#if defined(__XAML_UNITTESTS__)
    CCustomDependencyProperty() = default;
#endif

// Used to create custom DPs at runtime.
    static _Check_return_ HRESULT Create(
        _In_ KnownPropertyIndex index,
        _In_ MetaDataPropertyInfoFlags flags,
        _In_ const xstring_ptr_view& strName,
        _Outptr_ CCustomDependencyProperty** ppDP);

    _Check_return_ HRESULT EnsureTypesResolved();

    xaml::IPropertyChangedCallback* GetPropertyChangedCallbackNoRef() const
    {
        return m_spPropertyChangedCallback.Get();
    }

    _Check_return_ HRESULT SetDefaultMetadata(_In_ xaml::IPropertyMetadata* pDefaultMetadata);

    _Check_return_ HRESULT GetDefaultValue(_Outptr_ IInspectable** ppDefaultValue) const;

    void Invalidate()
    {
        m_spPropertyChangedCallback = nullptr;
        m_flags |= MetaDataPropertyInfoFlags::IsInvalid;
    }

    void UpdateTypeInfo(
        KnownTypeIndex propertyTypeIndex,
        KnownTypeIndex declaringTypeIndex,
        KnownTypeIndex targetTypeIndex)
    {
        m_nPropertyTypeIndex = propertyTypeIndex;
        m_nDeclaringTypeIndex = declaringTypeIndex;
        m_nTargetTypeIndex = targetTypeIndex;
    }

private:
    CCustomDependencyProperty(
        KnownPropertyIndex index,
        MetaDataPropertyInfoFlags flags,
        xstring_ptr&& name)
        : CDependencyProperty(
            index,
            KnownTypeIndex::UnknownType,
            KnownTypeIndex::UnknownType,
            KnownTypeIndex::UnknownType,
            flags | MetaDataPropertyInfoFlags::IsCustomDependencyProperty)
        , m_strName(name)
    {}

    xstring_ptr                                                 m_strName;
    ctl::ComPtr<IInspectable>                                   m_spDefaultValue;
    ctl::ComPtr<xaml::IPropertyChangedCallback>    m_spPropertyChangedCallback;
    ctl::ComPtr<xaml::ICreateDefaultValueCallback> m_spCreateDefaultValueCallback;
};

// Naming is somewhat odd here... Consider renaming CDependencyProperty to CProperty?
class CCustomProperty : public CDependencyProperty
{
    friend class CPropertyBase;

    public:
    // Used to create custom properties at runtime.
    static _Check_return_ HRESULT Create(
        _In_ KnownPropertyIndex index,
        _In_ KnownTypeIndex propertyTypeIndex,
        _In_ KnownTypeIndex declaringTypeIndex,
        _In_ MetaDataPropertyInfoFlags flags,
        _In_ xaml_markup::IXamlMember* xamlProperty,
        _In_ const xstring_ptr_view& strName,
        _Outptr_ CCustomProperty** ppProperty);

    xaml_markup::IXamlMember* GetXamlPropertyNoRef() const
    {
        return m_spXamlProperty.Get();
    }

    _Check_return_ HRESULT TryGetUnderlyingDP(_Outptr_ const CDependencyProperty** ppUnderlyingDP);

    void UpdateUnderlyingDP(_In_ const CDependencyProperty* pUnderlyingDP)
    {
        m_pUnderlyingDP = pUnderlyingDP;
    }

    void Invalidate()
    {
        m_flags |= MetaDataPropertyInfoFlags::IsInvalid;
    }

private:
    CCustomProperty(
        _In_ KnownPropertyIndex index,
        _In_ KnownTypeIndex propertyTypeIndex,
        _In_ KnownTypeIndex declaringTypeIndex,
        _In_ MetaDataPropertyInfoFlags flags,
        _In_ xaml_markup::IXamlMember* xamlProperty,
        _Inout_ xstring_ptr&& strName)
        : CDependencyProperty(
            index,
            propertyTypeIndex,
            declaringTypeIndex,
            KnownTypeIndex::UnknownType,
            flags | MetaDataPropertyInfoFlags::IsCustomProperty)
        , m_strName(strName)
        , m_spXamlProperty(xamlProperty)
    {}

    xstring_ptr                                         m_strName;
    ctl::ComPtr<xaml_markup::IXamlMember> m_spXamlProperty;
    const CDependencyProperty*                          m_pUnderlyingDP = nullptr;
};
