// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "FlyweightCoreAdapter.h"
#include "FlyweightCoreState.h"
#include <corep.h>

namespace Flyweight
{
    template <>
    typename Factory<CDOSharedState>::State* GetFactoryState<CDOSharedState>(
        _In_opt_ CCoreServices* core)
    {
        return (core) ? &core->GetFlyweightState().m_CDOSharedState : nullptr;
    }

    template <>
    typename Factory<DurationVO>::State* GetFactoryState<DurationVO>(
        _In_opt_ CCoreServices* core)
    {
        return (core) ? &core->GetFlyweightState().m_durationVO : nullptr;
    }

    template <>
    typename Factory<RepeatBehaviorVO>::State* GetFactoryState<RepeatBehaviorVO>(
        _In_opt_ CCoreServices* core)
    {
        return (core) ? &core->GetFlyweightState().m_repeatBehaviorVO : nullptr;
    }

    template <>
    typename Factory<KeyTimeVO>::State* GetFactoryState<KeyTimeVO>(
        _In_opt_ CCoreServices* core)
    {
        return (core) ? &core->GetFlyweightState().m_keyTimeVO : nullptr;
    }
}