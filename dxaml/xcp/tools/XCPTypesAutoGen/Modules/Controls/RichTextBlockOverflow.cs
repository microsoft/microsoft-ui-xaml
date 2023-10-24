// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls
{
    [CodeGen(partial: true)]
    [NativeName("CRichTextBlockOverflow")]
    [Guids(ClassGuid = "28b1261a-787e-402c-8c99-70e0e5afa1f8")]
    public sealed class RichTextBlockOverflow
        : Microsoft.UI.Xaml.FrameworkElement
    {
        #region Properties

        [RequiresMultipleAssociationCheck]
        [PropertyFlags(AffectsArrange = true, DoNotEnterOrLeaveValue = true)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pOverflowTarget")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Controls.RichTextBlockOverflow OverflowContentTarget
        {
            get;
            set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_padding")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Microsoft.UI.Xaml.Thickness Padding
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyFlags(DoNotEnterOrLeaveValue = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pMaster")]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.RichTextBlock ContentSource
        {
            get;
            private set;
        }

        [NativeMethod("CRichTextBlockOverflow", "HasOverflowContent")]
        [NativeStorageType(ValueType.valueBool)]
        [ReadOnly]
        public Windows.Foundation.Boolean HasOverflowContent
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Documents.TextPointer ContentStart
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Documents.TextPointer ContentEnd
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double BaselineOffset
        {
            get;
            private set;
        }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_maxLines")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Int32 MaxLines
        {
            get;
            set;
        }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isTextTrimmed")]
        public Windows.Foundation.Boolean IsTextTrimmed
        {
            get;
        }

        #endregion

        #region Events

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<RichTextBlockOverflow, IsTextTrimmedChangedEventArgs> IsTextTrimmedChanged;

        #endregion

        #region Methods

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Documents.TextPointer GetPositionFromPoint(Windows.Foundation.Point point)
        {
            return default(Microsoft.UI.Xaml.Documents.TextPointer);
        }

        #endregion

        public RichTextBlockOverflow() { }
    }
}

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls")]
    [NativeName("CRichTextBlockOverflowAutomationPeer")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "2c2086b4-6899-44ba-8324-376ed6d845d5")]
    public class RichTextBlockOverflowAutomationPeer
        : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public RichTextBlockOverflowAutomationPeer(Microsoft.UI.Xaml.Controls.RichTextBlockOverflow owner)
            : base(owner) { }
    }
}
