// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct XamlPropertyToken;

class CTemplateBindingExtension
    final : public CMarkupExtensionBase
{
private:
    CTemplateBindingExtension(_In_ CCoreServices *pCore)
        : CMarkupExtensionBase(pCore)
        , m_pSourceProperty(NULL)
        , m_fTemplateBindingComplete(FALSE)
    {
    }

public:
    // Creation method
    DECLARE_CREATE(CTemplateBindingExtension);

    ~CTemplateBindingExtension() override
    {
        Reset();
    }

    _Check_return_ HRESULT SetTemplateBinding(
                _In_ CDependencyObject* pDependencyObject,
                _In_ XamlPropertyToken tokTargetProperty);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTemplateBindingExtension>::Index;
    }

    _Check_return_ HRESULT ProvideValue(
                _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
                _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue) override;

    void Reset() override;

    void SetTemplateBindingComplete() { m_fTemplateBindingComplete = TRUE; }

public:
    CDependencyPropertyProxy *m_pSourceProperty;
    bool m_fTemplateBindingComplete;

private:
};

