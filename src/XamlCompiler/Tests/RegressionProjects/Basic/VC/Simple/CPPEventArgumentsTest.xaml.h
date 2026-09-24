// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// CPPEventArgumentsTest.xaml.h
// Declaration of the CPPEventArgumentsTest class
//

#pragma once

#include "CPPEventArgumentsTest.g.h"

namespace Simple
{

	public delegate void MyFirstEvent(const Platform::Array<unsigned int>^ args);
	public delegate void MySecondEvent(Platform::WriteOnlyArray<unsigned int>^ args);
	public delegate void MyThirdEvent(Platform::Array<unsigned int>^* args);
	public delegate void MyFourthEvent(Platform::String^* args);


	[::Windows::Foundation::Metadata::WebHostHidden]
	public ref class CPPEventArgumentsTest sealed
	{
	public:
		CPPEventArgumentsTest();
		event MyFirstEvent^ TheFirstEvent;
		event MySecondEvent^ TheSecondEvent;
		event MyThirdEvent^ TheThirdEvent;
		event MyFourthEvent^ TheFourthEvent;
	};
}
