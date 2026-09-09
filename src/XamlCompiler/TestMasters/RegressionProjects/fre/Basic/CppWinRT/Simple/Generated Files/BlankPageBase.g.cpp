// WARNING: Please don't edit this file...

void* winrt_make_Simple_BlankPageBase()
{
    return winrt::detach_abi(winrt::make<winrt::Simple::factory_implementation::BlankPageBase>());
}
WINRT_EXPORT namespace winrt::Simple
{
    BlankPageBase::BlankPageBase() :
        BlankPageBase(make<Simple::implementation::BlankPageBase>())
    {
    }
}
