// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

// These are the two generated CPP files we recompile in the test DLL. They are
// required for TypeInfo to function correctly.
#include <XamlTypeInfo.Impl.g.cpp>
#include <XamlTypeInfo.g.cpp>

// Additionally we include the CPP files for the custom controls we're creating as well.
// They will be compiled a second time in the test DLL. We could attempt to directly include
// the .obj file but that seems fragile because they are using entirely separate build
// environments. 
#include <Sample.CustomButton.cpp>
#include <Sample.CustomPage.xaml.cpp>