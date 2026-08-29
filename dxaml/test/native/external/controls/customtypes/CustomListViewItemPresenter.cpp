// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <pch.h>
#include <CustomListViewItemPresenter.h>

namespace Tests { namespace External { namespace Controls { namespace CustomTypes {
    bool CustomListViewItemPresenter::m_PlaceHolderStateReached = false;

    void CustomListViewItemPresenter::CustomListViewPresenter()
    {}

    bool CustomListViewItemPresenter::Get_PlaceholderStateReached()
    {
        return m_PlaceHolderStateReached;
    }

    void CustomListViewItemPresenter::Set_PlaceholderStateReached(bool value)
    {
        m_PlaceHolderStateReached = value;
    }

    bool CustomListViewItemPresenter::GoToElementStateCore(Platform::String^ stateName, bool useTransitions)
    {
        if (stateName == "DataPlaceholder")
        {
            m_PlaceHolderStateReached = true;
        }

        return __super::GoToElementStateCore(stateName, useTransitions);
    }
};
}}}