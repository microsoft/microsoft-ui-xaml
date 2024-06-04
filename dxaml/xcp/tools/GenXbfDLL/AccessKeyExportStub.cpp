// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AkExport.h"
#include "VisualTree.h"

using namespace AccessKeys;

struct AccessKeys::AccessKeyExportImpl {};

AccessKeyExport::AccessKeyExport(_In_ CCoreServices* const core)
{
}

AccessKeys::AccessKeyExport::~AccessKeyExport() = default;

void AccessKeyExport::SetVisualTree(_In_ VisualTree* tree) const
{
}

void AccessKeyExport::SetFocusManager(_In_ CFocusManager* focusManager) const
{
}