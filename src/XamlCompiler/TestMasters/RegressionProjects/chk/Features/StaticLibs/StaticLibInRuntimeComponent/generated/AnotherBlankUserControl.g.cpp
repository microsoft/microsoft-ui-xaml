// WARNING: Please don't edit this file...

void* winrt_make_StaticLibInRuntimeComponent_AnotherBlankUserControl()
{
    return winrt::detach_abi(winrt::make<winrt::StaticLibInRuntimeComponent::factory_implementation::AnotherBlankUserControl>());
}
WINRT_EXPORT namespace winrt::StaticLibInRuntimeComponent
{
    AnotherBlankUserControl::AnotherBlankUserControl() :
        AnotherBlankUserControl(make<StaticLibInRuntimeComponent::implementation::AnotherBlankUserControl>())
    {
    }
}
