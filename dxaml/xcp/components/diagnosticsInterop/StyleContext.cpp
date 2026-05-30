// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "inc\StyleContext.h"
#include "Resources.h"
#include "ObjectWriterStack.h"
#include "ObjectWriterFrame.h"
#include "DOPointerCast.h"
#include "Setter.h"

namespace Diagnostics
{
    StyleContext::StyleContext(std::unique_ptr<CustomWriterRuntimeContext> context)
        : m_cachedContext(std::move(context))
        , m_flags(StyleContextFlags::ParserContext)
    {
    }

    StyleContext::StyleContext(
        _In_ CDependencyObject* parentObject,
        bool isImplicit)
        : m_parent(xref::get_weakref(parentObject))
        , m_flags(StyleContextFlags::RuntimeContext)
    {
        if (isImplicit)
        {
            SetIsImplicit();
        }
    }

    xref_ptr<CDependencyObject> StyleContext::GetParent()
    {
        xref_ptr<CDependencyObject> parent;
        if (HasFlag(StyleContextFlags::ParserContext))
        {
            auto parentFrame = m_cachedContext->GetObjectWriterStack()->Parent();
            if (parentFrame.exists_WeakRefInstance())
            {
                parent = parentFrame.get_WeakRefInstance().lock();
                if (auto parentAsStyle = do_pointer_cast<CStyle>(parent))
                {
                    auto currentFrame = m_cachedContext->GetObjectWriterStack()->Current();
                    if (currentFrame.exists_WeakRefInstance())
                    {
                        auto current = currentFrame.get_WeakRefInstance().lock();
                        // If the parent is a style, then we need to find the actual setter we are associated with
                        for (const auto setter : *parentAsStyle->GetSetterCollection())
                        {
                            CValue setterValue;
                            IFCFAILFAST(setter->GetValueByIndex(KnownPropertyIndex::Setter_Value, &setterValue));
                            if (setterValue.AsObject() == current)
                            {
                                parent = setter;
                                break;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            parent = m_parent.lock();
        }
        
        return parent;
    }

    CustomWriterRuntimeContext* StyleContext::GetCachedContext()
    {
        return HasFlag(StyleContextFlags::ParserContext) ? m_cachedContext.get() : nullptr;
    }

    bool StyleContext::IsImplicit()
    {
        return HasFlag(StyleContextFlags::ImplicitStyle);
    }

    void StyleContext::SetIsImplicit()
    {
        m_flags |= StyleContextFlags::ImplicitStyle;
    }

    bool StyleContext::HasFlag(StyleContextFlags flag)
    {
        return (m_flags & flag) == flag;
    }
}
