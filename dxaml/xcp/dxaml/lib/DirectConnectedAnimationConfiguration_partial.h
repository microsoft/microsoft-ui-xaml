// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#pragma once
#include "DirectConnectedAnimationConfiguration.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DirectConnectedAnimationConfiguration)
    {
    public:
        DirectConnectedAnimationConfiguration() {};

        _Check_return_ HRESULT GetDefaultEasingFunction(_Outptr_ WUComp::ICompositionEasingFunction** value) override;
        const wf::TimeSpan GetDefaultDuration() override;
    private:
    };
 }
