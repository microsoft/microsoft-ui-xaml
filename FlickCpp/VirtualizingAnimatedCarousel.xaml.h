//
// VirtualizingAnimatedCarousel.xaml.h
// Declaration of the VirtualizingAnimatedCarousel class
//

#pragma once

#include "VirtualizingAnimatedCarousel.g.h"
#include "VirtualizingAnimatedUniformCarouselStackLayout.h"
#include "SelectableSnapPointForwardingRepeater.h"
#include "DemoViewModel.h"

namespace FlickCpp
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class VirtualizingAnimatedCarousel sealed
        : public Windows::UI::Xaml::Data::INotifyPropertyChanged
	{

	public:
		VirtualizingAnimatedCarousel();

        // INotifyPropertyChanged
        virtual event Windows::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

        property FlickCpp::DemoViewModel^ ViewModel
        {
            FlickCpp::DemoViewModel^ get();
        }

    private:
        enum class ScrollDirection
        {
            Previous = 0,
            Next
        };

        static double Floor(double num);
        static double AbsoluteValue(double num);

        property Windows::System::Threading::ThreadPoolTimer^ PrevButtonContinuousScrollingPeriodicTimer
        {
            Windows::System::Threading::ThreadPoolTimer^ get();
            void set(Windows::System::Threading::ThreadPoolTimer^ value);
        }

        property Windows::System::Threading::ThreadPoolTimer^ NextButtonContinuousScrollingPeriodicTimer
        {
            Windows::System::Threading::ThreadPoolTimer^ get();
            void set(Windows::System::Threading::ThreadPoolTimer^ value);
        }

        property Windows::System::Threading::ThreadPoolTimer^ PrevButtonHoldTimer
        {
            Windows::System::Threading::ThreadPoolTimer^ get();
            void set(Windows::System::Threading::ThreadPoolTimer^ value);
        }

        property Windows::System::Threading::ThreadPoolTimer^ NextButtonHoldTimer
        {
            Windows::System::Threading::ThreadPoolTimer^ get();
            void set(Windows::System::Threading::ThreadPoolTimer^ value);
        }

        property Windows::System::Threading::ThreadPoolTimer^ ScrollViewerChangeViewTimer
        {
            Windows::System::Threading::ThreadPoolTimer^ get();
            void set(Windows::System::Threading::ThreadPoolTimer^ value);
        }

        property Windows::System::Threading::ThreadPoolTimer^ PreserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangeTimer
        {
            Windows::System::Threading::ThreadPoolTimer^ get();
            void set(Windows::System::Threading::ThreadPoolTimer^ value);
        }

        void HideCarouselNextPrevButtons();
        void ShowCarouselNextPrevButtons();
        double CenterPointOfViewportInExtent();
        int GetSelectedIndexFromViewport();
        Platform::Object^ GetSelectedItemFromViewport();
        void DetermineIfCarouselNextPrevButtonsShouldBeHidden();
        void SetSelectedItemInViewport(int itemIndexInItemsSource);
        void ScrollViewer_ViewChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs^ e);
        void ScrollViewer_ViewChanging(Platform::Object^ sender, Windows::UI::Xaml::Controls::ScrollViewerViewChangingEventArgs^ e);
        void ScrollViewer_LayoutUpdated(Platform::Object^ sender, Platform::Object^ e);
        void ResetCarouselWithDefaultSelectionForCurrentItemCount();
        void Repeater_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void Repeater_ElementPrepared(Microsoft::UI::Xaml::Controls::ItemsRepeater^ sender, Microsoft::UI::Xaml::Controls::ItemsRepeaterElementPreparedEventArgs^ e);
        void CancelAllCarouselPrevButtonTimers();
        void CancelAllCarouselNextButtonTimers();
        void CancelAllCarouselScrollRelatedTimers();
        void ScrollViewer_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
        void SelectNextItem(int numberOfItemsToSkip);
        void SelectNextItem();
        void SelectPreviousItem(int numberOfItemsToSkip);
        void SelectPreviousItem();
        void StartContinuousScrolling(FlickCpp::VirtualizingAnimatedCarousel::ScrollDirection scrollDirection);
        void CarouselPrevButton_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
        void CarouselPrevButton_PointerCanceled(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
        void CarouselPrevButton_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
        void CarouselPrevButton_PointerExited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
        void CarouselPrevButton_PointerCaptureLost(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
        void CarouselNextButton_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
        void CarouselNextButton_PointerCanceled(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
        void CarouselNextButton_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
        void CarouselNextButton_PointerExited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
        void CarouselNextButton_PointerCaptureLost(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
        void VirtualizingAnimatedCarousel_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);

        static const Windows::Foundation::TimeSpan s_prevNextButtonHoldPeriod;
        static const int s_continousScrollingItemSkipCount;
        static const Windows::Foundation::TimeSpan s_prevNextButtonContinousScrollingSelectionPeriod;
        static const Windows::Foundation::TimeSpan s_scrollViewerChangeViewDelayPeriodForDefaultItemSelectionInEvenItemCountScenario;
        static const Windows::Foundation::TimeSpan s_scrollViewerChangeViewDelayPeriodToEnsureAnimationIsShown;
        static const Windows::Foundation::TimeSpan s_preserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangePeriod;

        Windows::UI::Core::CoreDispatcher^ m_dispatcher = nullptr;;
        Windows::UI::Xaml::Controls::Button^ m_carouselPrevButton = nullptr;
        Windows::UI::Xaml::Controls::Button^ m_carouselNextButton = nullptr;
        Windows::UI::Xaml::Controls::ScrollViewer^ m_scrollViewer = nullptr;
        FlickCpp::VirtualizingAnimatedUniformCarouselStackLayout^ m_repeaterLayout = nullptr;
        FlickCpp::SelectableSnapPointForwardingRepeater^ m_repeater = nullptr;
        Windows::System::Threading::ThreadPoolTimer^ m_prevButtonContinuousScrollingPeriodicTimer = nullptr;
        Windows::System::Threading::ThreadPoolTimer^ m_nextButtonContinuousScrollingPeriodicTimer = nullptr;
        Windows::System::Threading::ThreadPoolTimer^ m_prevButtonHoldTimer = nullptr;
        Windows::System::Threading::ThreadPoolTimer^ m_nextButtonHoldTimer = nullptr;
        Windows::System::Threading::ThreadPoolTimer^ m_scrollViewerChangeViewTimer = nullptr;
        Windows::System::Threading::ThreadPoolTimer^ m_preserveSelectedItemAfterVirtualizingAnimatedCarouselSizeChangeTimer = nullptr;
        int m_selectedItemIndexPriorToVirtualizingAnimatedCarouselSizeChange = FlickCpp::SelectableSnapPointForwardingRepeater::SelectedIndexValueWhenNoItemIsSelected;
        FlickCpp::DemoViewModel^ m_viewModel;
	};
}
