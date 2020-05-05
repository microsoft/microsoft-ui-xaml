using ItemsRepeaterExperiments.Common;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
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
    public sealed partial class NestedCollectionsPage : Page
    {
        private List<Team> teams = DataSourceCreator<Team>.CreateRandomizedList(10, 2, 10);

        public NestedCollectionsPage()
        {
            this.InitializeComponent();
        }

        private void SelectionModel_SelectionChanged(SelectionModel sender, SelectionModelSelectionChangedEventArgs args)
        {
            // Having the deselected items would really be handy here ...
            foreach (var item in teams)
            {
                // Since we don't get the sender from a given object, we need to handle this here!
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


    public class TeamTemplateSelector : DataTemplateSelector
    {
        public DataTemplate GroupTemplate { get; set; }

        public DataTemplate ItemTemplate { get; set; }

        protected override DataTemplate SelectTemplateCore(object item)
        {
            var team = item as Team;

            return team.SubTeams.Count > 0? GroupTemplate : ItemTemplate;
        }
    }

}
