// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "INotifyDataErrorInfoTests.g.h"

namespace winrt::BindTestbed::implementation
{
    struct INotifyDataErrorInfoTests : INotifyDataErrorInfoTestsT<INotifyDataErrorInfoTests>
    {
        INotifyDataErrorInfoTests();

        BindTestbedModel::DataErrorModel ErrorModel();
        BindTestbedModel::DODataErrorModel DOErrorModel();

    private:
        BindTestbedModel::DataErrorModel _errorModel;
        BindTestbedModel::DODataErrorModel _doDataErrorModel;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct INotifyDataErrorInfoTests : INotifyDataErrorInfoTestsT<INotifyDataErrorInfoTests, implementation::INotifyDataErrorInfoTests>
    {
    };
}
