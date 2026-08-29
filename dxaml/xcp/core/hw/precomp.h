// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xcpwindows.h"
#include <windows.foundation.h>
#include <windows.foundation.collections.h>

// External dependencies

//
// Common XCP includes
//
#include "xcperror.h"
#include "macros.h"
#include "xcpdebug.h"
#include "pal.h"
#include "core.h"
#include "refcounting.h"
#include "ctypes.h"
#include "values.h"
#include "matrix.h"
#include "xcplist.h"
#include "corep.h"
#include "DependencyObjectTraits.g.h"
#include "rendertypes.h"
#include "xcpmath.h"
#include "real.h"
#include "memutils.h"
#include "sort.h"
#include "pixelformatutils.h"
#include "elements.h"
#include "TextBoxView.h"
#include "TextBox.h"
#include "text.h"

#include "image.h"

// Compositor includes
#include "compositor-all.h"

// HW includes exposed to other libraries
#include "hw-all.h"

#include "DManipData.h"

#include "D2DUtils.h"
#include "Windowsgraphicsdevicemanager.h"
#include "DCompTreeHost.h"
#include "SystemMemoryBits.h"
#include "D3D11Device.h"
#include "DCompSurface.h"

#include "MUX-ETWEvents.h"
