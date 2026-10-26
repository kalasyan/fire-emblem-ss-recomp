#ifndef PORT_PLATFORM_H
#define PORT_PLATFORM_H

/*
 * fire-emblem-ss-recomp: native (PC / Android) build glue for the game code.
 *
 * The port maps the GBA memory regions at their hardware addresses in the
 * host process (EWRAM 0x02000000, IWRAM 0x03000000, I/O 0x04000000, palette,
 * VRAM, OAM, and the player's ROM at 0x08000000), and the game itself is
 * loaded below 4 GiB. So the game's fixed addresses, the 32-bit pointers in
 * its ROM data and the pointers it keeps in 32-bit fields all stay valid.
 *
 * This header is shared by game code and port code: no game types here.
 */

#ifdef PLATFORM_PC

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char gPortEwram[256 * 0x400];
extern unsigned char gPortIwram[32 * 0x400];
extern unsigned char gPortIo[0x400];
extern unsigned char gPortPalram[0x400];
extern unsigned char gPortVram[96 * 0x400];
extern unsigned char gPortOam[0x400];
extern unsigned char gPortRom[32 * 0x400 * 0x400];
extern unsigned char gPortSram[64 * 0x400];

/* DMA: the port performs the transfer (or schedules it for HBlank/VBlank),
 * with full-width pointers (the source is often a stack variable). */
void Port_DmaSet(unsigned int channel, const void *src, void *dst, unsigned int control);

/* Called after the game writes save memory, so the host can persist it. */
void Port_SramWritten(void);

/* Called by AgbMain's loop after each main update: the C stack is always the
 * same there, so the host can take or restore save states. */
void Port_HostFrameBoundary(void);

/* Testing aid: the chapter a new game starts on, from the FE8_WARP environment
 * variable (a chapter index), or 0. */
int Port_NewGameChapter(void);

/* Sound (port/game/m4a_asm.c): the next 16 samples DMA 1/2 would feed to
 * FIFO A/B. Returns 0 when there is nothing to play. */
int Port_SoundFifoBlock(const signed char **fifoA, const signed char **fifoB);

/* The words at the top of IWRAM the BIOS uses (0x03007FF0...). Natively they
 * hold full-width pointers, so they are ordinary variables. */
extern void *gPortSoundInfoPtr;
extern unsigned short gPortIntrCheck;
extern void *gPortIntrVector;

void Port_Log(const char *fmt, ...);

/* Rumble (port/src/rumble.c): a hit's damage shows up now, in a battle scene
 * or on the map. */
void Port_RumbleHit(int damage, int critical, int lethal);

/* The message to read for a message index: in the European version the
 * message table holds every language in turn (port/game/eu_language.c). */
int Port_LocalizeMsg(int index);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_PC */

#endif /* PORT_PLATFORM_H */
