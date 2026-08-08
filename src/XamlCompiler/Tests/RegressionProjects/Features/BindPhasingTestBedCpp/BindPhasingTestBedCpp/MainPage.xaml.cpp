// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace BindPhasingTestBedCpp;

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Interop;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Navigation;
using namespace Microsoft::UI::Xaml::Shapes;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

ExtraInfo::ExtraInfo(String^ caption)
{
    Caption = caption;
}
MyInfo::MyInfo(String^ imageUrl, String^ caption)
{
    ImageUrl = imageUrl;
    Caption = caption;
}
MyItem::MyItem(String^ title, String^ subtitle, String^ description, MyInfo^ info, ExtraInfo^ otherInfo, String^ dp)
{
    Title = title;
    Subtitle = subtitle;
    Description = description;
    Info = info;
    OtherInfo = otherInfo;
    DPOnMyItem = dp;
}
void MyItem::NotifyPropertyChanged(String^ propertyName)
{
    //if (PropertyChanged != nullptr)
    {
        PropertyChangedEventArgs^ args =
            ref new PropertyChangedEventArgs(propertyName);
        PropertyChanged(this, args);
    }
}
DependencyProperty^ MyItem::_DPOnMyItemProperty = DependencyProperty::Register("DPOnMyItem", ::Windows::UI::Xaml::Interop::TypeName(String::typeid), ::Windows::UI::Xaml::Interop::TypeName(MyItem::typeid), nullptr);

MainPage::MainPage()
{
    myItems = ref new Platform::Collections::Vector<MyItem^>();
    CreateTestItems();
    InitializeComponent();
}

// Create a simulated list of 150,000 items.
void MainPage::CreateTestItems()
{
    for (int i = 1; i < 150000; i++)
    {
        MyItem^ myItem = ref new MyItem(
            "Title:" + i.ToString(), // Title.
            "Sub:" + i.ToString(), // Subtitle.
            "Desc:" + i.ToString(), // Description.
            ref new MyInfo(
            "ImageUrl" + i.ToString(), // ImageUrl of MyInfo
            "Caption" + i.ToString()), // Caption of MyInfo
            ref new ExtraInfo(
            "OtherCaption" + i.ToString()
            ),
            "DP" + i.ToString()
            );
        myItems->Append(myItem);
    }
}

// Connect the grid view to the list of items.
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
    myGridView->ItemsSource = myItems;
}

// Display each item incrementally to improve performance.
void MainPage::MyGridView_ContainerContentChanging(
    ListViewBase^ sender,
    ContainerContentChangingEventArgs^ args)
{
    args->Handled = true;

    if (args->Phase != 0)
    {
        throw ref new FailureException();
    }

    // First, show the items' placeholders.
    StackPanel^ templateRoot =
        dynamic_cast<StackPanel^>(args->ItemContainer->ContentTemplateRoot);
    Rectangle^ placeholderRectangle =
        dynamic_cast<Rectangle^>(templateRoot->FindName("placeholderRectangle"));
    TextBlock^ titleTextBlock =
        dynamic_cast<TextBlock^>(templateRoot->FindName("titleTextBlock"));
    TextBlock^ subtitleTextBlock =
        dynamic_cast<TextBlock^>(templateRoot->FindName("subtitleTextBlock"));
    TextBlock^ descriptionTextBlock =
        dynamic_cast<TextBlock^>(templateRoot->FindName("descriptionTextBlock"));

    // Make the placeholder rectangle opaque.
    placeholderRectangle->Opacity = 1;

    // Make everything else invisible.
    titleTextBlock->Opacity = 0;
    subtitleTextBlock->Opacity = 0;
    descriptionTextBlock->Opacity = 0;

    // Show the items' titles in the next phase.
    args->RegisterUpdateCallback(ref new TypedEventHandler<ListViewBase^, ContainerContentChangingEventArgs^>(this, &MainPage::ShowTitle));
}

// Show the items' titles.
void MainPage::ShowTitle(
    ListViewBase^ sender,
    ContainerContentChangingEventArgs^ args)
{
    if (args->Phase != 1)
    {
        throw ref new FailureException();
    }

    // Next, show the items' titles. Keep everything else invisible.
    MyItem^ myItem = dynamic_cast<MyItem^>(args->Item);
    SelectorItem^ itemContainer =
        dynamic_cast<SelectorItem^>(args->ItemContainer);
    StackPanel^ templateRoot =
        dynamic_cast<StackPanel^>(itemContainer->ContentTemplateRoot);
    TextBlock^ titleTextBlock =
        dynamic_cast<TextBlock^>(templateRoot->FindName("titleTextBlock"));

    titleTextBlock->Text = myItem->Title;
    titleTextBlock->Opacity = 1;

    // Show the items' subtitles in the next phase.
    args->RegisterUpdateCallback(ref new TypedEventHandler<ListViewBase^, ContainerContentChangingEventArgs^>(this, &MainPage::ShowSubtitle));

}

// Show the items' subtitles.
void MainPage::ShowSubtitle(
    ListViewBase^ sender,
    ContainerContentChangingEventArgs^ args)
{
    if (args->Phase != 2)
    {
        throw ref new FailureException();
    }

    // Next, show the items' subtitles. Keep everything else invisible.
    MyItem^ myItem = dynamic_cast<MyItem^>(args->Item);
    SelectorItem^ itemContainer = dynamic_cast<SelectorItem^>(args->ItemContainer);

    StackPanel^ templateRoot =
        dynamic_cast<StackPanel^>(itemContainer->ContentTemplateRoot);
    TextBlock^ subtitleTextBlock =
        dynamic_cast<TextBlock^>(templateRoot->FindName("subtitleTextBlock"));

    subtitleTextBlock->Text = myItem->Subtitle;
    subtitleTextBlock->Opacity = 1;

    // Show the items' descriptions in the next phase.
    args->RegisterUpdateCallback(ref new TypedEventHandler<ListViewBase^, ContainerContentChangingEventArgs^>(this, &MainPage::ShowDescription));
}

// Show the items' descriptions.
void MainPage::ShowDescription(
    ListViewBase^ sender,
    ContainerContentChangingEventArgs^ args)
{
    if (args->Phase != 3)
    {
        throw ref new FailureException();
    }

    // Finally, show the items' descriptions. 
    // Next, show the items' subtitles. Keep everything else invisible.
    MyItem^ myItem = dynamic_cast<MyItem^>(args->Item);
    SelectorItem^ itemContainer = dynamic_cast<SelectorItem^>(args->ItemContainer);

    StackPanel^ templateRoot =
        dynamic_cast<StackPanel^>(itemContainer->ContentTemplateRoot);

    Rectangle^ placeholderRectangle =
        dynamic_cast<Rectangle^>(templateRoot->FindName("placeholderRectangle"));
    TextBlock^ descriptionTextBlock =
        dynamic_cast<TextBlock^>(templateRoot->FindName("descriptionTextBlock"));

    descriptionTextBlock->Text = myItem->Description;
    descriptionTextBlock->Opacity = 1;

	// Make the placeholder rectangle invisible.
	placeholderRectangle->Opacity = 0;


	// Undefer
	templateRoot->FindName("deferedTextBlock");
	templateRoot->FindName("deferedAndPhasedTextBlock");
}
