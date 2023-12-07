// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once


//------------------------------------------------------------------------
//
//  ISwapChainPanelOwner:  A simple CSwapChainPanel accessor interface
//  This interface serves two purposes:
//  1) There are two DXaml objects that own a CSwapChainPanel core object
//     (SwapChainPanel and SwapChainBackgroundPanel).  Both of these need
//     to be able to provide its CSwapChainPanel from core code.  This interface
//     abstracts away this detail so core code can simply ask the SwapChainPanelOwner
//     for its CSwapChainPanel when required rather than having to know details about the
//     DXaml objects. 
//  2) When a caller creates a CoreInput object, it does so from a background thread.
//     We post a dispatcher message to the UI thread to do this hook-up, which opens
//     the possibility of a race - the SCP/SCBP could be deleted between the time
//     the message is enqueued and delivered.  This interface allows us to AddRef
//     the DXaml object during this period of time and keep it alive.
//
//------------------------------------------------------------------------
class __declspec(uuid("79cc68c4-0076-44ad-bd35-28e5e9914195")) ISwapChainPanelOwner : public IUnknown
{
public:
    virtual CSwapChainPanel* GetSwapChainPanel() = 0;
};
