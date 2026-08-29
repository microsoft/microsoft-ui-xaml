// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Helper class used to represent a pseudo-keyed set of timelines. This class contains
//  a list of timelines which is created by flattening one or more storyboards. (Flatten because
//  a storyboard can contain other storyboards...)
//
//  The list nodes are "keyed" on the DO and DP which their timeline affects.

class TimelineLookupList
{
public:
    class Node
    {
    public:
        CTimeline* m_timeline = nullptr;
        xref::weakref_ptr<CDependencyObject> m_doWeakRef;
        KnownPropertyIndex m_propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
        CDynamicTimeline* m_dynamicTimeline = nullptr;
        const VisualStateSetterHelper::ResolvedVisualStateSetter* m_setter = nullptr;

        _Check_return_ HRESULT Initialize(_In_ CTimeline* timeline, _In_opt_ CDynamicTimeline* dynamicTimeline)
        {
            m_timeline = timeline;
            m_dynamicTimeline = dynamicTimeline;
            m_setter = nullptr;

            bool allowingTransitionTargetCreation = false;
            if (dynamicTimeline)
            {
                allowingTransitionTargetCreation = timeline->GetContextInterface()->IsAllowingTransitionTargetCreations();
                timeline->GetContextInterface()->SetAllowTransitionTargetCreation(TRUE);
            }
            auto resetTransitionTargetCreation = wil::scope_exit([&] {
                if (dynamicTimeline && !allowingTransitionTargetCreation)
                {
                    timeline->GetContextInterface()->SetAllowTransitionTargetCreation(FALSE);
                }
            });

            const CDependencyProperty* doProperty = nullptr;
            IFC_RETURN(static_cast<CAnimation*>(timeline)->GetAnimationTargets(
                &m_doWeakRef, &doProperty));

            m_propertyIndex = doProperty->GetIndex();
            return S_OK;
        }

        Node() = default;
        // The copy constructor is no longer implicitly declared because Node has a
        // user-defined move constructor. As a result, it does not exist and we dont need to
        // explicitly delete it.

        Node(_In_ const VisualStateSetterHelper::ResolvedVisualStateSetter* setter) noexcept
        {
            m_setter = setter;
            m_timeline = nullptr;
            m_dynamicTimeline = nullptr;
            m_doWeakRef = setter->get_TargetObject();
            m_propertyIndex = setter->get_TargetProperty()->GetIndex();
        }

        Node(Node&& other) noexcept
        {
            m_timeline = other.m_timeline;
            m_doWeakRef = std::move(other.m_doWeakRef);
            m_propertyIndex = other.m_propertyIndex;
            m_dynamicTimeline = other.m_dynamicTimeline;
            m_setter = other.m_setter;
        }

        Node& operator=(Node&& other) noexcept
        {
            if (this != &other)
            {
                m_timeline = other.m_timeline;
                m_doWeakRef = std::move(other.m_doWeakRef);
                m_propertyIndex = other.m_propertyIndex;
                m_dynamicTimeline = other.m_dynamicTimeline;
                m_setter = other.m_setter;
            }

            return *this;
        }

        bool operator==(_In_ const Node& other) const noexcept
        {
            return other.m_doWeakRef == m_doWeakRef &&
                other.m_propertyIndex == m_propertyIndex;
        }
    };

    _Check_return_ HRESULT Initialize(_In_ const std::vector<CStoryboard*>& storyboards)
    {
        IFC_RETURN(FlattenStoryboardList(storyboards));
        return S_OK;
    }

    _Check_return_ HRESULT Initialize(_In_opt_ CParallelTimeline* storyboard)
    {
        IFC_RETURN(FlattenStoryboard(static_cast<CParallelTimeline*>(storyboard)));
        return S_OK;
    }

    template<size_t storyboardCount>
    _Check_return_ HRESULT Initialize(_In_ const std::array<CStoryboard*, storyboardCount>& storyboards)
    {
        IFC_RETURN(FlattenStoryboardList<storyboardCount>(storyboards));
        return S_OK;
    }

    TimelineLookupList() = default;

    TimelineLookupList(_In_ const VisualStateSetterHelper::ResolvedVisualStateSetterCollection& setters)
    {
        for (auto& setter : setters)
        {
            Node node(&setter);
            m_list.emplace_back(std::move(node));
        }
    }

    _Check_return_ HRESULT FlattenTimeline(_In_opt_ CTimeline* timeline,
        _In_opt_ CDynamicTimeline* dynamicTimeline)
    {
        if (timeline)
        {
            // If the timeline is actually a Storyboard we could have
            // nested Timelines.
            auto storyboard = do_pointer_cast<CStoryboard>(timeline);
            auto childDynamicTimeline = do_pointer_cast<CDynamicTimeline>(timeline);
            // DynamicTimelines do not nest.
            ASSERT(!(childDynamicTimeline && dynamicTimeline));
            auto usableDynamicTimline = dynamicTimeline ? dynamicTimeline : childDynamicTimeline;

            if (storyboard || childDynamicTimeline)
            {
                IFC_RETURN(FlattenStoryboard(static_cast<CParallelTimeline*>(timeline), usableDynamicTimline));
            }
            else
            {
                Node node;
                IFC_RETURN(node.Initialize(timeline, usableDynamicTimline));
                m_list.emplace_back(std::move(node));
            }
        }
        return S_OK;
    }

    _Check_return_ HRESULT FlattenStoryboard(_In_opt_ CParallelTimeline* pStoryboard,
        _In_opt_ CDynamicTimeline* pDynamicTimeline = nullptr)
    {
        CTimelineCollection* pChildren = pStoryboard ? pStoryboard->m_pChild : nullptr;
        if (pChildren)
        {
            for (auto timeline : *pChildren)
            {
                IFC_RETURN(FlattenTimeline(static_cast<CTimeline*>(timeline), pDynamicTimeline));
            }
        }
        return S_OK;
    }

    _Check_return_ HRESULT FlattenStoryboardList(const std::vector<CStoryboard*>& storyboards)
    {
        for (auto storyboard : storyboards)
        {
            IFC_RETURN(FlattenStoryboard(storyboard, nullptr));
        }
        return S_OK;
    }

    template<size_t storyboardCount>
    _Check_return_ HRESULT FlattenStoryboardList(const std::array<CStoryboard*, storyboardCount>& storyboards)
    {
        for (auto storyboard : storyboards)
        {
            IFC_RETURN(FlattenStoryboard(storyboard, nullptr));
        }
        return S_OK;
    }

    // Remove nodes in this list which affect the same DO/DP pairs as
    // the keyed value from another list
    void RemoveMatchingNodes(const Node& keyedValue)
    {
        m_list.erase(std::remove(m_list.begin(), m_list.end(), keyedValue), m_list.end());
    }

    std::vector<Node>::const_iterator begin() const { return m_list.begin(); }
    std::vector<Node>::const_iterator end() const { return m_list.end(); }

private:
    std::vector<Node> m_list;
};