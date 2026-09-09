// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// NOT a generated file - this is a hand-authored forwarding header, and it is deliberately
// named after the runtime class rather than after the XAML file.
//
// This project exercises XAML file names that contain periods: the page lives in
// Namespace.CustomControl.xaml but declares x:Class="MultipleViewsTestbedCppWinRT.Namespace".
// The XAML compiler names its generated header after the *file*
// (Namespace.CustomControl.xaml.g.h), while C++/WinRT's Namespace.g.h only looks for a header
// named after the *class* ("Namespace.xaml.g.h") when deciding whether a XAML-aware NamespaceT
// exists. Without this shim C++/WinRT falls back to an alias with no InitializeComponent and
// the code-behind fails to compile.
//
#pragma once

#include "Namespace.CustomControl.xaml.g.h"
