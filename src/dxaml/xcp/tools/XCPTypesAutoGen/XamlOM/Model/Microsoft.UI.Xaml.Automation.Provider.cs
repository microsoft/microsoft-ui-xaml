// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Automation.Provider
{
    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "097bcce2-e99b-463c-8aa0-ae09f4e2d423")]
    public sealed class IRawElementProviderSimple
     : Microsoft.UI.Xaml.DependencyObject
    {
        private IRawElementProviderSimple() { }
    }

    public interface IInvokeProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Invoke();
    }

    public interface IDockProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.DockPosition DockPosition
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetDockPosition(Microsoft.UI.Xaml.Automation.DockPosition dockPosition);
    }

    public interface IDragProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean IsGrabbed
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String DropEffect
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String[] DropEffects
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple[] GetGrabbedItems();
    }

    public interface IDropTargetProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String DropEffect
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String[] DropEffects
        {
            get;
        }
    }

    public interface IExpandCollapseProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.ExpandCollapseState ExpandCollapseState
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Collapse();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Expand();
    }

    public interface IGridItemProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Int32 Column
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Int32 ColumnSpan
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple ContainingGrid
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Int32 Row
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Int32 RowSpan
        {
            get;
        }
    }

    public interface IGridProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Int32 ColumnCount
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Int32 RowCount
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple GetItem(Windows.Foundation.Int32 row, Windows.Foundation.Int32 column);
    }

    public interface IMultipleViewProvider
    {
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Int32 CurrentView
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Int32[] GetSupportedViews();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.String GetViewName(Windows.Foundation.Int32 viewId);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetCurrentView(Windows.Foundation.Int32 viewId);
    }

    public interface IRangeValueProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean IsReadOnly
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double LargeChange
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double Maximum
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double Minimum
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double SmallChange
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double Value
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetValue(Windows.Foundation.Double value);
    }

    public interface IScrollItemProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void ScrollIntoView();
    }

    public interface IScrollProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean HorizontallyScrollable
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double HorizontalScrollPercent
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double HorizontalViewSize
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean VerticallyScrollable
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double VerticalScrollPercent
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double VerticalViewSize
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Scroll(Microsoft.UI.Xaml.Automation.ScrollAmount horizontalAmount, Microsoft.UI.Xaml.Automation.ScrollAmount verticalAmount);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetScrollPercent(Windows.Foundation.Double horizontalPercent, Windows.Foundation.Double verticalPercent);
    }

    public interface ISelectionItemProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean IsSelected
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple SelectionContainer
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void AddToSelection();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void RemoveFromSelection();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Select();
    }

    public interface ISelectionProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean CanSelectMultiple
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean IsSelectionRequired
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple[] GetSelection();
    }

    public interface ITableItemProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple[] GetColumnHeaderItems();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple[] GetRowHeaderItems();
    }

    public interface ITableProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.RowOrColumnMajor RowOrColumnMajor
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple[] GetColumnHeaders();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple[] GetRowHeaders();
    }

    public interface ITextRangeProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider Clone();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean Compare(Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider textRangeProvider);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Int32 CompareEndpoints(Microsoft.UI.Xaml.Automation.Text.TextPatternRangeEndpoint endpoint, Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider textRangeProvider, Microsoft.UI.Xaml.Automation.Text.TextPatternRangeEndpoint targetEndpoint);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void ExpandToEnclosingUnit(Microsoft.UI.Xaml.Automation.Text.TextUnit unit);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider FindAttribute(Windows.Foundation.Int32 attributeId, Windows.Foundation.Object value, Windows.Foundation.Boolean backward);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider FindText(Windows.Foundation.String text, Windows.Foundation.Boolean backward, Windows.Foundation.Boolean ignoreCase);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object GetAttributeValue(Windows.Foundation.Int32 attributeId);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReturnTypeIsOut]
        Windows.Foundation.Double[] GetBoundingRectangles();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple GetEnclosingElement();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.String GetText(Windows.Foundation.Int32 maxLength);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Int32 Move(Microsoft.UI.Xaml.Automation.Text.TextUnit unit, Windows.Foundation.Int32 count);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Int32 MoveEndpointByUnit(Microsoft.UI.Xaml.Automation.Text.TextPatternRangeEndpoint endpoint, Microsoft.UI.Xaml.Automation.Text.TextUnit unit, Windows.Foundation.Int32 count);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void MoveEndpointByRange(Microsoft.UI.Xaml.Automation.Text.TextPatternRangeEndpoint endpoint, Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider textRangeProvider, Microsoft.UI.Xaml.Automation.Text.TextPatternRangeEndpoint targetEndpoint);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Select();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void AddToSelection();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void RemoveFromSelection();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void ScrollIntoView(Windows.Foundation.Boolean alignToTop);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple[] GetChildren();
    }

    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider))]
    public interface ITextRangeProvider2
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void ShowContextMenu();
    }

    public interface ITextProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider DocumentRange
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.SupportedTextSelection SupportedTextSelection
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider[] GetSelection();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider[] GetVisibleRanges();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider RangeFromChild(Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple childElement);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider RangeFromPoint(Windows.Foundation.Point screenLocation);
    }

    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ITextProvider))]
    public interface ITextProvider2
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider RangeFromAnnotation(Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple annotationElement);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider GetCaretRange(out Windows.Foundation.Boolean isActive);
    }

    public interface ITextChildProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple TextContainer
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider TextRange
        {
            get;
        }
    }

    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ITextProvider))]
    [DXamlIdlGroup("coretypes2")]
    public interface ITextEditProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider GetActiveComposition();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.ITextRangeProvider GetConversionTarget();

    };

    public interface IToggleProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.ToggleState ToggleState
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Toggle();
    }

    public interface ITransformProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean CanMove
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean CanResize
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean CanRotate
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Move(Windows.Foundation.Double x, Windows.Foundation.Double y);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Resize(Windows.Foundation.Double width, Windows.Foundation.Double height);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Rotate(Windows.Foundation.Double degrees);
    }

    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.ITransformProvider))]
    public interface ITransformProvider2
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean CanZoom
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double ZoomLevel
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double MaxZoom
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double MinZoom
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Zoom(Windows.Foundation.Double zoom);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void ZoomByUnit(Microsoft.UI.Xaml.Automation.ZoomUnit zoomUnit);
    }

    public interface IValueProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean IsReadOnly
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String Value
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetValue(Windows.Foundation.String value);
    }

    public interface IWindowProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean IsModal
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean IsTopmost
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean Maximizable
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean Minimizable
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.WindowInteractionState InteractionState
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.WindowVisualState VisualState
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Close();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetVisualState(Microsoft.UI.Xaml.Automation.WindowVisualState state);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean WaitForInputIdle(Windows.Foundation.Int32 milliseconds);
    }

    public interface IItemContainerProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple FindItemByProperty([Optional] Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple startAfter, [Optional] Microsoft.UI.Xaml.Automation.AutomationProperty automationProperty, [Optional] Windows.Foundation.Object value);
    }

    public interface IVirtualizedItemProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Realize();
    }

    public interface IAnnotationProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Int32 AnnotationTypeId
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String AnnotationTypeName
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String Author
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String DateTime
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple Target
        {
            get;
        }
    }


    public interface IObjectModelProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object GetUnderlyingObjectModel();
    }

    public interface ISpreadsheetProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple GetItemByName(Windows.Foundation.String name);
    };

    public interface ISpreadsheetItemProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String Formula
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.Provider.IRawElementProviderSimple[] GetAnnotationObjects();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Automation.AnnotationType[] GetAnnotationTypes();
    };

    public interface IStylesProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String ExtendedProperties
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.UI.Color FillColor
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.UI.Color FillPatternColor
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String FillPatternStyle
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String Shape
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Int32 StyleId
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.String StyleName
        {
            get;
        }
    };

    public interface ISynchronizedInputProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void Cancel();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void StartListening(Microsoft.UI.Xaml.Automation.SynchronizedInputType inputType);
    };


    [DXamlIdlGroup("coretypes2")]
    public interface ICustomNavigationProvider
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object NavigateCustom(Microsoft.UI.Xaml.Automation.Peers.AutomationNavigationDirection direction);
    }
}
