// The game side of the second screen (include/port/screen2.h): a snapshot of
// the battle map, the units, the unit under the cursor and the battle
// forecast, plus the full map picture and the units' standing sprites, drawn
// from the chapter's own tileset and palettes.
//
// Everything is read, never written, except for the cursor requests, which
// are applied between two frames the same way the game moves the cursor to a
// unit (TrySetCursorOn).
#include "global.h"
#include "proc.h"
#include "bm.h"
#include "bmunit.h"
#include "bmmap.h"
#include "bmitem.h"
#include "bmbattle.h"
#include "bmlib.h"
#include "bksel.h"
#include "chapterdata.h"
#include "playerphase.h"
#include "unit_icon_data.h"
#include "port/screen2.h"
#include "port/widescreen.h"

// The snapshot and the picture caches are not machine state: keep them out of
// save states and soft resets (include/port/native.h).
#pragma clang section bss = "" data = ""

extern struct ProcCmd gProc_BMapMain[];
extern UnitIconWait unit_icon_wait_table[];
extern u16 gPal_MapSprite[];
extern u16 CONST_DATA gUnknown_0880D374[];

const u16 *Port_BmTilesetConfig(void);
int Port_StatusNameTextId(int status);

static struct PortS2State sState;

// ---- text -------------------------------------------------------------------

// Latin-1 letters without their accents (the second screen's font is ASCII).
static char PlainChar(u8 c)
{
    static const char hi[] =
        "AAAAAAACEEEEIIII" // C0
        "DNOOOOOxOUUUUYPs" // D0
        "aaaaaaaceeeeiiii" // E0
        "dnooooo/ouuuuypy"; // F0

    if (c >= 0x20 && c < 0x7F)
        return c;
    if (c >= 0xC0)
        return hi[c - 0xC0];
    return 0;
}

// Message `id` as plain ASCII into `out` (control codes dropped).
static void CopyMsg(char *out, int size, int id)
{
    // Decoded messages, by (localized) id: the same names come up every frame.
    static struct { int id; char text[32]; } cache[256];
    static char buf[0x1000];
    const u8 *s;
    int n = 0, key, slot;

    out[0] = 0;
    if (id <= 0)
        return;
    key = Port_LocalizeMsg(id);
    slot = key & 0xFF;
    if (cache[slot].id == key && size <= (int)sizeof(cache[slot].text)) {
        memcpy(out, cache[slot].text, size);
        out[size - 1] = 0;
        return;
    }
    GetStringFromIndexInBuffer(id, buf);
    for (s = (const u8 *)buf; *s && n < size - 1; s++) {
        char c = PlainChar(*s);
        if (*s < 0x20 && n > 0 && out[n - 1] != ' ')
            c = ' '; // line breaks
        if (c)
            out[n++] = c;
    }
    while (n > 0 && out[n - 1] == ' ')
        n--;
    out[n] = 0;
    if (size <= (int)sizeof(cache[slot].text)) {
        cache[slot].id = key;
        memcpy(cache[slot].text, out, n + 1);
    }
}

// ---- map picture ------------------------------------------------------------

#define MAP_MAX 64

static u8 sTileGfx[0x8000];   // the tileset, as BG3 sees it (chr 0x000-0x3FF)
static u16 sTilePal[10 * 16]; // palettes 6-15
static const void *sTileGfxSrc[3];
static unsigned int sMapPx[MAP_MAX * 16 * MAP_MAX * 16];
static u32 sMapHash;

static unsigned int Bgr555(u16 c)
{
    unsigned r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    r = r << 3 | r >> 2;
    g = g << 3 | g >> 2;
    b = b << 3 | b >> 2;
    return 0xFF000000u | b << 16 | g << 8 | r;
}

static void LoadTileset(void)
{
    const struct ROMChapterData *ch = GetROMChapterStruct(gPlaySt.chapterIndex);
    const void *obj1 = CHAPTER_ASSET(ch->map.obj1Id);
    const void *obj2 = CHAPTER_ASSET(ch->map.obj2Id);
    const void *pal = CHAPTER_ASSET(ch->map.paletteId);

    if (obj1 == sTileGfxSrc[0] && obj2 == sTileGfxSrc[1] && pal == sTileGfxSrc[2])
        return;
    sTileGfxSrc[0] = obj1;
    sTileGfxSrc[1] = obj2;
    sTileGfxSrc[2] = pal;
    memset(sTileGfx, 0, sizeof(sTileGfx));
    if (obj1)
        Decompress(obj1, sTileGfx);
    if (obj2)
        Decompress(obj2, sTileGfx + 0x4000);
    if (pal)
        memcpy(sTilePal, pal, sizeof(sTilePal));
    sMapHash = 0;
}

static void DrawMapTile(int x, int y, u16 entry)
{
    const int pitch = gBmMapSize.x * 16;
    const u8 *gfx = sTileGfx + (entry & 0x3FF) * 32;
    const int hflip = (entry >> 10) & 1, vflip = (entry >> 11) & 1;
    const int pal = (entry >> 12) - 6;
    unsigned int *out = sMapPx + y * pitch + x;
    int row, col;

    for (row = 0; row < 8; row++) {
        const int sy = vflip ? 7 - row : row;
        for (col = 0; col < 8; col++) {
            const int sx = hflip ? 7 - col : col;
            const u8 b = gfx[sy * 4 + sx / 2];
            const int i = (sx & 1) ? b >> 4 : b & 15;
            // Colour 0 is the backdrop: palette 0's first colour.
            out[row * pitch + col] = (pal >= 0 && pal < 10) ? Bgr555(sTilePal[pal * 16 + i]) : 0xFF000000u;
        }
    }
}

// Redraws the map when its tiles or the fog changed (DisplayBmTile's output).
static void UpdateMapPicture(void)
{
    const u16 *config = Port_BmTilesetConfig();
    u32 hash = 2166136261u;
    int x, y;

    LoadTileset();
    for (y = 0; y < gBmMapSize.y; y++) {
        for (x = 0; x < gBmMapSize.x; x++) {
            hash = (hash ^ gBmMapBaseTiles[y][x]) * 16777619u;
            hash = (hash ^ (gBmMapFog[y][x] ? 1 : 0)) * 16777619u;
        }
    }
    hash ^= (u32)(unsigned long)sTileGfxSrc[0] ^ (gBmMapSize.x << 8 | gBmMapSize.y);
    if (hash == sMapHash && sState.mapPx)
        return;
    sMapHash = hash;
    for (y = 0; y < gBmMapSize.y; y++) {
        for (x = 0; x < gBmMapSize.x; x++) {
            const u16 *tile = config + gBmMapBaseTiles[y][x];
            const u16 base = gBmMapFog[y][x] ? (6 << 12) : (11 << 12);
            DrawMapTile(x * 16 + 0, y * 16 + 0, base + tile[0]);
            DrawMapTile(x * 16 + 8, y * 16 + 0, base + tile[1]);
            DrawMapTile(x * 16 + 0, y * 16 + 8, base + tile[2]);
            DrawMapTile(x * 16 + 8, y * 16 + 8, base + tile[3]);
        }
    }
    sState.mapPx = sMapPx;
    sState.mapVersion++;
}

// ---- standing map sprites ---------------------------------------------------

#define SMS_COUNT 0x80
#define SMS_PALETTES 5

static unsigned int *sSprites[SMS_COUNT][SMS_PALETTES];
static u8 sSpriteBuf[0x2000];
static unsigned int sSpritePool[0x80000];
static int sSpritePoolUsed;

const unsigned int *Port_S2Sprite(int sms, int palette, int frame, int *w, int *h)
{
    static const u8 sizes[][2] = { { 16, 16 }, { 16, 32 }, { 32, 32 } };
    const UnitIconWait *info;
    unsigned int *px;
    int size;

    if (sms <= 0 || sms >= SMS_COUNT || palette < 0 || palette >= SMS_PALETTES || frame < 0 || frame > 2)
        return NULL;
    info = &unit_icon_wait_table[sms];
    size = info->size;
    if (size > 2 || !info->sheet)
        return NULL;
    *w = sizes[size][0];
    *h = sizes[size][1];

    px = sSprites[sms][palette];
    if (!px) {
        // The three frames, one under the other.
        const int tw = *w / 8, th = *h / 8;
        const u16 *pal;
        u16 grey[16];
        int f, t, i;

        if (palette == 4) {
            // Units that already moved: the blue palette in greys.
            for (i = 0; i < 16; i++) {
                const u16 c = gPal_MapSprite[i];
                const int l = ((c & 31) * 5 + ((c >> 5) & 31) * 9 + ((c >> 10) & 31) * 2) / 16;
                grey[i] = l | l << 5 | l << 10;
            }
            pal = grey;
        } else {
            static const u8 lut[] = { 0, 2, 1, 1 }; // blue, green, red (purple: red)
            pal = gPal_MapSprite + lut[palette] * 16;
        }
        if (sSpritePoolUsed + *w * *h * 3 > (int)(sizeof(sSpritePool) / sizeof(sSpritePool[0]))) {
            memset(sSprites, 0, sizeof(sSprites));
            sSpritePoolUsed = 0;
        }
        px = sSpritePool + sSpritePoolUsed;
        sSpritePoolUsed += *w * *h * 3;
        Decompress(info->sheet, sSpriteBuf);
        for (f = 0; f < 3; f++) {
            for (t = 0; t < tw * th; t++) {
                const u8 *gfx = sSpriteBuf + (f * tw * th + t) * 32;
                const int tx = (t % tw) * 8, ty = (t / tw) * 8 + f * *h;
                int row, col;
                for (row = 0; row < 8; row++) {
                    for (col = 0; col < 8; col++) {
                        const u8 b = gfx[row * 4 + col / 2];
                        const int c = (col & 1) ? b >> 4 : b & 15;
                        px[(ty + row) * *w + tx + col] = c ? Bgr555(pal[c]) : 0;
                    }
                }
            }
        }
        sSprites[sms][palette] = px;
    }
    return px + frame * *w * *h;
}

// ---- units ------------------------------------------------------------------

static int Faction(const struct Unit *unit)
{
    switch (UNIT_FACTION(unit)) {
    case FACTION_GREEN:
        return PORT_S2_FACTION_GREEN;
    case FACTION_RED:
        return PORT_S2_FACTION_RED;
    case FACTION_PURPLE:
        return PORT_S2_FACTION_PURPLE;
    default:
        return PORT_S2_FACTION_BLUE;
    }
}

static void FillUnit(struct PortS2Unit *out, struct Unit *unit)
{
    out->id = unit->index;
    out->faction = Faction(unit);
    out->x = unit->xPos;
    out->y = unit->yPos;
    out->hp = GetUnitCurrentHp(unit);
    out->maxHp = GetUnitMaxHp(unit);
    out->level = unit->level;
    out->acted = (unit->state & US_UNSELECTABLE) != 0;
    out->boss = (UNIT_CATTRIBUTES(unit) & CA_BOSS) != 0;
    out->sms = GetUnitSMSId(unit);
    out->rescuing = (unit->state & US_RESCUING) != 0;
    CopyMsg(out->name, sizeof(out->name), unit->pCharacterData->nameTextId);
    CopyMsg(out->className, sizeof(out->className), unit->pClassData->nameTextId);
}

static void FillItem(struct PortS2Item *out, int item)
{
    const struct ItemData *data = GetItemData(ITEM_INDEX(item));

    CopyMsg(out->name, sizeof(out->name), data->nameTextId);
    out->uses = ITEM_USES(item);
    out->maxUses = (data->attributes & IA_UNBREAKABLE) ? 0 : data->maxUses;
    out->icon = GetItemIconId(item);
    out->equipped = 0;
}

static void FillDetail(struct PortS2Detail *out, struct Unit *unit)
{
    int i, equipped;

    memset(out, 0, sizeof(*out));
    if (!UNIT_IS_VALID(unit) || !unit->pClassData)
        return;
    out->valid = 1;
    FillUnit(&out->unit, unit);
    out->exp = unit->exp;
    out->pow = GetUnitPower(unit);
    out->skl = GetUnitSkill(unit);
    out->spd = GetUnitSpeed(unit);
    out->lck = GetUnitLuck(unit);
    out->def = GetUnitDefense(unit);
    out->res = GetUnitResistance(unit);
    out->con = UNIT_CON(unit);
    out->mov = UNIT_MOV(unit);
    out->aid = GetUnitAid(unit);
    out->magic = UnitHasMagicRank(unit);
    if (unit->statusIndex)
        CopyMsg(out->status, sizeof(out->status), Port_StatusNameTextId(unit->statusIndex));
    equipped = GetUnitEquippedWeaponSlot(unit);
    for (i = 0; i < UNIT_ITEM_COUNT && i < PORT_S2_MAX_ITEMS && unit->items[i]; i++) {
        FillItem(&out->items[i], unit->items[i]);
        out->items[i].equipped = i == equipped;
    }
    out->itemCount = i;
}

static void FillFighter(struct PortS2Fighter *out, struct BattleUnit *bu, struct BattleUnit *foe, int hits, int eff)
{
    const int canAttack = bu->weapon != 0 || bu->weaponBroke;

    memset(out, 0, sizeof(*out));
    CopyMsg(out->name, sizeof(out->name), bu->unit.pCharacterData ? bu->unit.pCharacterData->nameTextId : 0);
    if (bu->weaponBefore)
        CopyMsg(out->weapon, sizeof(out->weapon), GetItemData(ITEM_INDEX(bu->weaponBefore))->nameTextId);
    out->faction = bu->unit.index ? Faction(&bu->unit) : PORT_S2_FACTION_OBJECT;
    out->hp = bu->hpInitial;
    out->maxHp = bu->unit.maxHP;
    out->def = bu->battleDefense;
    out->as = bu->battleSpeed;
    if (canAttack) {
        out->mt = bu->battleAttack - foe->battleDefense;
        if (out->mt < 0)
            out->mt = 0;
        out->hit = bu->battleEffectiveHitRate;
        out->crit = bu->battleEffectiveCritRate < 0 ? 0 : bu->battleEffectiveCritRate;
        out->hits = hits;
    } else {
        out->mt = out->hit = out->crit = -1;
        out->hits = 0;
    }
    out->effective = eff;
}

// ---- snapshot ---------------------------------------------------------------

static ProcPtr IdlePlayerPhase(void)
{
    struct Proc *proc = Proc_Find(gProcScr_PlayerPhase);

    if (!proc || proc->proc_lockCnt)
        return NULL;
    if (proc->proc_idleCb != (ProcFunc)PlayerPhase_MainIdle &&
        proc->proc_idleCb != (ProcFunc)PlayerPhase_RangeDisplayIdle)
        return NULL;
    return proc;
}

const struct PortS2State *Port_S2Snapshot(void)
{
    struct PortS2State *s = &sState;
    struct BattleForecastProc *bksel;
    struct Unit *unit;
    int x, y;

    s->onMap = Proc_Find(gProc_BMapMain) != NULL && gBmMapUnit && gBmMapFog && gBmMapBaseTiles &&
               gBmMapSize.x > 0 && gBmMapSize.y > 0 && gBmMapSize.x <= MAP_MAX && gBmMapSize.y <= MAP_MAX;
    s->unitCount = 0;
    s->detail.valid = 0;
    s->forecast.valid = 0;
    if (!s->onMap)
        return s;

    CopyMsg(s->chapter, sizeof(s->chapter), GetROMChapterStruct(gPlaySt.chapterIndex)->chapTitleTextId);
    s->turn = gPlaySt.chapterTurnNumber;
    s->phase = gPlaySt.faction == FACTION_RED ? PORT_S2_FACTION_RED
             : gPlaySt.faction == FACTION_GREEN ? PORT_S2_FACTION_GREEN
             : PORT_S2_FACTION_BLUE;
    s->gold = gPlaySt.partyGoldAmount;
    s->canControl = IdlePlayerPhase() != NULL;
    s->mapW = gBmMapSize.x;
    s->mapH = gBmMapSize.y;
    s->cursorX = gBmSt.playerCursor.x;
    s->cursorY = gBmSt.playerCursor.y;
    // With the widescreen the main screen shows more on the sides.
    s->cameraX = gBmSt.camera.x - Port_WideLeft();
    s->cameraW = DISPLAY_WIDTH + Port_WideLeft() + Port_WideRight();
    if (s->cameraX < 0) {
        s->cameraW += s->cameraX;
        s->cameraX = 0;
    }
    if (s->cameraX + s->cameraW > s->mapW * 16)
        s->cameraW = s->mapW * 16 - s->cameraX;
    s->cameraY = gBmSt.camera.y;
    UpdateMapPicture();

    // The units the map shows (enemies hidden by the fog are not on it).
    for (y = 0; y < gBmMapSize.y; y++) {
        for (x = 0; x < gBmMapSize.x; x++) {
            const int id = gBmMapUnit[y][x];
            if (!id || s->unitCount >= PORT_S2_MAX_UNITS)
                continue;
            unit = GetUnit(id);
            if (!UNIT_IS_VALID(unit) || !unit->pClassData || (unit->state & (US_HIDDEN | US_DEAD)))
                continue;
            FillUnit(&s->units[s->unitCount++], unit);
        }
    }

    if (s->cursorX >= 0 && s->cursorX < s->mapW && s->cursorY >= 0 && s->cursorY < s->mapH) {
        CopyMsg(s->terrain, sizeof(s->terrain), gUnknown_0880D374[gBmMapTerrain[s->cursorY][s->cursorX]]);
        unit = GetUnit(gBmMapUnit[s->cursorY][s->cursorX]);
        if (!gBmMapUnit[s->cursorY][s->cursorX] && Proc_Find(gProcScr_PlayerPhase) && !IdlePlayerPhase())
            unit = gActiveUnit; // moving or acting
        else if (gBmMapUnit[s->cursorY][s->cursorX] == 0)
            unit = NULL;
        if (unit)
            FillDetail(&s->detail, unit);
    } else {
        s->terrain[0] = 0;
    }

    bksel = Proc_Find(gProcScr_BKSEL);
    if (bksel && gBattleActor.unit.pCharacterData && gBattleTarget.unit.pCharacterData) {
        s->forecast.valid = 1;
        FillFighter(&s->forecast.attacker, &gBattleActor, &gBattleTarget, bksel->hitCountA, bksel->isEffectiveA);
        FillFighter(&s->forecast.defender, &gBattleTarget, &gBattleActor, bksel->hitCountB, bksel->isEffectiveB);
    }
    return s;
}

int Port_S2TerrainAt(int x, int y)
{
    if (!gBmMapTerrain || x < 0 || y < 0 || x >= gBmMapSize.x || y >= gBmMapSize.y)
        return -1;
    return gBmMapTerrain[y][x];
}

// ---- requests ---------------------------------------------------------------

static int sMoveX = -1, sMoveY = -1;

void Port_S2MoveCursor(int x, int y)
{
    sMoveX = x;
    sMoveY = y;
}

void Port_S2Frame(void)
{
    ProcPtr proc;
    const int x = sMoveX, y = sMoveY;

    if (x < 0)
        return;
    sMoveX = sMoveY = -1;
    proc = IdlePlayerPhase();
    if (!proc || x >= gBmMapSize.x || y >= gBmMapSize.y)
        return;
    // Not while the cursor is still sliding from one tile to the next.
    if ((gBmSt.playerCursorDisplay.x | gBmSt.playerCursorDisplay.y) & 0xF)
        return;
    EnsureCameraOntoPosition(proc, x, y);
    SetCursorMapPosition(x, y);
}
