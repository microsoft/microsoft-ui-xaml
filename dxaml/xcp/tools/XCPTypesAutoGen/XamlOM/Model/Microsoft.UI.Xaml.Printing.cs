// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Printing
{
    [CodeGen(partial: true)]
    [NativeName("CPrintDocument")]
    [Guids(ClassGuid = "00f4014b-5d4a-4f4c-b5d3-dc9cd6163fa6")]
    public class PrintDocument
     : Microsoft.UI.Xaml.DependencyObject
    {
        [CodeGen(CodeGenLevel.CoreOnly)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_nPrintedPageCount")]
        [ReadOnly]
        public Windows.Foundation.Int32 PrintedPageCount
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_eDesiredFormat")]
        private Microsoft.UI.Xaml.Printing.PrintDocumentFormat DesiredFormat
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [ReadOnly]
        public Windows.Graphics.Printing.IPrintDocumentSource DocumentSource
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true)]
        internal event Microsoft.UI.Xaml.EventHandler BeginPrint;

        [CodeGen(CodeGenLevel.CoreOnly)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true)]
        internal event Microsoft.UI.Xaml.EventHandler EndPrint;

        public event Microsoft.UI.Xaml.Printing.PaginateEventHandler Paginate;

        public event Microsoft.UI.Xaml.Printing.GetPreviewPageEventHandler GetPreviewPage;

        public event Microsoft.UI.Xaml.Printing.AddPagesEventHandler AddPages;

        public PrintDocument() { }

        [PInvoke]
        public void AddPage(Microsoft.UI.Xaml.UIElement pageVisual)
        {
        }

        [PInvoke]
        public void AddPagesComplete()
        {
        }

        [PInvoke]
        public void SetPreviewPageCount(Windows.Foundation.Int32 count, Microsoft.UI.Xaml.Printing.PreviewPageCountType type)
        {
        }

        [PInvoke]
        public void SetPreviewPage(Windows.Foundation.Int32 pageNumber, Microsoft.UI.Xaml.UIElement pageVisual)
        {
        }

        [PInvoke]
        public void InvalidatePreview()
        {
        }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [ClassFlags(RequiresCoreServices = true)]
    [DXamlName("PrintPageEventArgs")]
    [NativeName("CPrintPageEventArgs")]
    [Guids(ClassGuid = "735cac02-30fa-4c19-ac3d-45464e9aba8a")]
    public sealed class PrintPageEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CPrintPageEventArgs", "PageVisual")]
        public Microsoft.UI.Xaml.UIElement PageVisual
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_fHasMorePages")]
        public Windows.Foundation.Boolean HasMorePages
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueSize)]
        [OffsetFieldName("m_sPrintableArea")]
        [ReadOnly]
        public Windows.Foundation.Size PrintableArea
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueThickness)]
        [OffsetFieldName("m_tPageMargins")]
        [ReadOnly]
        public Microsoft.UI.Xaml.Thickness PageMargins
        {
            get;
            private set;
        }

        public PrintPageEventArgs() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CBeginPrintEventArgs")]
    [Guids(ClassGuid = "1d9113fc-b12b-4b06-9693-6fb372f9d75d")]
    public sealed class BeginPrintEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        public BeginPrintEventArgs() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CEndPrintEventArgs")]
    [Guids(ClassGuid = "edcc918f-e61a-466e-b9ab-7f4dba17b98b")]
    public sealed class EndPrintEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_iErrorCode")]
        private Windows.Foundation.Int32 ErrorCode
        {
            get;
            set;
        }

        public EndPrintEventArgs() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CPaginateEventArgs")]
    [Guids(ClassGuid = "92383aa3-94a2-4f6a-b9f7-6d253b85f3ea")]
    public sealed class PaginateEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Graphics.Printing.PrintTaskOptions PrintTaskOptions
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Int32 CurrentPreviewPageNumber
        {
            get;
            internal set;
        }

        public PaginateEventArgs() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CAddPagesEventArgs")]
    [Guids(ClassGuid = "2be72ee1-09db-4990-94cc-7ce3dac8de1a")]
    public sealed class AddPagesEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Graphics.Printing.PrintTaskOptions PrintTaskOptions
        {
            get;
            internal set;
        }

        public AddPagesEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CGetPreviewPageEventArgs")]
    [Guids(ClassGuid = "cc461965-8f2c-496d-98b4-67965aedd925")]
    public sealed class GetPreviewPageEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_pageNumber")]
        public Windows.Foundation.Int32 PageNumber
        {
            get;
            internal set;
        }

        public GetPreviewPageEventArgs() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Comment("Determines the printing strategy to use - Bitmap or Vector")]
    [NativeName("PrintDocumentFormat")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [EnumFlags(HasTypeConverter = true)]
    internal enum PrintDocumentFormat
    {
        [NativeValueName("PrintDocumentFormat_Bitmap")]
        Bitmap = 0,
        [NativeValueName("PrintDocumentFormat_Vector")]
        Vector = 1,
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("PreviewPageCountType")]
    [EnumFlags(HasTypeConverter = true)]
    public enum PreviewPageCountType
    {
        [NativeValueName("PreviewPageCountType_Final")]
        Final = 0,
        [NativeValueName("PreviewPageCountType_Intermediate")]
        Intermediate = 1,
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    internal delegate void PrintPageEventHandler(Windows.Foundation.Object sender, Windows.Foundation.Object e);

    [CodeGen(CodeGenLevel.CoreOnly)]
    internal delegate void BeginPrintEventHandler(Windows.Foundation.Object sender, Windows.Foundation.Object e);

    [CodeGen(CodeGenLevel.CoreOnly)]
    internal delegate void EndPrintEventHandler(Windows.Foundation.Object sender, Windows.Foundation.Object e);

    public delegate void PaginateEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Printing.PaginateEventArgs e);

    public delegate void AddPagesEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Printing.AddPagesEventArgs e);

    public delegate void GetPreviewPageEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Printing.GetPreviewPageEventArgs e);
}

