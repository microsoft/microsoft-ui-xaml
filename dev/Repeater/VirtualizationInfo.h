// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <limits>

enum class ElementOwner
{
    // All elements are originally owned by the view generator.
    ElementFactory,
    // Ownership is transferred to the layout when it calls GetElement.
    Layout,
    // Ownership is transferred to the pinned pool if the element is cleared (outside of
    // a 'remove' collection change of course).
    PinnedPool,
    // Ownership is transfered to the reset pool if the element is cleared by a reset and
    // the data source supports unique ids.
    UniqueIdResetPool,
    // Ownership is transfered to the animator if the element is cleared due to a
    // 'remove'-like collection change.
    Animator
};

// Would be nice to have this be part of UIElement similar to how MCBP does it.
// That would make the lookups much more performant than an attached property.
class VirtualizationInfo : public winrt::implements<VirtualizationInfo, winrt::IInspectable>
{
public:
    VirtualizationInfo();

    ElementOwner Owner() const { return m_owner; }
    int Index() const { return m_index; }

    // Pinned means that the element is protected from getting cleared by layout.
    // A pinned element may still get cleared by a collection change.
    // IsPinned == true doesn't necessarly mean that the element is currently
    // owned by the PinnedPool, only that its ownership may be transferred to the
    // PinnedPool if it gets cleared by layout.
    bool IsPinned() const;
    bool IsHeldByLayout() const;
    bool IsRealized() const;
    bool IsInUniqueIdResetPool() const;

    // Info for phasing
    void UpdatePhasingInfo(int phase, const winrt::IInspectable& data, const winrt::IDataTemplateComponent& component);
    int Phase() const { return m_phase; }
    void Phase(int phase) { m_phase = phase; }
    winrt::IInspectable Data() const { return m_data.get(); }
    winrt::IDataTemplateComponent DataTemplateComponent() const { return m_dataTemplateComponent.get(); }

    bool MustClearDataContext() const { return m_mustClearDataContext; }
    void MustClearDataContext(bool mustClearDataContext) { m_mustClearDataContext = mustClearDataContext; }

    static constexpr int PhaseNotSpecified = std::numeric_limits<int>::min();
    static constexpr int PhaseReachedEnd = -1;

#pragma region Ownership state machine

    void MoveOwnershipToLayoutFromElementFactory(int index, wstring_view uniqueId);
    void MoveOwnershipToLayoutFromUniqueIdResetPool();
    void MoveOwnershipToLayoutFromPinnedPool();
    void MoveOwnershipToElementFactory();
    void MoveOwnershipToUniqueIdResetPoolFromLayout();
    void MoveOwnershipToAnimator();
    void MoveOwnershipToPinnedPool();

#pragma endregion

    unsigned AddPin();
    unsigned RemovePin();

    void UpdateIndex(int newIndex);

    winrt::Rect ArrangeBounds() const { return m_arrangeBounds; }
    void ArrangeBounds(winrt::Rect value) { m_arrangeBounds = value; }

    wstring_view UniqueId() const { return m_uniqueId; }

#pragma region Keep element from being recycled
    bool KeepAlive() { return m_keepAlive; }
    void KeepAlive(bool value) { m_keepAlive = value; }
#pragma endregion

    bool AutoRecycleCandidate() { return m_autoRecycleCandidate; }
    void AutoRecycleCandidate(bool value) { m_autoRecycleCandidate = value; }

private:
    unsigned m_pinCounter{ 0u };
    int m_index{ -1 };
    winrt::hstring m_uniqueId;
    ElementOwner m_owner{ ElementOwner::ElementFactory };
    winrt::Rect m_arrangeBounds;
    int m_phase{ PhaseNotSpecified };
    bool m_keepAlive{ false };
    bool m_autoRecycleCandidate{ false };
    bool m_mustClearDataContext{ false };

    weak_ref<winrt::IInspectable> m_data;
    weak_ref<winrt::IDataTemplateComponent> m_dataTemplateComponent;
};
