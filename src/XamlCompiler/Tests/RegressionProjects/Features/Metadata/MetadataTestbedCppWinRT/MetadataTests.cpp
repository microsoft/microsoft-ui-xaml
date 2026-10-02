// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ArrayProperties.h"
#include "XamlMetaDataProvider.h"
#include <algorithm>
#include <cstdio>
#include <string_view>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Microsoft::UI::Xaml::Markup;

namespace
{
    void Verify(bool condition, hstring const& message)
    {
        if (!condition)
        {
            throw hresult_error(E_FAIL, message);
        }
    }

    template<typename T>
    void VerifyArray(IInspectable const& boxed, array_view<T const> expected, hstring const& name)
    {
        auto actual = unbox_value<com_array<T>>(boxed);
        Verify(std::equal(actual.begin(), actual.end(), expected.begin(), expected.end()), name);
    }

    template<typename Callback>
    void VerifyNoInterface(Callback&& callback)
    {
        try
        {
            callback();
        }
        catch (hresult_no_interface const&)
        {
            return;
        }
        throw hresult_error(E_FAIL, L"Expected E_NOINTERFACE for an invalid array value");
    }

    template<typename T>
    void RoundTrip(IXamlType const& type, IInspectable const& instance, hstring const& name,
        std::initializer_list<T> values, bool isAttachable = false)
    {
        auto member = type.GetMember(name);
        Verify(member != nullptr, name);
        Verify(!member.IsReadOnly(), name);
        Verify(member.IsAttachable() == isAttachable, name);

        com_array<T> expected(values);
        member.SetValue(instance, box_value(expected));
        VerifyArray<T>(member.GetValue(instance), expected, name);

        VerifyNoInterface([&] { member.SetValue(instance, nullptr); });
        VerifyNoInterface([&] { member.SetValue(instance, box_value(17)); });
        VerifyArray<T>(member.GetValue(instance), expected, name);

        member.SetValue(instance, box_value(com_array<T>{}));
        VerifyArray<T>(member.GetValue(instance), {}, name);
    }

    void RunArrayMetadataTests()
    {
        auto provider = make<MetadataTestbedCppWinRT::implementation::XamlMetaDataProvider>();
        auto type = provider.GetXamlType(L"MetadataTestbedCppWinRT.ArrayProperties");
        Verify(type != nullptr, L"ArrayProperties metadata is missing");
        auto instance = make<MetadataTestbedCppWinRT::implementation::ArrayProperties>();

        RoundTrip<int32_t>(type, instance, L"Int32Values", { -1, 0, 42 });
        RoundTrip<uint16_t>(type, instance, L"UInt16Values", { 0, 65535 });
        RoundTrip<hstring>(type, instance, L"StringValues", { L"", L"one", L"two" });
        RoundTrip<guid>(type, instance, L"GuidValues", { {}, guid{ "12345678-1234-abcd-0123-456789abcdef" } });
        RoundTrip<char16_t>(type, instance, L"CharValues", { u'\0', u'A', u'\u4e2d', u'\uffff' });
        RoundTrip<IInspectable>(type, instance, L"ObjectValues", { box_value(42), nullptr, box_value(L"text") });
        RoundTrip<Point>(type, instance, L"PointValues", { { 1, 2 }, { -3, 4 } });
        RoundTrip<int32_t>(type, instance, L"AttachedValues", { 7, 8 }, true);

        auto member = type.GetMember(L"Int32Values");
        VerifyNoInterface([&]
        {
            member.SetValue(instance, box_value(com_array<hstring>{ L"wrong element type" }));
        });

        instance.Int32Values({ 3, 5 });
        auto readOnlyMember = type.GetMember(L"ReadOnlyValues");
        Verify(readOnlyMember != nullptr && readOnlyMember.IsReadOnly(), L"ReadOnlyValues must be read-only");
        VerifyArray<int32_t>(readOnlyMember.GetValue(instance), { 3, 5 }, L"ReadOnlyValues");
    }
}

int __stdcall wXamlGeneratedMain();

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR commandLine, int)
{
    if (std::wstring_view(commandLine) != L"--test-array-metadata")
    {
        return wXamlGeneratedMain();
    }

    try
    {
        init_apartment(apartment_type::single_threaded);
        RunArrayMetadataTests();
        std::puts("Array metadata tests passed.");
        return 0;
    }
    catch (hresult_error const& error)
    {
        std::fwprintf(stderr, L"Array metadata test failed: %ls (0x%08X)\n",
            error.message().c_str(), static_cast<uint32_t>(error.code()));
        return static_cast<int>(error.code());
    }
}
