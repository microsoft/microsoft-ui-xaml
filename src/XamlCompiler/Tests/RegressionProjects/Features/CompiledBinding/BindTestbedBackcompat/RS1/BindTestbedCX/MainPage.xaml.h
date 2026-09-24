// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "BasicTests.xaml.h"
#include "FunctionTests.xaml.h"
#include "TestsPage2.xaml.h"
#include "PhasingTests.xaml.h"

namespace BindTestbed
{
	using namespace ::Platform;
	using namespace ::Windows::Foundation::Collections;

	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class MainModel sealed
	{
	public:
        property BindTestbedModel::DataModel^ Model;
		property BindTestbedModel::DOModel^ DOModel;
		property BindTestbedCXModel::ModelCX^ ModelCX;

		property IObservableVector<BindTestbedModel::IEmployee^>^ Employees;
		property IObservableVector<String^>^ AllFirstNames;
		property IObservableVector<String^>^ AllLastNames;

		MainModel()
		{
			this->Model = App::Model;
			this->DOModel = App::DOModel;
			this->ModelCX = App::ModelCX;
			this->Employees = ref new ::Platform::Collections::Vector<BindTestbedModel::IEmployee ^>();
			this->AllFirstNames = ref new ::Platform::Collections::Vector<String ^>();
			this->AllLastNames = ref new ::Platform::Collections::Vector<String ^>();

			this->InitializeValues();
		}
	
	private:
		void InitializeValues()
		{
			this->Model->InitializeValues();
			this->DOModel->UpdateValues();
			this->ModelCX->InitializeValues();

			for each (BindTestbedModel::IEmployee^ e in Model->ManagerProp->ReportsList)
			{
				this->Employees->Append(e);
				this->AllFirstNames->Append(e->FirstName);
				this->AllLastNames->Append(e->LastName);
			}
		}
	};

	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();
		property MainModel^ MainModel;

	private:
		void DetectLeaks_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);

    };
}