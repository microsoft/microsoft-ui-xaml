// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemsRepeater.common.h"
#include "QPCTimer.h"
#include "BuildTreeScheduler.h"
#include "VirtualizationInfo.h"
#include "ItemsRepeater.h"
#include "Phaser.h"

Phaser::Phaser(ItemsRepeater* owner) :
    m_owner(owner)
{
    // ItemsRepeater is not fully constructed yet. Don't interact with it.
}

void Phaser::PhaseElement(
    const winrt::UIElement& element,
    const winrt::com_ptr<VirtualizationInfo>& virtInfo)
{
    auto dataTemplateComponent = virtInfo->DataTemplateComponent();
    auto nextPhase = virtInfo->Phase();
    bool shouldPhase = false;

    if (nextPhase > 0)
    {
        if (dataTemplateComponent)
        {
            // Perf optimization: RecyclingElementFactory sets up the virtualization info so we don't need to update it here.
            shouldPhase = true;
        }
        else
        {
            throw winrt::hresult_error(E_FAIL, L"Phase was set on virtualization info, but dataTemplateComponent was not.");
        }
    }
    else if(nextPhase == VirtualizationInfo::PhaseNotSpecified)
    {
        // If virtInfo->Phase is not specified, virtInfo->DataTemplateComponent cannot be valid.
        MUX_ASSERT(!dataTemplateComponent);
        // ItemsRepeater might be using a custom view generator in which case, virtInfo would not be bootstrapped.
        // In this case, fallback to querying for the data template component and setup virtualization info now.
        dataTemplateComponent = winrt::XamlBindingHelper::GetDataTemplateComponent(element);
        if (dataTemplateComponent)
        {
            // Clear out old data. 
            dataTemplateComponent.Recycle();

            nextPhase = VirtualizationInfo::PhaseReachedEnd;
            const auto index = virtInfo->Index();
            const auto data = m_owner->ItemsSourceView().GetAt(index);
            // Run Phase 0
            dataTemplateComponent.ProcessBindings(data, index, 0 /* currentPhase */, nextPhase);

            // Update phase on virtInfo. Set data and templateComponent only if x:Phase was used.
            virtInfo->UpdatePhasingInfo(nextPhase, nextPhase > 0 ? data : nullptr, nextPhase > 0 ? dataTemplateComponent : nullptr);
            shouldPhase = nextPhase > 0;
        }
    }

    if (shouldPhase)
    {
        // Insert at the top since we remove from bottom during DoPhasedWorkCallback. This keeps the ordering of items
        // the same as the order in which items are realized.
        m_pendingElements.insert(m_pendingElements.begin(), ElementInfo(element, virtInfo));
        RegisterForCallback();
    }
}

void Phaser::StopPhasing(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo)
{
    // We need to remove the element from the pending elements list. We cannot just change the phase to -1
    // since it will get updated when the element gets recycled.
    if (virtInfo->DataTemplateComponent())
    {
        auto it = std::find_if(m_pendingElements.begin(), m_pendingElements.end(), [&element](const ElementInfo& info) { return info.Element() == element; });

        if (it != m_pendingElements.end())
        {
            m_pendingElements.erase(it);
        }
    }

    // Clean Phasing information for this item.
    virtInfo->UpdatePhasingInfo(VirtualizationInfo::PhaseNotSpecified, nullptr /* data */, nullptr /* dataTemplateComponent */);
}

void Phaser::DoPhasedWorkCallback()
{
    MarkCallbackRecieved();

    if (!m_pendingElements.empty() && !BuildTreeScheduler::ShouldYield())
    {
        const auto visibleWindow = m_owner->VisibleWindow();
        SortElements(visibleWindow);
        int currentIndex = static_cast<int>(m_pendingElements.size()) - 1;
        do
        {
            const auto info = m_pendingElements[currentIndex];
            const auto element = info.Element();
            const auto virtInfo = info.VirtInfo();
            const auto dataIndex = virtInfo->Index();

            const int currentPhase = virtInfo->Phase();
            if (currentPhase > 0)
            {
                int nextPhase = VirtualizationInfo::PhaseReachedEnd;
                virtInfo->DataTemplateComponent().ProcessBindings(virtInfo->Data(), -1 /* item index unused */, currentPhase, nextPhase);
                ValidatePhaseOrdering(currentPhase, nextPhase);

                const auto previousAvailableSize = winrt::LayoutInformation::GetAvailableSize(element);
                element.Measure(previousAvailableSize);

                if (nextPhase > 0)
                {
                    virtInfo->Phase(nextPhase);
                    // If we are the first item or 
                    // If the current items phase is higher than the next items phase, then move to the next item.
                    if (currentIndex == 0 ||
                        virtInfo->Phase() > m_pendingElements[currentIndex - 1].VirtInfo()->Phase())
                    {
                        currentIndex--;
                    }
                }
                else
                {
                    m_pendingElements.erase(m_pendingElements.begin() + currentIndex);
                    currentIndex--;
                }
            }
            else
            {
                throw winrt::hresult_error(E_FAIL, L"Cleared element found in pending list which is not expected");
            }

            const auto pendingCount = static_cast<int>(m_pendingElements.size());
            if (currentIndex == -1)
            {
                // Reached the top, start from the bottom again
                currentIndex = pendingCount - 1;
            }
            else if (currentIndex > -1 && currentIndex < pendingCount - 1)
            {
                // If the next element is oustide the visible window and there are elements in the visible window
                // go back to the visible window.
                const bool nextItemIsVisible = SharedHelpers::DoRectsIntersect(visibleWindow, m_pendingElements[currentIndex].VirtInfo()->ArrangeBounds());
                if (!nextItemIsVisible)
                {
                    const bool haveVisibleItems = SharedHelpers::DoRectsIntersect(visibleWindow, m_pendingElements[pendingCount - 1].VirtInfo()->ArrangeBounds());
                    if (haveVisibleItems)
                    {
                        currentIndex = pendingCount - 1;
                    }
                }
            }
        } while (!m_pendingElements.empty() && !BuildTreeScheduler::ShouldYield());
    }

    if (!m_pendingElements.empty())
    {
        RegisterForCallback();
    }
}

void Phaser::RegisterForCallback()
{
    if (!m_registeredForCallback)
    {
        MUX_ASSERT(!m_pendingElements.empty());
        m_registeredForCallback = true;
        BuildTreeScheduler::RegisterWork(
            m_pendingElements[m_pendingElements.size() - 1].VirtInfo()->Phase(), // Use the phase of the last one in the sorted list
            [this]()
        {
            DoPhasedWorkCallback();
        });
    }
}

void Phaser::MarkCallbackRecieved()
{
    m_registeredForCallback = false;
}

/* static */
void Phaser::ValidatePhaseOrdering(int currentPhase, int nextPhase)
{
    if (nextPhase > 0 && nextPhase <= currentPhase)
    {
        // nextPhase <= currentPhase is invalid
        throw winrt::hresult_error(E_FAIL, L"Phases are required to be monotonically increasing.");
    }
}

void Phaser::SortElements(const winrt::Rect& visibleWindow)
{
    // Sort in descending order (inVisibleWindow, phase)
    std::sort(
        m_pendingElements.begin(),
        m_pendingElements.end(),
        [visibleWindow](const auto& lhs, const auto& rhs)
    {
        const auto lhsBounds = lhs.VirtInfo()->ArrangeBounds();
        const auto lhsIntersects = SharedHelpers::DoRectsIntersect(lhsBounds, visibleWindow);
        const auto rhsBounds = rhs.VirtInfo()->ArrangeBounds();
        const auto rhsIntersects = SharedHelpers::DoRectsIntersect(rhsBounds, visibleWindow);

        if ((lhsIntersects && rhsIntersects) ||
            (!lhsIntersects && !rhsIntersects))
        {
            // Both are in the visible window or both are not
            return lhs.VirtInfo()->Phase() < rhs.VirtInfo()->Phase();
        }
        else if (lhsIntersects)
        {
            // Left is in visible window
            return false;
        }
        else
        {
            return true;
        }
    }
    );
}
