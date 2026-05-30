// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>
#include "Pal.h"

extern EncodedPtr<IPlatformServices> gps;

// IMockPlatformClock
struct IMockPlatformClock : public IPALClock
{
    virtual void SetAbsoluteTimeInSeconds(double time) = 0;
    virtual void AddSeconds(double seconds) = 0;
};

// IMockPlatformServices

struct IMockPlatformServices : public IPlatformServices
{
    virtual void GetMockClock(IMockPlatformClock** clock) = 0;
};

void CreateMockPlatformServices(IMockPlatformServices ** services);

//  PlatformServicesMocksHelper - Used to mock the Platform Services and associated objects

class PlatformServicesMocksHelper
{
public:

    PlatformServicesMocksHelper()
    {
        CreateMockPlatformServices(&m_mockServices);
        m_previousGps = gps.Get();
        gps.Set(m_mockServices);
    }
    ~PlatformServicesMocksHelper()
    {
        gps.Set(m_previousGps);
        delete m_mockServices;
    }

    __declspec(property (get = GetServices)) IMockPlatformServices* Services;
    IMockPlatformServices* GetServices()
    {
        return m_mockServices;
    }

private:
    IPlatformServices * m_previousGps;
    IMockPlatformServices*  m_mockServices;
};


