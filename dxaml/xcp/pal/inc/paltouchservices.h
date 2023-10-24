// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef __PAL__TOUCH__INTERACTION_SERVICES__
#define __PAL__TOUCH__INTERACTION_SERVICES__

#include <memory>

struct IXcpInputPaneHandler;
class DirectManipulationServiceSharedState;

struct IPALTouchInteractionServices
{
    virtual _Check_return_ HRESULT IsDirectManipulationSupported(_Out_ bool &isDirectManipulationSupported) = 0;
    
    // Returns an IPALDirectManipulationService implementation for interacting with the Windows8 DirectManipulation APIs.
    virtual _Check_return_ HRESULT GetDirectManipulationService(std::shared_ptr<DirectManipulationServiceSharedState> sharedState, _Outptr_ IPALDirectManipulationService** pDirectManipulationService) = 0;

    virtual _Check_return_ HRESULT GetInputPaneInteraction(
        _In_ IXcpInputPaneHandler *pInputPaneHandler, 
        _Outptr_ IPALInputPaneInteraction** ppInputPaneInteraction) = 0;

};

#endif //__PAL__TOUCH__INTERACTION_SERVICES__
