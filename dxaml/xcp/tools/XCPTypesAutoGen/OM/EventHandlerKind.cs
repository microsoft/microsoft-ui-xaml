// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
namespace OM
{
    public enum EventHandlerKind
    {
        Concrete = 0, // XEventHandler
        TypedSenderAndArgs = 1, // TypedEventHandler<T, U>
        TypedArgs = 2 // EventHandler<T>
    }
}
