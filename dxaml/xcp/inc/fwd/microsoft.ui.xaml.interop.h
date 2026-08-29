// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Forward declarations for external headers.  Please use in header files instead of declaring manually.

#include <abi/xaml_abi.h>

XAML_ABI_NAMESPACE_BEGIN
namespace Microsoft {
namespace UI {
namespace Xaml {
namespace Interop {
    class NotifyCollectionChangedEventArgs;
    enum NotifyCollectionChangedAction : int;
    enum TypeKind : int;
    interface IBindableIterable;
    interface IBindableIterator;
    interface IBindableObservableVector;
    interface IBindableVector;
    interface IBindableVectorChangedEventHandler;
    interface IBindableVectorView;
    interface INotifyCollectionChanged;
    interface INotifyCollectionChangedEventArgs;
    interface INotifyCollectionChangedEventArgsFactory;
    interface INotifyCollectionChangedEventHandler;
    struct TypeName;
} // Interop
} // Xaml
} // UI
} // Microsoft

XAML_ABI_NAMESPACE_END
