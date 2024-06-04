// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "encodedptr.h"

#pragma warning(disable: 4073)
#pragma init_seg(lib)

EncodedPtrWithDeleteAndgpsReset<CWindowsServices> theOnlyWinServices(new CWindowsServices());