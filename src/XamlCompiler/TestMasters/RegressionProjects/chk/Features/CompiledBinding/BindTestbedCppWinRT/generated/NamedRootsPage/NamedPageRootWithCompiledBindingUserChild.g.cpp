// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_NamedRootsPage_NamedPageRootWithCompiledBindingUserChild()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::NamedRootsPage::factory_implementation::NamedPageRootWithCompiledBindingUserChild>());
}
WINRT_EXPORT namespace winrt::BindTestbed::NamedRootsPage
{
    NamedPageRootWithCompiledBindingUserChild::NamedPageRootWithCompiledBindingUserChild() :
        NamedPageRootWithCompiledBindingUserChild(make<BindTestbed::NamedRootsPage::implementation::NamedPageRootWithCompiledBindingUserChild>())
    {
    }
}
