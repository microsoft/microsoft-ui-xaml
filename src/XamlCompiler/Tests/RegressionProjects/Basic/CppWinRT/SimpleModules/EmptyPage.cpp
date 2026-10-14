#include <unknwn.h>

#ifdef WINRT_IMPORT_MODULE
import Simple.Application_Xaml.EmptyPage;
#endif
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "EmptyPage.h"
#include "EmptyPage.g.cpp"

namespace winrt::Simple::implementation
{
    EmptyPage::EmptyPage()
    {
        // Intentionally does not call InitializeComponent(). The regression removes and
        // restores EmptyPage.xaml without cleaning, so the runtime class must remain a
        // valid non-XAML component when the Page item is temporarily absent.
    }
}
