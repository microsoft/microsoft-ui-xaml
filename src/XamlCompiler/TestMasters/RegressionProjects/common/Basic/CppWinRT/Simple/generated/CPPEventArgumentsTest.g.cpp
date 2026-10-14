// WARNING: Please don't edit this file...

void* winrt_make_Simple_CPPEventArgumentsTest()
{
    return winrt::detach_abi(winrt::make<winrt::Simple::factory_implementation::CPPEventArgumentsTest>());
}
WINRT_EXPORT namespace winrt::Simple
{
    CPPEventArgumentsTest::CPPEventArgumentsTest() :
        CPPEventArgumentsTest(make<Simple::implementation::CPPEventArgumentsTest>())
    {
    }
}
