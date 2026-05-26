// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ScrollContentControl.h"
#include <ComPtr.h>

class CScrollViewer: public CScrollContentControl
{
protected:
    CScrollViewer(_In_ CCoreServices *pCore);
    ~CScrollViewer() override;

public:
    DECLARE_CREATE(CScrollViewer);

    _Check_return_ HRESULT EnsureManipulationTransformPropertySet(
        _Outptr_result_nullonfailure_ WUComp::ICompositionPropertySet **result);
    _Check_return_ HRESULT OnSharedContentTransformChanged(_In_ CUIElement *contentElement);
    _Check_return_ HRESULT OnContentOffsetChanged(_In_ CUIElement *contentElement);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::ScrollViewer;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    DOUBLE m_viewportWidth;
    DOUBLE m_scrollableWidth;
    DOUBLE m_extentWidth;
    DOUBLE m_viewportHeight;
    DOUBLE m_scrollableHeight;
    DOUBLE m_extentHeight;

private:
    _Check_return_ HRESULT UpdateManipulationTransformPropertySet(
        _In_ WUComp::ICompositionPropertySet *manipulationTransformPropertySet,
        _In_ CUIElement *contentElement);

    ctl::ComPtr<WUComp::ICompositionPropertySet> m_manipulationTransformPropertySet;
    ctl::ComPtr<WUComp::ICompositionObject> m_manipulationTransformCO;
    CUIElement* m_sharedPrimaryContentTransformOwner;
};
