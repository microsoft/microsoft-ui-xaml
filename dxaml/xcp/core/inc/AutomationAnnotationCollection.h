// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      A trivial AutomationAnnotation Collection to facilitate DXAML marshalling.

class CAutomationAnnotationCollection final : public CDependencyObjectCollection
{
public:
    // Creation method
    DECLARE_CREATE(CAutomationAnnotationCollection);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CAutomationAnnotationCollection>::Index;
    }

private:
    CAutomationAnnotationCollection(_In_ CCoreServices *pCore)
        : CDependencyObjectCollection(pCore)
    {
    }
};

