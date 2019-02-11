using Windows.UI.Xaml;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public sealed class DragDropSourceInfo : DependencyObject
    {
        private static readonly DependencyProperty OperationProperty = DependencyProperty.Register(
           name: nameof(DragDropSourceInfo.AllowedOperations),
           propertyType: typeof(DragAndDropOperation),
           ownerType: typeof(DragDropSourceInfo),
           typeMetadata: null);

        public DragAndDropOperation AllowedOperations
        {
            get => (DragAndDropOperation)this.GetValue(DragDropSourceInfo.OperationProperty);
            set => this.SetValue(DragDropSourceInfo.OperationProperty, value);
        }
    }

    public sealed class DragDropTargetInfo : DependencyObject
    {
        private static readonly DependencyProperty OperationProperty = DependencyProperty.Register(
            name: nameof(DragDropTargetInfo.Operation),
            propertyType: typeof(DragAndDropOperation),
            ownerType: typeof(DragDropTargetInfo),
            typeMetadata: null);

        private static readonly DependencyProperty DataProperty = DependencyProperty.Register(
            name: nameof(DragDropTargetInfo.Data),
            propertyType: typeof(object),
            ownerType: typeof(DragDropTargetInfo),
            typeMetadata: null);

        public DragAndDropOperation Operation
        {
            get => (DragAndDropOperation)this.GetValue(DragDropTargetInfo.OperationProperty);
            set => this.SetValue(DragDropTargetInfo.OperationProperty, value);
        }

        public object Data
        {
            get => this.GetValue(DragDropTargetInfo.DataProperty);
            set => this.SetValue(DragDropTargetInfo.DataProperty, value);
        }
    }
}
