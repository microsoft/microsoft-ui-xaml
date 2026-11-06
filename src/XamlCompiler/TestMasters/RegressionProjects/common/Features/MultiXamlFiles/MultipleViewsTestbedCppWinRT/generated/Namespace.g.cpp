// WARNING: Please don't edit this file...

void* winrt_make_MultipleViewsTestbedCppWinRT_Namespace()
{
    return winrt::detach_abi(winrt::make<winrt::MultipleViewsTestbedCppWinRT::factory_implementation::Namespace>());
}
WINRT_EXPORT namespace winrt::MultipleViewsTestbedCppWinRT
{
    Namespace::Namespace() :
        Namespace(make<MultipleViewsTestbedCppWinRT::implementation::Namespace>())
    {
    }
}
