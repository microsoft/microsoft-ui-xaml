// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// A StreamOffsetToken represents
// an abstraction away from the XamlOptimizedNodeStream. When a CustomWriter is interacting
// with a ObjectWriter to defer creation of a XAML object it will ask for a StreamOffsetToken
// when it wants to get a handle that will allow the type to create this object
// during runtime. Today it will simply be an index into a nodestream, tomorrow it will likely
// be translated into an index in the new XBF bytecode or something.
class StreamOffsetToken
{
public:
    StreamOffsetToken()
        : m_index(0)
    {}

    explicit StreamOffsetToken(unsigned int index)
        : m_index(index)
    {}

    // This exposes a little bit of the implementation of StreamOffsetToken, but that's
    // a reasonable tradeoff for keeping this an easily-copyable POD object. This should
    // only be accessed by CustomWriterRuntimeObjectCreator.
    unsigned int GetIndex() const { return m_index; }

    bool operator<(_In_ const StreamOffsetToken other) const { return m_index < other.m_index; }

private:
    unsigned int m_index;
};