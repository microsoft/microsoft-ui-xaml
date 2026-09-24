// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "INotifyDataErrorInfoTests.h"
#include "INotifyDataErrorInfoTests.g.cpp"

namespace winrt::BindTestbed::implementation
{
    INotifyDataErrorInfoTests::INotifyDataErrorInfoTests()
    {
        InitializeComponent();
    }

    BindTestbedModel::DataErrorModel INotifyDataErrorInfoTests::ErrorModel()
    {
        return _errorModel;
    }

    BindTestbedModel::DODataErrorModel INotifyDataErrorInfoTests::DOErrorModel()
    {
        return _doDataErrorModel;
    }
}
