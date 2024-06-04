// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    interface
        DECLSPEC_NOVTABLE DECLSPEC_UUID("3794b2a0-636d-484d-af92-f87a423c989c")
        ITimedTextSourcePresenter : public IUnknown
    {
        IFACEMETHOD(ResetActiveCues)();
        IFACEMETHOD(Reset)();
        IFACEMETHOD(GetNaturalVideoSize)(_Out_ INT32* pHeight, _Out_ INT32* pWidth);
        IFACEMETHOD(get_IsFullWindow)(_Out_ BOOLEAN* isFullWindow);
        IFACEMETHOD(get_Stretch)(_Out_ xaml_media::Stretch* stretch);
        IFACEMETHOD(SetMTCOffset)(_In_ double offset);
    };
}
