// WARNING: Please don't edit this file...

void* winrt_make_ConditionalsCppWinRT_BindTests()
{
    return winrt::detach_abi(winrt::make<winrt::ConditionalsCppWinRT::factory_implementation::BindTests>());
}
WINRT_EXPORT namespace winrt::ConditionalsCppWinRT
{
    BindTests::BindTests() :
        BindTests(make<ConditionalsCppWinRT::implementation::BindTests>())
    {
    }
}
