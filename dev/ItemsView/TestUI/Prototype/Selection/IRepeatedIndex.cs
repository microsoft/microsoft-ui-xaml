using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    interface IRepeatedIndex
    {
        /// <summary>
        /// The index that is used to identify what is selected in the SelectionModel.  In a situation where
        /// UIElements are being recycled this index needs to be kept up-to-date to represent the item in the data.
        /// In a non-recycling situation it just needs to be set once.  
        /// Two separate elements with the same SelectionIndex and registered to the same selection behavior/model 
        /// would "share" selection state.  When that index is selected both will be notified.  It's not the
        /// norm, but may be useful under certain UX designs.
        /// </summary>
        int RepeatedIndex { get; set; }

        IndexPath GetIndexPath();
    }
}
