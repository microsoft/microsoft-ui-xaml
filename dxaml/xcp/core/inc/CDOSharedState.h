// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ValueObjectBase.h>
#include "DirtyFlags.h"         // for RENDERCHANGEDPFN
#include <functional>           // for std::hash
#include <xref_ptr.h>
#include <CommonUtilities.h>
#include <weakref_ptr.h>

class CDOSharedState;
class CCoreServices;
class VisualTree;
struct IPALUri;

class CDOSharedState
{
    // std::tie needs to work directly with member fields.
    friend bool Flyweight::Operators::equal<CDOSharedState>(const CDOSharedState&, const CDOSharedState&);
    friend bool Flyweight::Operators::less<CDOSharedState>(const CDOSharedState&, const CDOSharedState&);

public:
    using Wrapper = Flyweight::ValueObjectWrapper<CDOSharedState>;

    static constexpr CCoreServices*     s_defaultCoreServices           = nullptr;
    static constexpr RENDERCHANGEDPFN   s_defaultRenderChangedHandler   = nullptr;
    static constexpr IPALUri*           s_defaultBaseUri                = nullptr;

    CDOSharedState() = delete;

    CDOSharedState(
        CCoreServices* coreServices,
        RENDERCHANGEDPFN renderChangedHandler,
        IPALUri* baseUri,
        xref::weakref_ptr<VisualTree> visualTree)
        : m_coreServices(coreServices)
        , m_renderChangedHandler(renderChangedHandler)
        , m_baseUri(baseUri)
        , m_visualTreeWeak(std::move(visualTree))
    {}

    CCoreServices* GetCoreServices() const
    {
        return m_coreServices;
    }

    RENDERCHANGEDPFN GetRenderChangedHandler() const
    {
        return m_renderChangedHandler;
    }

    IPALUri* GetBaseUri() const
    {
        return m_baseUri;
    }

    xref::weakref_ptr<VisualTree> GetVisualTree() const
    {
        return m_visualTreeWeak;
    }

private:
    CCoreServices*                  m_coreServices{ nullptr };
    RENDERCHANGEDPFN                m_renderChangedHandler{ nullptr };
    xref_ptr<IPALUri>               m_baseUri{ nullptr };
    xref::weakref_ptr<VisualTree>   m_visualTreeWeak;
};

namespace Flyweight
{
    namespace Operators
    {
        template <>
        inline CDOSharedState Default()
        {
            return CDOSharedState(
                CDOSharedState::s_defaultCoreServices,
                CDOSharedState::s_defaultRenderChangedHandler,
                CDOSharedState::s_defaultBaseUri,
                xref::weakref_ptr<VisualTree>());
        }

        template <>
        inline bool equal(const CDOSharedState& lhs, const CDOSharedState& rhs)
        {
            return std::tie(lhs.m_baseUri, lhs.m_renderChangedHandler, lhs.m_coreServices, lhs.m_visualTreeWeak) ==
                   std::tie(rhs.m_baseUri, rhs.m_renderChangedHandler, rhs.m_coreServices, rhs.m_visualTreeWeak);
        }

        template <>
        inline bool less(const CDOSharedState& lhs, const CDOSharedState& rhs)
        {
            return std::tie(lhs.m_baseUri, lhs.m_renderChangedHandler, lhs.m_coreServices, lhs.m_visualTreeWeak) <
                   std::tie(rhs.m_baseUri, rhs.m_renderChangedHandler, rhs.m_coreServices, rhs.m_visualTreeWeak);
        }

        template <>
        inline std::size_t hash(const CDOSharedState& inst)
        {
            std::size_t hash = 0;
            CommonUtilities::hash_combine(hash, inst.GetCoreServices());
            CommonUtilities::hash_combine(hash, inst.GetRenderChangedHandler());
            CommonUtilities::hash_combine(hash, inst.GetBaseUri());
            CommonUtilities::hash_combine(hash, inst.GetVisualTree());
            return hash;
        }
    }
}