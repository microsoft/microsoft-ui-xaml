// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <stack_vector.h>
#include <weakref_ptr.h>
#include <IndependentAnimationType.h>

struct IATarget
{
    IndependentAnimationType animationType = IndependentAnimationType::None;
    xref::weakref_ptr<CDependencyObject> targetWeakRef;
};

typedef Jupiter::stack_vector<IATarget, 4> IATargetVectorWrapper;
typedef IATargetVectorWrapper::vector_t IATargetVector;
