// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Panel.h>

class VisualTree;

class CXamlIslandRootCollection final : public CPanel
{
private:
    CXamlIslandRootCollection(_In_ CCoreServices *pCore)
        : CPanel(pCore)
    {
    }

    ~CXamlIslandRootCollection() override;

public:
    DECLARE_CREATE(CXamlIslandRootCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CXamlIslandRootCollection>::Index;
    }

    protected:
       _Check_return_ HRESULT MeasureOverride(_In_ XSIZEF availableSize, _Out_ XSIZEF& desiredSize) final;
       _Check_return_ HRESULT ArrangeOverride(_In_ XSIZEF finalSize, _Out_ XSIZEF& newFinalSize) final;
};
