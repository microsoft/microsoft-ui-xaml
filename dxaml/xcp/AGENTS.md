
Most code in this repo uses the `IFC_RETURN` macro to record some information about an HRESULT
and return if it's a failure.

- Prefer `IFC_RETURN` where applicable.  Covert `IFC` to `IFC_RETURN` opportunistically, only if you're already changing that function.
- Instead of using `return <expr>;` to return an HRESULT, prefer to `IFC_RETURN(<expr>)` first and then `return S_OK;`.
