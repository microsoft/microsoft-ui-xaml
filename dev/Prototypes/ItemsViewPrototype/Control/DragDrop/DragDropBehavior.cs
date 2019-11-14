using System.Windows.Input;
using Windows.ApplicationModel.DataTransfer;
using Windows.UI.Xaml;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public enum DragAndDropOperation
    {
        None,
        Copy,
        Move,
    }

    public static class DragAndDropBehavior
    {
        public static DragDropSourceInfo GetSource(UIElement element) => (DragDropSourceInfo)element.GetValue(DragAndDropBehavior.SourceProperty);
        public static void SetSource(UIElement element, DragDropSourceInfo value) => element.SetValue(DragAndDropBehavior.SourceProperty, value);

        public static DragDropTargetInfo GetTarget(UIElement element) => (DragDropTargetInfo)element.GetValue(DragAndDropBehavior.TargetProperty);
        public static void SetTarget(UIElement element, DragDropTargetInfo value) => element.SetValue(DragAndDropBehavior.TargetProperty, value);


        public static ICommand GetDropCommand(UIElement element) => (ICommand)element.GetValue(DragAndDropBehavior.DropCommandProperty);
        public static void SetDropCommand(UIElement element, ICommand value) => element.SetValue(DragAndDropBehavior.DropCommandProperty, value);

        private static void OnTargetPropertyChanged(DependencyObject obj, DependencyPropertyChangedEventArgs args)
        {
            var newValue = (DragDropTargetInfo)args.NewValue;
            var oldValue = (DragDropTargetInfo)args.OldValue;
            var element = (UIElement)obj;

            if (oldValue?.Operation != DragAndDropOperation.None)
            {
                element.AllowDrop = false;
                element.DragOver -= DragAndDropBehavior.OnDragOver;
                element.Drop -= DragAndDropBehavior.OnDrop;
            }

            if (newValue?.Operation != DragAndDropOperation.None)
            {
                element.AllowDrop = true;
                element.DragOver += DragAndDropBehavior.OnDragOver;
                element.Drop += DragAndDropBehavior.OnDrop;
            }
        }

        private static void OnDragOver(object sender, DragEventArgs e)
        {
            var payload = DragAndDropBehavior.GetTarget((UIElement)sender);
            var operation = payload.Operation.ToDataPackageOperation();
            var data = payload.Data;
            e.AcceptedOperation = operation;
        }

        private static void OnDrop(object sender, DragEventArgs e)
        {
            var element = (UIElement)sender;
            var payload = DragAndDropBehavior.GetTarget(element);
            var command = DragAndDropBehavior.GetDropCommand((UIElement)sender);
            command?.Execute(payload);
        }

        private static void OnSourcePropertyChanged(DependencyObject obj, DependencyPropertyChangedEventArgs args)
        {
            var newValue = (DragDropSourceInfo)args.NewValue;
            var oldValue = (DragDropSourceInfo)args.OldValue;
            var element = (UIElement)obj;

            if (oldValue?.AllowedOperations != DragAndDropOperation.None)
            {
                element.CanDrag = false;
                element.DragStarting -= DragAndDropBehavior.OnDragStarting;
            }

            if (newValue?.AllowedOperations != DragAndDropOperation.None)
            {
                element.CanDrag = true;
                element.DragStarting += DragAndDropBehavior.OnDragStarting;
            }
        }

        private static void OnDragStarting(UIElement sender, DragStartingEventArgs args)
        {
            var payload = DragAndDropBehavior.GetSource(sender);
            var operation = payload.AllowedOperations.ToDataPackageOperation();

            args.AllowedOperations = operation;
        }

        private static DataPackageOperation ToDataPackageOperation(this DragAndDropOperation operation)
        {
            switch (operation)
            {
                case DragAndDropOperation.Copy:
                    return DataPackageOperation.Copy;
                case DragAndDropOperation.Move:
                    return DataPackageOperation.Move;
                case DragAndDropOperation.Copy | DragAndDropOperation.Move:
                    return DataPackageOperation.Copy | DataPackageOperation.Move;
            }

            return DataPackageOperation.None;
        }

        private static DependencyProperty SourceProperty = DependencyProperty.RegisterAttached(
            name: "Source",
            propertyType: typeof(DragDropSourceInfo),
            ownerType: typeof(DragAndDropBehavior),
            defaultMetadata: new PropertyMetadata(default(DragDropSourceInfo), DragAndDropBehavior.OnSourcePropertyChanged));

        private static readonly DependencyProperty TargetProperty = DependencyProperty.RegisterAttached(
            name: "Target",
            propertyType: typeof(DragDropTargetInfo),
            ownerType: typeof(DragAndDropBehavior),
            defaultMetadata: new PropertyMetadata(default(DragDropTargetInfo), DragAndDropBehavior.OnTargetPropertyChanged));

        private static DependencyProperty DropCommandProperty = DependencyProperty.RegisterAttached(
            name: "DropCommand",
            propertyType: typeof(ICommand),
            ownerType: typeof(DragAndDropBehavior),
            defaultMetadata: null);
    }
}