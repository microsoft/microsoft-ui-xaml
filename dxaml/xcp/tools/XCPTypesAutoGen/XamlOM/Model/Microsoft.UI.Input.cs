// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;
using XamlOM.NewBuilders;

namespace Microsoft.UI.Input
{
    [Imported]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeFlags(IsDXamlSystemType = true)]
    [NativeName("PointerDeviceType")]
    public enum PointerDeviceType
    {
        [NativeValueName("XcpTouch")]
        Touch = 0,
        [NativeValueName("XcpPen")]
        Pen = 1,
        [NativeValueName("XcpMouse")]
        Mouse = 2,
    }

    [Imported("microsoft.ui.input.idl")]
    [WindowsTypePattern]
    public interface IPointerPointTransform
    {
    }

    [Imported("microsoft.ui.input.idl")]
    [WindowsTypePattern]
    [DXamlName("PointerPoint")]
    public sealed class PointerPoint
    {
    }

    [Imported("microsoft.ui.input.idl")]
    [WindowsTypePattern]
    [DXamlName("ManipulationDelta")]
    public struct ManipulationDelta
    {
    }

    [Imported("microsoft.ui.input.idl")]
    [WindowsTypePattern]
    [DXamlName("ManipulationVelocities")]
    public struct ManipulationVelocities
    {
    }

    [Imported("microsoft.ui.input.idl")]
    [WindowsTypePattern]
    public enum HoldingState
    {
        [NativeValueName("XcpStarted")]
        Started = 0,
        [NativeValueName("XcpCompleted")]
        Completed = 1,
        [NativeValueName("XcpCanceled")]
        Canceled = 2,
    }

    [Imported("microsoft.ui.input.idl")]
    [EnumFlags(AreValuesFlags = true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [WindowsTypePattern]
    public enum InputPointerSourceDeviceKinds
    {
        // This enum is left intentionally empty. Its only use is as a 
        // parameter to SwapChainPanel.CreateCoreIndependentInputSource, and is
        // never expected to be used in markup or as a DependencyProperty.
    }

    [Imported("microsoft.ui.input.idl")]
    [WindowsTypePattern]
    [DXamlName("InputPointerSource")]
    public sealed class InputPointerSource
    {
    }

    [Imported("microsoft.ui.input.idl")]
    [WindowsTypePattern]
    [DXamlName("InputCursor")]
    public sealed class InputCursor
    {
    }
    
}
