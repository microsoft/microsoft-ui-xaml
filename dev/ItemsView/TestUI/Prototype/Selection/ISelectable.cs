using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp.ItemsViewPrototype
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
