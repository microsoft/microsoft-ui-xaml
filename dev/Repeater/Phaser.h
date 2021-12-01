// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class ItemsRepeater;

struct ElementInfo
{
    ElementInfo(const winrt::UIElement&  element, const winrt::com_ptr<VirtualizationInfo>& virtInfo) :
        m_element(element),
        m_virtInfo(virtInfo)
    {}

    winrt::UIElement Element() const { return m_element; }
    winrt::com_ptr<VirtualizationInfo> VirtInfo() const { return m_virtInfo; }
private:
    winrt::UIElement m_element{ nullptr };
    winrt::com_ptr<VirtualizationInfo> m_virtInfo{ nullptr };
};

class Phaser final
{
public:
    Phaser(ItemsRepeater* owner);
    void PhaseElement(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo);
    void StopPhasing(const winrt::UIElement& element, const winrt::com_ptr<VirtualizationInfo>& virtInfo);

private:
    void DoPhasedWorkCallback();
    void RegisterForCallback();
    void MarkCallbackRecieved();
    void SortElements(const winrt::Rect& visibleWindow);
    static void ValidatePhaseOrdering(int currentPhase, int nextPhase);

    ItemsRepeater* m_owner{ nullptr };
    std::vector<ElementInfo> m_pendingElements{};
    bool m_registeredForCallback{ false };
};
