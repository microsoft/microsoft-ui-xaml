// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class IRawData
{
public:
    virtual ~IRawData() = default;

    virtual const uint8_t* GetData() const = 0;
    virtual size_t GetSize() const = 0;
};

class RawData final : public IRawData
{
public:

    RawData() = default;
    RawData(const RawData&) = delete;
    RawData(RawData&&) = default;

    void Allocate(size_t size)
    {
        m_size = size;
        m_pData = wil::make_unique_failfast<uint8_t[]>(m_size);
    }

    void Release()
    {
        m_pData = nullptr;
    }

    uint8_t* GetData() { return m_pData.get(); }
    const uint8_t* GetData() const override { return m_pData.get(); }
    size_t GetSize() const override { return m_size; }

private:

    RawData& operator=(const RawData&);

    wistd::unique_ptr<uint8_t[]> m_pData;
    size_t m_size = 0;
};
