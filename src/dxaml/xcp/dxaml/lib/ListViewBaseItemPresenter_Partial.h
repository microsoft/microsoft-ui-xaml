// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ListViewBaseItemPresenter.g.h"

namespace DirectUI
{

PARTIAL_CLASS(ListViewBaseItemPresenter), private ListViewBaseItemAnimationCommandVisitor
{
    public:

    ListViewBaseItemPresenter()
    {}

    ~ListViewBaseItemPresenter() override
    {}

    protected:

        _Check_return_ HRESULT ProcessAnimationCommands();

    private:
        struct AnimationState
        {
            AnimationState()
                : pCommand(nullptr)
            { }

            ~AnimationState()
            {
                tpStoryboard.Clear();
                delete pCommand;
            }

            TrackerPtr<xaml_animation::IStoryboard> tpStoryboard; // Ref is stored by parent class.
            ListViewBaseItemAnimationCommand* pCommand;
        };

        // ListViewBaseItemAnimationCommandVisitor impl
        _Check_return_ HRESULT VisitAnimationCommand(_In_ ListViewBaseItemAnimationCommand_Pressed* pCommand) override;
        _Check_return_ HRESULT VisitAnimationCommand(_In_ ListViewBaseItemAnimationCommand_ReorderHint* pCommand) override;
        _Check_return_ HRESULT VisitAnimationCommand(_In_ ListViewBaseItemAnimationCommand_DragDrop* pCommand) override;
        _Check_return_ HRESULT VisitAnimationCommand(_In_ ListViewBaseItemAnimationCommand_MultiSelect* pCommand) override;
        _Check_return_ HRESULT VisitAnimationCommand(_In_ ListViewBaseItemAnimationCommand_IndicatorSelect* pCommand) override;
        _Check_return_ HRESULT VisitAnimationCommand(_In_ ListViewBaseItemAnimationCommand_SelectionIndicatorVisibility* pCommand) override;

        _Check_return_ HRESULT ClearAnimation(_In_ AnimationState* pAnimation);

        _Check_return_ HRESULT OnReorderHintReturnCompleted(
            _In_opt_ IInspectable* pUnused1,
            _In_opt_ IInspectable* pUnused2);

        _Check_return_ HRESULT OnMultiSelectCompleted(
            _In_opt_ IInspectable* pUnused1,
            _In_opt_ IInspectable* pUnused2);

        _Check_return_ HRESULT OnIndicatorSelectCompleted(
            _In_opt_ IInspectable* unused1,
            _In_opt_ IInspectable* unused2);

        _Check_return_ HRESULT OnSelectionIndicatorCompleted(
            _In_opt_ IInspectable* unused1,
            _In_opt_ IInspectable* unused2);

        // Gets the value of a resource by querying the ThemeResource dictionary
        _Check_return_ HRESULT GetValueFromThemeResources(
            _In_ const wrl_wrappers::HStringReference& resourceKey,
            _Out_ double* pValue);

        // Private state
        AnimationState m_pointerPressedAnimation;
        AnimationState m_reorderHintAnimation;
        EventRegistrationToken m_reorderHintBackCompletedToken;
        AnimationState m_dragDropAnimation;
        AnimationState m_multiSelectAnimation;
        EventRegistrationToken m_multiSelectCompletedToken;
        AnimationState m_indicatorSelectAnimation;
        EventRegistrationToken m_indicatorSelectCompletedToken;
        AnimationState m_selectionIndicatorAnimation;
        EventRegistrationToken m_selectionIndicatorCompletedToken;
};

}
