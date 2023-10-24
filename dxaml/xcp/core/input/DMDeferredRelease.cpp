// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DMDeferredRelease.h>

DMDeferredRelease::DMDeferredRelease()
: m_batchId(0)
{
}

void DMDeferredRelease::SetDMViewport(_In_ IDirectManipulationViewport* pDMViewport)
{
    m_spDMViewport = pDMViewport;
}

IDirectManipulationViewport* DMDeferredRelease::GetDMViewport()
{
    return m_spDMViewport;
}

void DMDeferredRelease::SetDMContent(_In_ IDirectManipulationContent* pDMContent)
{
    m_spDMContent = pDMContent;
}

void DMDeferredRelease::SetDMCompositor(_In_ IDirectManipulationCompositor* pDMCompositor)
{
    m_spDMCompositor = pDMCompositor;
}

void DMDeferredRelease::SetBatchId(ULONG batchId)
{
    // This is the batch ID of the current batch we're on.  It's expected that during
    // this batch we're also committing a change to create the new content/transform.
    // When we see this batch ID has been picked up and processed by the DWM,
    // it signals it's now safe to release the content and shared transform we're holding on to.
    m_batchId = batchId;
}

ULONG DMDeferredRelease::GetBatchId() const
{
    return m_batchId;
}

// Finally carry out the removing/releasing of DM content and shared transform
HRESULT DMDeferredRelease::DoDeferredRelease()
{
    ASSERT(m_spDMCompositor != nullptr);
    ASSERT(m_spDMContent != nullptr);
    ASSERT(m_spDMViewport != nullptr);
    IFC_RETURN(m_spDMCompositor->RemoveContent(m_spDMContent));
    IFC_RETURN(m_spDMViewport->RemoveContent(m_spDMContent));

    return S_OK;
}


