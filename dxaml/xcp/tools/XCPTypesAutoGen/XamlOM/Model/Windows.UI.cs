// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Windows.UI
{
    [LiftedOptions(ExcludeFromLiftedCodegen = true)]
    [TypeFlags(IsWebHostHidden = false)]
    [NativeName("CColor")]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.DependencyObject))]
    [ClassFlags(HasTypeConverter = true)]
    [Platform(typeof(Windows.Foundation.UniversalApiContract), 1)]
    [Platform(2, typeof(Windows.Foundation.UniversalApiContract), 4)]
    [CodeGen(CodeGenLevel.Stub)]
    [ExternalIdl]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "5bd3ab50-210f-4531-9e08-2a09638cd369")]
    public struct Color
    {
        [CodeGen(CodeGenLevel.CoreOnly)]
        [NativeStorageType(ValueType.valueColor)]
        [OffsetFieldName("m_rgb")]
        [RenderDirtyFlagClassName("CDependencyObject")]
        [RenderDirtyFlagMethodName("NWSetRenderDirty")]
        public object ContentProperty
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Byte A
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Byte R
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Byte G
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        public Windows.Foundation.Byte B
        {
            get;
            set;
        }
    }
}

namespace Microsoft.UI
{
    [Imported("microsoft.ui.idl")]
    [WindowsTypePattern]
    public static class Colors
    {}
}