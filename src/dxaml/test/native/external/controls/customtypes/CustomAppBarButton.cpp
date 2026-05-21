// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <pch.h>
#include <CustomAppBarButton.h>

namespace Tests { namespace External { namespace Controls { namespace CustomTypes {

    CustomAppBarButton::CustomAppBarButton()
    {
        m_isCompact = false;
        m_dynamicOverflowOrder = 0;
        m_IsInOverflow = false;
    }

}}}}