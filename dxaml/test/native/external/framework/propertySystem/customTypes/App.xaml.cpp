// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This file is simply to get the app.xaml.h to be compiled.  Previously, this was included by adding it
// to the pch.h file and something would include it.  However, now that we are using a common pch file,
// we need to be able to compile it.
#include "pch.h"
#include "app.xaml.h"