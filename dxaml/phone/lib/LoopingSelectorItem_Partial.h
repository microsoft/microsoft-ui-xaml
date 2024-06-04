// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{
    class LoopingSelectorItem :
        public LoopingSelectorItemGenerated
    {

    public:
        enum class State
        {
            Normal,
            Expanded,
            Selected,
            PointerOver,
            Pressed,
        };

        LoopingSelectorItem();

        // IUIElementOverrides
        _Check_return_ HRESULT OnCreateAutomationPeerImpl(
            _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue) override;

        // Internal automation methods
        _Check_return_ HRESULT AutomationSelect();
        _Check_return_ HRESULT AutomationGetSelectionContainerUIAPeer(_Outptr_result_maybenull_ xaml::Automation::Peers::IAutomationPeer** ppPeer);
        _Check_return_ HRESULT AutomationGetIsSelected(_Out_ BOOLEAN *value);
        _Check_return_ HRESULT AutomationUpdatePeerIfExists(_In_ int itemIndex);

        // Internal methods
        _Check_return_ HRESULT SetState(_In_ State newState, _In_ BOOLEAN useTransitions);

        _Check_return_ HRESULT GetVisualIndex(_Out_ INT* idx)
        {
            *idx = _visualIndex;
            return S_OK;
        }

        _Check_return_ HRESULT SetVisualIndex(_In_ INT idx)
        {
            _visualIndex = idx;
            return S_OK;
        }

        _Check_return_ HRESULT SetParent(_In_ LoopingSelector* pValue)
        {
            _pParentNoRef = pValue;
            return S_OK;
        }

        _Check_return_ HRESULT GetParentNoRef(_Outptr_ LoopingSelector** ppValue)
        {
            *ppValue = _pParentNoRef;
            return S_OK;
        }

    protected:

        _Check_return_ HRESULT OnPointerEnteredImpl(_In_ xaml_input::IPointerRoutedEventArgs* e) override;
        _Check_return_ HRESULT OnPointerPressedImpl(_In_ xaml_input::IPointerRoutedEventArgs* e) override;
        _Check_return_ HRESULT OnPointerExitedImpl(_In_ xaml_input::IPointerRoutedEventArgs* e) override;
        _Check_return_ HRESULT OnPointerCaptureLostImpl(_In_ xaml_input::IPointerRoutedEventArgs* e) override;

    private:
        ~LoopingSelectorItem() {}

        _Check_return_ HRESULT InitializeImpl() override;

        _Check_return_ HRESULT GoToState(_In_ State newState, _In_ BOOLEAN useTransitions);

        State _state;

        // The visual index of the data item this item is displaying.
        // Note: Due to the looping behavior, this is not equal to the index of the item in the collection.
        // e.g. consider the case of Minute 59 looping around to 0. The Item after 59 does not have an index of 0. It has an index of 60.
        INT _visualIndex;

        // The parent is used by the AutomationPeer for ScrollIntoView
        // and Selection. We don't keep a strong ref to prevent cycles.
        LoopingSelector* _pParentNoRef;

        // There's no way to know if an AP has been created except to
        // keep track with an internal boolean.
        BOOLEAN _hasPeerBeenCreated;
    };

} } } } } XAML_ABI_NAMESPACE_END
