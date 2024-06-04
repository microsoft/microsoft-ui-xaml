// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xcpwindows.h>
#include <windows.foundation.collections.h>
#include <vector>

#include <D2D1.h>
#include <D2D1helper.h>
#include <D2D1_1.h>
#include <D2D1_1helper.h>
#include <DWrite_3.h>
#include <wincodec.h>

// XCP types
#include "macros.h"
#include "palcore.h"
#include "paltypes.h"
#include "paldebugging.h"
#include "palexports.h"

// Brush
#include "pal.h"
#include "core.h"
#include "values.h"
#include "corep.h"
#include "host.h"
#include "DependencyObjectTraits.g.h"
#include "depends.h"
#include "brush.h"
#include "tilebrush.h"
#include "imagebrush.h"

// PC Walk
#include "xcpmath.h"
#include "real.h"
#include "rendertypes.h"
#include "hwwalk.h"
#include "hwtexturemgr.h"
#include "hwrealization.h"
#include "SystemMemoryBits.h"
#include "D3D11Device.h"
#include "DCompSurface.h"

// D2D Walk
#include "collectionbase.h"
#include "docollection.h"
#include "LayoutStorage.h"
#include "DirectManipulationContainerHandler.h"
#include "DirectManipulationContainer.h"
#include "cmatrix.h"
#include "transforms.h"
#include "uielement.h"

#include "DependencyPropertyProxy.h"
#include "VisualStateGroupCollection.h"
#include "framework.h"
#include "mediabase.h"
#include "imagebase.h"
#include "TiledSurface.h"
#include "ImageSurfaceWrapper.h"
#include "imagesource.h"
#include "WriteableBitmap.h"

// Projection
#include "perspectiveplane.h"
#include "compositortree.h"

// RTS and general text layout.
#include "RtsInterop.h"
#include "TextDrawingContext.h"
#include "TextRunProperties.h"
#include "TxUtil.h"
#include "text.h"

#include "DWriteFontFace.h"
#include "D2DAcceleratedBrushes.h"

#include "MUX-ETWEvents.h"
