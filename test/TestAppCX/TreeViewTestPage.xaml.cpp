#include "pch.h"
#include "TreeViewTestPage.xaml.h"

using namespace TestAppCX;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

TreeViewTestPage::TreeViewTestPage()
{
    InitializeComponent();

    TreeViewData^ d1 = ref new TreeViewData("111");
    TreeViewData^ d2 = ref new TreeViewData("222");
    TreeViewData^ d3 = ref new TreeViewData("333");
    Items->Append(d1);
    Items->Append(d2);
    Items->Append(d3);
}


void TestAppCX::TreeViewTestPage::ReplaceAll_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto newItems = ref new Platform::Array<TreeViewData^>(2);
    newItems[0] = ref new TreeViewData("444");
    newItems[1] = ref new TreeViewData("555");
    Items->ReplaceAll(newItems);
}
