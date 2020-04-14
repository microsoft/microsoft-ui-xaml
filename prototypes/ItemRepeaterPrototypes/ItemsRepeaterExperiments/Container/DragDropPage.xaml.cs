using ItemsRepeaterExperiments.Common;
using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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
    public sealed partial class DragDropPage : Page
    {
        private ObservableCollection<Team> firstTeams = DataSourceCreator<Team>.CreateRandomizedObservableCollection(4);
        private ObservableCollection<Team> secondTeams = DataSourceCreator<Team>.CreateRandomizedObservableCollection(4);
        public DragDropPage()
        {
            this.InitializeComponent();

            FirstTeamPresenter.ItemsSource = firstTeams;
            SecondTeamPresenter.ItemsSource = secondTeams;
        }

        private void FirstTeamPresenter_Drop(object sender, DragEventArgs e)
        {
            var container = e.DataView.Properties["Container"] as Control;
            if (FirstTeamPresenter.GetElementIndex(container) == -1)
            {
                var team = GetItemFromElement(SecondTeamPresenter, container);
                secondTeams.Remove(team);
                firstTeams.Add(team);
            }
        }

        private void FirstTeamPresenter_DragOver(object sender, DragEventArgs e)
        {
            if (FirstTeamPresenter.GetElementIndex(e.DataView.Properties["Container"] as Control) == -1)
            {
                e.AcceptedOperation = Windows.ApplicationModel.DataTransfer.DataPackageOperation.Move;
            }
        }
            
        private void SecondTeamPresenter_Drop(object sender, DragEventArgs e)
        {
            var container = e.DataView.Properties["Container"] as Control;
            if (SecondTeamPresenter.GetElementIndex(container) == -1)
            {
                var team = GetItemFromElement(FirstTeamPresenter, container);
                firstTeams.Remove(team);
                secondTeams.Add(team);
            }
        }

        private void SecondTeamPresenter_DragOver(object sender, DragEventArgs e)
        {
            if(SecondTeamPresenter.GetElementIndex(e.DataView.Properties["Container"] as Control) == -1)
            {
                e.AcceptedOperation = Windows.ApplicationModel.DataTransfer.DataPackageOperation.Move;
            }
        }

        /// <summary>
        /// @WINUI This is a pattern that was used often in this project. Maybe this should be added to the ItemsRepeater API?
        /// </summary>
        /// <param name="repeater"></param>
        /// <param name="element"></param>
        /// <returns></returns>
        static Team GetItemFromElement(ItemsRepeater repeater, UIElement element)
        {
            return repeater.ItemsSourceView.GetAt(repeater.GetElementIndex(element)) as Team;
        }
    }
}
