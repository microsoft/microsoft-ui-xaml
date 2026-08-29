// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xcpwindows.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.foundation.collections.h>
#include <windows.ui.text.core.h>

#define MCGEN_LEVEL_KEYWORD_ENABLED_DEF
#include "MUX-ETWEvents.h"

#include <microsoft.ui.xaml.h>
#include <microsoft.ui.xaml.controls.controls.h>
#include <Activators.g.h>

#include <windows.ui.popups.h>
#include <ole2.h>
#include <Richedit.h>
#include <RichOle.h>
#include <InputScope.h>
#include <D2d1.h>
#include <D3d11.h>
#include <DWrite_3.h>

#define IPROVIDEFONTINFO
#include <textserv.h>
#include <tom.h>

#include <shobjidl_core.h>

// Windows headers will #define these method names with A/W suffixes, which wreaks havoc on the link.
#ifdef GetClassName
#undef GetClassName
#endif

#include "macros.h"
#include "xcpdebug.h"
#include "pal.h"
#include "core.h"
#include "host.h"
#include "values.h"
#include "matrix.h"
#include "corep.h"
#include "DependencyObjectTraits.g.h"
#include "rendertypes.h"
#include "memutils.h"
#include "xstrutil.h"
#include "xcpmath.h"
#include "real.h"
#include "elements.h"
#include "TextBoxView.h"
#include "TextBoxBase.h"
#include "TextBox.h"
#include "PasswordBox.h"
#include "RichEditBox.h"
#include "TextBoxBaseAutomationPeer.h"
#include "TextBoxAutomationPeer.h"
#include "PasswordBoxAutomationPeer.h"
#include "RichEditBoxAutomationPeer.h"
#include "eventargs.h"
#include "routedeventargs.h"
#include "keyboardeventargs.h"
#include "CharacterReceivedEventArgs.h"
#include "textchangedeventargs.h"
#include "textcompositioneventargs.h"
#include "InputPointEventArgs.h"
#include "eventmgr.h"
#include "FocusMgr.h"
#include "Pointer.h"
#include "PointerEventArgs.h"
#include "DoubleTappedEventArgs.h"
#include "TappedEventArgs.h"
#include "RightTappedEventArgs.h"
#include "HoldingEventArgs.h"
#include "TextControlPasteEventArgs.h"
#include "TextControlCopyingToClipboardEventArgs.h"
#include "TextControlCuttingToClipboardEventArgs.h"
#include "CharacterProperties.h"
#include "TypefaceCollection.h"
#include "DWriteFontFace.h"
#include "DWriteFontFamily.h"
#include "Text.h"
#include "Span.h"
#include "Fonts.h"
#include "CInputScope.h"
#include "RichEditGripper.h"
#include "RichEditGripperChild.h"
#include "PrivateTextInputSettings.h"
#include "definitioncollection.h"

#include "D2DAccelerated.h"
#include "D2DAcceleratedRT.h"
#include "D2DPrintTarget.h"

#include "compositortree.h"

// For HW rendering support
#include "hw-all.h"

#include <DCommon.h>
#include <d2dBaseTypes.h>
#include <D2D1.h>
#include <D2D1_1.h>

#include "text.h"

#include "RtsInterop.h"
#include "TextDrawingContext.h"
#include "DWriteFontFace.h"
