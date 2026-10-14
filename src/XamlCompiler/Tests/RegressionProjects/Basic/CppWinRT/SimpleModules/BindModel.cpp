// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "BindModel.h"
#include "Models/BindModel.g.cpp"

namespace winrt::Simple::Models::implementation
{
    BindModel::BindModel()
        : m_currentItem(Simple::Models::BindItem())
    {
    }
}
