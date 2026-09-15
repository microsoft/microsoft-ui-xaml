// WARNING: Please don't edit this file...

void* winrt_make_Simple_BlankPage()
{
    return winrt::detach_abi(winrt::make<winrt::Simple::factory_implementation::BlankPage>());
}
WINRT_EXPORT namespace winrt::Simple
{
    BlankPage::BlankPage() :
        BlankPage(make<Simple::implementation::BlankPage>())
    {
    }
}
