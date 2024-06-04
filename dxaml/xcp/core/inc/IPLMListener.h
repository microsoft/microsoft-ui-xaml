// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Implement this on any class that wants to get PLM (ProcessLifeManagement) events.
//      The objects should be registered using CCoreServices::RegisterPLMListener/UnregisterPLMListener.
//
//      Flag isTriggeredByResourceTimer identifies whether this is triggered
//      by a true suspend or the firing of the Xaml background resource timer.
//
//------------------------------------------------------------------------------
struct IPLMListener
{
    virtual _Check_return_ HRESULT OnSuspend(_In_ bool isTriggeredByResourceTimer) = 0;
    virtual _Check_return_ HRESULT OnResume() = 0;
    virtual void OnLowMemory() = 0;
};
