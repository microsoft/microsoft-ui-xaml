// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

CLayoutStorage::~CLayoutStorage()
{
    ResetLayoutInformation();
}

void CLayoutStorage::ResetLayoutInformation()
{
    m_previousAvailableSize.width = m_previousAvailableSize.height = 0;
    m_desiredSize.width = m_desiredSize.height = 0;
    m_finalRect.X = m_finalRect.Y = m_finalRect.Width = m_finalRect.Height = 0;
    m_offset.x = m_offset.y = 0;
    m_unclippedDesiredSize.width = m_unclippedDesiredSize.height = 0;
    m_size.width = m_size.height = 0;

    ReleaseInterface(m_pLayoutClipGeometry);
}