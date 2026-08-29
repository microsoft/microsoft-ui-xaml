// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "StickyHeadersHelper.h"
namespace AppAnalysisTestHelpers {

    enum class LoadComponentOptions : bool {
        PlaceInTree = true,
        DoNotPlaceInTree = false,
    };

    template<typename T>
    T^ LoadXaml(Platform::String^ location, LoadComponentOptions options = LoadComponentOptions::PlaceInTree)
    {
        T^ rootElement = nullptr;
        RunOnUIThread([&]()
        {
            rootElement = ref new T();
            Application::LoadComponent(
                rootElement,
                ref new ::Windows::Foundation::Uri(location),
                Primitives::ComponentResourceLocation::Application);

            WEX::Common::Throw::If(rootElement == nullptr, E_POINTER);

            if (options == LoadComponentOptions::PlaceInTree)
            {
                TestServices::WindowHelper->WindowContent = rootElement;
            }

        });

        if (options == LoadComponentOptions::PlaceInTree)
        {
            TestServices::WindowHelper->WaitForIdle();
        }

        return rootElement;
    }

    static Platform::Collections::Vector<Platform::Object^>^ GetGroupedData()
    {
        auto groupedData = ref new Platform::Collections::Vector<Platform::Object^>();

        for (unsigned int i = 0; i < 2; ++i)
        {
            Microsoft::UI::Xaml::Tests::Common::GroupedHeader^ group = nullptr;

            switch (i)
            {
            case 0:
                group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"States");
                VERIFY_IS_NOT_NULL(group);

                group->Append(L"Washington");
                group->Append(L"Oregon");
                group->Append(L"California");
                break;

            case 1:
                group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"Cities");
                VERIFY_IS_NOT_NULL(group);

                group->Append(L"Seattle");
                group->Append(L"Portland");
                group->Append(L"Los Angeles");
                break;

            }
            groupedData->Append(group);
        }

        return groupedData;
    }
}

namespace aa_help = AppAnalysisTestHelpers;