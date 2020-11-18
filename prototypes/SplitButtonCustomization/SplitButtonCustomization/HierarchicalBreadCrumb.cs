using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace SplitButtonCustomization
{
    class HierarchicalBreadCrumb : ItemsControl
    {
        // P1s 
        public HierarchicalBreadCrumb() { }

        // Gets the current data item associated with the leaf BreadcrumbItem in  
        // the Breadcrumb. 
        public object CurrentItem { get; }

        // Object shown for the root of the Breadcrumb.  
        public object Header { get; set; }

        // Allows customization of the root Header representation. 
        public DataTemplate HeaderTemplate { get; set; }

        // Gets or sets a path to a value on the source object to serve as  
        // the BreadcrumbItem.Header. 
        public string HeaderMemberPath { get; set; }

        // Gets or sets a path to a value on the source object to serve as  
        // the BreadcrumbItem.DropDownHeader. 
        public string HierarchicalMemberPath { get; set; }

        // Gets or sets a path to a value on the source object to serve as  
        // the BreadcrumbItem 's ItemsSource collection. 
        public string HierarchicalItemsSource { get; set; }

        // Raised when the CurrentItem property changed. 

        public event TypedEventHandler<HierarchicalBreadCrumb, object> CurrentItemChanged;
    }
}
