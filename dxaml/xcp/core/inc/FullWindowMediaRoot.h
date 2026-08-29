// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <DisplayListener.h>

class CFullWindowMediaRoot final : public CPanel
{
public:
    DECLARE_CREATE(CFullWindowMediaRoot);

    _Check_return_ HRESULT InitInstance() override;

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CFullWindowMediaRoot>::Index;
    }

    void GetChildrenInRenderOrder(
        _Outptr_opt_result_buffer_(*puiChildCount) CUIElement ***pppUIElements,
        _Out_ XUINT32 *puiChildCount
        ) override;

protected:
   ~CFullWindowMediaRoot() override;

   _Check_return_ HRESULT MeasureOverride(_In_ XSIZEF availableSize, _Out_ XSIZEF& desiredSize) override;
   _Check_return_ HRESULT ArrangeOverride(_In_ XSIZEF finalSize, _Out_ XSIZEF& newFinalSize) override;

private:
    CFullWindowMediaRoot(
        _In_ CCoreServices *pCore)
        : CPanel(pCore),
        m_ppRenderChildren(NULL) {};

private:
    CUIElement** m_ppRenderChildren;
};
