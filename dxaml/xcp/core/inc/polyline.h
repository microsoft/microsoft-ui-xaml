// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CPolyline final : public CShape
{
protected:
    CPolyline(_In_ CCoreServices *pCore)
        : CShape(pCore)
    {
        m_nFillRule = XcpFillModeAlternate;
        m_pPoints   = NULL;
    }

   ~CPolyline() override;

public:
    DECLARE_CREATE(CPolyline);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPolyline>::Index;
    }

    // CShape overrides
    _Check_return_ HRESULT UpdateRenderGeometry() override;

private:
    _Check_return_ HRESULT EnsureGeometry();

public:
    // CPolyline fields
    XcpFillMode m_nFillRule;
    CPointCollection *m_pPoints;

};
