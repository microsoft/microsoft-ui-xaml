// WARNING: Please don't edit this file...

void* winrt_make_LinkedMDControlsCppWinRT_B()
{
    return winrt::detach_abi(winrt::make<winrt::LinkedMDControlsCppWinRT::factory_implementation::B>());
}
WINRT_EXPORT namespace winrt::LinkedMDControlsCppWinRT
{
    B::B() :
        B(make<LinkedMDControlsCppWinRT::implementation::B>())
    {
    }
}
