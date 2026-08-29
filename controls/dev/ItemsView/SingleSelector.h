// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SelectorBase.h"

class SingleSelector : public SelectorBase
{
public:
    SingleSelector();
    ~SingleSelector();

    void FollowFocus(bool followFocus);

    void OnInteractedAction(winrt::IndexPath const& index, bool ctrl, bool shift) override;
    void OnFocusedAction(winrt::IndexPath const& index, bool ctrl, bool shift) override;

private:
    bool m_followFocus{ true };
};
