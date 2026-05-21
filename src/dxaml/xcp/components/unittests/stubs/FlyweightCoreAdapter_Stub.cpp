// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "FlyweightCoreAdapter.h"

namespace Flyweight
{
    template <>
    typename Factory<CDOSharedState>::State* GetFactoryState<CDOSharedState>(CCoreServices* core)
    {
        return nullptr;
    }

    template <>
    typename Factory<DurationVO>::State* GetFactoryState<DurationVO>(CCoreServices* core)
    {
        return nullptr;
    }

    template <>
    typename Factory<RepeatBehaviorVO>::State* GetFactoryState<RepeatBehaviorVO>(CCoreServices* core)
    {
        return nullptr;
    }

    template <>
    typename Factory<KeyTimeVO>::State* GetFactoryState<KeyTimeVO>(CCoreServices* core)
    {
        return nullptr;
    }
}