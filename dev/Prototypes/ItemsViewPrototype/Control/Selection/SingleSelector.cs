namespace Microsoft.UI.Xaml.Controls.Primitives
{
    internal class SingleSelector : SelectorDelegateBase
    {
        public SingleSelector()
        {
            FollowFocus = true;
        }

        public bool FollowFocus { get; set; }

        public override void OnPrimaryInteractionAction(IndexPath index, bool ctrl, bool shift)
        {
            this.Model.SingleSelect = true;
            if (!ctrl) this.Model.SelectAt(index);
            else if (!(base.IsSelected(index)))
                this.Model.SelectAt(index);
            else
                this.Model.DeselectAt(index);
        }

        public override void OnFocusedAction(IndexPath index, bool ctrl, bool shift)
        {
            this.Model.SingleSelect = true;
            if (!ctrl && FollowFocus) this.Model.SelectAt(index);
        }
    }
}
