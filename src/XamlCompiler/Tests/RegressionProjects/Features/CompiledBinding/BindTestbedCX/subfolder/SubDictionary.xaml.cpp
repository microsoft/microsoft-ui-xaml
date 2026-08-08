// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SubDictionary.xaml.h"
#include "DetectLeaksPage.xaml.h"

using namespace BindTestbedCX;

subfolder::SubDictionary::SubDictionary()
{
    this->InitializeComponent();
    DetectLeaksPage::TrackObject(this, SubDictionary::GetType()->FullName);
}
