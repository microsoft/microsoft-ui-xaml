// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <HWRedirectedComptreeNodeWinRT.h>

// HWWindowedPopupCompTreeNodeWinRT is a special type of redirected CompNode used
// specifically for Windowed Popups.
// Windowed Popups are special in that they create their own dedicated HWND, effectively
// splitting the DComp tree off into a separate branch. See Popup.h and Popup.cpp for more details.
class HWWindowedPopupCompTreeNodeWinRT final : public HWRedirectedCompTreeNodeWinRT
{
public:
    explicit HWWindowedPopupCompTreeNodeWinRT(
        _In_ CCoreServices *core,
        _In_ CompositorTreeHost *compositorTreeHost,
        _In_ DCompTreeHost* dcompTreeHost);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<HWWindowedPopupCompTreeNodeWinRT>::Index;
    }

    void UpdatePrimaryVisualTransformParent(_In_ DCompTreeHost *dcompTreeHost) override;

    bool GetRedirectionTransformInfo(_In_ RedirectionTransformInfo* rto) override;
};
