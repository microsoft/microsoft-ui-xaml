// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "DragDropTestPoolFilter.h"
#include "XamlTailored.h"
#include "IXamlTestHooks-win.h"
#include "WindowHelper.h"

using namespace Private::Infrastructure;
using namespace Microsoft::UI::Xaml::Tests::Common; 
using namespace WEX::Common;
using namespace WEX::Logging;

bool DragDropTestPoolFilter::IsDirty()
{
    bool isDirty = false;
    RunOnUIThread([&]() {
        isDirty = WindowHelper::GetTestHooks()->IsDragDropInProgress();
    });
    return isDirty;
}