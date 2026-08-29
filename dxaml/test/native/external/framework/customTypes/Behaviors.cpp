// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "Behaviors.h"

using namespace Microsoft::UI::Xaml;
using namespace ::Tests::Native::External::Framework;

DependencyProperty^ Interactivity::m_BehaviorsProperty = nullptr;
DependencyProperty^ MyBehavior::m_ActionsProperty = nullptr;

void Interactivity::RegisterDependencyProperties()
{
    if (!m_BehaviorsProperty)
    {
        m_BehaviorsProperty = DependencyProperty::RegisterAttached(
            L"Behaviors",
            Platform::Object::typeid,
            Interactivity::typeid,
            nullptr);
    }
}

void MyBehavior::RegisterDependencyProperties()
{
    if (!m_ActionsProperty)
    {
        m_ActionsProperty = DependencyProperty::Register(
            L"Actions",
            Platform::Object::typeid,
            MyBehavior::typeid,
            nullptr);
    }
}

void Interactivity::ClearDependencyProperties()
{
    m_BehaviorsProperty = nullptr;
}

void MyBehavior::ClearDependencyProperties()
{
    m_ActionsProperty = nullptr;
}


