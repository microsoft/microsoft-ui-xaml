// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct IXcpInputPaneHandler
{
    virtual _Check_return_ HRESULT Showing(_In_ XRECTF *occludedRectangle, _In_ BOOL ensureFocusedElementInView) = 0;
    virtual _Check_return_ HRESULT Hiding(_In_ BOOL ensureFocusedElementInView) = 0;
    virtual _Check_return_ HRESULT NotifyEditFocusRemoval() = 0;
    virtual _Check_return_ HRESULT NotifyEditControlInputPaneHiding() = 0;
    virtual XRECTF GetInputPaneExposureRect() = 0;
};


