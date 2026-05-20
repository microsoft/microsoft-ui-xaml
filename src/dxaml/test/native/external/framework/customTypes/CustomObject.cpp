// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomObject.h"

using namespace Tests::Native::External::Framework;

unsigned int CustomObject::InstanceCount = 0U;

CustomObject::CustomObject()
{
    InstanceCount++;
}
CustomObject::~CustomObject()
{
    InstanceCount--;
}
void CustomObject::ClearInstanceCount()
{
    InstanceCount = 0;
}
unsigned int CustomObject::GetInstanceCount()
{
    return InstanceCount;
}