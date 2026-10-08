// WARNING: Please don't edit this file...

void* winrt_make_MetadataTestbedCppWinRT_ArrayProperties()
{
    return winrt::detach_abi(winrt::make<winrt::MetadataTestbedCppWinRT::factory_implementation::ArrayProperties>());
}
WINRT_EXPORT namespace winrt::MetadataTestbedCppWinRT
{
    ArrayProperties::ArrayProperties() :
        ArrayProperties(make<MetadataTestbedCppWinRT::implementation::ArrayProperties>())
    {
    }
    com_array<int32_t> ArrayProperties::GetAttachedValues(winrt::Windows::Foundation::IInspectable const& target)
    {
        return MetadataTestbedCppWinRT::implementation::ArrayProperties::GetAttachedValues(target);
    }
    void ArrayProperties::SetAttachedValues(winrt::Windows::Foundation::IInspectable const& target, array_view<int32_t const> value)
    {
        MetadataTestbedCppWinRT::implementation::ArrayProperties::SetAttachedValues(target, value);
    }
    char16_t ArrayProperties::GetAttachedCharValue(winrt::Windows::Foundation::IInspectable const& target)
    {
        return MetadataTestbedCppWinRT::implementation::ArrayProperties::GetAttachedCharValue(target);
    }
    void ArrayProperties::SetAttachedCharValue(winrt::Windows::Foundation::IInspectable const& target, char16_t value)
    {
        MetadataTestbedCppWinRT::implementation::ArrayProperties::SetAttachedCharValue(target, value);
    }
}
