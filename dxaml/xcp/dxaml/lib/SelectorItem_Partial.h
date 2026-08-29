// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SelectorItem.g.h"

namespace DirectUI
{
    // Represents a selectable item in a Selector.
    PARTIAL_CLASS(SelectorItem)
    {
        // Allow Selector to access the protected IsSelected, ParentSelector,
        // and m_isPlaceholder fields.
        friend class Selector;
        friend class ListViewBase;

        private:
            // Weak reference to the Selector that contains this SelectorItem.
            ctl::WeakRefPtr m_wrParentSelector;

            // A value indicating whether the SelectorItem's content is 
            // currently a ui virtualized placeholder that will be replaced with an 
            // actual value once it has been set asynchronously.
            // This almost matches the data placeholder state (m_isPlaceholder) but not quite
            // since in this case the data that is set cannot be used as an indicator.
            bool m_isUIPlaceholder;

        protected:
            // A value indicating whether the SelectorItem's content is
            // currently a data virtualized placeholder that will be replaced
            // with an actual value once it has been retrieved.
            bool m_isPlaceholder;

        public:
            // Initializes a new instance of the SelectorItem class.
            SelectorItem() :
                m_isPlaceholder(FALSE)
                , m_isUIPlaceholder(FALSE)
            {
            }
            
            // Destroys an instance of the SelectorItem class.
            ~SelectorItem() override
            {
            }

            // Change to the correct visual state for the SelectorItem.
            _Check_return_ HRESULT ChangeVisualState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions) override;
                
            // Change to the correct visual state for the SelectorItem
            // using an existing ChangeVisualStateWithContext
            virtual _Check_return_ HRESULT ChangeVisualStateWithContext(
                _In_ VisualStateManagerBatchContext *pContext,
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions);

            // If this item is unfocused, sets focus on the SelectorItem.
            // Otherwise, sets focus to whichever element currently has focus
            // (so focusState can be propagated).
            virtual _Check_return_ HRESULT FocusSelfOrChild(
                _In_ xaml::FocusState focusState,
                _In_ BOOLEAN animateIfBringIntoView,
                _Out_ BOOLEAN* pFocused,
                _In_ xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None,
                InputActivationBehavior inputActivationBehavior = InputActivationBehavior::RequestActivation); // default to request activation to match legacy behavior
            
            // allows ItemContainerGenerator to know whether this container was considered to be a placeholder
            bool GetIsPlaceholder() const { return m_isPlaceholder; }

            // allows itemscontrol to indicate that the current data is not truly reflective of real data
            void SetIsUIPlaceholder(bool isPlaceholder) { m_isUIPlaceholder = isPlaceholder; }
            bool GetIsUIPlaceholder() const { return m_isUIPlaceholder; }

            // Get the parent Selector.
            _Check_return_ HRESULT GetParentSelector(
                _Outptr_ Selector** ppParentSelector);

            _Check_return_ HRESULT GetValue(
                _In_ const CDependencyProperty* pDP,
                _Outptr_ IInspectable **ppValue) override;

            _Check_return_ HRESULT get_IsSelectedImpl(
                _Out_ BOOLEAN* pValue);

            _Check_return_ HRESULT put_IsSelectedImpl(
                _In_ BOOLEAN value);

            _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

        protected:
            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            // Called when IsSelected property has changed
            virtual _Check_return_ HRESULT OnIsSelectedChanged(
                _In_ BOOLEAN isSelected);

            // Sets the parent Selector.
            virtual _Check_return_ HRESULT SetParentSelector(
                _In_opt_ Selector* pParentSelector);

            // Called when the Content property changes.
            IFACEMETHOD(OnContentChanged)(
                _In_ IInspectable* oldContent,
                _In_ IInspectable* newContent)
                override;
    };
}
