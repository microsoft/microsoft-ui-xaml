// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "FocusAdapter.h"
#include "ContentRoot.h"

using namespace ContentRootAdapters;

FocusAdapter::FocusAdapter(_In_ CContentRoot& contentRoot)
    : m_contentRoot(contentRoot)
{
}

bool FocusAdapter::ShouldDepartFocus(_In_ DirectUI::FocusNavigationDirection direction) const
{
    return false;
}