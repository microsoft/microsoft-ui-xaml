// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "FlyoutShowOptions_partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

FlyoutShowOptions::FlyoutShowOptions()
{
    m_position.Clear();
    m_exclusionRect.Clear();
    m_showMode = xaml_primitives::FlyoutShowMode_Auto;
    m_placement = xaml_primitives::FlyoutPlacementMode_Auto;
}

FlyoutShowOptions::~FlyoutShowOptions()
{
}