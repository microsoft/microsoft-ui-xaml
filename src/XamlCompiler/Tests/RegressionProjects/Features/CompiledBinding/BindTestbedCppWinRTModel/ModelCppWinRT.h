// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ModelCppWinRT.g.h"

namespace winrt::BindTestbedCppWinRTModel::implementation
{
    struct ModelCppWinRT : ModelCppWinRTT<ModelCppWinRT>
    {
        ModelCppWinRT() = default;

        static winrt::BindTestbedCppWinRTModel::Coordinates MakeStarCppWinRT(hstring const& args);
        winrt::Windows::Foundation::Collections::IObservableVector<hstring> ObservableVectorOfStrings();
        winrt::BindTestbedCppWinRTModel::Coordinates Location();
        void InitializeValues();
        void UpdateValues();
    };
}
namespace winrt::BindTestbedCppWinRTModel::factory_implementation
{
    struct ModelCppWinRT : ModelCppWinRTT<ModelCppWinRT, implementation::ModelCppWinRT>
    {
    };
}
