// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CPath final : public CShape
{
protected:
    CPath(_In_ CCoreServices *pCore)
        : CShape(pCore)
    {
        m_pGeometryData = NULL;
    }

   ~CPath() override;

public:
    DECLARE_CREATE(CPath);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPath>::Index;
    }

    // CUIElement overrides
    bool GetIsLayoutElement() const final { return IsCustomType(); }

    // CShape overrides
    _Check_return_ HRESULT UpdateRenderGeometry() override;

public:
    // CPath fields
    CGeometry *m_pGeometryData;
};
