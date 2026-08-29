// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Synopsis:
//      A Popup that holds a more solid reference to its managed peer.

class CGripperPopup : public CPopup
{    
    CGripperPopup(_In_ CCoreServices *pCore)
        : CPopup(pCore)
    {
    }

public:

    DECLARE_CREATE(CGripperPopup);
    
    //override
    bool ControlsManagedPeerLifetime() override
    {
        // When there's a native ref on this object, strengthen the reference on the 
        // managed peer so that it's not collected.
        return true;
    }

};
