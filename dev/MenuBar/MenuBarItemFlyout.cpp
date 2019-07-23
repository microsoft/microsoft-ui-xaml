// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "MenuBarItemFlyout.h"
#include "common.h"

winrt::Control MenuBarItemFlyout::CreatePresenter()
{
    m_presenter.set(__super::CreatePresenter());
    return m_presenter.get();
}
