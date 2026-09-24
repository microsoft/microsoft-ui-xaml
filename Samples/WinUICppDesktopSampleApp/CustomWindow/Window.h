// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Window.g.h"

namespace winrt::CustomWindow::implementation
{
	struct Window : WindowT<Window>
	{
		Window();
	};
}

namespace winrt::CustomWindow::factory_implementation
{
	struct Window : WindowT<Window, implementation::Window>
	{
	};
}
