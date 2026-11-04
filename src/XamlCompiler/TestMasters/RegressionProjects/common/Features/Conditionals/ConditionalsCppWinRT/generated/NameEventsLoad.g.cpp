// WARNING: Please don't edit this file...

void* winrt_make_ConditionalsCppWinRT_NameEventsLoad()
{
    return winrt::detach_abi(winrt::make<winrt::ConditionalsCppWinRT::factory_implementation::NameEventsLoad>());
}
WINRT_EXPORT namespace winrt::ConditionalsCppWinRT
{
    NameEventsLoad::NameEventsLoad() :
        NameEventsLoad(make<ConditionalsCppWinRT::implementation::NameEventsLoad>())
    {
    }
}
