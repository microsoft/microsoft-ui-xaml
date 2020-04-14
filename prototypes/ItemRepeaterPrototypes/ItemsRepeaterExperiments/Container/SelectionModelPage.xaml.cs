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
    public sealed partial class SelectionModelPage : Page
    {
        private SelectionModel selectionModel = new SelectionModel();
        private List<Team> teams = DataSourceCreator<Team>.CreateRandomizedList(100);
        public SelectionModelPage()
        {
            this.InitializeComponent();
            TeamPresenter.ItemsSource = teams;
            TeamPresenter.ElementPrepared += TeamPresenter_ElementPrepared;
            selectionModel.Source = teams;
            selectionModel.SelectionChanged += SelectionModel_SelectionChanged;
        }

        /// <summary>
        /// @WINUI We need this to ensure proper visualstates. 
        /// If this method gets commented out, selecting the first item will result in the ~25th, ~50th,... item behaving like they were selected.
        /// Can we improve this?
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void TeamPresenter_ElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            // Since presenters get reused, we need to update them everytime they get prepared/used ...
            (args.Element as HandleContainer).IsSelected = selectionModel.IsSelected(args.Index).GetValueOrDefault();
        }


        /// <summary>
        /// This event gets raised when an item gets selected
        /// @WINUI This event definitely needs a list of selected and deselected items
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void SelectionModel_SelectionChanged(SelectionModel sender, SelectionModelSelectionChangedEventArgs args)
        {
            // Having the deselected items would really be handy here ...
            foreach (var item in teams)
            {
                var container = TeamPresenter.TryGetElement(teams.IndexOf(item as Team)) as HandleContainer;
                // Container not realized ... , so skip
                if (container == null)
                {
                    Debug.WriteLine(item.Name);
                    continue;
                }
                if (selectionModel.SelectedItems.Contains(item))
                {
                    container.IsSelected = true;
                }
                else

                {
                    container.IsSelected = false;
                }
            }
        }

        /// <summary>
        /// This event get's raised when an item gets clicked.
        /// @WINUI This could either be ItemsRepeater.ItemInvoked or we ship the container as separate control
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void HandleContainer_Click(object sender, RoutedEventArgs e)
        {
            selectionModel.Select(TeamPresenter.GetElementIndex(sender as HandleContainer));
        }

        //--------------------- Options --------------------------//
        private void LayoutChooser_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            TeamPresenterScroller.HorizontalScrollBarVisibility = Windows.UI.Xaml.Controls.ScrollBarVisibility.Auto;
            TeamPresenterScroller.VerticalScrollBarVisibility = Windows.UI.Xaml.Controls.ScrollBarVisibility.Auto;
            switch(LayoutChooser.SelectedItem as string)
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
    }
}
