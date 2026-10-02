// WARNING: Please don't edit this file...

void* winrt_make_StaticControlsLib_BlankUserControl()
{
    return winrt::detach_abi(winrt::make<winrt::StaticControlsLib::factory_implementation::BlankUserControl>());
}
WINRT_EXPORT namespace winrt::StaticControlsLib
{
    BlankUserControl::BlankUserControl() :
        BlankUserControl(make<StaticControlsLib::implementation::BlankUserControl>())
    {
    }
}
