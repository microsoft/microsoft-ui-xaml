// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CNullExtension
    final : public CMarkupExtensionBase
{
private:
    CNullExtension(_In_ CCoreServices *pCore)
        : CMarkupExtensionBase(pCore)
    {
    }

public:
    // Creation method
    DECLARE_CREATE(CNullExtension);

    ~CNullExtension() override
    {
    }

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CNullExtension>::Index;
    }

    _Check_return_ HRESULT ProvideValue(
                _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
                _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue) override;
};

