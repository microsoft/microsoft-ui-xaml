using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace ItemsRepeaterExperiments.AttachedBehaviors
{
    public class SelectionBehavior : DependencyObject
    {



        public bool IsSelected
        {
            get { return (bool)GetValue(IsSelectedProperty); }
            set { 
                SetValue(IsSelectedProperty, value);
                IsSelectedChanged(value);
            }
        }

        // Using a DependencyProperty as the backing store for IsSelected.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty IsSelectedProperty =
            DependencyProperty.Register("IsSelected", typeof(bool), typeof(SelectionBehavior), new PropertyMetadata(false));



        private readonly Control element;

        public SelectionBehavior(Control element)
        {
            if(element == null)
            {
                return;
            }
            this.element = element;
            VisualStateManager.GoToState(element, "Deselected", true);
        }
        private void IsSelectedChanged(bool value)
        {
            if(value)
            {
                VisualStateManager.GoToState(element, "Selected", true);
            }
            else
            {
                VisualStateManager.GoToState(element, "Deselected", true);
            }
        }

    }
}
