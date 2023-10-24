// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Forward declarations for external headers.  Please use in header files instead of declaring manually.

#include <abi/xaml_abi.h>

XAML_ABI_NAMESPACE_BEGIN
namespace Windows {
namespace Foundation {
    class Deferral;
    class MemoryBuffer;
    class Uri;
    class WwwFormUrlDecoder;
    class WwwFormUrlDecoderEntry;
    interface IAsyncAction;
    interface IAsyncActionCompletedHandler;
    interface IClosable;
    interface IDeferral;
    interface IDeferralCompletedHandler;
    interface IDeferralFactory;
    interface IGetActivationFactory;
    interface IGuidHelperStatics;
    interface IMemoryBuffer;
    interface IMemoryBufferFactory;
    interface IMemoryBufferReference;
    interface IPropertyValue;
    interface IPropertyValueStatics;
    interface IStringable;
    interface IUriEscapeStatics;
    interface IUriRuntimeClass;
    interface IUriRuntimeClassFactory;
    interface IUriRuntimeClassWithAbsoluteCanonicalUri;
    interface IWwwFormUrlDecoderEntry;
    interface IWwwFormUrlDecoderRuntimeClass;
    interface IWwwFormUrlDecoderRuntimeClassFactory;
    struct DateTime;
    struct Point;
    struct Rect;
    struct Size;
    struct TimeSpan;
    template <typename T> struct IAsyncOperation;
    template <typename T, typename U> struct IAsyncOperationWithProgress;
    template <typename T> struct IAsyncOperationCompletedHandler;
    template <typename T, typename U> struct IAsyncOperationWithProgressCompletedHandler;
    template <typename T> struct IReference;
    template <typename T, typename U> struct ITypedEventHandler;
} // Foundation
} // Windows
XAML_ABI_NAMESPACE_END
