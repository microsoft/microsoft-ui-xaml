// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

_Check_return_ HRESULT CompositorTreeHost::Create(_Outptr_ CompositorTreeHost **ppCompTreeHost)
{
    HRESULT hr = S_OK;

    CompositorTreeHost *pCompTreeHost = new CompositorTreeHost();

    *ppCompTreeHost = pCompTreeHost;
    RRETURN(hr);//RRETURN_REMOVAL
}

CompositorTreeHost::CompositorTreeHost()
{
}

CompositorTreeHost::~CompositorTreeHost()
{
    Cleanup();
}

void CompositorTreeHost::Cleanup()
{
    for (xvector<HWCompNode *>::iterator it = m_temporaryNodes.begin(); it != m_temporaryNodes.end(); it++)
    {
        ReleaseInterface(*it);
    }

    m_temporaryNodes.clear();
}

void CompositorTreeHost::TrackTemporaryNode(HWCompNode& temporaryNode)
{
    IFCFAILFAST(m_temporaryNodes.push_back(&temporaryNode));
    temporaryNode.AddRef();
}

_Check_return_ HRESULT CompositorTreeHost::RemoveTemporaryNodes()
{
    for (xvector<HWCompNode *>::iterator it = m_temporaryNodes.begin(); it != m_temporaryNodes.end(); it++)
    {
        HWCompNode* nodeToRemove = *it;
        IFCFAILFAST(nodeToRemove->Remove());
        ReleaseInterface(nodeToRemove);
    }

    m_temporaryNodes.clear();

    return S_OK;//RRETURN_REMOVAL
}
