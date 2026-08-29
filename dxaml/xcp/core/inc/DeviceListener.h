// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct DeviceListener
{
    virtual _Check_return_ HRESULT OnDeviceRemoved(bool cleanupDComp) = 0;
    virtual _Check_return_ HRESULT OnDeviceCreated() = 0;
};
