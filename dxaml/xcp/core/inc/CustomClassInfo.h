// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "TypeTableStructs.h"
#include "TypeNamePtr.h"

class CCustomClassInfo : public CClassInfo
{
    friend class CClassInfo;

public:
    static _Check_return_ HRESULT Create(
        _In_ KnownNamespaceIndex eNamespaceIndex,
        _In_ const CClassInfo* pBaseType,
        _In_ KnownTypeIndex eTypeIndex,
        _In_ MetaDataTypeInfoFlags eTypeFlags,
        _In_ xaml_markup::IXamlType* pXamlType,
        _In_ const xstring_ptr_view& strName,
        _In_ const xstring_ptr_view& strFullName,
        _In_ bool isBindable,
        _In_ KnownTypeIndex boxedTypeIndex,
        _Outptr_ CCustomClassInfo** ppType);

    xaml_markup::IXamlType* GetXamlTypeNoRef() const
    {
        return m_spXamlType.Get();
    }

    bool IsBindable() const
    {
        return m_isBindable;
    }

    void UpdateContentPropertyIndex(KnownPropertyIndex index)
    {
        m_nContentPropertyIndex = index;
    }

    bool RepresentsBoxedType() const;

    const CClassInfo* GetBoxedType() const;

private:
    CCustomClassInfo() = default;

    ctl::ComPtr<xaml_markup::IXamlType>       m_spXamlType;
    xstring_ptr                                             m_strName;
    xstring_ptr                                             m_strFullName;
    KnownPropertyIndex                                      m_nContentPropertyIndex;
    KnownTypeIndex                                          m_boxedTypeIndex;
    KnownNamespaceIndex                                     m_nNamespaceIndex;
    bool                                                    m_isBindable;
};
