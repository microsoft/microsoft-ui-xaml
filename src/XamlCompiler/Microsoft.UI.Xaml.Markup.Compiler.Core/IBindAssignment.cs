// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public interface IBindAssignment
    {
        BindStatus BindStatus { get; }
        int ComputedPhase { get; }
        bool HasSetValueHelper { get; }
        bool IsTrackingSource { get; }
        bool IsTrackingTarget { get; }
    }
}