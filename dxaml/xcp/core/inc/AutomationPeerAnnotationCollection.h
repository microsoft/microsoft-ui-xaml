// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      A trivial AutomationPeerAnnotation Collection to facilitate DXAML marshalling.

class CAutomationPeerAnnotationCollection final : public CDOCollection
{
public:
    // Creation method
    DECLARE_CREATE(CAutomationPeerAnnotationCollection);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CAutomationPeerAnnotationCollection>::Index;
    }

private:
    CAutomationPeerAnnotationCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}
};