// WARNING: Please don't edit this file...

void* winrt_make_LinkedMDSubControlsCppWinRT_T()
{
    return winrt::detach_abi(winrt::make<winrt::LinkedMDSubControlsCppWinRT::factory_implementation::T>());
}
WINRT_EXPORT namespace winrt::LinkedMDSubControlsCppWinRT
{
    T::T() :
        T(make<LinkedMDSubControlsCppWinRT::implementation::T>())
    {
    }
}
