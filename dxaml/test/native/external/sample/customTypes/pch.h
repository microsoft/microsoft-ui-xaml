// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// For non-XAML-backed custom types we must include their header
// files here, otherwise the XamlCompiler-generated files will fail
// to compile due to the files not already being included.
#include <Sample.CustomButton.h>