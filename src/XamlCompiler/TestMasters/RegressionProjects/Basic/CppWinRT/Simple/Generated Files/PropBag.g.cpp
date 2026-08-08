// WARNING: Please don't edit this file...

void* winrt_make_Simple_PropBag()
{
    return winrt::detach_abi(winrt::make<winrt::Simple::factory_implementation::PropBag>());
}
namespace winrt::Simple
{
    PropBag::PropBag() :
        PropBag(make<Simple::implementation::PropBag>())
    {
    }
}
