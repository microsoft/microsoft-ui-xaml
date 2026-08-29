// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Windows.Threading;

namespace System.Threading.Tasks
{
    internal static class TaskExtensions
    {
        public static T GetResultWhileDoingEvents<T>(this Task<T> task)
        {
            while (!task.IsCompleted && !task.IsCanceled && !task.IsFaulted)
            {
                Dispatcher.CurrentDispatcher.DoEvents();
            }

            return task.Result;
        }

        private static async Task<int> ExecuteTaskWithResult(Task task)
        {
            await task;
            return 0;
        }

        public static void WaitWhileDoingEvents(this Task task)
        {
            var taskWithResult = ExecuteTaskWithResult(task);
            taskWithResult.GetResultWhileDoingEvents();
        }
    }
}
