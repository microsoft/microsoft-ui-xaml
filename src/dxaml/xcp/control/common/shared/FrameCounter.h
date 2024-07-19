// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CFrameCounter
{
public:
    CFrameCounter();

    void
        Reset();

    void
        Inc()
    {
        m_uFrames++;
    }

    XINT32
        Update();

    XINT64_LARGE_INTEGER
        GetFreq()
    {
        return m_lFreq;
    }

    inline float
        GetFPS()
    {
        return m_fps;
    }
private:

// Variables for reporting frames per second

    XINT64_LARGE_INTEGER    m_lTimeStart; 
    XINT64_LARGE_INTEGER    m_lTimeEnd;
    XINT64_LARGE_INTEGER    m_lFreq;

    XUINT32         m_uFrames;
    XFLOAT          m_fps;
};

