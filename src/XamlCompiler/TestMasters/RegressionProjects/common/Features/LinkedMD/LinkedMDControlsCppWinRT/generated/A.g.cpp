// WARNING: Please don't edit this file...

void* winrt_make_LinkedMDControlsCppWinRT_A()
{
    return winrt::detach_abi(winrt::make<winrt::LinkedMDControlsCppWinRT::factory_implementation::A>());
}
WINRT_EXPORT namespace winrt::LinkedMDControlsCppWinRT
{
    A::A() :
        A(make<LinkedMDControlsCppWinRT::implementation::A>())
    {
    }
}
