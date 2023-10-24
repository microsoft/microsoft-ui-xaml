// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// These need to be moved into PAL functions, but for now, we
//  hack them up here. They are implemented elsewhere.

//-------------------------------------------------------------------------
//
//  Method:   CFrameCounter::CFrameCounter
//
//  Synopsis:   
//     ctor
// 
//-------------------------------------------------------------------------
CFrameCounter::CFrameCounter()
{
    m_lTimeStart.QuadPart = 0LL;
    m_lTimeEnd.QuadPart = 0LL;
    m_lFreq.QuadPart = 0LL;
    
     // start timer.
    gps->PerformanceCounter(&m_lTimeStart);
    gps->PerformanceFrequency(&m_lFreq);

    Reset();
}

//-------------------------------------------------------------------------
//
//  Method:   CFrameCounter::Reset
//
//  Synopsis:   
//     Reset frame counter
// 
//-------------------------------------------------------------------------
void 
CFrameCounter::Reset()
{
    m_uFrames = 0;
    m_fps = 0;
}

//-------------------------------------------------------------------------
//
//  Method:   CFrameCounter::Update
//
//  Synopsis:   
//     Reset frame counter
// 
//-------------------------------------------------------------------------
XINT32 
CFrameCounter::Update()
{
    gps->PerformanceCounter(&m_lTimeEnd);
    XDOUBLE fctime = XDOUBLE(m_lTimeEnd.QuadPart-m_lTimeStart.QuadPart)/m_lFreq.QuadPart;

    if (fctime > 2.0)
    {
        m_fps = static_cast<XFLOAT>(m_uFrames / fctime);
        m_lTimeStart = m_lTimeEnd;
        m_uFrames = 0;
        
        return true;
    }

    return false;
}
