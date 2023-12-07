// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "HyperlinkButton.g.h"

namespace DirectUI
{
    // Represents the HyperlinkButton control
    PARTIAL_CLASS(HyperlinkButton)
    {
        public:
            // Initializes a new instance of the HyperlinkButton class.
            HyperlinkButton();
            ~HyperlinkButton() override;

        protected:
            _Check_return_ HRESULT Initialize() override;
            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            // Change to the correct visual state
            _Check_return_ HRESULT ChangeVisualState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions) override;

            // Raises the Click event.
            _Check_return_ HRESULT OnClick() override;

            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
            IFACEMETHOD(MeasureOverride)(
                _In_ wf::Size availableSize,
                _Out_ wf::Size* returnValue)
                override;

        private:
            _Check_return_ HRESULT UpdateContentPresenterTextUnderline();
            _Check_return_ HRESULT SetHyperlinkForegroundOverrideForBackPlate();
            void SetHyperlinkForegroundOverrideForBackPlateRecursive(_In_ CUIElement* pElement);

            static const WCHAR c_ContentPresenterName[];
            static const WCHAR c_ContentPresenterLegacyName[];
    };
}
