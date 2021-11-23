#include "pch.h"
#include "InfoBadgeImpl.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace InfoBadgeImpl;

int main()
{
    init_apartment();
    Uri uri(L"http://aka.ms/cppwinrt");
    printf("Hello, %ls!\n", uri.AbsoluteUri().c_str());

    printf("MeasureOverrideImpl(winrt::Windows::Foundation::Size{ 0,0 }) = { %f, %f }\n", MeasureOverrideImpl(winrt::Windows::Foundation::Size{ 0,0 }).Width, MeasureOverrideImpl(winrt::Windows::Foundation::Size{ 0,0 }).Height);
    printf("MeasureOverrideImpl(winrt::Windows::Foundation::Size{ 0,10 }) = { %f, %f }\n", MeasureOverrideImpl(winrt::Windows::Foundation::Size{ 0,10 }).Width, MeasureOverrideImpl(winrt::Windows::Foundation::Size{ 0,10 }).Height);
    printf("MeasureOverrideImpl(winrt::Windows::Foundation::Size{ 10,0 }) = { %f, %f }\n", MeasureOverrideImpl(winrt::Windows::Foundation::Size{ 10,0 }).Width, MeasureOverrideImpl(winrt::Windows::Foundation::Size{ 10,0 }).Height);
    printf("MeasureOverrideImpl(winrt::Windows::Foundation::Size{ 10,10 }) = { %f, %f }\n", MeasureOverrideImpl(winrt::Windows::Foundation::Size{ 10,10 }).Width, MeasureOverrideImpl(winrt::Windows::Foundation::Size{ 10,10 }).Height);

}
