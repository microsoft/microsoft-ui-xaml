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

namespace ItemsRepeaterExperiments.AttachedBehaviors
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class SelectionModelPage : Page
    {
        private List<Team> teams = DataSourceCreator<Team>.CreateRandomizedList(100);
        private SelectionModel selectionModel = new SelectionModel();
        public SelectionModelPage()
        {
            this.InitializeComponent();
            selectionModel.Source = teams;
            selectionModel.SelectionChanged += SelectionModel_SelectionChanged;
            selectionModel.SingleSelect = true;
        }


        /// <summary>
        /// Attach properties to virtualized items and update containers
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void ItemsRepeater_ElementPrepared(Microsoft.UI.Xaml.Controls.ItemsRepeater sender, Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs args)
        {
            new PointerBehaviors(args.Element as Control).Click += Item_Click;
            var selectionbehavior = new SelectionBehavior(args.Element as Control);

            if(selectionModel.SelectedIndex != null && selectionModel.SelectedIndex.GetAt(0) == args.Index)
            {
                selectionbehavior.IsSelected = true;
            }
            else
            {
                selectionbehavior.IsSelected = false;
            }
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
                var control = TeamPresenter.TryGetElement(teams.IndexOf(item as Team)) as Control;
                // Container not realized ... , so skip
                if (control == null)
                {
                    Debug.WriteLine(item.Name);
                    continue;
                }
                var container = new SelectionBehavior(control);
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
        private void Item_Click(object sender, RoutedEventArgs e)
        {
            selectionModel.Select(TeamPresenter.GetElementIndex(sender as Control));
        }
    }
}
