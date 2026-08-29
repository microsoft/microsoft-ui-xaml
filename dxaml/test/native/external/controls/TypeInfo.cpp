// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

// We include the CPP files for the custom controls we're creating.
// They will be compiled a second time in the test DLL. We could attempt to directly include
// the .obj file but that seems fragile because they are using entirely separate build environments. 
#include <CustomListViewItemPresenter.cpp>
#include <CustomListViewItemPanel.cpp>
#include <CustomAppBarButton.cpp>
#include <Grid.CustomTypes.cpp>
#include <Grid.CustomTypes.xaml.cpp>
#include <FirstTestPage.cpp>
#include <SecondTestPage.cpp>
#include <ThirdTestPage.cpp>

// These are the two generated CPP files we recompile in the test DLL. They are
// required for TypeInfo to function correctly.
#include <XamlTypeInfo.Impl.g.cpp>
#include <XamlTypeInfo.g.cpp>