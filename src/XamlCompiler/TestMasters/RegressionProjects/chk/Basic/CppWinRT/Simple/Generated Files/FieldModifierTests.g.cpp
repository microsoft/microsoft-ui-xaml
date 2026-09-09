// WARNING: Please don't edit this file...

void* winrt_make_Simple_FieldModifierTests()
{
    return winrt::detach_abi(winrt::make<winrt::Simple::factory_implementation::FieldModifierTests>());
}
WINRT_EXPORT namespace winrt::Simple
{
    FieldModifierTests::FieldModifierTests() :
        FieldModifierTests(make<Simple::implementation::FieldModifierTests>())
    {
    }
}
