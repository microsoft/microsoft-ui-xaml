#pragma once

#include "GreetingModel.g.h"

namespace winrt::XamlCppWinRTModulesSample::Models::implementation
{
    struct GreetingModel : GreetingModelT<GreetingModel>
    {
        GreetingModel() = default;

        hstring Message() const
        {
            return L"x:Bind reached a model from a separate WinRT namespace.";
        }
    };
}

namespace winrt::XamlCppWinRTModulesSample::Models::factory_implementation
{
    struct GreetingModel : GreetingModelT<GreetingModel, implementation::GreetingModel>
    {
    };
}
