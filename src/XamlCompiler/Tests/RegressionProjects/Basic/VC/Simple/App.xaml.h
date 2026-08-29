// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"

namespace Simple
{
	/// <summary>
	/// Provides application-specific behavior to supplement the default Application class.
	/// </summary>
	ref class App sealed
	{
	protected:
		virtual void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs^ e) override;

	internal:
		App();

	private:
		void OnSuspending(Platform::Object^ sender, ::Windows::ApplicationModel::SuspendingEventArgs^ e);
		void OnNavigationFailed(Platform::Object ^sender, Microsoft::UI::Xaml::Navigation::NavigationFailedEventArgs ^e);
	};
}
