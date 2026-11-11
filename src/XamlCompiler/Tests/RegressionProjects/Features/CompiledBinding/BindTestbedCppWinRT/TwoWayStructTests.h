// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "TwoWayStructTests.g.h"

namespace winrt::BindTestbed::implementation
{
    struct TwoWayStructTests : TwoWayStructTestsT<TwoWayStructTests>
    {
        TwoWayStructTests();

        Windows::Foundation::Size MySize() { return m_size; }
        void MySize(Windows::Foundation::Size const& value) { m_size = value; ++m_sizeWrites; }
        Windows::Foundation::Rect MyRect() { return m_rect; }
        void MyRect(Windows::Foundation::Rect const& value) { m_rect = value; ++m_rectWrites; }
        BindTestbed::NestedGeometry Geometry() { return m_geometry; }
        void Geometry(BindTestbed::NestedGeometry const& value) { m_geometry = value; ++m_geometryWrites; }
        Windows::Foundation::Collections::IVector<Windows::Foundation::Size> Sizes() { return m_sizes; }
        Windows::Foundation::Collections::IMap<hstring, Windows::Foundation::Size> SizeMap() { return m_sizeMap; }

        void RunTests();

    private:
        Windows::Foundation::Size m_size{ 10, 20 };
        Windows::Foundation::Rect m_rect{ 1, 2, 3, 4 };
        BindTestbed::NestedGeometry m_geometry{ { 30, 40 }, 71 };
        Windows::Foundation::Collections::IVector<Windows::Foundation::Size> m_sizes{ nullptr };
        Windows::Foundation::Collections::IMap<hstring, Windows::Foundation::Size> m_sizeMap{ nullptr };
        int m_sizeWrites = 0;
        int m_rectWrites = 0;
        int m_geometryWrites = 0;
    };

    bool IsStructBindingTestMode();
    void RunStructBindingTests();
}

namespace winrt::BindTestbed::factory_implementation
{
    struct TwoWayStructTests : TwoWayStructTestsT<TwoWayStructTests, implementation::TwoWayStructTests>
    {
    };
}
