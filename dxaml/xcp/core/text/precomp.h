// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xcpwindows.h"
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include "winnls.h"
#include "MuiUdk.h"

#undef CDECL 
#include <combaseapi.h>

// Windows headers will #define these method names with A/W suffixes, which wreaks havoc on the link.
#ifdef GetClassName
#undef GetClassName
#endif

#include "xcperror.h"
#include "macros.h"
#include "xcpdebug.h"
#include "pal.h"
#include "core.h"
#include "refcounting.h"
#include "values.h"
#include "core_media.h"
#include "matrix.h"

#include "xstring_ptr.h"
#include "xstrutil.h"
#include "xcplist.h"
#include "corep.h"
#include "DependencyObjectTraits.g.h"
#include "rendertypes.h"
#include "real.h"
#include "memutils.h"
#include "span.h"
#include "ctypes.h"
#include "xcpmath.h"
#include "elements.h"
#include "text.h"
#include "EnumValueTable.h"
#include "LineFormatter.h"
#include <TextPosition.h>
#include <SelectionWordBreaker.h>

#include "DWritetypes.h"

// ShapingTypes.h is still used by Typography.cpp
#include "ShapingTypes.h"

typedef unsigned char BYTE;

#include "UcdProperties.h"
#include "UcdData.h"
#include "RichTextServicesHelper.h"
#include "TextAdapter.h"
#include "TextRangeAdapter.h"
#include "TextBoxHelpers.h"
#include "TxUtil.h"

#include "EventArgs.h"
#include "RoutedEventArgs.h"
#include "KeyboardEventArgs.h"
#include "InputPointEventArgs.h"
#include "TappedEventArgs.h"
#include "PointerEventArgs.h"
#include "RightTappedEventArgs.h"
#include "DoubleTappedEventArgs.h"
#include "PageNode.h"
#include "TextPosition.h"
#include "ITextSelection.h"
#include "textselection.h"
#include "SelectionWordBreaker.h"
#include "FocusRectangle.h"

#include <fonts.h>

#include <xmllite_error.h>

#include <host.h>

// RichTextServices
#include "LSBRK.H"
#include "BRKCOND.H"
#include "BRKCLS.H"

// RichTextServices/TextFormatter
#include "Ls.h"
#include "LsClient.h"
#include "LineBreaking.h"
#include "CharacterProperties.h"

// RichTextArea
#include "LineMetricsCache.h"

// RichTextBlock
#include "RichTextBlockBreak.h"

// TextBlock
#include "TextSchema.h"
#include "HyperlinkClickEventArgs.h"
#include "IsTextTrimmedChangedEventArgs.h"

// For HW rendering support
#include "hwwalk.h"
#include "compositor-all.h"
#include "hw-all.h"

#include "pixelformatutils.h"
#include "XcpTextGripperInputProcessor.h"
#include <new.h>

#include "MUX-ETWEvents.h"
