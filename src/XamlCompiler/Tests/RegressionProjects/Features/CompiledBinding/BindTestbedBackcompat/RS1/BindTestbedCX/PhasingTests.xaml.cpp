// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// PhasingTests.xaml.cpp
// Implementation of the PhasingTests class.
//

#include "pch.h"
#include "PhasingTests.xaml.h"
#include "MainPage.xaml.h"
#include "DetectLeaksPage.xaml.h"

using namespace BindTestbed;

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
using namespace ::Windows::UI::Popups;

ExtraInfo::ExtraInfo(String^ caption)
{
	Caption = caption;
}

MyInfo::MyInfo(int index, String^ imageUrl, String^ caption)
{
	ImageUrl = imageUrl;
	Caption = caption;
	Prop1 = "Property1-" + index.ToString();
	Prop2 = "Property2-" + index.ToString();
	Prop3 = "Property3-" + index.ToString();
}

MyItem::MyItem(int index, String^ title, String^ subtitle, String^ description, MyInfo^ info, ExtraInfo^ otherInfo, String^ dp)
{
	Index = index;
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

DependencyProperty^ MyItem::_DPOnMyItemProperty = DependencyProperty::Register("DPOnMyItem", ::Windows::UI::Xaml::Interop::TypeName(::Platform::String::typeid), ::Windows::UI::Xaml::Interop::TypeName(MyItem::typeid), nullptr);

PhasingTests::PhasingTests()
{
	myItems = ref new Platform::Collections::Vector<MyItem^>();
	InitializeComponent();
	InitializeValues();
	myGridView->ItemsSource = myItems;
	Initialized = true;
	DetectLeaksPage::TrackObject(this, PhasingTests::GetType()->FullName);
}

void PhasingTests::InitializeValues()
{
	for (int i = 1; i < itemsCount; i++)
	{
		MyItem^ myItem = ref new MyItem(i,
			"Title:" + i.ToString(), // Title.
			"Sub:" + i.ToString(), // Subtitle.
			"Desc:" + i.ToString(), // Description.
			ref new MyInfo(i,
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

void PhasingTests::Reset_Click(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	myItems->Clear();
	InitializeValues();
}

void PhasingTests::Reload_Click(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	myItems = ref new Platform::Collections::Vector<MyItem^>();
	InitializeValues();
	myGridView->ItemsSource = myItems;
}


// Display each item incrementally to improve performance.
void PhasingTests::MyGridView_ContainerContentChanging(Microsoft::UI::Xaml::Controls::ListViewBase^ sender, Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args)
{
	wait(1);
	args->Handled = true;
	if (args->Phase < 20)
	{
		args->RegisterUpdateCallback(ref new TypedEventHandler<ListViewBase^, ContainerContentChangingEventArgs^>(this, &PhasingTests::MyGridView_ContainerContentChanging));
	}
}

void PhasingTests::wait(int)
{
}

void PhasingTests::StackPanel_PointerReleased(Object^ sender, ::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	StackPanel^ root = safe_cast<StackPanel^>(sender);
	root->Background = ref new SolidColorBrush(::Windows::UI::Colors::White);
	root->FindName("deferedTextBlock");
	root->FindName("deferedAndPhasedTextBlock");
}

void PhasingTests::SlowPhasing_UnChecked(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	SlowPhasing_Checked(sender, e);
}

void PhasingTests::SlowPhasing_Checked(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	if (Initialized)
	{
		if (SlowPhasing->IsChecked->Value)
		{
			INPC_token = myGridView->ContainerContentChanging += ref new TypedEventHandler<ListViewBase^, ContainerContentChangingEventArgs^>(this, &PhasingTests::MyGridView_ContainerContentChanging);
		}
		else
		{
			myGridView->ContainerContentChanging -= INPC_token;
		}
	}
}

void PhasingTests::PhasedTemplate_UnChecked(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	PhasedTemplate_Checked(sender, e);
}

void PhasingTests::PhasedTemplate_Checked(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	if (Initialized)
	{
		if (PhasedTemplateCbx->IsChecked->Value)
		{
			myGridView->ItemTemplate = dynamic_cast<DataTemplate^>(this->Resources->Lookup("PhasedTemplate"));
		}
		else
		{
			myGridView->ItemTemplate = dynamic_cast<DataTemplate^>(this->Resources->Lookup("NonPhasedTemplate"));
		}
	}
}