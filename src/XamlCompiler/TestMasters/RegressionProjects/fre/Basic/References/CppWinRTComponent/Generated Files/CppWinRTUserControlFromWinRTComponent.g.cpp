// WARNING: Please don't edit this file...

void* winrt_make_CppWinRTComponent_CppWinRTUserControlFromWinRTComponent()
{
    return winrt::detach_abi(winrt::make<winrt::CppWinRTComponent::factory_implementation::CppWinRTUserControlFromWinRTComponent>());
}
WINRT_EXPORT namespace winrt::CppWinRTComponent
{
    CppWinRTUserControlFromWinRTComponent::CppWinRTUserControlFromWinRTComponent() :
        CppWinRTUserControlFromWinRTComponent(make<CppWinRTComponent::implementation::CppWinRTUserControlFromWinRTComponent>())
    {
    }
}
