// WARNING: Please don't edit this file...

void* winrt_make_BindTestbed_BasicTests()
{
    return winrt::detach_abi(winrt::make<winrt::BindTestbed::factory_implementation::BasicTests>());
}
namespace winrt::BindTestbed
{
    BasicTests::BasicTests() :
        BasicTests(make<BindTestbed::implementation::BasicTests>())
    {
    }
    ::Windows::UI::Xaml::DependencyProperty BasicTests::DPOnPageProperty()
    {
        return BindTestbed::implementation::BasicTests::DPOnPageProperty();
    }
    void BasicTests::DPOnPageProperty(::Windows::UI::Xaml::DependencyProperty const& value)
    {
        return BindTestbed::implementation::BasicTests::DPOnPageProperty(value);
    }
}
