// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class OfferableMemory final
{
public:
    explicit OfferableMemory(uint32_t size);
    ~OfferableMemory();

    void* GetBuffer() { return m_buffer; }
    const void* GetBuffer() const { return m_buffer; }
    uint32_t GetSize() const { return m_size; }
    uint32_t GetRoundedSize() const { return m_roundedsize; }

    // Offers up memory to the operation system which can later be reclaimed.
    void Offer();

    // Returns true if the resources were discarded by the OS, false otherwise.
    bool Reclaim();

private:
    OfferableMemory(const OfferableMemory&) = delete;
    OfferableMemory& operator=(const OfferableMemory&) = delete;

    void* m_buffer = nullptr;
    uint32_t m_size = 0;
    uint32_t m_roundedsize = 0;
    bool m_offered = false;
};
