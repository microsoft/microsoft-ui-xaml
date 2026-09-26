// WARNING: Please don't edit this file...

void* winrt_make_BindPhasingTestBedCppWinRT_MyItem()
{
    return winrt::detach_abi(winrt::make<winrt::BindPhasingTestBedCppWinRT::factory_implementation::MyItem>());
}
WINRT_EXPORT namespace winrt::BindPhasingTestBedCppWinRT
{
    MyItem::MyItem(param::hstring const& title, param::hstring const& subtitle, param::hstring const& description, winrt::BindPhasingTestBedCppWinRT::MyInfo const& info, winrt::BindPhasingTestBedCppWinRT::ExtraInfo const& otherInfo, param::hstring const& dp) :
        MyItem(make<BindPhasingTestBedCppWinRT::implementation::MyItem>(title, subtitle, description, info, otherInfo, dp))
    {
    }
    winrt::Microsoft::UI::Xaml::DependencyProperty MyItem::DPOnMyItemProperty()
    {
        return BindPhasingTestBedCppWinRT::implementation::MyItem::DPOnMyItemProperty();
    }
}
