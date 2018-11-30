// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RefreshInteractionRatioChangedEventArgs.g.h"

class RefreshInteractionRatioChangedEventArgs :
    public winrt::implementation::RefreshInteractionRatioChangedEventArgsT<RefreshInteractionRatioChangedEventArgs>
{
public:
    RefreshInteractionRatioChangedEventArgs(double value);

    double InteractionRatio();

private:
    double m_interactionRatio{ 0.0 };
};