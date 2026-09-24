//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

using namespace MultipleViewsTestbedCPP;

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace ::Windows::UI::Core;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();
	Window::Current->SizeChanged += ref new ::Windows::Foundation::TypedEventHandler<Platform::Object^, Microsoft::UI::Xaml::WindowSizeChangedEventArgs^>(this, &MultipleViewsTestbedCPP::MainPage::Current_SizeChanged);
	this->LoadCorrectXamlFile();
}

void MultipleViewsTestbedCPP::MainPage::Current_SizeChanged(Platform::Object^ sender, Microsoft::UI::Xaml::WindowSizeChangedEventArgs^ e)
{
	this->LoadCorrectXamlFile();
}

void MultipleViewsTestbedCPP::MainPage::LoadCorrectXamlFile()
{
	// Figure out which file we want, do nothing if we've already chosen that file
	Platform::String^ correctFilename = (Window::Current->Bounds.Width > Window::Current->Bounds.Height) ? "MainPage.xaml" : "MainPage.Portrait.xaml";
	if (correctFilename == this->filename)
	{
		return;
	}

	// Clear the results of the last file - markup compiler generated code will eventually do this part
	this->filename = correctFilename;
	this->_contentLoaded = false;
	this->Resources = nullptr;

	// Load the new file. TBD: Do you pass the whole filename or just the qualifierish part (e.g. "Portrait")
	this->InitializeComponent(ref new ::Windows::Foundation::Uri(L"ms-appx:///" + this->filename));

	// Set the text. Note that MyControlText is a TextBlock in both files, and MyControl is in both files with different types
	this->DisplayText->Text = "MyControl is a " + this->MyControl->GetType()->FullName;
}

void MultipleViewsTestbedCPP::MainPage::MyControl_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	this->DisplayText->Text += " (Clicked)";
}

void MultipleViewsTestbedCPP::MainPage::MyControl_Checked(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e)
{
	this->DisplayText->Text += " (Checked)";
}
