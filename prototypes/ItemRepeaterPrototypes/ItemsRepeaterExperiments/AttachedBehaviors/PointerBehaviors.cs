using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

namespace ItemsRepeaterExperiments.AttachedBehaviors
{
    public class PointerBehaviors
    {
        public bool IsPointerOver { get; private set;} = false;
        private Control element;

        public event RoutedEventHandler Click;
        
        public PointerBehaviors(Control element)
        {
            if(element == null)
            {
                return;
            }
            this.element = element;

            element.PointerReleased += Element_PointerReleased;
            element.PointerEntered += Element_PointerEntered;
            element.PointerPressed += Element_PointerPressed;
            element.PointerExited += Element_PointerExited;
        }


        private void Element_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            if (IsPointerOver)
            {
                VisualStateManager.GoToState(element, "PointerOver", true);
            }
            else
            {
                VisualStateManager.GoToState(element, "Normal", true);
            }
            Click?.Invoke(element, new RoutedEventArgs());
        }

        private void Element_PointerEntered(object sender, PointerRoutedEventArgs e)
        {
            IsPointerOver = true;
            VisualStateManager.GoToState(element, "PointerOver", false);
        }

        private void Element_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            VisualStateManager.GoToState(element, "Pressed", true);
        }

        private void Element_PointerExited(object sender, PointerRoutedEventArgs e)
        {
            IsPointerOver = false;
            VisualStateManager.GoToState(element, "Normal", true);
        }
    }
}
