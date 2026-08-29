// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContextMenuEventArgs.g.h"
#include "RoutedEventArgs.h"
#include "ContextMenuEventArgs.h"

CEventArgs* DirectUI::ContextMenuEventArgs::CreateCorePeer()
{
    return new CContextMenuEventArgs();
}
