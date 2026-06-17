// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Settings
{
    // Identifies an individual optional change that can be enabled or disabled
    // through XamlOptionalChanges. Each value corresponds to a specific feature,
    // fix, or behavioral change documented in the WinUI release notes.
    [Contract(typeof(Microsoft.UI.Xaml.WinUIContract), 9)]
    [DXamlIdlGroup("coretypes2")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public enum XamlChangeId
    {
        // When adding a value, also add a case to GetBitIndex() in XamlOptionalChanges_Partial.cpp
        // and a bit-index constant in OptionalChangeState.h.
        _Reserved = 0,
        IconNoGridOptimization = 61276805,
        OptimizeApplyStyles = 61697456,
        DefaultStyleOptimizations = 60995620,
        DeferContextFlyoutInit = 61098986,
    }

    // Provides static methods to opt in to or out of individual breaking or
    // behavioral changes identified by XamlChangeId. All methods are static;
    // you do not instantiate this class.
    //
    // Call EnableChange / DisableChange before Application.Start() or
    // WindowsXamlManager.InitializeForCurrentThread(). The state is
    // automatically locked at the entry point of whichever of those methods
    // is called first in the process.
    [Contract(typeof(Microsoft.UI.Xaml.WinUIContract), 9, ForcePrimaryInterfaceGeneration = true)]
    [DXamlIdlGroup("coretypes2")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [ClassFlags(HasBaseTypeInDXamlInterface = false)]
    [CodeGen(partial: true)]
    [ThreadingModel(ThreadingModel.Both)]
    [Guids(ClassGuid = "baf25863-8d7a-402f-9e09-45d71aeee566")]
    public sealed class XamlOptionalChanges
    {
        internal XamlOptionalChanges() { }

        // Enables the specified optional change. Returns true if the change is
        // recognized in this SDK and is now enabled; returns false if the value
        // is unrecognized (the change does not exist in this build).
        // Throws InvalidOperationException (E_ILLEGAL_STATE_CHANGE) if the state
        // has been locked, for any changeId (recognized or not).
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static bool EnableChange(XamlChangeId changeId) { return default(bool); }

        // Explicitly disables the specified optional change. Returns true if the
        // change is recognized and is now disabled; returns false if the value is
        // unrecognized (or a future permanent change that cannot be opted out of).
        // Throws InvalidOperationException (E_ILLEGAL_STATE_CHANGE) if the state
        // has been locked, for any changeId (recognized or not).
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static bool DisableChange(XamlChangeId changeId) { return default(bool); }

        // Returns true if the specified change is currently enabled.
        // Safe to call at any time (before or after locking). Never throws.
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static bool IsChangeEnabled(XamlChangeId changeId) { return default(bool); }

        // Freezes the optional-changes state. Returns true if this call
        // performed the lock, false if already locked. Never throws.
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static bool Lock() { return default(bool); }

        // Returns true if the optional-changes state has been locked. Never throws.
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static bool IsLocked() { return default(bool); }
    }
}
