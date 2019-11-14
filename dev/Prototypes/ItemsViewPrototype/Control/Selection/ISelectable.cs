namespace Microsoft.UI.Xaml.Controls.Primitives
{
    /// <summary>
    /// Prototyping... The minimal set of interactions that a given UIElement needs to support to work
    /// with a selection service.  Open issue: Does this need a CanSelect or an IsSelectionEnabled to 
    /// toggle between displaying a visual to indicate the element can be selected (e.g. show a checkbox)?  Seems
    /// like a UX policy decision for a specific list implementation.
    /// </summary>
    /// <remarks>
    /// Interfaces don't version well from a platform API perspective so we'd likely provide a concrete base
    /// type to use as a starting point similar to the Windows.UI.Xaml.Controls.Primitives.SelectorItem.
    /// </remarks>
    public interface ISelectable
    {
        /// <summary>
        /// The index that is used to identify what is selected in the SelectionModel.  In a situation where
        /// UIElements are being recycled this index needs to be kept up-to-date to represent the item in the data.
        /// In a non-recycling situation it just needs to be set once.  
        /// Two separate elements with the same SelectionIndex and registered to the same selection behavior/model 
        /// would "share" selection state.  When that index is selected both will be notified.  It's not the
        /// norm, but may be useful under certain UX designs.
        /// </summary>
        int ModelIndex { get; set; }

        IndexPath GetIndexPath();

        /// <summary>
        /// Property to toggle the selected state or not.
        /// </summary>
        bool IsSelected { get; set; }

        /// <summary>
        /// Register the element with the Selector to be notified when changes to its 
        /// selection state occur (e.g. programmatically or via user input).
        /// </summary>
        /// <param name="selector"></param>
        void SelectorAttached(Selector selector);
        void SelectorDetached(Selector selector);
    }
}
