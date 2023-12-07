// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <RPNode.h>
#include <RPGraph.h>

class CRelativePanel final : public CPanel
{
protected:
    CRelativePanel(_In_ CCoreServices *pCore);

    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) final;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) final;

public:
    ~CRelativePanel() override;

    // Creation method
    DECLARE_CREATE(CRelativePanel);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRelativePanel>::Index;
    }

    xref_ptr<CBrush> GetBorderBrush() const final;
    XTHICKNESS GetBorderThickness() const final;
    XTHICKNESS GetPadding() const final;
    XCORNERRADIUS GetCornerRadius() const final;
    DirectUI::BackgroundSizing GetBackgroundSizing() const final;

private:
    _Check_return_ HRESULT GenerateGraph();

    RPGraph m_graph;

}; // CRelativePanel
