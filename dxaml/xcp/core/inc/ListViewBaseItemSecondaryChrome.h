// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CListViewBaseItemSecondaryChrome : public CFrameworkElement
{
public:
    // Earmark geometry data.
    // Chrome2 owns the earmark.
    CGeometry*                  m_pEarmarkGeometryData      = nullptr;

    // A pointer to the primary chrome.
    CListViewBaseItemChrome*    m_pPrimaryChromeNoRef       = nullptr;

    XRECTF_RB                   m_earmarkGeometryBounds     = {};

    // Earmark rendering fields.
    bool                        m_fFillBrushDirty           = false;

protected:
    CListViewBaseItemSecondaryChrome(
        _In_ CCoreServices *pCore)
        : CFrameworkElement(pCore)
    {}

    ~CListViewBaseItemSecondaryChrome() override
    {
        ReleaseInterface(m_pEarmarkGeometryData);
    }

public:
    DECLARE_CREATE(CListViewBaseItemSecondaryChrome);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CListViewBaseItemSecondaryChrome>::Index;
    }

    static void NWSetContentDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
    {
        CListViewBaseItemSecondaryChrome *pChrome = static_cast<CListViewBaseItemSecondaryChrome *>(pTarget);

        CFrameworkElement::NWSetContentDirty(pTarget, flags);

        if (!flags_enum::is_set(flags, DirtyFlags::Independent))
        {
            pChrome->m_fFillBrushDirty = true;
        }
    }

    void NWCleanDirtyFlags() override
    {
        m_fFillBrushDirty = false;
        CFrameworkElement::NWCleanDirtyFlags();
    }

    // Builds the earmark path.
    _Check_return_ HRESULT PrepareEarmarkPath();

    // Provides the geometry of the CheckMark, so the renderer knows how to size the brush.
    _Check_return_ HRESULT GetEarmarkBounds(_Out_ XRECTF_RB* pBounds);

private:
    // SegmentCollection helper.
    _Check_return_ HRESULT AddLineSegmentToSegmentCollection(_In_ CPathSegmentCollection *pSegments, _In_ XPOINTF point);
};
