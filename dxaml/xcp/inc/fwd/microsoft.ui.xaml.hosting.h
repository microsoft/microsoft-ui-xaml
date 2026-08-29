// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Forward declarations for external headers.  Please use in header files instead of declaring manually.

#include <abi/xaml_abi.h>

XAML_ABI_NAMESPACE_BEGIN
namespace Microsoft {
namespace UI {
namespace Xaml {
namespace Hosting {
    class DesktopWindowXamlSource;
    class DesktopWindowXamlSourceGotFocusEventArgs;
    class DesktopWindowXamlSourceTakeFocusRequestedEventArgs;
    class ElementCompositionPreview;
    class WindowsXamlManager;
    class XamlSourceFocusNavigationRequest;
    class XamlSourceFocusNavigationResult;
    enum class RefCountType : bool;
    enum class RefCountUpdateType : bool;
    enum XamlSourceFocusNavigationReason : int;
    interface IDesktopWindowXamlSource;
    interface IDesktopWindowXamlSourceFactory;
    interface IDesktopWindowXamlSourceGotFocusEventArgs;
    interface IDesktopWindowXamlSourceTakeFocusRequestedEventArgs;
    interface IElementCompositionPreview;
    interface IElementCompositionPreviewStatics;
    interface IElementCompositionPreviewStatics2;
    interface IElementCompositionPreviewStatics3;
    interface IFocusController;
    interface IReferenceTrackerGCLock;
    interface IReferenceTrackerInternal;
    interface ITrackerPtrWrapper;
    interface IWindowsXamlManager;
    interface IWindowsXamlManagerStatics;
    interface IXamlIslandRoot;
    interface IXamlIslandRootStatics;
    interface IXamlSourceFocusNavigationRequest;
    interface IXamlSourceFocusNavigationRequestFactory;
    interface IXamlSourceFocusNavigationResult;
    interface IXamlSourceFocusNavigationResultFactory;
} // Hosting
} // Xaml
} // UI
} // Microsoft
XAML_ABI_NAMESPACE_END
