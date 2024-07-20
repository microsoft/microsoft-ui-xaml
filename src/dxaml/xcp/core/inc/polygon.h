// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CPolygon final : public CShape
{
protected:
    CPolygon(_In_ CCoreServices *pCore)
        : CShape(pCore)
    {
        m_nFillRule = XcpFillModeAlternate;
        m_pPoints   = NULL;
    }

   ~CPolygon() override;

public:
    DECLARE_CREATE(CPolygon);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPolygon>::Index;
    }

    // CShape overrides
    _Check_return_ HRESULT UpdateRenderGeometry() override;

public:
    // CPolygon fields
    XcpFillMode m_nFillRule;
    CPointCollection *m_pPoints;


private:
    _Check_return_ HRESULT EnsureGeometry();
};
