// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RevealListViewItemPresenter.g.h"

class RevealListViewItemPresenter :
    public ReferenceTracker<RevealListViewItemPresenter, winrt::implementation::RevealListViewItemPresenterT>
{
public:
    bool GoToElementStateCore(winrt::hstring const& stateName, bool useTransitions);
};

#include "RevealListViewItemPresenter.properties.cpp"
