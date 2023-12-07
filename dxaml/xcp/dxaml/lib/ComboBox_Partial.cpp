// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComboBox.g.h"
#include "ComboBoxAutomationPeer.g.h"
#include "ComboBoxItem.g.h"
#include "ComboBoxTemplateSettings.g.h"
#include "Border.g.h"
#include "CarouselPanel.g.h"
#include "ContentPresenter.g.h"
#include "ComboBoxLightDismiss.g.h"
#include "SolidColorBrush.g.h"
#include "Canvas.g.h"
#include "ItemContainerGenerator.g.h"
#include "Window.g.h"
#include "TranslateTransform.g.h"
#include "Storyboard.g.h"
#include "PointerRoutedEventArgs.g.h"
#include "RectangleGeometry.g.h"
#include "Popup.g.h"
#include "TextBox.g.h"
#include "TextBoxBase.g.h"
#include "TextBlock.g.h"
#include "PopupRoot.g.h"
#include "KeyRoutedEventArgs.g.h"
#include "XboxUtility.h"
#include <LightDismissOverlayHelper.h>
#include "CharacterReceivedRoutedEventArgs.g.h"
#include "RoutedEventArgs.h"
#include "CharacterReceivedEventArgs.h"
#include <FrameworkUdk/BackButtonIntegration.h>
#include "PropertyPathParser.h"
#include "PropertyPath.h"
#include "VisualTreeHelper.h"
#include "ResourceDictionary.g.h"
#include "localizedResource.h"
#include "focusmgr.h"
#include "eventmgr.h"
#include "ComboBoxTextSubmittedEventArgs.h"
#include "ElevationHelper.h"
#include "ElementSoundPlayerService_Partial.h"
#include <PhoneImports.h>

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;
using namespace ::Windows::Internal;

const UINT ComboBox::s_itemCountThreashold = 5;

#pragma region Initialization/Destruction

ComboBox::ComboBox()
    : m_IsPointerOverMain(false)
    , m_IsPointerOverPopup(false)
    , m_bIsPressed(false)
    , m_preparingContentPresentersElement(false)
    , m_isDropDownClosing(false)
    , m_bIsPopupPannable(true)
    , m_bShouldCarousel(false)
    , m_bShouldCenterSelectedItem(false)
    , m_bPopupHasBeenArrangedOnce(false)
    , m_iLastGeneratedItemIndexforFaceplate(-1)
    , m_shouldPerformActions(false)
    , m_handledGamepadOrRemoteKeyDown(false)
    , m_ignoreCancelKeyDowns(false)
    , m_isExpanded(false)
    , m_itemCount(0)
    , m_itemsPresenterIndex(-1)
    , m_doKeepInView(false)
    , m_isOverlayVisible(false)
    , m_candidateWindowBoundsRect()
    , m_openedUp(false)
    , m_restoreIndexSet(false)
    , m_isClosingDueToCancel(false)
    , m_searchResultIndexSet(false)
    , m_selectAllOnTouch(false)
    , m_openPopupOnTouch(false)
    , m_isEditModeConfigured(false)
    , m_shouldMoveFocusToTextBox(false)
    , m_IsPointerOverDropDownOverlay(false)
{
    m_inputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::None;
    m_previousInputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::None;
}

ComboBox::~ComboBox()
{
    VERIFYHR(BackButtonIntegration_UnregisterListener(this));
}


// Prepares object's state
_Check_return_
HRESULT
ComboBox::Initialize()
{
    ctl::ComPtr<xaml::ISizeChangedEventHandler> spSizeChangedHandler;
    EventRegistrationToken sizeChangedToken;

    IFC_RETURN(ComboBoxGenerated::Initialize());

    spSizeChangedHandler.Attach(
        new ClassMemberEventHandler<
        ComboBox,
        xaml_controls::IComboBox,
        xaml::ISizeChangedEventHandler,
        IInspectable,
        xaml::ISizeChangedEventArgs>(this, &ComboBox::OnSizeChanged, true /* subscribingToSelf */));
    IFC_RETURN(add_SizeChanged(spSizeChangedHandler.Get(), &sizeChangedToken));

    ctl::ComPtr<wf::ITypedEventHandler<xaml::UIElement*, xaml_input::CharacterReceivedRoutedEventArgs*>> characterReceivedHandler;
    CharacterReceivedEventSourceType* characterReceivedEventSource = nullptr;

    characterReceivedHandler.Attach(
        new ClassMemberEventHandler<
        ComboBox,
        xaml_controls::IComboBox,
        wf::ITypedEventHandler<xaml::UIElement*, xaml_input::CharacterReceivedRoutedEventArgs*>,
        IUIElement,
        xaml_input::ICharacterReceivedRoutedEventArgs>(this, &ComboBox::OnCharacterReceived, true /* subscribingToSelf */));
    IFC_RETURN(GetCharacterReceivedEventSourceNoRef(&characterReceivedEventSource));
    IFC_RETURN(characterReceivedEventSource->AddHandler(characterReceivedHandler.Get(), true /* handledEventsToo */));

    IFC_RETURN(m_epUnloadedHandler.AttachEventHandler(this, std::bind(&ComboBox::OnUnloaded, this, _1, _2)));

    // Update the max number of items that can be shown by Combobox.
    // Resources which we look for may not exist in older versions, in which case,
    // leave at default value.
    ctl::ComPtr<xaml::IResourceDictionary> resources;
    IFC_RETURN(get_Resources(&resources));

    int queriedComboBoxPopupMaxNumberOfItems = m_MaxNumberOfItemsThatCanBeShown;
    resources.Cast<ResourceDictionary>()->TryLookupBoxedValue(wrl_wrappers::HStringReference(
        L"ComboBoxPopupMaxNumberOfItems").Get(), &queriedComboBoxPopupMaxNumberOfItems);
    m_MaxNumberOfItemsThatCanBeShown = static_cast<INT>(queriedComboBoxPopupMaxNumberOfItems);

    int queriedComboBoxPopupMaxNumberOfItemsThatCanBeShownOnOneSide = m_MaxNumberOfItemsThatCanBeShownOnOneSide;
    resources.Cast<ResourceDictionary>()->TryLookupBoxedValue(wrl_wrappers::HStringReference(
        L"ComboBoxPopupMaxNumberOfItemsThatCanBeShownOnOneSide").Get(), &queriedComboBoxPopupMaxNumberOfItemsThatCanBeShownOnOneSide);
    m_MaxNumberOfItemsThatCanBeShownOnOneSide = static_cast<INT>(queriedComboBoxPopupMaxNumberOfItemsThatCanBeShownOnOneSide);

    return S_OK;
}

// Sets up instances that are expected on this type.
_Check_return_ HRESULT ComboBox::PrepareState()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ComboBoxTemplateSettings> spTemplateSettings;
    IFC(ComboBoxGenerated::PrepareState());
    IFC(ctl::make(&spTemplateSettings));
    IFC(put_TemplateSettings(spTemplateSettings.Get()));

    IFC(spTemplateSettings->put_SelectedItemDirection(xaml_primitives::AnimationDirection_Top));

Cleanup:
    RRETURN(hr);
}

#pragma endregion Initialization/Destruction

#pragma region Apply Template

// Apply a template to the ComboBox.
IFACEMETHODIMP ComboBox::OnApplyTemplate()
{
    ctl::ComPtr<IInspectable> spEmptyContent;

    IFC_RETURN(ReleaseMembers());

    // bring Content back to swapped container
    if (m_tpContentPresenterPart && m_tpSwappedOutComboBoxItem)
    {
        IFC_RETURN(SetContentPresenter(-1));
    }

    IFC_RETURN(put_IsDropDownOpen(FALSE));

    if (IsEditable())
    {
        IFC_RETURN(DisableEditableMode());
    }

    // Call base OnApplyTemplate
    IFC_RETURN(ComboBoxGenerated::OnApplyTemplate());

    if (m_tpItemsPresenterHostPart)
    {
        ctl::ComPtr<IDependencyObject> spItemsPresenterHostParent;
        IFC_RETURN(m_tpItemsPresenterHostPart.AsOrNull<IFrameworkElement>()->get_Parent(&spItemsPresenterHostParent));
        SetPtrValueWithQIOrNull(m_tpItemsPresenterHostParent, spItemsPresenterHostParent.Get());
    }

    if (m_tpContentPresenterPart)
    {
        // Save a reference to content to be used when the ComboBox is empty.
        INT selectedIndex = -1;
        IFC_RETURN(m_tpContentPresenterPart.Get()->get_Content(&spEmptyContent));
        SetPtrValue(m_tpEmptyContent, spEmptyContent.Get());
        if (!IsInline())
        {
            IFC_RETURN(get_SelectedIndex(&selectedIndex));
            IFC_RETURN(SetContentPresenter(selectedIndex));
            m_tpContentPresenterPart.Cast<ContentPresenter>()->SetFaceplateForComboBox();
        }
    }

    // Lookup the animations to use for the window overlay.
    ctl::ComPtr<xaml::IFrameworkElement> layoutRoot;
    IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"LayoutRoot"), layoutRoot.ReleaseAndGetAddressOf()));
    if (layoutRoot)
    {
        ctl::ComPtr<xaml::IResourceDictionary> resources;
        IFC_RETURN(layoutRoot->get_Resources(&resources));

        ctl::ComPtr<wfc::IMap<IInspectable*, IInspectable*>> resourcesMap;
        IFC_RETURN(resources.As(&resourcesMap));

        ctl::ComPtr<IInspectable> boxedResourceKey;
        IFC_RETURN(PropertyValue::CreateFromString(wrl_wrappers::HStringReference(L"OverlayOpeningAnimation").Get(), boxedResourceKey.ReleaseAndGetAddressOf()));

        BOOLEAN hasKey = FALSE;
        IFC_RETURN(resourcesMap->HasKey(boxedResourceKey.Get(), &hasKey));
        if (hasKey)
        {
            ctl::ComPtr<IInspectable> boxedResource;
            IFC_RETURN(resourcesMap->Lookup(boxedResourceKey.Get(), &boxedResource));

            ctl::ComPtr<xaml_animation::IStoryboard> overlayOpeningStoryboard;
            IFC_RETURN(boxedResource.As(&overlayOpeningStoryboard));

            SetPtrValue(m_overlayOpeningStoryboard, overlayOpeningStoryboard.Get());
        }

        IFC_RETURN(PropertyValue::CreateFromString(wrl_wrappers::HStringReference(L"OverlayClosingAnimation").Get(), boxedResourceKey.ReleaseAndGetAddressOf()));
        IFC_RETURN(resourcesMap->HasKey(boxedResourceKey.Get(), &hasKey));
        if (hasKey)
        {
            ctl::ComPtr<IInspectable> boxedResource;
            IFC_RETURN(resourcesMap->Lookup(boxedResourceKey.Get(), &boxedResource));

            ctl::ComPtr<xaml_animation::IStoryboard> overlayOpeningStoryboard;
            IFC_RETURN(boxedResource.As(&overlayOpeningStoryboard));

            SetPtrValue(m_overlayClosingStoryboard, overlayOpeningStoryboard.Get());
        }
    }

    // Initialize header visibility
    IFC_RETURN(UpdateHeaderPresenterVisibility());

    IFC_RETURN(SetupElementPopup());
    IFC_RETURN(SetupElementOutsidePopupChild());

    IFC_RETURN(SetupOtherEventHandlers());
    IFC_RETURN(SetupElementPopupChild());

    IFC_RETURN(SetupElementPopupChildCanvas());

    if (IsEditable())
    {
        IFC_RETURN(SetupEditableMode());
        IFC_RETURN(CreateEditableContentPresenterTextBlock());
    }

    if (IsInline())
    {
        IFC_RETURN(ForceApplyInlineLayoutUpdate());
    }

    IFC_RETURN(ChangeVisualState(false));

    return S_OK;
}

_Check_return_ HRESULT ComboBox::ReleaseMembers()
{
    HRESULT hr = S_OK;

    m_tpElementPopupChild.Clear();
    m_tpEmptyContent.Clear();
    m_tpElementPopupChildCanvas.Clear();
    m_tpElementOutsidePopup.Clear();

    if (m_tpFlyoutButtonPart)
    {
        IFC(DetachHandler(m_epFlyoutButtonClickHandler, m_tpFlyoutButtonPart));
        // Cleared by ComboBoxGenerated::OnApplyTemplate
    }
    if (m_tpItemsPresenterHostParent)
    {
        IFC(DetachHandler(m_epHostParentSizeChangedHandler, m_tpItemsPresenterHostParent));
        m_tpItemsPresenterHostParent.Clear();
    }
    if (m_tpItemsPresenterPart)
    {
        IFC(DetachHandler(m_epItemsPresenterSizeChangedHandler, m_tpItemsPresenterPart));
        // Cleared by ComboBoxGenerated::OnApplyTemplate
    }
    if (m_tpClosedStoryboard)
    {
        IFC(m_tpClosedStoryboard.Cast<Storyboard>()->remove_Completed(m_closedStateStoryboardCompletedToken));
        m_tpClosedStoryboard.Clear();
    }
    if (m_tpPopupPart && m_epPopupClosedHandler)
    {
        IFC(m_epPopupClosedHandler.DetachEventHandler(m_tpPopupPart.Get()));
    }

    m_tpHeaderContentPresenterPart.Clear();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::SetupEditableMode()
{
    if (m_isEditModeConfigured || !m_tpEditableTextPart)
    {
        return S_OK;
    }

    ctl::ComPtr<IInspectable> spSelectedItem;
    IFC_RETURN(get_SelectedItem(&spSelectedItem));
    IFC_RETURN(UpdateEditableTextBox(spSelectedItem.Get(), false /*selectText*/, false /*selectAll*/));

    auto pEditableTextPartAsTextBox = m_tpEditableTextPart.Cast<TextBox>();

    // We make the TextBox visible so that UIA clients can identify this one as an editable
    // ComboBox, but we keep the TextBox invisible and disabled until is actually needed.
    IFC_RETURN(pEditableTextPartAsTextBox->put_Visibility(xaml::Visibility_Visible));
    IFC_RETURN(pEditableTextPartAsTextBox->put_Width(0.0f));
    IFC_RETURN(pEditableTextPartAsTextBox->put_Height(0.0f));
    IFC_RETURN(m_spEditableTextPreviewKeyDownHandler.AttachEventHandler(
        pEditableTextPartAsTextBox,
        std::bind(&ComboBox::OnTextBoxPreviewKeyDown, this, _1, _2)));

    IFC_RETURN(m_spEditableTextKeyDownHandler.AttachEventHandler(
        pEditableTextPartAsTextBox,
        std::bind(&ComboBox::OnKeyDownPrivate, this, _1, _2)));

    IFC_RETURN(m_spEditableTextTextChangedHandler.AttachEventHandler(
        pEditableTextPartAsTextBox,
        std::bind(&ComboBox::OnTextBoxTextChanged, this, _1, _2)));

    IFC_RETURN(m_spEditableTextCandidateWindowBoundsChangedEventHandler.AttachEventHandler(
        pEditableTextPartAsTextBox,
        std::bind(&ComboBox::OnTextBoxCandidateWindowBoundsChanged, this, _1, _2)));

    IFC_RETURN(m_spEditableTextSizeChangedHandler.AttachEventHandler(
        pEditableTextPartAsTextBox,
        std::bind(&ComboBox::OnTextBoxSizeChanged, this, _1, _2)));

    m_spEditableTextPointerPressedEventHandler.Attach(
        new ClassMemberEventHandler
        <ComboBox, IComboBox, IPointerEventHandler, IInspectable, IPointerRoutedEventArgs>
        (this, &ComboBox::OnTextBoxPointerPressedPrivate));

    m_spEditableTextTappedEventHandler.Attach(
        new ClassMemberEventHandler
        <ComboBox, IComboBox, xaml_input::ITappedEventHandler, IInspectable, ITappedRoutedEventArgs>
        (this, &ComboBox::OnTextBoxTapped));

    PointerPressedEventSourceType* pPointerPressedEventSource = nullptr;

    IFC_RETURN(pEditableTextPartAsTextBox->GetPointerPressedEventSourceNoRef(&pPointerPressedEventSource));
    IFC_RETURN(pPointerPressedEventSource->AddHandler(m_spEditableTextPointerPressedEventHandler.Get(), true /* handledEventsToo */));

    TappedEventSourceType* pTappedEventSource = nullptr;

    IFC_RETURN(pEditableTextPartAsTextBox->GetTappedEventSourceNoRef(&pTappedEventSource));
    IFC_RETURN(pTappedEventSource->AddHandler(m_spEditableTextTappedEventHandler.Get(), true /* handledEventsToo */));

    if (m_tpDropDownOverlayPart)
    {
        IFC_RETURN(m_spDropDownOverlayPointerEnteredHandler.AttachEventHandler(
            m_tpDropDownOverlayPart.Cast<Border>(),
            std::bind(&ComboBox::OnDropDownOverlayPointerEntered, this, _1, _2)));

        IFC_RETURN(m_spDropDownOverlayPointerExitedHandler.AttachEventHandler(
            m_tpDropDownOverlayPart.Cast<Border>(),
            std::bind(&ComboBox::OnDropDownOverlayPointerExited, this, _1, _2)));

        IFC_RETURN(m_tpDropDownOverlayPart.Cast<Border>()->put_Visibility(xaml::Visibility_Visible));
    }

    // Tells the selector to allow Custom Values.
    SetAllowCustomValues(true /*allow*/);

    m_restoreIndexSet = false;
    m_indexToRestoreOnCancel = -1;
    ResetSearch();
    ResetSearchString();

    wrl::ComPtr<xaml_controls::IInputValidationContext> context;
    IFC_RETURN(get_ValidationContext(&context));
    IFC_RETURN(pEditableTextPartAsTextBox->put_ValidationContext(context.Get()));

    wrl::ComPtr<xaml_controls::IInputValidationCommand> command;
    IFC_RETURN(get_ValidationCommand(&command));
    IFC_RETURN(pEditableTextPartAsTextBox->put_ValidationCommand(command.Get()));

    if (m_tpPopupPart)
    {
        IFC_RETURN(m_tpPopupPart.Cast<Popup>()->put_OverlayInputPassThroughElement(this));
    }

    m_isEditModeConfigured = true;

    return S_OK;
}

_Check_return_ HRESULT ComboBox::DisableEditableMode()
{
    if (!m_isEditModeConfigured || !m_tpEditableTextPart)
    {
        return S_OK;
    }

    if (m_tpPopupPart)
    {
        IFC_RETURN(m_tpPopupPart.Cast<Popup>()->put_OverlayInputPassThroughElement(nullptr));
    }

    auto pEditableTextPartAsTextBox = m_tpEditableTextPart.Cast<TextBox>();
    auto pEditableTextPartAsI = iinspectable_cast(pEditableTextPartAsTextBox);

    // We hide the TextBox in order to prevent UIA clients from thinking this is an editable ComboBox.
    IFC_RETURN(pEditableTextPartAsTextBox->put_Visibility(xaml::Visibility_Collapsed));
    IFC_RETURN(pEditableTextPartAsTextBox->put_Width(0.0f));
    IFC_RETURN(pEditableTextPartAsTextBox->put_Height(0.0f));

    IFC_RETURN(m_spEditableTextPreviewKeyDownHandler.DetachEventHandler(pEditableTextPartAsI));
    IFC_RETURN(m_spEditableTextKeyDownHandler.DetachEventHandler(pEditableTextPartAsI));
    IFC_RETURN(m_spEditableTextTextChangedHandler.DetachEventHandler(pEditableTextPartAsI));
    IFC_RETURN(m_spEditableTextPointerPressedHandler.DetachEventHandler(pEditableTextPartAsI));
    IFC_RETURN(m_spEditableTextCandidateWindowBoundsChangedEventHandler.DetachEventHandler(pEditableTextPartAsI));
    IFC_RETURN(m_spEditableTextSizeChangedHandler.DetachEventHandler(pEditableTextPartAsI));

    if (m_tpDropDownOverlayPart)
    {
        auto pDropDownOverlayPartAsI = iinspectable_cast(m_tpDropDownOverlayPart.Cast<Border>());

        IFC_RETURN(m_spDropDownOverlayPointerEnteredHandler.DetachEventHandler(pDropDownOverlayPartAsI));
        IFC_RETURN(m_spDropDownOverlayPointerExitedHandler.DetachEventHandler(pDropDownOverlayPartAsI));

        IFC_RETURN(m_tpDropDownOverlayPart.Cast<Border>()->put_Visibility(xaml::Visibility_Collapsed));
    }

    PointerPressedEventSourceType* pPointerPressedEventSource = nullptr;

    IFC_RETURN(pEditableTextPartAsTextBox->GetPointerPressedEventSourceNoRef(&pPointerPressedEventSource));
    IFC_RETURN(pPointerPressedEventSource->RemoveHandler(m_spEditableTextPointerPressedEventHandler.Get()));

    TappedEventSourceType* pTappedEventSource = nullptr;

    IFC_RETURN(pEditableTextPartAsTextBox->GetTappedEventSourceNoRef(&pTappedEventSource));
    IFC_RETURN(pTappedEventSource->RemoveHandler(m_spEditableTextTappedEventHandler.Get()));

    ResetSearch();
    ResetSearchString();
    m_selectAllOnTouch = false;
    m_openPopupOnTouch = false;
    m_shouldMoveFocusToTextBox = false;
    m_restoreIndexSet = false;
    m_indexToRestoreOnCancel = -1;

    if (m_customValueRef)
    {
        m_customValueRef.Reset();
        IFC_RETURN(SetContentPresenter(-1));
        put_SelectedItem(nullptr);
    }

    // Tells the selector to prevent Custom Values.
    SetAllowCustomValues(false /*allow*/);

    m_tpEditableTextPart->put_Text(nullptr);

    m_isEditModeConfigured = false;

    return S_OK;
}

_Check_return_ HRESULT ComboBox::SetupElementPopupChildCanvas()
{
    HRESULT hr = S_OK;
    BOOLEAN bPeggedElementPopupChild = FALSE;
    ctl::ComPtr<wfc::IVector<xaml::UIElement *>> spElementPopupChildCanvasChildren;

    if (m_tpElementPopupChildCanvas && m_tpElementOutsidePopup)
    {
        // We're about to remove an object from the tree, (when we override m_tpElementPopup::Child),
        // which will leave it unprotected from GC.  Peg it until it's back in the tree.
        m_tpElementPopupChild.Cast<FrameworkElement>()->UpdatePeg(true);
        bPeggedElementPopupChild = TRUE;

        IFC(m_tpPopupPart.Get()->put_Child(m_tpElementPopupChildCanvas.Cast<Canvas>()));
        IFC(m_tpElementPopupChildCanvas.Cast<Canvas>()->get_Children(&spElementPopupChildCanvasChildren));
        if (spElementPopupChildCanvasChildren)
        {
            //
            // In the case of windowed popups, we don't want to put up a giant transparent hwnd over the entire desktop
            // holding the light dismiss element, because that will block clicks targeted at the host app (in the case
            // of islands), clicks on other apps, or clicks meant to resize the window.
            //
            // Conveniently, the CComboBoxLightDismiss element is a Canvas, and its MeasureOverride reports 0x0 as the
            // desired size (see CCanvas::MeasureOverride). Windowed popups look at desired size when sizing the hwnd,
            // so it will ignore the giant CComboBoxLightDismiss element. We then end up with a small hwnd for the
            // ComboBox popup as desired.
            //
            // When it comes time to hit test, a click over the Xaml island will pass the point through the Xaml tree,
            // where it will hit the giant CComboBoxLightDismiss canvas under the popup root.
            //
            IFC(spElementPopupChildCanvasChildren->Append(m_tpElementOutsidePopup.Cast<Canvas>()));

            IFC(spElementPopupChildCanvasChildren->Append(m_tpElementPopupChild.Cast<FrameworkElement>()));
        }
    }

Cleanup:

    if( bPeggedElementPopupChild )
    {
        m_tpElementPopupChild.Cast<FrameworkElement>()->UpdatePeg(false);
    }

    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::SetupElementPopup()
{
    if (m_tpPopupPart)
    {
        ctl::ComPtr<xaml::IUIElement> spElementPopupChild;
        ctl::ComPtr<DirectUI::ComboBoxLightDismiss> spElementOutsidePopup;

        IFC_RETURN(m_tpPopupPart.Get()->get_Child(&spElementPopupChild));
        IFC_RETURN(SetPtrValueWithQI(m_tpElementPopupChild, spElementPopupChild.Get()));

        IFC_RETURN(ctl::make(&spElementOutsidePopup));
        IFC_RETURN(spElementOutsidePopup->put_Owner(this));
        SetPtrValue(m_tpElementOutsidePopup, spElementOutsidePopup);

        // For the most part ComboBox implements its own light dismiss logic, but it doesn't handle window size changes.
        // We use Popup's light dismiss logic for window size changes.
        IFC_RETURN(m_tpPopupPart.Cast<Popup>()->SetDismissalTriggerFlags(Popup::DismissalTriggerFlags::WindowSizeChange));
        IFC_RETURN(m_epPopupClosedHandler.AttachEventHandler(m_tpPopupPart.Get(), std::bind(&ComboBox::OnPopupClosed, this, _1, _2)));

        // ComboBoxes should always use a windowed popup to break out of the bounds of the main island/window.
        IFC_RETURN(m_tpPopupPart.Cast<Popup>()->put_ShouldConstrainToRootBounds(false));

        if (m_tpElementPopupChild)
        {
            IFC_RETURN(m_tpElementPopupChild.Get()->put_MaxHeight(CarouselPanel::m_InitialMeasureHeight));
            ctl::ComPtr<xaml::IUIElement> popupScrollViewer;
            ctl::ComPtr<xaml_controls::IControl> popupContent;
            ctl::ComPtr<xaml_controls::IBorder> popupChildAsBorder = m_tpElementPopupChild.AsOrNull<IBorder>();
            if (popupChildAsBorder)
            {
                IFC_RETURN(popupChildAsBorder.Get()->get_Child(&popupScrollViewer));
                ctl::ComPtr<xaml_controls::IContentControl> popupScrollViewerAsCC = popupScrollViewer.AsOrNull<IContentControl>();
                if (popupScrollViewerAsCC)
                {
                    IFC_RETURN(popupScrollViewerAsCC.Get()->get_Content(&popupContent));
                    ctl::ComPtr<xaml::IFrameworkElement> popupContentAsFE = popupContent.AsOrNull<IFrameworkElement>();
                    if (popupContentAsFE)
                    {
                        SetPtrValue(m_tpElementPopupContent, popupContentAsFE);
                        IFC_RETURN(popupContentAsFE.Get()->get_Margin(&popupContentMargin));
                    }
                }
            }
        }

        // Reconfigure the popup's overlay if it is enabled.
        IFC_RETURN(ReevaluateIsOverlayVisible());
    }
    else
    {
        m_tpElementPopupChild.Clear();
        m_tpElementOutsidePopup.Clear();
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::SetupElementOutsidePopupChild()
{
    HRESULT hr = S_OK;

    if (m_tpElementOutsidePopup)
    {
        ctl::ComPtr<SolidColorBrush> spTransparentBrush;
        wu::Color transparentColor;
        ctl::ComPtr<xaml_input::IPointerEventHandler> spElementOutSidePopupPointerPressedHandler;

        IFC(ctl::make(&spTransparentBrush));
        transparentColor.A = 0;
        transparentColor.B = 255;
        transparentColor.G = 255;
        transparentColor.R = 255;
        IFC(spTransparentBrush->put_Color(transparentColor));
        IFC(m_tpElementOutsidePopup.Cast<Canvas>()->put_Background(spTransparentBrush.Get()));
        spElementOutSidePopupPointerPressedHandler.Attach(
            new ClassMemberEventHandler<
            ComboBox,
            xaml_controls::IComboBox,
            xaml_input::IPointerEventHandler,
            IInspectable,
            xaml_input::IPointerRoutedEventArgs>(this, &ComboBox::OnElementOutsidePopupPointerPressed));
        IFC(m_tpElementOutsidePopup.Cast<Canvas>()->add_PointerPressed(spElementOutSidePopupPointerPressedHandler.Get(), &m_pElementOutSidePopupPointerPressedToken));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::SetupElementPopupChild()
{
    ctl::ComPtr<xaml_input::IKeyEventHandler> spElementPopupChildKeyDownHandler;
    ctl::ComPtr<xaml_input::IKeyEventHandler> spElementPopupChildKeyUpHandler;
    ctl::ComPtr<xaml::IRoutedEventHandler> spElementPopupChildGotFocusHandler;
    ctl::ComPtr<xaml::IRoutedEventHandler> spElementPopupChildLostFocusHandler;
    ctl::ComPtr<xaml_input::IPointerEventHandler> spElementPopupChildPointerEnteredHandler;
    ctl::ComPtr<xaml_input::IPointerEventHandler> spElementPopupChildPointerExitedHandler;
    ctl::ComPtr<xaml::ISizeChangedEventHandler> spElementPopupChildSizeChangedHandler;
    ctl::ComPtr<xaml::IRoutedEventHandler> spElementPopupChildLoadedHandler;
    ctl::ComPtr<DirectUI::Canvas> spElementPopupChildCanvas;
    ctl::ComPtr<wf::ITypedEventHandler<xaml::UIElement*, xaml_input::CharacterReceivedRoutedEventArgs*>> spElementPopupChildCharacterReceivedHandler;


    if (!m_tpElementPopupChild)
    {
        m_tpElementPopupChildCanvas.Clear();
        return S_OK;
    }

    // Wire up CharacterReceived in our child so that we can handle typeahead if it's enabled.
    // We know we can do this here, as with the other events, because we explicitly create m_tpElementPopupChild in SetupElementPopup.
    spElementPopupChildCharacterReceivedHandler.Attach(
        new ClassMemberEventHandler<
        ComboBox,
        xaml_controls::IComboBox,
        wf::ITypedEventHandler<xaml::UIElement*, xaml_input::CharacterReceivedRoutedEventArgs*>,
        IUIElement,
        xaml_input::ICharacterReceivedRoutedEventArgs>(this, &ComboBox::OnPopupCharacterReceived));
    IFC_RETURN(m_tpElementPopupChild.Cast<Canvas>()->add_CharacterReceived(spElementPopupChildCharacterReceivedHandler.Get(), &m_pElementPopupChildCharacterReceivedToken));

    //Key down event handler.
    // We have to hook the visual tree in two spots to catch all the keyboard events.  Once at the ComboBox and once for
    // the visual root of the popup control.
    spElementPopupChildKeyDownHandler.Attach(new ClassMemberEventHandler<
        ComboBox,
        xaml_controls::IComboBox,
        xaml_input::IKeyEventHandler,
        IInspectable,
        xaml_input::IKeyRoutedEventArgs>(this, &ComboBox::OnKeyDownPrivate));
    IFC_RETURN(m_tpElementPopupChild.Cast<Canvas>()->add_KeyDown(spElementPopupChildKeyDownHandler.Get(), &m_pElementPopupChildKeyDownToken));

    spElementPopupChildKeyUpHandler.Attach(new ClassMemberEventHandler<
        ComboBox,
        xaml_controls::IComboBox,
        xaml_input::IKeyEventHandler,
        IInspectable,
        xaml_input::IKeyRoutedEventArgs>(this, &ComboBox::OnKeyUpPrivate));
    IFC_RETURN(m_tpElementPopupChild.Cast<Canvas>()->add_KeyUp(spElementPopupChildKeyUpHandler.Get(), &m_pElementPopupChildKeyUpToken));

    // Got focus event handler
    spElementPopupChildGotFocusHandler.Attach(
        new ClassMemberEventHandler<
        ComboBox,
        xaml_controls::IComboBox,
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs>(this, &ComboBox::OnElementPopupChildGotFocus));

    IFC_RETURN(m_tpElementPopupChild.Cast<Canvas>()->add_GotFocus(spElementPopupChildGotFocusHandler.Get(), &m_pElementPopupChildGotFocusToken));

    // Leave focus event handler
    spElementPopupChildLostFocusHandler.Attach(
        new ClassMemberEventHandler<
        ComboBox,
        xaml_controls::IComboBox,
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs>(this, &ComboBox::OnElementPopupChildLostFocus));

    IFC_RETURN(m_tpElementPopupChild.Cast<Canvas>()->add_LostFocus(spElementPopupChildLostFocusHandler.Get(), &m_pElementPopupChildLostFocusToken));


    spElementPopupChildPointerEnteredHandler.Attach(
        new ClassMemberEventHandler<
        ComboBox,
        xaml_controls::IComboBox,
        xaml_input::IPointerEventHandler,
        IInspectable,
        xaml_input::IPointerRoutedEventArgs>(this, &ComboBox::OnElementPopupChildPointerEntered));

    IFC_RETURN(m_tpElementPopupChild.Cast<Canvas>()->add_PointerEntered(spElementPopupChildPointerEnteredHandler.Get(), &m_pElementPopupChildPointerEnteredToken));

    spElementPopupChildPointerExitedHandler.Attach(
        new ClassMemberEventHandler<
        ComboBox,
        xaml_controls::IComboBox,
        xaml_input::IPointerEventHandler,
        IInspectable,
        xaml_input::IPointerRoutedEventArgs>(this, &ComboBox::OnElementPopupChildPointerExited));

    IFC_RETURN(m_tpElementPopupChild.Cast<Canvas>()->add_PointerExited(spElementPopupChildPointerExitedHandler.Get(), &m_pElementPopupChildPointerExitedToken));

    spElementPopupChildSizeChangedHandler.Attach(
        new ClassMemberEventHandler<
        ComboBox,
        xaml_controls::IComboBox,
        xaml::ISizeChangedEventHandler,
        IInspectable,
        xaml::ISizeChangedEventArgs>(this, &ComboBox::OnElementPopupChildSizeChanged));
    IFC_RETURN(m_tpElementPopupChild.Cast<Canvas>()->add_SizeChanged(spElementPopupChildSizeChangedHandler.Get(), &m_pElementPopupChildISizeChangedToken));

    spElementPopupChildLoadedHandler.Attach(
        new ClassMemberEventHandler<
        ComboBox,
        xaml_controls::IComboBox,
        xaml::IRoutedEventHandler,
        IInspectable,
        xaml::IRoutedEventArgs>(this, &ComboBox::OnElementPopupChildLoaded));
    IFC_RETURN(m_tpElementPopupChild.Cast<Canvas>()->add_Loaded(spElementPopupChildLoadedHandler.Get(), &m_pElementPopupChildLoadedToken));

    IFC_RETURN(ctl::make(&spElementPopupChildCanvas));
    SetPtrValue(m_tpElementPopupChildCanvas, spElementPopupChildCanvas);

    return S_OK;
}

_Check_return_ HRESULT ComboBox::SetupOtherEventHandlers()
{
    HRESULT hr = S_OK;

    IFC(SetupVisualStateEventHandlerForDropDownClosedState());

    if (m_tpFlyoutButtonPart)
    {
        IFC(m_epFlyoutButtonClickHandler.AttachEventHandler(m_tpFlyoutButtonPart.Get(),
            std::bind(&ComboBox::OnFlyoutButtonClick, this, _1, _2)));
    }

    if (m_tpItemsPresenterHostParent)
    {
        IFC(m_epHostParentSizeChangedHandler.AttachEventHandler(m_tpItemsPresenterHostParent.Get(),
            std::bind(&ComboBox::OnItemsPresenterHostParentSizeChanged, this, _1, _2)));
    }

    if (m_tpItemsPresenterPart)
    {
        IFC(m_epItemsPresenterSizeChangedHandler.AttachEventHandler(m_tpItemsPresenterPart.AsOrNull<IFrameworkElement>().Get(),
            std::bind(&ComboBox::OnItemsPresenterSizeChanged, this, _1, _2)));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::SetupVisualStateEventHandlerForDropDownClosedState()
{
    HRESULT hr = S_OK;

    if (!m_tpClosedStoryboard)
    {
        BOOLEAN bResult = FALSE;
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> spClosedStateStoryboardCompletedHandler;
        ctl::ComPtr<IStoryboard> spStoryboardAsI;
        ctl::ComPtr<IVisualState> spState;

        IFC(VisualStateManager::TryGetState(this, L"Closed",  nullptr, &spState, &bResult));
        if (bResult && spState)
        {
            spClosedStateStoryboardCompletedHandler.Attach(
                new ClassMemberEventHandler<
                    ComboBox,
                    IComboBox,
                    wf::IEventHandler<IInspectable*>,
                    IInspectable,
                    IInspectable>(this, &ComboBox::OnClosedStateStoryboardCompleted));
            IFC(spState->get_Storyboard(&spStoryboardAsI));
            if(spStoryboardAsI)
            {
                SetPtrValue(m_tpClosedStoryboard, spStoryboardAsI.Get());    // gets ownership
                IFC(m_tpClosedStoryboard.Cast<Storyboard>()->add_Completed(spClosedStateStoryboardCompletedHandler.Get(), &m_closedStateStoryboardCompletedToken));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

#pragma endregion Apply Template

#pragma region Update Visuals

// Change to the correct visual state for the ComboBox.
_Check_return_ HRESULT ComboBox::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool useTransitions)
{
    BOOLEAN isEnabled = FALSE;
    BOOLEAN isPointerOver = m_IsPointerOverMain || m_IsPointerOverPopup;
    BOOLEAN ignored = FALSE;
    BOOLEAN isDropDownOpen = FALSE;
    BOOLEAN isSelectionActive = FALSE;
    INT32 selectedIndex = -1;

    IFC_RETURN(get_IsEnabled(&isEnabled));
    IFC_RETURN(get_IsSelectionActive(&isSelectionActive));
    IFC_RETURN(get_IsDropDownOpen(&isDropDownOpen));
    IFC_RETURN(get_SelectedIndex(&selectedIndex));

    const auto focusManager = VisualTree::GetFocusManagerForElement(GetHandle());

    // Ingores pressed visual over the entire control when pointer is over the DropDown button used for Editable Mode.
    bool ignorePressedVisual = false;

    // EditableModeStates VisualStateGroup.
    if (IsEditable())
    {
        bool editableTextHasFocus = EditableTextHasFocus();

        if (m_IsPointerOverDropDownOverlay)
        {
            if (m_bIsPressed)
            {
                ignorePressedVisual = true;
                IFC_RETURN(GoToState(useTransitions, editableTextHasFocus ? L"TextBoxFocusedOverlayPressed" : L"TextBoxOverlayPressed", &ignored));
            }
            else
            {
                IFC_RETURN(GoToState(useTransitions, editableTextHasFocus ? L"TextBoxFocusedOverlayPointerOver" : L"TextBoxOverlayPointerOver", &ignored));
            }
        }
        else
        {
            IFC_RETURN(GoToState(useTransitions, editableTextHasFocus ? L"TextBoxFocused" : L"TextBoxUnfocused", &ignored));
        }
    }

    // CommonStates VisualStateGroup.
    if (!isEnabled)
    {
        IFC_RETURN(GoToState(useTransitions, L"Disabled", &ignored));
    }
    else if (IsInline() && isDropDownOpen)
    {
        IFC_RETURN(GoToState(useTransitions, L"Highlighted", &ignored));
    }
    else if (m_bIsPressed && !ignorePressedVisual)
    {
        IFC_RETURN(GoToState(useTransitions, L"Pressed", &ignored));
    }
    else if (isPointerOver)
    {
        IFC_RETURN(GoToState(useTransitions, L"PointerOver", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"Normal", &ignored));
    }

    // FocusStates VisualStateGroup.
    if (!isSelectionActive || !isEnabled || !focusManager->IsPluginFocused())
    {
        IFC_RETURN(GoToState(useTransitions, L"Unfocused", &ignored));
    }
    else if (isDropDownOpen)
    {
        IFC_RETURN(GoToState(useTransitions, L"FocusedDropDown", &ignored));
    }
    else
    {
        auto focusState = xaml::FocusState_Unfocused;

        IFC_RETURN(get_FocusState(&focusState));

        if (xaml::FocusState_Unfocused == focusState)
        {
            IFC_RETURN(GoToState(useTransitions, L"Unfocused", &ignored));
        }
        else if (xaml::FocusState_Pointer == focusState)

        {
            IFC_RETURN(GoToState(useTransitions, L"PointerFocused", &ignored));
        }
        else
        {
            if (m_bIsPressed)
            {
                IFC_RETURN(GoToState(useTransitions, L"FocusedPressed", &ignored));
            }
            else
            {
                IFC_RETURN(GoToState(useTransitions, L"Focused", &ignored));
            }
        }
    }

    // PresenterStates VisualStateGroup.
    if (!IsInline())
    {
        // Either inline mode is not supported based on the template parts available,
        // or the number of items is too large for use of inline mode.
        IFC_RETURN(GoToState(useTransitions, L"Full", &ignored));
    }
    else if (m_bIsPressed || isDropDownOpen || m_isDropDownClosing || selectedIndex >= 0)
    {
        IFC_RETURN(GoToState(useTransitions, L"InlineNormal", &ignored));
    }
    else
    {
        // drop-down is fully closed and nothing is selected
        IFC_RETURN(GoToState(useTransitions, L"InlinePlaceholder", &ignored));
    }

    // DropDownStates VisualStateGroup.
    if (!IsFullMode())
    {
        if (m_isDropDownClosing)
        {
            IFC_RETURN(GoToState(useTransitions, L"Closed", &ignored));
        }
        else if (isDropDownOpen && (IsSmallFormFactor() || m_bPopupHasBeenArrangedOnce))
        {
            // Note: The non-small-form-factor combo box uses a popup to display it's
            // items. We can't transition to the Opened state until the popup has been
            // laid out.
            IFC_RETURN(GoToState(useTransitions, L"Opened", &ignored));
        }
    }

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
    // HeaderStates VisualStateGroup.
    xaml_controls::ControlHeaderPlacement headerPlacement = xaml_controls::ControlHeaderPlacement_Top;
    IFC_RETURN(get_HeaderPlacement(&headerPlacement));

    switch (headerPlacement)
    {
        case DirectUI::ControlHeaderPlacement::Top:
            IFC_RETURN(GoToState(useTransitions, L"TopHeader", &ignored));
            break;

        case DirectUI::ControlHeaderPlacement::Left:
            IFC_RETURN(GoToState(useTransitions, L"LeftHeader", &ignored));
            break;
    }
#endif

    checked_cast<CControl>(GetHandle())->EnsureValidationVisuals();

    return S_OK;
}

_Check_return_ HRESULT ComboBox::SetContentPresenter(
    _In_ INT index, _In_ bool forceSelectionBoxToNull) noexcept
{
    BOOLEAN bGeneratedComboBoxItem = FALSE;
    ctl::ComPtr<IInspectable> spItem;
    ctl::ComPtr<xaml_controls::IItemContainerGenerator> spGenerator;
    ctl::ComPtr<xaml::IDependencyObject> spContainer;
    ctl::ComPtr<xaml::IDependencyObject> spGeneratedComboBoxItemAsDO;
    ctl::ComPtr<xaml_controls::IComboBoxItem> spComboBoxItem;
    ctl::ComPtr<IInspectable> spContent;
    ctl::ComPtr<IDataTemplate> spDataTemplate;
    ctl::ComPtr<IDataTemplateSelector> spDataTemplateSelector;
    xaml_primitives::GeneratorPosition generatorPosition;

    ASSERT(!IsInline(), L"ContentPresenter is not used in inline mode.");

    // Avoid reentrancy.
    if (m_preparingContentPresentersElement)
    {
        return S_OK;
    }

    if (m_tpSwappedOutComboBoxItem)
    {
        if (m_tpContentPresenterPart)
        {
            IFC_RETURN(m_tpContentPresenterPart->get_Content(&spContent));
            {
                IFC_RETURN(m_tpContentPresenterPart->put_Content(NULL));
                IFC_RETURN(m_tpSwappedOutComboBoxItem.Cast<ComboBoxItem>()->put_Content(spContent.Get()));
                m_tpSwappedOutComboBoxItem.Clear();
            }
        }
    }

    IFC_RETURN(get_ItemContainerGenerator(&spGenerator));
    ItemContainerGenerator* pItemContainerGenerator = spGenerator.Cast<ItemContainerGenerator>();
    if (m_iLastGeneratedItemIndexforFaceplate > 0)
    {
        // This container was generated just for the purpose of extracting Content and ContentTemplate.
        // This is the case where we generated an item which was its own container (e.g. defined in XAML or code behind).
        // We keep this until the next item is being put on faceplate or popup is opened so that ItemContainerGenerator.ContainerFromIndex returns the
        // correct container for this item which a developer would expect.
        // We need to remove this item once popup opens (or another item takes its place on faceplate)
        // so that virtualizing panel underneath does not get items out of order.
        // We want to remove instead of recycle because we do not want to change the collection order by reusing containers for different data.
        IFC_RETURN(pItemContainerGenerator->GeneratorPositionFromIndex(m_iLastGeneratedItemIndexforFaceplate, &generatorPosition));
        if (generatorPosition.Offset == 0 && generatorPosition.Index >= 0)
        {
            // Only remove if the position returned by Generator is correct
            IFC_RETURN(pItemContainerGenerator->Remove(generatorPosition, 1));
        }

        m_iLastGeneratedItemIndexforFaceplate = -1;
    }

    if (index == -1)
    {
        if (m_tpContentPresenterPart)
        {
            IFC_RETURN(m_tpContentPresenterPart->put_ContentTemplateSelector(NULL));
            IFC_RETURN(m_tpContentPresenterPart->put_ContentTemplate(NULL));
            IFC_RETURN(m_tpContentPresenterPart->put_Content(m_tpEmptyContent.Get()));
        }

        // Only reset the SelectionBoxItem if a custom value is not selected.
        if (forceSelectionBoxToNull || !IsEditable() || !m_customValueRef)
        {
            IFC_RETURN(put_SelectionBoxItem(NULL));
        }

        IFC_RETURN(put_SelectionBoxItemTemplate(NULL));
        return S_OK;
    }

    if (m_tpContentPresenterPart)
    {
        IFC_RETURN(m_tpContentPresenterPart.Get()->put_Content(NULL));
    }

    IFC_RETURN(ContainerFromIndex(index, &spContainer));
    IFC_RETURN(spContainer.As(&spComboBoxItem));

    if (!spComboBoxItem)
    {
        BOOLEAN isNewlyRealized = FALSE;
        IFC_RETURN(pItemContainerGenerator->GeneratorPositionFromIndex(index, &generatorPosition));
        IFC_RETURN(pItemContainerGenerator->StartAt(generatorPosition, xaml_primitives::GeneratorDirection_Forward, TRUE));
        IFC_RETURN(pItemContainerGenerator->GenerateNext(&isNewlyRealized, &spGeneratedComboBoxItemAsDO));
        IFC_RETURN(pItemContainerGenerator->Stop());
        m_preparingContentPresentersElement = true;
        SetPtrValue(m_tpGeneratedContainerForContentPresenter, spGeneratedComboBoxItemAsDO);
        HRESULT hr = pItemContainerGenerator->PrepareItemContainer(spGeneratedComboBoxItemAsDO.Get());
        m_tpGeneratedContainerForContentPresenter.Clear();
        m_preparingContentPresentersElement = false;
        IFC_RETURN(hr);
        IFC_RETURN(spGeneratedComboBoxItemAsDO.As(&spComboBoxItem));
        // We dont want to remove the comboBoxItem if it was created explicitly in XAML and exists in Items collection
        IFC_RETURN(spComboBoxItem.Cast<ComboBoxItem>()->ReadLocalValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_ItemForItemContainer),
            &spItem));
        IFC_RETURN(IsItemItsOwnContainer(spItem.Get(), &bGeneratedComboBoxItem));
        bGeneratedComboBoxItem = !bGeneratedComboBoxItem;
        m_iLastGeneratedItemIndexforFaceplate = index;
    }

    IFC_RETURN(spComboBoxItem.Cast<ComboBoxItem>()->get_Content(&spContent));
    {
        // Because we can't keep UIElement in 2 different place
        // we need to reset ComboBoxItem.Content property. And we need to do it for UIElement only
        if (ctl::is<IUIElement>(spContent))
        {
            IFC_RETURN(spComboBoxItem.Cast<ComboBoxItem>()->put_Content(NULL));
            if (!bGeneratedComboBoxItem)
            {
                SetPtrValue(m_tpSwappedOutComboBoxItem, spComboBoxItem);
            }
        }

        IFC_RETURN(spComboBoxItem.Cast<ComboBoxItem>()->put_IsPointerOver(FALSE));
        IFC_RETURN(spComboBoxItem.Cast<ComboBoxItem>()->ChangeVisualState(TRUE));

        // We want the item displayed in the 'selected item' ContentPresenter to have the same visual representation as the
        // items in the Popup's StackPanel, to do that we copy the DataTemplate of the ComboBoxItem.
        IFC_RETURN(spComboBoxItem.Cast<ComboBoxItem>()->get_ContentTemplate(&spDataTemplate));
        IFC_RETURN(spComboBoxItem.Cast<ComboBoxItem>()->get_ContentTemplateSelector(&spDataTemplateSelector));
        if (m_tpContentPresenterPart)
        {
            IFC_RETURN(m_tpContentPresenterPart->put_Content(spContent.Get()));
            IFC_RETURN(m_tpContentPresenterPart->put_ContentTemplate(spDataTemplate.Get()));
            IFC_RETURN(m_tpContentPresenterPart->put_ContentTemplateSelector(spDataTemplateSelector.Get()));
            if (!spDataTemplate)
            {
                IFC_RETURN(m_tpContentPresenterPart.Cast<ContentPresenter>()->get_SelectedContentTemplate(&spDataTemplate));
            }
        }

        IFC_RETURN(put_SelectionBoxItem(spContent.Get()));
        IFC_RETURN(put_SelectionBoxItemTemplate(spDataTemplate.Get()));

    }

    if (bGeneratedComboBoxItem)
    {
        // This container was generated just for the purpose of extracting Content and ContentTemplate
        // It is not connected to the visual tree which might have unintended consequences, so remove it
        IFC_RETURN(pItemContainerGenerator->GeneratorPositionFromIndex(index, &generatorPosition));
        IFC_RETURN(pItemContainerGenerator->Recycle(generatorPosition, 1));
        m_iLastGeneratedItemIndexforFaceplate = -1;
    }

    return S_OK;
}

_Check_return_ HRESULT
ComboBox::UpdateSelectionBoxItemProperties(_In_ INT32 index)
{
    HRESULT hr = S_OK;

    ASSERT(IsInline(), L"When not in inline mode SetContentPresenter should be used instead of UpdateSelectionBoxItemProperties.");

    if (-1 == index)
    {
        IFC(put_SelectionBoxItemTemplate(NULL));
        IFC(put_SelectionBoxItem(NULL));
    }
    else
    {
        ctl::ComPtr<IDataTemplate> spDataTemplate;
        ctl::ComPtr<IInspectable> spItem;
        ctl::ComPtr<xaml::IDependencyObject> spContainer;
        ctl::ComPtr<xaml_controls::IComboBoxItem> spComboBoxItem;

        IFC(ContainerFromIndex(index, &spContainer));

        // The item will not have been realized if SelectedItem/Index was set in xaml and we're being called
        // from OnApplyTemplate, but in that case the item will be realized when the ItemsPresenter
        // is added to the visual tree later in the layout pass, and SetContentPresenter will be called
        // again then.

        if (spContainer)
        {
            IFC(spContainer.As(&spComboBoxItem));
            IFC(spComboBoxItem.Cast<ComboBoxItem>()->get_ContentTemplate(&spDataTemplate));
            IFC(spComboBoxItem.Cast<ComboBoxItem>()->get_Content(&spItem));

            IFC(put_SelectionBoxItem(spItem.Get()));
            IFC(put_SelectionBoxItemTemplate(spDataTemplate.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}
#pragma endregion Update Visuals

#pragma region ItemsControl Overrides

// Determines if the specified item is (or is eligible to be) its own container.
IFACEMETHODIMP ComboBox::IsItemItsOwnContainerOverride(
    _In_ IInspectable* item,
    _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    IFCEXPECT(returnValue);
    *returnValue = !!ctl::value_is<xaml_controls::IComboBoxItem>(item);

Cleanup:
    RRETURN(hr);
}

// Creates or identifies the element that is used to display the given item.
IFACEMETHODIMP ComboBox::GetContainerForItemOverride(
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ComboBoxItem> spComboBoxItem;

    IFCEXPECT(returnValue);
    IFC(ctl::make(&spComboBoxItem));

    IFC(spComboBoxItem.MoveTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ComboBox::PrepareContainerForItemOverride(
    _In_ xaml::IDependencyObject* element,
    _In_ IInspectable* item)
{
    ctl::ComPtr<IInspectable> spSelectedItem;

    IFC_RETURN(ComboBoxGenerated::PrepareContainerForItemOverride(element, item));

    BOOLEAN bIsDropDownOpen = FALSE;
    INT selectedIndex = -1;

    IFC_RETURN(get_IsDropDownOpen(&bIsDropDownOpen));
    IFC_RETURN(get_SelectedIndex(&selectedIndex));
    IFC_RETURN(get_SelectedItem(&spSelectedItem));
    bool areEqual = false;
    IFC_RETURN(PropertyValue::AreEqual(spSelectedItem.Get(), item, &areEqual));
    if (!bIsDropDownOpen && !m_tpSwappedOutComboBoxItem && areEqual && !IsInline())
    {
        IFC_RETURN(SetContentPresenter(selectedIndex));
    }

    return S_OK;
}

IFACEMETHODIMP ComboBox::ClearContainerForItemOverride(
    _In_ xaml::IDependencyObject* element,
    _In_ IInspectable* item)
{
    ctl::ComPtr<xaml_controls::IComboBoxItem> sPassedElement;
    IFCPTR_RETURN(element);
    IFC_RETURN(ctl::do_query_interface<xaml_controls::IComboBoxItem>(sPassedElement, element));
    BOOLEAN bIsItemsHostInvalid = FALSE;
    IFC_RETURN(get_IsItemsHostInvalid(&bIsItemsHostInvalid));

    if (!bIsItemsHostInvalid && sPassedElement.Get() == m_tpSwappedOutComboBoxItem.Get())
    {
        ASSERT(!IsInline(), "m_tpSwappedOutComboBoxItem is not used in inline mode.");
        IFC_RETURN(SetContentPresenter(-1));
    }

    IFC_RETURN(ComboBoxGenerated::ClearContainerForItemOverride(element, item));

    return S_OK;
}

// The first generated container is not part of the visual tree until its prepared
// During prepare container we set IsSelected property on the item being prepared which calls this method
// Since the container is not hooked into Visual tree yet, we return False from the base class's method.
// We cover this case by keeping track of the generated container in m_tpGeneratedContainerForContentPresenter and overriding this method.
_Check_return_ IFACEMETHODIMP
ComboBox::IsHostForItemContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _Out_ BOOLEAN* pIsHost)
{
    HRESULT hr = S_OK;
    IFCPTR(pIsHost);

    IFC(ComboBoxGenerated::IsHostForItemContainer(pContainer, pIsHost));
    if (!*pIsHost && m_tpGeneratedContainerForContentPresenter)
    {
        *pIsHost = pContainer == m_tpGeneratedContainerForContentPresenter.Get();
    }

Cleanup:
    RRETURN(hr);
}

#pragma endregion ItemsControl Overrides

#pragma region Property Change Handlers

_Check_return_ HRESULT ComboBox::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(ComboBoxGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ComboBox_IsDropDownOpen:
        IFC_RETURN(OnIsDropDownOpenChanged());
        break;

    case KnownPropertyIndex::ComboBox_MaxDropDownHeight:
        IFC_RETURN(OnMaxDropDownHeightChanged());
        break;

    case KnownPropertyIndex::ComboBox_HeaderTemplate:
    case KnownPropertyIndex::ComboBox_Header:
        IFC_RETURN(UpdateHeaderPresenterVisibility());
        break;

    case KnownPropertyIndex::ComboBox_LightDismissOverlayMode:
        {
            if (m_tpPopupPart)
            {
                IFC_RETURN(ReevaluateIsOverlayVisible());
            }
        }
        break;

    case KnownPropertyIndex::UIElement_Visibility:
        IFC_RETURN(OnVisibilityChanged());
        break;

    case KnownPropertyIndex::Selector_IsSelectionActive:
        IFC_RETURN(OnIsSelectionActiveChanged());
        break;

    case KnownPropertyIndex::ItemsControl_DisplayMemberPath:
        m_spPropertyPathListener.Reset();
        break;

    case KnownPropertyIndex::ComboBox_Text:
        if (m_tpEditableTextPart && IsEditable())
        {
            wrl_wrappers::HString text;
            IFC_RETURN(get_Text(text.GetAddressOf()));

            IFC_RETURN(UpdateEditableContentPresenterTextBlock(text));
        }
        break;

    case KnownPropertyIndex::ComboBox_IsEditable:
        if (m_tpEditableTextPart)
        {
            if (args.m_pNewValue->AsBool())
            {
                IFC_RETURN(SetupEditableMode());
                IFC_RETURN(CreateEditableContentPresenterTextBlock());
            }
            else
            {
                IFC_RETURN(DisableEditableMode());
            }
        }
        break;

    case KnownPropertyIndex::Selector_SelectedItem:
        if (IsEditable())
        {
            int selectedIndex = -1;
            IFC_RETURN(get_SelectedIndex(&selectedIndex));
            ctl::ComPtr<IInspectable> spSelectedItem;
            IFC_RETURN(get_SelectedItem(&spSelectedItem));

            SetSearchResultIndex(selectedIndex);

            // If SelectedItem is a custom value (is valid and has index -1) keep a reference to it. We need this to allow
            // reverting to this value in case selection changes.
            if (spSelectedItem && selectedIndex == -1)
            {
                m_customValueRef = spSelectedItem;
            }

            IFC_RETURN(UpdateEditableTextBox(spSelectedItem.Get(), true /*selectText*/, true /*selectAll*/));

            BOOLEAN isDropDownOpen = FALSE;
            IFC_RETURN(get_IsDropDownOpen(&isDropDownOpen));

            // In the case a user has not typed in a custom value, we commit the search and reset the selected index when the drop-down closes,
            // but if it's already closed, that won't happen.  In that case, let's do that now.
            if ((m_searchResultIndex > -1) && !isDropDownOpen)
            {
                IFC_RETURN(CommitRevertEditableSearch(false /* restoreValue */));
            }
        }
        break;

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
    case KnownPropertyIndex::ComboBox_HeaderPlacement:
        IFC_RETURN(UpdateVisualState());
        break;
#endif
    }

    return S_OK;
}

// Handler for the dropdown state changed event
_Check_return_ HRESULT ComboBox::OnIsDropDownOpenChanged()
{
    BOOLEAN isDropDownOpen = FALSE;
    ctl::ComPtr<IUIElement> spElement;

    IFC_RETURN(get_IsDropDownOpen(&isDropDownOpen));

    ASSERT(!(m_isDropDownClosing && !isDropDownOpen), L"The drop down cannot already be closing if IsDropDownOpen was just changed to FALSE.");

    m_skipFocusSuggestion = !isDropDownOpen;

    spElement = m_tpEmptyContent.AsOrNull<IUIElement>();
    if (spElement)
    {
        INT selectedIndex = -1;
        IFC_RETURN(get_SelectedIndex(&selectedIndex));

        // hide default placeholder text is we open dropdown or have anything is selected.
        IFC_RETURN(spElement->put_Opacity(isDropDownOpen || selectedIndex >= 0 ? 0 : 1));
    }

    if (m_isDropDownClosing && isDropDownOpen)
    {
        // We are opening the drop down before it is fully closed. Wrap up on the closing
        // logic before initiating the opening logic.
        IFC_RETURN(FinishClosingDropDown());
    }

    if (IsSmallFormFactor())
    {
        IFC_RETURN(isDropDownOpen? OnOpenSmallFormFactor() : OnCloseSmallFormFactor());
    }
    else
    {
        IFC_RETURN(isDropDownOpen ? OnOpen() : OnClose());
    }

    IFC_RETURN(ElementSoundPlayerService::RequestInteractionSoundForElementStatic(isDropDownOpen ? xaml::ElementSoundKind_Show : xaml::ElementSoundKind_Hide, this));

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnMaxDropDownHeightChanged()
{
    HRESULT hr = S_OK;

    if (!IsSmallFormFactor())
    {
        IFC(ArrangePopup(false));
        IFC(UpdateVisualState());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnSelectionChanged(
    _In_ INT oldSelectedIndex,
    _In_ INT newSelectedIndex,
    _In_ IInspectable* pOldSelectedItem,
    _In_ IInspectable* pNewSelectedItem,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection)
{
    BOOLEAN bIsDropDownOpen = FALSE;

    IFC_RETURN(get_IsDropDownOpen(&bIsDropDownOpen));

    if (bIsDropDownOpen)
    {
        // If is Editable skip focusing the selected item.
        if (!IsEditable())
        {
            IFC_RETURN(ComboBoxGenerated::OnSelectionChanged(oldSelectedIndex, newSelectedIndex, pOldSelectedItem, pNewSelectedItem, animateIfBringIntoView, focusNavigationDirection));
        }
    }
    else
    {
        ctl::ComPtr<IUIElement> spElement;

        if (IsInline())
        {
            // In inline mode, we need to update layout in case the size of the selected item
            // has changed
            IFC_RETURN(UpdateSelectionBoxItemProperties(newSelectedIndex));
            IFC_RETURN(ForceApplyInlineLayoutUpdate());
        }
        else if (m_tpContentPresenterPart)
        {
            // The user is cycling through values in the SelectionBox
            IFC_RETURN(SetContentPresenter(newSelectedIndex));
        }

        spElement = m_tpEmptyContent.AsOrNull<IUIElement>();
        if (spElement)
        {
            INT selectedIndex = -1;
            IFC_RETURN(get_SelectedIndex(&selectedIndex));

            // Hide the default placeholder text if we have any selected item,
            // or show it if we don't.
            IFC_RETURN(spElement->put_Opacity(selectedIndex >= 0 ? 0 : 1));
        }

        // When ComboBox is Editable we need to keep track of the restore index as soon as selection changes, not only
        // on Open as non-editable ComboBox does. This allows us to revert when Selection Trigger is set to Always.
        if (IsEditable())
        {
            if (!m_restoreIndexSet)
            {
                m_restoreIndexSet = true;
                m_indexToRestoreOnCancel = oldSelectedIndex;
            }
        }

        // In Phone 8.1, we relied on visual states to show or hide the default placeholder text.
        // Though that's no longer the case in Threshold, we still call this here for app compat,
        // since existing apps need this call to show or hide the placeholder text.
        IFC_RETURN(UpdateVisualState(FALSE));
    }

    return S_OK;
}


_Check_return_ HRESULT ComboBox::UpdateEditableTextBox(_In_ IInspectable* item, _In_ bool selectText, _In_ bool selectAll)
{
    if (!item)
    {
        return S_OK;
    }

    wrl_wrappers::HString strItem;

    IFC_RETURN(EnsurePropertyPathListener());
    IFC_RETURN(TryGetStringValue(item, m_spPropertyPathListener.Get(), strItem.GetAddressOf()));

    IFC_RETURN(UpdateEditableTextBox(&strItem, selectText, selectAll));

    return S_OK;
}

_Check_return_ HRESULT ComboBox::UpdateEditableTextBox(_In_ wrl_wrappers::HString* str, _In_ bool selectText, _In_ bool selectAll)
{
    if (!str)
    {
        return S_OK;
    }

    if (m_tpEditableTextPart)
    {
        wrl_wrappers::HString textBoxText;
        m_tpEditableTextPart->get_Text(textBoxText.GetAddressOf());

        if (AreStringsEqual(*str, textBoxText))
        {
            return S_OK;
        }

        IFC_RETURN(m_searchString.Duplicate(*str));

        if (selectAll)
        {
            // Selects all the text.
            IFC_RETURN(m_tpEditableTextPart->put_Text(m_searchString));

            if (selectText)
            {
                IFC_RETURN(m_tpEditableTextPart->SelectAll());
            }
        }
        else
        {
            // Selects auto-completed text for quick replacement.
            INT selectionStart = 0;
            m_tpEditableTextPart->get_SelectionStart(&selectionStart);
            IFC_RETURN(m_tpEditableTextPart->put_Text(m_searchString));

            if (selectText)
            {
                IFC_RETURN(m_tpEditableTextPart->Select(selectionStart, str->Length() - selectionStart));
            }
        }
    }

    return S_OK;
}

// Called when the IsSelectionActive property has changed.
_Check_return_ HRESULT ComboBox::OnIsSelectionActiveChanged()
{
    HRESULT hr = S_OK;
    IFC(UpdateVisualState());
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    IFC_RETURN(ComboBoxGenerated::OnIsEnabledChanged(pArgs));
    BOOLEAN bIsEnabled = FALSE;

    IFC_RETURN(get_IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC_RETURN(ClearStateFlags());
    }
    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

// Update the visual states when the Visibility property is changed.
_Check_return_ HRESULT ComboBox::OnVisibilityChanged()
{
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC_RETURN(get_Visibility(&visibility));
    if (xaml::Visibility_Visible != visibility)
    {
        IFC_RETURN(ClearStateFlags());
    }

    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

// Clear flags relating to the visual state.  Called when IsEnabled is set to FALSE
// or when Visibility is set to Hidden or Collapsed.
_Check_return_ HRESULT ComboBox::ClearStateFlags()
{
    HRESULT hr = S_OK;

    IFC(put_IsDropDownOpen(FALSE));
    m_IsPointerOverMain = false;
    m_IsPointerOverPopup = false;
    m_IsPointerOverDropDownOverlay = false;
    m_bIsPressed = false;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::UpdateSelectionBoxHighlighted()
{
    BOOLEAN isDropDownOpen = FALSE;
    IFC_RETURN(get_IsDropDownOpen(&isDropDownOpen));
    BOOLEAN hasFocus = FALSE;
    IFC_RETURN(HasFocus(&hasFocus));
    BOOLEAN value = isDropDownOpen && hasFocus;
    IFC_RETURN(put_IsSelectionBoxHighlighted(value));
    return S_OK;
}

#pragma endregion Property Change Handlers

#pragma region Open/Close Logic

_Check_return_ HRESULT ComboBox::OnOpen()
{
    if (DXamlCore::GetCurrent()->GetHandle()->BackButtonSupported())
    {
        IFC_RETURN(BackButtonIntegration_RegisterListener(this));
    }

    INT selectedItemIndex = -1;
    IFC_RETURN(get_SelectedIndex(&selectedItemIndex));

    // Save off the selected index when opened so that we can
    // restore it when the user cancels using ESC/GamepadB.
    if (!m_restoreIndexSet)
    {
        m_restoreIndexSet = true;
        m_indexToRestoreOnCancel = selectedItemIndex;
    }

    m_isClosingDueToCancel = false;

    if (m_tpPopupPart)
    {
        IFC_RETURN(m_tpPopupPart->put_IsOpen(TRUE));

        bool isDefaultShadowEnabled = true;
        IFC_RETURN(IsDefaultShadowEnabled(this, &isDefaultShadowEnabled));

        // Cast a shadow
        if (isDefaultShadowEnabled)
        {
            IFC_RETURN(ApplyElevationEffect(m_tpElementPopupChild.AsOrNull<IUIElement>().Get()));
        }
        else
        {
            IFC_RETURN(ClearElevationEffect(m_tpElementPopupChild.AsOrNull<IUIElement>().Get()));
        }
    }

    if (m_isOverlayVisible)
    {
        IFC_RETURN(PlayOverlayOpeningAnimation());
    }

    // Before CarouselPanel gets into the measure pass we have to update m_bIsPopupPannable flag
    // and propagate m_bShouldCarousel flag to the CarouselPanel
    // On Edit mode we don't carousel because popup appears aligned to the bottom of the ComboBox
    // instead of centered above the ComboBox, so carouseling doesn't make sense for this design.
    m_bShouldCarousel = m_inputDeviceTypeUsedToOpen == DirectUI::InputDeviceType::Touch && !IsEditable();
    IFC_RETURN(SetIsPopupPannable());
    IFC_RETURN(SetContentPresenter(-1, true /*forceSelectionBoxToNull*/));

    IFC_RETURN(RaiseDropDownOpenChangedEvents(TRUE));

    // At this point, the template settings are holding old values which might
    // not be correct anymore. Thus, if we move to the "Opened" visual state
    // and begin the SplitOpenThemeAnimation right away, the animation parameters
    // might be incorrect. To avoid this, will call UpdateLayout to trigger an
    // arrange pass that will end up calling ComboBox::ArrangePopup. This
    // method will update the template settings appropriately.
    IFC_RETURN(UpdateLayout());
    IFC_RETURN(UpdateVisualState());
    IFC_RETURN(UpdateSelectionBoxHighlighted());

    // Focus is forcibly set to the ComboBox when it is opened.  This is needed to make sure
    // narrator scenarios function as expected.  For example, when an item is selected from
    // an open ComboBox using narrator, the high-light rectangle needs to move back to the
    // ComboBox; this only happens if the ComboBox or one of it's items has focus.  If it
    // doesn't have focus, the high-light rectangle will stay where the item was prior to the
    // popup closing, which is confusing to narrator users.  Another example is when opening
    // the ComboBox, if there is already a selected item, it should get narrator focus as
    // soon as it opens.
    // This does not change the behavior for keyboard/touch/mouse users since interacting
    // with the control using any of those input methods would have set focus to it anyway.
    // This only affects opening the ComboBox programmatically, which is what narrator does.
    if (IsEditable())
    {
        BOOLEAN isSuccessful = FALSE;

        // Ensure focus is in TextBox when popup opens.
        if (!EditableTextHasFocus() && m_tpEditableTextPart)
        {
            IFC_RETURN(m_tpEditableTextPart.Cast<TextBox>()->Focus(xaml::FocusState_Programmatic, &isSuccessful));
        }
    }
    else
    {

        // If no item is selected when we open the combo box and there's at least one item,
        // give focus to or select the first item to ensure that keyboarding can function normally.
        if (selectedItemIndex >= 0)
        {
            IFC_RETURN(SetFocusedItem(selectedItemIndex, m_bShouldCenterSelectedItem /*shouldScrollIntoView*/, TRUE /*forceFocus*/, xaml::FocusState_Programmatic));
        }
        else
        {
            uint32_t itemCount = 0;
            IFC_RETURN(GetItemCount(itemCount));

            if (itemCount > 0)
            {
                xaml_controls::ComboBoxSelectionChangedTrigger selectionChangedTrigger;
                IFC_RETURN(get_SelectionChangedTrigger(&selectionChangedTrigger));

                if (selectionChangedTrigger == xaml_controls::ComboBoxSelectionChangedTrigger_Always)
                {
                    IFC_RETURN(put_SelectedIndex(0));
                }
                else
                {
                    IFC_RETURN(OverrideSelectedIndexForVisualStates(0));
                }

                IFC_RETURN(SetFocusedItem(0, FALSE /*shouldScrollIntoView*/, TRUE /*forceFocus*/, xaml::FocusState_Programmatic));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnClose()
{
    IFC_RETURN(BackButtonIntegration_UnregisterListener(this));

    m_isDropDownClosing = true;

    if (IsEditable())
    {
        IFC_RETURN(CommitRevertEditableSearch(m_isClosingDueToCancel /*restoreValue*/));
    }
    else if (m_isClosingDueToCancel && m_indexToRestoreOnCancel != -1)
    {
        IFC_RETURN(put_SelectedIndex(m_indexToRestoreOnCancel));
    }

    m_indexToRestoreOnCancel = -1;
    m_restoreIndexSet = false;
    m_isClosingDueToCancel = false;

    IFC_RETURN(ClearSelectedIndexOverrideForVisualStates());
    IFC_RETURN(SetClosingAnimationDirection());

    if (m_isOverlayVisible)
    {
        PlayOverlayClosingAnimation();
    }

    IFC_RETURN(UpdateVisualState(TRUE));
    if (!m_tpClosedStoryboard)
    {
        // If we do not have a storyboard for closed state, the completed handler for the
        // animation will never be called, so we need to complete closing the drop down
        // now.
        IFC_RETURN(FinishClosingDropDown());
    }
    else
    {
        // Editable ComboBox handles ContentPresenter changes in CommitRevertEditableSearch.
        if (!IsEditable())
        {
            INT selectedIndex = -1;

            IFC_RETURN(get_SelectedIndex(&selectedIndex));
            IFC_RETURN(SetContentPresenter(selectedIndex));
        }
    }

    // Clear pointer over status while the dropdown is closing.
    // Sometimes when the dropdown animates out from under the pointer we don't get a
    // PointerExited so the ComboBox stays in PointerOver state indefinitely.
    m_IsPointerOverPopup = false;
    m_IsPointerOverMain = false;
    m_IsPointerOverDropDownOverlay = false;
    ChangeVisualState(FALSE);

    return S_OK;
}

// Processing to complete the process of closing the drop-down is either
// - triggered when the closing storyboard finishes. See OnClosedStateStoryboardCompleted OR
// - Manually invoked because there is no storyboard for the closing state OR
// - Manually invoked because the drop down state was changed by an external caller while
//   the animation was ongoing.
_Check_return_ HRESULT
ComboBox::FinishClosingDropDown()
{
    INT selectedIndex = -1;

    // Clean up any existing operation. Important to clear this before firing the
    // DropDownClosed event because the event handler may set IsDropDownOpen back to true,
    // which will begin a new async operation. Dropping this ref will destroy the
    // operation which will prevent the completion event from firing, ensuring that
    // our completion callback doesn't erroneously call FinishClosingDropDown again.
    m_tpAsyncSelectionInfo.Clear();

    IFC_RETURN(get_SelectedIndex(&selectedIndex));

    // This returns focus to ComboBox after clicking on a ComboBoxItem.
    IFC_RETURN(SetFocusedItem(-1, FALSE /*shouldScrollIntoView*/));

    // Ensure Focus moves over to Textbox next time ComboBox is focused.
    m_shouldMoveFocusToTextBox = true;

    if (IsInline())
    {
        IFC_RETURN(EnsurePresenterReadyForInlineMode());
        IFC_RETURN(UpdateSelectionBoxItemProperties(selectedIndex));
        IFC_RETURN(ForceApplyInlineLayoutUpdate());
    }
    else
    {
        // Editable ComboBox handles ContentPresenter changes in CommitRevertEditableSearch.
        if (!IsEditable())
        {
            IFC_RETURN(SetContentPresenter(selectedIndex));
        }

        if (m_tpPopupPart)
        {
            IFC_RETURN(m_tpPopupPart.Get()->put_IsOpen(FALSE));
            m_IsPointerOverPopup = false; // closing the popup will not fire a PointerExited
            IFC_RETURN(ResetCarouselPanelState());
            IFC_RETURN(ClearStateFlagsOnItems());
        }
    }

    m_isExpanded = false;
    m_isDropDownClosing = false;
    m_previousInputDeviceTypeUsedToOpen = m_inputDeviceTypeUsedToOpen;
    m_inputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::None;

    IFC_RETURN(RaiseDropDownOpenChangedEvents(FALSE));
    IFC_RETURN(UpdateVisualState());
    IFC_RETURN(UpdateSelectionBoxHighlighted());

    return S_OK;
}

_Check_return_ HRESULT
ComboBox::RaiseDropDownOpenChangedEvents(
    _In_ BOOLEAN isDropDownOpen)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<RoutedEventArgs> spRoutedEventArgs;
    ctl::ComPtr<IInspectable> spArgsAsI;

    // Create the args
    IFC(ctl::make(&spRoutedEventArgs));
    IFC(spRoutedEventArgs->put_OriginalSource(ctl::as_iinspectable(this)));
    IFC(spRoutedEventArgs.As(&spArgsAsI));

    if (isDropDownOpen)
    {
        IFC(OnDropDownOpenedProtected(spArgsAsI.Get()));
    }
    else
    {
        IFC(OnDropDownClosedProtected(spArgsAsI.Get()));
    }

    IFC(RaiseAutomationPeerExpandCollapse(isDropDownOpen));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnDropDownOpenedImpl(
    _In_ IInspectable* pArgs)
{
    HRESULT hr = S_OK;

    DropDownOpenedEventSourceType* pEventSource = nullptr;

    IFC(ComboBoxGenerated::GetDropDownOpenedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), pArgs));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnDropDownClosedImpl(
    _In_ IInspectable* pArgs)
{
    HRESULT hr = S_OK;

    DropDownClosedEventSourceType* pEventSource = nullptr;

    IFC(ComboBoxGenerated::GetDropDownClosedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), pArgs));

Cleanup:
    RRETURN(hr);
}

#pragma endregion Open/Close Logic

#pragma region Small Form Factor Open/Close Logic

_Check_return_ HRESULT ComboBox::OnOpenSmallFormFactor()
{
    ASSERT(IsSmallFormFactor(), L"Expected to be in small form factor mode.");

    if (IsInline())
    {
        BOOLEAN isSuccessful = FALSE;
        BOOLEAN hasFocus = FALSE;

        if (DXamlCore::GetCurrent()->GetHandle()->BackButtonSupported())
        {
            IFC_RETURN(BackButtonIntegration_RegisterListener(this));
        }

        IFC_RETURN(HasFocus(&hasFocus));
        if (!hasFocus)
        {
            IFC_RETURN(Focus(xaml::FocusState_Programmatic, &isSuccessful));
            ASSERT(isSuccessful);
        }

        m_doKeepInView = true;
        IFC_RETURN(PrepareLayoutForInlineDropDownOpened());
        IFC_RETURN(UpdateSelectedItemVisualState());
        IFC_RETURN(UpdateVisualState(TRUE));

        m_isExpanded = true;
        IFC_RETURN(RaiseDropDownOpenChangedEvents(TRUE));
    }
    else
    {
        IFC_RETURN(SetContentPresenter(-1));

        ctl::ComPtr<IInspectable> spAsyncOperationAsI;
        ctl::ComPtr<wf::IAsyncOperation<INT32>> spAsyncOperation;
        ctl::ComPtr<wf::IAsyncOperationCompletedHandler<INT32>> spCompletedHandler;

        IFC_RETURN(EnsurePresenterReadyForFullMode());
        auto xamlControlsGetListPickerSelectionPtr = reinterpret_cast<decltype(&XamlControlsGetListPickerSelection)>(::GetProcAddress(GetPhoneModule(), "XamlControlsGetListPickerSelection"));
        IFC_RETURN(xamlControlsGetListPickerSelectionPtr(ctl::as_iinspectable(this), &spAsyncOperationAsI));
        IFC_RETURN(spAsyncOperationAsI.As(&spAsyncOperation));
        IFC_RETURN(SetPtrValueWithQI(m_tpAsyncSelectionInfo, spAsyncOperation.Get()));

        spCompletedHandler.Attach(new ClassMemberCallback2<
            ComboBox,
            IComboBox,
            wf::IAsyncOperationCompletedHandler<INT32>,
            wf::IAsyncOperation<INT32>,
            wf::AsyncStatus>(this, &ComboBox::OnGetListPickerSelectionAsyncCompleted));

        IFC_RETURN(spAsyncOperation->put_Completed(spCompletedHandler.Get()));

        IFC_RETURN(RaiseDropDownOpenChangedEvents(TRUE));
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnCloseSmallFormFactor()
{
    ASSERT(IsSmallFormFactor(), L"Expected to be in small form factor mode.");

    // We check m_isExpanded instead of IsInline() because the combo box may have been
    // closed due to an item being added/removed, putting us above/below the inline mode threshold.
    if (m_isExpanded)
    {
        IFC_RETURN(BackButtonIntegration_UnregisterListener(this));

        m_isDropDownClosing = true;
        IFC_RETURN(UpdateSelectedItemVisualState());
        m_doKeepInView = false; // We don't want to change the scroll view while closing.
        IFC_RETURN(PrepareLayoutForInlineDropDownClosed());
        IFC_RETURN(UpdateVisualState(TRUE));
        if (!m_tpClosedStoryboard)
        {
            // If we do not have a storyboard for closed state, the completed handler for the
            // animation will never be called, so we need to complete closing the drop down
            // now.
            IFC_RETURN(FinishClosingDropDown());
        }
    }
    else // We were in full mode
    {
        wf::AsyncStatus status = wf::AsyncStatus::Started;

        ASSERT(m_tpAsyncSelectionInfo, "Expected AsyncInfo to be stored if we're closing a full-mode ComboBox.");
        IFC_RETURN(m_tpAsyncSelectionInfo->get_Status(&status));
        ASSERT(status == wf::AsyncStatus::Completed || status == wf::AsyncStatus::Started);
        if (status == wf::AsyncStatus::Started)
        {
            // if this operation has not completed yet, then the ComboBox dropdown must
            // have been closed programatically before a selection was made. In this case
            // we need to cancel the operation to close the flyout.
            m_isDropDownClosing = true;
            IFC_RETURN(m_tpAsyncSelectionInfo->Cancel());
        }
        else
        {
            // We're done with the async operation.
            IFC_RETURN(FinishClosingDropDown());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::UpdateTemplateSettings()
{
    ctl::ComPtr<IComboBoxTemplateSettings> templateSettings;
    IFC_RETURN(get_TemplateSettings(&templateSettings));
    ComboBoxTemplateSettings* templateSettingsConcrete = templateSettings.Cast<ComboBoxTemplateSettings>();
    if (templateSettingsConcrete)
    {
        // Query ComboBox Popup MinWidth, given the input mode, from resource dictionary.
        ctl::ComPtr<xaml::IResourceDictionary> resources;
        IFC_RETURN(get_Resources(&resources));

        double comboBoxPopupMinWidth = 0.0;
        resources.Cast<ResourceDictionary>()->TryLookupBoxedValue(wrl_wrappers::HStringReference(
            (m_inputDeviceTypeUsedToOpen == DirectUI::InputDeviceType::Touch || m_inputDeviceTypeUsedToOpen  == DirectUI::InputDeviceType::GamepadOrRemote)
                ? L"ComboBoxPopupThemeTouchMinWidth" : L"ComboBoxPopupThemeMinWidth").Get(), &comboBoxPopupMinWidth);

        wf::Rect visibleBounds = {};
        IFC_RETURN(DXamlCore::GetCurrent()->GetVisibleContentBoundsForElement(GetHandle(), &visibleBounds));

        IFC_RETURN(templateSettingsConcrete->put_DropDownContentMinWidth(DoubleUtil::Min(visibleBounds.Width, comboBoxPopupMinWidth)));
    }

    return S_OK;
}

// ComboBoxItem knows that if it's host ComboBox is in inline mode and the drop down is
// closed, it should display in the Unselected visual state even if it is selected.
// This method is used to trigger a state change on the selected items so that logic can
// be applied.
_Check_return_ HRESULT ComboBox::UpdateSelectedItemVisualState()
{
    HRESULT hr = S_OK;
    INT selectedItemIndex = -1;

    ASSERT(IsInline(), L"UpdateSelectedItemVisualState is only used in inline mode.");

    IFC(get_SelectedIndex(&selectedItemIndex));

    if (selectedItemIndex != -1)
    {
        ctl::ComPtr<ComboBoxItem> spSelectedComboBoxItem;
        ctl::ComPtr<IDependencyObject> spContainerAsDO;

        IFC(ContainerFromIndex(selectedItemIndex, &spContainerAsDO));
        spSelectedComboBoxItem = spContainerAsDO.Cast<ComboBoxItem>();

        ASSERT(spSelectedComboBoxItem, L"Container of selected item of ComboBox is expected to be of type ComboBoxItem.");
        if (spSelectedComboBoxItem)
        {
            IFC(spSelectedComboBoxItem->ChangeVisualState(TRUE));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::EnsurePresenterReadyForFullMode()
{
    HRESULT hr = S_OK;

    if (m_itemsPresenterIndex == -1)
    {
        // Ensure that the items are removed from visual tree so that they can be safely
        // displayed when the ComboBox is reopened in full mode.
        ctl::ComPtr<IPanel> spItemsPresenterHostAsPanel;
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spHostChildren;
        UINT32 index = 0;
        BOOLEAN isFound = FALSE;
        IFC(m_tpItemsPresenterHostPart.As(&spItemsPresenterHostAsPanel));
        IFC(spItemsPresenterHostAsPanel->get_Children(&spHostChildren));
        IFC(spHostChildren->IndexOf(m_tpItemsPresenterPart.AsOrNull<IUIElement>().Get(), &index, &isFound));
        ASSERT(isFound, L"Expected items presenter to be a child of itemspresenterhost.");
        IFC(spHostChildren->RemoveAt(index));
        m_itemsPresenterIndex = index;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::EnsurePresenterReadyForInlineMode()
{
    HRESULT hr = S_OK;

    if (m_itemsPresenterIndex != -1)
    {
        // We're coming out of full mode. Restore the items presenter.

        ctl::ComPtr<IPanel> spItemsPresenterHostAsPanel;
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spHostChildren;
        UINT32 size = 0;
        IFC(m_tpItemsPresenterHostPart.As(&spItemsPresenterHostAsPanel));
        IFC(spItemsPresenterHostAsPanel->get_Children(&spHostChildren));
        IFC(spHostChildren->get_Size(&size));
        if (size < static_cast<UINT32>(m_itemsPresenterIndex))
        {
            m_itemsPresenterIndex = size;
        }

        IFC(spHostChildren->InsertAt(m_itemsPresenterIndex, m_tpItemsPresenterPart.AsOrNull<IUIElement>().Get()));
        m_itemsPresenterIndex = -1;
    }

Cleanup:
    RRETURN(hr);
}

#pragma endregion Small Form Factor Open/Close Logic

#pragma region Event Handlers

_Check_return_ HRESULT ComboBox::OnUnloaded(_In_ IInspectable* /*pSender*/, _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    return put_IsDropDownOpen(FALSE);
}

_Check_return_ HRESULT ComboBox::OnSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    if (!IsSmallFormFactor())
    {
        IFC(ArrangePopup(false));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ComboBox::OnClosedStateStoryboardCompleted(
    _In_ IInspectable* /*pSender*/,
    _In_ IInspectable* /*pArgs*/)
{
    HRESULT hr = S_OK;

    if (m_isDropDownClosing)
    {
        // Only finish closing the drop down if it hasn't already been done
        // before the animation completed.
        IFC(FinishClosingDropDown());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::FocusChanged(
    _In_ BOOLEAN hasFocus)
{
    // The OnGotFocus & OnLostFocus are asynchronous and cannot reliably tell you that have the focus.  All they do is
    // let you know that the focus changed sometime in the past.  To determine if you currently have the focus you need
    // to do consult the FocusManager (see HasFocus()).
    IFC_RETURN(UpdateSelectionBoxHighlighted());
    IFC_RETURN(put_IsSelectionActive(hasFocus));

    if (!hasFocus && !IsFullMode())
    {
        m_isClosingDueToCancel = true;
        IFC_RETURN(put_IsDropDownOpen(FALSE));
    }

    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

_Check_return_ HRESULT ComboBox::HasFocus(
    _Out_ BOOLEAN *pbHasFocus)
{
    IFCPTR_RETURN(pbHasFocus);
    *pbHasFocus = false;

    if(const auto focusManager = VisualTree::GetFocusManagerForElement(GetHandle()))
    {
        if (auto spFocused = focusManager->GetFocusedElementNoRef())
        {
            ctl::ComPtr<DependencyObject> spFocusedAsDO;
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(spFocused, &spFocusedAsDO));
            ctl::ComPtr<IDependencyObject> spFocusedAsI;
            spFocusedAsDO.As<IDependencyObject>(&spFocusedAsI);
            IFC_RETURN(IsChildOfTarget(spFocusedAsI.Get(), true, false, pbHasFocus));
        }
    }

    return S_OK;
}

bool ComboBox::EditableTextHasFocus()
{
    if (m_tpEditableTextPart)
    {
        const auto focusManager = VisualTree::GetFocusManagerForElement(GetHandle());
        return (m_tpEditableTextPart.Cast<TextBox>()->GetHandle() == focusManager->GetFocusedElementNoRef());
    }
    else
    {
        return false;
    }
}

_Check_return_ HRESULT ComboBox::EnsureTextBoxIsEnabled(bool moveFocusToTextBox)
{
    if (m_tpEditableTextPart)
    {
        IFC_RETURN(m_tpEditableTextPart.Cast<TextBox>()->put_Width(DoubleUtil::NaN));
        IFC_RETURN(m_tpEditableTextPart.Cast<TextBox>()->put_Height(DoubleUtil::NaN));
        IFC_RETURN(m_tpContentPresenterPart.Cast<ContentPresenter>()->put_Visibility(xaml::Visibility_Collapsed));

        if (moveFocusToTextBox)
        {
            BOOLEAN isSuccessful = false;
            IFC_RETURN(m_tpEditableTextPart.Cast<TextBox>()->Focus(xaml::FocusState_Programmatic, &isSuccessful));

            m_shouldMoveFocusToTextBox = false;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::ProcessTabStopOverride(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_opt_ DependencyObject* pCandidateTabStopElement,
    const bool isBackward,
    const bool didCycleFocusAtRootVisualScope,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pIsTabStopOverridden)
{
    // An editable ComboBox has special tab behavior. We want to be able to Tab a single time
    // directly into the TextBox and to Shift + Tab a single time to move focus outside to a
    // different Control, if applicable. In this sense, it is similar to AutoSuggestBox, where the
    // TextBox inside is a tab stop but the AutoSuggestBox itself is not (see IsTabStop is set to
    // false in the AutoSuggestBox's Style). However, contrary to AutoSuggestBox, an editable
    // ComboBox must actually appear as two separate tab stops when using a Gamepad. This is to
    // support behavior such as pressing B while the TextBox is focused, which should move focus
    // back to the ComboBox itself (which would not be possible if the ComboBox had IsTabStop set
    // to false). Given this, we will manipulate the tab stops to force backward navigation to skip
    // the ComboBox.
    if (IsEditable() && m_tpEditableTextPart && isBackward)
    {
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(GetHandle());
        auto lastInputDeviceType = contentRoot->GetInputManager().GetLastInputDeviceType();
        xref_ptr<CDependencyObject> newTabStop;

        if (lastInputDeviceType == DirectUI::InputDeviceType::Keyboard)
        {
            newTabStop = contentRoot->GetFocusManagerNoRef()->GetPreviousTabStop(GetHandle());

            // If we found a candidate, then query its corresponding peer.
            if (newTabStop)
            {
                ctl::ComPtr<DependencyObject> spNewTabStop;
                IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(newTabStop, &spNewTabStop));
                IFC_RETURN(spNewTabStop.MoveTo(ppNewTabStop));
                *pIsTabStopOverridden = TRUE;
            }
        }
    }

    return S_OK;
}

IFACEMETHODIMP ComboBox::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    BOOLEAN hasFocus = FALSE;
    IFC_RETURN(ComboBoxGenerated::OnGotFocus(pArgs));
    IFC_RETURN(HasFocus(&hasFocus));
    IFC_RETURN(FocusChanged(hasFocus));

    if (IsEditable() && m_tpEditableTextPart)
    {
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(GetHandle());
        auto lastPointerType = contentRoot->GetInputManager().GetLastInputDeviceType();

        // If EditableText is not focused, make the control visible, focus and return. Next time we receive OnGotFocus we will setup the control.
        if (EditableTextHasFocus())
        {
            m_shouldMoveFocusToTextBox = false;
            IFC_RETURN(EnsureTextBoxIsEnabled(false /* moveFocusToTextBox */));
        }
        else
        {
            // For gamepad TextBox should only be visible when popup is open.
            if (lastPointerType == DirectUI::InputDeviceType::GamepadOrRemote)
            {
                return S_OK;
            }

            DOUBLE editableTextPartWidth;
            DOUBLE editableTextPartHeight;
            IFC_RETURN(m_tpEditableTextPart.Cast<TextBox>()->get_Width(&editableTextPartWidth));
            IFC_RETURN(m_tpEditableTextPart.Cast<TextBox>()->get_Height(&editableTextPartHeight));

            if ((editableTextPartWidth == 0 && editableTextPartHeight == 0)|| ShouldMoveFocusToTextBox())
            {
                IFC_RETURN(EnsureTextBoxIsEnabled(true /* moveFocusToTextBox */));
            }

            return S_OK;
        }

        if (lastPointerType == DirectUI::InputDeviceType::Touch)
        {
            wrl_wrappers::HString str;
            IFC_RETURN(m_tpEditableTextPart->get_Text(str.GetAddressOf()));
            IFC_RETURN(m_tpEditableTextPart->put_SelectionStart(str.Length()));

            // According to the interaction model design, after the control is focused we need to select all the text next time it is touched.
            m_selectAllOnTouch = true;
        }
        else
        {
            IFC_RETURN(m_tpEditableTextPart->SelectAll());

            // ProcessSearch when TextBox gains focus, this ensures TextBox.Text matches an item or a custom value index even if Text was modified when control wasn't focused.
            IFC_RETURN(ProcessSearch(L' '));
        }

        BOOLEAN popupIsOpen = TRUE;

        if (m_tpPopupPart)
        {
            IFC_RETURN(m_tpPopupPart->get_IsOpen(&popupIsOpen));
        }

        // On Touch open DropDown when getting focus.
        if (m_openPopupOnTouch && !popupIsOpen && lastPointerType == DirectUI::InputDeviceType::Touch)
        {
            m_inputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::Touch;
            IFC_RETURN(put_IsDropDownOpen(TRUE));
        }

        m_openPopupOnTouch = false;
    }

    return S_OK;
}

IFACEMETHODIMP ComboBox::OnLostFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    BOOLEAN hasFocus = FALSE;
    IFC_RETURN(ComboBoxGenerated::OnLostFocus(pArgs));
    IFC_RETURN(HasFocus(&hasFocus));
    IFC_RETURN(FocusChanged(hasFocus));
    m_selectAllOnTouch = false;

    if (IsEditable())
    {
        if (!hasFocus)
        {
            // Commit the selected value.
            IFC_RETURN(CommitRevertEditableSearch(false /*restoreValue*/));

            if (m_tpEditableTextPart)
            {
                IFC_RETURN(m_tpEditableTextPart.Cast<TextBox>()->put_Width(0.0f));
                IFC_RETURN(m_tpEditableTextPart.Cast<TextBox>()->put_Height(0.0f));
                IFC_RETURN(m_tpContentPresenterPart.Cast<ContentPresenter>()->put_Visibility(xaml::Visibility_Visible));
            }

            // When ComboBox loses focus, ensure to move the focus over to the TextBox next time ComboBox is focused.
            m_shouldMoveFocusToTextBox = true;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::UpdateEditableContentPresenterTextBlock(_In_ IInspectable* item)
{
    if (!item)
    {
        return S_OK;
    }

    wrl_wrappers::HString itemString;

    IFC_RETURN(EnsurePropertyPathListener());
    IFC_RETURN(TryGetStringValue(item, m_spPropertyPathListener.Get(), itemString.GetAddressOf()));
    IFC_RETURN(UpdateEditableContentPresenterTextBlock(itemString));

    return S_OK;
}

_Check_return_ HRESULT ComboBox::UpdateEditableContentPresenterTextBlock(const wrl_wrappers::HString& text)
{
    if (!m_tpContentPresenterPart || !m_tpEditableContentPresenterTextBlock || !text)
    {
        return S_OK;
    }

    // Reset ContentPresenter in case it is storing a swapped ComboBoxItem.
    IFC_RETURN(SetContentPresenter(-1));

    IFC_RETURN(m_tpEditableContentPresenterTextBlock->put_Text(text.Get()));
    IFC_RETURN(m_tpContentPresenterPart->put_Content(m_tpEditableContentPresenterTextBlock.Get()));

    IFC_RETURN(InvokeValidationCommand(this, text.Get()));
    return S_OK;
}

_Check_return_ HRESULT ComboBox::CommitRevertEditableSearch(_In_ bool restoreValue)
{
    if (!m_tpEditableTextPart)
    {
        return S_OK;
    }

    if (restoreValue)
    {
        if (m_restoreIndexSet)
        {
            if (m_indexToRestoreOnCancel != -1)
            {
                IFC_RETURN(put_SelectedIndex(m_indexToRestoreOnCancel));
            }
            else
            {
                // If m_indexToRestoreOnCancel is -1 this means we need to either restore to a custom value or to -1
                // check if we are holding a reference to a custom value.
                if (m_customValueRef)
                {
                    IFC_RETURN(put_SelectedItem(m_customValueRef.Get()));
                }
                else
                {
                    IFC_RETURN(put_SelectedIndex(-1));
                }
            }
        }
    }
    else
    {
        // We set SearchResultIndex when typing, arrowing up/down, or if a selection change happened. In those cases we already processed the search.
        // If SearchResultIndex is not set we need to ensure the current TextBox.Text matches the correct Index in case ComboBox.Text or TextBox.Text were
        // changed programatically.
        if (!IsSearchResultIndexSet())
        {
            IFC_RETURN(ProcessSearch(L' '));
        }

        // If search has a match within the Data Source select the item.
        if (m_searchResultIndex > -1)
        {
            IFC_RETURN(put_SelectedIndex(m_searchResultIndex));
            m_customValueRef.Reset();
        }
        // If searched value is not in the Data Source it means we are trying to commit a value outside the Data Source. Raise a CommitRequest with the new value.
        else
        {
            wrl_wrappers::HString searchString;
            IFC_RETURN(m_tpEditableTextPart->get_Text(searchString.GetAddressOf()));

            // Ensure searchString is not empty or contains only spaces.
            if (IsSearchStringValid(searchString))
            {
                bool sendEvent = true;

                if (m_customValueRef)
                {
                    wrl_wrappers::HString storedString;
                    IValueBoxer::UnboxValue(m_customValueRef.Get(), storedString.GetAddressOf());

                    // Prevent sending the event if the custom value is the same.
                    sendEvent = !AreStringsEqual(storedString, searchString);
                }

                if (sendEvent)
                {
                    ctl::ComPtr<IInspectable> spInspectable;
                    IFC_RETURN(PropertyValue::CreateFromString(searchString, &spInspectable));

                    BOOLEAN isHandled;
                    IFC_RETURN(RaiseTextSubmittedEvent(searchString, &isHandled));

                    // If event was not handled we assume we want to keep the current value as active.
                    if (!isHandled)
                    {
                        int foundIndex = -1;
                        IFC_RETURN(SearchItemSourceIndex(L' ', false /*startSearchFromCurrentIndex*/, true /*searchExactMatch*/, foundIndex));

                        if (foundIndex != -1)
                        {
                            m_customValueRef.Reset();
                            HRESULT hr = put_SelectedIndex(foundIndex);

                            // After the TextSubmittedEvent we try to match the current Custom Value with a value in our ItemSource in case the value was
                            // inserted during the event. In order to select this item, it needs to exist in our ItemContainer. This will not be the case when
                            // ItemSource is a List and not an ObservableCollection. Using ObservableCollection is required for this to work as our ItemContainerGenerator
                            // relies on INotifyPropertyChanged to update values when the ItemSource has changed.
                            // We improve the error message here to match what managed code returns when trying to set the SelectedIndex under the same conditions.
                            if (hr == E_INVALIDARG)
                            {
                                IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_DEPENDENCYOBJECTCOLLECTION_OUTOFRANGE));
                            }
                            else
                            {
                                IFC_RETURN(hr);
                            }
                        }
                        else
                        {
                            IFC_RETURN(put_SelectedItem(spInspectable.Get()));
                        }
                    }
                }
                else
                {
                    INT selectedIndex = -1;
                    IFC_RETURN(get_SelectedIndex(&selectedIndex));

                    // Ensure SelectedIndex is -1, this means the Custom Value is still active.
                    if (selectedIndex != -1)
                    {
                        ctl::ComPtr<IInspectable> spInspectable;
                        IFC_RETURN(PropertyValue::CreateFromString(searchString, &spInspectable));

                        IFC_RETURN(put_SelectedItem(spInspectable.Get()));
                    }
                }
            }
        }
    }

    ctl::ComPtr<IInspectable> spSelectedItem;
    IFC_RETURN(get_SelectedItem(&spSelectedItem));

    // Update ContentPresenter.
    if (spSelectedItem)
    {
        INT selectedIndex = -1;
        IFC_RETURN(get_SelectedIndex(&selectedIndex));

        // If SelectedItem exists but SelectedIndex is -1 it means we are using a custom value, clear the ContentPresenter in this case.
        if (selectedIndex > -1)
        {
            IFC_RETURN(SetContentPresenter(selectedIndex));
        }
        else
        {
            IFC_RETURN(UpdateEditableContentPresenterTextBlock(spSelectedItem.Get()));
            IFC_RETURN(put_SelectionBoxItem(spSelectedItem.Get()));
            IFC_RETURN(put_SelectionBoxItemTemplate(NULL));
        }

        IFC_RETURN(UpdateEditableTextBox(spSelectedItem.Get(), true /*selectText*/, true /*selectAll*/));
    }
    else
    {
        IFC_RETURN(m_tpEditableTextPart->put_Text(nullptr));
        IFC_RETURN(m_tpEditableTextPart->SelectAll());
        IFC_RETURN(SetContentPresenter(-1));
    }

    ResetSearch();
    m_restoreIndexSet = false;
    m_indexToRestoreOnCancel = -1;

    return S_OK;
}

void ComboBox::ResetSearch()
{
    m_searchResultIndexSet = false;
    m_searchResultIndex = -1;
}

void ComboBox::SetSearchResultIndex(_In_ int index)
{
    m_searchResultIndexSet = true;
    m_searchResultIndex = index;
}

// Responds to the KeyDown event.
IFACEMETHODIMP ComboBox::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    return OnKeyDownPrivate(NULL, pArgs);
}

// Responds to the KeyUp event.
IFACEMETHODIMP ComboBox::OnKeyUp(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    return OnKeyUpPrivate(NULL, pArgs);
}

_Check_return_ HRESULT ComboBox::OnKeyDownPrivate(
    _In_opt_ IInspectable* pSender,
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    IFC_RETURN(ComboBoxGenerated::OnKeyDown(pArgs));
    IFCPTR_RETURN(pArgs);

    auto originalKey = wsy::VirtualKey_None;
    auto key = wsy::VirtualKey_None;
    IFC_RETURN(pArgs->get_Key(&key));
    IFC_RETURN(pArgs->get_OriginalKey(&originalKey));

    //Key maps to both gamepad B and Escape key
    if (m_ignoreCancelKeyDowns && key == wsy::VirtualKey_Escape)
    {
        IFC_RETURN(pArgs->put_Handled(TRUE));
        return S_OK;
    }

    BOOLEAN eventHandled = FALSE;
    IFC_RETURN(pArgs->get_Handled(&eventHandled));

    if (eventHandled)
    {
        return S_OK;
    }

    BOOLEAN isEnabled = FALSE;
    IFC_RETURN(get_IsEnabled(&isEnabled));

    if (!isEnabled)
    {
        return S_OK;
    }

    BOOLEAN bIsDropDownOpen = FALSE;
    IFC_RETURN(get_IsDropDownOpen(&bIsDropDownOpen));
    if (bIsDropDownOpen)
    {
        IFC_RETURN(PopupKeyDown(pArgs));
    }
    else
    {
        IFC_RETURN(MainKeyDown(pArgs));
    }

    IFC_RETURN(pArgs->get_Handled(&eventHandled));
    m_handledGamepadOrRemoteKeyDown = eventHandled && XboxUtility::IsGamepadNavigationInput(originalKey);

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnTextBoxPreviewKeyDown(
    _In_opt_ IInspectable* pSender,
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    wsy::VirtualKey keyObject = wsy::VirtualKey_None;

    IFC_RETURN(pArgs->get_Key(&keyObject));

    if (keyObject == wsy::VirtualKey::VirtualKey_Up || keyObject == wsy::VirtualKey::VirtualKey_Down)
    {
        IFC_RETURN(OnKeyDownPrivate(pSender, pArgs));
        pArgs->put_Handled(true);
    }

    return S_OK;
}

// Called whenever the we receive a key-up message.
_Check_return_ HRESULT ComboBox::OnKeyUpPrivate(
    _In_opt_ IInspectable* pSender,
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    IFC_RETURN(ComboBoxGenerated::OnKeyUp(pArgs));
    IFCPTR_RETURN(pArgs);

    auto originalKey = wsy::VirtualKey_None;
    auto key = wsy::VirtualKey_None;
    IFC_RETURN(pArgs->get_Key(&key));
    IFC_RETURN(pArgs->get_OriginalKey(&originalKey));

    //Key maps to both gamepad B and Escape key
    if (m_ignoreCancelKeyDowns && key == wsy::VirtualKey_Escape)
    {
        m_ignoreCancelKeyDowns = false;
        //We know some escape key was pressed, so make sure to mark it as handled.
        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    BOOLEAN eventHandled = FALSE;
    IFC_RETURN(pArgs->get_Handled(&eventHandled));

    if (eventHandled)
    {
        return S_OK;
    }

    BOOLEAN isEnabled = FALSE;
    IFC_RETURN(get_IsEnabled(&isEnabled));

    if (!isEnabled)
    {
        return S_OK;
    }

    // Ideally, we want the ComboBox to execute its functional behavior in response to KeyUp,
    // not KeyDown.  Our not doing so causes problems - for example, if a page listens to the
    // B button to know when to navigate back, it will do so in response to a B button press
    // that was intended just to close the ComboBox drop-down, since the ComboBox does not
    // handle the KeyUp event, so the Page ends up receiving it. However, for the purposes of TH2,
    // we'll scope things to simply unblock this specific Xbox scenario by merely marking KeyUp
    // as handled when KeyDown was handled, to prevent it from bubbling up to a page or
    // something else that might want to respond to it.  Task #4373221 has been filed to track
    // the more general work item to bring controls such as ComboBox in line with the
    // design philosophy that controls should execute functional behavior in response to KeyUp,
    // rather than KeyDown.
    if (m_handledGamepadOrRemoteKeyDown && XboxUtility::IsGamepadNavigationInput(originalKey))
    {
        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    m_handledGamepadOrRemoteKeyDown = false;

    return S_OK;
}

_Check_return_ HRESULT ComboBox::PopupKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    BOOLEAN handled = FALSE;
    wsy::VirtualKeyModifiers nModifierKeys;
    ctl::ComPtr<DependencyObject> spFocused;
    ctl::ComPtr<ComboBoxItem> spComboBoxItem;
    INT newFocusedIndex = -1;
    auto key = wsy::VirtualKey_None;
    xaml::FocusState focusState = xaml::FocusState_Programmatic;
    BOOLEAN bFocused = FALSE;
    bool skipSelection = false;

    IFCPTR_RETURN(pArgs);
    IFC_RETURN(pArgs->get_Key(&key));
    IFC_RETURN(CoreImports::Input_GetKeyboardModifiers(&nModifierKeys));

    const auto lastPointerType = VisualTree::GetContentRootForElement(GetHandle())->GetInputManager().GetLastInputDeviceType();

    switch (key)
    {
    case wsy::VirtualKey_Escape:
        // NOTE: GamepadNavigationCancel routes through the _Escape case here.
        // Ensure that the combobox has focus if it's closed via Escape or GamepadNavigationCancel.
        // Use programmatic focus here to be consistent with Tab; FocusManager will resolve to Keyboard focus.
        IFC_RETURN(Focus(focusState, &bFocused));

        m_isClosingDueToCancel = true;
        IFC_RETURN(put_IsDropDownOpen(FALSE));

        // If we closed the drop-down in response to a cancel key-down,
        // then we should ignore all subsequent cancel key-down messages
        // until we get a key-up, as otherwise we can run into the situation
        // where a user can press B to close a ComboBox, then hold B down
        // long enough for that key press to start repeating, and then at that point
        // the popup has closed and the user might unexpectedly navigate back.
        m_ignoreCancelKeyDowns = true;
        handled = TRUE;
        break;
    case wsy::VirtualKey_Tab:
        // Need to enable this to support focusing out of combobox using tab key.
        IFC_RETURN(Focus(focusState, &bFocused));
        ASSERT(bFocused, L"Focus could not leave ComboBox.");

        IFC_RETURN(put_IsDropDownOpen(FALSE));
        break;
    case wsy::VirtualKey_Space:
        // If we're in searching mode or in Editable mode, then we shouldn't handle the current space KeyDown. Let the Character event handle it.
        // Gamepad A button maps to VirtualKey_Space here, continue into next condition case to select new item.
        if ((IsEditable() || IsInSearchingMode()) && lastPointerType != DirectUI::InputDeviceType::GamepadOrRemote)
        {
            break;
        }
    case wsy::VirtualKey_Enter:
        if (wsy::VirtualKeyModifiers_Menu != (nModifierKeys & (wsy::VirtualKeyModifiers_Control | wsy::VirtualKeyModifiers_Menu)))
        {
            if (IsEditable() && EditableTextHasFocus())
            {
                IFC_RETURN(put_IsDropDownOpen(FALSE));
                handled = TRUE;
            }
            else
            {
                // KeyRoutedEventArgs.OriginalSource (used by WPF) isn't available in Silverlight; use FocusManager.GetFocusedElement instead
                IFC_RETURN(GetFocusedElement(&spFocused));
                spComboBoxItem = spFocused.AsOrNull<xaml_controls::IComboBoxItem>().Cast<ComboBoxItem>();
                if (spComboBoxItem)
                {
                    BOOLEAN bIsSelected = FALSE;
                    IFC_RETURN(spComboBoxItem->get_IsSelected(&bIsSelected));
                    if ((wsy::VirtualKeyModifiers_Control == (nModifierKeys & wsy::VirtualKeyModifiers_Control)) && bIsSelected)
                    {
                        IFC_RETURN(put_SelectedIndex(-1));
                    }
                    else
                    {
                        IFC_RETURN(put_SelectedIndex(m_iFocusedIndex));
                        IFC_RETURN(put_IsDropDownOpen(FALSE));
                    }
                    handled = TRUE;
                }
            }
        }
        break;
    case wsy::VirtualKey_Up:
    case wsy::VirtualKey_Down:
        if (IsEditable()
            && (EditableTextHasFocus() || lastPointerType == DirectUI::InputDeviceType::GamepadOrRemote))
        {
            INT currentSelectedIndex = -1;
            if (IsSearchResultIndexSet())
            {
                currentSelectedIndex = m_searchResultIndex;
            }
            else
            {
                IFC_RETURN(get_SelectedIndex(&currentSelectedIndex));
            }

            newFocusedIndex = currentSelectedIndex + (key == wsy::VirtualKey_Up ? -1 : 1 );

            if (lastPointerType == DirectUI::InputDeviceType::GamepadOrRemote)
            {
                uint32_t itemCount = 0;
                IFC_RETURN(GetItemCount(itemCount));

                // If Popup opened down and moving above the first element, return focus to TextBox.
                // If Popup opened up and moving below the last element, return focus to TextBox.
                if ((newFocusedIndex == -1 && !m_openedUp) || (newFocusedIndex >= (int)itemCount && m_openedUp))
                {
                    BOOLEAN isSuccessful = false;

                    if (m_tpEditableTextPart)
                    {
                        IFC_RETURN(m_tpEditableTextPart.Cast<TextBox>()->Focus(xaml::FocusState_Programmatic, &isSuccessful));
                    }

                    skipSelection = true;

                    handled = TRUE;
                }
                else
                {
                    newFocusedIndex = newFocusedIndex < 0 ? 0 : newFocusedIndex;
                }
            }
            else
            {
                newFocusedIndex = newFocusedIndex < 0 ? 0 : newFocusedIndex;
            }

            break;
        }
        else if (0 != (nModifierKeys & wsy::VirtualKeyModifiers_Menu))
        {
            IFC_RETURN(put_IsDropDownOpen(FALSE));
            handled = TRUE;
            break;
        }
    case wsy::VirtualKey_Home:
    case wsy::VirtualKey_End:
    case wsy::VirtualKey_PageUp:
    case wsy::VirtualKey_PageDown:
    case wsy::VirtualKey_GamepadLeftTrigger:
    case wsy::VirtualKey_GamepadRightTrigger:
        {
            newFocusedIndex = m_iFocusedIndex;
            IFC_RETURN(HandleNavigationKey(key, /*scrollViewport*/ TRUE, newFocusedIndex));
            // When the user presses a navigation key, we want to mark the key as handled. This prevents
            // the key presses from bubbling up to the parent ScrollViewer and inadvertently scrolling.
            handled = TRUE;
        }
        break;
    case wsy::VirtualKey_Left:
    case wsy::VirtualKey_Right:
        // Mark left/right keys as handled to prevent navigation out of the popup, but
        // don't actually change selection.
        handled = TRUE;
        break;
    case wsy::VirtualKey_F4:
        if (nModifierKeys == wsy::VirtualKeyModifiers_None)
        {
            IFC_RETURN(put_IsDropDownOpen(FALSE));
            handled = TRUE;
        }
        break;
    default:
        ASSERT(!handled);
        break;
    }

    if (newFocusedIndex != -1 && !skipSelection)
    {
        handled = TRUE;
        UINT itemCount = 0;
        IFC_RETURN(GetItemCount(itemCount));
        newFocusedIndex = static_cast<INT>(MIN(newFocusedIndex, static_cast<INT>(itemCount) - 1));
        if (0 <= newFocusedIndex)
        {
            auto selectionChangedTrigger = xaml_controls::ComboBoxSelectionChangedTrigger_Always;
            IFC_RETURN(get_SelectionChangedTrigger(&selectionChangedTrigger));

            if (IsEditable()
                && (EditableTextHasFocus() || lastPointerType == DirectUI::InputDeviceType::GamepadOrRemote))
            {
                SetSearchResultIndex(newFocusedIndex);

                ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
                ctl::ComPtr<wfc::IVector<IInspectable*>> spItemsVector;
                ctl::ComPtr<IInspectable> spItem;
                IFC_RETURN(get_Items(&spItems));
                IFC_RETURN(spItems.As(&spItemsVector));

                IFC_RETURN(spItemsVector->GetAt(newFocusedIndex, &spItem));

                IFC_RETURN(UpdateEditableTextBox(spItem.Get(), true /*selectText*/, true /*selectAll*/));

                if (lastPointerType == DirectUI::InputDeviceType::GamepadOrRemote)
                {
                    IFC_RETURN(SetFocusedItem(newFocusedIndex, TRUE /*shouldScrollIntoView*/, FALSE /*forceFocus*/, xaml::FocusState_Keyboard));
                }
                else
                {
                    IFC_RETURN(ScrollIntoView(
                        newFocusedIndex,
                        FALSE /*isGroupItemIndex*/,
                        FALSE /*isHeader*/,
                        FALSE /*isFooter*/,
                        FALSE /*isFromPublicAPI*/,
                        TRUE  /*ensureContainerRealized*/,
                        FALSE /*animateIfBringIntoView*/,
                        xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default));
                }
            }
            else
            {
                IFC_RETURN(SetFocusedItem(newFocusedIndex, TRUE /*shouldScrollIntoView*/, FALSE /*forceFocus*/, xaml::FocusState_Keyboard));
            }

            if (selectionChangedTrigger == xaml_controls::ComboBoxSelectionChangedTrigger_Always)
            {
                IFC_RETURN(put_SelectedIndex(newFocusedIndex));
            }
            else
            {
                IFC_RETURN(OverrideSelectedIndexForVisualStates(newFocusedIndex));
            }
        }
    }

    if (handled)
    {
        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::MainKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    IFCPTR_RETURN(pArgs);
    IFC_RETURN(pArgs->put_Handled(TRUE));
    INT newSelectedIndex = -1;
    wsy::VirtualKeyModifiers nModifierKeys;
    wsy::VirtualKey keyObject = wsy::VirtualKey_None;
    wsy::VirtualKey key = wsy::VirtualKey_None;

    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&key));
    m_inputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::Keyboard;

    IFC_RETURN(pArgs->get_Key(&keyObject));
    IFC_RETURN(CoreImports::Input_GetKeyboardModifiers(&nModifierKeys));
    switch (key)
    {
    case wsy::VirtualKey_Escape:
        if (IsEditable())
        {
            IFC_RETURN(CommitRevertEditableSearch(true /*restoreValue*/));
            IFC_RETURN(ClearSelectedIndexOverrideForVisualStates());

            break;
        }
        else
        {
            IFC_RETURN(pArgs->put_Handled(FALSE));
            break;
        }
    case wsy::VirtualKey_GamepadA:
        {
            if (IsEditable())
            {
                IFC_RETURN(EnsureTextBoxIsEnabled(true /* moveFocusToTextBox */));
            }

            m_inputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::GamepadOrRemote;
            IFC_RETURN(put_IsDropDownOpen(TRUE));
            break;
        }
    case wsy::VirtualKey_GamepadB:
    {
        if (IsEditable() && EditableTextHasFocus())
        {
            BOOLEAN bFocused = false;
            IFC_RETURN(Focus(xaml::FocusState_Programmatic, &bFocused));
        }
        else
        {
            IFC_RETURN(pArgs->put_Handled(FALSE));
        }
        break;
    }
    case wsy::VirtualKey_Enter:
        if (IsEditable())
        {
            IFC_RETURN(CommitRevertEditableSearch(false /*restoreValue*/));
        }
        else if (!IsInSearchingMode())
        {
            IFC_RETURN(put_IsDropDownOpen(TRUE));
        }
        break;
    case wsy::VirtualKey_Tab:
        if (IsEditable())
        {
            IFC_RETURN(CommitRevertEditableSearch(false /*restoreValue*/));
        }

        IFC_RETURN(pArgs->put_Handled(FALSE));
        break;
    case wsy::VirtualKey_Space:
        {
            if (IsEditable())
            {
                // Allow the CharacterReceived handler to handle space character.
                IFC_RETURN(pArgs->put_Handled(FALSE));
            }
            else if (IsInSearchingMode())
            {
                // If we're in TextSearch mode, then process the Space key as a character so it won't get eaten by our parent.
                IFC_RETURN(ProcessSearch(L' '));
            }
            else
            {
                IFC_RETURN(put_IsDropDownOpen(TRUE));
            }
            break;
        }
    case wsy::VirtualKey_Down:
    case wsy::VirtualKey_Up:
        if (IsEditable() || 0 != (nModifierKeys & wsy::VirtualKeyModifiers_Menu))
        {
            IFC_RETURN(put_IsDropDownOpen(TRUE));
            break;
        }
    case wsy::VirtualKey_Home:
    case wsy::VirtualKey_End:
        {
            INT currentSelectedIndex = -1;
            IFC_RETURN(get_SelectedIndex(&currentSelectedIndex));
            newSelectedIndex = currentSelectedIndex;
            IFC_RETURN(HandleNavigationKey(keyObject,  /*scrollViewport*/ FALSE, newSelectedIndex));
        }
        break;

    case wsy::VirtualKey_F4:
        if (nModifierKeys == wsy::VirtualKeyModifiers_None)
        {
            IFC_RETURN(put_IsDropDownOpen(TRUE));
        }
        else
        {
            IFC_RETURN(pArgs->put_Handled(FALSE));
        }
        break;
    default:
        IFC_RETURN(pArgs->put_Handled(FALSE));
        break;
    }

    if (0 <= newSelectedIndex)
    {
        IFC_RETURN(put_SelectedIndex(newSelectedIndex));
    }

    return S_OK;
}


HRESULT ComboBox::OnTextBoxTextChanged(_In_ IInspectable* pSender, _In_ xaml_controls::ITextChangedEventArgs* pArgs)
{
    //DEAD_CODE_REMOVAL
    return S_OK;
}

IFACEMETHODIMP ComboBox::OnPointerWheelChanged(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bHasFocus = FALSE;
    BOOLEAN bIsDropDownOpen = FALSE;
    INT delta = 0;
    INT selectedIndex = -1;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;

    IFC_RETURN(get_IsEnabled(&bIsEnabled));
    IFC_RETURN(HasFocus(&bHasFocus));
    IFC_RETURN(get_IsDropDownOpen(&bIsDropDownOpen));

    if (!bIsEnabled)
    {
        return S_OK;
    }

    if (bHasFocus)
    {
        if (!bIsDropDownOpen)
        {
            IFC_RETURN(pArgs->GetCurrentPoint(this, &spPointerPoint));
            IFCPTR_RETURN(spPointerPoint);
            IFC_RETURN(spPointerPoint->get_Properties(&spPointerProperties));
            IFCPTR_RETURN(spPointerProperties);
            IFC_RETURN(spPointerProperties->get_MouseWheelDelta(&delta));

            IFC_RETURN(get_SelectedIndex(&selectedIndex));
            if (delta < 0)
            {
                IFC_RETURN(SelectNext(selectedIndex));
            }
            else
            {
                IFC_RETURN(SelectPrev(selectedIndex));
            }
            IFC_RETURN(put_SelectedIndex(selectedIndex));
        }
        IFC_RETURN(pArgs->put_Handled(TRUE));
    }

    IFC_RETURN(ComboBoxGenerated::OnPointerWheelChanged(pArgs));

    return S_OK;
}

IFACEMETHODIMP ComboBox::OnPointerEntered(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isEventSourceTarget = FALSE;
    ctl::ComPtr<IRoutedEventArgs> spRoutedArgs;

    IFC(ComboBoxGenerated::OnPointerEntered(pArgs));

    IFC(ctl::do_query_interface(spRoutedArgs, pArgs));
    IFC(IsEventSourceTarget(spRoutedArgs.Get(), &isEventSourceTarget));

    if (isEventSourceTarget)
    {
        m_IsPointerOverMain = true;
        m_bIsPressed = false;
        IFC(UpdateVisualState());
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ComboBox::OnPointerMoved(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isEventSourceTarget = FALSE;
    ctl::ComPtr<IRoutedEventArgs> spRoutedArgs;

    IFC(ComboBoxGenerated::OnPointerMoved(pArgs));

    IFC(ctl::do_query_interface(spRoutedArgs, pArgs));
    IFC(IsEventSourceTarget(spRoutedArgs.Get(), &isEventSourceTarget));


    if (isEventSourceTarget)
    {
        if (!m_IsPointerOverMain)
        {
            // The pointer just entered the target area of the ComboBox
            m_IsPointerOverMain = true;
            IFC(UpdateVisualState());
        }
    }
    else if (m_IsPointerOverMain)
    {
        // The pointer just left the target area of the ComboBox
        m_IsPointerOverMain = false;
        m_bIsPressed = false;
        IFC(UpdateVisualState());
    }


Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ComboBox::OnPointerExited(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    IFC(ComboBoxGenerated::OnPointerExited(pArgs));

    m_IsPointerOverMain = false;
    m_bIsPressed = false;
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ComboBox::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;

    IFC(ComboBoxGenerated::OnPointerCaptureLost(pArgs));

    IFC(pArgs->get_Pointer(&spPointer));

    // For touch, we can clear PointerOver when receiving PointerCaptureLost, which we get when the finger is lifted
    // or from cancellation, e.g. pinch-zoom gesture in ScrollViewer.
    // For mouse, we need to wait for PointerExited because the mouse may still be above the ButtonBase when
    // PointerCaptureLost is received from clicking.
    IFC(pArgs->GetCurrentPoint(nullptr, &spPointerPoint));
    IFCPTR(spPointerPoint);
    IFC(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));
    if (nPointerDeviceType == mui::PointerDeviceType_Touch)
    {
        m_IsPointerOverMain = false;
    }

    m_bIsPressed = false;
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::IsLeftButtonPressed(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs,
    _Out_ BOOLEAN* pIsLeftButtonPressed,
    _Out_opt_ mui::PointerDeviceType* pPointerDeviceType)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;

    // Caller guarantees non-NULL:
    ASSERT(pArgs);
    ASSERT(pIsLeftButtonPressed);

    IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
    IFCPTR(spPointerPoint);
    IFC(spPointerPoint->get_Properties(&spPointerProperties));
    IFCPTR(spPointerProperties);
    IFC(spPointerProperties->get_IsLeftButtonPressed(pIsLeftButtonPressed));

    if (pPointerDeviceType)
    {
        IFC(spPointerPoint->get_PointerDeviceType(pPointerDeviceType));
    }

Cleanup:
    RRETURN(hr);
}

// PointerPressed event handler.
IFACEMETHODIMP ComboBox::OnPointerPressed(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    BOOLEAN bIsHandled = FALSE;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsLeftButtonPressed = FALSE;
    BOOLEAN isEventSourceTarget = FALSE;
    ctl::ComPtr<IRoutedEventArgs> spRoutedArgs;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;

    IFC_RETURN(ComboBoxGenerated::OnPointerPressed(pArgs));
    IFCPTR_RETURN(pArgs);

    IFC_RETURN(pArgs->get_Handled(&bIsHandled));
    if (bIsHandled)
    {
        return S_OK;
    }

    IFC_RETURN(get_IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        return S_OK;
    }

    IFC_RETURN(ctl::do_query_interface(spRoutedArgs, pArgs));
    IFC_RETURN(IsEventSourceTarget(spRoutedArgs.Get(), &isEventSourceTarget));

    if (isEventSourceTarget)
    {
        IFC_RETURN(IsLeftButtonPressed(pArgs, &bIsLeftButtonPressed, NULL));

        if (bIsLeftButtonPressed)
        {
            IFC_RETURN(pArgs->put_Handled(TRUE));

            m_bIsPressed = true;

            // for "Pressed" visual state to render
            IFC_RETURN(UpdateVisualState());
        }
    }

    IFC_RETURN(pArgs->GetCurrentPoint(nullptr, &spPointerPoint));
    IFCPTR_RETURN(spPointerPoint);
    IFC_RETURN(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));

    BOOLEAN popupIsOpen = TRUE;

    if (m_tpPopupPart)
    {
        IFC_RETURN(m_tpPopupPart->get_IsOpen(&popupIsOpen));
    }

    if (!popupIsOpen && nPointerDeviceType == mui::PointerDeviceType_Touch)
    {
        // Open popup after ComboBox is focused due to the PointerPressed event.
        m_openPopupOnTouch = true;
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnTextBoxPointerPressedPrivate(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;

    IFC_RETURN(pArgs->GetCurrentPoint(nullptr, &spPointerPoint));
    IFCPTR_RETURN(spPointerPoint);
    IFC_RETURN(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));

    BOOLEAN popupIsOpen = TRUE;

    if (m_tpPopupPart)
    {
        IFC_RETURN(m_tpPopupPart->get_IsOpen(&popupIsOpen));
    }

    // On Touch open DropDown when getting focus.
    if (!popupIsOpen && nPointerDeviceType == mui::PointerDeviceType_Touch)
    {
        m_inputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::Touch;
        IFC_RETURN(put_IsDropDownOpen(TRUE));
    }

    pArgs->put_Handled(TRUE);

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnTextBoxTapped(
    _In_ IInspectable* pSender,
    _In_ xaml_input::ITappedRoutedEventArgs* pArgs)
{
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;

    IFC_RETURN(pArgs->get_PointerDeviceType(&nPointerDeviceType));

    if (m_selectAllOnTouch && nPointerDeviceType == mui::PointerDeviceType_Touch && m_tpEditableTextPart)
    {
        IFC_RETURN(m_tpEditableTextPart->SelectAll());
    }

    // Reset this flag even on mouse click
    m_selectAllOnTouch = false;

    pArgs->put_Handled(TRUE);

    return S_OK;
}

// PointerReleased event handler.
IFACEMETHODIMP ComboBox::OnPointerReleased(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    BOOLEAN bIsHandled = FALSE;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsLeftButtonPressed = FALSE;
    GestureModes gestureFollowing = GestureModes::None;
    mui::PointerDeviceType nPointerDeviceType;
    BOOLEAN isEventSourceTarget = FALSE;
    ctl::ComPtr<IRoutedEventArgs> spRoutedArgs;

    IFC_RETURN(ComboBoxGenerated::OnPointerReleased(pArgs));
    IFCPTR_RETURN(pArgs);

    IFC_RETURN(pArgs->get_Handled(&bIsHandled));
    if (bIsHandled)
    {
        return S_OK;
    }

    IFC_RETURN(get_IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        return S_OK;
    }

    IFC_RETURN(ctl::do_query_interface(spRoutedArgs, pArgs));
    IFC_RETURN(IsEventSourceTarget(spRoutedArgs.Get(), &isEventSourceTarget));

    if (isEventSourceTarget)
    {
        IFC_RETURN(IsLeftButtonPressed(pArgs, &bIsLeftButtonPressed, &nPointerDeviceType));
        m_shouldPerformActions = (m_bIsPressed && !bIsLeftButtonPressed);

        if (m_shouldPerformActions)
        {
            m_bIsPressed = false;
            if (nPointerDeviceType == mui::PointerDeviceType_Touch)
            {
                m_inputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::Touch;
            }
            else
            {
                m_inputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::Mouse;
            }
        }
        IFC_RETURN(static_cast<PointerRoutedEventArgs*>(pArgs)->get_GestureFollowing(&gestureFollowing));
        if (gestureFollowing == GestureModes::RightTapped)
        {
            // We will get a right tapped event for every time we visit here, and
            // we will visit before each time we receive a right tapped event
            return S_OK;
        }

        if (m_shouldPerformActions)
        {
            // Note that we are intentionally NOT handling the args
            // if we do not fall through here because basically we are no_opting in that case.
            IFC_RETURN(pArgs->put_Handled(TRUE));
            m_bIsPressed = false;
            if (nPointerDeviceType == mui::PointerDeviceType_Touch)
            {
                m_inputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::Touch;
            }
            else
            {
                m_inputDeviceTypeUsedToOpen = DirectUI::InputDeviceType::Mouse;
            }

            IFC_RETURN(PerformPointerUpAction(IsDropDownOverlay(spRoutedArgs.Get())));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnRightTappedUnhandled(
    _In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
{
    BOOLEAN isHandled = FALSE;
    BOOLEAN isEventSourceTarget = FALSE;
    ctl::ComPtr<IRoutedEventArgs> spRoutedArgs;

    IFC_RETURN(ComboBoxGenerated::OnRightTappedUnhandled(pArgs));
    IFC_RETURN(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        return S_OK;
    }

    IFC_RETURN(ctl::do_query_interface(spRoutedArgs, pArgs));
    IFC_RETURN(IsEventSourceTarget(spRoutedArgs.Get(), &isEventSourceTarget));

    if (isEventSourceTarget)
    {
        IFC_RETURN(PerformPointerUpAction(false /*isDropDownOverlay*/));
    }

    return S_OK;
}

// Perform the primary action related to pointer up.
_Check_return_ HRESULT
ComboBox::PerformPointerUpAction(_In_ bool isDropDownOverlay)
{
    BOOLEAN bFocused = false;

    if (m_shouldPerformActions)
    {
        m_shouldPerformActions = false;

        IFC_RETURN(Focus(xaml::FocusState_Pointer, &bFocused));

        // No need to test bFocused - it is possible no focusable element is present if IsTabStop = FALSE for ComboBox
        // We use isDropDownOverlay to determine if dropdown arrow was clicked on Editable mode.
        if(!IsEditable())
        {
            IFC_RETURN(put_IsDropDownOpen(TRUE));
        }
        else if(isDropDownOverlay)
        {
            BOOLEAN isDropDownOpen = false;
            IFC_RETURN(get_IsDropDownOpen(&isDropDownOpen));

            // Open/Close the DropDown when clicking the DropDownOverlay for Editable Mode.
            IFC_RETURN(put_IsDropDownOpen(!isDropDownOpen));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnElementPopupChildGotFocus(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN hasFocus = FALSE;
    ASSERT(!IsSmallFormFactor(), L"OnElementPopupChildGotFocus is not used in small form factor mode");

    IFC(HasFocus(&hasFocus));
    IFC(FocusChanged(hasFocus));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnElementPopupChildLostFocus(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN hasFocus = FALSE;
    ASSERT(!IsSmallFormFactor(), L"OnElementPopupChildLostFocus is not used in small form factor mode");

    IFC(HasFocus(&hasFocus));
    IFC(FocusChanged(hasFocus));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnElementPopupChildPointerEntered(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ASSERT(!IsSmallFormFactor(), L"OnElementPopupChildPointerEntered is not used in small form factor mode");

    m_IsPointerOverPopup = true;
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnElementPopupChildPointerExited(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    m_IsPointerOverPopup = false;
    ASSERT(!IsSmallFormFactor(), L"OnElementPopupChildPointerExited is not used in small form factor mode");

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnDropDownOverlayPointerEntered(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    m_IsPointerOverDropDownOverlay = true;
    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnDropDownOverlayPointerExited(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    m_IsPointerOverDropDownOverlay = false;
    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnElementPopupChildSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ASSERT(!IsSmallFormFactor(), L"OnElementPopupChildSizeChanged is not used in small form factor mode");

    IFC(ArrangePopup(false));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnTextBoxSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    IFC_RETURN(ArrangePopup(false));

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnTextBoxCandidateWindowBoundsChanged(
    _In_ xaml_controls::ITextBox* /*pSender*/,
    _In_ xaml_controls::ICandidateWindowBoundsChangedEventArgs* pArgs)
{
    wf::Rect candidateWindowBounds;

    IFC_RETURN(pArgs->get_Bounds(&candidateWindowBounds));

    // Do nothing if the candidate windows bound did not change
    if (RectUtil::AreEqual(m_candidateWindowBoundsRect, candidateWindowBounds))
    {
        return S_OK;
    }

    m_candidateWindowBoundsRect = candidateWindowBounds;
    IFC_RETURN(ArrangePopup(false));

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnElementPopupChildLoaded(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    // Ensure searched item gets selected when the popup opens.
    if (IsEditable())
    {
        auto selectionChangedTrigger = xaml_controls::ComboBoxSelectionChangedTrigger_Always;
        IFC_RETURN(get_SelectionChangedTrigger(&selectionChangedTrigger));

        INT currentSelectedIndex = 0;

        if (selectionChangedTrigger == xaml_controls::ComboBoxSelectionChangedTrigger_Always || !IsSearchResultIndexSet())
        {
            INT selectedIndex = -1;
            IFC_RETURN(get_SelectedIndex(&selectedIndex));

            currentSelectedIndex = selectedIndex;
        }
        else
        {
            // Ensure searched item gets selected.
            IFC_RETURN(OverrideSelectedIndexForVisualStates(m_searchResultIndex));

            currentSelectedIndex = m_searchResultIndex;
        }

        if (currentSelectedIndex >= 0)
        {
            IFC_RETURN(ScrollIntoView(
                currentSelectedIndex,
                FALSE /*isGroupItemIndex*/,
                FALSE /*isHeader*/,
                FALSE /*isFooter*/,
                FALSE /*isFromPublicAPI*/,
                TRUE  /*ensureContainerRealized*/,
                FALSE /*animateIfBringIntoView*/,
                xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnElementOutsidePopupPointerPressed(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsLeftButtonPressed = FALSE;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;

    ASSERT(!IsSmallFormFactor(), L"OnElementOutsidePopupPointerPressed is not used in small form factor mode");

    IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
    IFCPTR(spPointerPoint);
    IFC(spPointerPoint->get_Properties(&spPointerProperties));
    IFCPTR(spPointerProperties);
    IFC(spPointerProperties->get_IsLeftButtonPressed(&bIsLeftButtonPressed));

    if (bIsLeftButtonPressed)
    {
        m_isClosingDueToCancel = true;
        IFC(put_IsDropDownOpen(FALSE));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnPopupClosed(_In_opt_ IInspectable*, _In_opt_ IInspectable*)
{
    // Under most circumstances, this is unnecessary as it is ComboBox that closes the Popup. However for certain light-dismiss
    // scenarios (i.e. window resize) the Popup will close itself, so we need to close the ComboBox here to keep everything in
    // sync.
    if (m_tpPopupPart)
    {
        // Popup.Closed is an asynchronous event, however, so we should check to make sure that the popup is still closed
        // before we do anything.
        BOOLEAN popupIsOpen;
        IFC_RETURN(m_tpPopupPart->get_IsOpen(&popupIsOpen));

        if (!popupIsOpen)
        {
            IFC_RETURN(put_IsDropDownOpen(false));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnItemsChanged(
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    BOOLEAN isDropDownOpen = FALSE;
    BOOLEAN oldIsInline = IsInline();

    // Cache the new item count
    IFC(GetItemCount(m_itemCount));
    IFC(get_IsDropDownOpen(&isDropDownOpen));

    IFC(ComboBoxGenerated::OnItemsChanged(e));

    if (IsSmallFormFactor())
    {
        if (isDropDownOpen)
        {
            // In small form factor mode we don't allow the ComboBox to remain open through
            // changing the items collection.
            // This will trigger an UpdateVisualState.
            IFC(put_IsDropDownOpen(FALSE));
        }
        else if (IsInline() != oldIsInline)
        {
            if (oldIsInline)
            {
                INT32 selectedIndex = 0;
                IFC(get_SelectedIndex(&selectedIndex));
                IFC(EnsurePresenterReadyForFullMode());
                IFC(SetContentPresenter(selectedIndex));
            }
            else
            {
                IFC(EnsurePresenterReadyForInlineMode());
                IFC(ForceApplyInlineLayoutUpdate());
            }

            IFC(UpdateVisualState(TRUE));
        }
    }

Cleanup:
    RRETURN(hr);
}

// should the keycode be ignored when processing characters for search
bool ComboBox::ShouldIgnoreKeyCode(WCHAR keyCode)
{
    return keyCode == VK_ESCAPE;
}

_Check_return_ HRESULT ComboBox::OnCharacterReceived(_In_ IUIElement* pSender, _In_ xaml_input::ICharacterReceivedRoutedEventArgs* pArgs)
{
    if (IsTextSearchEnabled())
    {
        WCHAR keyCode;
        pArgs->get_Character(&keyCode);

        if (!IsEditable())
        {
            // Space should have been handled by now because we handle the Space key in the KeyDown event handler.
            // NOTE: The 2 below specifies the map type, and maps VK to CHAR
            ASSERT(L' ' != keyCode);
        }

        if (!ShouldIgnoreKeyCode(keyCode))
        {
            IFC_RETURN(ProcessSearch(static_cast<WCHAR>(keyCode)));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OnPopupCharacterReceived(_In_ IUIElement* pSender, _In_ xaml_input::ICharacterReceivedRoutedEventArgs* pEventArgs)
{
    ctl::ComPtr<ComboBox> parentComboBox;

    UIElement* const pSenderAsUIE = static_cast<UIElement*>(pSender);

    IFC_RETURN(FindParentComboBoxFromDO(pSenderAsUIE->GetHandle(), &parentComboBox));

    if (parentComboBox != nullptr && parentComboBox->IsTextSearchEnabled())
    {
        WCHAR keyCode;
        IFC_RETURN(pEventArgs->get_Character(&keyCode));

        if (!ShouldIgnoreKeyCode(keyCode))
        {
            IFC_RETURN(parentComboBox->ProcessSearch(keyCode));
        }
    }

    return S_OK;
}

// Given a DependencyObject, attempt to find a ComboBox ancestor in its logical tree.
// This method allows us to go from items within the dropdown to the ComboBox, and is useful for
// scenarios where we need to get to the ComboBox from the items inside (like TypeAhead).
_Check_return_ HRESULT ComboBox::FindParentComboBoxFromDO(_In_ CDependencyObject* pSender, _Out_ ComboBox** parentComboBox)
{
    ctl::ComPtr<IFrameworkElement> current = nullptr;

    ctl::ComPtr<DependencyObject> spItemManagedPeer;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pSender, &spItemManagedPeer));

    current = spItemManagedPeer.AsOrNull<IFrameworkElement>();

    while (current)
    {
        ctl::ComPtr<IComboBox> comboBox = current.AsOrNull<IComboBox>();
        if (comboBox)
        {
            IFC_RETURN(comboBox.CopyTo(parentComboBox));

            break;
        }

        ctl::ComPtr<IDependencyObject> parent;
        IFC_RETURN(current->get_Parent(&parent));

        // Try querying for our templated parent if the logical
        // parent is null to handle the case where our target
        // element is a template part.  We don't just use
        // VisualTreeHelper because that wouldn't return our
        // parent popup; it would give us the popup root, which
        // isn't useful.
        if (!parent)
        {
            ctl::ComPtr<DependencyObject> parentDO;
            IFC_RETURN(current.Cast<FrameworkElement>()->get_TemplatedParent(&parentDO));
            IFC_RETURN(parentDO.As(&parent));
        }

        current = parent.AsOrNull<IFrameworkElement>();
    }

    return S_OK;
}

bool ComboBox::IsTextSearchEnabled()
{
    BOOLEAN isTextSearchEnabled = FALSE;
    get_IsTextSearchEnabled(&isTextSearchEnabled);

    return !!isTextSearchEnabled;
}

bool ComboBox::IsEditable()
{
    BOOLEAN isEditable = FALSE;
    IFCFAILFAST(get_IsEditable(&isEditable));

    return !!isEditable;
}

bool ComboBox::HasSearchStringTimedOut()
{
    const int timeOutInMilliseconds = 1000;

    auto now = Jupiter::HighResolutionClock::now();

    return (now - m_timeSinceLastCharacterReceived) > std::chrono::milliseconds(timeOutInMilliseconds);
}

_Check_return_ HRESULT ComboBox::ProcessSearch(_In_ WCHAR keyCode)
{
    int foundIndex = -1;

    if (IsEditable())
    {
        if (!m_tpEditableTextPart)
        {
            return S_OK;
        }

        wrl_wrappers::HString textBoxText;
        IFC_RETURN(m_tpEditableTextPart->get_Text(textBoxText.GetAddressOf()));

        // Don't process search if new text is equal to previous searched text.
        if (AreStringsEqual(textBoxText, m_searchString))
        {
            return S_OK;
        }

        if (textBoxText.Length() != 0)
        {
            IFC_RETURN(SearchItemSourceIndex(keyCode, false /*startSearchFromCurrentIndex*/, false /*searchExactMatch*/, foundIndex));
        }
        else
        {
            m_searchString.Release();
        }

        SetSearchResultIndex(foundIndex);

        auto selectionChangedTrigger = xaml_controls::ComboBoxSelectionChangedTrigger_Always;
        IFC_RETURN(get_SelectionChangedTrigger(&selectionChangedTrigger));

        if (selectionChangedTrigger == xaml_controls::ComboBoxSelectionChangedTrigger_Always && foundIndex > -1)
        {
            IFC_RETURN(put_SelectedIndex(foundIndex));
        }

        BOOLEAN isDropDownOpen = FALSE;
        IFC_RETURN(get_IsDropDownOpen(&isDropDownOpen));

        // Override selected visuals only if popup is open
        if (isDropDownOpen)
        {
            IFC_RETURN(OverrideSelectedIndexForVisualStates(foundIndex));
        }

        if (foundIndex >= 0)
        {
            if (isDropDownOpen)
            {
                IFC_RETURN(ScrollIntoView(
                    foundIndex,
                    FALSE /*isGroupItemIndex*/,
                    FALSE /*isHeader*/,
                    FALSE /*isFooter*/,
                    FALSE /*isFromPublicAPI*/,
                    TRUE  /*ensureContainerRealized*/,
                    FALSE /*animateIfBringIntoView*/,
                    xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default));
            }
        }
    }
    else
    {
        IFC_RETURN(SearchItemSourceIndex(keyCode, true /*startSearchFromCurrentIndex*/, false /*searchExactMatch*/, foundIndex));

        if (foundIndex >= 0)
        {
            IFC_RETURN(put_SelectedIndex(foundIndex));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::SearchItemSourceIndex(_In_ WCHAR keyCode, _In_ bool startSearchFromCurrentIndex, _In_ bool searchExactMatch, _Outptr_ int& foundIndex)
{
    // Get all of the ComboBox items; we'll try to convert them to strings later.
    UINT itemCount = 0;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spItemsVector;

    IFC_RETURN(get_Items(&spItems));
    IFC_RETURN(spItems.As(&spItemsVector));
    IFC_RETURN(GetItemCount(itemCount));

    int searchIndex = -1;

    BOOLEAN newStringCreated;

    // Editable ComboBox uses the text in the TextBox to search for values, Non-Editable ComboBox appends received characters to the current search string
    if (IsEditable())
    {
        if (m_tpEditableTextPart)
        {
            IFC_RETURN(m_tpEditableTextPart->get_Text(m_searchString.GetAddressOf()));
        }
    }
    else
    {
        IFC_RETURN(AppendCharToSearchString(keyCode, &newStringCreated));
    }

    if (startSearchFromCurrentIndex)
    {
        INT currentSelectedIndex = -1;
        IFC_RETURN(get_SelectedIndex(&currentSelectedIndex));
        searchIndex = currentSelectedIndex;

        if (newStringCreated)
        {
            // If we've created a new search string, then we shouldn't search at i, but rather at i+1.
            if (searchIndex < static_cast<INT>(itemCount) - 1)
            {
                // We have at least one more item after this one to start searching at.
                searchIndex++;
            }
            else
            {
                // We are at the end of the list. Loop the search.
                searchIndex = 0;
            }
        }
        else
        {
            // If we just appended to the search string, then ensure that the search index is valid (>= 0)
            searchIndex = (searchIndex >= 0) ? searchIndex : 0;
        }
    }
    else
    {
        searchIndex = 0;
    }

    ASSERT(searchIndex >= 0);

    ctl::ComPtr<IInspectable> spItem;
    wrl_wrappers::HString strItem;
    foundIndex = -1;

    IFC_RETURN(EnsurePropertyPathListener());

    // Iterate through all of the items. Try to get a string out of the item; if it matches, break. If not, keep looking.
    // TODO: [https://task.ms/6720676] Use CoreDispatcher/BuildTree to slice TypeAhead search logic
    for (UINT i = 0; i < itemCount; i++)
    {
        IFC_RETURN(spItemsVector->GetAt(searchIndex, &spItem));

        if (spItem)
        {
            IFC_RETURN(TryGetStringValue(spItem.Get(), m_spPropertyPathListener.Get(), strItem.GetAddressOf()));

            if (strItem == nullptr)
            {
                // We couldn't get the string representing this item; it doesn't make sense to continue searching because
                // we're probably not going to be able to get strings from more items in this collection.
                break;
            }

            // Trim leading spaces on the item before comparing.
            IFC_RETURN(strItem.TrimStart(wrl_wrappers::HStringReference(L" "), strItem));

            // On Editable mode Backspace should only search for exact matches. This prevents auto-complete from stopping backspacing.
            if (searchExactMatch || IsEditable() && keyCode == VK_BACK)
            {
                if (AreStringsEqual(strItem, m_searchString))
                {
                    foundIndex = searchIndex;

                    break;
                }
            }
            else if (ComboBox::StartsWithIgnoreLinguisticSemantics(strItem, m_searchString))
            {
                foundIndex = searchIndex;

                // If matching item was found auto-complete word.
                if (IsEditable())
                {
                    IFC_RETURN(UpdateEditableTextBox(spItem.Get(), true /*selectText*/, false /*selectAll*/));
                }

                break;
            }
        }

        searchIndex++;

        // If we've gotten to the end of the list, loop the search.
        if (searchIndex == static_cast<INT>(itemCount))
        {
            searchIndex = 0;
        }
    }

    return S_OK;
}

bool ComboBox::StartsWithIgnoreLinguisticSemantics(const wrl_wrappers::HString& strSource, const wrl_wrappers::HString& strPrefix)
{
    // The goal of this method is to return true if strPrefix is found at the start of strSource regardless of linguistic semantics.
    // For example, if we've got strSource = "wAsHINGton" and strPrefix = "Wa", we should return true from this method.
    // FindNLSStringEx will return a 0-based index into the source string if it's successful; it will return < 0 if it failed to find a match.
    // We pass in a number of flags to achieve this behavior:
    // FIND_STARTSWITH : Test to find out if the strPrefix value is the first value in the Source string.
    // NORM_IGNORECASE: Ignore case (broader than LINGUISTIC_IGNORECASE)
    // NORM_IGNOREKANATYPE: Do not differentiate between hiragana and katakana characters (corresponding chars compare as equal)
    // NORM_IGNOREWIDTH: Used in Japanese and Chinese scripts, this flag ignores the difference between half- and full-width characters
    // NORM_LINGUISTIC_CASING: Use linguistic rules for casing instead of file system rules
    // LINGUISTIC_IGNOREDIACRITIC: Ignore diacritics (Dotless Turkish i maps to dotted i).
    UINT32 lenSource = 0;
    UINT32 lenPrefix = 0;

    const wchar_t* bufferSource = strSource.GetRawBuffer(&lenSource);
    const wchar_t* bufferPrefix = strPrefix.GetRawBuffer(&lenPrefix);
    bool compareValue = false;
    if (lenPrefix <= lenSource)
    {
        compareValue = FindNLSStringEx(
            LOCALE_NAME_USER_DEFAULT,
            FIND_STARTSWITH | NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | NORM_LINGUISTIC_CASING | LINGUISTIC_IGNOREDIACRITIC,
            bufferSource, lenSource,
            bufferPrefix, lenPrefix,
            nullptr, /* pcchFound */
            nullptr, /* lpVersionInformation */
            nullptr, /* lpReserved */
            0 /* sortHandle */) >= 0;
    }

    return compareValue;
}

bool ComboBox::AreStringsEqual(const wrl_wrappers::HString& str1, const wrl_wrappers::HString& str2)
{
    UINT32 lenStr1 = 0;
    UINT32 lenStr2 = 0;

    const wchar_t* buffer1 = str1.GetRawBuffer(&lenStr1);
    const wchar_t* buffer2 = str2.GetRawBuffer(&lenStr2);

    if (lenStr1 != lenStr2)
        return false;

    int result = _wcsicmp(buffer1, buffer2);
    return result == 0;
}

bool ComboBox::IsSearchStringValid(const wrl_wrappers::HString& str)
{
    if (str.IsEmpty())
    {
        return false;
    }

    wrl_wrappers::HString trimmedStr;
    IFCFAILFAST(str.TrimStart(wrl_wrappers::HStringReference(L" "), trimmedStr));

    return !trimmedStr.IsEmpty();
}

bool ComboBox::IsInSearchingMode()
{
    if (HasSearchStringTimedOut())
    {
        m_isInSearchingMode = false;
    }
    return IsTextSearchEnabled() && m_isInSearchingMode;
}

_Check_return_ HRESULT ComboBox::TryGetStringValue(_In_ IInspectable* object, _In_opt_ PropertyPathListener* pathListener, _Out_ HSTRING* value)
{
    IFCPTR_RETURN(object);
    ASSERT(value != nullptr);

    ctl::ComPtr<IInspectable> spBoxedValue;
    ctl::ComPtr<IInspectable> spObject(object);
    ctl::ComPtr<xaml_data::ICustomPropertyProvider> spObjectPropertyAccessor;
    ctl::ComPtr<wf::IStringable> spStringable;

    if( SUCCEEDED(spObject.As(&spObjectPropertyAccessor)) )
    {
        if (pathListener != nullptr)
        {
            // Our caller has provided us with a PropertyPathListener. By setting the source of the listener, we can pull a value out.
            // This is our boxedValue, which we effectively ToString below.
            IFC_RETURN(pathListener->SetSource(spObject.Get()));
            IFC_RETURN(pathListener->GetValue(&spBoxedValue));
        }
        else
        {
            // No PathListener specified, but this object implements
            // ICustomPropertyProvider. Call .ToString on the object:
            IFC_RETURN(spObjectPropertyAccessor->GetStringRepresentation(value));
            return S_OK;
        }
    }
    else
    {
        // Try to get the string value by unboxing the object itself.
        spBoxedValue = spObject;
    }

    if (spBoxedValue != nullptr)
    {
        if(SUCCEEDED(spBoxedValue.As(&spStringable)))
        {
            // We've set a BoxedValue. If it is castable to a string, try to ToString it.
            IFC_RETURN(spStringable->ToString(value));
        }
        else
        {
            // We've set a BoxedValue, but we can't directly ToString it. Try to get a string out of it.
            IFC_RETURN(FrameworkElement::GetStringFromObject(spBoxedValue.Get(), value));
        }
    }
    else
    {
        // If we haven't found a BoxedObject and it's not Stringable, try one last time to get a string out.
        IFC_RETURN(FrameworkElement::GetStringFromObject(object, value));
    }

    return S_OK;
}

HRESULT ComboBox::AppendCharToSearchString(_In_ WCHAR ch, _Out_ BOOLEAN* createdNewString)
{
    IFCPTR_RETURN(createdNewString);

    *createdNewString = FALSE;
    if (HasSearchStringTimedOut())
    {
        ResetSearchString();
        *createdNewString = TRUE;
    }

    m_timeSinceLastCharacterReceived = Jupiter::HighResolutionClock::now();

    const UINT maxNumCharacters = 256;

    // Only append a new character if we're less than the max string length.
    if (m_searchString.Length() <= maxNumCharacters)
    {
        //While we only want to append a single character (ch), we need to create a
        // null-terminated string in order to be able to correctly concatenate onto
        // m_searchString.
        WCHAR toConcat[2];
        toConcat[0] = ch;
        toConcat[1] = '\0';
        IFC_RETURN(m_searchString.Concat(wrl_wrappers::HStringReference(toConcat, 1), m_searchString));
    }
    m_isInSearchingMode = true;

    return S_OK;
}

void ComboBox::ResetSearchString()
{
    m_searchString.Release();
}

_Check_return_ HRESULT ComboBox::RaiseTextSubmittedEvent(
    _In_ wrl_wrappers::HString& text,
    _Out_ BOOLEAN* isHandled)
{
    //// Create and set event args
    xref_ptr<CComboBoxTextSubmittedEventArgs> eventArgs = make_xref<CComboBoxTextSubmittedEventArgs>(text.Get());

    //// Raise TextSubmitted event
    ctl::ComPtr<IInspectable> pArgsAsI;
    IFC_RETURN(eventArgs->CreateFrameworkPeer(&pArgsAsI));
    ctl::ComPtr<IComboBoxTextSubmittedEventArgs> pArgs;
    pArgsAsI.As(&pArgs);
    TextSubmittedEventSourceType *pEventSource = nullptr;
    IFC_RETURN(ComboBoxGenerated::GetTextSubmittedEventSourceNoRef(&pEventSource));
    IFC_RETURN(pEventSource->Raise(this, pArgs.Get()));

    *isHandled = eventArgs->GetHandled();
    return S_OK;
}

#pragma endregion Event Handlers

#pragma region Small Form Factor Event Handlers

_Check_return_ HRESULT ComboBox::OnFlyoutButtonClick(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    ASSERT(m_tpFlyoutButtonPart, "The flyout button click event cannot occur if the flyout button is null.");

    IFC(put_IsDropDownOpen(TRUE));

Cleanup:
    RRETURN(hr);
}

// Callback passed to the GetListPickerSelectionAsync method. Called when a "full mode" selection
// is completed or cancelled
_Check_return_ HRESULT ComboBox::OnGetListPickerSelectionAsyncCompleted(
    _In_ wf::IAsyncOperation<INT32>* pAsyncState,
    _In_ wf::AsyncStatus asyncStatus)
{
    HRESULT hr = S_OK;

    ASSERT(m_tpAsyncSelectionInfo);

    if (asyncStatus == wf::AsyncStatus::Completed)
    {
        INT32 selectedIndex = 0;
        IFC(pAsyncState->GetResults(&selectedIndex));
        IFC(put_SelectedIndex(selectedIndex));
        IFC(put_IsDropDownOpen(FALSE));
    }
    else
    {
#if DBG
        BOOLEAN isDropDownOpen = FALSE;
        IFC(get_IsDropDownOpen(&isDropDownOpen));
        ASSERT(!isDropDownOpen, "The only way the operation can be cancelled is to programatically set isDropDownOpen to FALSE.");
#endif
        // We're done with the async operation.
        IFC(FinishClosingDropDown());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnItemsPresenterHostParentSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    wf::Size hostParentNewSize = {};
    wf::Rect clipRect = {};
    ctl::ComPtr<RectangleGeometry> spClipRect;
    ctl::ComPtr<IFrameworkElement> spItemsPresenterAsFE;
    ctl::ComPtr<IUIElement> spItemsPresenterAsUI;
    wf::Size dummySize = {XFLOAT_INF, XFLOAT_INF};
    wf::Size desiredSize = {};
    DOUBLE width = 0;
    xaml::HorizontalAlignment horizontalAlignment = xaml::HorizontalAlignment_Left;

    IFCEXPECT_ASSERT(m_tpItemsPresenterHostPart && m_tpItemsPresenterHostParent);

    IFC(pArgs->get_NewSize(&hostParentNewSize));
    IFC(get_Width(&width));
    IFC(get_HorizontalAlignment(&horizontalAlignment));

    IFC(m_tpItemsPresenterPart.As(&spItemsPresenterAsFE));
    IFC(m_tpItemsPresenterPart.As(&spItemsPresenterAsUI));
    IFC(spItemsPresenterAsFE->put_Width(DoubleUtil::NaN));
    IFC(spItemsPresenterAsUI->Measure(dummySize));
    IFC(spItemsPresenterAsUI->get_DesiredSize(&desiredSize));

    if (DoubleUtil::IsNaN(width) && horizontalAlignment != xaml::HorizontalAlignment_Stretch)
    {
        ctl::ComPtr<IFrameworkElement> spItemsPresenterHostAsFE;

        IFC(m_tpItemsPresenterHostPart.As(&spItemsPresenterHostAsFE));

        // We set the host's width to the presenter's desired width only if no explicit width is set and
        // the horizontal alignment isn't stretch (when the horizontal alignment is stretch, the canvas is
        // automatically stretched).
        IFC(spItemsPresenterHostAsFE->put_Width(desiredSize.Width));
    }

    if (hostParentNewSize.Width > desiredSize.Width)
    {
        IFC(spItemsPresenterAsFE->put_Width(hostParentNewSize.Width));
    }

    clipRect.Width = hostParentNewSize.Width;
    clipRect.Height = hostParentNewSize.Height;
    IFC(ctl::make<RectangleGeometry>(&spClipRect));
    IFC(spClipRect->put_Rect(clipRect));

    IFC(m_tpItemsPresenterHostPart.AsOrNull<IUIElement>()->put_Clip(spClipRect.AsOrNull<IRectangleGeometry>().Get()));

    // Keep the entire ComboBox in view while opening
    if (m_doKeepInView)
    {
        BOOLEAN hasFocus = FALSE;
        IFC(HasFocus(&hasFocus));

        if (hasFocus)
        {
            wf::Rect boundsForScroll = {};
            DOUBLE actualHeight;
            IFC(get_ActualHeight(&actualHeight));
            boundsForScroll.Height = static_cast<float>(actualHeight);
            BringIntoView(boundsForScroll, true /*forceIntoView*/, false /*useAnimation*/, true /*skipDuringManipulation*/);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnItemsPresenterSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    ASSERT(m_tpItemsPresenterPart, L"The ItemsPresenter's size changed event cannot occur if the ItemsPresenter part is null.");

    if (IsInline())
    {
        DOUBLE width = DoubleUtil::NaN;
        xaml::HorizontalAlignment horizontalAlignment = xaml::HorizontalAlignment_Stretch;

        IFC(get_Width(&width));
        IFC(get_HorizontalAlignment(&horizontalAlignment));

        if (width != DoubleUtil::NaN && horizontalAlignment != xaml::HorizontalAlignment_Stretch)
        {
            wf::Size presenterDesiredSize = {};

            IFC(m_tpItemsPresenterPart.AsOrNull<IUIElement>()->get_DesiredSize(&presenterDesiredSize));
            IFC(m_tpItemsPresenterHostPart.AsOrNull<IFrameworkElement>()->put_Width(presenterDesiredSize.Width));
        }

        IFC(ForceApplyInlineLayoutUpdate());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::OnBackButtonPressedImpl(
    _Out_ BOOLEAN* pHandled)
{
    HRESULT hr = S_OK;

#if DBG
    BOOLEAN isDropDownOpen = FALSE;
    IFC(get_IsDropDownOpen(&isDropDownOpen));
    ASSERT(isDropDownOpen,
        L"ComboBox should not be registered to receive back button press notifications unless the drop down is open");
#endif

    m_isClosingDueToCancel = true;
    IFC(put_IsDropDownOpen(FALSE));
    *pHandled = TRUE;

Cleanup:
    RRETURN(hr);
}

#pragma endregion Small Form Factor Event Handlers

#pragma region Header Support

_Check_return_ HRESULT ComboBox::UpdateHeaderPresenterVisibility()
{
    ctl::ComPtr<xaml::IDataTemplate> spHeaderTemplate;
    ctl::ComPtr<IInspectable> spHeader;

    IFC_RETURN(get_HeaderTemplate(&spHeaderTemplate));
    IFC_RETURN(get_Header(&spHeader));

    IFC_RETURN(ConditionallyGetTemplatePartAndUpdateVisibility(
        XSTRING_PTR_EPHEMERAL(L"HeaderContentPresenter"),
        (spHeader || spHeaderTemplate),
        m_tpHeaderContentPresenterPart));

    IFC_RETURN(ConditionallyGetTemplatePartAndUpdateVisibility(
        XSTRING_PTR_EPHEMERAL(L"RequiredHeaderPresenter"),
        (spHeader || spHeaderTemplate) && IsValueRequired(this),
        m_requiredHeaderContentPresenterPart));

    return S_OK;
}

// Used in hit-testing for the ComboBox target area, which must exclude the header
_Check_return_ HRESULT ComboBox::IsEventSourceTarget(
    _In_ IRoutedEventArgs* pArgs,
    _Out_ BOOLEAN* pIsEventSourceChildOfTarget)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSourceAsI;
    ctl::ComPtr<IDependencyObject> spOriginalSourceAsDO;
    IFC(pArgs->get_OriginalSource(&spOriginalSourceAsI));
    IFC(spOriginalSourceAsI.As(&spOriginalSourceAsDO));
    IFC(IsChildOfTarget(spOriginalSourceAsDO.Get(), FALSE, TRUE, pIsEventSourceChildOfTarget));

Cleanup:
    RRETURN(hr);
}

// Used to determine if DropDown arrow was hit on Editable mode
bool ComboBox::IsDropDownOverlay(
    _In_ IRoutedEventArgs* pArgs)
{
    if (!m_tpDropDownOverlayPart)
    {
        return false;
    }

    ctl::ComPtr<IInspectable> spOriginalSourceAsI;
    ctl::ComPtr<IDependencyObject> spOriginalSourceAsDO;
    IFCFAILFAST(pArgs->get_OriginalSource(&spOriginalSourceAsI));
    IFCFAILFAST(spOriginalSourceAsI.As(&spOriginalSourceAsDO));

    auto originalSource = spOriginalSourceAsDO.Get();
    ctl::ComPtr<IDependencyObject> templatePart;
    m_tpDropDownOverlayPart.As<IDependencyObject>(&templatePart);

    return (originalSource && templatePart && originalSource == templatePart.Get());
}

// Used in hit-testing for the ComboBox target area, which must exclude the header
_Check_return_ HRESULT ComboBox::IsChildOfTarget(
    _In_opt_ IDependencyObject* pChild,
    _In_ BOOLEAN doSearchLogicalParents,
    _In_ BOOLEAN doCacheResult,
    _Out_ BOOLEAN* pIsChildOfTarget)
{
    // Simple perf optimization: most pointer events have the same source as the previous
    // event, so we'll cache the most recent result and reuse it whenever possible.
    static IDependencyObject* pMostRecentSearchChildNoRef = NULL;
    static BOOLEAN mostRecentResult = FALSE;

    HRESULT hr = S_OK;
    BOOLEAN result = mostRecentResult;
    ctl::ComPtr<IDependencyObject> spHeaderPresenterAsDO;
    ctl::ComPtr<IDependencyObject> spCurrentDO(pChild);
    ctl::ComPtr<IDependencyObject> spParentDO;
    IDependencyObject* pThisAsDONoRef = static_cast<IDependencyObject*>(this);
    BOOLEAN isFound = FALSE;

    IFCPTR(pIsChildOfTarget);

    if (!pChild)
    {
        *pIsChildOfTarget = FALSE;
        goto Cleanup;
    }

    spHeaderPresenterAsDO = m_tpHeaderContentPresenterPart.AsOrNull<IDependencyObject>();

    while (spCurrentDO && !isFound)
    {
        if (spCurrentDO.Get() == pMostRecentSearchChildNoRef)
        {
            // use the cached result
            isFound = TRUE;
        }
        else if (spCurrentDO.Get() == pThisAsDONoRef)
        {
            result = TRUE;
            isFound = TRUE;
        }
        else if (spHeaderPresenterAsDO && spCurrentDO.Get() == spHeaderPresenterAsDO.Get())
        {
            result = FALSE;
            isFound = TRUE;
        }
        else
        {
            ctl::ComPtr<PopupRoot> spPopup;
            IFC(VisualTreeHelper::GetParentStatic(spCurrentDO.Get(), &spParentDO));

            if (doSearchLogicalParents && SUCCEEDED(spParentDO.As(&spPopup)) && spPopup)
            {
                // Try the logical parent. This lets us look through popup boxes
                ctl::ComPtr<IFrameworkElement> spCurrentAsFE = spCurrentDO.AsOrNull<IFrameworkElement>();
                if (spCurrentAsFE)
                {
                    IFC(spCurrentAsFE->get_Parent(&spParentDO));
                }
            }

            // refcounting note: Attach releases the previously stored ptr, and does not
            // addref the new one.
            spCurrentDO.Attach(spParentDO.Detach());
        }
    }

    if (!isFound)
    {
        result = FALSE;
    }

    if (doCacheResult)
    {
        pMostRecentSearchChildNoRef = pChild;
        mostRecentResult = result;
    }

    *pIsChildOfTarget = result;

Cleanup:
    RRETURN(hr);
}

#pragma endregion Header Support

#pragma region Popup Support

// Provides the behavior for the Arrange pass of layout.  Classes can override
// this method to define their own Arrange pass behavior.
IFACEMETHODIMP ComboBox::ArrangeOverride(
    // The computed size that is used to arrange the content.
    _In_ wf::Size finalSize,
    // The size of the control.
    _Out_ wf::Size* pReturnValue)
{
    // Call base ArrangeOverride
    IFC_RETURN(ComboBoxGenerated::ArrangeOverride(finalSize, pReturnValue));
    if (!IsSmallFormFactor())
    {
        // Don't center the selected item for Editable mode.
        IFC_RETURN(ArrangePopup(!IsEditable()));
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::GetItemLayoutHeight(
    _In_ INT index,
    _In_ wf::Size availableSize,
    _Out_ DOUBLE& itemHeight)
{
    ctl::ComPtr<xaml::IDependencyObject> spContainer;
    ctl::ComPtr<xaml::IUIElement> spUIElement;
    wf::Size desiredSize = {};

    ASSERT(!IsSmallFormFactor(), L"GetItemLayoutHeight is not used in small form factor mode");

    itemHeight = 0.0;
    IFC_RETURN(ContainerFromIndex(index, &spContainer));
    IFC_RETURN(spContainer.As(&spUIElement));

    if (spUIElement == nullptr)
    {
        // Get the first child from the carousel panel and then it will also
        // apply the visual state properly with the input mode.
        IFC_RETURN(GetRealizedFirstChildFromCarouselPanel(&spUIElement));
    }

    if (spUIElement)
    {
        xaml::Visibility visibility = xaml::Visibility_Collapsed;

        IFC_RETURN(spUIElement->get_Visibility(&visibility));

        if (visibility == xaml::Visibility_Visible)
        {
            ctl::ComPtr<xaml_controls::IComboBoxItem> spIComboBoxItem;

            spIComboBoxItem = spUIElement.AsOrNull<xaml_controls::IComboBoxItem>();
            if (spIComboBoxItem != nullptr)
            {
                // The ComboBoxItem's UpdateVisualState() must be called to ensure that the item
                // is in the correct visual state when we try to determine its size.
                IFC_RETURN(spIComboBoxItem.Cast<ComboBoxItem>()->ChangeVisualState(FALSE /* bUseTransitions */));
            }

            IFC_RETURN(spUIElement->get_DesiredSize(&desiredSize));

            if (desiredSize.Height == 0)
            {
                // This item seems not measured yet and the result is not accurate.
                IFC_RETURN(spUIElement->Measure({ static_cast<FLOAT>(DoubleUtil::PositiveInfinity), static_cast<FLOAT>(DoubleUtil::PositiveInfinity) }));
                IFC_RETURN(spUIElement->get_DesiredSize(&desiredSize));
            }
        }
    }

    itemHeight = desiredSize.Height;

    return S_OK;
}

_Check_return_ HRESULT ComboBox::UpdateIsPopupPannable(
    _In_ UINT itemCount,
    _In_ DOUBLE maxAllowedPopupHeight,
    _In_ wf::Size availableSize,
    _Inout_ DOUBLE& childHeight)
{
    ASSERT(!IsSmallFormFactor(), L"UpdateIsPopupPannable is not used in small form factor mode");

    m_bIsPopupPannable = false;
    if (itemCount <= 0)
    {
        return S_OK;
    }

    if (itemCount > static_cast<UINT>(m_MaxNumberOfItemsThatCanBeShown))
    {
        m_bIsPopupPannable = true;
        return S_OK;
    }

    maxAllowedPopupHeight = DoubleUtil::Min(maxAllowedPopupHeight, availableSize.Height);
    if (childHeight > 0 && childHeight > maxAllowedPopupHeight)
    {
        m_bIsPopupPannable = true;
        return S_OK;
    }

    DOUBLE totalLayoutSize = 0.0;
    childHeight = 0.0;
    if (itemCount > 0)
    {
        DOUBLE currentItemLayoutSize = 0.0;
        IFC_RETURN(GetItemLayoutHeight(itemCount-1, availableSize, currentItemLayoutSize));
        totalLayoutSize = 2*currentItemLayoutSize; // For nullptr element. Size of nullptr element = Size of first element.
        childHeight -= currentItemLayoutSize;
        for(UINT i = 0; i < itemCount-1; i++)
        {
            IFC_RETURN(GetItemLayoutHeight(i, availableSize, currentItemLayoutSize));
            totalLayoutSize += currentItemLayoutSize;
            if (totalLayoutSize > maxAllowedPopupHeight) // Iterate only until we exceed max allowed size.
            {
                m_bIsPopupPannable = true;
                break;
            }
        }
        childHeight += totalLayoutSize;
    }

    return S_OK;
}

// For non-pannable popup calculate Y coordinate, popupMaxHeight and firstItemIndex.
// X coordinate is same for both pannable and non-pannable
_Check_return_ HRESULT ComboBox::GetNonPannablePopupLayout(
    _In_ INT centerItemIndex,
    _In_ UINT itemCount,
    _In_ DOUBLE cbY,
    _In_ DOUBLE cbHeight,
    _In_ xaml::Thickness cbPopupContentMargin,
    _In_ wf::Size rootWindowSize,
    _Out_ DOUBLE& popupY,
    _Out_ DOUBLE& popupMaxHeight,
    _Out_ DOUBLE& offset)
{
    ASSERT(!IsSmallFormFactor(), L"GetNonPannablePopupLayout is not used in small form factor mode");

    if (static_cast<INT>(itemCount) < centerItemIndex || centerItemIndex < 0) // CenterItem is out of bounds
    {
        centerItemIndex = itemCount/2;
    }

    if (itemCount == 0)
    {
        popupY = cbY;
        popupMaxHeight = cbHeight;
        offset = 0;
        return S_OK;
    }

    // Layout centerItem on faceplate
    DOUBLE currentItemHeight = 0.0;
    IFC_RETURN(GetItemLayoutHeight(centerItemIndex, rootWindowSize, currentItemHeight));

    // If the comboBox button appears outside of the bounds of the Window, try to position the
    // popup as if the box is within the bounds of the window by faking the .Y location.
    if ((cbY + cbHeight) >= rootWindowSize.Height)
    {
        cbY = rootWindowSize.Height - cbHeight;
    }

    DOUBLE calculatedLayoutLocationAbove = cbY + cbHeight/2 - currentItemHeight/2 - cbPopupContentMargin.Top;
    DOUBLE layoutLocationAbove = MAX(calculatedLayoutLocationAbove, 0);
    DOUBLE upperLimit = MAX(cbY + cbHeight/2 - popupMaxHeight/2, 0);
    DOUBLE calculatedLayoutLocationBelow = layoutLocationAbove + currentItemHeight + cbPopupContentMargin.Top + cbPopupContentMargin.Bottom;
    DOUBLE layoutLocationBelow = MIN(calculatedLayoutLocationBelow, rootWindowSize.Height);
    DOUBLE lowerLimit = MIN(upperLimit  + popupMaxHeight, rootWindowSize.Height);
    INT itemIndexAbove = centerItemIndex - 1;
    INT itemIndexBelow = centerItemIndex + 1;
    INT totalItemsLayed = 1;
    INT maxNumberOfItemsAllowedOnOneSide = MIN(m_MaxNumberOfItemsThatCanBeShownOnOneSide, static_cast<INT>(itemCount));
    INT maxNumberOfItemsAllowed = MIN(m_MaxNumberOfItemsThatCanBeShown, static_cast<INT>(itemCount));

    // Compensate the missing amount height between calculatedLayoutLocationBelow and layoutLocationBelow on layoutLocationAbove
    // that ensures both the top and bottom margin is applied properly into the calculating popupMaxHeight.
    if (calculatedLayoutLocationBelow > rootWindowSize.Height)
    {
        layoutLocationAbove = MAX(layoutLocationAbove - calculatedLayoutLocationBelow + rootWindowSize.Height, 0);
    }

    // Layout Items above, itemIndexAbove = centerItemIndex - 1
    //      itemIndexAbove >= 0, layoutLocationAbove - nextItemSize > 0, totalItemsLayed <= m_MaxNumberOfItemsThatCanBeShownOnOneSide
    if (itemIndexAbove >= 0)
    {
        IFC_RETURN(GetItemLayoutHeight(itemIndexAbove, rootWindowSize, currentItemHeight));

        while (itemIndexAbove >= 0
            && layoutLocationAbove - currentItemHeight >= upperLimit
            && totalItemsLayed < maxNumberOfItemsAllowedOnOneSide)
        {
            layoutLocationAbove -= currentItemHeight;
            totalItemsLayed++;
            itemIndexAbove--;
            if (itemIndexAbove >= 0)
            {
                IFC_RETURN(GetItemLayoutHeight(itemIndexAbove, rootWindowSize, currentItemHeight));
            }
        }
    }

    // Layout items below, itemIndexBelow = centerItemIndex + 1
    //      itemIndexBelow < itemCount, layoutLocationBelow + nextItemSize < rootWindowSize.Height, totalItemsLayed <= maxallowed
    if (itemIndexBelow < static_cast<INT>(itemCount))
    {
        IFC_RETURN(GetItemLayoutHeight(itemIndexBelow, rootWindowSize, currentItemHeight));

        while (itemIndexBelow < static_cast<INT>(itemCount)
            && layoutLocationBelow + currentItemHeight < lowerLimit
            && layoutLocationBelow - layoutLocationAbove < popupMaxHeight
            && totalItemsLayed < maxNumberOfItemsAllowed)
        {
            layoutLocationBelow += currentItemHeight;
            totalItemsLayed++;
            itemIndexBelow++;
            if (itemIndexBelow < static_cast<INT>(itemCount))
            {
                IFC_RETURN(GetItemLayoutHeight(itemIndexBelow, rootWindowSize, currentItemHeight));
            }
        }
    }


    // Try filling in more items if there are more items, the items fit, and we didn't put more than the max allowed.
    if (itemIndexAbove >= 0 || itemIndexBelow < static_cast<INT>(itemCount))
    {
        BOOLEAN isAbove = (itemIndexAbove >= 0);
        INT currentItemIndex = isAbove ? itemIndexAbove : itemIndexBelow;
        IFC_RETURN(GetItemLayoutHeight(currentItemIndex, rootWindowSize, currentItemHeight));

        // Make sure the current item can fit below or above the popup.
        while (layoutLocationBelow - layoutLocationAbove + currentItemHeight <= popupMaxHeight
            && (layoutLocationBelow + currentItemHeight < rootWindowSize.Height  || layoutLocationAbove - currentItemHeight >= 0)
            && totalItemsLayed < maxNumberOfItemsAllowed)

        {
            if (isAbove)
            {
                itemIndexAbove--;
            }
            else
            {
                itemIndexBelow++;
            }

            if (layoutLocationAbove - currentItemHeight <= 0)
            {
                layoutLocationBelow += currentItemHeight;
            }
            else
            {
                layoutLocationAbove -= currentItemHeight;
            }

            totalItemsLayed++;
            if (itemIndexAbove >= 0 || itemIndexBelow < static_cast<INT>(itemCount))
            {
                isAbove = (itemIndexAbove >= 0);
                currentItemIndex = isAbove ? itemIndexAbove : itemIndexBelow;
                IFC_RETURN(GetItemLayoutHeight(currentItemIndex, rootWindowSize, currentItemHeight));
            }
        }
    }

    offset = itemIndexAbove+1;
    popupY = layoutLocationAbove;
    popupMaxHeight = layoutLocationBelow - layoutLocationAbove;

    return S_OK;
}

_Check_return_ HRESULT ComboBox::GetEditableComboBoxPopupLayout(
    _In_ uint32_t itemCount,
    _In_ double cbY,
    _In_ double cbHeight,
    _In_ xaml::Thickness cbPopupContentMargin,
    _In_ wf::Size rootWindowSize,
    _Out_ double& popupY,
    _Out_ double& popupMaxHeight)
{
    ASSERT(!IsSmallFormFactor(), L"GetNonPannablePopupLayout is not used in small form factor mode");

    if (m_tpEditableTextPart)
    {
        float fWidth, fHeight;
        static_cast<CTextBoxBase*>(m_tpEditableTextPart.Cast<TextBox>()->GetHandle())->GetActualSize(fWidth, fHeight);
        const double actualHeight = (double)(fHeight);

        cbHeight = std::max(cbHeight, actualHeight);
    }

    if (itemCount == 0)
    {
        popupY = cbY;
        popupMaxHeight = cbHeight;
        return S_OK;
    }

    const double calculatedLayoutLocationAbove = cbY + cbHeight + m_candidateWindowBoundsRect.Height;
    const double layoutLocationAbove = std::max(calculatedLayoutLocationAbove, 0.0);
    double layoutLocationBelow = layoutLocationAbove + cbPopupContentMargin.Top + cbPopupContentMargin.Bottom;
    uint32_t currentIndex = 0;
    uint32_t totalItemsLayed = 0;
    const uint32_t maxNumberOfItemsAllowed = std::min(m_MaxNumberOfItemsThatCanBeShown, (int)itemCount);

    while (currentIndex < itemCount && totalItemsLayed < maxNumberOfItemsAllowed)
    {
        double currentItemHeight = 0;
        IFC_RETURN(GetItemLayoutHeight(currentIndex, rootWindowSize, currentItemHeight));

        layoutLocationBelow += currentItemHeight;
        totalItemsLayed++;
        currentIndex++;
    }

    popupY = layoutLocationAbove;
    popupMaxHeight = layoutLocationBelow - layoutLocationAbove;

    m_openedUp = false;

    // If the popup overflows the available window height see if we can open it above.
    if (popupY + popupMaxHeight > rootWindowSize.Height)
    {
        if (cbY - popupMaxHeight >= 0)
        {
            // Move popup above the ComboBox.
            popupY = std::max(cbY - popupMaxHeight, 0.0);
            m_openedUp = true;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::GetPannablePopupLayout(
    _In_ INT centerItemIndex,
    _In_ UINT itemCount,
    _In_ DOUBLE cbY,
    _In_ DOUBLE cbHeight,
    _In_ DOUBLE childHeight,
    _In_ wf::Size rootWindowSize,
    _Out_ DOUBLE& popupY,
    _Inout_ DOUBLE& popupMaxHeight,
    _Out_ DOUBLE& offset)
{
    ASSERT(!IsSmallFormFactor(), L"GetPannablePopupLayout is not used in small form factor mode");

    if (static_cast<INT>(itemCount) < centerItemIndex || centerItemIndex < 0) // CenterItem is out of bounds
    {
        centerItemIndex = itemCount/2;
    }

    DOUBLE popupSize = 0.0;
    IFC_RETURN(GetItemLayoutHeight(centerItemIndex, rootWindowSize, popupSize));

    DOUBLE roomAvailableAbove = MIN((popupMaxHeight - cbHeight)/2, cbY);
    DOUBLE roomAvailableBelow = MIN(popupMaxHeight - roomAvailableAbove - cbHeight, MAX(0, rootWindowSize.Height - cbY - popupSize));

    INT maxItemsAllowedAbove = static_cast<INT>(DoubleUtil::Min(m_MaxNumberOfItemsThatCanBeShownOnOneSide,  (itemCount-1)/2));
    INT maxItemsAllowedBelow = static_cast<INT>(DoubleUtil::Min(m_MaxNumberOfItemsThatCanBeShownOnOneSide, (itemCount-1)/2));

    INT itemsAddedAbove = 0;
    DOUBLE nextItemHeight = 0.0;
    INT nextItemIndex = (centerItemIndex-1 >= 0) ? centerItemIndex-1 : itemCount-1;
    if (nextItemIndex >= 0)
    {
        IFC_RETURN(GetItemLayoutHeight(nextItemIndex, rootWindowSize, nextItemHeight));
    }

    // Ensures the current Popup Y position isn't out of the available window's vertical area.
    popupY = MAX(MIN(cbY, rootWindowSize.Height - popupSize), 0);

    while(popupSize+nextItemHeight <= popupMaxHeight
        && itemsAddedAbove < maxItemsAllowedAbove
        && roomAvailableAbove - nextItemHeight > 0)
    {
        itemsAddedAbove++;
        popupSize += nextItemHeight;
        roomAvailableAbove -= nextItemHeight;
        popupY -= nextItemHeight;
        nextItemIndex = (nextItemIndex-1 >= 0) ? nextItemIndex-1 : itemCount-1;
        if (nextItemIndex >= 0)
        {
            IFC_RETURN(GetItemLayoutHeight(nextItemIndex, rootWindowSize, nextItemHeight));
        }
    }

    offset = centerItemIndex - itemsAddedAbove;

    INT itemsAddedBelow = 0;
    nextItemHeight = 0.0;
    nextItemIndex = (centerItemIndex+1 < static_cast<INT>(itemCount)) ? centerItemIndex+1 : 0;
    if (nextItemIndex < static_cast<INT>(itemCount))
    {
        IFC_RETURN(GetItemLayoutHeight(nextItemIndex, rootWindowSize, nextItemHeight));
    }

    while(popupSize+nextItemHeight <= popupMaxHeight
        && itemsAddedBelow < maxItemsAllowedBelow
        && roomAvailableBelow - nextItemHeight > 0)
    {
        itemsAddedBelow++;
        popupSize += nextItemHeight;
        roomAvailableBelow -= nextItemHeight;
        nextItemIndex = (nextItemIndex+1 < static_cast<INT>(itemCount)) ? nextItemIndex+1 : 0;
        if (nextItemIndex < static_cast<INT>(itemCount))
        {
            IFC_RETURN(GetItemLayoutHeight(nextItemIndex, rootWindowSize, nextItemHeight));
        }
    }

    // Adjust values for cutoff items
    if (roomAvailableAbove >= nextItemHeight / 2)
    {
        popupSize += nextItemHeight / 2;
        popupY -= nextItemHeight / 2;
        offset -= 0.50;
    }

    popupMaxHeight = DoubleUtil::Min(popupMaxHeight, popupSize);

    // Wrap first Item Index
    while (offset < 0)
    {
        offset += itemCount + 1;
    }

    while (offset >= static_cast<DOUBLE>(itemCount)+1)
    {
        offset -= (itemCount + 1);
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::TransformComboBoxToVisual(
    _Outptr_ xaml_media::IGeneralTransform** ppTransform)
{
    HRESULT hr = S_OK;

    if (m_tpBackgroundPart)
    {
        IFC(m_tpBackgroundPart.Cast<Border>()->TransformToVisual(nullptr, ppTransform));
    }
    else
    {
        IFC(TransformToVisual(nullptr, ppTransform));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::SetIsPopupPannable()
{
    HRESULT hr = S_OK;
    wf::Point p00 = { 0, 0 };
    wf::Point p10 = { 1, 0 };
    wf::Point p01 = { 0, 1 };
    wf::Point p00t = {};
    wf::Point p10t = {};
    wf::Point p01t = {};
    ctl::ComPtr<xaml_media::IGeneralTransform> spGt;
    UINT itemCount = 0;
    DOUBLE popupMaxHeight= 0.0;
    wf::Rect rootRect;
    wf::Size rootWindowSize;
    wf::Size scaledRootWindowSize;
    XFLOAT zoom = 1.0;
    DOUBLE childHeight = 0.0;

    ASSERT(!IsSmallFormFactor(), L"SetIsPopupPannable is not used in small form factor mode");

    if (!m_tpPopupPart || !m_tpElementPopupChild || !m_tpContentPresenterPart || !m_tpElementOutsidePopup)
    {
        goto Cleanup;
    }

    m_bIsPopupPannable = false;

    IFC(GetItemCount(itemCount));

    IFC(GetVisibleBoundsInternal(&rootRect));
    rootWindowSize.Width = rootRect.Width;
    rootWindowSize.Height = rootRect.Height;

    IFC(TransformComboBoxToVisual(&spGt));

    if (!spGt)
    {
        goto Cleanup;
    }

    IFC(spGt->TransformPoint(p00, &p00t));
    IFC(spGt->TransformPoint(p10, &p10t));
    IFC(spGt->TransformPoint(p01, &p01t));

    xaml_media::Matrix transformToRootMatrix;
    transformToRootMatrix.M11 = p10t.X - p00t.X;
    transformToRootMatrix.M12 = p10t.Y - p00t.Y;
    transformToRootMatrix.M21 = p01t.X - p00t.X;
    transformToRootMatrix.M22 = p01t.Y - p00t.Y;
    // Don't need to set offsets X & Y. They don't affect the scale dimensions.

    XFLOAT scaleX;
    XFLOAT scaleY;
    DirectUI::MatrixHelper::GetScaleDimensions(transformToRootMatrix, &scaleX, &scaleY);

    scaledRootWindowSize.Width = rootWindowSize.Width / scaleX;
    scaledRootWindowSize.Height = rootWindowSize.Height / scaleY;


    IFC(get_MaxDropDownHeight(&popupMaxHeight));
    if (DoubleUtil::IsInfinity(popupMaxHeight) || DoubleUtil::IsNaN(popupMaxHeight))
    {
        popupMaxHeight = scaledRootWindowSize.Height;
    }

    IFC(m_tpElementPopupChild.Get()->get_ActualHeight(&childHeight));
    childHeight *= static_cast<DOUBLE>(zoom);

    IFC(UpdateIsPopupPannable(itemCount, popupMaxHeight, scaledRootWindowSize, childHeight));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::ArrangePopup(
    _In_ bool bCenterSelectedItem) noexcept
{
    wf::Point p00 = { 0, 0 };
    wf::Point p10 = { 1, 0 };
    wf::Point p01 = { 0, 1 };
    wf::Point p00t = {};
    wf::Point p10t = {};
    wf::Point p01t = {};
    wf::Point p00local = { 0, 0 };
    ctl::ComPtr<CanvasFactory> spFactory;
    ctl::ComPtr<xaml_media::IGeneralTransform> spGt;
    ctl::ComPtr<IComboBoxTemplateSettings> spTemplateSettings;
    DOUBLE offset = 0;

    ASSERT(!IsSmallFormFactor(), L"ArrangePopup is not used in small form factor mode");

    bool isEditable = IsEditable();

    // If we get request to Arrange popup with bCenterSelectedItem being set
    // we won't center the item if drop down is closed (see protection bellow).
    m_bShouldCenterSelectedItem |= bCenterSelectedItem;

    if (!m_tpPopupPart || !m_tpElementPopupChild || !m_tpElementPopupContent || !m_tpContentPresenterPart || !m_tpElementOutsidePopup)
    {
        return S_OK;
    }

    BOOLEAN bIsDropDownOpen = FALSE;
    IFC_RETURN(get_IsDropDownOpen(&bIsDropDownOpen));
    if (!bIsDropDownOpen)
    {
        return S_OK;
    }

    IFC_RETURN(TransformComboBoxToVisual(&spGt));

    if (!spGt)
    {
        return S_OK;
    }

    //
    // Take the origin (0, 0), the X axis [1, 0], and the Y axis [0, 1] through the transformer. The transformed origin
    // shows the translation applied by the tranformer. The transformed X and Y axes will contain the scale and rotation.
    //
    // Note: If there's a projection somewhere between the ComboBox and the root, then the transform can't be captured
    // by just a 2D matrix. In that case, we approximate it by the local scale applied near (0, 0).
    //
    IFC_RETURN(spGt->TransformPoint(p00, &p00t));
    IFC_RETURN(spGt->TransformPoint(p10, &p10t));
    IFC_RETURN(spGt->TransformPoint(p01, &p01t));

    // Get current window size
    wf::Rect rootRect;

    if (m_tpPopupPart.Cast<Popup>()->IsWindowed())
    {
        IFC_RETURN(DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(m_tpElementPopupChild.Cast<FrameworkElement>(), p00t, &rootRect));
    }
    else
    {
        IFC_RETURN(GetVisibleBoundsInternal(&rootRect));
    }

    wf::Size rootWindowSize;

    rootWindowSize.Width = rootRect.Width;
    rootWindowSize.Height = rootRect.Height;
    if (rootWindowSize.Height == 0 || rootWindowSize.Width == 0)
    {
        return S_OK;
    }

    DOUBLE cbX = p00t.X - rootRect.X;
    DOUBLE cbY = p00t.Y - rootRect.Y;
    DOUBLE zoomFactor = 1.0;

    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;
    IFC_RETURN(get_FlowDirection(&flowDirection));

    //
    // Construct transformToRootMatrix such that it transforms the origin, X axis, and Y axis the same way that the
    // transformer does. This means putting the transformed X axis in the first row, the transformed Y axis in the
    // second row, and the transformed origin as the offset.
    //
    xaml_media::Matrix transformToRootMatrix;
    transformToRootMatrix.M11 = p10t.X - p00t.X;
    transformToRootMatrix.M12 = p10t.Y - p00t.Y;
    transformToRootMatrix.M21 = p01t.X - p00t.X;
    transformToRootMatrix.M22 = p01t.Y - p00t.Y;
    transformToRootMatrix.OffsetX = p00t.X;
    transformToRootMatrix.OffsetY = p00t.Y;

    IFC_RETURN(HandleRotateTransformOnParentForPopupPlacement(transformToRootMatrix, flowDirection, cbX, cbY, rootWindowSize));

    FLOAT scaleX;
    FLOAT scaleY;
    DirectUI::MatrixHelper::GetScaleDimensions(transformToRootMatrix, &scaleX, &scaleY);

    wf::Size scaledRootWindowSize;

    scaledRootWindowSize.Width = rootWindowSize.Width / scaleX;
    scaledRootWindowSize.Height = rootWindowSize.Height / scaleY;

    DOUBLE actualHeight= 0.0;
    DOUBLE actualWidth= 0.0;

    if (m_tpBackgroundPart)
    {
        IFC_RETURN(m_tpBackgroundPart.Cast<Border>()->get_ActualHeight(&actualHeight));
        IFC_RETURN(m_tpBackgroundPart.Cast<Border>()->get_ActualWidth(&actualWidth));
    }
    else
    {
        IFC_RETURN(get_ActualHeight(&actualHeight));
        IFC_RETURN(get_ActualWidth(&actualWidth));
    }

    DOUBLE cbHeight = actualHeight * zoomFactor;
    DOUBLE cbWidth = actualWidth * zoomFactor;

    DOUBLE scaledCBX = cbX / static_cast<DOUBLE>(scaleX);
    DOUBLE scaledCBY = cbY / static_cast<DOUBLE>(scaleY);

    if (cbHeight == 0 || cbWidth == 0)
        return S_OK;

    DOUBLE childWidth = 0.0;
    DOUBLE childHeight = 0.0;
    IFC_RETURN(m_tpElementPopupChild.Get()->get_ActualWidth(&childWidth));
    IFC_RETURN(m_tpElementPopupChild.Get()->get_ActualHeight(&childHeight));

    // In ActualBounds mode, the bound size is determined by ActualWidth&ActualHeight of popup child, so we need to explicitly set width&height on canvas here.
    // If popup border has negative margins, content will be pushed out of HWND bound, so we need to increase the bound size to include the out of scope content.
    // We don't really need to worry about positive margins since they will be completely transparent and safe to clip.
    xaml::Thickness borderMargin = { 0, 0, 0, 0 };
    m_tpElementPopupChild.Get()->get_Margin(&borderMargin);

    auto canvasWidth = childWidth;
    auto canvasHeight = childHeight;
    canvasWidth -= DoubleUtil::Min(0, borderMargin.Left + borderMargin.Right);
    canvasHeight -= DoubleUtil::Min(0, borderMargin.Top + borderMargin.Bottom);

    auto popupChildCanvasAsFE = m_tpElementPopupChildCanvas.AsOrNull<IFrameworkElement>();
    IFC_RETURN(popupChildCanvasAsFE.Get()->put_Width(canvasWidth));
    IFC_RETURN(popupChildCanvasAsFE.Get()->put_Height(canvasHeight));

    childWidth *= zoomFactor;
    childHeight *= zoomFactor;

    DOUBLE popupMaxHeight= 0.0;
    IFC_RETURN(get_MaxDropDownHeight(&popupMaxHeight));
    if (DoubleUtil::IsInfinity(popupMaxHeight) || DoubleUtil::IsNaN(popupMaxHeight))
    {
        popupMaxHeight = scaledRootWindowSize.Height;
    }

    childWidth = DoubleUtil::Min(childWidth, scaledRootWindowSize.Width);
    childHeight = DoubleUtil::Min(childHeight, popupMaxHeight);
    childWidth = DoubleUtil::Max(cbWidth, childWidth);

    // We prefer to align the popup box with the left edge of the combobox.  If it will fit.
    DOUBLE popupX = 0.0;

    // Popup X position must be within the available visible bounds area
    if (p00t.X < rootRect.X)
    {
        popupX = rootRect.X - p00t.X;
    }

    if (flowDirection == xaml::FlowDirection_LeftToRight)
    {
        if (scaledRootWindowSize.Width < scaledCBX + childWidth)
        {
            // Since it doesn't fit when strictly left aligned, we shift it to the left until it does fit.
            popupX = scaledRootWindowSize.Width - (childWidth + scaledCBX);
        }
    }
    else
    {
        if (0 > scaledCBX - childWidth)
        {
            popupX = scaledCBX - childWidth;
        }
    }

    UINT itemCount = 0;
    IFC_RETURN(GetItemCount(itemCount));
    INT selectedIndex = -1;
    IFC_RETURN(get_SelectedIndex(&selectedIndex));

    DOUBLE popupY = 0.0;
    IFC_RETURN(SetIsPopupPannable()); // update the value of m_bIsPopupPannable as appropriate
    if (m_bIsPopupPannable && m_inputDeviceTypeUsedToOpen == DirectUI::InputDeviceType::Touch)
    {
        // Set ComboBox Content Margin to the 0px in the Touch+Carousel case.
        xaml::Thickness popupContentMarginZero = { 0, 0, 0, 0 };
        IFC_RETURN(m_tpElementPopupContent.Get()->put_Margin(popupContentMarginZero));

        if (isEditable)
        {
            IFC_RETURN(GetEditableComboBoxPopupLayout(itemCount, scaledCBY, cbHeight, popupContentMargin, scaledRootWindowSize, popupY, popupMaxHeight));
        }
        else
        {
            // Get popupY and height
            IFC_RETURN(GetPannablePopupLayout(selectedIndex, itemCount, scaledCBY, cbHeight, childHeight, scaledRootWindowSize, popupY, popupMaxHeight, offset));
        }
    }
    else
    {
        // Set ComboBox Content Margin to the value defined in the template which is 7px by default.
        IFC_RETURN(m_tpElementPopupContent.Get()->put_Margin(popupContentMargin));

        if (isEditable)
        {
            IFC_RETURN(GetEditableComboBoxPopupLayout(itemCount, scaledCBY, cbHeight, popupContentMargin, scaledRootWindowSize, popupY, popupMaxHeight));
        }
        else
        {
            // Get popupY. We already have calculated that popup will fit in available space.
            IFC_RETURN(GetNonPannablePopupLayout(selectedIndex, itemCount, scaledCBY, cbHeight, popupContentMargin, scaledRootWindowSize, popupY, popupMaxHeight, offset));
        }
    }

    IFC_RETURN(m_tpElementOutsidePopup.Cast<Canvas>()->put_Width(rootWindowSize.Width));
    IFC_RETURN(m_tpElementOutsidePopup.Cast<Canvas>()->put_Height(rootWindowSize.Height));

    DOUBLE minWidth = cbWidth / zoomFactor;

    IFC_RETURN(m_tpElementPopupChild.Get()->put_MinWidth(minWidth));
    IFC_RETURN(m_tpElementPopupChild.Get()->put_MaxWidth(DoubleUtil::Max(minWidth, scaledRootWindowSize.Width / zoomFactor)));

    IFC_RETURN(m_tpElementPopupChild.Get()->put_MinHeight(cbHeight / zoomFactor));

    DOUBLE margin = 0.0;
    IFC_RETURN(GetMeasureDeltaForVisualsBetweenPopupAndCarouselPanelFromCarouselPanel(margin));

    if (m_bIsPopupPannable && m_inputDeviceTypeUsedToOpen == DirectUI::InputDeviceType::Touch)
    {
        popupMaxHeight += margin;
    }
    else
    {
        // Do not apply the popup content margin top and bottom again that is already
        // calculated in above GetNonPannablePopupLayout() method
        popupMaxHeight += (margin - (popupContentMargin.Top + popupContentMargin.Bottom));
    }

    // Ensures the popup isn't overflow to the available window height
    // Don't do this check for Editable mode, if popup doesn't fit here we already tried to open it
    // above the ComboBox, and we don't want to occlude the TextBox.
    if (popupY + popupMaxHeight > scaledRootWindowSize.Height)
    {
        // Move Popup Y position not to overflow to the bottom of the available window height
        popupY = DoubleUtil::Max(popupY - (popupY + popupMaxHeight - scaledRootWindowSize.Height), 0);
    }

    IFC_RETURN(m_tpElementPopupChild.Get()->put_MaxHeight(DoubleUtil::Max(0, popupMaxHeight/ zoomFactor)));

    IFC_RETURN(m_tpElementPopupChild.Get()->put_HorizontalAlignment(xaml::HorizontalAlignment::HorizontalAlignment_Left));
    IFC_RETURN(m_tpElementPopupChild.Get()->put_VerticalAlignment(xaml::VerticalAlignment::VerticalAlignment_Top));

    BOOLEAN roundPopupOffsets = FALSE;
    XFLOAT popupTop = 0.0f;
    XFLOAT popupLeft = 0.0f;

    if (m_tpBackgroundPart)
    {
        ctl::ComPtr<xaml_media::IGeneralTransform> spLocalGt;

        IFC_RETURN(m_tpBackgroundPart.Cast<Border>()->TransformToVisual(this, &spLocalGt));

        if (spLocalGt)
        {
            IFC_RETURN(spLocalGt->TransformPoint(p00, &p00local));
        }
    }

    // set the top left corner for combobox's items list
    IFC_RETURN(ctl::make(&spFactory));
    popupLeft = static_cast<XFLOAT>(popupX / zoomFactor + p00local.X);
    popupTop = static_cast<XFLOAT>((popupY - scaledCBY) / zoomFactor + p00local.Y);

    // If layout rounding is enabled, snap popup's child's offsets to device pixels, or they will render at sub pixel boundaries.
    IFC_RETURN(get_UseLayoutRounding(&roundPopupOffsets));
    if (roundPopupOffsets)
    {
        IFC_RETURN(LayoutRound(popupTop, &popupTop));
        IFC_RETURN(LayoutRound(popupLeft, &popupLeft));
    }

    // In ActualBounds mode, the position of the HWND does not include Popup.Child.Margin, move margin values to popup's offset properties instead.
    DOUBLE verticalOffset = popupTop + borderMargin.Top;
    DOUBLE horizontalOffset = popupLeft + borderMargin.Left;
    IFC_RETURN(m_tpPopupPart.Get()->put_VerticalOffset(verticalOffset));
    IFC_RETURN(m_tpPopupPart.Get()->put_HorizontalOffset(horizontalOffset));

    //
    // Transform the invisible canvas to the coordinate space origin.
    // -horizontal/verticalOffset gets us to the ComboBox's origin;
    // -p00t.X/Y then gets us to the top-left corner of the app window.
    //
    ctl::ComPtr<TranslateTransform> translateTransform;

    IFC_RETURN(ctl::make(&translateTransform));
    IFC_RETURN(translateTransform->put_X(-horizontalOffset - p00t.X));
    IFC_RETURN(translateTransform->put_Y(-verticalOffset - p00t.Y));
    IFC_RETURN(m_tpElementOutsidePopup.Cast<Canvas>()->put_RenderTransform(translateTransform.Get()));

    // PopupBorder has a negative {0,-1,0,-1} margin, that will push the content out of the HWND bound.
    // We do an extra check to set opposite values on canvas margin to compensate the negatives.
    if (borderMargin.Top < 0)
    {
        IFC_RETURN(spFactory->SetTop(m_tpElementPopupChild.Cast<Canvas>(), -borderMargin.Top));
    }
    if (borderMargin.Left < 0)
    {
        IFC_RETURN(spFactory->SetLeft(m_tpElementPopupChild.Cast<Canvas>(), -borderMargin.Left));
    }

    // set templatesettings to communicate to animations
    popupMaxHeight = DoubleUtil::Min(popupMaxHeight, childHeight);
    IFC_RETURN(get_TemplateSettings(&spTemplateSettings));
    ComboBoxTemplateSettings* pTemplateSettingsConcrete = spTemplateSettings.Cast<ComboBoxTemplateSettings>();
    if (spTemplateSettings && popupMaxHeight > 0 && cbHeight > 0)
    {
        IFC_RETURN(UpdateTemplateSettings());

        DOUBLE TopToCenter = (scaledCBY + cbHeight / 2) - popupY;
        DOUBLE DifferenceFromCenter = TopToCenter - (popupMaxHeight / 2);

        IFC_RETURN(pTemplateSettingsConcrete->put_DropDownOpenedHeight(popupMaxHeight));
        IFC_RETURN(pTemplateSettingsConcrete->put_DropDownClosedHeight(cbHeight));
        IFC_RETURN(pTemplateSettingsConcrete->put_DropDownOffset( DifferenceFromCenter ));
        IFC_RETURN(pTemplateSettingsConcrete->put_SelectedItemDirection(DifferenceFromCenter > 0 ? xaml_primitives::AnimationDirection_Bottom : xaml_primitives::AnimationDirection_Top));

        // In case of the first ArrangePopup() by changing the input device type,
        // the item height isn't ready yet by applying the different visual style on the each item according to the input device type.
        // UpdateVisualState() will be skipped not to call SplitOpenThemeAnimation here that can apply the incorrect OpenedLength/ClosedLength
        // by having the incorrect items height. But UpdateVisualState() will be applied the next coming ArrangePopup().
        if (m_previousInputDeviceTypeUsedToOpen != DirectUI::InputDeviceType::None &&
            m_previousInputDeviceTypeUsedToOpen != m_inputDeviceTypeUsedToOpen)
        {
            m_bPopupHasBeenArrangedOnce = false;
        }

        // In the case where we are first opening the popup, it renders with initial height CarouselPanel::m_InitialMeasureHeight
        // (see OnIsDropDownOpenChanged()).  In the case where its max height ends up being greater than this value, the
        // SplitOpenThemeAnimation would get called with the wrong OpenedLength/ClosedLength values in this first ArrangePopup() pass.
        // Luckily, we can rely on the fact that ArrangePopup() will be called again in OnElementPopupChildSizeChanged()
        // (if not before that as a second call to ArrangeOverride() as part of the current layout pass), and
        // go to the "Opened" state once we have the correct values set on our ComboBoxTemplateSettings.
        if (m_bPopupHasBeenArrangedOnce)
        {
            IFC_RETURN(UpdateVisualState(TRUE));
        }
        else
        {
            m_bPopupHasBeenArrangedOnce = true;
        }
    }

    // Set the vertical offset to the centered position for  the selected item before SetFocusedItem().
    // SetFocusedItem() shouldn't call for the bring into view if the selected item is
    // already center position with the proper offset setting. Otherwise, there will be a
    // layout cycling issue between reset the offset between SetVerticalOffsetOnCarouselPanel()
    // and SetFocusedItem().
    if (m_bShouldCenterSelectedItem && m_bIsPopupPannable)
    {
        IFC_RETURN(SetVerticalOffsetOnCarouselPanel(offset));

        // Do not scroll into view since SetVerticalOffsetOnCarouselPanel(offset) is already
        // set the proper offset directly
        m_bShouldCenterSelectedItem = false;
    }

    m_bShouldCenterSelectedItem = false;

    return S_OK;
}

// If an ancestor of the ComboBox has a RotateTransform of 90, 180 or 270 degrees, we update the computed values for the offset
// and the window size that are used by the popup placement logic in ArrangePopup.
// We want to ensure that these values are correct relative to the orientation of the ComboBox. For example, if the ComboBox is
// rotated 90 degress, we switch the width and height of the window size.
_Check_return_ HRESULT ComboBox::HandleRotateTransformOnParentForPopupPlacement(
    _In_ xaml_media::Matrix transformToRootMatrix,
    _In_ xaml::FlowDirection flowDirection,
    _Inout_ DOUBLE& comboBoxOffsetX,
    _Inout_ DOUBLE& comboBoxOffsetY,
    _Inout_ wf::Size& rootWindowSize)
{
    // We want to be able to handle the case where an ancestor of the ComboBox has rotation set as a RenderTransform.
    // An app might have applied a RotateTransform to their UI to handle changes in the device orientation. Although
    // the platform provides support for handling device orientation, an app might choose to opt-out of this and perform
    // the appropriate transforms itself. For this reason it is important for ComboBox to be able to support rotations
    // of multiples of 90 degrees.
    // We are scoping the fix to only handle rotations of multiples of 90 degrees. We do not handle rotations in the general
    // case or rotations combined with other transforms such as scale transforms.

    // We want to determine if the transform on the ComboBox is a rotation of 90, 180 or 270. We ignore the offset.
    CMILMatrix transform = DirectUI::MatrixHelper::ToCMILMatrix(transformToRootMatrix);
    transform.SetDx(0);
    transform.SetDy(0);

    // Construct the three rotation matrices that we are going to compare the transform against:
    CMILMatrix rotate90;
    CMILMatrix rotate180;
    CMILMatrix rotate270;

    XPOINTF zeroOffset = {};
    CTransform::BuildRotateMatrix(&rotate90, 90.0f, zeroOffset);
    CTransform::BuildRotateMatrix(&rotate180, 180.0f, zeroOffset);
    CTransform::BuildRotateMatrix(&rotate270, 270.0f, zeroOffset);

    if (flowDirection == xaml::FlowDirection_RightToLeft)
    {
        CMILMatrix flipTransform(true /*initialize*/);
        flipTransform.Scale(-1, 1);

        rotate90.Prepend(flipTransform);
        rotate180.Prepend(flipTransform);
        rotate270.Prepend(flipTransform);
    }

    if (transform == rotate90)
    {
        DOUBLE oldX = comboBoxOffsetX;

        comboBoxOffsetX = comboBoxOffsetY;
        comboBoxOffsetY = rootWindowSize.Width - oldX;
        std::swap(rootWindowSize.Width, rootWindowSize.Height);
    }
    else if (transform == rotate180)
    {
        comboBoxOffsetX = rootWindowSize.Width - comboBoxOffsetX;
        comboBoxOffsetY = rootWindowSize.Height - comboBoxOffsetY;
    }
    else if (transform == rotate270)
    {
        DOUBLE oldX = comboBoxOffsetX;

        comboBoxOffsetX = rootWindowSize.Height - comboBoxOffsetY;
        comboBoxOffsetY = oldX;
        std::swap(rootWindowSize.Width, rootWindowSize.Height);
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::SetVerticalOffsetOnCarouselPanel(
    _In_ DOUBLE offset)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IPanel> spPanel;
    ctl::ComPtr<xaml_primitives::ICarouselPanel> spCarouselPanel;
    ASSERT(!IsSmallFormFactor(), L"SetVerticalOffsetOnCarouselPanel is not used in small form factor mode");

    IFC(get_ItemsHost(&spPanel));

    if (spPanel)
    {
        spCarouselPanel= spPanel.AsOrNull<xaml_primitives::ICarouselPanel>();
        if (spCarouselPanel)
        {
            IFC(spCarouselPanel.Cast<CarouselPanel>()->SetVerticalOffset(offset));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::ResetCarouselPanelState()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IPanel> spPanel;
    ctl::ComPtr<xaml_primitives::ICarouselPanel> spCarouselPanel;
    ASSERT(!IsSmallFormFactor(), L"ResetCarouselPanelState is not used in small form factor mode");

    IFC_RETURN(get_ItemsHost(&spPanel));

    if (spPanel)
    {
        spCarouselPanel= spPanel.AsOrNull<xaml_primitives::ICarouselPanel>();
        if (spCarouselPanel)
        {
            spCarouselPanel.Cast<CarouselPanel>()->ResetOffsetLoop();
            spCarouselPanel.Cast<CarouselPanel>()->ResetMinimumDesiredWindowWidth();
        }
    }

    return hr;
}

_Check_return_ HRESULT ComboBox::GetMeasureDeltaForVisualsBetweenPopupAndCarouselPanelFromCarouselPanel(
    _Out_ DOUBLE& delta)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IPanel> spPanel;
    ctl::ComPtr<xaml_primitives::ICarouselPanel> spCarouselPanel;
    ASSERT(!IsSmallFormFactor(), L"GetMeasureDeltaForVisualsBetweenPopupAndCarouselPanelFromCarouselPanel is not used in small form factor mode");

    IFC(get_ItemsHost(&spPanel));
    delta = 0.0;
    if (spPanel)
    {
        spCarouselPanel = spPanel.AsOrNull<xaml_primitives::ICarouselPanel>();
        if (spCarouselPanel)
        {
            delta = spCarouselPanel.Cast<CarouselPanel>()->GetMeasureDeltaForVisualsBetweenPopupAndCarouselPanel();
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::GetRealizedFirstChildFromCarouselPanel(
    _Outptr_ xaml::IUIElement**  ppRealizedFirstChild)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IPanel> spPanel;
    ctl::ComPtr<xaml_primitives::ICarouselPanel> spCarouselPanel;
    ASSERT(!IsSmallFormFactor(), L"GetRealizedFirstChild is not used in small form factor mode");

    *ppRealizedFirstChild = nullptr;

    IFC(get_ItemsHost(&spPanel));

    if (spPanel)
    {
        spCarouselPanel = spPanel.AsOrNull<xaml_primitives::ICarouselPanel>();
        if (spCarouselPanel)
        {
            IFC(spCarouselPanel.Cast<CarouselPanel>()->GetRealizedFirstChild(ppRealizedFirstChild));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::InvertMatrix(
    _In_ xaml_media::Matrix* pMatrix)
{
    HRESULT hr = S_OK;
    DOUBLE m11 = pMatrix->M11;
    DOUBLE m12 = pMatrix->M12;
    DOUBLE m21 = pMatrix->M21;
    DOUBLE m22 = pMatrix->M22;
    DOUBLE offSetY = pMatrix->OffsetY;
    DOUBLE offSetX = pMatrix->OffsetX;
    DOUBLE determinant = m11 * m22 - m12 * m21;

    ASSERT(!IsSmallFormFactor(), L"InvertMatrix is not used in small form factor mode");

    if(determinant == 0.0)
        goto Cleanup;

    pMatrix->M11 = m22 / determinant;
    pMatrix->M12 = -1 * m12 / determinant;
    pMatrix->M21 = -1 * m21 / determinant;
    pMatrix->M22 = m11 / determinant;
    pMatrix->OffsetX = (offSetY * m21 - offSetX * m22) / determinant;
    pMatrix->OffsetY = (offSetX * m12 - offSetY * m11) / determinant;
Cleanup:
    RRETURN(hr);

}

_Check_return_ HRESULT ComboBox::SetClosingAnimationDirection()
{
    HRESULT hr = S_OK;
    INT selectedItemIndex = -1;
    ctl::ComPtr<IComboBoxTemplateSettings> spTemplateSettings;
    ctl::ComPtr<IDependencyObject> spContainerAsDO;
    ctl::ComPtr<xaml::IUIElement> spSelectedItem;
    wf::Point cbLayoutPosition = {};
    wf::Point selectedItemLayoutPosition = {};

    ASSERT(!IsSmallFormFactor(), L"SetClosingAnimationDirection is not used in small form factor mode");

    if (!m_tpContentPresenterPart)
        goto Cleanup;

    IFC(GetLayoutPosition(m_tpContentPresenterPart.Cast<ContentPresenter>(), cbLayoutPosition));
    IFC(get_SelectedIndex(&selectedItemIndex));
    if (selectedItemIndex == -1)
        goto Cleanup;

    IFC(ContainerFromIndex(selectedItemIndex, &spContainerAsDO));
    IFC(spContainerAsDO.As(&spSelectedItem));
    if (spSelectedItem)
    {
        IFC(GetLayoutPosition(spSelectedItem.Get(), selectedItemLayoutPosition));
        IFC(get_TemplateSettings(&spTemplateSettings));
        ComboBoxTemplateSettings* pTemplateSettingsConcrete = spTemplateSettings.Cast<ComboBoxTemplateSettings>();
        if (pTemplateSettingsConcrete)
        {
            IFC(pTemplateSettingsConcrete->put_SelectedItemDirection(cbLayoutPosition.Y < selectedItemLayoutPosition.Y ? xaml_primitives::AnimationDirection_Top : xaml_primitives::AnimationDirection_Bottom));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::GetLayoutPosition(
    _In_ xaml::IUIElement* pUIElement,
    _Out_ wf::Point& layoutPosition)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_media::IGeneralTransform> spGt;
    wf::Point p00 = {0,0};

    ASSERT(!IsSmallFormFactor(), L"GetLayoutPosition is not used in small form factor mode");

    IFCPTR(pUIElement);

    if (!static_cast<UIElement*>(pUIElement)->IsInLiveTree())
        goto Cleanup;

    IFC(pUIElement->TransformToVisual(nullptr, &spGt));
    if (!spGt)
        goto Cleanup;

    IFC(spGt->TransformPoint(p00, &layoutPosition));

Cleanup:
    RRETURN(hr);
}

// Called when popup is closing to clear visual states on items.  The reason for this, is that there is no exit event
// fired for ComboBoxItems when popup is closing and variables affecting visual states are not cleared.
_Check_return_ HRESULT ComboBox::ClearStateFlagsOnItems()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IPanel> spPanel = NULL;

    ASSERT(!IsSmallFormFactor(), L"ClearStateFlagsOnItems is not used in small form factor mode");

    IFC(get_ItemsHost(&spPanel));

    if(spPanel != NULL)
    {
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren = NULL;
        UINT childrenCount = 0;

        IFC(spPanel->get_Children(&spChildren));
        IFC(spChildren->get_Size(&childrenCount));

        for(UINT childIndex = 0; childIndex < childrenCount; childIndex++)
        {
            ctl::ComPtr<xaml::IUIElement> spChild = NULL;
            ctl::ComPtr<xaml_controls::IComboBoxItem> spIComboBoxItem = NULL;

            IFC(spChildren->GetAt(childIndex, &spChild));

            spIComboBoxItem = spChild.AsOrNull<xaml_controls::IComboBoxItem>();

            if(spIComboBoxItem != NULL)
            {
                IFC(spIComboBoxItem.Cast<ComboBoxItem>()->ClearVisualStateFlagsOnExit());
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

#pragma endregion Popup Support

#pragma region Inline Mode Layout

_Check_return_ HRESULT ComboBox::PrepareLayoutForInlineDropDownClosed()
{
    HRESULT hr = S_OK;
    DOUBLE itemsHostHeight = 0;
    INT32 indexToUse;
    UINT32 itemCount = 0;
    ctl::ComPtr<IDependencyObject> spContainerAsDO;
    ctl::ComPtr<IComboBoxTemplateSettings> spTemplateSettings;

    ASSERT(IsInline() || m_isExpanded, L"PrepareLayoutForInlineDropDownClosed is used only in inline mode");

    IFC(get_TemplateSettings(&spTemplateSettings));
    IFC(get_SelectedIndex(&indexToUse));
    IFC(GetItemCount(itemCount));

    if (indexToUse == -1 && itemCount > 0)
    {
        // If no item is selected then the ComboBox will be empty while closed, but when it
        // is pressed it flies open to reveal the list of elements, and for that animation
        // to look right the position of the hidden itemspresenter needs to be the same as
        // if the first item were selected.
        indexToUse = 0;
    }

    if (indexToUse != -1)
    {
        IFC(ContainerFromIndex(indexToUse, &spContainerAsDO));
    }

    if (spContainerAsDO)
    {
        DOUBLE containerHeight = 0;
        wf::Rect containerLayoutSlot = {};
        xaml::Thickness containerMargin = {};
        xaml::Thickness containerPadding = {};
        ctl::ComPtr<IFrameworkElement> spContainerAsFE;
        ctl::ComPtr<LayoutInformation> spLayoutInformation;
        ctl::ComPtr<IControl> spContainerAsControl;

        IFC(spContainerAsDO.As(&spContainerAsFE));
        IFC(spContainerAsDO.As(&spContainerAsControl));
        IFC(spContainerAsFE->get_ActualHeight(&containerHeight));
        IFC(spContainerAsFE->get_Margin(&containerMargin));
        IFC(spContainerAsControl->get_Padding(&containerPadding));

        if (containerHeight > 0)
        {
            // Items are padded to provide good spacing when the drop down is open.
            // When the drop down is closed this extra padding is not desired.
            itemsHostHeight = containerHeight + containerMargin.Top + containerMargin.Bottom -
                (containerPadding.Bottom + containerPadding.Top);

            // floating point rounding error can sometimes cause itemsHostHeight to be negative
            itemsHostHeight = MAX(0, itemsHostHeight);
        }

        IFC(ctl::make<LayoutInformation>(&spLayoutInformation));
        IFC(spLayoutInformation->GetLayoutSlot(spContainerAsFE.Get(), &containerLayoutSlot));
        IFC(spTemplateSettings.Cast<ComboBoxTemplateSettings>()->put_DropDownOffset(
            -containerLayoutSlot.Y - containerPadding.Top));
    }

    // Note that there is a MinHeight enforced by the template.
    IFC(spTemplateSettings.Cast<ComboBoxTemplateSettings>()->put_DropDownClosedHeight(itemsHostHeight));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::PrepareLayoutForInlineDropDownOpened()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IComboBoxTemplateSettings> spTemplateSettings;

    ASSERT(IsInline(), L"PrepareLayoutForInlineDropDownOpened is used only in inline mode");

    IFC(get_TemplateSettings(&spTemplateSettings));

    if (m_tpItemsPresenterPart)
    {
        DOUBLE itemsPresenterHeight = 0;
        IFC(m_tpItemsPresenterPart.AsOrNull<IFrameworkElement>()->get_ActualHeight(&itemsPresenterHeight));
        IFC(spTemplateSettings.Cast<ComboBoxTemplateSettings>()->put_DropDownOpenedHeight(itemsPresenterHeight));
    }

    IFC(spTemplateSettings.Cast<ComboBoxTemplateSettings>()->put_DropDownOffset(0));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::ForceApplyInlineLayoutUpdate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IComboBoxTemplateSettings> spTemplateSettings;
    BOOLEAN isDropDownOpen = FALSE;

    ASSERT(IsInline(), L"ForceApplyInlineLayoutUpdate is used only in inline mode");

    IFC(get_TemplateSettings(&spTemplateSettings));
    IFC(get_IsDropDownOpen(&isDropDownOpen));

    IFC(isDropDownOpen? PrepareLayoutForInlineDropDownOpened() : PrepareLayoutForInlineDropDownClosed());

    if (m_tpItemsPresenterTranslateTransformPart)
    {
        DOUBLE dropDownOffset = 0;
        IFC(spTemplateSettings->get_DropDownOffset(&dropDownOffset));
        IFC(m_tpItemsPresenterTranslateTransformPart->put_Y(dropDownOffset));
    }

    if (m_tpItemsPresenterHostPart)
    {
        DOUBLE dropDownHeight = 0;

        if (isDropDownOpen)
        {
            IFC(spTemplateSettings->get_DropDownOpenedHeight(&dropDownHeight));
        }
        else
        {
            IFC(spTemplateSettings->get_DropDownClosedHeight(&dropDownHeight));
        }

        IFC(m_tpItemsPresenterHostPart.AsOrNull<IFrameworkElement>()->put_Height(dropDownHeight));
    }

Cleanup:
    RRETURN(hr);
}

#pragma endregion Inline Mode Layout

#pragma region Automation Support

// Create ComboBoxAutomationPeer to represent the ComboBox.
IFACEMETHODIMP ComboBox::OnCreateAutomationPeer(
    _Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IComboBoxAutomationPeer> spComboBoxAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IComboBoxAutomationPeerFactory> spComboBoxAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ComboBoxAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spComboBoxAPFactory));

    IFC(spComboBoxAPFactory.Cast<ComboBoxAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spComboBoxAutomationPeer));
    IFC(spComboBoxAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// This code gets called from Automation Provider e.g. ItemAutomationPeer and must not be called from any
// internal APIs in the control itself. It basically returns the Container for the DataItem in case it exist.
// When container doesn't exist and Item is selected Item faceplate ContentPresenter is returned.
_Check_return_ HRESULT
ComboBox::UIA_GetContainerForDataItemOverride(
    _In_ IInspectable* pItem,
    _In_ INT itemIndex,
    _Outptr_ xaml::IUIElement** ppContainer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spContainer;
    BOOLEAN bIsDropDownOpen = FALSE;

    *ppContainer = nullptr;

    IFC(ComboBoxGenerated::UIA_GetContainerForDataItemOverride(pItem, itemIndex, &spContainer));


    if (!spContainer)
    {
        ctl::ComPtr<IInspectable> spSelectedItem;
        bool areEqual = false;

        IFC(get_IsDropDownOpen(&bIsDropDownOpen));

        // If we have the selection and looking a container for Selected item in case drop down is closed, we must return the faceplate comboboxItem.
        if (!bIsDropDownOpen && m_tpContentPresenterPart)
        {
            IFC(get_SelectedItem(&spSelectedItem));
            IFC(PropertyValue::AreEqual(spSelectedItem.Get(), pItem, &areEqual));
            if (areEqual)
            {
                IFC(m_tpContentPresenterPart.As<IUIElement>(&spContainer));
            }
        }
    }

    IFC(spContainer.MoveTo(ppContainer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::RaiseAutomationPeerExpandCollapse(
    _In_ BOOLEAN isDropDownOpen)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IComboBoxAutomationPeer> spComboBoxAutomationPeer;
    BOOLEAN isListener = FALSE;

    IFC(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &isListener));
    if (isListener)
    {
        IFC(GetOrCreateAutomationPeer(&spAutomationPeer));
        if(spAutomationPeer)
        {
            spComboBoxAutomationPeer = spAutomationPeer.AsOrNull<xaml_automation_peers::IComboBoxAutomationPeer>();
            if(spComboBoxAutomationPeer)
            {
                IFC(spComboBoxAutomationPeer.Cast<ComboBoxAutomationPeer>()->RaiseExpandCollapseAutomationEvent(isDropDownOpen));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBox::GetLightDismissElement(
    _Outptr_ xaml::IUIElement** ppUIElement)
{
    IFCPTR_RETURN(ppUIElement);
    *ppUIElement = NULL;

    if (m_tpElementOutsidePopup.Get())
    {
        IFC_RETURN(m_tpElementOutsidePopup.CopyTo(ppUIElement));
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::GetEditableTextPart(
    _Outptr_ xaml::IUIElement** ppUIElement)
{
    IFCPTR_RETURN(ppUIElement);
    *ppUIElement = NULL;

    if (m_tpEditableTextPart.Get())
    {
        IFC_RETURN(m_tpEditableTextPart.CopyTo(ppUIElement));
    }

    return S_OK;
}

// Returns Faceplate ContentPresnter
_Check_return_ HRESULT ComboBox::GetContentPresenterPart(_Outptr_ xaml_controls::IContentPresenter** ppContentPresenterPart)
{
    HRESULT hr = S_OK;

    *ppContentPresenterPart = NULL;
    if (m_tpContentPresenterPart)
    {
        IFC(m_tpContentPresenterPart.CopyTo(ppContentPresenterPart));
    }

Cleanup:
    RRETURN(hr);
}

#pragma endregion Automation Support

#pragma region Internal Methods

// Required by ComboBoxItem::ChangeVisualState
_Check_return_ HRESULT
ComboBox::IsInlineMode(_Out_ BOOLEAN* isInlineMode)
{
    *isInlineMode = IsInline();
    RRETURN(S_OK);
}

DirectUI::InputDeviceType
ComboBox::GetInputDeviceTypeUsedToOpen()
{
    return m_inputDeviceTypeUsedToOpen;
}

/// Called to detect whether we can scroll to the View or not.
/// Returns true when base is true and drop down list is opened
_Check_return_ HRESULT ComboBox::CanScrollIntoView(
    _Out_ BOOLEAN& canScroll)
{
    HRESULT hr = S_OK;
    BOOLEAN bBaseCanScroll = FALSE;
    BOOLEAN bIsDropDownOpen = FALSE;
    IFC(get_IsDropDownOpen(&bIsDropDownOpen));
    IFC(ComboBoxGenerated::CanScrollIntoView(bBaseCanScroll));
    canScroll = bBaseCanScroll && bIsDropDownOpen;

Cleanup:
    RRETURN(hr);
}

// Get the visible bounds area that excludes the application title bar height
// in case using CoreApplicationViewTitleBar ExtendViewIntoTitleBar=true.
_Check_return_ HRESULT ComboBox::GetVisibleBoundsInternal(
    _Out_ wf::Rect* pVisibleBounds)
{
    IFC_RETURN(DXamlCore::GetCurrent()->GetVisibleContentBoundsForElement(GetHandle(), pVisibleBounds));

    ctl::ComPtr<wac::ICoreApplication> coreApplication;

    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
        &coreApplication));

    if (coreApplication)
    {
        ctl::ComPtr<wac::ICoreApplicationView> coreApplicationView;

        HRESULT hr = coreApplication->GetCurrentView(&coreApplicationView);
        if (FAILED(hr))
        {
            // Xaml hosting apps like as SystemSettingsAdminFlows(Settings\Accounts\Other Users\LocalAccount) doesn't
            // support the core window or views and return the failure instead of nullptr.
            return S_OK;
        }

        if (coreApplicationView)
        {
            ctl::ComPtr<wac::ICoreApplicationView3> coreApplicationView3;

            IFC_RETURN(coreApplicationView.As(&coreApplicationView3));

            if (coreApplicationView3)
            {
                ctl::ComPtr<wac::ICoreApplicationViewTitleBar> coreApplicationViewTitleBar;
                IFC_RETURN(coreApplicationView3->get_TitleBar(&coreApplicationViewTitleBar));

                if (coreApplicationViewTitleBar)
                {
                    BOOLEAN extendViewIntoTitleBar = FALSE;

                    IFC_RETURN(coreApplicationViewTitleBar->get_ExtendViewIntoTitleBar(&extendViewIntoTitleBar));

                    if (extendViewIntoTitleBar)
                    {
                        double titleBarHeight = 0;

                        IFC_RETURN(coreApplicationViewTitleBar->get_Height(&titleBarHeight));
                        if (titleBarHeight > 0)
                        {
                            pVisibleBounds->Y += static_cast<float>(titleBarHeight);
                            pVisibleBounds->Height -= static_cast<float>(titleBarHeight);
                        }
                    }
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ComboBox::ReevaluateIsOverlayVisible()
{
    ASSERT(m_tpPopupPart);

    bool isOverlayVisible = false;
    IFC_RETURN(LightDismissOverlayHelper::ResolveIsOverlayVisibleForControl(this, &isOverlayVisible));

    if (isOverlayVisible != m_isOverlayVisible)
    {
        m_isOverlayVisible = isOverlayVisible;

        if (m_isOverlayVisible)
        {
            // Normally, the overlay is only shown for light-dismiss popups, but for controls that roll their own
            // light-dismiss logic (and therefore configure their popup's to not be light-dismiss) we still want
            // to re-use the popup's overlay code.
            IFC_RETURN(m_tpPopupPart.Cast<Popup>()->put_DisableOverlayIsLightDismissCheck(TRUE));
            IFC_RETURN(m_tpPopupPart.Cast<Popup>()->put_LightDismissOverlayMode(xaml_controls::LightDismissOverlayMode_On));

            // Set the appropriate brush resource to use for the overlay.
            xstring_ptr themeBrush;
            IFC_RETURN(xstring_ptr::CloneBuffer(L"ComboBoxLightDismissOverlayBackground", &themeBrush));
            IFC_RETURN(static_cast<CPopup*>(m_tpPopupPart.Cast<Popup>()->GetHandle())->SetOverlayThemeBrush(themeBrush));

            IFC_RETURN(UpdateTargetForOverlayAnimations());
        }
        else
        {
            IFC_RETURN(m_tpPopupPart.Cast<Popup>()->put_LightDismissOverlayMode(xaml_controls::LightDismissOverlayMode_Off));

            // Make sure we've stopped our animations.
            if (m_overlayOpeningStoryboard)
            {
                IFC_RETURN(m_overlayOpeningStoryboard->Stop());
            }

            if (m_overlayClosingStoryboard)
            {
                IFC_RETURN(m_overlayClosingStoryboard->Stop());
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ComboBox::UpdateTargetForOverlayAnimations()
{
    ctl::ComPtr<xaml::IFrameworkElement> overlayElement;
    IFC_RETURN(m_tpPopupPart.Cast<Popup>()->get_OverlayElement(&overlayElement));
    ASSERT(overlayElement);

    if (m_overlayOpeningStoryboard)
    {
        IFC_RETURN(m_overlayOpeningStoryboard->Stop());

        IFC_RETURN(CoreImports::Storyboard_SetTarget(
            static_cast<CTimeline*>(m_overlayOpeningStoryboard.Cast<Storyboard>()->GetHandle()),
            overlayElement.Cast<FrameworkElement>()->GetHandle())
            );
    }

    if (m_overlayClosingStoryboard)
    {
        IFC_RETURN(m_overlayClosingStoryboard->Stop());

        IFC_RETURN(CoreImports::Storyboard_SetTarget(
            static_cast<CTimeline*>(m_overlayClosingStoryboard.Cast<Storyboard>()->GetHandle()),
            overlayElement.Cast<FrameworkElement>()->GetHandle())
            );
    }

    return S_OK;
}

_Check_return_ HRESULT
ComboBox::PlayOverlayOpeningAnimation()
{
    ASSERT(m_isOverlayVisible);

    if (m_overlayClosingStoryboard)
    {
        IFC_RETURN(m_overlayClosingStoryboard->Stop());
    }

    if (m_overlayOpeningStoryboard)
    {
        IFC_RETURN(m_overlayOpeningStoryboard->Begin());
    }

    return S_OK;
}

_Check_return_ HRESULT
ComboBox::PlayOverlayClosingAnimation()
{
    ASSERT(m_isOverlayVisible);
    ASSERT(m_isDropDownClosing);

    if (m_overlayOpeningStoryboard)
    {
        IFC_RETURN(m_overlayOpeningStoryboard->Stop());
    }

    if (m_overlayClosingStoryboard)
    {
        IFC_RETURN(m_overlayClosingStoryboard->Begin());
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::OverrideSelectedIndexForVisualStates(
    _In_ int selectedIndexOverride)
{
#if DBG
    BOOLEAN canSelectMultiple = FALSE;
    IFC_RETURN(get_CanSelectMultiple(&canSelectMultiple));
    ASSERT(!canSelectMultiple);
#endif

    ClearSelectedIndexOverrideForVisualStates();

    // We only need to override the selected visual if the specified item is not
    // also the selected item.
    int selectedIndex = -1;
    IFC_RETURN(get_SelectedIndex(&selectedIndex));
    if (selectedIndexOverride != selectedIndex)
    {
        ctl::ComPtr<IDependencyObject> container;
        ctl::ComPtr<IComboBoxItem> comboBoxItem;

        // Force the specified override  item to appear selected.
        if (selectedIndexOverride != -1)
        {
            IFC_RETURN(ContainerFromIndex(selectedIndexOverride, &container));
            comboBoxItem = container.AsOrNull<IComboBoxItem>();
            if (comboBoxItem)
            {
                IFC_RETURN(comboBoxItem.Cast<ComboBoxItem>()->OverrideSelectedVisualState(true /* appearSelected */));
            }
        }

        m_indexForcedToSelectedVisual = selectedIndexOverride;

        if (selectedIndex != -1)
        {
            // Force the actual selected item to appear unselected.
            IFC_RETURN(ContainerFromIndex(selectedIndex, &container));
            comboBoxItem = container.AsOrNull<IComboBoxItem>();
            if (comboBoxItem)
            {
                IFC_RETURN(comboBoxItem.Cast<ComboBoxItem>()->OverrideSelectedVisualState(false /* appearSelected */));
            }

            m_indexForcedToUnselectedVisual = selectedIndex;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::ClearSelectedIndexOverrideForVisualStates()
{
#if DBG
    BOOLEAN canSelectMultiple = FALSE;
    IFC_RETURN(get_CanSelectMultiple(&canSelectMultiple));
    ASSERT(!canSelectMultiple);
#endif

    ctl::ComPtr<IDependencyObject> container;
    ctl::ComPtr<IComboBoxItem> comboBoxItem;

    if (m_indexForcedToUnselectedVisual != -1)
    {
        IFC_RETURN(ContainerFromIndex(m_indexForcedToUnselectedVisual, &container));
        comboBoxItem = container.AsOrNull<IComboBoxItem>();
        if (comboBoxItem)
        {
            IFC_RETURN(comboBoxItem.Cast<ComboBoxItem>()->ClearSelectedVisualState());
        }

        m_indexForcedToUnselectedVisual = -1;
    }

    if (m_indexForcedToSelectedVisual != -1)
    {
        IFC_RETURN(ContainerFromIndex(m_indexForcedToSelectedVisual, &container));
        comboBoxItem = container.AsOrNull<IComboBoxItem>();
        if (comboBoxItem)
        {
            IFC_RETURN(comboBoxItem.Cast<ComboBoxItem>()->ClearSelectedVisualState());
        }

        m_indexForcedToSelectedVisual = -1;
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::EnsurePropertyPathListener()
{
    if (!m_spPropertyPathListener)
    {
        wrl_wrappers::HString strDisplayMemberPath;
        IFC_RETURN(get_DisplayMemberPath(strDisplayMemberPath.GetAddressOf()));

        if (!strDisplayMemberPath.IsEmpty())
        {
            // If we don't have one cached, create the property path listener
            // If strDisplayMemberPath contains something (a path), then use that to inform our PropertyPathListener.
            auto pPropertyPathParser = std::make_unique<PropertyPathParser>();

            IFC_RETURN(pPropertyPathParser->SetSource(WindowsGetStringRawBuffer(strDisplayMemberPath.Get(), nullptr), FALSE));

            IFC_RETURN(ctl::make<PropertyPathListener>(nullptr, pPropertyPathParser.get(), false /*fListenToChanges*/, false /*fUseWeakReferenceForSource*/, &m_spPropertyPathListener));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ComboBox::CreateEditableContentPresenterTextBlock()
{
    if (!m_tpEditableContentPresenterTextBlock)
    {
        ctl::ComPtr<IInspectable> spTextBlock;

        const CClassInfo* pTextBlockTypeInfo = MetadataAPI::GetClassInfoByIndex(KnownTypeIndex::TextBlock);
        IFC_RETURN(ActivationAPI::ActivateInstance(pTextBlockTypeInfo, &spTextBlock));

        ctl::ComPtr<xaml_controls::ITextBlock> spEditableContentPresenterTextBlockAsI;
        spTextBlock.As(&spEditableContentPresenterTextBlockAsI);

        SetPtrValue(m_tpEditableContentPresenterTextBlock, spEditableContentPresenterTextBlockAsI.Get());
    }

    return S_OK;
}

#pragma endregion Internal Methods

_Check_return_ HRESULT ComboBoxFactory::get_IsEditablePropertyImpl(_Outptr_result_maybenull_ xaml::IDependencyProperty** ppValue)
{
    IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::ComboBox_IsEditable, ppValue));
    return S_OK;
}
