// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FocusRectHost.h"

#include <CDependencyObject.h>
#include <uielement.h>

FocusRectHost::FocusRectHost()
    : Element(nullptr)
    , type(FocusRectHost::Type::None)
    , ScrollContentPresenter(nullptr)
{
}

FocusRectHost::FocusRectHost(_In_ CDependencyObject* obj, _In_ FocusRectHost::Type type, _In_ CUIElement* scp)
    : type(type)
    , ScrollContentPresenter(scp)
{
    FAIL_FAST_ASSERT(!obj || obj->OfTypeByIndex<KnownTypeIndex::UIElement>());
    Element = static_cast<CUIElement*>(obj);
    ASSERT((type == Type::None && Element == nullptr) ||
            (type != Type::None && Element != nullptr));

    if (   type == FocusRectHost::Type::ScrollContentPresenterChild
        || type == FocusRectHost::Type::ScrollContentPresenterParent
        || type == FocusRectHost::Type::ScrollContentPresenterPeer)
    {
        ASSERT(ScrollContentPresenter != nullptr);
    }
    else
    {
        ASSERT(ScrollContentPresenter == nullptr);
    }
}