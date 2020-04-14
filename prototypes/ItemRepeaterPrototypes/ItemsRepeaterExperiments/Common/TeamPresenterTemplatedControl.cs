using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;

// The Templated Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234235

namespace ItemsRepeaterExperiments.Common
{
    public sealed class TeamPresenterTemplatedControl : Control
    {


        public Team Team
        {
            get { return (Team)GetValue(TeamProperty); }
            set { 
                SetValue(TeamProperty, value);
                if(value != null)
                {
                    (GetTemplateChild("TeamName") as TextBlock).Text = Team.Name;
                    (GetTemplateChild("TeamMembers") as TextBlock).Text = Team.Name;
                }

            }
        }

        // Using a DependencyProperty as the backing store for Team.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty TeamProperty =
            DependencyProperty.Register("Team", typeof(Team), typeof(TeamPresenterTemplatedControl), new PropertyMetadata(null));



        public TeamPresenterTemplatedControl()
        {
            this.DefaultStyleKey = typeof(TeamPresenterTemplatedControl);

            Loaded += TeamPresenterTemplatedControl_Loaded;
        }

        private void TeamPresenterTemplatedControl_Loaded(object sender, RoutedEventArgs e)
        {

            (GetTemplateChild("TeamName") as TextBlock).Text = Team.Name;
            (GetTemplateChild("TeamMembers") as TextBlock).Text = Team.MembersString;
        }
    }
}
