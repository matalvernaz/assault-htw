/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  Ack 2.2 improvements copyright (C) 1994 by Stephen Dooley              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *       _/          _/_/_/     _/    _/     _/    ACK! MUD is modified    *
 *      _/_/        _/          _/  _/       _/    Merc2.0/2.1/2.2 code    *
 *     _/  _/      _/           _/_/         _/    (c)Stephen Zepp 1998    *
 *    _/_/_/_/      _/          _/  _/             Version #: 4.3          *
 *   _/      _/      _/_/_/     _/    _/     _/                            *
 *                                                                         *
 *                        http://ackmud.nuc.net/                           *
 *                        zenithar@ackmud.nuc.net                          *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
/* For forks etc. */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define DEC_STRFUNS_H       1

#ifndef  DEC_ACK_H
#include "ack.h"
#include "tables.h"
#endif

char *  learnt_name           ( int learnt            );
int     my_strlen               ( const char *text            );
char *get_moon_phase_name     ( void  );
char *get_tribe_standing_name ( int standing );
char *get_wolf_auspice        ( int auspice );
bool is_number                ( char *arg );
bool is_name                  ( const char *str, char *namelist );
bool list_in_list             ( char * first_list, char * second_list );
int     number_argument       ( char *argument, char *arg );
char *  one_argument          ( char *argument, char *arg_first );
char *  two_args              ( char *argument, char *arg_first, char *arg_second );
char * str_mod                ( char * mod_string,  char *argument );
void    rand_arg              ( char *argument, char *output );
char * space_pad              ( const char * str, sh_int final_size );
void safe_strcat              (int max_len,char * dest,char * source);
char *center_text         ( char *text, int width );
char *  item_type_name        ( OBJ_DATA *obj             );
char *  item_type_desc        ( int type              );
char *  affect_loc_name       ( int location          );
char *  affect_bit_name       ( int vector            );
char *  raffect_bit_name      ( int vector            );

char *  material_name         ( int type          );
char *  extra_bit_name        ( int extra_flags       );
char *  weapon_bit_name       ( int weapon_flags      );
bool    str_cmp               ( const char *astr, const char *bstr );
bool    str_prefix            ( const char *astr, const char *bstr );
bool    str_infix             ( const char *astr, const char *bstr );
bool    str_suffix            ( const char *astr, const char *bstr );
char *  capitalize            ( const char *str );
void    smash_tilde           ( char *str );
void    smash_system          ( char *str );
char *  smash_swear          ( char *str );
char *strip_out               (const char *orig, const char *strip);
char *strip_color             (const char *orig, const char *strip);
int nocol_strlen( const char *text );
int ccode_len( const char *text, sh_int desired );

void pre_parse( char * list, char * victimname, char * containername, char * things );
char *one_word( char *argument, char *arg_first ) ;
char *get_wolf_name( CHAR_DATA *ch );
