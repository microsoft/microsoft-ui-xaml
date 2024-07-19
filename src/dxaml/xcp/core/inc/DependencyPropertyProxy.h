// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDependencyPropertyProxy final : public CNoParentShareableDependencyObject
{
public:
    CDependencyPropertyProxy(CCoreServices *pCore)
        : CNoParentShareableDependencyObject(pCore)
    {}

    ~CDependencyPropertyProxy() override = default;

    DECLARE_CREATE_WITH_TYPECONVERTER(CDependencyPropertyProxy);

    _Check_return_ HRESULT FromString(CREATEPARAMETERS *pCreate);

    _Check_return_ HRESULT FromXamlProperty(_In_ const std::shared_ptr<XamlProperty>& spXamlProperty);

    // Public accessors for this object
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDependencyPropertyProxy>::Index;
    }

    const CDependencyProperty* GetDP();
    void SetDP(_In_ const CDependencyProperty* pDependencyProperty);

    bool IsEqual(CDependencyPropertyProxy *pDependencyPropertyProxy);

    KnownPropertyIndex GetPropertyIndex() const
    {
        return static_cast<KnownPropertyIndex>(m_nPropertyIndex);
    }

    void SetPropertyIndex(KnownPropertyIndex index)
    {
        m_nPropertyIndex = static_cast<XINT32>(index);
    }

    // Public fields
    XINT32 m_nPropertyIndex = static_cast<XINT32>(KnownPropertyIndex::UnknownType_UnknownProperty);
};