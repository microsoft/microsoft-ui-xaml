// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <FrameworkView.g.h>
#include <MetadataResetter.h>

namespace DirectUI
{
    PARTIAL_CLASS(FrameworkView)
    {
    public:
        IFACEMETHOD(Initialize)(_In_ wac::ICoreApplicationView* applicationView) override;
        IFACEMETHOD(SetWindow)(_In_ wuc::ICoreWindow* window) override;
        IFACEMETHOD(Load)(_In_ HSTRING contentId) override;
        IFACEMETHOD(Run)() override;
        IFACEMETHOD(Uninitialize)() override;

    protected:
        FrameworkView();
        ~FrameworkView() override;

        // un-hide ComBase::Initialize (used from ctl::ComObject<ViewProvider>::CreateInstance)
        using ComBase::Initialize;

    private:
        wac::ICoreApplicationView* m_pCoreApplicationView;
        
        _Check_return_ HRESULT HookCoreActivationEvents(bool fRegister);

        _Check_return_ HRESULT OnActivated(
            _In_ wac::ICoreApplicationView* pSender,                            
            _In_ waa::IActivatedEventArgs* pArgs);                                

        EventRegistrationToken m_ActivatedEventToken;

        std::shared_ptr<MetadataResetter> m_metadataRef;
    };
}
