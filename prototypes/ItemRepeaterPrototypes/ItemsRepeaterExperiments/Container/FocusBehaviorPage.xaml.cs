using ItemsRepeaterExperiments.Common;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace ItemsRepeaterExperiments.Container
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class FocusBehaviorPage : Page
    {
        private List<Team> teams = DataSourceCreator<Team>.CreateRandomizedList(100);
        
        public FocusBehaviorPage()
        {
            this.InitializeComponent();
            TeamPresenter.ItemsSource = teams;
        }

        /// <summary>
        /// Determine what element we want to focus next. This is something that can only be done with knowledge of the layout.
        /// @WINUI: We should probably provide a way to scroll a certain element into view and not having to do this manually
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void HandleContainer_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            // This is all handled by the StackLayout. For custom layout we would need to handle it ourselves.
            /*
            var index = TeamPresenter.GetElementIndex(sender as Control);
            // If we would focus an element that does not exist, just skip it
            if(index >= teams.Count - 1|| index < 0)
            {
                return;
            }

            var nextIndex = index;
            if(e.Key == Windows.System.VirtualKey.Up)
            {
                nextIndex--;
            }
            if (e.Key == Windows.System.VirtualKey.Down)
            {
                nextIndex++;
            }

            Debug.WriteLine(nextIndex + " " + e.Key);
            // Determine next element!
            var nextElement = (TeamPresenter.TryGetElement(nextIndex) as Control);

            // The following calculations are only done to scroll to the correct element
            var transform = nextElement.TransformToVisual((UIElement)TeamPresenterScroller.Content);
            var point = transform.TransformPoint(new Point(0, 0));
            TeamPresenterScroller.ChangeView(point.X, point.Y, null, false);
            */    
        }



        //--------------------- Options and logging --------------------------//
        private void LayoutChooser_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            TeamPresenterScroller.HorizontalScrollBarVisibility = Windows.UI.Xaml.Controls.ScrollBarVisibility.Auto;
            TeamPresenterScroller.VerticalScrollBarVisibility = Windows.UI.Xaml.Controls.ScrollBarVisibility.Auto;
            switch (LayoutChooser.SelectedItem as string)
            {
                case "Stacklayout horizontal":
                    TeamPresenter.Layout = new StackLayout() { Orientation = Orientation.Horizontal };
                    break;
                case "Stacklayout vertical":
                    TeamPresenter.Layout = new StackLayout() { Orientation = Orientation.Vertical };
                    break;
                case "FlowLayout horizontal":
                    // To prevent this from getting flaky, disable horizontal scroll
                    TeamPresenterScroller.HorizontalScrollBarVisibility = Windows.UI.Xaml.Controls.ScrollBarVisibility.Disabled;
                    TeamPresenter.Layout = new FlowLayout()
                    {
                        Orientation = Orientation.Horizontal
                    };
                    break;
                case "FlowLayout vertical":
                    TeamPresenterScroller.VerticalScrollBarVisibility = Windows.UI.Xaml.Controls.ScrollBarVisibility.Disabled;
                    TeamPresenter.Layout = new FlowLayout()
                    {
                        Orientation = Orientation.Vertical
                    };
                    break;
            }
        }

        private void HandleContainer_GotFocus(object sender, RoutedEventArgs e)
        {
            var index = TeamPresenter.GetElementIndex(sender as Control);
            focusedElementName.Text = teams[index].Name;
        }
    }
}
