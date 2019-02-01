using Windows.UI.Xaml;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    internal sealed class DragAndDropPayload : DependencyObject
    {
        private static readonly DependencyProperty OperationProperty = DependencyProperty.Register(
            name: nameof(DragAndDropPayload.Operation),
            propertyType: typeof(DragAndDropOperation),
            ownerType: typeof(DragAndDropPayload),
            typeMetadata: null);

        // TODO : change Data to be a unique ID that can be used to verify correct data payload
        private static readonly DependencyProperty DataProperty = DependencyProperty.Register(
            name: nameof(DragAndDropPayload.Data),
            propertyType: typeof(object),
            ownerType: typeof(DragAndDropPayload),
            typeMetadata: null);

        public DragAndDropOperation Operation
        {
            get => (DragAndDropOperation)this.GetValue(DragAndDropPayload.OperationProperty);
            set => this.SetValue(DragAndDropPayload.OperationProperty, value);
        }

        public object Data
        {
            get => this.GetValue(DragAndDropPayload.DataProperty);
            set => this.SetValue(DragAndDropPayload.DataProperty, value);
        }
    }
}
