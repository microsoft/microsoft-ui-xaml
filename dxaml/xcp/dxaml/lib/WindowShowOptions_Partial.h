// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowShowOptions.g.h"
#include "WindowPlacementPublic.h"

namespace DirectUI
{
    PARTIAL_CLASS(WindowShowOptions)
    {
    public:
        _Check_return_ HRESULT get_PlacementImpl(_Outptr_result_maybenull_ xaml::IWindowPlacement** value);
        _Check_return_ HRESULT put_PlacementImpl(_In_opt_ xaml::IWindowPlacement* value);
        _Check_return_ HRESULT get_ReasonImpl(_Out_ xaml::WindowShowReason* value);
        _Check_return_ HRESULT put_ReasonImpl(xaml::WindowShowReason value);
        _Check_return_ HRESULT get_CascadeBehaviorImpl(_Out_ xaml::WindowCascadeBehavior* value);
        _Check_return_ HRESULT put_CascadeBehaviorImpl(xaml::WindowCascadeBehavior value);
        _Check_return_ HRESULT get_DoNotActivateImpl(_Out_ BOOLEAN* value);
        _Check_return_ HRESULT put_DoNotActivateImpl(BOOLEAN value);
        _Check_return_ HRESULT get_SkipInitialPlacementImpl(_Out_ BOOLEAN* value);
        _Check_return_ HRESULT put_SkipInitialPlacementImpl(BOOLEAN value);

        // Only for eligible initial requests. Later Show reads DoNotActivate alone.
        _Check_return_ HRESULT CopyInitialRequest(
            bool isHiddenApplication,
            WindowPlacementPersistence::InitialRequest& result);

    private:
        wil::srwlock m_lock;
        ctl::ComPtr<xaml::IWindowPlacement> m_placement;
        xaml::WindowShowReason m_reason{};
        xaml::WindowCascadeBehavior m_cascadeBehavior{};
        BOOLEAN m_doNotActivate{};
        BOOLEAN m_skipInitialPlacement{};
    };
}
