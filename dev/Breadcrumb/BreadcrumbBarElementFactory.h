// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "common.h"
#include "ElementFactory.h"

class BreadcrumbElementFactory :
    public winrt::implements<BreadcrumbElementFactory, ElementFactory>
{
public:
    BreadcrumbElementFactory();

    void UserElementFactory(const winrt::IInspectable& newValue);
    winrt::UIElement GetElementCore(const winrt::ElementFactoryGetArgs& args) override;
    void RecycleElementCore(const winrt::ElementFactoryRecycleArgs& args) override;

private:
    winrt::IElementFactoryShim m_itemTemplateWrapper{ nullptr };
};

