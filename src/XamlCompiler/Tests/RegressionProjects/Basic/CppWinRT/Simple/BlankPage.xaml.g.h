// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// NOT a generated file - this is a hand-authored forwarding header, and it is deliberately
// named after the runtime class rather than after the XAML file.
//
// BlankPage.With.Dots.xaml declares x:Class="Simple.BlankPage", exercising XAML file names that
// contain periods. The XAML compiler names its generated header after the *file*
// (BlankPage.With.Dots.xaml.g.h), while C++/WinRT's BlankPage.g.h only looks for a header named
// after the *class* ("BlankPage.xaml.g.h") when deciding whether a XAML-aware BlankPageT exists.
// Without this shim C++/WinRT falls back to an alias with no InitializeComponent and the
// code-behind fails to compile.
//
#pragma once

#include "BlankPage.With.Dots.xaml.g.h"
