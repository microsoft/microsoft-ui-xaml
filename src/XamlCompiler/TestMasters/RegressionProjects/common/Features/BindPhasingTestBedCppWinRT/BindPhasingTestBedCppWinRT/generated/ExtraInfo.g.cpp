// WARNING: Please don't edit this file...

void* winrt_make_BindPhasingTestBedCppWinRT_ExtraInfo()
{
    return winrt::detach_abi(winrt::make<winrt::BindPhasingTestBedCppWinRT::factory_implementation::ExtraInfo>());
}
WINRT_EXPORT namespace winrt::BindPhasingTestBedCppWinRT
{
    ExtraInfo::ExtraInfo(param::hstring const& caption) :
        ExtraInfo(make<BindPhasingTestBedCppWinRT::implementation::ExtraInfo>(caption))
    {
    }
}
