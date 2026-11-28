// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include "CustomAutomationPeer.h"
#include <ProviderSideIntegrationTests.h>
#include <TestCleanupWrapper.h>
#include <AutomationClient\AutomationClientManager.h>
#include <UIAutomationHelper.h>
#include <collection.h>
#include <vector>
#include <string>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Tests::Native::External::Automation::Provider;

#pragma warning(push)
#pragma warning(disable: 4100) // Unreferenced parameters are legal for formal interface implementations.

namespace ProviderSideMocks {

    namespace xap = ::Microsoft::UI::Xaml::Automation::Provider;

    // Builds the color directly; Windows::UI::ColorHelper would load Windows.UI.Xaml.dll into the test process.
    inline ::Windows::UI::Color MakeColor(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
    {
        ::Windows::UI::Color color = {};
        color.A = a;
        color.R = r;
        color.G = g;
        color.B = b;
        return color;
    }

    // A peer that derives directly from AutomationPeer and overrides nothing, so every Core call
    // reaches the default AutomationPeer implementation.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class BaseOnlyAutomationPeer sealed : public xaml_automation_peers::AutomationPeer
    {
    public:
        BaseOnlyAutomationPeer() {}
    };

    // A peer that only supplies children, so navigation uses the default NavigateCore implementation.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class NavigationParentPeer sealed : public xaml_automation_peers::AutomationPeer
    {
    public:
        NavigationParentPeer(wfc::IVector<xaml_automation_peers::AutomationPeer^>^ children) : m_children(children) {}

    protected:
        wfc::IVector<xaml_automation_peers::AutomationPeer^>^ GetChildrenCore() override
        {
            return m_children;
        }

    private:
        wfc::IVector<xaml_automation_peers::AutomationPeer^>^ m_children;
    };

    // Button peer that records ShowContextMenu requests.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class ContextMenuRecordingPeer sealed : public xaml_automation_peers::ButtonAutomationPeer
    {
    public:
        ContextMenuRecordingPeer(xaml_controls::Button^ owner) : xaml_automation_peers::ButtonAutomationPeer(owner), m_showContextMenuCount(0) {}

    internal:
        int m_showContextMenuCount;

    protected:
        void ShowContextMenuCore() override
        {
            ++m_showContextMenuCount;
        }
    };

    // Text range used by the Text/TextChild mocks. Only GetText carries a meaningful value.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class LabeledTextRange sealed : public xap::ITextRangeProvider
    {
    public:
        LabeledTextRange(Platform::String^ label) : m_label(label) {}

        virtual xap::ITextRangeProvider^ Clone() { return ref new LabeledTextRange(m_label); }
        virtual bool Compare(xap::ITextRangeProvider^ textRangeProvider) { return textRangeProvider == this; }
        virtual int CompareEndpoints(xaml_automation::Text::TextPatternRangeEndpoint endpoint, xap::ITextRangeProvider^ textRangeProvider, xaml_automation::Text::TextPatternRangeEndpoint targetEndpoint) { return 0; }
        virtual void ExpandToEnclosingUnit(xaml_automation::Text::TextUnit unit) {}
        virtual xap::ITextRangeProvider^ FindAttribute(int attributeId, Platform::Object^ value, bool backward) { return nullptr; }
        virtual xap::ITextRangeProvider^ FindText(Platform::String^ text, bool backward, bool ignoreCase) { return nullptr; }
        virtual Platform::Object^ GetAttributeValue(int attributeId) { return nullptr; }
        virtual void GetBoundingRectangles(Platform::Array<double>^* returnValue) { *returnValue = ref new Platform::Array<double>(0); }
        virtual xap::IRawElementProviderSimple^ GetEnclosingElement() { return nullptr; }
        virtual Platform::String^ GetText(int maxLength) { return m_label; }
        virtual int Move(xaml_automation::Text::TextUnit unit, int count) { return 0; }
        virtual int MoveEndpointByUnit(xaml_automation::Text::TextPatternRangeEndpoint endpoint, xaml_automation::Text::TextUnit unit, int count) { return 0; }
        virtual void MoveEndpointByRange(xaml_automation::Text::TextPatternRangeEndpoint endpoint, xap::ITextRangeProvider^ textRangeProvider, xaml_automation::Text::TextPatternRangeEndpoint targetEndpoint) {}
        virtual void Select() {}
        virtual void AddToSelection() {}
        virtual void RemoveFromSelection() {}
        virtual void ScrollIntoView(bool alignToTop) {}
        virtual Platform::Array<xap::IRawElementProviderSimple^>^ GetChildren() { return ref new Platform::Array<xap::IRawElementProviderSimple^>(0); }

    private:
        Platform::String^ m_label;
    };

    // Button peer that implements the provider interfaces that no built-in control exposes. All values are distinct so a
    // UIA client can tell whether each value was marshalled from the right member.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class PatternMockPeer sealed :
        public xaml_automation_peers::ButtonAutomationPeer,
        public xap::IDockProvider,
        public xap::IGridProvider,
        public xap::IGridItemProvider,
        public xap::ITableProvider,
        public xap::IMultipleViewProvider,
        public xap::ITransformProvider2,
        public xap::IVirtualizedItemProvider,
        public xap::IWindowProvider,
        public xap::ISynchronizedInputProvider,
        public xap::ISpreadsheetProvider,
        public xap::ISpreadsheetItemProvider,
        public xap::IStylesProvider,
        public xap::IObjectModelProvider
    {
    public:
        PatternMockPeer(xaml_controls::Button^ owner) : xaml_automation_peers::ButtonAutomationPeer(owner), m_realizeCount(0), m_closeCount(0), m_cancelCount(0) {}

        // IDockProvider
        virtual property xaml_automation::DockPosition DockPosition { xaml_automation::DockPosition get() { return xaml_automation::DockPosition::Left; } }
        virtual void SetDockPosition(xaml_automation::DockPosition dockPosition) { m_setDockPositions.push_back(dockPosition); }

        // IGridProvider
        virtual property int ColumnCount { int get() { return 3; } }
        virtual property int RowCount { int get() { return 5; } }
        virtual xap::IRawElementProviderSimple^ GetItem(int row, int column)
        {
            m_getItemRequests.push_back(std::make_pair(row, column));
            return ProviderFromPeer(m_cells.at(1));
        }

        // IGridItemProvider
        virtual property int Column { int get() { return 7; } }
        virtual property int ColumnSpan { int get() { return 3; } }
        virtual property int Row { int get() { return 6; } }
        virtual property int RowSpan { int get() { return 2; } }
        virtual property xap::IRawElementProviderSimple^ ContainingGrid { xap::IRawElementProviderSimple^ get() { return ProviderFromPeer(m_cells.at(2)); } }

        // ITableProvider
        virtual property xaml_automation::RowOrColumnMajor RowOrColumnMajor { xaml_automation::RowOrColumnMajor get() { return xaml_automation::RowOrColumnMajor::ColumnMajor; } }
        virtual Platform::Array<xap::IRawElementProviderSimple^>^ GetColumnHeaders() { return MakeProviders(0, 1); }
        virtual Platform::Array<xap::IRawElementProviderSimple^>^ GetRowHeaders() { return MakeProviders(2, 0); }

        // IMultipleViewProvider
        virtual property int CurrentView { int get() { return 9; } }
        virtual Platform::Array<int>^ GetSupportedViews()
        {
            auto views = ref new Platform::Array<int>(2);
            views[0] = 7;
            views[1] = 9;
            return views;
        }
        virtual Platform::String^ GetViewName(int viewId)
        {
            m_viewNameRequests.push_back(viewId);
            return viewId == 7 ? L"Seven" : (viewId == 9 ? L"Nine" : L"Unknown");
        }
        virtual void SetCurrentView(int viewId) { m_setCurrentViews.push_back(viewId); }

        // ITransformProvider
        virtual property bool CanMove { bool get() { return true; } }
        virtual property bool CanResize { bool get() { return false; } }
        virtual property bool CanRotate { bool get() { return true; } }
        virtual void Move(double x, double y) { m_moveRequests.push_back(std::make_pair(x, y)); }
        virtual void Resize(double width, double height) { m_resizeRequests.push_back(std::make_pair(width, height)); }
        virtual void Rotate(double degrees) { m_rotateRequests.push_back(degrees); }

        // ITransformProvider2
        virtual property bool CanZoom { bool get() { return true; } }
        virtual property double ZoomLevel { double get() { return 1.5; } }
        virtual property double MaxZoom { double get() { return 4.0; } }
        virtual property double MinZoom { double get() { return 0.5; } }
        virtual void Zoom(double zoom) { m_zoomRequests.push_back(zoom); }
        virtual void ZoomByUnit(xaml_automation::ZoomUnit zoomUnit) { m_zoomByUnitRequests.push_back(zoomUnit); }

        // IVirtualizedItemProvider
        virtual void Realize() { ++m_realizeCount; }

        // IWindowProvider
        virtual property bool IsModal { bool get() { return true; } }
        virtual property bool IsTopmost { bool get() { return false; } }
        virtual property bool Maximizable { bool get() { return true; } }
        virtual property bool Minimizable { bool get() { return false; } }
        virtual property xaml_automation::WindowInteractionState InteractionState { xaml_automation::WindowInteractionState get() { return xaml_automation::WindowInteractionState::BlockedByModalWindow; } }
        virtual property xaml_automation::WindowVisualState VisualState { xaml_automation::WindowVisualState get() { return xaml_automation::WindowVisualState::Maximized; } }
        virtual void Close() { ++m_closeCount; }
        virtual void SetVisualState(xaml_automation::WindowVisualState state) { m_setVisualStates.push_back(state); }
        virtual bool WaitForInputIdle(int milliseconds)
        {
            m_waitForInputIdleRequests.push_back(milliseconds);
            // Not observable by the client today: CManagedWindowProvider::WaitForInputIdle (UIAPatternProvider.cpp) never writes
            // the provider's result to its out-parameter (product concern PC-2 of PR #12805, pending a bug).
            return true;
        }

        // ISynchronizedInputProvider
        virtual void Cancel() { ++m_cancelCount; }
        virtual void StartListening(xaml_automation::SynchronizedInputType inputType) { m_startListeningRequests.push_back(inputType); }

        // ISpreadsheetProvider
        virtual xap::IRawElementProviderSimple^ GetItemByName(Platform::String^ name)
        {
            m_itemByNameRequests.push_back(name ? std::wstring(name->Data()) : std::wstring());
            if (Platform::String::CompareOrdinal(name, L"B2") == 0)
            {
                return ProviderFromPeer(m_cells.at(1));
            }
            return nullptr;
        }

        // ISpreadsheetItemProvider
        virtual property Platform::String^ Formula { Platform::String^ get() { return L"=A1+1"; } }
        virtual Platform::Array<xap::IRawElementProviderSimple^>^ GetAnnotationObjects() { return MakeProviders(0, 2); }
        virtual Platform::Array<xaml_automation::AnnotationType>^ GetAnnotationTypes()
        {
            auto types = ref new Platform::Array<xaml_automation::AnnotationType>(2);
            types[0] = xaml_automation::AnnotationType::Comment;
            types[1] = xaml_automation::AnnotationType::SpellingError;
            return types;
        }

        // IStylesProvider
        virtual property Platform::String^ ExtendedProperties { Platform::String^ get() { return L"ext=1"; } }
        virtual property ::Windows::UI::Color FillColor { ::Windows::UI::Color get() { return MakeColor(0x7F, 0x12, 0x34, 0x56); } }
        virtual property ::Windows::UI::Color FillPatternColor { ::Windows::UI::Color get() { return MakeColor(0x40, 0xAB, 0xCD, 0xEF); } }
        virtual property Platform::String^ FillPatternStyle { Platform::String^ get() { return L"dots"; } }
        virtual property Platform::String^ Shape { Platform::String^ get() { return L"circle"; } }
        virtual property int StyleId { int get() { return 70010; } }
        virtual property Platform::String^ StyleName { Platform::String^ get() { return L"Custom Style"; } }

        // IObjectModelProvider
        virtual Platform::Object^ GetUnderlyingObjectModel() { return m_objectModel; }

    internal:
        std::vector<xaml_automation_peers::AutomationPeer^> m_cells;
        Platform::Object^ m_objectModel;
        std::vector<xaml_automation::DockPosition> m_setDockPositions;
        std::vector<std::pair<int, int>> m_getItemRequests;
        std::vector<int> m_viewNameRequests;
        std::vector<int> m_setCurrentViews;
        std::vector<std::pair<double, double>> m_moveRequests;
        std::vector<std::pair<double, double>> m_resizeRequests;
        std::vector<double> m_rotateRequests;
        std::vector<double> m_zoomRequests;
        std::vector<xaml_automation::ZoomUnit> m_zoomByUnitRequests;
        int m_realizeCount;
        int m_closeCount;
        std::vector<xaml_automation::WindowVisualState> m_setVisualStates;
        std::vector<int> m_waitForInputIdleRequests;
        int m_cancelCount;
        std::vector<xaml_automation::SynchronizedInputType> m_startListeningRequests;
        std::vector<std::wstring> m_itemByNameRequests;

    protected:
        Platform::Object^ GetPatternCore(xaml_automation_peers::PatternInterface patternInterface) override
        {
            switch (patternInterface)
            {
            case xaml_automation_peers::PatternInterface::Dock:
            case xaml_automation_peers::PatternInterface::Grid:
            case xaml_automation_peers::PatternInterface::GridItem:
            case xaml_automation_peers::PatternInterface::Table:
            case xaml_automation_peers::PatternInterface::MultipleView:
            case xaml_automation_peers::PatternInterface::Transform:
            case xaml_automation_peers::PatternInterface::Transform2:
            case xaml_automation_peers::PatternInterface::VirtualizedItem:
            case xaml_automation_peers::PatternInterface::Window:
            case xaml_automation_peers::PatternInterface::SynchronizedInput:
            case xaml_automation_peers::PatternInterface::Spreadsheet:
            case xaml_automation_peers::PatternInterface::SpreadsheetItem:
            case xaml_automation_peers::PatternInterface::Styles:
            case xaml_automation_peers::PatternInterface::ObjectModel:
                return this;
            default:
                return __super::GetPatternCore(patternInterface);
            }
        }

    private:
        Platform::Array<xap::IRawElementProviderSimple^>^ MakeProviders(size_t first, size_t second)
        {
            auto providers = ref new Platform::Array<xap::IRawElementProviderSimple^>(2);
            providers[0] = ProviderFromPeer(m_cells.at(first));
            providers[1] = ProviderFromPeer(m_cells.at(second));
            return providers;
        }
    };

    // Button peer that implements the Drag pattern with a non-empty grabbed item list.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class DragMockPeer sealed : public xaml_automation_peers::ButtonAutomationPeer, public xap::IDragProvider
    {
    public:
        DragMockPeer(xaml_controls::Button^ owner) : xaml_automation_peers::ButtonAutomationPeer(owner) {}

        virtual property bool IsGrabbed { bool get() { return true; } }
        virtual property Platform::String^ DropEffect { Platform::String^ get() { return L"copy"; } }
        virtual property Platform::Array<Platform::String^>^ DropEffects { Platform::Array<Platform::String^>^ get() { return ref new Platform::Array<Platform::String^>(0); } }
        virtual Platform::Array<xap::IRawElementProviderSimple^>^ GetGrabbedItems()
        {
            auto providers = ref new Platform::Array<xap::IRawElementProviderSimple^>(2);
            providers[0] = ProviderFromPeer(m_cells.at(0));
            providers[1] = ProviderFromPeer(m_cells.at(1));
            return providers;
        }

    internal:
        std::vector<xaml_automation_peers::AutomationPeer^> m_cells;

    protected:
        Platform::Object^ GetPatternCore(xaml_automation_peers::PatternInterface patternInterface) override
        {
            if (patternInterface == xaml_automation_peers::PatternInterface::Drag)
            {
                return this;
            }
            return __super::GetPatternCore(patternInterface);
        }
    };

    // Button peer that implements Text2 (and so Text), TextChild and Annotation.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class TextMockPeer sealed :
        public xaml_automation_peers::ButtonAutomationPeer,
        public xap::ITextProvider2,
        public xap::ITextChildProvider,
        public xap::IAnnotationProvider
    {
    public:
        TextMockPeer(xaml_controls::Button^ owner) : xaml_automation_peers::ButtonAutomationPeer(owner), m_rangeFromPointCount(0), m_textContainerCount(0), m_returnNoRanges(false) {}

        // ITextProvider
        virtual Platform::Array<xap::ITextRangeProvider^>^ GetSelection() { return ref new Platform::Array<xap::ITextRangeProvider^>(0); }
        virtual Platform::Array<xap::ITextRangeProvider^>^ GetVisibleRanges() { return ref new Platform::Array<xap::ITextRangeProvider^>(0); }
        virtual xap::ITextRangeProvider^ RangeFromChild(xap::IRawElementProviderSimple^ childElement)
        {
            m_rangeFromChildPeers.push_back(PeerFromProvider(childElement));
            return ref new LabeledTextRange(L"child");
        }
        virtual xap::ITextRangeProvider^ RangeFromPoint(::Windows::Foundation::Point screenLocation)
        {
            ++m_rangeFromPointCount;
            return nullptr;
        }
        virtual property xap::ITextRangeProvider^ DocumentRange { xap::ITextRangeProvider^ get() { return ref new LabeledTextRange(L"document"); } }
        virtual property xaml_automation::SupportedTextSelection SupportedTextSelection { xaml_automation::SupportedTextSelection get() { return xaml_automation::SupportedTextSelection::Single; } }

        // ITextProvider2
        virtual xap::ITextRangeProvider^ RangeFromAnnotation(xap::IRawElementProviderSimple^ annotationElement)
        {
            m_rangeFromAnnotationPeers.push_back(PeerFromProvider(annotationElement));
            return m_returnNoRanges ? nullptr : ref new LabeledTextRange(L"annotation");
        }
        virtual xap::ITextRangeProvider^ GetCaretRange(bool* isActive)
        {
            *isActive = false;
            return nullptr;
        }

        // ITextChildProvider
        virtual property xap::IRawElementProviderSimple^ TextContainer
        {
            xap::IRawElementProviderSimple^ get()
            {
                ++m_textContainerCount;
                return ProviderFromPeer(m_cells.at(0));
            }
        }
        virtual property xap::ITextRangeProvider^ TextRange { xap::ITextRangeProvider^ get() { return m_returnNoRanges ? nullptr : ref new LabeledTextRange(L"textchild"); } }

        // IAnnotationProvider
        virtual property int AnnotationTypeId { int get() { return static_cast<int>(xaml_automation::AnnotationType::Comment); } }
        virtual property Platform::String^ AnnotationTypeName { Platform::String^ get() { return L"Comment"; } }
        virtual property Platform::String^ Author { Platform::String^ get() { return L"Ann"; } }
        virtual property Platform::String^ DateTime { Platform::String^ get() { return L"2026-01-02"; } }
        virtual property xap::IRawElementProviderSimple^ Target { xap::IRawElementProviderSimple^ get() { return ProviderFromPeer(m_cells.at(2)); } }

    internal:
        std::vector<xaml_automation_peers::AutomationPeer^> m_cells;
        std::vector<xaml_automation_peers::AutomationPeer^> m_rangeFromChildPeers;
        std::vector<xaml_automation_peers::AutomationPeer^> m_rangeFromAnnotationPeers;
        int m_rangeFromPointCount;
        int m_textContainerCount;
        // When set, RangeFromAnnotation and TextRange report that there is no range.
        bool m_returnNoRanges;

    protected:
        Platform::Object^ GetPatternCore(xaml_automation_peers::PatternInterface patternInterface) override
        {
            switch (patternInterface)
            {
            case xaml_automation_peers::PatternInterface::Text:
            case xaml_automation_peers::PatternInterface::Text2:
            case xaml_automation_peers::PatternInterface::TextChild:
            case xaml_automation_peers::PatternInterface::Annotation:
                return this;
            default:
                return __super::GetPatternCore(patternInterface);
            }
        }
    };

    // Button peer that implements ITextProvider only, but answers both Text and Text2 pattern requests.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class TextOnlyMockPeer sealed : public xaml_automation_peers::ButtonAutomationPeer, public xap::ITextProvider
    {
    public:
        TextOnlyMockPeer(xaml_controls::Button^ owner) : xaml_automation_peers::ButtonAutomationPeer(owner) {}

        virtual Platform::Array<xap::ITextRangeProvider^>^ GetSelection() { return ref new Platform::Array<xap::ITextRangeProvider^>(0); }
        virtual Platform::Array<xap::ITextRangeProvider^>^ GetVisibleRanges() { return ref new Platform::Array<xap::ITextRangeProvider^>(0); }
        virtual xap::ITextRangeProvider^ RangeFromChild(xap::IRawElementProviderSimple^ childElement) { return nullptr; }
        virtual xap::ITextRangeProvider^ RangeFromPoint(::Windows::Foundation::Point screenLocation) { return nullptr; }
        virtual property xap::ITextRangeProvider^ DocumentRange { xap::ITextRangeProvider^ get() { return ref new LabeledTextRange(L"textonly"); } }
        virtual property xaml_automation::SupportedTextSelection SupportedTextSelection { xaml_automation::SupportedTextSelection get() { return xaml_automation::SupportedTextSelection::None; } }

    protected:
        Platform::Object^ GetPatternCore(xaml_automation_peers::PatternInterface patternInterface) override
        {
            if (patternInterface == xaml_automation_peers::PatternInterface::Text ||
                patternInterface == xaml_automation_peers::PatternInterface::Text2)
            {
                return this;
            }
            return __super::GetPatternCore(patternInterface);
        }
    };

    enum class MockPeerKind
    {
        Pattern,
        Drag,
        Text,
        TextOnly,
        ContextMenu,
    };

    // A Button whose automation peer is one of the mocks above.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class MockPeerHostButton sealed : public xaml_controls::Button
    {
    internal:
        MockPeerHostButton(MockPeerKind kind) : m_kind(kind) {}

    protected:
        xaml_automation_peers::AutomationPeer^ OnCreateAutomationPeer() override
        {
            switch (m_kind)
            {
            case MockPeerKind::Pattern: return ref new PatternMockPeer(this);
            case MockPeerKind::Drag: return ref new DragMockPeer(this);
            case MockPeerKind::Text: return ref new TextMockPeer(this);
            case MockPeerKind::TextOnly: return ref new TextOnlyMockPeer(this);
            default: return ref new ContextMenuRecordingPeer(this);
            }
        }

    private:
        MockPeerKind m_kind;
    };

    // App peer derived from the public CalendarDatePickerAutomationPeer, which makes the factory aggregate the XAML peer.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    ref class DerivedCalendarDatePickerAutomationPeer sealed : public xaml_automation_peers::CalendarDatePickerAutomationPeer
    {
    public:
        DerivedCalendarDatePickerAutomationPeer(xaml_controls::CalendarDatePicker^ owner) : xaml_automation_peers::CalendarDatePickerAutomationPeer(owner) {}

    protected:
        Platform::String^ GetClassNameCore() override { return L"DerivedCalendarDatePicker"; }
    };
}

#pragma warning(pop)

using namespace ProviderSideMocks;

// windows.h maps GetClassName to GetClassNameW, which hides AutomationPeer::GetClassName.
#ifdef GetClassName
#undef GetClassName
#endif

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Provider {

    bool ProviderSideIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();

        return true;
    }
     
    bool ProviderSideIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ProviderSideIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ProviderSideIntegrationTests::VerifyLocalizedControlType()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            CustomControl^ control = ref new CustomControl();
            CustomAutomationPeer^ customAP = static_cast<CustomAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(control));

            const ControlTypeData controlTypes[] =
            {
                // String               Expected role
                { xaml_automation_peers::AutomationControlType::Button, L"button" },
                { xaml_automation_peers::AutomationControlType::Calendar, L"calendar" },
                { xaml_automation_peers::AutomationControlType::CheckBox, L"check box" },
                { xaml_automation_peers::AutomationControlType::ComboBox, L"combo box" },
                { xaml_automation_peers::AutomationControlType::Edit, L"edit" },
                { xaml_automation_peers::AutomationControlType::Hyperlink, L"link" },
                { xaml_automation_peers::AutomationControlType::Image, L"image" },
                { xaml_automation_peers::AutomationControlType::ListItem, L"list item" },
                { xaml_automation_peers::AutomationControlType::List, L"list" },
                { xaml_automation_peers::AutomationControlType::Menu, L"menu" },
                { xaml_automation_peers::AutomationControlType::MenuBar, L"menu bar" },
                { xaml_automation_peers::AutomationControlType::MenuItem, L"menu item" },
                { xaml_automation_peers::AutomationControlType::ProgressBar, L"progress bar" },
                { xaml_automation_peers::AutomationControlType::RadioButton, L"radio button" },
                { xaml_automation_peers::AutomationControlType::ScrollBar, L"scroll bar" },
                { xaml_automation_peers::AutomationControlType::Slider, L"slider" },
                { xaml_automation_peers::AutomationControlType::Spinner, L"spinner" },
                { xaml_automation_peers::AutomationControlType::StatusBar, L"status bar" },
                { xaml_automation_peers::AutomationControlType::Tab, L"tab" },
                { xaml_automation_peers::AutomationControlType::TabItem, L"tab item" },
                { xaml_automation_peers::AutomationControlType::Text, L"text" },
                { xaml_automation_peers::AutomationControlType::ToolBar, L"tool bar" },
                { xaml_automation_peers::AutomationControlType::ToolTip, L"tool tip" },
                { xaml_automation_peers::AutomationControlType::Tree, L"tree" },
                { xaml_automation_peers::AutomationControlType::TreeItem, L"tree item" },
                { xaml_automation_peers::AutomationControlType::Custom, L"custom" },
                { xaml_automation_peers::AutomationControlType::Group, L"group" },
                { xaml_automation_peers::AutomationControlType::Thumb, L"thumb" },
                { xaml_automation_peers::AutomationControlType::DataGrid, L"data grid" },
                { xaml_automation_peers::AutomationControlType::DataItem, L"data item" },
                { xaml_automation_peers::AutomationControlType::Document, L"document" },
                { xaml_automation_peers::AutomationControlType::SplitButton, L"split button" },
                { xaml_automation_peers::AutomationControlType::Window, L"window" },
                { xaml_automation_peers::AutomationControlType::Pane, L"pane" },
                { xaml_automation_peers::AutomationControlType::Header, L"header" },
                { xaml_automation_peers::AutomationControlType::HeaderItem, L"header item" },
                { xaml_automation_peers::AutomationControlType::Table, L"table" },
                { xaml_automation_peers::AutomationControlType::TitleBar, L"title bar" },
                { xaml_automation_peers::AutomationControlType::Separator, L"separator" },
                { xaml_automation_peers::AutomationControlType::SemanticZoom, L"semantic zoom" },
                { xaml_automation_peers::AutomationControlType::AppBar, L"app bar" },
            };

            int controlTypesSize = sizeof(controlTypes) / sizeof(ControlTypeData);
            for (int i = 0; i < controlTypesSize; i++)
            {
                customAP->SetControlType(controlTypes[i].controlType);
                Platform::String^ localizedControlType = customAP->GetLocalizedControlType();
                VERIFY_IS_TRUE(!Platform::String::CompareOrdinal(localizedControlType, controlTypes[i].localizedControlType));
            }
        });
    }

    namespace
    {
        bool AreSameObject(Platform::Object^ first, Platform::Object^ second)
        {
            if (first == nullptr || second == nullptr)
            {
                return first == nullptr && second == nullptr;
            }

            wrl::ComPtr<IUnknown> firstIdentity;
            wrl::ComPtr<IUnknown> secondIdentity;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(first)->QueryInterface(IID_PPV_ARGS(&firstIdentity)));
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(second)->QueryInterface(IID_PPV_ARGS(&secondIdentity)));
            return firstIdentity.Get() == secondIdentity.Get();
        }

        void VerifyIsEmptyString(Platform::String^ value, const wchar_t* description)
        {
            LOG_OUTPUT(L"%s returned \"%s\".", description, value ? value->Data() : L"<null>");
            VERIFY_IS_TRUE(value == nullptr || value->IsEmpty(), description);
        }

        void VerifyStringEquals(const wchar_t* expected, Platform::String^ actual)
        {
            LOG_OUTPUT(L"Expected \"%s\", actual \"%s\".", expected, actual ? actual->Data() : L"<null>");
            VERIFY_ARE_EQUAL(0, Platform::String::CompareOrdinal(ref new Platform::String(expected), actual));
        }

        void VerifyBstrEquals(const wchar_t* expected, AutoBSTR& actual)
        {
            VERIFY_IS_TRUE(static_cast<bool>(actual), L"Expected a non-null BSTR.");
            AutoBSTR::VerifyAreEqual(expected, actual);
        }

        void VerifyElementName(IUIAutomationElement* element, const wchar_t* expectedName)
        {
            VERIFY_IS_NOT_NULL(element);
            AutoBSTR name;
            VERIFY_SUCCEEDED(element->get_CurrentName(name.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(expectedName, name);
        }

        void VerifyElementArrayNames(IUIAutomationElementArray* elements, std::initializer_list<const wchar_t*> expectedNames)
        {
            VERIFY_IS_NOT_NULL(elements);
            int length = 0;
            VERIFY_SUCCEEDED(elements->get_Length(&length));
            VERIFY_ARE_EQUAL(static_cast<int>(expectedNames.size()), length);

            int index = 0;
            for (auto expectedName : expectedNames)
            {
                wrl::ComPtr<IUIAutomationElement> element;
                VERIFY_SUCCEEDED(elements->GetElement(index++, &element));
                VerifyElementName(element.Get(), expectedName);
            }
        }

        void VerifyRangeText(IUIAutomationTextRange* range, const wchar_t* expectedText)
        {
            VERIFY_IS_NOT_NULL(range);
            AutoBSTR text;
            VERIFY_SUCCEEDED(range->GetText(-1, text.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(expectedText, text);
        }

        std::vector<int> ReadIntSafeArray(SAFEARRAY* values)
        {
            std::vector<int> result;
            if (values != nullptr)
            {
                LONG lower = 0;
                LONG upper = -1;
                VERIFY_SUCCEEDED(SafeArrayGetLBound(values, 1, &lower));
                VERIFY_SUCCEEDED(SafeArrayGetUBound(values, 1, &upper));
                for (LONG i = lower; i <= upper; ++i)
                {
                    int value = 0;
                    VERIFY_SUCCEEDED(SafeArrayGetElement(values, &i, &value));
                    result.push_back(value);
                }
            }
            return result;
        }

        LONG GetSafeArrayCount(SAFEARRAY* values)
        {
            if (values == nullptr)
            {
                return 0;
            }
            LONG lower = 0;
            LONG upper = -1;
            VERIFY_SUCCEEDED(SafeArrayGetLBound(values, 1, &lower));
            VERIFY_SUCCEEDED(SafeArrayGetUBound(values, 1, &upper));
            return upper - lower + 1;
        }

        // Must be called on the UIA client thread.
        wrl::ComPtr<IUIAutomationElement> FindUiaElementByName(const wchar_t* name)
        {
            Automation::AutomationClient::UIAElementInfo info = {};
            info.m_Name = name;
            auto clientManager = Automation::AutomationClient::AutomationClientManager::CreateAutomationClientManagerFromInfo(info);
            wrl::ComPtr<IUIAutomationElement> element;
            clientManager->GetCurrentUIAutomationElement(&element);
            VERIFY_IS_NOT_NULL(element.Get());
            return element;
        }

        template <typename TPattern>
        wrl::ComPtr<TPattern> GetUiaPattern(IUIAutomationElement* element, PATTERNID patternId)
        {
            wrl::ComPtr<TPattern> pattern;
            VERIFY_SUCCEEDED(element->GetCurrentPatternAs(patternId, __uuidof(TPattern), &pattern));
            VERIFY_IS_NOT_NULL(pattern.Get());
            return pattern;
        }

        struct MockHostTree
        {
            std::vector<MockPeerHostButton^> hosts;
            std::vector<xaml_controls::Button^> cells;
        };

        // Must be called on the UI thread. Creates the mock hosts followed by the buttons CellA, CellB and CellC.
        MockHostTree CreateMockHostTree(std::initializer_list<std::pair<MockPeerKind, const wchar_t*>> hosts)
        {
            MockHostTree tree;
            auto root = ref new xaml_controls::StackPanel();

            for (const auto& host : hosts)
            {
                auto hostButton = ref new MockPeerHostButton(host.first);
                hostButton->Content = ref new Platform::String(host.second);
                xaml_automation::AutomationProperties::SetName(hostButton, ref new Platform::String(host.second));
                root->Children->Append(hostButton);
                tree.hosts.push_back(hostButton);
            }

            for (auto cellName : { L"CellA", L"CellB", L"CellC" })
            {
                auto cell = ref new xaml_controls::Button();
                cell->Content = ref new Platform::String(cellName);
                xaml_automation::AutomationProperties::SetName(cell, ref new Platform::String(cellName));
                root->Children->Append(cell);
                tree.cells.push_back(cell);
            }

            TestServices::WindowHelper->WindowContent = root;
            return tree;
        }

        // Must be called on the UI thread.
        std::vector<xaml_automation_peers::AutomationPeer^> CreateCellPeers(const MockHostTree& tree)
        {
            std::vector<xaml_automation_peers::AutomationPeer^> peers;
            for (auto cell : tree.cells)
            {
                auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(cell);
                VERIFY_IS_NOT_NULL(peer);
                peers.push_back(peer);
            }
            return peers;
        }
    }

    void ProviderSideIntegrationTests::VerifyBaseAutomationPeerDefaultCoreValues()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto peer = ref new BaseOnlyAutomationPeer();

            VerifyIsEmptyString(peer->GetName(), L"GetName");
            VerifyIsEmptyString(peer->GetAutomationId(), L"GetAutomationId");
            VerifyIsEmptyString(peer->GetClassName(), L"GetClassName");
            VerifyIsEmptyString(peer->GetHelpText(), L"GetHelpText");
            VerifyIsEmptyString(peer->GetItemStatus(), L"GetItemStatus");
            VerifyIsEmptyString(peer->GetItemType(), L"GetItemType");
            VerifyIsEmptyString(peer->GetAcceleratorKey(), L"GetAcceleratorKey");
            VerifyIsEmptyString(peer->GetAccessKey(), L"GetAccessKey");
            VerifyIsEmptyString(peer->GetLocalizedLandmarkType(), L"GetLocalizedLandmarkType");

            VERIFY_ARE_EQUAL(static_cast<int>(xaml_automation_peers::AutomationControlType::Custom), static_cast<int>(peer->GetAutomationControlType()));
            VerifyStringEquals(L"custom", peer->GetLocalizedControlType());

            const auto bounds = peer->GetBoundingRectangle();
            VERIFY_ARE_EQUAL(0.0f, bounds.X);
            VERIFY_ARE_EQUAL(0.0f, bounds.Y);
            VERIFY_ARE_EQUAL(0.0f, bounds.Width);
            VERIFY_ARE_EQUAL(0.0f, bounds.Height);

            const auto clickablePoint = peer->GetClickablePoint();
            VERIFY_ARE_EQUAL(0.0f, clickablePoint.X);
            VERIFY_ARE_EQUAL(0.0f, clickablePoint.Y);

            VERIFY_IS_NULL(peer->GetLabeledBy());
            VERIFY_ARE_EQUAL(static_cast<int>(xaml_automation_peers::AutomationLiveSetting::Off), static_cast<int>(peer->GetLiveSetting()));
            VERIFY_ARE_EQUAL(static_cast<int>(xaml_automation_peers::AutomationLandmarkType::None), static_cast<int>(peer->GetLandmarkType()));
            VERIFY_ARE_EQUAL(-1, peer->GetPositionInSet());
            VERIFY_ARE_EQUAL(-1, peer->GetSizeOfSet());
            VERIFY_ARE_EQUAL(-1, peer->GetLevel());

            VERIFY_IS_TRUE(peer->IsEnabled());
            VERIFY_IS_FALSE(peer->IsContentElement());
            VERIFY_IS_FALSE(peer->IsControlElement());
            VERIFY_IS_FALSE(peer->IsKeyboardFocusable());
            VERIFY_IS_FALSE(peer->IsOffscreen());
            VERIFY_IS_FALSE(peer->IsRequiredForForm());

            auto controlledPeers = peer->GetControlledPeers();
            VERIFY_IS_TRUE(controlledPeers == nullptr || controlledPeers->Size == 0);
            auto annotations = peer->GetAnnotations();
            VERIFY_IS_TRUE(annotations == nullptr || annotations->Size == 0);

            // The default ShowContextMenu does nothing and must not fail.
            peer->ShowContextMenu();

            VERIFY_IS_NULL(peer->EventsSource);
            auto eventsSource = ref new BaseOnlyAutomationPeer();
            peer->EventsSource = eventsSource;
            VERIFY_IS_TRUE(AreSameObject(peer->EventsSource, eventsSource));
        });
    }

    void ProviderSideIntegrationTests::VerifyDefaultNavigateCoreWithCustomChildren()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            using Direction = xaml_automation_peers::AutomationNavigationDirection;

            std::vector<xaml_controls::Button^> owners;
            std::vector<xaml_automation_peers::AutomationPeer^> childPeers;
            auto children = ref new Platform::Collections::Vector<xaml_automation_peers::AutomationPeer^>();
            for (auto name : { L"First", L"Middle", L"Last" })
            {
                auto button = ref new xaml_controls::Button();
                button->Content = ref new Platform::String(name);
                auto childPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(button);
                owners.push_back(button);
                childPeers.push_back(childPeer);
                children->Append(childPeer);
            }

            auto parent = ref new NavigationParentPeer(children);

            VERIFY_IS_TRUE(AreSameObject(parent->Navigate(Direction::FirstChild), childPeers[0]));
            VERIFY_IS_TRUE(AreSameObject(parent->Navigate(Direction::LastChild), childPeers[2]));

            // Enumerating the children makes the custom peer their parent.
            VERIFY_IS_TRUE(AreSameObject(childPeers[1]->Navigate(Direction::Parent), parent));

            VERIFY_IS_TRUE(AreSameObject(childPeers[2]->Navigate(Direction::PreviousSibling), childPeers[1]));
            VERIFY_IS_TRUE(AreSameObject(childPeers[1]->Navigate(Direction::PreviousSibling), childPeers[0]));
            VERIFY_IS_NULL(childPeers[0]->Navigate(Direction::PreviousSibling));
            VERIFY_IS_TRUE(AreSameObject(childPeers[0]->Navigate(Direction::NextSibling), childPeers[1]));
            VERIFY_IS_TRUE(AreSameObject(childPeers[1]->Navigate(Direction::NextSibling), childPeers[2]));
            VERIFY_IS_NULL(childPeers[2]->Navigate(Direction::NextSibling));

            auto childlessParent = ref new NavigationParentPeer(nullptr);
            VERIFY_IS_NULL(childlessParent->Navigate(Direction::FirstChild));
            VERIFY_IS_NULL(childlessParent->Navigate(Direction::LastChild));

            bool unsupportedDirectionFailed = false;
            try
            {
                parent->Navigate(static_cast<Direction>(99));
            }
            catch (Platform::Exception^ e)
            {
                LOG_OUTPUT(L"Navigate with an unsupported direction failed with 0x%08x.", e->HResult);
                unsupportedDirectionFailed = true;
            }
            VERIFY_IS_TRUE(unsupportedDirectionFailed);
        });
    }

    void ProviderSideIntegrationTests::VerifyLabeledByControlledPeersAndShowContextMenuThroughUia()
    {
        TestCleanupWrapper cleanup;
        MockPeerHostButton^ contextMenuHost = nullptr;
        ContextMenuRecordingPeer^ contextMenuPeer = nullptr;
        auto releaseOnUIThread = wil::scope_exit([&]()
        {
            RunOnUIThread([&]()
            {
                contextMenuHost = nullptr;
                contextMenuPeer = nullptr;
            });
        });

        RunOnUIThread([&]()
        {
            auto root = ref new xaml_controls::StackPanel();

            auto label = ref new xaml_controls::TextBlock();
            label->Text = L"Label42";
            root->Children->Append(label);

            auto labeled = ref new xaml_controls::Button();
            labeled->Content = ref new Platform::String(L"Labeled");
            xaml_automation::AutomationProperties::SetName(labeled, L"LabeledButton");
            xaml_automation::AutomationProperties::SetLabeledBy(labeled, label);
            root->Children->Append(labeled);

            auto controlledA = ref new xaml_controls::Button();
            controlledA->Content = ref new Platform::String(L"ControlledA");
            xaml_automation::AutomationProperties::SetName(controlledA, L"ControlledA");
            auto controlledB = ref new xaml_controls::Button();
            controlledB->Content = ref new Platform::String(L"ControlledB");
            xaml_automation::AutomationProperties::SetName(controlledB, L"ControlledB");
            auto controller = ref new xaml_controls::Button();
            controller->Content = ref new Platform::String(L"Controller");
            xaml_automation::AutomationProperties::SetName(controller, L"ControllerButton");
            xaml_automation::AutomationProperties::GetControlledPeers(controller)->Append(controlledA);
            xaml_automation::AutomationProperties::GetControlledPeers(controller)->Append(controlledB);
            root->Children->Append(controller);
            root->Children->Append(controlledA);
            root->Children->Append(controlledB);

            contextMenuHost = ref new MockPeerHostButton(MockPeerKind::ContextMenu);
            contextMenuHost->Content = ref new Platform::String(L"ContextMenu");
            xaml_automation::AutomationProperties::SetName(contextMenuHost, L"ContextMenuButton");
            root->Children->Append(contextMenuHost);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            contextMenuPeer = static_cast<ContextMenuRecordingPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(contextMenuHost));
            VERIFY_IS_NOT_NULL(contextMenuPeer);
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            LOG_OUTPUT(L"LabeledBy is reported as the label's UIA element.");
            auto labeledElement = FindUiaElementByName(L"LabeledButton");
            wrl::ComPtr<IUIAutomationElement> labelElement;
            VERIFY_SUCCEEDED(labeledElement->get_CurrentLabeledBy(&labelElement));
            VerifyElementName(labelElement.Get(), L"Label42");

            LOG_OUTPUT(L"ControlledPeers are reported as ControllerFor, in order.");
            auto controllerElement = FindUiaElementByName(L"ControllerButton");
            wrl::ComPtr<IUIAutomationElementArray> controlledElements;
            VERIFY_SUCCEEDED(controllerElement->get_CurrentControllerFor(&controlledElements));
            VerifyElementArrayNames(controlledElements.Get(), { L"ControlledA", L"ControlledB" });

            LOG_OUTPUT(L"ShowContextMenu reaches the peer's ShowContextMenuCore.");
            auto contextMenuElement = FindUiaElementByName(L"ContextMenuButton");
            wrl::ComPtr<IUIAutomationElement3> contextMenuElement3;
            VERIFY_SUCCEEDED(contextMenuElement.As(&contextMenuElement3));
            VERIFY_SUCCEEDED(contextMenuElement3->ShowContextMenu());
        });

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1, contextMenuPeer->m_showContextMenuCount);
        });
    }

    void ProviderSideIntegrationTests::VerifyDockGridAndTableProvidersThroughUia()
    {
        TestCleanupWrapper cleanup;
        MockHostTree tree;
        PatternMockPeer^ mockPeer = nullptr;
        auto releaseOnUIThread = wil::scope_exit([&]()
        {
            RunOnUIThread([&]()
            {
                tree = MockHostTree();
                mockPeer = nullptr;
            });
        });

        RunOnUIThread([&]()
        {
            tree = CreateMockHostTree({ { MockPeerKind::Pattern, L"PatternMock" } });
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            mockPeer = static_cast<PatternMockPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(tree.hosts[0]));
            VERIFY_IS_NOT_NULL(mockPeer);
            mockPeer->m_cells = CreateCellPeers(tree);
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto element = FindUiaElementByName(L"PatternMock");

            LOG_OUTPUT(L"Dock pattern.");
            auto dock = GetUiaPattern<IUIAutomationDockPattern>(element.Get(), UIA_DockPatternId);
            DockPosition dockPosition = DockPosition_None;
            VERIFY_SUCCEEDED(dock->get_CurrentDockPosition(&dockPosition));
            VERIFY_ARE_EQUAL(static_cast<int>(DockPosition_Left), static_cast<int>(dockPosition));
            VERIFY_SUCCEEDED(dock->SetDockPosition(DockPosition_Fill));

            LOG_OUTPUT(L"Grid pattern.");
            auto grid = GetUiaPattern<IUIAutomationGridPattern>(element.Get(), UIA_GridPatternId);
            int value = 0;
            VERIFY_SUCCEEDED(grid->get_CurrentRowCount(&value));
            VERIFY_ARE_EQUAL(5, value);
            VERIFY_SUCCEEDED(grid->get_CurrentColumnCount(&value));
            VERIFY_ARE_EQUAL(3, value);
            wrl::ComPtr<IUIAutomationElement> item;
            VERIFY_SUCCEEDED(grid->GetItem(1, 2, &item));
            VerifyElementName(item.Get(), L"CellB");

            LOG_OUTPUT(L"GridItem pattern.");
            auto gridItem = GetUiaPattern<IUIAutomationGridItemPattern>(element.Get(), UIA_GridItemPatternId);
            VERIFY_SUCCEEDED(gridItem->get_CurrentRow(&value));
            VERIFY_ARE_EQUAL(6, value);
            VERIFY_SUCCEEDED(gridItem->get_CurrentColumn(&value));
            VERIFY_ARE_EQUAL(7, value);
            VERIFY_SUCCEEDED(gridItem->get_CurrentRowSpan(&value));
            VERIFY_ARE_EQUAL(2, value);
            VERIFY_SUCCEEDED(gridItem->get_CurrentColumnSpan(&value));
            VERIFY_ARE_EQUAL(3, value);
            wrl::ComPtr<IUIAutomationElement> containingGrid;
            VERIFY_SUCCEEDED(gridItem->get_CurrentContainingGrid(&containingGrid));
            VerifyElementName(containingGrid.Get(), L"CellC");

            LOG_OUTPUT(L"Table pattern.");
            auto table = GetUiaPattern<IUIAutomationTablePattern>(element.Get(), UIA_TablePatternId);
            wrl::ComPtr<IUIAutomationElementArray> headers;
            VERIFY_SUCCEEDED(table->GetCurrentColumnHeaders(&headers));
            VerifyElementArrayNames(headers.Get(), { L"CellA", L"CellB" });
            VERIFY_SUCCEEDED(table->GetCurrentRowHeaders(headers.ReleaseAndGetAddressOf()));
            VerifyElementArrayNames(headers.Get(), { L"CellC", L"CellA" });
            RowOrColumnMajor rowOrColumnMajor = RowOrColumnMajor_RowMajor;
            VERIFY_SUCCEEDED(table->get_CurrentRowOrColumnMajor(&rowOrColumnMajor));
            VERIFY_ARE_EQUAL(static_cast<int>(RowOrColumnMajor_ColumnMajor), static_cast<int>(rowOrColumnMajor));
        });

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(mockPeer->m_setDockPositions.size()));
            VERIFY_ARE_EQUAL(static_cast<int>(xaml_automation::DockPosition::Fill), static_cast<int>(mockPeer->m_setDockPositions[0]));
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(mockPeer->m_getItemRequests.size()));
            VERIFY_ARE_EQUAL(1, mockPeer->m_getItemRequests[0].first);
            VERIFY_ARE_EQUAL(2, mockPeer->m_getItemRequests[0].second);
        });
    }

    void ProviderSideIntegrationTests::VerifyMultipleViewTransformAndInputProvidersThroughUia()
    {
        TestCleanupWrapper cleanup;
        MockHostTree tree;
        PatternMockPeer^ mockPeer = nullptr;
        auto releaseOnUIThread = wil::scope_exit([&]()
        {
            RunOnUIThread([&]()
            {
                tree = MockHostTree();
                mockPeer = nullptr;
            });
        });

        RunOnUIThread([&]()
        {
            tree = CreateMockHostTree({ { MockPeerKind::Pattern, L"PatternMock" } });
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            mockPeer = static_cast<PatternMockPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(tree.hosts[0]));
            VERIFY_IS_NOT_NULL(mockPeer);
            mockPeer->m_cells = CreateCellPeers(tree);
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto element = FindUiaElementByName(L"PatternMock");

            LOG_OUTPUT(L"MultipleView pattern.");
            auto multipleView = GetUiaPattern<IUIAutomationMultipleViewPattern>(element.Get(), UIA_MultipleViewPatternId);
            int currentView = 0;
            VERIFY_SUCCEEDED(multipleView->get_CurrentCurrentView(&currentView));
            VERIFY_ARE_EQUAL(9, currentView);
            SAFEARRAY* supportedViews = nullptr;
            VERIFY_SUCCEEDED(multipleView->GetCurrentSupportedViews(&supportedViews));
            auto destroySupportedViews = wil::scope_exit([&]() { if (supportedViews) { SafeArrayDestroy(supportedViews); } });
            const auto views = ReadIntSafeArray(supportedViews);
            VERIFY_ARE_EQUAL(2u, static_cast<unsigned int>(views.size()));
            VERIFY_ARE_EQUAL(7, views[0]);
            VERIFY_ARE_EQUAL(9, views[1]);
            AutoBSTR viewName;
            VERIFY_SUCCEEDED(multipleView->GetViewName(7, viewName.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(L"Seven", viewName);
            VERIFY_SUCCEEDED(multipleView->GetViewName(9, viewName.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(L"Nine", viewName);
            VERIFY_SUCCEEDED(multipleView->SetCurrentView(7));

            LOG_OUTPUT(L"Transform pattern.");
            auto transform = GetUiaPattern<IUIAutomationTransformPattern>(element.Get(), UIA_TransformPatternId);
            BOOL flag = FALSE;
            VERIFY_SUCCEEDED(transform->get_CurrentCanMove(&flag));
            VERIFY_IS_TRUE(!!flag);
            VERIFY_SUCCEEDED(transform->get_CurrentCanResize(&flag));
            VERIFY_IS_FALSE(!!flag);
            VERIFY_SUCCEEDED(transform->get_CurrentCanRotate(&flag));
            VERIFY_IS_TRUE(!!flag);
            VERIFY_SUCCEEDED(transform->Move(11.0, 22.0));
            VERIFY_SUCCEEDED(transform->Resize(33.0, 44.0));
            VERIFY_SUCCEEDED(transform->Rotate(55.0));

            LOG_OUTPUT(L"Transform2 pattern.");
            auto transform2 = GetUiaPattern<IUIAutomationTransformPattern2>(element.Get(), UIA_TransformPattern2Id);
            VERIFY_SUCCEEDED(transform2->get_CurrentCanZoom(&flag));
            VERIFY_IS_TRUE(!!flag);
            double zoom = 0.0;
            VERIFY_SUCCEEDED(transform2->get_CurrentZoomLevel(&zoom));
            VERIFY_ARE_EQUAL(1.5, zoom);
            VERIFY_SUCCEEDED(transform2->get_CurrentZoomMinimum(&zoom));
            VERIFY_ARE_EQUAL(0.5, zoom);
            VERIFY_SUCCEEDED(transform2->get_CurrentZoomMaximum(&zoom));
            VERIFY_ARE_EQUAL(4.0, zoom);
            VERIFY_SUCCEEDED(transform2->Zoom(2.5));
            VERIFY_SUCCEEDED(transform2->ZoomByUnit(ZoomUnit_LargeIncrement));

            LOG_OUTPUT(L"VirtualizedItem pattern.");
            auto virtualizedItem = GetUiaPattern<IUIAutomationVirtualizedItemPattern>(element.Get(), UIA_VirtualizedItemPatternId);
            VERIFY_SUCCEEDED(virtualizedItem->Realize());

            LOG_OUTPUT(L"SynchronizedInput pattern.");
            auto synchronizedInput = GetUiaPattern<IUIAutomationSynchronizedInputPattern>(element.Get(), UIA_SynchronizedInputPatternId);
            VERIFY_SUCCEEDED(synchronizedInput->StartListening(SynchronizedInputType_KeyDown));
            VERIFY_SUCCEEDED(synchronizedInput->Cancel());
        });

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(2u, static_cast<unsigned int>(mockPeer->m_viewNameRequests.size()));
            VERIFY_ARE_EQUAL(7, mockPeer->m_viewNameRequests[0]);
            VERIFY_ARE_EQUAL(9, mockPeer->m_viewNameRequests[1]);
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(mockPeer->m_setCurrentViews.size()));
            VERIFY_ARE_EQUAL(7, mockPeer->m_setCurrentViews[0]);

            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(mockPeer->m_moveRequests.size()));
            VERIFY_ARE_EQUAL(11.0, mockPeer->m_moveRequests[0].first);
            VERIFY_ARE_EQUAL(22.0, mockPeer->m_moveRequests[0].second);
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(mockPeer->m_resizeRequests.size()));
            VERIFY_ARE_EQUAL(33.0, mockPeer->m_resizeRequests[0].first);
            VERIFY_ARE_EQUAL(44.0, mockPeer->m_resizeRequests[0].second);
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(mockPeer->m_rotateRequests.size()));
            VERIFY_ARE_EQUAL(55.0, mockPeer->m_rotateRequests[0]);

            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(mockPeer->m_zoomRequests.size()));
            VERIFY_ARE_EQUAL(2.5, mockPeer->m_zoomRequests[0]);
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(mockPeer->m_zoomByUnitRequests.size()));
            VERIFY_ARE_EQUAL(static_cast<int>(xaml_automation::ZoomUnit::LargeIncrement), static_cast<int>(mockPeer->m_zoomByUnitRequests[0]));

            VERIFY_ARE_EQUAL(1, mockPeer->m_realizeCount);

            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(mockPeer->m_startListeningRequests.size()));
            VERIFY_ARE_EQUAL(static_cast<int>(xaml_automation::SynchronizedInputType::KeyDown), static_cast<int>(mockPeer->m_startListeningRequests[0]));
            VERIFY_ARE_EQUAL(1, mockPeer->m_cancelCount);
        });
    }

    void ProviderSideIntegrationTests::VerifyWindowProviderThroughUia()
    {
        TestCleanupWrapper cleanup;
        MockHostTree tree;
        PatternMockPeer^ mockPeer = nullptr;
        auto releaseOnUIThread = wil::scope_exit([&]()
        {
            RunOnUIThread([&]()
            {
                tree = MockHostTree();
                mockPeer = nullptr;
            });
        });

        RunOnUIThread([&]()
        {
            tree = CreateMockHostTree({ { MockPeerKind::Pattern, L"PatternMock" } });
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            mockPeer = static_cast<PatternMockPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(tree.hosts[0]));
            VERIFY_IS_NOT_NULL(mockPeer);
            mockPeer->m_cells = CreateCellPeers(tree);
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto element = FindUiaElementByName(L"PatternMock");
            auto window = GetUiaPattern<IUIAutomationWindowPattern>(element.Get(), UIA_WindowPatternId);

            BOOL flag = FALSE;
            VERIFY_SUCCEEDED(window->get_CurrentIsModal(&flag));
            VERIFY_IS_TRUE(!!flag);
            VERIFY_SUCCEEDED(window->get_CurrentIsTopmost(&flag));
            VERIFY_IS_FALSE(!!flag);
            VERIFY_SUCCEEDED(window->get_CurrentCanMaximize(&flag));
            VERIFY_IS_TRUE(!!flag);
            VERIFY_SUCCEEDED(window->get_CurrentCanMinimize(&flag));
            VERIFY_IS_FALSE(!!flag);

            WindowInteractionState interactionState = WindowInteractionState_Running;
            VERIFY_SUCCEEDED(window->get_CurrentWindowInteractionState(&interactionState));
            VERIFY_ARE_EQUAL(static_cast<int>(WindowInteractionState_BlockedByModalWindow), static_cast<int>(interactionState));
            WindowVisualState visualState = WindowVisualState_Normal;
            VERIFY_SUCCEEDED(window->get_CurrentWindowVisualState(&visualState));
            VERIFY_ARE_EQUAL(static_cast<int>(WindowVisualState_Maximized), static_cast<int>(visualState));

            VERIFY_SUCCEEDED(window->SetWindowVisualState(WindowVisualState_Minimized));
            // Only the forwarded timeout is asserted: CManagedWindowProvider::WaitForInputIdle does not copy the provider's
            // result into its out-parameter, so the value the client receives is not the provider's answer. This is product
            // concern PC-2 of PR #12805, which is being triaged and has no bug ID yet. Add VERIFY_IS_TRUE(idle) once the
            // product returns the provider's value.
            BOOL idle = FALSE;
            VERIFY_SUCCEEDED(window->WaitForInputIdle(1234, &idle));
            VERIFY_SUCCEEDED(window->Close());
        });

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(mockPeer->m_setVisualStates.size()));
            VERIFY_ARE_EQUAL(static_cast<int>(xaml_automation::WindowVisualState::Minimized), static_cast<int>(mockPeer->m_setVisualStates[0]));
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(mockPeer->m_waitForInputIdleRequests.size()));
            VERIFY_ARE_EQUAL(1234, mockPeer->m_waitForInputIdleRequests[0]);
            VERIFY_ARE_EQUAL(1, mockPeer->m_closeCount);
        });
    }

    void ProviderSideIntegrationTests::VerifyTextChildAnnotationAndTextProvidersThroughUia()
    {
        TestCleanupWrapper cleanup;
        MockHostTree tree;
        TextMockPeer^ mockPeer = nullptr;
        std::vector<xaml_automation_peers::AutomationPeer^> cellPeers;
        auto releaseOnUIThread = wil::scope_exit([&]()
        {
            RunOnUIThread([&]()
            {
                tree = MockHostTree();
                mockPeer = nullptr;
                cellPeers.clear();
            });
        });

        RunOnUIThread([&]()
        {
            tree = CreateMockHostTree({ { MockPeerKind::Text, L"TextMock" }, { MockPeerKind::TextOnly, L"TextOnlyMock" } });
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            mockPeer = static_cast<TextMockPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(tree.hosts[0]));
            VERIFY_IS_NOT_NULL(mockPeer);
            cellPeers = CreateCellPeers(tree);
            mockPeer->m_cells = cellPeers;
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto element = FindUiaElementByName(L"TextMock");
            auto cellA = FindUiaElementByName(L"CellA");
            auto cellB = FindUiaElementByName(L"CellB");
            wrl::ComPtr<IUIAutomationTextRange> range;

            LOG_OUTPUT(L"Text pattern: RangeFromChild and RangeFromPoint.");
            auto text = GetUiaPattern<IUIAutomationTextPattern>(element.Get(), UIA_TextPatternId);
            VERIFY_SUCCEEDED(text->RangeFromChild(cellA.Get(), &range));
            VerifyRangeText(range.Get(), L"child");

            RECT bounds = {};
            VERIFY_SUCCEEDED(element->get_CurrentBoundingRectangle(&bounds));
            const POINT center = { (bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2 };
            const HRESULT rangeFromPointResult = text->RangeFromPoint(center, range.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"RangeFromPoint returned 0x%08x.", rangeFromPointResult);
            VERIFY_SUCCEEDED(rangeFromPointResult);
            VERIFY_IS_NULL(range.Get());

            LOG_OUTPUT(L"Text2 pattern: RangeFromAnnotation.");
            auto text2 = GetUiaPattern<IUIAutomationTextPattern2>(element.Get(), UIA_TextPattern2Id);
            VERIFY_SUCCEEDED(text2->RangeFromAnnotation(cellB.Get(), range.ReleaseAndGetAddressOf()));
            VerifyRangeText(range.Get(), L"annotation");

            LOG_OUTPUT(L"TextChild pattern: the TextContainer request reaches the provider.");
            auto textChild = GetUiaPattern<IUIAutomationTextChildPattern>(element.Get(), UIA_TextChildPatternId);
            wrl::ComPtr<IUIAutomationElement> container;
            VERIFY_SUCCEEDED(textChild->get_TextContainer(&container));
            // The returned element is not asserted: CUIATextChildProviderWrapper::get_TextContainer queries the wrapper into
            // the address of its out-parameter instead of the out-parameter itself, so the client always receives null. This is
            // product concern PC-4 of PR #12805, which is being triaged and has no bug ID yet. Add
            // VerifyElementName(container.Get(), L"CellA") once the product returns the provider's element.
            VERIFY_SUCCEEDED(textChild->get_TextRange(range.ReleaseAndGetAddressOf()));
            VerifyRangeText(range.Get(), L"textchild");

            LOG_OUTPUT(L"Annotation pattern.");
            auto annotation = GetUiaPattern<IUIAutomationAnnotationPattern>(element.Get(), UIA_AnnotationPatternId);
            int annotationTypeId = 0;
            VERIFY_SUCCEEDED(annotation->get_CurrentAnnotationTypeId(&annotationTypeId));
            VERIFY_ARE_EQUAL(static_cast<int>(AnnotationType_Comment), annotationTypeId);
            AutoBSTR value;
            VERIFY_SUCCEEDED(annotation->get_CurrentAnnotationTypeName(value.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(L"Comment", value);
            VERIFY_SUCCEEDED(annotation->get_CurrentAuthor(value.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(L"Ann", value);
            VERIFY_SUCCEEDED(annotation->get_CurrentDateTime(value.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(L"2026-01-02", value);
            wrl::ComPtr<IUIAutomationElement> target;
            VERIFY_SUCCEEDED(annotation->get_CurrentTarget(&target));
            VerifyElementName(target.Get(), L"CellC");

            LOG_OUTPUT(L"A provider that only implements ITextProvider does not expose Text2.");
            auto textOnlyElement = FindUiaElementByName(L"TextOnlyMock");
            auto textOnly = GetUiaPattern<IUIAutomationTextPattern>(textOnlyElement.Get(), UIA_TextPatternId);
            wrl::ComPtr<IUIAutomationTextRange> textOnlyRange;
            VERIFY_SUCCEEDED(textOnly->get_DocumentRange(&textOnlyRange));
            VerifyRangeText(textOnlyRange.Get(), L"textonly");
            wrl::ComPtr<IUIAutomationTextPattern2> textOnly2;
            const HRESULT text2Result = textOnlyElement->GetCurrentPatternAs(UIA_TextPattern2Id, __uuidof(IUIAutomationTextPattern2), &textOnly2);
            LOG_OUTPUT(L"GetCurrentPatternAs(Text2) on the ITextProvider-only element returned 0x%08x.", text2Result);
            VERIFY_SUCCEEDED(text2Result);
            VERIFY_IS_NULL(textOnly2.Get());

            LOG_OUTPUT(L"When a provider has no range for a child, an annotation or a text child, the client gets S_OK and no range.");
            VERIFY_SUCCEEDED(textOnly->RangeFromChild(cellA.Get(), range.ReleaseAndGetAddressOf()));
            VERIFY_IS_NULL(range.Get());
            RunOnUIThread([&]()
            {
                mockPeer->m_returnNoRanges = true;
            });
            VERIFY_SUCCEEDED(text2->RangeFromAnnotation(cellB.Get(), range.ReleaseAndGetAddressOf()));
            VERIFY_IS_NULL(range.Get());
            VERIFY_SUCCEEDED(textChild->get_TextRange(range.ReleaseAndGetAddressOf()));
            VERIFY_IS_NULL(range.Get());
        });

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(1u, static_cast<unsigned int>(mockPeer->m_rangeFromChildPeers.size()));
            VERIFY_IS_TRUE(AreSameObject(mockPeer->m_rangeFromChildPeers[0], cellPeers[0]));
            VERIFY_ARE_EQUAL(2u, static_cast<unsigned int>(mockPeer->m_rangeFromAnnotationPeers.size()));
            VERIFY_IS_TRUE(AreSameObject(mockPeer->m_rangeFromAnnotationPeers[0], cellPeers[1]));
            VERIFY_IS_TRUE(AreSameObject(mockPeer->m_rangeFromAnnotationPeers[1], cellPeers[1]));
            VERIFY_ARE_EQUAL(1, mockPeer->m_rangeFromPointCount);
            VERIFY_ARE_EQUAL(1, mockPeer->m_textContainerCount);
        });
    }

    void ProviderSideIntegrationTests::VerifySpreadsheetStylesObjectModelAndDragProvidersThroughUia()
    {
        TestCleanupWrapper cleanup;
        MockHostTree tree;
        PatternMockPeer^ mockPeer = nullptr;
        DragMockPeer^ dragPeer = nullptr;
        Platform::Object^ objectModel = nullptr;
        wrl::ComPtr<IUnknown> expectedObjectModelIdentity;
        auto releaseOnUIThread = wil::scope_exit([&]()
        {
            RunOnUIThread([&]()
            {
                if (mockPeer)
                {
                    mockPeer->m_objectModel = nullptr;
                }
                tree = MockHostTree();
                mockPeer = nullptr;
                dragPeer = nullptr;
                objectModel = nullptr;
                expectedObjectModelIdentity.Reset();
            });
        });

        RunOnUIThread([&]()
        {
            tree = CreateMockHostTree({ { MockPeerKind::Pattern, L"PatternMock" }, { MockPeerKind::Drag, L"DragMock" } });
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const auto cellPeers = CreateCellPeers(tree);
            mockPeer = static_cast<PatternMockPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(tree.hosts[0]));
            VERIFY_IS_NOT_NULL(mockPeer);
            mockPeer->m_cells = cellPeers;
            objectModel = ref new Platform::Collections::Vector<int>();
            mockPeer->m_objectModel = objectModel;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(objectModel)->QueryInterface(IID_PPV_ARGS(&expectedObjectModelIdentity)));

            dragPeer = static_cast<DragMockPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(tree.hosts[1]));
            VERIFY_IS_NOT_NULL(dragPeer);
            dragPeer->m_cells = cellPeers;
        });

        UIAutomationHelper::RunOnCorrectThreadForUIA([&]()
        {
            auto element = FindUiaElementByName(L"PatternMock");

            LOG_OUTPUT(L"Spreadsheet pattern.");
            auto spreadsheet = GetUiaPattern<IUIAutomationSpreadsheetPattern>(element.Get(), UIA_SpreadsheetPatternId);
            AutoBSTR itemName(L"B2");
            wrl::ComPtr<IUIAutomationElement> cell;
            VERIFY_SUCCEEDED(spreadsheet->GetItemByName(itemName, &cell));
            VerifyElementName(cell.Get(), L"CellB");
            const HRESULT emptyNameResult = spreadsheet->GetItemByName(nullptr, cell.ReleaseAndGetAddressOf());
            LOG_OUTPUT(L"GetItemByName(null) returned 0x%08x.", emptyNameResult);
            VERIFY_SUCCEEDED(emptyNameResult);
            VERIFY_IS_NULL(cell.Get());

            LOG_OUTPUT(L"SpreadsheetItem pattern.");
            auto spreadsheetItem = GetUiaPattern<IUIAutomationSpreadsheetItemPattern>(element.Get(), UIA_SpreadsheetItemPatternId);
            AutoBSTR value;
            VERIFY_SUCCEEDED(spreadsheetItem->get_CurrentFormula(value.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(L"=A1+1", value);
            wrl::ComPtr<IUIAutomationElementArray> annotationObjects;
            VERIFY_SUCCEEDED(spreadsheetItem->GetCurrentAnnotationObjects(&annotationObjects));
            VerifyElementArrayNames(annotationObjects.Get(), { L"CellA", L"CellC" });
            SAFEARRAY* annotationTypes = nullptr;
            VERIFY_SUCCEEDED(spreadsheetItem->GetCurrentAnnotationTypes(&annotationTypes));
            auto destroyAnnotationTypes = wil::scope_exit([&]() { if (annotationTypes) { SafeArrayDestroy(annotationTypes); } });
            const auto types = ReadIntSafeArray(annotationTypes);
            VERIFY_ARE_EQUAL(2u, static_cast<unsigned int>(types.size()));
            VERIFY_ARE_EQUAL(static_cast<int>(AnnotationType_Comment), types[0]);
            VERIFY_ARE_EQUAL(static_cast<int>(AnnotationType_SpellingError), types[1]);

            LOG_OUTPUT(L"Styles pattern.");
            auto styles = GetUiaPattern<IUIAutomationStylesPattern>(element.Get(), UIA_StylesPatternId);
            int intValue = 0;
            VERIFY_SUCCEEDED(styles->get_CurrentStyleId(&intValue));
            VERIFY_ARE_EQUAL(70010, intValue);
            VERIFY_SUCCEEDED(styles->get_CurrentStyleName(value.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(L"Custom Style", value);
            VERIFY_SUCCEEDED(styles->get_CurrentFillColor(&intValue));
            VERIFY_ARE_EQUAL(0x7F123456, intValue);
            VERIFY_SUCCEEDED(styles->get_CurrentFillPatternColor(&intValue));
            VERIFY_ARE_EQUAL(0x40ABCDEF, intValue);
            VERIFY_SUCCEEDED(styles->get_CurrentFillPatternStyle(value.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(L"dots", value);
            VERIFY_SUCCEEDED(styles->get_CurrentShape(value.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(L"circle", value);
            VERIFY_SUCCEEDED(styles->get_CurrentExtendedProperties(value.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(L"ext=1", value);

            LOG_OUTPUT(L"ObjectModel pattern.");
            auto objectModelPattern = GetUiaPattern<IUIAutomationObjectModelPattern>(element.Get(), UIA_ObjectModelPatternId);
            wrl::ComPtr<IUnknown> underlyingObjectModel;
            VERIFY_SUCCEEDED(objectModelPattern->GetUnderlyingObjectModel(&underlyingObjectModel));
            VERIFY_IS_NOT_NULL(underlyingObjectModel.Get());
            wrl::ComPtr<IUnknown> underlyingObjectModelIdentity;
            VERIFY_SUCCEEDED(underlyingObjectModel.As(&underlyingObjectModelIdentity));
            VERIFY_IS_TRUE(underlyingObjectModelIdentity.Get() == expectedObjectModelIdentity.Get());

            LOG_OUTPUT(L"Drag pattern.");
            auto dragElement = FindUiaElementByName(L"DragMock");
            auto drag = GetUiaPattern<IUIAutomationDragPattern>(dragElement.Get(), UIA_DragPatternId);
            BOOL isGrabbed = FALSE;
            VERIFY_SUCCEEDED(drag->get_CurrentIsGrabbed(&isGrabbed));
            VERIFY_IS_TRUE(!!isGrabbed);
            VERIFY_SUCCEEDED(drag->get_CurrentDropEffect(value.ReleaseAndGetAddressOf()));
            VerifyBstrEquals(L"copy", value);
            SAFEARRAY* dropEffects = nullptr;
            const HRESULT dropEffectsResult = drag->get_CurrentDropEffects(&dropEffects);
            auto destroyDropEffects = wil::scope_exit([&]() { if (dropEffects) { SafeArrayDestroy(dropEffects); } });
            LOG_OUTPUT(L"get_CurrentDropEffects for an empty list returned 0x%08x.", dropEffectsResult);
            VERIFY_SUCCEEDED(dropEffectsResult);
            VERIFY_ARE_EQUAL(0, static_cast<int>(GetSafeArrayCount(dropEffects)));
            wrl::ComPtr<IUIAutomationElementArray> grabbedItems;
            VERIFY_SUCCEEDED(drag->GetCurrentGrabbedItems(&grabbedItems));
            VerifyElementArrayNames(grabbedItems.Get(), { L"CellA", L"CellB" });
        });

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(mockPeer->m_itemByNameRequests.size() >= 1u);
            VERIFY_IS_TRUE(mockPeer->m_itemByNameRequests[0] == L"B2");
            for (size_t i = 1; i < mockPeer->m_itemByNameRequests.size(); ++i)
            {
                VERIFY_IS_TRUE(mockPeer->m_itemByNameRequests[i].empty());
            }
        });
    }

    void ProviderSideIntegrationTests::VerifyCalendarDatePickerAutomationPeerPublicConstructor()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto picker = ref new xaml_controls::CalendarDatePicker();
            auto peer = ref new xaml_automation_peers::CalendarDatePickerAutomationPeer(picker);
            VERIFY_IS_NOT_NULL(peer);
            VERIFY_IS_TRUE(AreSameObject(peer->Owner, picker));
            VerifyStringEquals(L"CalendarDatePicker", peer->GetClassName());

            bool nullOwnerFailed = false;
            try
            {
                auto invalidPeer = ref new xaml_automation_peers::CalendarDatePickerAutomationPeer(nullptr);
                LOG_OUTPUT(L"Unexpectedly created a peer without an owner.");
                (void)invalidPeer;
            }
            catch (Platform::Exception^ e)
            {
                LOG_OUTPUT(L"Creating the peer without an owner failed with 0x%08x.", e->HResult);
                nullOwnerFailed = true;
            }
            VERIFY_IS_TRUE(nullOwnerFailed);

            LOG_OUTPUT(L"An app peer derived from CalendarDatePickerAutomationPeer keeps the owner and its own override.");
            auto derivedPeer = ref new ProviderSideMocks::DerivedCalendarDatePickerAutomationPeer(picker);
            VERIFY_IS_TRUE(AreSameObject(derivedPeer->Owner, picker));
            VerifyStringEquals(L"DerivedCalendarDatePicker", derivedPeer->GetClassName());
            VERIFY_ARE_EQUAL(static_cast<int>(peer->GetAutomationControlType()), static_cast<int>(derivedPeer->GetAutomationControlType()));
        });
    }

    void ProviderSideIntegrationTests::VerifyLocalizedControlTypeForFlipViewAndInvalidType()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            CustomControl^ control = ref new CustomControl();
            CustomAutomationPeer^ customAP = static_cast<CustomAutomationPeer^>(xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(control));

            customAP->SetControlType(xaml_automation_peers::AutomationControlType::FlipView);
            VerifyStringEquals(L"flip view", customAP->GetLocalizedControlType());

            customAP->SetControlType(static_cast<xaml_automation_peers::AutomationControlType>(999));
            bool unknownTypeFailed = false;
            try
            {
                auto localizedControlType = customAP->GetLocalizedControlType();
                LOG_OUTPUT(L"Unexpected localized control type \"%s\".", localizedControlType ? localizedControlType->Data() : L"<null>");
            }
            catch (Platform::Exception^ e)
            {
                LOG_OUTPUT(L"GetLocalizedControlType for an unknown control type failed with 0x%08x.", e->HResult);
                unknownTypeFailed = true;
            }
            VERIFY_IS_TRUE(unknownTypeFailed);
        });
    }

    void ProviderSideIntegrationTests::VerifyRaiseStructureChangedEventWithInvalidType()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            auto peer = ref new BaseOnlyAutomationPeer();
            HRESULT raiseResult = S_OK;
            try
            {
                peer->RaiseStructureChangedEvent(static_cast<xaml_automation_peers::AutomationStructureChangeType>(999), nullptr);
            }
            catch (Platform::Exception^ e)
            {
                raiseResult = e->HResult;
            }
            LOG_OUTPUT(L"RaiseStructureChangedEvent with an unknown change type returned 0x%08x.", raiseResult);
            VERIFY_ARE_EQUAL(E_UNEXPECTED, raiseResult);
        });
    }

    void ProviderSideIntegrationTests::VerifyAutomationPeerFactoryDefaultInterfaces()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            wrl::ComPtr<IActivationFactory> factory;
            VERIFY_SUCCEEDED(RoGetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Automation.Peers.AutomationPeer").Get(), IID_PPV_ARGS(&factory)));
            ULONG iidCount = 0;
            IID* iids = nullptr;
            VERIFY_SUCCEEDED(factory->GetIids(&iidCount, &iids));
            auto freeIids = wil::scope_exit([&]() { CoTaskMemFree(iids); });
            LOG_OUTPUT(L"The AutomationPeer activation factory reports %u interfaces.", static_cast<unsigned int>(iidCount));
            VERIFY_IS_GREATER_THAN(iidCount, 0ul);
            VERIFY_IS_NOT_NULL(iids);
            for (ULONG i = 0; i < iidCount; ++i)
            {
                wrl::ComPtr<IUnknown> iface;
                VERIFY_SUCCEEDED(factory.Get()->QueryInterface(iids[i], &iface));
                VERIFY_IS_NOT_NULL(iface.Get());
            }
        });
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Automation::Provider
