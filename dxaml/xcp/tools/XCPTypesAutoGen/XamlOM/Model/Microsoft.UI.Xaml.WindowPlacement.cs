// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;

namespace Windows.Graphics
{
    [Imported]
    [WindowsTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    public struct RectInt32
    {
    }
}

namespace Microsoft.UI.Xaml
{
    [Platform(typeof(WinUIContract), 12)]
    [DXamlIdlGroup("coretypes2")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public enum WindowPlacementState
    {
        Normal = 0,
        Maximized = 1,
        Minimized = 2,
        Snapped = 3,
        MinimizedFromMaximized = 4,
        MinimizedFromSnapped = 5,
    }

    [Platform(typeof(WinUIContract), 12)]
    [DXamlIdlGroup("coretypes2")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public enum WindowShowReason
    {
        Default = 0,
        Launch = 1,
        ApplicationRestart = 2,
    }

    [Platform(typeof(WinUIContract), 12)]
    [DXamlIdlGroup("coretypes2")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public enum WindowCascadeBehavior
    {
        Automatic = 0,
        Enabled = 1,
        Disabled = 2,
    }

    [Platform(typeof(WinUIContract), 12)]
    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [ThreadingModel(ThreadingModel.Both)]
    [Guids(ClassGuid = "d92047ab-b193-42cf-bbfd-17ed7d975748")]
    public sealed class WindowPlacement
    {
        public WindowPlacement(Windows.Graphics.RectInt32 normalRect, Windows.Graphics.RectInt32 workArea, int dpi) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static WindowPlacement LoadForPersistPlacementId(Windows.Foundation.String persistPlacementId) { return default(WindowPlacement); }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Graphics.RectInt32 NormalRect { get; set; }
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Graphics.RectInt32 WorkArea { get; set; }
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public int Dpi { get; set; }
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public WindowPlacementState State { get; set; }
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Graphics.RectInt32? SnapRect { get; set; }
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public string DisplayDeviceName { get; set; }
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Guid? VirtualDesktopId { get; set; }
    }

    [Platform(typeof(WinUIContract), 12)]
    [DXamlIdlGroup("coretypes2")]
    [CodeGen(partial: true)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [ThreadingModel(ThreadingModel.Both)]
    [Guids(ClassGuid = "6f5e795e-4ae2-430e-b61a-b9087171d855")]
    public sealed class WindowShowOptions
    {
        public WindowShowOptions() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public WindowPlacement Placement { get; set; }
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public WindowShowReason Reason { get; set; }
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public WindowCascadeBehavior CascadeBehavior { get; set; }
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public bool DoNotActivate { get; set; }
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public bool SkipInitialPlacement { get; set; }
    }
}
