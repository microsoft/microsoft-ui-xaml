// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Event args for CAutomationPeer to use to create an async callback.

#pragma once

class CAutomationPeerEventArgs final : public CEventArgs
{
public:
    // Destructor
    ~CAutomationPeerEventArgs() override
    {
    }

    CAutomationPeerEventArgs()
    {
        m_pAP = NULL;
    }

    CAutomationPeerEventArgs* AsAutomationPeerEventArgs() override
    {
        return this;
    }

    CAutomationPeer *m_pAP;
};
