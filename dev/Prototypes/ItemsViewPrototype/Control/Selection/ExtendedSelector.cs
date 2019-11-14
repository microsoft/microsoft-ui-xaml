namespace Microsoft.UI.Xaml.Controls.Primitives
{
    /// <summary>
    /// Implements the policy for an extended selection mode.  Extended selection normally behaves like single
    /// selection, but switches to a multiple selection mode when using the Ctrl modifier key or to a range
    /// selection mode when using the Shift modifier key.  This mimics File Explorer selection behavior.
    /// </summary>
    internal class ExtendedSelector : SelectorDelegateBase
    {
        private bool m_enableRangeSelection = true;     // Note: different default than MultipleSelector.
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
                ExtendSelectionRange(index, ctrl);
            }
            else if (ctrl) // different than just Multiple selection
            {
                Toggle(index);
            }
            else // also different
            {
                this.SingleSelect(index);
            }
        }

        public override void OnFocusedAction(IndexPath index, bool ctrl, bool shift)
        {
            if (shift && this.IsRangeSelectionEnabled)
            {
                // if the user is holding Shift, extend the selection range on keyboard activation
                ExtendSelectionRange(index, ctrl);
            }
            else if (ctrl) // different than just Multiple selection
            {
                // Do not modify the selection if the user is holding ctrl and keyboarding.
            }
            else // also different
            {
                // If the user is not holding Shift or Ctrl, select just the keyboard activated item.
                this.SingleSelect(index);
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

        private bool ExtendSelectionRange(IndexPath index, bool ctrl)
        {
            if (!this.CanSelect(index)) return false;

            var anchorIndex = this.Model.AnchorIndex;

            if (base.IsSelected(anchorIndex))
            {
                if (!ctrl)
                    this.Model.ClearSelection();

                this.Model.AnchorIndex = anchorIndex;
                this.Model.SelectRangeFromAnchorTo(index);
            }
            else
            {
                // If the anchor item is deselected then behavior depends on whether Ctrl is used.
                // Ctrl+Shift deselects the range from the anchor.
                // Shift only will clear selection and select from the anchor.
                if (ctrl)
                {
                    // User scenario:
                    //  1. Shift + arrow to select multiple items
                    //  2. Ctrl + arrow to go to multi-select mode and move focus without affecting selection
                    //  3. Ctrl + space to deselect an item in the middle of the selected range and also establish a new anchor
                    //  4. Ctrl + Shift + arrow to multi-DEselect from the anchor
                    this.Model.AnchorIndex = anchorIndex;
                    this.Model.DeselectRangeFromAnchorTo(index);
                }
                else
                {
                    // This should deselect everything else.  
                    // We're taking a heavy-handed approach here by dropping everything
                    this.Model.ClearSelection();
                    this.Model.AnchorIndex = anchorIndex;
                    this.Model.SelectRangeFromAnchorTo(index);
                }
            }

            return true;
        }

        private bool SingleSelect(IndexPath index)
        {
            if (!this.CanSelect(index)) return false;

            var wasSingleSelectEnabled = this.Model.SingleSelect;

            this.Model.SingleSelect = true;

            this.Model.SelectAt(index);

            this.Model.SingleSelect = wasSingleSelectEnabled;

            return true;
        }
    }
}
