// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Binding markup extension

#pragma once

#include "MarkupExtension.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

class CBindingBase: public CMarkupExtensionBase
{
protected:
    CBindingBase(_In_ CCoreServices* pCore)
        : CMarkupExtensionBase(pCore)
    {
    }

public:
    // Creation method
    DECLARE_CREATE(CBindingBase);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBindingBase>::Index;
    }

    _Check_return_ HRESULT ProvideValue(
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue) override
    {
        RRETURN(E_NOTIMPL);
    }

};

class CBinding final : public CBindingBase
{
private:
    CBinding(_In_ CCoreServices* pCore)
        : CBindingBase(pCore)
        , m_strPath()
    {
    }

public:
    // Creation method
    DECLARE_CREATE(CBinding);

    _Check_return_ HRESULT SetBinding(
        _In_ CDependencyObject* pDependencyObject,
        _In_ KnownPropertyIndex propIndex);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBinding>::Index;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    xstring_ptr m_strPath;
};

class CRelativeSource final : public CMarkupExtensionBase
{
protected:
    CRelativeSource(_In_ CCoreServices *pCore)
        : CMarkupExtensionBase(pCore)
        , m_eMode(DirectUI::RelativeSourceMode::None)
    {
    }

    ~CRelativeSource() override
    {
    }

public:
    DECLARE_CREATE(CRelativeSource);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::RelativeSource;
    }

    _Check_return_ HRESULT ProvideValue(
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue) override;

    DirectUI::RelativeSourceMode m_eMode;
};

