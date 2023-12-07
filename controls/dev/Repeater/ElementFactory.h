// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ElementFactory.g.h"

class ElementFactory :
    public winrt::implementation::ElementFactoryT<ElementFactory>
{
public:
#pragma region IElementFactory
    winrt::UIElement GetElement(winrt::ElementFactoryGetArgs const& args);
    void RecycleElement(winrt::ElementFactoryRecycleArgs const& args);
#pragma endregion

#pragma region IElementFactoryOverrides
    virtual winrt::UIElement GetElementCore(winrt::ElementFactoryGetArgs const& args);
    virtual void RecycleElementCore(winrt::ElementFactoryRecycleArgs const& args);
#pragma endregion

    // TODO: Bug 14901501: Figure out a better way to have reference tracking for types doing in-component derivation (e.g. RecyclingElementFactory : ElementFactory)
    virtual int32_t __stdcall NonDelegatingQueryInterface(const winrt::guid& id, void** object)
    {
        return __super::NonDelegatingQueryInterface(id, object);
    }

    virtual unsigned long __stdcall NonDelegatingAddRef()
    {
        return __super::NonDelegatingAddRef();
    }

    virtual unsigned long __stdcall NonDelegatingRelease()
    {
        return __super::NonDelegatingRelease();
    }

};
