// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Important:
//  <new> needs to be first to ensure correct linkage of new/delete operators.
//  If we dont do this, the delete of a vector new[] will be generated as scalar delete instead of a vector delete.
//  This will fail chk builds and can potentially lead to heap corruption.

#include <new>

#include <windows.h>
#include <strsafe.h>
#include <initguid.h>

// WRL
#include <wrl\async.h>
#include <wrl.h>

#include <windows.globalization.h>
#include <Windows.UI.ViewManagement.h>
#include <windowscollections.h>
#include <windows.storage.h>

// WIL
#include <wil/common.h>

// WinRT
#include <windows.graphics.display.h>
#include <robuffer.h>
#include <windows.foundation.h>
#include <windows.foundation.declarations.h>

// STL
#include <list>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <chrono>
#include <type_traits>

// XAML
#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime

#include <microsoft.ui.xaml.h>
#include <microsoft.ui.xaml.controls.controls.h>
#include <microsoft.ui.xaml.private.h>
#include <microsoft.ui.xaml.phone.h>
//#include <microsoft.ui.xaml.phone-private.h>

#include <ReferenceTrackerExtension.h>

#pragma pop_macro("GetCurrentTime")

// common
#include "NamespaceAliases.h"
#include "SatMacros.h"
#include "minerror.h"
#include "macros.h"
#include "RuntimeErrorCodes.h"

// Reference tracking classes
#include "TrackerPtrFamily.h"
#include "ReferenceTrackerRuntimeClass.h"

// utility
#include "DllInitializedHelpers.h"
#include "LocalizationHelpers.h"
#include "DebugWriter.h"
#include "AutomationHelper.h"
#include "ValueBoxer.h"
#include "ValueHelpers.h"
#include "TypeUtils.h"
#include "ApplicationResourceHelpers.h"
#include "BindableToObservableVectorWrapper.h"

// COM aggregation support
#include "ComTemplateLibrary.h"

// Localization resources
#include "PhoneResource.h"

// Generated types
#include "PhoneTypes.g.h"

// UIA Headers
#include <uiautomationcore.h>
#include <uiautomationcoreapi.h>

#include "RoVariant.h"

// ------------------------------------------------------------------------------------------------
// ***** Everything after this point should be order-independent *****
// ------------------------------------------------------------------------------------------------

// controls
#include "DatePickerFlyout_Partial.h"
#include "DatePickerFlyoutPresenter_Partial.h"
#include "DatePickerFlyoutItem_Partial.h"
#include "DatePickerFlyoutPresenterAutomationPeer_Partial.h"
#include "DateTimePickerFlyoutHelper.h"
#include "LoopingSelector_Partial.h"
#include "LoopingSelectorAutomationPeer_Partial.h"
#include "LoopingSelectorItem_Partial.h"
#include "LoopingSelectorItemAutomationPeer_Partial.h"
#include "LoopingSelectorItemDataAutomationPeer_Partial.h"
#include "LoopingSelectorPanel_Partial.h"
#include "Pivot_Partial.h"
#include "PivotPanel_Partial.h"
#include "PivotHeaderPanel_Partial.h"
#include "PivotAutomationPeer_Partial.h"
#include "PivotItem_Partial.h"
#include "PivotItemAutomationPeer_Partial.h"
#include "PivotItemDataAutomationPeer_Partial.h"
#include "PivotHeaderItem.h"
#include "PivotItemEventArgs.h"
#include "TimePickerFlyout_Partial.h"
#include "TimePickerFlyoutPresenter_Partial.h"
#include "TimePickerFlyoutPresenterAutomationPeer_Partial.h"
#include "NavigateTransitionHelper.h"
#include "ThemeTransitions.h"
#include "PickerFlyoutBase_Partial.h"
#include "PickerFlyout_Partial.h"
#include "PickerFlyoutPresenter_Partial.h"
#include "PickerFlyoutPresenterAutomationPeer_Partial.h"
#include "ListPickerFlyout_Partial.h"
#include "ListPickerFlyoutPresenter_Partial.h"
#include "ListPickerFlyoutPresenterAutomationPeer_Partial.h"
#include "PickerConfirmedEventArgs_Partial.h"
#include "DatePickedEventArgs_Partial.h"
#include "TimePickedEventArgs_Partial.h"
#include "ItemsPickedEventArgs_Partial.h"
#include "JumpListHelper.h"
#include "JumpListItemForegroundConverter_Partial.h"
#include "JumpListItemBackgroundConverter_Partial.h"
#include "PlatformHelpers.h"

#include "SelectionExports.h"
#include "MuiUdk.h"
