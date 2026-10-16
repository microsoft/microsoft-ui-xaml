// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>
#include <guiddef.h>

// Original protocol, independent of the Microsoft sample implementation.
// {064CBEC3-A0A1-4765-BCF5-9B9F7EC02ACA}
static const GUID MTT_DEVICE_INTERFACE =
{ 0x064cbec3, 0xa0a1, 0x4765, { 0xbc, 0xf5, 0x9b, 0x9f, 0x7e, 0xc0, 0x2a, 0xca } };

#define MTT_HARDWARE_ID L"Root\\MonitorTopologyTest"
#define MTT_PROTOCOL_VERSION 1u
#define MTT_MAX_MONITORS 4u
#define MTT_MIN_DIMENSION 640u
#define MTT_MAX_DIMENSION 3840u
#define MTT_MIN_REFRESH_HZ 24u
#define MTT_MAX_REFRESH_HZ 120u

// Private device type 0x8000, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA.
#define IOCTL_MTT_GET_STATE 0x8000e000u
#define IOCTL_MTT_ADD_MONITOR 0x8000e004u
#define IOCTL_MTT_REMOVE_MONITOR 0x8000e008u
#define IOCTL_MTT_CLEAR_MONITORS 0x8000e00cu

#pragma pack(push, 4)
struct MTT_REQUEST
{
    uint32_t Size;
    uint32_t Version;
};

struct MTT_ADD_REQUEST
{
    uint32_t Size;
    uint32_t Version;
    uint32_t Id;
    uint32_t Width;
    uint32_t Height;
    uint32_t RefreshNumerator;
    uint32_t RefreshDenominator;
};

struct MTT_REMOVE_REQUEST
{
    uint32_t Size;
    uint32_t Version;
    uint32_t Id;
};

struct MTT_MONITOR
{
    uint32_t Id;
    uint32_t Width;
    uint32_t Height;
    uint32_t RefreshNumerator;
    uint32_t RefreshDenominator;
    uint32_t AdapterLuidLow;
    int32_t AdapterLuidHigh;
    uint32_t TargetId;
};

struct MTT_STATE
{
    uint32_t Size;
    uint32_t Version;
    uint32_t Ready;
    uint32_t MaxMonitors;
    uint32_t MonitorCount;
    uint32_t Generation;
    MTT_MONITOR Monitors[MTT_MAX_MONITORS];
};
#pragma pack(pop)

static_assert(sizeof(MTT_REQUEST) == 8);
static_assert(sizeof(MTT_ADD_REQUEST) == 28);
static_assert(sizeof(MTT_REMOVE_REQUEST) == 12);
static_assert(sizeof(MTT_MONITOR) == 32);
static_assert(sizeof(MTT_STATE) == 152);
