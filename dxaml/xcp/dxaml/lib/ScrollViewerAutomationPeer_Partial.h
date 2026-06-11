// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollViewerAutomationPeer.g.h"

namespace DirectUI
{
    // Represents the ScrollViewerAutomationPeer
    PARTIAL_CLASS(ScrollViewerAutomationPeer)
    {
        public:
            // Initializes a new instance of the ScrollViewerAutomationPeer class.
            ScrollViewerAutomationPeer();
            ~ScrollViewerAutomationPeer() override;

            IFACEMETHOD(GetPatternCore)(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue);
            IFACEMETHOD(GetClassNameCore)(_Out_ HSTRING* returnValue);
            IFACEMETHOD(GetAutomationControlTypeCore)(_Out_ xaml_automation_peers::AutomationControlType* returnValue);

            // IScrollProvider
            _Check_return_ HRESULT ScrollImpl(_In_ xaml_automation::ScrollAmount horizontalAmount, _In_ xaml_automation::ScrollAmount verticalAmount);
            _Check_return_ HRESULT SetScrollPercentImpl(_In_ DOUBLE horizontalPercent, _In_ DOUBLE verticalPercent);
            _Check_return_ HRESULT get_HorizontallyScrollableImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_HorizontalScrollPercentImpl(_Out_ DOUBLE* pValue);
            _Check_return_ HRESULT get_HorizontalViewSizeImpl(_Out_ DOUBLE* pValue);
            _Check_return_ HRESULT get_VerticallyScrollableImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT get_VerticalScrollPercentImpl(_Out_ DOUBLE* pValue);
            _Check_return_ HRESULT get_VerticalViewSizeImpl(_Out_ DOUBLE* pValue);

            _Check_return_ HRESULT RaiseAutomationEvents(DOUBLE extentX,
                                                        DOUBLE extentY,
                                                        DOUBLE viewportX,
                                                        DOUBLE viewportY,
                                                        DOUBLE minOffsetX,
                                                        DOUBLE minOffsetY,
                                                        DOUBLE offsetX,
                                                        DOUBLE offsetY);

            _Check_return_ BOOLEAN AutomationIsScrollable(_In_ DOUBLE extent, _In_ DOUBLE viewport, _In_ DOUBLE minOffset);
            _Check_return_ DOUBLE AutomationGetScrollPercent(_In_ DOUBLE extent, _In_ DOUBLE viewport, _In_ DOUBLE actualOffset, _In_ DOUBLE minOffset);
            _Check_return_ DOUBLE AutomationGetViewSize(_In_ DOUBLE extent, _In_ DOUBLE viewport, _In_ DOUBLE minOffset);

            _Check_return_ HRESULT ChildIsAcceptable(
                _In_ xaml::IUIElement* pElement,
                _Out_ BOOLEAN* bchildIsAcceptable) override;
        
        private:
            static const DOUBLE minimumPercent;
            static const DOUBLE maximumPercent;
            static const DOUBLE noScroll;
    };
}
