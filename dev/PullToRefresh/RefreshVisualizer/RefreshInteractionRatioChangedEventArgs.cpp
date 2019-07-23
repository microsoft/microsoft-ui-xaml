// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PTRTracing.h"
#include "RefreshInteractionRatioChangedEventArgs.h"
#include "common.h"

RefreshInteractionRatioChangedEventArgs::RefreshInteractionRatioChangedEventArgs(double value)
{
    PTR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    m_interactionRatio = value;
}

double RefreshInteractionRatioChangedEventArgs::InteractionRatio()
{
    return m_interactionRatio;
}