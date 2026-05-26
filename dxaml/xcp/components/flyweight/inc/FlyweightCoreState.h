// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FlyweightFactory.h"
#include "CDOSharedState.h"
#include "DurationVO.h"
#include "RepeatBehaviorVO.h"
#include "KeyTimeVO.h"

struct CoreState
{
    CoreState(
        _In_ const CDOSharedState& cdoSharedDefault)
        : m_CDOSharedState(cdoSharedDefault)
    {}

    void Reset()
    {
        m_durationVO.Reset();
        m_repeatBehaviorVO.Reset();
        m_keyTimeVO.Reset();
        m_CDOSharedState.Reset();
    }

    Flyweight::Factory<CDOSharedState>::State   m_CDOSharedState;
    Flyweight::Factory<DurationVO>::State       m_durationVO;
    Flyweight::Factory<RepeatBehaviorVO>::State m_repeatBehaviorVO;
    Flyweight::Factory<KeyTimeVO>::State        m_keyTimeVO;
};