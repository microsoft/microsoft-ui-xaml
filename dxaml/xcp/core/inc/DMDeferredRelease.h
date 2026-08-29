// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Microsoft.DirectManipulation.h>

// DMDeferredRelease: A helper class to help carry out releasing DManip content
// and shared transform until after they are no longer "live".
// See more details on deferred DM Release in CInputServices::PrepareSecondaryContentRelationshipForCurveUpdate().
class DMDeferredRelease
{
public:
    DMDeferredRelease();

    void SetDMViewport(_In_ IDirectManipulationViewport* pDMViewport);
    IDirectManipulationViewport* GetDMViewport();
    void SetDMContent(_In_ IDirectManipulationContent* pDMContent);
    void SetDMCompositor(_In_ IDirectManipulationCompositor* pDMCompositor);
    void SetBatchId(ULONG batchId);
    ULONG GetBatchId() const;

    HRESULT DoDeferredRelease();

private:
    xref_ptr<IDirectManipulationViewport> m_spDMViewport;       // The viewport we're removing content from
    xref_ptr<IDirectManipulationContent> m_spDMContent;         // The DM content we're removing
    xref_ptr<IDirectManipulationCompositor> m_spDMCompositor;  // The compositor we're removing shared transform from
    ULONG m_batchId;                                            // The DComp batch ID, that when confirmed, signals it's safe to release objects
};
