// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "Vector.h"

template<int flags = MakeVectorParam<VectorFlag::Bindable, VectorFlag::Observable, VectorFlag::DependencyObjectBase>(), 
    typename Helper = VectorFlagHelper<flags>>
class BindableVector :
    public VectorBase<winrt::IInspectable, Helper::isObservable, true, Helper::isDependencyObjectBase, Helper::isNoTrackerRef>
{
};
