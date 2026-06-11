// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Synopsis:
//      Used as a root for an element to be printed to allow for proper
//      Measure and Arrange

class CPrintRoot final : public CPanel
{
private:

    CPrintRoot(_In_ CCoreServices *pCore): CPanel(pCore)
    {
        m_sPageSize.width  = 0.0f;
        m_sPageSize.height = 0.0f;
    }

public:
    DECLARE_CREATE(CPrintRoot);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPrintRoot>::Index;
    }

    bool GetIsLayoutElement() const final { return true; }

    _Check_return_ HRESULT SetSize(_In_ XSIZEF pageSize);


protected:
    _Check_return_ HRESULT MeasureOverride(_In_ XSIZEF availableSize, _Out_ XSIZEF& desiredSize) override;

    _Check_return_ HRESULT ArrangeOverride(_In_ XSIZEF finalSize, _Out_ XSIZEF& newFinalSize) override;


private:
    XSIZEF m_sPageSize;
};
