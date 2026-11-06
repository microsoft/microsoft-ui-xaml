// WARNING: Please don't edit this file...

void* winrt_make_Simple_MainPageBase()
{
    return winrt::detach_abi(winrt::make<winrt::Simple::factory_implementation::MainPageBase>());
}
WINRT_EXPORT namespace winrt::Simple
{
    MainPageBase::MainPageBase() :
        MainPageBase(make<Simple::implementation::MainPageBase>())
    {
    }
}
