// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <Microsoft.UI.Xaml.h>
#include <Microsoft.UI.Xaml.private.h>
#include <microsoft.ui.xaml.phone.h>
//#include <microsoft.ui.xaml.phone-private.h>

#include <minxcptypes.h>

#include <SatMacros.h>
#include <NamespaceAliases.h>
#include <ReferenceTrackerExtension.h>
#include <TrackerPtrFamily.h>
#include <ReferenceTrackerRuntimeClass.h>
#include <ComTemplateLibrary.h>
#include <FeatureFlags.h>

#include "PhoneTypes.g.h"

#include "DatePickerFlyout_Partial.h"
#include "DatePickerFlyoutPresenter_Partial.h"
#include "DatePickerFlyoutItem_Partial.h"
#include "DatePickerFlyoutPresenterAutomationPeer_Partial.h"
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

#include <XamlTypeInfo.h>
#include <ValueBoxer.h>

#include "XamlTypeInfo.g.cpp"
