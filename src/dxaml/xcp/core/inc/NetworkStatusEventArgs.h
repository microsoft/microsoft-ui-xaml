// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Event args for CNetworkStatusCallback to use to create an async callback

class CNetworkStatusEventArgs : public CEventArgs
{
    CNetworkStatusEventArgs()
    {
    }

    // Destructor
    ~CNetworkStatusEventArgs() override
    {
    }
};