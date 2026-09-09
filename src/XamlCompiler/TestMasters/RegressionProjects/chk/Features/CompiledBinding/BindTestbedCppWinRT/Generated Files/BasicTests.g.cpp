// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_BasicTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::BasicTests>());
}
WINRT_EXPORT namespace winrt::BindTestbed
{
    BasicTests::BasicTests() :
        BasicTests(make<BindTestbed::implementation::BasicTests>())
    {
    }
    winrt::Microsoft::UI::Xaml::DependencyProperty BasicTests::DPOnPageProperty()
    {
        return BindTestbed::implementation::BasicTests::DPOnPageProperty();
    }
    void BasicTests::DPOnPageProperty(winrt::Microsoft::UI::Xaml::DependencyProperty const& value)
    {
        BindTestbed::implementation::BasicTests::DPOnPageProperty(value);
    }
}
