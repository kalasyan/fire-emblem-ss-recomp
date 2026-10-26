// SPDX-License-Identifier: GPL-3.0-or-later
// fire-emblem-ss-recomp — save states.
//
// The machine state is:
//   - the GBA memory regions (EWRAM, IWRAM, I/O, palettes, VRAM, OAM). Save
//     memory (SRAM) is not included: loading a state never touches the
//     player's real save file;
//   - every game global: include/port/native.h puts the game's data and bss
//     in the sections fe8_data / fe8_bss, except EWRAM_DATA / IWRAM_DATA
//     variables, which keep their own sections (ewram_data / iwram_data);
//   - the port's own hardware state (PORT_STATE: DMA channels, sound chip).
//
// States are only taken and restored at a frame boundary (see
// Port_HostFrameBoundary), where the C stack is always the same, so nothing
// on it needs saving. The game image, the GBA regions and the ROM are at
// fixed addresses, so pointers in the state stay valid across runs of the
// same build.
#include "port.h"
#include "port_assets.h"

#define STATE_MAGIC "FE8STATE"
#define STATE_VERSION 2

// Bounds of the sections, made by the linker.
extern unsigned char __start_fe8_data[];
extern unsigned char __stop_fe8_data[];
extern unsigned char __start_fe8_bss[];
extern unsigned char __stop_fe8_bss[];
extern unsigned char __start_fe8_state[];
extern unsigned char __stop_fe8_state[];
extern unsigned char __start_ewram_data[] __attribute__((weak));
extern unsigned char __stop_ewram_data[] __attribute__((weak));
extern unsigned char __start_iwram_data[] __attribute__((weak));
extern unsigned char __stop_iwram_data[] __attribute__((weak));

typedef struct Region {
    unsigned char* data;
    size_t size;
} Region;

typedef struct StateHeader {
    char magic[8];
    uint32_t version;
    uint32_t headerSize;
    char region[4];
    uint32_t pad;
    // Layout of this build: a state from another build would restore
    // variables into the wrong places.
    uint64_t layout[6];
    uint64_t frameCount;
} StateHeader;

static size_t Regions(Region* out)
{
    size_t n = 0;
    out[n++] = (Region){ gPortEwram, sizeof(gPortEwram) };
    out[n++] = (Region){ gPortIwram, sizeof(gPortIwram) };
    out[n++] = (Region){ gPortIo, sizeof(gPortIo) };
    out[n++] = (Region){ gPortPalram, sizeof(gPortPalram) };
    out[n++] = (Region){ gPortVram, sizeof(gPortVram) };
    out[n++] = (Region){ gPortOam, sizeof(gPortOam) };
    out[n++] = (Region){ __start_fe8_data, (size_t)(__stop_fe8_data - __start_fe8_data) };
    out[n++] = (Region){ __start_fe8_bss, (size_t)(__stop_fe8_bss - __start_fe8_bss) };
    out[n++] = (Region){ __start_fe8_state, (size_t)(__stop_fe8_state - __start_fe8_state) };
    out[n++] = (Region){ __start_ewram_data, (size_t)(__stop_ewram_data - __start_ewram_data) };
    out[n++] = (Region){ __start_iwram_data, (size_t)(__stop_iwram_data - __start_iwram_data) };
    return n;
}

static void FillHeader(StateHeader* h)
{
    memset(h, 0, sizeof(*h));
    memcpy(h->magic, STATE_MAGIC, 8);
    h->version = STATE_VERSION;
    h->headerSize = sizeof(*h);
    strncpy(h->region, Port_RomRegionName(), sizeof(h->region));
    h->layout[0] = (uintptr_t)__start_fe8_data;
    h->layout[1] = (uintptr_t)__stop_fe8_data;
    h->layout[2] = (uintptr_t)__start_fe8_bss;
    h->layout[3] = (uintptr_t)__stop_fe8_bss;
    h->layout[4] = (uintptr_t)__start_fe8_state;
    h->layout[5] = (uintptr_t)&Port_StateSave;
}

size_t Port_StateSize(void)
{
    Region r[16];
    size_t n = Regions(r);
    size_t total = sizeof(StateHeader);

    for (size_t i = 0; i < n; i++)
        total += r[i].size;
    return total;
}

void Port_StateSave(uint8_t* buf)
{
    Region r[16];
    size_t n = Regions(r);
    StateHeader h;

    FillHeader(&h);
    h.frameCount = Port_FrameCount();
    memcpy(buf, &h, sizeof(h));
    buf += sizeof(h);
    for (size_t i = 0; i < n; i++) {
        Port_RawCopy(buf, r[i].data, r[i].size);
        buf += r[i].size;
    }
}

PortStateResult Port_StateLoad(const uint8_t* buf, size_t size)
{
    Region r[16];
    size_t n = Regions(r);
    StateHeader want;
    StateHeader h;

    if (size != Port_StateSize())
        return size < sizeof(h) ? PORT_STATE_BAD_FILE : PORT_STATE_OTHER_BUILD;
    memcpy(&h, buf, sizeof(h));
    if (memcmp(h.magic, STATE_MAGIC, 8) != 0)
        return PORT_STATE_BAD_FILE;
    FillHeader(&want);
    if (h.version != want.version || h.headerSize != want.headerSize
        || memcmp(h.region, want.region, sizeof(h.region)) != 0
        || memcmp(h.layout, want.layout, sizeof(h.layout)) != 0)
        return PORT_STATE_OTHER_BUILD;
    buf += sizeof(h);
    for (size_t i = 0; i < n; i++) {
        Port_RawCopy(r[i].data, buf, r[i].size);
        buf += r[i].size;
    }
    Port_RumbleReset();
    return PORT_STATE_OK;
}

const char* Port_StateResultText(PortStateResult result)
{
    switch (result) {
    case PORT_STATE_OK:
        return "OK";
    case PORT_STATE_BAD_FILE:
        return "not a save state";
    case PORT_STATE_OTHER_BUILD:
        return "made by another version of the game";
    }
    return "?";
}
