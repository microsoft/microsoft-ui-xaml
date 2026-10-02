// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowPlacement.g.h"
#include "WindowPlacementRecord.h"
#include <windows.graphics.h>

namespace DirectUI
{
    PARTIAL_CLASS(WindowPlacement)
    {
    public:
        _Check_return_ HRESULT get_NormalRectImpl(_Out_ ABI::Windows::Graphics::RectInt32* value);
        _Check_return_ HRESULT put_NormalRectImpl(ABI::Windows::Graphics::RectInt32 value);
        _Check_return_ HRESULT get_WorkAreaImpl(_Out_ ABI::Windows::Graphics::RectInt32* value);
        _Check_return_ HRESULT put_WorkAreaImpl(ABI::Windows::Graphics::RectInt32 value);
        _Check_return_ HRESULT get_DpiImpl(_Out_ INT32* value);
        _Check_return_ HRESULT put_DpiImpl(INT32 value);
        _Check_return_ HRESULT get_StateImpl(_Out_ xaml::WindowPlacementState* value);
        _Check_return_ HRESULT put_StateImpl(xaml::WindowPlacementState value);
        _Check_return_ HRESULT get_SnapRectImpl(_Outptr_result_maybenull_ wf::IReference<ABI::Windows::Graphics::RectInt32>** value);
        _Check_return_ HRESULT put_SnapRectImpl(_In_opt_ wf::IReference<ABI::Windows::Graphics::RectInt32>* value);
        _Check_return_ HRESULT get_DisplayDeviceNameImpl(_Out_ HSTRING* value);
        _Check_return_ HRESULT put_DisplayDeviceNameImpl(_In_opt_ HSTRING value);
        _Check_return_ HRESULT get_VirtualDesktopIdImpl(_Outptr_result_maybenull_ wf::IReference<GUID>** value);
        _Check_return_ HRESULT put_VirtualDesktopIdImpl(_In_opt_ wf::IReference<GUID>* value);

        // Copies all fields under one lock; no window, monitor, storage or policy calls.
        _Check_return_ HRESULT CopySnapshot(WindowPlacementPersistence::Snapshot& value);
        static _Check_return_ HRESULT CreateFromSnapshot(
            const WindowPlacementPersistence::Snapshot& value,
            _Outptr_ xaml::IWindowPlacement** result);

    private:
        wil::srwlock m_lock;
        WindowPlacementPersistence::Snapshot m_snapshot;
    };
}
