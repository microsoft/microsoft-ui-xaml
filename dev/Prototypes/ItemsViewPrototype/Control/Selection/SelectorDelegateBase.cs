using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace Microsoft.UI.Xaml.Controls.Primitives
{
    /// <summary>
    /// Base class for a selection handler that applies the "policy" to a model
    /// in response to user interaction driven by the Selector.
    /// </summary>
    public class SelectorDelegateBase : INotifyPropertyChanged
    {
        public SelectionModel Model
        {
            get
            {
                if (m_selectionModel == null)
                {
                    m_selectionModel = new SelectionModel();
                }

                return m_selectionModel;
            }

            set
            {
                m_selectionModel = value;
            }
        }

        #region Events

        public event PropertyChangedEventHandler PropertyChanged;

        protected virtual void RaisePropertyChanged([CallerMemberName]string propertyName = null)
        {
            if (this.PropertyChanged != null)
            {
                this.PropertyChanged.Invoke(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        #endregion

        #region Protected methods

        protected bool IsSelected(IndexPath index)
        {
            var selected = false;
            var isSelectedNullable = this.Model.IsSelectedAt(index);
            if (this.Model != null && isSelectedNullable != null)
            {
                selected = isSelectedNullable.Value;
            }

            return selected;
        }

        //protected bool IsPartiallySelected(IndexPath index)
        //{
        //    bool? result = null;
        //    if (this.Model != null)
        //    {
        //        result = this.Model.IsSelectedAt(index);
        //    }

        //    return result == null;
        //}

        protected virtual bool CanSelect(IndexPath index)
        {
            if (this.Model == null) return false;

            return true;
        }

        #endregion

        #region virtual methods

        //public virtual void Select(IndexPath index) { }

        //public virtual void Deselect(IndexPath index) { }

        //public virtual void SelectRange(IndexPath first, IndexPath last) { }

        //public virtual void DeselectRange(IndexPath first, IndexPath last) { }

        public virtual void SelectAll() { this.Model.SelectAll(); }
        public virtual void Clear() { this.Model.ClearSelection(); }

        /// <summary>
        /// Apply selection logic in response to interactions other than receiving focus.  For example,
        /// invoking an item via Narrator or after clicking with a mouse or lifting a finger.
        /// </summary>
        /// <param name="index"></param>
        /// <param name="ctrl"></param>
        /// <param name="shift"></param>
        public virtual void OnPrimaryInteractionAction(IndexPath index, bool ctrl, bool shift) { }

        /// <summary>
        /// Apply selection policy in response to receiving focus from non-pointer based input (i.e. keyboard)
        /// </summary>
        /// <param name="index"></param>
        /// <param name="ctrl"></param>
        /// <param name="shift"></param>
        public virtual void OnFocusedAction(IndexPath index, bool ctrl, bool shift) { }

        #endregion

        private SelectionModel m_selectionModel;

    }
}
