// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include <base\inc\weakref_ptr.h>
#include <Style.h>
#include <collection\inc\SetterBaseCollection.h>
#include <deferral\inc\CustomWriterRuntimeContext.h>

class CDependencyObject;

namespace Diagnostics
{
    enum class StyleContextFlags: uint8_t
    {
        ImplicitStyle = 0x1,  // Context for updating implicit styles
        ParserContext = 0x2,  // Stored during parse time
        RuntimeContext = 0x4, // Added to dictionary during EnC
    };
    DEFINE_ENUM_FLAG_OPERATORS(StyleContextFlags);

    class StyleContext final
    {
    public:
        StyleContext(std::unique_ptr<CustomWriterRuntimeContext> context);

        StyleContext(
            _In_ CDependencyObject* parent,
            bool isImplicit);
        
        StyleContext(const StyleContext& rhs) = delete;
        StyleContext(StyleContext&& rhs) = default;
        StyleContext& operator=(const StyleContext& rhs) = delete;
        StyleContext& operator=(StyleContext&& rhs) = default;
        
        xref_ptr<CDependencyObject>                 GetParent();
        CustomWriterRuntimeContext*                 GetCachedContext();
        bool                                        IsImplicit();
        void                                        SetIsImplicit();
        
    private:
        bool                                        HasFlag(StyleContextFlags flag);
    private:

        xref::weakref_ptr<CDependencyObject>        m_parent;
        std::unique_ptr<CustomWriterRuntimeContext> m_cachedContext;
        StyleContextFlags                           m_flags;
    };
}
