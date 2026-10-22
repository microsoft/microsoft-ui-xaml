using Microsoft.Data.Sqlite;

namespace WinUIProfiler.Engine.Query;

internal static class SqliteReadExecution
{
    internal static T Run<T>(SqliteConnection db, CancellationToken cancellationToken, Func<T> read)
    {
        cancellationToken.ThrowIfCancellationRequested();
        // An idle sqlite3_interrupt is not sticky. The progress callback also covers
        // cancellation between the last managed check and the next native step.
        SQLitePCL.delegate_progress progress = _ => cancellationToken.IsCancellationRequested ? 1 : 0;
        SQLitePCL.raw.sqlite3_progress_handler(db.Handle, 64, progress, null);
        try
        {
            using var registration = cancellationToken.Register(() => SQLitePCL.raw.sqlite3_interrupt(db.Handle));
            cancellationToken.ThrowIfCancellationRequested();
            try
            {
                var result = read();
                cancellationToken.ThrowIfCancellationRequested();
                return result;
            }
            catch (SqliteException error) when (error.SqliteErrorCode == 9 && cancellationToken.IsCancellationRequested)
            {
                throw new OperationCanceledException("SQLite read execution was cancelled.", error, cancellationToken);
            }
        }
        finally
        {
            SQLitePCL.raw.sqlite3_progress_handler(db.Handle, 0, null!, null);
            GC.KeepAlive(progress);
        }
    }
}
