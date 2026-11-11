// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "App.h"
#include "TwoWayStructTests.h"
#include "TwoWayStructTests.g.cpp"
#include <cstdio>
#include <map>
#include <string_view>

namespace
{
    bool structBindingTestMode = false;
    int structBindingTestResult = E_FAIL;

    void Verify(bool condition, winrt::hstring const& message)
    {
        if (!condition)
        {
            throw winrt::hresult_error(E_FAIL, message);
        }
    }
}

namespace winrt::BindTestbed::implementation
{
    TwoWayStructTests::TwoWayStructTests()
    {
        m_sizes = single_threaded_vector<wf::Size>({ { 50, 60 }, { 70, 80 } });
        m_sizeMap = single_threaded_map<hstring, wf::Size>(
            std::map<hstring, wf::Size>{ { L"first", { 90, 100 } }, { L"other", { 110, 120 } } });
        InitializeComponent();
    }

    void TwoWayStructTests::RunTests()
    {
        // Initialize the generated bindings without requiring the fixture to be displayed.
        Bindings->Update();
        Verify(SizeWidth().Value() == 10 && RectX().Value() == 1 &&
            NestedWidth().Value() == 30 && VectorWidth().Value() == 50 && MapHeight().Value() == 100,
            L"Initial source values must reach the NumberBoxes");
        Verify(m_sizeWrites == 0 && m_rectWrites == 0 && m_geometryWrites == 0,
            L"Initializing targets must not write back to the source");

        SizeWidth().Value(-42.25);
        Verify(m_size.Width == -42.25f && m_size.Height == 20 && m_sizeWrites == 1,
            L"Size.Width must persist once without changing Size.Height");
        RectX().Value(12.5);
        Verify(m_rect.X == 12.5f && m_rect.Y == 2 && m_rect.Width == 3 &&
            m_rect.Height == 4 && m_rectWrites == 1,
            L"Rect.X must persist once without changing other Rect fields");
        NestedWidth().Value(-7.5);
        Verify(m_geometry.Size.Width == -7.5f && m_geometry.Size.Height == 40 &&
            m_geometry.Unchanged == 71 && m_geometryWrites == 1,
            L"Nested fields must be written back through the outer struct property once");
        VectorWidth().Value(25.5);
        Verify(m_sizes.GetAt(0).Width == 25.5f && m_sizes.GetAt(0).Height == 60 &&
            m_sizes.GetAt(1).Width == 70 && m_sizes.GetAt(1).Height == 80,
            L"A vector field update must preserve other fields and elements");
        MapHeight().Value(-3.5);
        Verify(m_sizeMap.Lookup(L"first").Width == 90 && m_sizeMap.Lookup(L"first").Height == -3.5f &&
            m_sizeMap.Lookup(L"other").Width == 110 && m_sizeMap.Lookup(L"other").Height == 120,
            L"A map field update must preserve other fields and entries");
    }

    bool IsStructBindingTestMode()
    {
        return structBindingTestMode;
    }

    void RunStructBindingTests()
    {
        try
        {
            auto fixture = make_self<TwoWayStructTests>();
            fixture->RunTests();
            std::puts("Two-way struct binding tests passed.");
            structBindingTestResult = 0;
        }
        catch (hresult_error const& error)
        {
            std::fwprintf(stderr, L"Two-way struct binding test failed: %ls (0x%08X)\n",
                error.message().c_str(), static_cast<uint32_t>(error.code()));
            structBindingTestResult = static_cast<int>(error.code());
        }
        wux::Application::Current().Exit();
    }
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR commandLine, int)
{
    structBindingTestMode = std::wstring_view(commandLine) == L"--test-struct-bindings";

    // Preserve the original entry point's unconditional App construction for ordinary launches
    // as well as tests, without the generated helper's optional-constructor detection.
    winrt::init_apartment(winrt::apartment_type::single_threaded);
    winrt::Microsoft::UI::Xaml::Application::Start([](auto&&)
    {
        winrt::make<winrt::BindTestbed::implementation::App>();
    });
    return structBindingTestMode ? structBindingTestResult : 0;
}
