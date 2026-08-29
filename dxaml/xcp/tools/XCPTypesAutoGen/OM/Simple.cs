// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
namespace OM
{
    public enum SimpleTypeKind
    {
        Value,
        ValueByRef,
        BigImmutable,
        BigMutable
    }

    public enum SimplePropertyStorage
    {
        Field,
        Sparse,
        SparseGroup
    }
}
