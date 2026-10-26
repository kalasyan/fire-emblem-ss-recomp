// SPDX-License-Identifier: GPL-3.0-or-later
// fire-emblem-ss-recomp — rumble from battle hits.
//
// The game reports every hit as its damage shows up: when a battle scene
// starts draining an HP bar (src/banim-efxhpbar.c) and when a hit lands on
// the map with battle scenes off (src/mapanim_spellassoc.c). Each hit starts
// a short rumble "effect", stronger for bigger hits, critical hits and the
// blow that defeats a unit; the strongest one sets the motors. The host gets
// both the motor levels (gamepads, refreshed every frame) and, for simple
// vibrators like a phone's, a pulse when a new vibration starts.
//
// FE8_RUMBLE_LOG=1 logs every hit (used to test it in fe8_headless).
#include "port.h"

#include <stdlib.h>

#define MAX_EFFECTS 4
#define MIN_F(a, b) ((a) < (b) ? (a) : (b))
#define MAX_F(a, b) ((a) > (b) ? (a) : (b))

typedef struct Effect {
    float low, high; // motor strength 0..1
    int frames;      // frames left
    int total;
} Effect;

static Effect sEffects[MAX_EFFECTS];
static int sLog = -1;
static int sPulseFramesLeft;
static float sPulseStrength;

static void StartEffect(float low, float high, int frames)
{
    int slot = 0;

    // Replace the weakest effect when they're all in use.
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (sEffects[i].frames <= 0) {
            slot = i;
            break;
        }
        if (sEffects[i].low + sEffects[i].high < sEffects[slot].low + sEffects[slot].high)
            slot = i;
    }
    sEffects[slot] = (Effect){ low, high, frames, frames };
}

void Port_RumbleHit(int damage, int critical, int lethal)
{
    if (damage <= 0)
        return;
    if (sLog < 0) {
        const char* env = getenv("FE8_RUMBLE_LOG");
        sLog = env && *env && *env != '0';
    }
    if (sLog)
        Port_Log("rumble: frame %llu hit %d%s%s", (unsigned long long)Port_FrameCount(), damage,
                 critical ? " critical" : "", lethal ? " lethal" : "");

    // Most hits do 1-20 damage.
    const float k = MIN_F(damage, 30) / 30.0f;
    if (lethal)
        StartEffect(1.0f, 0.8f, 40);
    else if (critical)
        StartEffect(0.9f, 0.7f, 24);
    else
        StartEffect(0.35f + 0.45f * k, 0.25f + 0.35f * k, 12);
}

void Port_RumbleFrame(void)
{
    float low = 0.0f, high = 0.0f;
    int longest = 0;

    for (int i = 0; i < MAX_EFFECTS; i++) {
        Effect* fx = &sEffects[i];
        if (fx->frames <= 0)
            continue;
        // Full strength, then fades out over the last 40%.
        const float fade = MIN_F(1.0f, (float)fx->frames / (0.4f * (float)fx->total));
        low = MAX_F(low, fx->low * fade);
        high = MAX_F(high, fx->high * fade);
        if (fx->frames > longest)
            longest = fx->frames;
        fx->frames--;
    }

    // Pulses for simple vibrators: a new one when the rumble gets clearly
    // stronger or the previous one ran out while it still goes on.
    float pulse = 0.0f;
    int pulseMs = 0;
    const float level = MAX_F(low, high);
    if (sPulseFramesLeft > 0)
        sPulseFramesLeft--;
    if (level > 0.05f && (sPulseFramesLeft <= 0 || level > sPulseStrength + 0.15f)) {
        pulse = level;
        pulseMs = longest * 1000 / 60;
        if (pulseMs < 40)
            pulseMs = 40;
        sPulseStrength = level;
        sPulseFramesLeft = longest;
    } else if (sPulseFramesLeft <= 0) {
        sPulseStrength = 0.0f;
    }

    Port_HostRumble(low, high, pulse, pulseMs);
}

void Port_RumbleReset(void)
{
    for (int i = 0; i < MAX_EFFECTS; i++)
        sEffects[i].frames = 0;
}
