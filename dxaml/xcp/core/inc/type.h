// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NoParentShareableDependencyObject.h>

class CType final : public CNoParentShareableDependencyObject
{
public:
    DECLARE_CREATE_WITH_TYPECONVERTER(CType);

    _Check_return_ HRESULT FromString(_In_ CREATEPARAMETERS* pCreate);

    _Check_return_ HRESULT FromXamlType(_In_ const std::shared_ptr<XamlType>& spXamlType);

    CType(_In_opt_ CCoreServices* core = nullptr);

public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CType>::Index;
    }

    // Public accessors for this object
    bool IsCoreType() const;

    KnownTypeIndex GetReferencedTypeIndex()
    {
        return m_nTypeIndex;
    }

    _Check_return_ HRESULT GetClassName(_Out_ xstring_ptr* fullName) const;

    KnownTypeIndex m_nTypeIndex;
};
