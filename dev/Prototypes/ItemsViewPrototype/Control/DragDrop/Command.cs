using System;
using System.Windows.Input;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    internal sealed class Command : ICommand
    {
        public event EventHandler CanExecuteChanged;

        public bool CanExecuteValue
        {
            get => this.canExecuteValueBackingField;
            set
            {
                if (this.canExecuteValueBackingField != value)
                {
                    this.canExecuteValueBackingField = value;
                    this.CanExecuteChanged?.Invoke(this, EventArgs.Empty);
                }
            }
        }

        public Action<object> ExecuteHandler { get; set; }

        public bool CanExecute(object parameter) => this.CanExecuteValue;

        public void Execute(object parameter)
        {
            if (this.CanExecuteValue)
            {
                this.ExecuteHandler?.Invoke(parameter);
            }
        }

        private bool canExecuteValueBackingField = true;

    }
}