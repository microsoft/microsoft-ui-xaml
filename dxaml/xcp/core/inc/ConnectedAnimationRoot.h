// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "Framework.h"

class CConnectedAnimationRoot final : public CFrameworkElement
{
private:
    CConnectedAnimationRoot(_In_ CCoreServices *pCore);

public:
    DECLARE_CREATE(CConnectedAnimationRoot);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CConnectedAnimationRoot>::Index;
    }

    _Check_return_ HRESULT CreateConnectedAnimationVisual(_Outptr_ WUComp::IVisual** animationVisual);
    _Check_return_ HRESULT RemoveConnectedAnimationVisual(_In_ WUComp::IVisual* animationVisual);

    const std::vector<xref_ptr<CUIElement>>& GetUnloadingElements();

    void SetNeedsUnloadingHWWalk(bool needsUnloading) { m_needsUnloadingHWWalk = needsUnloading; }
    bool NeedsUnloadingHWWalk() { return m_needsUnloadingHWWalk; }

private:
    void SetDebugTag(_In_ WUComp::IVisual* visual, _In_ const wchar_t* debugTag);

    bool m_needsUnloadingHWWalk = false;

};
