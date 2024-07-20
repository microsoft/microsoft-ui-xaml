// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>

// Class used as the base class for native MarkupExtension.  This 
// is needed because otherwise there's no way to generically cast a 
// CDependencyObject pointer to a MarkupExtension.
class CMarkupExtensionBase 
    : public CDependencyObject
{
protected:
    CMarkupExtensionBase(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {
    }

public:
    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override;

    virtual _Check_return_ HRESULT ProvideValue(
                _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
                _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue) = 0;

    virtual void Reset()
    {
    }
};
