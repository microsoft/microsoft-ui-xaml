// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FlyoutPresenter.g.h"
#include <WRLHelper.h>

namespace DirectUI
{
    class Flyout;

    PARTIAL_CLASS(FlyoutPresenter)
    {
    public:
        // Initializes a new instance.
        FlyoutPresenter();

        // Destroys an instance.
        ~FlyoutPresenter() override;

        HRESULT put_Flyout(_In_ xaml_primitives::IFlyoutBase * pFlyoutBase);

        _Check_return_ HRESULT GetOwnerName(_Out_ HSTRING* pName);

        static _Check_return_ HRESULT GetTargetIfOpenedAsTransientStatic(_In_ CDependencyObject * nativeControl, _Outptr_ CDependencyObject * *nativeTarget);
        _Check_return_ HRESULT GetTargetIfOpenedAsTransient(_Outptr_ DependencyObject** target);

    protected:
        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        IFACEMETHOD(OnApplyTemplate)() override;

        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

        TrackerPtr<xaml_controls::IScrollViewer> m_tpInnerScrollViewer;

    private:
        void CleanupTemplateParts();

        _Check_return_ HRESULT GetOwnerFlyout(_Outptr_ FlyoutBase** ppOwnerFlyout);

        ctl::WeakRefPtr m_wrFlyout;
    };
}
