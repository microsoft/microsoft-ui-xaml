namespace Microsoft.UI.Xaml.Controls.Primitives
{
    interface IModelIndex
    {
        /// <summary>
        /// The index that is used to identify the backing model in a flat list.
        /// </summary>
        /// <remarks>
        /// It's useful for operations such as determining what is selected in the SelectionModel or
        /// creating a staggered animation.
        /// In a situation where UIElements are being recycled this index needs to be kept up-to-date 
        /// to represent the item in the data.  In a non-recycling situation it just needs to be set once.  
        /// Two separate elements with the same SelectionIndex and registered to the same selection 
        /// behavior/model would "share" selection state.  When that index is selected both will be 
        /// notified.  It's not the norm, but may be useful under certain UX designs.
        /// </remarks>
        int ModelIndex { get; set; }

        IndexPath GetIndexPath();
    }
}
