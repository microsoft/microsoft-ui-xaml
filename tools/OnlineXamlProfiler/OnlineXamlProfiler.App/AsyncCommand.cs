// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Windows.Input;

namespace OnlineXamlProfiler.App;

public sealed class AsyncCommand(Func<Task> execute, Func<bool> canExecute, Action<Exception> onError) : ICommand
{
    private bool _executing;
    public event EventHandler? CanExecuteChanged;
    public bool CanExecute(object? parameter) => !_executing && canExecute();

    public async void Execute(object? parameter)
    {
        if (!CanExecute(parameter))
        {
            return;
        }
        _executing = true;
        Notify();
        try
        {
            await execute();
        }
        catch (Exception exception)
        {
            onError(exception);
        }
        finally
        {
            _executing = false;
            Notify();
        }
    }

    public void Notify() => CanExecuteChanged?.Invoke(this, EventArgs.Empty);
}
