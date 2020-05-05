using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;

namespace ItemsRepeaterExperiments.AttachedBehaviors
{
    public class SelectionBehaviorItemPeer : ItemsRepeater
    {
        private readonly DependencyObject item;

        public SelectionBehaviorItemPeer(DependencyObject item)
        {
            this.item = item;
        }

        public void AddToSelection()
        {
            SelectionBehavior.SetIsSelected(item, true);
        }

        public void RemoveFromSelection()
        {
            SelectionBehavior.SetIsSelected(item, false);
        }

        public void Select()
        {
            SelectionBehavior.SetIsSelected(item, true);
        }

        public bool IsSelected
        {
            get
            {
                var isSelected = SelectionBehavior.GetIsSelected(item);
                return isSelected.HasValue && isSelected.Value;
            }

            set
            {
                SelectionBehavior.SetIsSelected(item, value);
            }
        }

        public IRawElementProviderSimple SelectionContainer => throw new NotImplementedException();
    }
}
