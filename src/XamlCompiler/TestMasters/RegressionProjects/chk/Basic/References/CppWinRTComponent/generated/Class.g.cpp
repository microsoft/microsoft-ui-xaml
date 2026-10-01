// WARNING: Please don't edit this file...

void* winrt_make_CppWinRTComponent_Class()
{
    return winrt::detach_abi(winrt::make<winrt::CppWinRTComponent::factory_implementation::Class>());
}
WINRT_EXPORT namespace winrt::CppWinRTComponent
{
    Class::Class() :
        Class(make<CppWinRTComponent::implementation::Class>())
    {
    }
}
