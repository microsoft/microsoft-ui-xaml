namespace Microsoft.UI.Xaml.Controls.Primitives
{
    /// <summary>
    /// Implements the policy for multiple selection mode.  Individual items can be toggled between a
    /// selected / unselected state.  Selecting one item doesn't cause others to be automatically de-selected.
    /// Also supports a range selection mode that when enabled can be toggled using Shift.  
    /// </summary>
    /// <remarks>
    /// Currently assumes the data is 1 level and isn't coded to work on multi-level data.
    /// </remarks>
    internal class MultipleSelector : SelectorDelegateBase
    {
        private bool m_enableRangeSelection = false;

        // This isn't currently exposed as an option since we're limiting the behavior to the SelectionMode enum.
        // Retaining here since it was supported in the Office's list selection behavior (ToggleSelectionBehavior)
        // but unclear what scenario it supports.
        public bool IsRangeSelectionEnabled
        {
            get
            {
                return m_enableRangeSelection;
            }

            set
            {
                if (m_enableRangeSelection != value)
                {
                    m_enableRangeSelection = value;
                    this.RaisePropertyChanged();
                }
            }
        }

        public override void OnPrimaryInteractionAction(IndexPath index, bool ctrl, bool shift)
        {
            if (shift && this.IsRangeSelectionEnabled)
            {
                ExtendSelectionRange(index);
            }
            else
            {
                Toggle(index);
            }
        }

        public override void OnFocusedAction(IndexPath index, bool ctrl, bool shift)
        {
            if (shift && this.IsRangeSelectionEnabled)
            {
                // if the user is holding Shift, extend the selection range on keyboard activation
                ExtendSelectionRange(index);
            }
            else
            {
                // Do not modify the selection if the user is keyboarding.
            }
        }

        private bool Toggle(IndexPath index)
        {
            if (!this.CanSelect(index)) return false;

            if (!base.IsSelected(index))
            {
                this.Model.SelectAt(index);
            }
            else
            {
                this.Model.DeselectAt(index);
            }

            this.Model.AnchorIndex = index;
            return true;
        }

        /// <summary>
        /// This behavior is enabled in Office's list control via the ToggleSelectionBehavior, but its unclear who needed it.
        /// It doesn't appear to be used anywhere in the Office codebase.  Besides, the behavior sits between what Multiple and
        /// Extended provide so why not just use one of those?  Plan to delete this in the future.
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        private bool ExtendSelectionRange(IndexPath index)
        {
            if (!this.CanSelect(index)) return false;

            var wasSingleSelectEnabled = this.Model.SingleSelect;
            this.Model.SingleSelect = false;

            var anchorIndex = this.Model.AnchorIndex;
            if (base.IsSelected(anchorIndex))
            {
                this.Model.SelectRangeFromAnchorTo(index);
            }
            else
            {
                // This should deselect the range from the anchor.
                this.Model.DeselectRangeFromAnchorTo(index);
            }

            this.Model.SingleSelect = wasSingleSelectEnabled;

            return true;
        }
    }
}