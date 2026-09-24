// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Templates.xaml.h"
#include "DetectLeaksPage.xaml.h"

BindTestbed::Templates::Templates()
{
    this->InitializeComponent();
	DetectLeaksPage::TrackObject(this, Templates::GetType()->FullName);
}
