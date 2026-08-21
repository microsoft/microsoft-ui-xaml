// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// This app targets .NET 6, whose corelib doesn't define the attributes that the C# 11
// 'required' members feature needs, so the compiler emits CS0656 without them. Polyfill
// them here so the regression tests in MainWindow.xaml.cs can use 'required' properties.
// Both types are defined in the corelib starting with .NET 7, so this can be deleted if
// the app is ever retargeted.
namespace System.Runtime.CompilerServices
{
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false, Inherited = false)]
    internal sealed class RequiredMemberAttribute : Attribute
    {
    }

    [AttributeUsage(AttributeTargets.All, AllowMultiple = true, Inherited = false)]
    internal sealed class CompilerFeatureRequiredAttribute : Attribute
    {
        public CompilerFeatureRequiredAttribute(string featureName)
        {
            FeatureName = featureName;
        }

        public string FeatureName { get; }

        public bool IsOptional { get; init; }

        public const string RefStructs = nameof(RefStructs);

        public const string RequiredMembers = nameof(RequiredMembers);
    }
}
