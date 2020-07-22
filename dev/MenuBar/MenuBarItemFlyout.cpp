// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MenuBarItemFlyout.h"

#include "MenuBarItemFlyout.properties.cpp"

winrt::Control MenuBarItemFlyout::CreatePresenter()
{
    m_presenter.set(__super::CreatePresenter());
    return m_presenter.get();
}
