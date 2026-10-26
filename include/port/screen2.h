#ifndef PORT_S2_STATE_H
#define PORT_S2_STATE_H

/*
 * fire-emblem-ss-recomp: what the second screen (AYN Thor's lower display, or
 * a second window on PC) shows of the battle map, read from the game state by
 * port/game/screen2_game.c and drawn by port/src/second_screen.c.
 *
 * Shared by game code and port code: plain C types only. Strings are ASCII
 * (accented letters lose their accents).
 */

#ifdef __cplusplus
extern "C" {
#endif

#define PORT_S2_NAME_LEN 16
#define PORT_S2_MAX_UNITS 128
#define PORT_S2_MAX_ITEMS 5

enum {
    PORT_S2_FACTION_BLUE,
    PORT_S2_FACTION_GREEN,
    PORT_S2_FACTION_RED,
    PORT_S2_FACTION_PURPLE,
    PORT_S2_FACTION_OBJECT, /* walls and snags */
};

struct PortS2Unit {
    unsigned char id;  /* game unit index (faction in the top bits) */
    unsigned char faction;
    unsigned char x, y;
    unsigned char hp, maxHp, level;
    unsigned char acted;  /* greyed out: already moved this turn */
    unsigned char boss;
    unsigned char sms;    /* standing map sprite id */
    unsigned char rescuing;
    char name[PORT_S2_NAME_LEN];
    char className[PORT_S2_NAME_LEN];
};

struct PortS2Item {
    char name[PORT_S2_NAME_LEN];
    unsigned char uses, maxUses; /* maxUses 0: unbreakable */
    unsigned char icon;
    unsigned char equipped;
};

struct PortS2Detail {
    int valid;
    struct PortS2Unit unit;
    unsigned char exp;
    signed char pow, skl, spd, lck, def, res, con, mov, aid;
    unsigned char magic; /* "Mag" instead of "Str" */
    char status[PORT_S2_NAME_LEN];
    int itemCount;
    struct PortS2Item items[PORT_S2_MAX_ITEMS];
};

struct PortS2Fighter {
    char name[PORT_S2_NAME_LEN];
    char weapon[PORT_S2_NAME_LEN];
    unsigned char faction;
    int hp, maxHp;
    int mt, hit, crit, as, def;  /* -1: can't counter */
    int hits;                    /* attacks in the round (x2, x4...) */
    int effective;
};

struct PortS2Forecast {
    int valid;
    struct PortS2Fighter attacker, defender;
};

struct PortS2State {
    int onMap;  /* 0: no battle map (title, world map, menus...) */
    char chapter[32];
    int turn;
    int phase;  /* PORT_S2_FACTION_* of the side whose turn it is */
    int gold;
    int canControl;  /* player phase, cursor free: touches move it */

    int mapW, mapH;              /* tiles */
    const unsigned int* mapPx;   /* ABGR, (mapW*16) x (mapH*16) */
    unsigned int mapVersion;     /* changes when mapPx changes */
    int cursorX, cursorY;        /* tiles */
    int cameraX, cameraY;        /* pixels: the top-left of the main screen */
    int cameraW;                 /* pixels: its width (wider with the widescreen) */

    char terrain[PORT_S2_NAME_LEN];

    int unitCount;
    struct PortS2Unit units[PORT_S2_MAX_UNITS];

    struct PortS2Detail detail;  /* the unit under the cursor (or acting) */
    struct PortS2Forecast forecast;
};

/* Refreshes and returns the snapshot. Game thread only. */
const struct PortS2State* Port_S2Snapshot(void);

/* A standing map sprite: ABGR, w x h, frame 0..2, palette PORT_S2_FACTION_*
 * (or 4 = grey). NULL if it can't be drawn. Game thread only. */
const unsigned int* Port_S2Sprite(int sms, int palette, int frame, int* w, int* h);

/* The terrain id (TERRAIN_*) of a map tile, -1 outside the map. */
int Port_S2TerrainAt(int x, int y);

/* Asks the game to put the map cursor on a tile (and the camera on it).
 * Applied at the next frame boundary, when the player can move the cursor. */
void Port_S2MoveCursor(int x, int y);

/* Called by AgbMain's loop after each update (applies queued requests). */
void Port_S2Frame(void);

#ifdef __cplusplus
}
#endif

#endif /* PORT_S2_STATE_H */
