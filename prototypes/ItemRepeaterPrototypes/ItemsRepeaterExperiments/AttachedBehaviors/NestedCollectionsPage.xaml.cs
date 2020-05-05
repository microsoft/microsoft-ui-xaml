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

            selectionModel.SelectionChanged += SelectionModel_SelectionChanged;
            selectionModel.ChildrenRequested += SelectionModel_ChildrenRequested;
        }

        private void SelectionModel_ChildrenRequested(SelectionModel sender, SelectionModelChildrenRequestedEventArgs args)
        {
            // This function does NOT get raised
            throw new NotImplementedException();
        }

        private void SelectionModel_SelectionChanged(SelectionModel sender, SelectionModelSelectionChangedEventArgs args)
        {
            // This function gets raised
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
