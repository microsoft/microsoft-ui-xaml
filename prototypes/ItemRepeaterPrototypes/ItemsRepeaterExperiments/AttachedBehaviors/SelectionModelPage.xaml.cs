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
        private List<Team> teams = DataSourceCreator<Team>.CreateRandomizedList(10);
        public SelectionModelPage()
        {
            this.InitializeComponent();
        }


        /// <summary>
        /// Attach properties to virtualized items and update containers
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void ItemsRepeater_ElementPrepared(Microsoft.UI.Xaml.Controls.ItemsRepeater sender, Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs args)
        {
            var control = args.Element as FrameworkElement;


            SelectionBehavior.SetIsSelected(control,
                selectionModel.SelectedItems.Contains(teams[TeamPresenter.GetElementIndex(control)]));
        }

        private void Control_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            var elementIndex = TeamPresenter.GetElementIndex(sender as UIElement);
            if (selectionModel.SingleSelect)
            {
                if (e.KeyModifiers == Windows.System.VirtualKeyModifiers.Control)
                {
                    selectionModel.Deselect(elementIndex);
                }
                else
                {
                    selectionModel.Select(elementIndex);
                }
            }
            else
            {
                switch (e.KeyModifiers)
                {
                    case Windows.System.VirtualKeyModifiers.Shift:
                        if (PointerBehaviors.LastFocused != null)
                        {
                            var lastFocused = TeamPresenter.GetElementIndex(PointerBehaviors.LastFocused as UIElement);
                            selectionModel.SetAnchorIndex(lastFocused);
                        }
                        selectionModel.SelectRangeFromAnchor(elementIndex);
                        break;
                    case Windows.System.VirtualKeyModifiers.Control:
                        selectionModel.Deselect(elementIndex);
                        break;
                    default:
                        selectionModel.Select(elementIndex);
                        break;
                }
            }
            PointerBehaviors.LastFocused = sender as UIElement;
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
                var control = TeamPresenter.TryGetElement(teams.IndexOf(item as Team)) as DependencyObject;
                // Container not realized ... , so skip
                if (control == null)
                {
                    //Debug.WriteLine("Not found: " + item.Name);
                    continue;
                }
                if (selectionModel.SelectedItems.Contains(item))
                {
                    SelectionBehavior.SetIsSelected(control, true);
                }
                else
                {
                    SelectionBehavior.SetIsSelected(control, false);
                }
            }
        }

        private void SingleSelectCheckbox_Checked(object sender, RoutedEventArgs e)
        {
            if (selectionModel != null)
            {
                selectionModel.SingleSelect = true;
                selectionModel.ClearSelection();
            }
        }

        private void SingleSelectCheckbox_Unchecked(object sender, RoutedEventArgs e)
        {

            if (selectionModel != null)
            {
                selectionModel.SingleSelect = false;
                selectionModel.ClearSelection();
            }
        }
    }
}
