// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Cli;

internal static class Program
{
    private static int Main(string[] args)
    {
        using var cancellation = new CancellationTokenSource();
        ConsoleCancelEventHandler handler = (_, e) =>
        {
            e.Cancel = true;
            cancellation.Cancel();
        };
        Console.CancelKeyPress += handler;
        try
        {
            return CliApplication.Run(args, Console.Out, Console.Error, cancellation.Token);
        }
        finally
        {
            Console.CancelKeyPress -= handler;
        }
    }
}
