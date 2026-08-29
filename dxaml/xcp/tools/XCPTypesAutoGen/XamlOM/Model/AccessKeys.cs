// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using XamlOM;
using XamlOM.NewBuilders;
using Windows.Foundation;

namespace Microsoft.UI.Xaml.Input
{
    public enum KeyTipPlacementMode
    {
        Auto,
        Bottom,
        Top,
        Left,
        Right,
        Center,
        Hidden
    }

    [Platform(2, typeof(Microsoft.UI.Xaml.WinUIContract), 5)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public static class AccessKeyManager
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static event TypedEventHandler<Object, Object> IsDisplayModeEnabledChanged;

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static bool IsDisplayModeEnabled
        {
            get
            {
                return false;
            }
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static bool AreKeyTipsEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static void ExitDisplayMode() { }

        [Version(2)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlOverloadName("EnterDisplayMode")]
        public static void EnterDisplayModeForXamlRoot(Microsoft.UI.Xaml.XamlRoot XamlRoot) { }
    }

    [NativeName("CAccessKeyDisplayRequestedEventArgs")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "4aa78368-46ed-4b39-b3f2-1ebfc9238ffb")]
    public sealed class AccessKeyDisplayRequestedEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        public AccessKeyDisplayRequestedEventArgs() { }

        [NativeStorageType(ValueType.valueString)]
        [OffsetFieldName("m_strPressedKeys")]
        [DelegateToCore]
        public string PressedKeys { get; internal set; }
    }

    [NativeName("CAccessKeyDisplayDismissedEventArgs")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "c9b22510-3344-4350-b7d2-60bf2d304b07")]
    public sealed class AccessKeyDisplayDismissedEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        public AccessKeyDisplayDismissedEventArgs() { }
    }

    [NativeName("CAccessKeyInvokedEventArgs")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "68c26118-fc64-4621-a7b0-6bbdb9a3179b")]
    public sealed class AccessKeyInvokedEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        public AccessKeyInvokedEventArgs() { }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public bool Handled { get; set; }
    }
}
