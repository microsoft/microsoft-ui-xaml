// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Text;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Interop;
using Windows.UI.Xaml.Markup;
using System.Runtime.CompilerServices;

// Forward all public Windows.UI types formerly provided by the WinUI projection assembly to the public Windows SDK projection assembly
[assembly: TypeForwardedToAttribute(typeof(CaretType))]
[assembly: TypeForwardedToAttribute(typeof(ContentLinkInfo))]
[assembly: TypeForwardedToAttribute(typeof(FindOptions))]
[assembly: TypeForwardedToAttribute(typeof(FormatEffect))]
[assembly: TypeForwardedToAttribute(typeof(HorizontalCharacterAlignment))]
[assembly: TypeForwardedToAttribute(typeof(ITextCharacterFormat))]
[assembly: TypeForwardedToAttribute(typeof(ITextDocument))]
[assembly: TypeForwardedToAttribute(typeof(ITextParagraphFormat))]
[assembly: TypeForwardedToAttribute(typeof(ITextRange))]
[assembly: TypeForwardedToAttribute(typeof(ITextSelection))]
[assembly: TypeForwardedToAttribute(typeof(LetterCase))]
[assembly: TypeForwardedToAttribute(typeof(LineSpacingRule))]
[assembly: TypeForwardedToAttribute(typeof(LinkType))]
[assembly: TypeForwardedToAttribute(typeof(MarkerAlignment))]
[assembly: TypeForwardedToAttribute(typeof(MarkerStyle))]
[assembly: TypeForwardedToAttribute(typeof(MarkerType))]
[assembly: TypeForwardedToAttribute(typeof(ParagraphAlignment))]
[assembly: TypeForwardedToAttribute(typeof(ParagraphStyle))]
[assembly: TypeForwardedToAttribute(typeof(PointOptions))]
[assembly: TypeForwardedToAttribute(typeof(RangeGravity))]
// RichEditMathMode is implemented on Windows 19041 and later and is available in those projections.
// WinUI depends on the Windows 17763 projection, so we can't declare a direct forwarding.
// In addition, doing a quick analysis, there is an MUX version of this type which is the one used
// in WinUI scenarios.  Due to this, not delcaring this as an explicit type forwarding.
// [assembly: TypeForwardedToAttribute(typeof(RichEditMathMode))]
[assembly: TypeForwardedToAttribute(typeof(RichEditTextDocument))]
[assembly: TypeForwardedToAttribute(typeof(RichEditTextRange))]
[assembly: TypeForwardedToAttribute(typeof(SelectionOptions))]
[assembly: TypeForwardedToAttribute(typeof(SelectionType))]
[assembly: TypeForwardedToAttribute(typeof(TabAlignment))]
[assembly: TypeForwardedToAttribute(typeof(TabLeader))]
[assembly: TypeForwardedToAttribute(typeof(TextConstants))]
[assembly: TypeForwardedToAttribute(typeof(TextDecorations))]
[assembly: TypeForwardedToAttribute(typeof(TextGetOptions))]
[assembly: TypeForwardedToAttribute(typeof(TextRangeUnit))]
[assembly: TypeForwardedToAttribute(typeof(TextScript))]
[assembly: TypeForwardedToAttribute(typeof(TextSetOptions))]
[assembly: TypeForwardedToAttribute(typeof(VerticalCharacterAlignment))]
[assembly: TypeForwardedToAttribute(typeof(BindableAttribute))]
[assembly: TypeForwardedToAttribute(typeof(BindableVectorChangedEventHandler))]
[assembly: TypeForwardedToAttribute(typeof(IBindableIterator))]
[assembly: TypeForwardedToAttribute(typeof(IBindableObservableVector))]
[assembly: TypeForwardedToAttribute(typeof(IBindableVectorView))]
[assembly: TypeForwardedToAttribute(typeof(ContentPropertyAttribute))]