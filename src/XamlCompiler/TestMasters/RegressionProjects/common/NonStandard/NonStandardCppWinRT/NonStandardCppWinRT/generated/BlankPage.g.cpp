// WARNING: Please don't edit this file...

void* winrt_make_NonStandardCppWinRT_BlankPage()
{
    return winrt::detach_abi(winrt::make<winrt::NonStandardCppWinRT::factory_implementation::BlankPage>());
}
WINRT_EXPORT namespace winrt::NonStandardCppWinRT
{
    BlankPage::BlankPage() :
        BlankPage(make<NonStandardCppWinRT::implementation::BlankPage>())
    {
    }
}
