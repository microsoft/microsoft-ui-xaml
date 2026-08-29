// netstandard2.0 does not ship System.Runtime.CompilerServices.IsExternalInit,
// which the compiler requires to emit `init`-only setters (used by the record
// types in this assembly). Defining it here lets us use init/records while still
// targeting netstandard2.0. It is intentionally internal so it doesn't leak.
namespace System.Runtime.CompilerServices
{
    internal static class IsExternalInit
    {
    }
}
