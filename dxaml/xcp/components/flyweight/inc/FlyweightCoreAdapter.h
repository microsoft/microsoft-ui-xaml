// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FlyweightFactory.h"
#include "StaticAssertFalse.h"

#include "CDOSharedState.h"
#include "DurationVO.h"
#include "RepeatBehaviorVO.h"
#include "KeyTimeVO.h"
#include <xref_ptr.h>
#include <utility>

class CCoreServices;

namespace Flyweight
{
    template <typename T>
    typename Factory<T>::State* GetFactoryState(
        _In_opt_  CCoreServices* core)
    {
        static_assert_false("Specialize Flyweight::GetFactoryState() to retrieve state.");
    }

    template <>
    typename Factory<CDOSharedState>::State* GetFactoryState<CDOSharedState>(
        _In_opt_  CCoreServices* core);

    template <>
    typename Factory<DurationVO>::State* GetFactoryState<DurationVO>(
        _In_opt_  CCoreServices* core);

    template <>
    typename Factory<RepeatBehaviorVO>::State* GetFactoryState<RepeatBehaviorVO>(
        _In_opt_  CCoreServices* core);

    template <>
    typename Factory<KeyTimeVO>::State* GetFactoryState<KeyTimeVO>(
        _In_opt_  CCoreServices* core);

    template <typename T, typename ...Types>
    inline xref_ptr<typename Wrapper<T>::Type> Create(
        _In_opt_  CCoreServices* core,
        _In_ Types&&... args)
    {
        return Factory<T>::Create(
            GetFactoryState<T>(core),
            std::forward<Types>(args)...);
    }
}