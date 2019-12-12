// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ElementFactory.h"

#include "ElementFactory.properties.cpp"

#pragma region IElementFactory

winrt::UIElement ElementFactory::GetElement(winrt::ElementFactoryGetArgs const& args)
{
    return overridable().GetElementCore(args);
}

void ElementFactory::RecycleElement(winrt::ElementFactoryRecycleArgs const& args)
{
    overridable().RecycleElementCore(args);
}

#pragma endregion

#pragma region IElementFactoryOverrides

winrt::UIElement ElementFactory::GetElementCore(winrt::ElementFactoryGetArgs const& args)
{
    throw winrt::hresult_not_implemented();
}

void ElementFactory::RecycleElementCore(winrt::ElementFactoryRecycleArgs const& args)
{
    throw winrt::hresult_not_implemented();
}

#pragma endregion
