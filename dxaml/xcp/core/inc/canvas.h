// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCanvas : public CPanel
{
protected:
    CCanvas(_In_ CCoreServices *pCore) : CPanel( pCore )
    {
    }

   ~CCanvas() override;

public:
    DECLARE_CREATE(CCanvas);

    bool GetIsLayoutElement() const final { return false; }

// FrameworkElement overrides
protected:
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) override;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) final;
    _Check_return_ HRESULT UpdateLayoutClip(bool forceClipToRenderSize) final;

// CDependencyObject overrides
public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CCanvas>::Index;
    }
};
