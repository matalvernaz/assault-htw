/*
 * aifaction.h - AI Faction system
 *
 * AI factions own buildings (bld->owned = faction->owner_name,
 * bld->owner = NULL) and autonomously attack enemy buildings and repair
 * their own. Multiple factions fight each other; player_hostility controls
 * how much each faction targets players vs rival AI.
 *
 * Admin command: aifaction (see do_aifaction in act_wiz.c)
 */

#ifndef AIFACTION_H
#define AIFACTION_H

#define MAX_AI_FACTIONS     20
#define MAX_AI_TARGETS      10
#define AI_OWNER_PREFIX     "AI_"   /* owner_name always starts with this */

/*
 * Attack interval in PULSE_TICK units.
 * aggression 100  -> interval ~AI_ATTACK_MIN ticks
 * aggression 0    -> interval ~AI_ATTACK_MAX ticks
 */
#define AI_ATTACK_MIN       15
#define AI_ATTACK_MAX       150
#define AI_REPAIR_INTERVAL  60      /* ticks between repair checks */

#define AIFAC_FILE  "../data/aifactions.dat"

typedef struct ai_faction_data AI_FACTION;
struct ai_faction_data
{
    char   *name;               /* Display name, e.g. "Red Clan"        */
    char   *owner_name;         /* bld->owned string, e.g. "AI_RedClan" */
    char   *description;
    bool    active;             /* Whether this faction takes actions    */
    int     aggression;         /* 0-100: attack frequency               */
    int     player_hostility;   /* 0-100: 0=only attacks AI, 100=prefers players */
    int     attack_timer;       /* ticks until next attack action        */
    int     repair_timer;       /* ticks until next repair action        */
    char   *target_names[MAX_AI_TARGETS]; /* rival faction names         */
    int     num_targets;
};

extern AI_FACTION  ai_factions[MAX_AI_FACTIONS];
extern int         num_ai_factions;

/* lifecycle */
void        load_ai_factions( void );
void        save_ai_factions( void );

/* lookup */
AI_FACTION *find_ai_faction( const char *name );
AI_FACTION *find_ai_faction_by_owner( const char *owner_name );
bool        is_ai_owned( const char *owner_name );

/* building placement (used by do_aifaction create) */
BUILDING_DATA *ai_place_building( AI_FACTION *fac, int type, int x, int y, int z );
bool           ai_find_free_spot( int type, int cx, int cy, int z, int radius, int *ox, int *oy );
bool           ai_find_faction_location( int *ox, int *oy, int *oz );

/* per-tick update called from update_handler */
void        ai_faction_update( void );

#endif /* AIFACTION_H */
