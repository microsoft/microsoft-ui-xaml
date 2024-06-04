// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    interface IXamlTypeMeta
    {
        bool ImplementsINotifyPropertyChanged { get; }
        bool ImplementsINotifyCollectionChanged { get; }
        bool ImplementsIObservableVector { get; }
        bool ImplementsIObservableMap { get; }
        bool ImplementsINotifyDataErrorInfo { get; }
        bool HasApiInformation { get; }
    }
}
