/*
 * aifaction.c - AI Faction system implementation
 *
 * AI factions own buildings (bld->owned = faction->owner_name, bld->owner = NULL)
 * and autonomously attack enemy buildings and repair their own.
 */

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ack.h"
#include "aifaction.h"

AI_FACTION  ai_factions[MAX_AI_FACTIONS];
int         num_ai_factions = 0;

/* -----------------------------------------------------------------------
 * Lifecycle
 * ----------------------------------------------------------------------- */

void save_ai_factions( void )
{
    FILE *fp;
    int   i, t;

    if ( ( fp = fopen( AIFAC_FILE, "w" ) ) == NULL )
    {
        log_f( "save_ai_factions: cannot open %s for writing", AIFAC_FILE );
        return;
    }

    fprintf( fp, "%d\n", num_ai_factions );

    for ( i = 0; i < num_ai_factions; i++ )
    {
        AI_FACTION *fac = &ai_factions[i];
        fprintf( fp, "#FACTION\n" );
        fprintf( fp, "Name        %s~\n",        fac->name        ? fac->name        : "" );
        fprintf( fp, "OwnerName   %s~\n",        fac->owner_name  ? fac->owner_name  : "" );
        fprintf( fp, "Description %s~\n",        fac->description ? fac->description : "" );
        fprintf( fp, "Active      %d\n",         fac->active ? 1 : 0 );
        fprintf( fp, "Aggression  %d\n",         fac->aggression );
        fprintf( fp, "Hostility   %d\n",         fac->player_hostility );
        fprintf( fp, "NumTargets  %d\n",         fac->num_targets );
        for ( t = 0; t < fac->num_targets; t++ )
            fprintf( fp, "Target      %s~\n",
                     fac->target_names[t] ? fac->target_names[t] : "" );
        fprintf( fp, "End\n\n" );
    }

    fprintf( fp, "#END\n" );
    fclose( fp );
}

void load_ai_factions( void )
{
    FILE *fp;
    char  word[MAX_INPUT_LENGTH];
    int   i;

    num_ai_factions = 0;
    memset( ai_factions, 0, sizeof( ai_factions ) );

    if ( ( fp = fopen( AIFAC_FILE, "r" ) ) == NULL )
        return;  /* no file yet — that's fine */

    /* first line: count (informational, not relied upon) */
    fscanf( fp, "%*d\n" );

    for ( ; ; )
    {
        if ( fscanf( fp, " %511s", word ) != 1 )
            break;

        if ( !strcmp( word, "#END" ) )
            break;

        if ( strcmp( word, "#FACTION" ) )
            continue;  /* skip unrecognised sections */

        if ( num_ai_factions >= MAX_AI_FACTIONS )
        {
            log_f( "load_ai_factions: too many factions (max %d)", MAX_AI_FACTIONS );
            break;
        }

        AI_FACTION *fac = &ai_factions[num_ai_factions];
        bool fMatch;
        bool done = FALSE;

        fac->aggression       = 50;
        fac->player_hostility = 30;
        fac->active           = TRUE;
        fac->attack_timer     = 0;
        fac->repair_timer     = 0;
        fac->num_targets      = 0;

        while ( !done )
        {
            if ( fscanf( fp, " %511s", word ) != 1 )
                break;

            fMatch = FALSE;

            switch ( UPPER( word[0] ) )
            {
            case 'A':
                if ( !strcmp( word, "Active" ) )
                {
                    int v; fscanf( fp, " %d", &v );
                    fac->active = ( v != 0 );
                    fMatch = TRUE;
                }
                if ( !strcmp( word, "Aggression" ) )
                {
                    fscanf( fp, " %d", &fac->aggression );
                    fMatch = TRUE;
                }
                break;
            case 'D':
                if ( !strcmp( word, "Description" ) )
                {
                    fac->description = fread_string( fp );
                    fMatch = TRUE;
                }
                break;
            case 'E':
                if ( !strcmp( word, "End" ) )
                {
                    done   = TRUE;
                    fMatch = TRUE;
                }
                break;
            case 'H':
                if ( !strcmp( word, "Hostility" ) )
                {
                    fscanf( fp, " %d", &fac->player_hostility );
                    fMatch = TRUE;
                }
                break;
            case 'N':
                if ( !strcmp( word, "Name" ) )
                {
                    fac->name = fread_string( fp );
                    fMatch = TRUE;
                }
                if ( !strcmp( word, "NumTargets" ) )
                {
                    /* purely informational; we just count Target lines */
                    int dummy; fscanf( fp, " %d", &dummy );
                    fMatch = TRUE;
                }
                break;
            case 'O':
                if ( !strcmp( word, "OwnerName" ) )
                {
                    fac->owner_name = fread_string( fp );
                    fMatch = TRUE;
                }
                break;
            case 'T':
                if ( !strcmp( word, "Target" ) )
                {
                    if ( fac->num_targets < MAX_AI_TARGETS )
                    {
                        fac->target_names[fac->num_targets] = fread_string( fp );
                        fac->num_targets++;
                    }
                    else
                        fread_string( fp );  /* discard */
                    fMatch = TRUE;
                }
                break;
            }

            if ( !fMatch )
            {
                /* skip unknown line */
                char lbuf[1024];
                if ( fgets( lbuf, sizeof(lbuf), fp ) == NULL )
                    break;
            }
        }

        /* validate */
        if ( fac->name && fac->owner_name )
        {
            /* initialise timers so factions don't all fire at once */
            fac->attack_timer = number_range( 0, 30 );
            fac->repair_timer = number_range( 0, AI_REPAIR_INTERVAL );
            num_ai_factions++;
        }
        else
        {
            /* free partial strings */
            if ( fac->name )        { free_string( fac->name );        fac->name        = NULL; }
            if ( fac->owner_name )  { free_string( fac->owner_name );  fac->owner_name  = NULL; }
            if ( fac->description ) { free_string( fac->description ); fac->description = NULL; }
            for ( i = 0; i < fac->num_targets; i++ )
                if ( fac->target_names[i] ) { free_string( fac->target_names[i] ); fac->target_names[i] = NULL; }
            memset( fac, 0, sizeof(*fac) );
        }
    }

    fclose( fp );
}

/* -----------------------------------------------------------------------
 * Lookup helpers
 * ----------------------------------------------------------------------- */

AI_FACTION *find_ai_faction( const char *name )
{
    int i;
    for ( i = 0; i < num_ai_factions; i++ )
        if ( ai_factions[i].name && !str_cmp( ai_factions[i].name, name ) )
            return &ai_factions[i];
    return NULL;
}

AI_FACTION *find_ai_faction_by_owner( const char *owner_name )
{
    int i;
    for ( i = 0; i < num_ai_factions; i++ )
        if ( ai_factions[i].owner_name && !str_cmp( ai_factions[i].owner_name, owner_name ) )
            return &ai_factions[i];
    return NULL;
}

bool is_ai_owned( const char *owner_name )
{
    if ( !owner_name )
        return FALSE;
    return ( strncmp( owner_name, AI_OWNER_PREFIX, strlen( AI_OWNER_PREFIX ) ) == 0 );
}

/* -----------------------------------------------------------------------
 * Attack / repair helpers
 * ----------------------------------------------------------------------- */

/*
 * ai_damage_building - apply damage from an AI faction to a building.
 * Bypasses damage_building() because we have no online CHAR_DATA attacker.
 */
static void ai_damage_building( AI_FACTION *fac, BUILDING_DATA *bld, int dam )
{
    char     buf[MAX_STRING_LENGTH];
    CHAR_DATA *owner;

    if ( dam <= 0 )
        return;

    /* absorb with shield first */
    if ( bld->shield > 0 )
    {
        int absorb = dam / 2;
        if ( absorb > bld->shield )
            absorb = bld->shield;
        bld->shield -= absorb;
        dam         -= absorb;
    }

    bld->hp -= dam;

    /* notify real owner if online */
    owner = get_ch( bld->owned );
    if ( owner )
    {
        snprintf( buf, sizeof(buf),
                  "@@rYour building at @@N%d/%d@@r is under attack by @@N%s@@r! (HP: %d/%d)@@N\n\r",
                  bld->x, bld->y, fac->name, bld->hp, bld->maxhp );
        send_to_char( buf, owner );
    }

    if ( bld->hp <= 0 )
    {
        snprintf( buf, sizeof(buf),
                  "@@SYSTEM: %s destroyed building at %d/%d (owned: %s)",
                  fac->name, bld->x, bld->y, bld->owned );
        monitor_chan( NULL, buf, MONITOR_SYSTEM );

        if ( owner )
        {
            snprintf( buf, sizeof(buf),
                      "@@rYour building at @@N%d/%d@@r has been destroyed by @@N%s@@r!@@N\n\r",
                      bld->x, bld->y, fac->name );
            send_to_char( buf, owner );
        }

        extract_building( bld, TRUE );
    }
}

/*
 * ai_pick_target - choose a building to attack for this faction.
 *
 * Priority pool A : buildings owned by explicit target factions.
 * Priority pool B : player-owned buildings (included with probability
 *                   player_hostility / 100 to avoid ganging up on players).
 * Fallback pool C : any AI-owned building not belonging to this faction.
 *
 * Returns a random entry from the highest non-empty pool.
 */
static BUILDING_DATA *ai_pick_target( AI_FACTION *fac )
{
#define MAX_CANDIDATES 64
    BUILDING_DATA *poolA[MAX_CANDIDATES];
    BUILDING_DATA *poolB[MAX_CANDIDATES];
    BUILDING_DATA *poolC[MAX_CANDIDATES];
    int cntA = 0, cntB = 0, cntC = 0;
    BUILDING_DATA *bld;
    int t;

    for ( bld = first_building; bld; bld = bld->next )
    {
        if ( !bld->owned )
            continue;

        /* skip own buildings */
        if ( !str_cmp( bld->owned, fac->owner_name ) )
            continue;

        if ( is_ai_owned( bld->owned ) )
        {
            /* check if this is an explicit target */
            bool is_target = FALSE;
            for ( t = 0; t < fac->num_targets; t++ )
            {
                if ( fac->target_names[t] )
                {
                    AI_FACTION *enemy = find_ai_faction( fac->target_names[t] );
                    if ( enemy && enemy->owner_name &&
                         !str_cmp( bld->owned, enemy->owner_name ) )
                    {
                        is_target = TRUE;
                        break;
                    }
                }
            }

            if ( is_target && cntA < MAX_CANDIDATES )
                poolA[cntA++] = bld;
            else if ( cntC < MAX_CANDIDATES )
                poolC[cntC++] = bld;
        }
        else
        {
            /* player building — include only if hostility roll passes */
            if ( number_percent() < fac->player_hostility && cntB < MAX_CANDIDATES )
                poolB[cntB++] = bld;
        }
    }

    if ( cntA > 0 )
        return poolA[number_range( 0, cntA - 1 )];
    if ( cntB > 0 )
        return poolB[number_range( 0, cntB - 1 )];
    if ( cntC > 0 )
        return poolC[number_range( 0, cntC - 1 )];
    return NULL;
#undef MAX_CANDIDATES
}

/*
 * ai_most_damaged - find the faction's own building most in need of repair.
 * Returns NULL if all buildings are above 80% HP.
 */
static BUILDING_DATA *ai_most_damaged( AI_FACTION *fac )
{
    BUILDING_DATA *worst = NULL;
    int worst_pct = 80;  /* don't bother repairing above 80% */
    BUILDING_DATA *bld;

    for ( bld = first_building; bld; bld = bld->next )
    {
        if ( !bld->owned || str_cmp( bld->owned, fac->owner_name ) )
            continue;
        if ( bld->maxhp <= 0 )
            continue;

        int pct = ( bld->hp * 100 ) / bld->maxhp;
        if ( pct < worst_pct )
        {
            worst_pct = pct;
            worst     = bld;
        }
    }

    return worst;
}

/* -----------------------------------------------------------------------
 * Per-tick update
 * ----------------------------------------------------------------------- */

void ai_faction_update( void )
{
    int i;

    for ( i = 0; i < num_ai_factions; i++ )
    {
        AI_FACTION *fac = &ai_factions[i];

        if ( !fac->active || !fac->name || !fac->owner_name )
            continue;

        /* ---- attack timer ---- */
        if ( fac->attack_timer > 0 )
        {
            fac->attack_timer--;
        }
        else
        {
            BUILDING_DATA *target = ai_pick_target( fac );

            if ( target )
            {
                int dam = number_range( 5, 15 + fac->aggression / 5 );
                ai_damage_building( fac, target, dam );
            }

            /* reset timer: more aggressive = shorter interval */
            int interval = AI_ATTACK_MIN
                + ( AI_ATTACK_MAX - AI_ATTACK_MIN )
                  * ( 100 - fac->aggression ) / 100;
            fac->attack_timer = interval + number_range( -5, 5 );
            if ( fac->attack_timer < 1 )
                fac->attack_timer = 1;
        }

        /* ---- repair timer ---- */
        if ( fac->repair_timer > 0 )
        {
            fac->repair_timer--;
        }
        else
        {
            BUILDING_DATA *damaged = ai_most_damaged( fac );

            if ( damaged )
            {
                int heal = number_range( 5, 20 );
                damaged->hp += heal;
                if ( damaged->hp > damaged->maxhp )
                    damaged->hp = damaged->maxhp;

                CHAR_DATA *owner = get_ch( damaged->owned );
                if ( owner )
                {
                    char buf[MAX_STRING_LENGTH];
                    snprintf( buf, sizeof(buf),
                              "@@gYour building at @@N%d/%d@@g has been repaired by @@N%s@@g. (HP: %d/%d)@@N\n\r",
                              damaged->x, damaged->y, fac->name,
                              damaged->hp, damaged->maxhp );
                    send_to_char( buf, owner );
                }
            }

            fac->repair_timer = AI_REPAIR_INTERVAL;
        }
    }
}
