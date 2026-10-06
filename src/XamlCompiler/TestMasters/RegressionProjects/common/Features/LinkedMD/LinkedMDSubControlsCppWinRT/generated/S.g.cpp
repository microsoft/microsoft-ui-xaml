// WARNING: Please don't edit this file...

void* winrt_make_LinkedMDSubControlsCppWinRT_S()
{
    return winrt::detach_abi(winrt::make<winrt::LinkedMDSubControlsCppWinRT::factory_implementation::S>());
}
WINRT_EXPORT namespace winrt::LinkedMDSubControlsCppWinRT
{
    S::S() :
        S(make<LinkedMDSubControlsCppWinRT::implementation::S>())
    {
    }
}
