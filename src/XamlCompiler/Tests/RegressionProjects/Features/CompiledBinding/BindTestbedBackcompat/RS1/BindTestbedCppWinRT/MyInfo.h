// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "MyInfo.g.h"

namespace winrt::BindTestbed::implementation
{
    struct MyInfo : MyInfoT<MyInfo>
    {
        MyInfo()
        {}

        MyInfo(int index, hstring const& imageUrl, hstring const& caption)
        {
            Caption(caption);
            ImageUrl(imageUrl);
            auto indexAsString = std::to_wstring(index);
            Prop1((std::wstring(L"Property1-") + indexAsString).c_str());
            Prop2((std::wstring(L"Property2-") + indexAsString).c_str());
            Prop3((std::wstring(L"Property3-") + indexAsString).c_str());
        }

        hstring Caption() { return caption; }
        void Caption(hstring value) { caption = value; }
        hstring ImageUrl() { return imageUrl; }
        void ImageUrl(hstring value) { imageUrl = value; }
        hstring Prop1() { return prop1; }
        void Prop1(hstring value) { prop1 = value; }
        hstring Prop2() { return prop2; }
        void Prop2(hstring value) { prop2 = value; }
        hstring Prop3() { return prop3; }
        void Prop3(hstring value) { prop3 = value; }

    private:
        hstring caption{};
        hstring imageUrl{};
        hstring prop1{};
        hstring prop2{};
        hstring prop3{};
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct MyInfo : MyInfoT<MyInfo, implementation::MyInfo>
    {
    };
}
