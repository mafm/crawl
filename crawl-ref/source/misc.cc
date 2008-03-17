/*
 *  File:       misc.cc
 *  Summary:    Misc functions.
 *  Written by: Linley Henzell
 *
 *  Modified for Crawl Reference by $Author$ on $Date$
 *
 *  Change History (most recent first):
 *
 *   <3>   11/14/99      cdl    evade with random40(ev) vice random2(ev)
 *   <2>    5/20/99      BWR    Multi-user support, new berserk code.
 *   <1>    -/--/--      LRH    Created
 */


#include "AppHdr.h"
#include "misc.h"
#include "notes.h"

#include <string.h>
#if !defined(__IBMCPP__) && !defined(_MSC_VER)
#include <unistd.h>
#endif

#ifdef __MINGW32__
#include <io.h>
#endif

#include <cstdlib>
#include <cstdio>
#include <cmath>

#ifdef DOS
#include <conio.h>
#endif

#include "externs.h"
#include "misc.h"

#include "abyss.h"
#include "branch.h"
#include "chardump.h"
#include "clua.h"
#include "cloud.h"
#include "delay.h"
#include "direct.h"
#include "dgnevent.h"
#include "direct.h"
#include "dungeon.h"
#include "files.h"
#include "food.h"
#include "format.h"
#include "hiscores.h"
#include "it_use2.h"
#include "itemprop.h"
#include "items.h"
#include "lev-pand.h"
#include "macro.h"
#include "message.h"
#include "mon-util.h"
#include "monstuff.h"
#include "ouch.h"
#include "overmap.h"
#include "place.h"
#include "player.h"
#include "religion.h"
#include "shopping.h"
#include "skills.h"
#include "skills2.h"
#include "spells3.h"
#include "stash.h"
#include "state.h"
#include "stuff.h"
#include "tiles.h"
#include "terrain.h"
#include "transfor.h"
#include "traps.h"
#include "travel.h"
#include "tutorial.h"
#include "view.h"
#include "xom.h"

static void create_monster_hide(int mons_class)
{
    int o = get_item_slot( 100 + random2(200) );
    if (o == NON_ITEM)
        return;

    mitm[o].quantity = 1;

    // these values are common to all: {dlb}
    mitm[o].base_type = OBJ_ARMOUR;
    mitm[o].plus      = 0;
    mitm[o].plus2     = 0;
    mitm[o].special   = 0;
    mitm[o].flags     = 0;
    mitm[o].colour    = mons_class_colour( mons_class );

    // these values cannot be set by a reasonable formula: {dlb}
    switch (mons_class)
    {
    case MONS_DRAGON:
        mitm[o].sub_type = ARM_DRAGON_HIDE;
        break;
    case MONS_TROLL:
        mitm[o].sub_type = ARM_TROLL_HIDE;
        break;
    case MONS_ICE_DRAGON:
        mitm[o].sub_type = ARM_ICE_DRAGON_HIDE;
        break;
    case MONS_STEAM_DRAGON:
        mitm[o].sub_type = ARM_STEAM_DRAGON_HIDE;
        break;
    case MONS_MOTTLED_DRAGON:
        mitm[o].sub_type = ARM_MOTTLED_DRAGON_HIDE;
        break;
    case MONS_STORM_DRAGON:
        mitm[o].sub_type = ARM_STORM_DRAGON_HIDE;
        break;
    case MONS_GOLDEN_DRAGON:
        mitm[o].sub_type = ARM_GOLD_DRAGON_HIDE;
        break;
    case MONS_SWAMP_DRAGON:
        mitm[o].sub_type = ARM_SWAMP_DRAGON_HIDE;
        break;
    case MONS_SHEEP:
    case MONS_YAK:
    default:
        mitm[o].sub_type = ARM_ANIMAL_SKIN;
        break;
    }

    move_item_to_grid( &o, you.x_pos, you.y_pos );
}

void turn_corpse_into_chunks( item_def &item )
{
    ASSERT( item.base_type == OBJ_CORPSES );

    const int mons_class = item.plus;
    const int max_chunks = mons_weight( mons_class ) / 150;

    // only fresh corpses bleed enough to colour the ground
    if (!food_is_rotten(item))
        bleed_onto_floor(you.x_pos, you.y_pos, mons_class, max_chunks, true);
    
    item.base_type = OBJ_FOOD;
    item.sub_type  = FOOD_CHUNK;
    item.quantity  = 1 + random2( max_chunks );
    item.quantity = stepdown_value( item.quantity, 4, 4, 12, 12 );

    if (you.species != SP_VAMPIRE)
        item.flags    &= ~(ISFLAG_THROWN | ISFLAG_DROPPED);

    // happens after the corpse has been butchered
    if (monster_descriptor(mons_class, MDSC_LEAVES_HIDE) && !one_chance_in(3))
        create_monster_hide(mons_class);
}

bool can_bottle_blood_from_corpse(int mons_type)
{
    if (you.species != SP_VAMPIRE || you.experience_level < 6
        || !mons_has_blood(mons_type))
    {
        return (false);
    }

    int chunk_type = mons_corpse_effect( mons_type );
    if (chunk_type == CE_CLEAN || chunk_type == CE_CONTAMINATED)
        return (true);
        
    return (false);
}

void turn_corpse_into_blood_potions( item_def &item )
{
    ASSERT( item.base_type == OBJ_CORPSES );
    ASSERT( !food_is_rotten(item) );

    const int mons_class = item.plus;
    ASSERT( can_bottle_blood_from_corpse(mons_class) );

    item.base_type = OBJ_POTIONS;
    item.sub_type  = POT_BLOOD;
    item.colour    = RED;
    item.special   = (item.special - 80) * 10; // potion's age

    // max. amount is about one third of the max. amount for chunks
    const int max_chunks = mons_weight( mons_class ) / 150;
    item.quantity  = 1 + random2( max_chunks/3 );
    item.quantity  = stepdown_value( item.quantity, 2, 2, 6, 6 );

    item.flags    &= ~(ISFLAG_THROWN | ISFLAG_DROPPED);

    // happens after the blood has been bottled
    if (monster_descriptor(mons_class, MDSC_LEAVES_HIDE) && !one_chance_in(3))
        create_monster_hide(mons_class);
}

// A variation of the mummy curse: for potions of blood (vital for Vampires!)
// split the stack and only turn part of it into POT_DECAY.
void split_blood_potions_into_decay( int obj, int amount )
{
    ASSERT(obj != -1);
    item_def potion = you.inv[obj];

    ASSERT(is_valid_item(potion));
    ASSERT(potion.base_type == OBJ_POTIONS && potion.sub_type == POT_BLOOD);
    ASSERT(amount <= potion.quantity);

    if (amount <= 0)
        amount = random2(potion.quantity) + 1;

    // if entire stack affected just change subtype
    if (amount == potion.quantity)
    {
        you.inv[obj].sub_type = POT_DECAY;
        return;
    }

    // try to merge into existing stacks of decayed potions
    for (int m = 0; m < ENDOFPACK; m++)
    {
         if (you.inv[m].base_type == OBJ_POTIONS
             && you.inv[m].sub_type == POT_DECAY)
         {
             inc_inv_item_quantity( m, amount );
             dec_inv_item_quantity( obj, amount);

             return;
         }
    }

    // only bother creating a distinct stack of potions
    // if it won't get destroyed right away
    if (!grid_destroys_items(grd[you.x_pos][you.y_pos]))
    {
        item_def potion2;
        potion2.base_type = OBJ_POTIONS;
        potion2.sub_type  = POT_DECAY;
        potion2.plus      = potion.plus; // are these even needed?
        potion2.plus2     = potion.plus2;
        potion2.flags     = potion.flags;
        potion2.quantity  = amount;
        potion2.special   = 0;

        switch (random2(4))
        {
        case 0:
            potion2.colour = RED;
            break;
        case 1:
            potion2.colour = BROWN;
            break;
        case 2:
            potion2.colour = GREEN;
            break;
        case 3:
            potion2.colour = LIGHTRED;
            break;
        }

        copy_item_to_grid( potion2, you.x_pos, you.y_pos );
   }
   // is decreased even if the decay stack goes splat
   dec_inv_item_quantity(obj, amount);
}

// checks whether the player or a monster is capable of bleeding
static bool victim_can_bleed(int montype)
{
    if (montype == -1) // player
    {
        if (you.is_undead && (you.species != SP_VAMPIRE
                              || you.hunger_state < HS_FULL))
        {
            return (false);
        }

        int tran = you.attribute[ATTR_TRANSFORMATION];
        if (tran == TRAN_STATUE || tran == TRAN_ICE_BEAST
            || tran == TRAN_AIR || tran == TRAN_LICH
            || tran == TRAN_SPIDER) // monster spiders don't bleed either
        {
            return (false);
        }
        return (true);
    }

    // now check monsters
    return (mons_has_blood(montype));
}

static bool allow_bleeding_on_square(int x, int y)
{
    // no bleeding onto sanctuary ground, please
    // also not necessary if already covered in blood
    if (env.map[x][y].property != FPROP_NONE)
        return (false);

    // no spattering into lava or water
    if (grd[x][y] >= DNGN_LAVA && grd[x][y] < DNGN_FLOOR)
        return (false);

    // the good gods like to keep their altars pristine
    if (is_good_god(grid_altar_god(grd[x][y])))
        return (false);
            
    return (true);
}

static void maybe_bloodify_square(int x, int y, int amount, bool spatter = false)
{
    if (amount < 1)
        return;

    if (!spatter && !allow_bleeding_on_square(x,y))
        return;

    if (amount > random2(20))
    {
#ifdef DEBUG_DIAGNOSTICS
        mprf(MSGCH_DIAGNOSTICS,
             "might bleed now; square: (%d, %d); amount = %d",
             x, y, amount);
#endif
        if (allow_bleeding_on_square(x,y))
            env.map[x][y].property = FPROP_BLOODY;

        if (spatter)
        {
            // smaller chance of spattering surrounding squares
            for (int i=-1;i<=1;i++)
                for (int j=-1;j<=1;j++)
                {
                     if (i == 0 && j == 0) // current square
                         continue;
                         
                     // spattering onto walls etc. less likely
                     if (grd[x+i][y+j] < DNGN_MINMOVE && one_chance_in(3))
                         continue;

                     maybe_bloodify_square(x+i, y+j, amount/10);
                }
        }
    }
}

// currently flavour only: colour ground (and possibly adjacent squares) red
// "damage" depends on damage taken (or hitpoints, if damage higher),
// or, for sacrifices, on the number of chunks possible to get out of a corpse
void bleed_onto_floor(int x, int y, int montype, int damage, bool spatter)
{
    if (!victim_can_bleed(montype))
        return;

    maybe_bloodify_square(x, y, damage, spatter);
}

void search_around( bool only_adjacent )
{
    int i;

    // Traps and doors stepdown skill:
    // skill/(2x-1) for squares at distance x
    int max_dist = (you.skills[SK_TRAPS_DOORS] + 1) / 2;
    if ( max_dist > 5 )
        max_dist = 5;
    if ( max_dist > 1 && only_adjacent )
        max_dist = 1;
    if ( max_dist < 1 )
        max_dist = 1;

    for ( int srx = you.x_pos - max_dist; srx <= you.x_pos + max_dist; ++srx )
    {
        for ( int sry=you.y_pos - max_dist; sry<=you.y_pos + max_dist; ++sry )
        {
            // must have LOS, with no translucent walls in the way.
            if ( see_grid_no_trans(srx,sry) )
            {
                // maybe we want distance() instead of grid_distance()?
                int dist = grid_distance(srx, sry, you.x_pos, you.y_pos);

                // don't exclude own square; may be levitating
                if (dist == 0)
                    ++dist;

                // making this harsher by removing the old +1
                int effective = you.skills[SK_TRAPS_DOORS] / (2*dist - 1);

                if (grd[srx][sry] == DNGN_SECRET_DOOR &&
                    random2(17) <= effective)
                {
                    reveal_secret_door(srx, sry);
                    mpr("You found a secret door!");
                    exercise(SK_TRAPS_DOORS, ((coinflip()) ? 2 : 1));
                }

                if (grd[srx][sry] == DNGN_UNDISCOVERED_TRAP &&
                    random2(17) <= effective)
                {
                    i = trap_at_xy(srx, sry);

                    if (i != -1)
                    {
                        grd[srx][sry] = trap_category(env.trap[i].type);
                        mpr("You found a trap!");
                        exercise(SK_TRAPS_DOORS, ((coinflip()) ? 2 : 1));
                    }
                    else
                    {
                        // Maybe we shouldn't kill the trap for debugging
                        // purposes - oh well.
                        grd[srx][sry] = DNGN_FLOOR;
                        mpr("You found a buggy trap! It vanishes!");
                    }
                }
            }
        }
    }

    return;
}                               // end search_around()

void in_a_cloud()
{
    int cl = env.cgrid[you.x_pos][you.y_pos];
    int hurted = 0;
    int resist;

    if (you.duration[DUR_CONDENSATION_SHIELD] > 0)
    {
        mpr("Your icy shield dissipates!", MSGCH_DURATION);
        you.duration[DUR_CONDENSATION_SHIELD] = 0;
        you.redraw_armour_class = 1;
    }

    switch (env.cloud[cl].type)
    {
    case CLOUD_FIRE:
        if (you.duration[DUR_FIRE_SHIELD])
            return;

        mpr("You are engulfed in roaring flames!");

        resist = player_res_fire();

        if (resist <= 0)
        {
            hurted += ((random2avg(23, 3) + 10) * you.time_taken) / 10;

            if (resist < 0)
                hurted += ((random2avg(14, 2) + 3) * you.time_taken) / 10;

            hurted -= random2(player_AC());

            if (hurted < 1)
                hurted = 0;
            else
                ouch( hurted, cl, KILLED_BY_CLOUD, "flame" );
        }
        else
        {
            canned_msg(MSG_YOU_RESIST);
            hurted += ((random2avg(23, 3) + 10) * you.time_taken) / 10;
            hurted /= (1 + resist * resist);
            ouch( hurted, cl, KILLED_BY_CLOUD, "flame" );
        }
        expose_player_to_element(BEAM_FIRE, 7);
        break;

    case CLOUD_STINK:
        // If you don't have to breathe, unaffected
        mpr("You are engulfed in noxious fumes!");
        if (player_res_poison())
            break;

        hurted += (random2(3) * you.time_taken) / 10;
        if (hurted < 1)
            hurted = 0;
        else
            ouch( (hurted * you.time_taken) / 10, cl, KILLED_BY_CLOUD, 
                    "noxious fumes" );

        if (1 + random2(27) >= you.experience_level)
        {
            mpr("You choke on the stench!");
            confuse_player( (coinflip() ? 3 : 2) );
        }
        break;

    case CLOUD_COLD:
        mpr("You are engulfed in freezing vapours!");

        resist = player_res_cold();

        if (resist <= 0)
        {
            hurted += ((random2avg(23, 3) + 10) * you.time_taken) / 10;

            if (resist < 0)
                hurted += ((random2avg(14, 2) + 3) * you.time_taken) / 10;

            hurted -= random2(player_AC());
            if (hurted < 0)
                hurted = 0;

            ouch( hurted, cl, KILLED_BY_CLOUD, "freezing vapour" );
        }
        else
        {
            canned_msg(MSG_YOU_RESIST);
            hurted += ((random2avg(23, 3) + 10) * you.time_taken) / 10;
            hurted /= (1 + resist * resist);
            ouch( hurted, cl, KILLED_BY_CLOUD, "freezing vapour" );
        }
        expose_player_to_element(BEAM_COLD, 7);
        break;

    case CLOUD_POISON:
        // If you don't have to breathe, unaffected
        mpr("You are engulfed in poison gas!");
        if (!player_res_poison())
        {
            ouch( (random2(10) * you.time_taken) / 10, cl, KILLED_BY_CLOUD, 
                  "poison gas" );
            poison_player(1);
        }
        break;

    case CLOUD_GREY_SMOKE:
    case CLOUD_BLUE_SMOKE:
    case CLOUD_PURP_SMOKE:
    case CLOUD_BLACK_SMOKE:
        mpr("You are engulfed in a cloud of smoke!");
        break;

    case CLOUD_STEAM:
    {
        mpr("You are engulfed in a cloud of scalding steam!");
        if (player_res_steam() > 0)
        {
            mpr("It doesn't seem to affect you.");
            return;
        }

        const int base_dam = steam_cloud_damage(env.cloud[cl]);
        hurted += (random2avg(base_dam, 2) * you.time_taken) / 10;

        const int res_fire = player_res_fire();
        if (res_fire < 0)
            hurted += (random2(base_dam / 2 + 1) * you.time_taken) / 10;
        else if (res_fire)
            hurted /= 1 + (res_fire / 2);

        if (hurted < 0)
            hurted = 0;

        ouch( (hurted * you.time_taken) / 10, cl, KILLED_BY_CLOUD,
              "steam" );
        break;
    }

    case CLOUD_MIASMA:
        mpr("You are engulfed in a dark miasma.");

        if (player_prot_life() > random2(3))
            return;

        poison_player(1);

        hurted += (random2avg(12, 3) * you.time_taken) / 10;    // 3

        if (hurted < 0)
            hurted = 0;

        ouch( hurted, cl, KILLED_BY_CLOUD, "foul pestilence" );
        potion_effect(POT_SLOWING, 5);

        if (you.hp_max > 4 && coinflip())
            rot_hp(1);

        break;
    default:
        break;
    }

    return;
}                               // end in_a_cloud()

void curare_hits_player(int agent, int degree)
{
    const bool res_poison = player_res_poison();

    poison_player(degree);

    if (!player_res_asphyx())
    {
        int hurted = roll_dice(2, 6);
        // Note that the hurtage is halved by poison resistance.
        if (res_poison)
            hurted /= 2;

        if (hurted)
        {
            mpr("You feel difficulty breathing.");
            ouch( hurted, agent, KILLED_BY_CURARE, "curare-induced apnoea" );
        }
        potion_effect(POT_SLOWING, 2 + random2(4 + degree));
    }
}

void merfolk_start_swimming(void)
{
    if (you.attribute[ATTR_TRANSFORMATION] != TRAN_NONE)
        untransform();

    std::set<equipment_type> removed;
    removed.insert(EQ_BOOTS);

    // Perhaps a bit to easy for the player, but we allow merfolk
    // to slide out of heavy body armour freely when entering water,
    // rather than handling emcumbered swimming. -- bwr
    if (!player_light_armour())
        removed.insert(EQ_BODY_ARMOUR);

    remove_equipment(removed);
    you.redraw_evasion = true;
}

static void exit_stair_message(dungeon_feature_type stair, bool /* going_up */)
{
    if (grid_is_rock_stair(stair))
        mpr("The hatch slams shut behind you.");
}

static void climb_message(dungeon_feature_type stair, bool going_up,
                          level_area_type old_level_type)
{
    if (old_level_type != LEVEL_DUNGEON)
        return;

    if (grid_is_portal(stair))
        mpr("The world spins around you as you enter the gateway.");
    else if (grid_is_rock_stair(stair))
    {
        if (going_up)
            mpr("A mysterious force pulls you upwards.");
        else
        {
            mprf("You %s downwards.", you.flight_mode() == FL_FLY? "fly" :
                                     (player_is_airborne()? "float" :
                                      "slide"));
        }
    }
    else
    {
        mprf("You %s %swards.", you.flight_mode() == FL_FLY? "fly" :
                                (player_is_airborne()? "float" : "climb"),
                                going_up? "up": "down");
    }
}

static void leaving_level_now()
{
    // Note the name ahead of time because the events may cause
    // markers to be discarded.
    const std::string newtype =
        env.markers.property_at(you.pos(), MAT_ANY, "dst");
    
    dungeon_events.fire_position_event(DET_PLAYER_CLIMBS, you.pos());
    dungeon_events.fire_event(DET_LEAVING_LEVEL);

    you.level_type_name = newtype;
}

static void set_entry_cause(entry_cause_type default_cause,
                            level_area_type old_level_type)
{
    ASSERT(default_cause != NUM_ENTRY_CAUSE_TYPES);

    if (!(old_level_type != you.level_type
          || you.entry_cause == EC_UNKNOWN))
    {
        return;
    }

    if (crawl_state.is_god_acting())
    {
        if (crawl_state.is_god_retribution())
            you.entry_cause = EC_GOD_RETRIUBTION;
        else
            you.entry_cause = EC_GOD_ACT;

        you.entry_cause_god = crawl_state.which_god_acting();
    }
    else if (default_cause != EC_UNKNOWN)
    {
        you.entry_cause     = default_cause;
        you.entry_cause_god = GOD_NO_GOD;
    }
    else
    {
        you.entry_cause     = EC_SELF_EXPLICIT;
        you.entry_cause_god = GOD_NO_GOD;
    }
}

static int runes_in_pack()
{
    int num_runes = 0;

    for (int i = 0; i < ENDOFPACK; i++)
    {
        if (is_valid_item( you.inv[i] )
            && you.inv[i].base_type == OBJ_MISCELLANY
            && you.inv[i].sub_type == MISC_RUNE_OF_ZOT)
        {
            num_runes += you.inv[i].quantity;
        }
    }

    return num_runes;
}

void up_stairs(dungeon_feature_type force_stair,
               entry_cause_type entry_cause)
{
    dungeon_feature_type stair_find =
        force_stair? force_stair : grd[you.x_pos][you.y_pos];
    const branch_type     old_where      = you.where_are_you;
    const level_area_type old_level_type = you.level_type;

    if (stair_find == DNGN_ENTER_SHOP)
    {
        shop();
        return;
    }

    // probably still need this check here (teleportation) -- bwr
    if (grid_stair_direction(stair_find) != CMD_GO_UPSTAIRS)
    {
        if (stair_find == DNGN_STONE_ARCH)
            mpr("There is nothing on the other side of the stone arch.");
        else
            mpr("You can't go up here.");
        return;
    }

    level_id  old_level_id    = level_id::current();
    LevelInfo &old_level_info = travel_cache.get_level_info(old_level_id);

    // Does the next level have a warning annotation?
    coord_def pos(you.x_pos, you.y_pos);
    level_id  next_level_id = level_id::get_next_level_id(pos);

    crawl_state.level_annotation_shown = false;

    if (level_annotation_has("!", next_level_id)
        && next_level_id != level_id::current()
        && next_level_id.level_type == LEVEL_DUNGEON && !force_stair)
    {
        mpr("Warning: level annotation for next level is:", MSGCH_PROMPT);
        mpr(get_level_annotation(next_level_id).c_str(), MSGCH_PLAIN, YELLOW);

        if (!yesno("Enter next level anyway?", true, 0, true, false))
        {
            interrupt_activity( AI_FORCE_INTERRUPT );
            return;
        }

        crawl_state.level_annotation_shown = true;
    }

    // Since the overloaded message set turn_is_over, I'm assuming that
    // the overloaded character makes an attempt... so we're doing this
    // check before that one. -- bwr
    if (!player_is_airborne()
        && you.duration[DUR_CONF] 
        && (stair_find >= DNGN_STONE_STAIRS_UP_I 
            && stair_find <= DNGN_STONE_STAIRS_UP_III)
        && random2(100) > you.dex)
    {
        mpr("In your confused state, you trip and fall back down the stairs.");

        ouch( roll_dice( 3 + you.burden_state, 5 ), 0, 
              KILLED_BY_FALLING_DOWN_STAIRS );

        you.turn_is_over = true;
        return;
    }

    if (you.burden_state == BS_OVERLOADED
        && !grid_is_rock_stair(stair_find))
    {
        mpr("You are carrying too much to climb upwards.");
        you.turn_is_over = true;
        return;
    }

    if (you.your_level == 0
          && !yesno("Are you sure you want to leave the Dungeon?", false, 'n'))
    {
        mpr("Alright, then stay!");
        return;
    }

    // Checks are done, the character is committed to moving between levels.
    leaving_level_now();
    
    int old_level  = you.your_level;

    // Interlevel travel data:
    const bool collect_travel_data = can_travel_interlevel();

    if (collect_travel_data)
        old_level_info.update();

    // Make sure we return to our main dungeon level... labyrinth entrances
    // in the abyss or pandemonium are a bit trouble (well the labyrinth does
    // provide a way out of those places, its really not that bad I suppose)
    if (level_type_exits_up(you.level_type))
        you.level_type = LEVEL_DUNGEON;

    you.your_level--;

    int i = 0;

    if (you.your_level < 0)
    {
        mpr("You have escaped!");

        for (i = 0; i < ENDOFPACK; i++)
        {
            if (is_valid_item( you.inv[i] ) 
                && you.inv[i].base_type == OBJ_ORBS)
            {
                ouch(INSTANT_DEATH, 0, KILLED_BY_WINNING);
            }
        }

        ouch(INSTANT_DEATH, 0, KILLED_BY_LEAVING);
    }

    you.prev_targ  = MHITNOT;
    you.pet_target = MHITNOT;

    you.prev_grd_targ = coord_def(0, 0);

    if (player_in_branch( BRANCH_VESTIBULE_OF_HELL ))
    {
        mpr("Thank you for visiting Hell. Please come again soon.");
        you.where_are_you = BRANCH_MAIN_DUNGEON;
        you.your_level = you.hell_exit;
        stair_find = DNGN_STONE_STAIRS_UP_I;
    }

    if (player_in_hell())
    {
        you.where_are_you = BRANCH_VESTIBULE_OF_HELL;
        you.your_level = 27;
    }

    // did we take a branch stair?
    for ( i = 0; i < NUM_BRANCHES; ++i )
    {
        if ( branches[i].exit_stairs == stair_find )
        {
            you.where_are_you = branches[i].parent_branch;

            // If leaving a branch which wasn't generated in this
            // particular game (like the Swamp or Shoals), then
            // its startdepth is set to -1; compensate for that,
            // so we don't end up on "level -1".
            if (branches[i].startdepth == -1)
                you.your_level += 2;
            break;
        }
    }

    const dungeon_feature_type stair_taken = stair_find;

    if (you.flight_mode() == FL_FLY)
        mpr("You fly upwards.");
    else if (you.flight_mode() == FL_LEVITATE)
        mpr("You float upwards... And bob straight up to the ceiling!");
    else
        climb_message(stair_find, true, old_level_type);

    exit_stair_message(stair_find, true);

    if (old_where != you.where_are_you && you.level_type == LEVEL_DUNGEON)
        mprf("Welcome back to %s!",
             branches[you.where_are_you].longname);

#ifdef USE_TILE
    const bool newlevel =
#endif
        load(stair_taken, LOAD_ENTER_LEVEL, old_level_type,
             old_level, old_where);

    set_entry_cause(entry_cause, old_level_type);
    entry_cause = you.entry_cause;

    you.turn_is_over = true;

#ifdef USE_TILE
    TileNewLevel(newlevel);
#endif // USE_TILE

    save_game_state();

    new_level();

    viewwindow(1, true);

    // Left Zot without enough runes to get back in (probably because
    // of dropping some runes within Zot), but need to get back in Zot
    // to get the Orb?  Xom finds that funny.
    if (stair_find == DNGN_RETURN_FROM_ZOT
        && branches[BRANCH_HALL_OF_ZOT].branch_flags & BFLAG_HAS_ORB)
    {
        int runes_avail = you.attribute[ATTR_UNIQUE_RUNES]
            + you.attribute[ATTR_DEMONIC_RUNES]
            + you.attribute[ATTR_ABYSSAL_RUNES]
            - you.attribute[ATTR_RUNES_IN_ZOT];

        if (runes_avail < NUMBER_OF_RUNES_NEEDED)
            xom_is_stimulated(255, "Xom snickers loudly.", true);
    }

    if (you.skills[SK_TRANSLOCATIONS] > 0 && !allow_control_teleport( true ))
        mpr( "You sense a powerful magical force warping space.", MSGCH_WARN );

    // Tell stash-tracker and travel that we've changed levels.
    trackers_init_new_level(true);
    
    if (collect_travel_data)
    {
        // Update stair information for the stairs we just ascended, and the
        // down stairs we're currently on.
        level_id  new_level_id    = level_id::current();

        if (can_travel_interlevel())
        {
            LevelInfo &new_level_info = 
                        travel_cache.get_level_info(new_level_id);
            new_level_info.update();

            int stair_x = you.x_pos, stair_y = you.y_pos;

            // First we update the old level's stair.
            level_pos lp;
            lp.id  = new_level_id;
            lp.pos.x = you.x_pos; 
            lp.pos.y = you.y_pos;

            bool guess = false;
            // Ugly hack warning:
            // The stairs in the Vestibule of Hell exhibit special behaviour:
            // they always lead back to the dungeon level that the player
            // entered the Vestibule from. This means that we need to pretend
            // we don't know where the upstairs from the Vestibule go each time
            // we take it. If we don't, interlevel travel may try to use portals
            // to Hell as shortcuts between dungeon levels, which won't work,
            // and will confuse the dickens out of the player (well, it confused
            // the dickens out of me when it happened).
            if (new_level_id.branch == BRANCH_MAIN_DUNGEON &&
                    old_level_id.branch == BRANCH_VESTIBULE_OF_HELL)
            {
                lp.id.depth = -1;
                lp.pos.x = lp.pos.y = -1;
                guess = true;
            }

            old_level_info.update_stair(you.x_pos, you.y_pos, lp, guess);

            // We *guess* that going up a staircase lands us on a downstair,
            // and that we can descend that downstair and get back to where we
            // came from. This assumption is guaranteed false when climbing out
            // of one of the branches of Hell.
            if (new_level_id.branch != BRANCH_VESTIBULE_OF_HELL)
            {
                // Set the new level's stair, assuming arbitrarily that going
                // downstairs will land you on the same upstairs you took to
                // begin with (not necessarily true).
                lp.id = old_level_id;
                lp.pos.x = stair_x;
                lp.pos.y = stair_y;
                new_level_info.update_stair(you.x_pos, you.y_pos, lp, true);
            }
        }
    }
    request_autopickup();
}                               // end up_stairs()

void down_stairs( int old_level, dungeon_feature_type force_stair,
                  entry_cause_type entry_cause )
{
    int i;
    const level_area_type      old_level_type = you.level_type;
    const dungeon_feature_type stair_find =
        force_stair? force_stair : grd[you.x_pos][you.y_pos];

    branch_type old_where = you.where_are_you;

    bool shaft = ((!force_stair
                   && trap_type_at_xy(you.x_pos, you.y_pos) == TRAP_SHAFT)
                  || force_stair == DNGN_TRAP_NATURAL);
    level_id shaft_dest;
    int      shaft_level = -1;

#ifdef SHUT_LABYRINTH
    if (stair_find == DNGN_ENTER_LABYRINTH)
    {
        mpr("Sorry, this section of the dungeon is closed for fumigation.");
        mpr("Try again next release.");
        return;
    }
#endif

    // probably still need this check here (teleportation) -- bwr
    if (grid_stair_direction(stair_find) != CMD_GO_DOWNSTAIRS && !shaft)
    {
        if (stair_find == DNGN_STONE_ARCH)
            mpr("There is nothing on the other side of the stone arch.");
        else
            mpr( "You can't go down here!" );
        return;
    }

    if (stair_find >= DNGN_ENTER_LABYRINTH
        && stair_find <= DNGN_ROCK_STAIRS_DOWN
        && player_in_branch( BRANCH_VESTIBULE_OF_HELL ))
    {
        mpr("A mysterious force prevents you from descending the staircase.");
        return;
    }                           /* down stairs in vestibule are one-way */

    if (stair_find == DNGN_STONE_ARCH)
    {
        mpr("There is nothing on the other side of the stone arch.");
        return;
    }

    if (!force_stair && you.flight_mode() == FL_LEVITATE)
    {
        mpr("You're floating high up above the floor!");
        return;
    }

    if (shaft)
    {
        bool known_trap = (grd[you.x_pos][you.y_pos] != DNGN_UNDISCOVERED_TRAP
                           && !force_stair);

        if (!known_trap && !force_stair)
        {
            mpr("You can't go down here!");
            return;
        }

        if (you.flight_mode() == FL_LEVITATE && !force_stair)
        {
            if (known_trap)
                mpr("You can't fall through a shaft while levitating.");
            return;
        }

        if (!is_valid_shaft_level())
        {
            if (known_trap)
                mpr("Strange, the shaft doesn't seem to lead anywhere.");
            return;
        }

        shaft_dest = you.shaft_dest();
        if (shaft_dest == level_id::current())
        {
            if (known_trap)
                mpr("Strange, the shaft doesn't seem to lead anywhere.");
            return;
        }
        shaft_level = absdungeon_depth(shaft_dest.branch,
                                       shaft_dest.depth);

        if (you.flight_mode() != FL_FLY || force_stair)
            mpr("You fall through a shaft!");
    }

    // All checks are done, the player is on the move now.

    // Fire level-leaving trigger.
    leaving_level_now();
    
#ifdef DGL_MILESTONES
    if (!force_stair)
    {
        // Not entirely accurate - the player could die before
        // reaching the Abyss.
        if (grd[you.x_pos][you.y_pos] == DNGN_ENTER_ABYSS)
            mark_milestone("abyss.enter", "entered the Abyss!");
        else if (grd[you.x_pos][you.y_pos] == DNGN_EXIT_ABYSS)
            mark_milestone("abyss.exit", "escaped from the Abyss!");
    }
#endif

    // [ds] Descending into the Labyrinth increments your_level. Going
    // downstairs from a labyrinth implies that you've been banished (or been
    // sent to Pandemonium somehow). Decrementing your_level here is needed
    // to fix this buggy sequence: D:n -> Labyrinth -> Abyss -> D:(n+1).
    if (level_type_exits_up(you.level_type))
        you.your_level--;

    if (stair_find == DNGN_ENTER_ZOT)
    {
        const int num_runes = runes_in_pack();

        if (num_runes < NUMBER_OF_RUNES_NEEDED)
        {
            switch (NUMBER_OF_RUNES_NEEDED)
            {
            case 1:
                mpr("You need one more Rune to enter this place.");
                break;

            default:
                mprf( "You need at least %d Runes to enter this place.",
                      NUMBER_OF_RUNES_NEEDED );
            }
            return;
        }
    }

    // Does the next level have a warning annotation?
    coord_def pos = you.pos();
    level_id  next_level_id = level_id::get_next_level_id(pos);

    crawl_state.level_annotation_shown = false;

    if (level_annotation_has("!", next_level_id)
        && next_level_id != level_id::current()
        && next_level_id.level_type == LEVEL_DUNGEON && !force_stair)
    {
        mpr("Warning: level annotation for next level is:", MSGCH_PROMPT);
        mpr(get_level_annotation(next_level_id).c_str(), MSGCH_PLAIN, YELLOW);

        if (!yesno("Enter next level anyway?", true, 0, true, false))
        {
            interrupt_activity( AI_FORCE_INTERRUPT );
            return;
        }

        crawl_state.level_annotation_shown = true;
    }

    // Interlevel travel data:
    bool collect_travel_data = can_travel_interlevel();

    level_id  old_level_id    = level_id::current();
    LevelInfo &old_level_info = travel_cache.get_level_info(old_level_id);
    int stair_x = you.x_pos, stair_y = you.y_pos;
    if (collect_travel_data)
        old_level_info.update();

    // Preserve abyss uniques now, since this Abyss level will be deleted.
    if (you.level_type == LEVEL_ABYSS)
        save_abyss_uniques();
    
    if (you.level_type != LEVEL_DUNGEON
        && (you.level_type != LEVEL_PANDEMONIUM
            || stair_find != DNGN_TRANSIT_PANDEMONIUM))
    {
        you.level_type = LEVEL_DUNGEON;
    }

    you.prev_targ  = MHITNOT;
    you.pet_target = MHITNOT;

    you.prev_grd_targ = coord_def(0, 0);

    if (stair_find == DNGN_ENTER_HELL)
    {
        you.where_are_you = BRANCH_VESTIBULE_OF_HELL;
        you.hell_exit = you.your_level;

        you.your_level = 26;
    }

    // welcome message
    // try to find a branch stair
    bool entered_branch = false;
    for ( i = 0; i < NUM_BRANCHES; ++i )
    {
        if ( branches[i].entry_stairs == stair_find )
        {
            entered_branch = true;
            you.where_are_you = branches[i].id;
            break;
        }
    }

    if (stair_find == DNGN_ENTER_LABYRINTH)
        dungeon_terrain_changed(you.pos(), DNGN_FLOOR);

    if (stair_find == DNGN_ENTER_LABYRINTH)
    {
        you.level_type = LEVEL_LABYRINTH;
    }
    else if (stair_find == DNGN_ENTER_ABYSS)
    {
        you.level_type = LEVEL_ABYSS;
    }
    else if (stair_find == DNGN_ENTER_PANDEMONIUM)
    {
        you.level_type = LEVEL_PANDEMONIUM;
    }
    else if (stair_find == DNGN_ENTER_PORTAL_VAULT)
    {
        you.level_type = LEVEL_PORTAL_VAULT;
    }

    // When going downstairs into a special level, delete any previous
    // instances of it
    if (you.level_type != LEVEL_DUNGEON)
    {
        std::string lname = make_filename(you.your_name, you.your_level,
                                          you.where_are_you,
                                          you.level_type, false );
#if DEBUG_DIAGNOSTICS
        mprf( MSGCH_DIAGNOSTICS, "Deleting: %s", lname.c_str() );
#endif
        unlink(lname.c_str());
    }

    if (stair_find == DNGN_EXIT_ABYSS || stair_find == DNGN_EXIT_PANDEMONIUM)
    {
        mpr("You pass through the gate.");
        if (!(you.wizard && crawl_state.is_replaying_keys()))
            more();
    }

    if (old_level_type != you.level_type && you.level_type == LEVEL_DUNGEON)
        mprf("Welcome back to %s!", branches[you.where_are_you].longname);

    if (!player_is_airborne()
        && you.duration[DUR_CONF] 
        && !grid_is_rock_stair(stair_find)
        && random2(100) > you.dex)
    {
        mpr("In your confused state, you trip and fall down the stairs.");

        // Nastier than when climbing stairs, but you'll aways get to 
        // your destination, -- bwr
        ouch( roll_dice( 6 + you.burden_state, 10 ), 0, 
              KILLED_BY_FALLING_DOWN_STAIRS );
    }

    if (shaft)
    {
        you.your_level    = shaft_level;
        you.where_are_you = shaft_dest.branch;
    }
    else if (you.level_type == LEVEL_DUNGEON)
        you.your_level++;

    dungeon_feature_type stair_taken = stair_find;

    if (you.level_type == LEVEL_ABYSS)
        stair_taken = DNGN_FLOOR;

    if (you.level_type == LEVEL_PANDEMONIUM)
        stair_taken = DNGN_TRANSIT_PANDEMONIUM;

    if (shaft)
        stair_taken = DNGN_ROCK_STAIRS_DOWN;

    switch (you.level_type)
    {
    case LEVEL_LABYRINTH:
        mpr("You enter a dark and forbidding labyrinth.");
        break;

    case LEVEL_ABYSS:
        if (!force_stair)
            mpr("You enter the Abyss!");
        mpr("To return, you must find a gate leading back.");
        break;

    case LEVEL_PANDEMONIUM:
        if (old_level_type == LEVEL_PANDEMONIUM)
            mpr("You pass into a different region of Pandemonium.");
        else
        {
            mpr("You enter the halls of Pandemonium!");
            mpr("To return, you must find a gate leading back.");
        }
        break;

    default:
        if (shaft)
        {
            if (you.flight_mode() == FL_FLY && !force_stair)
                mpr("You dive down through the shaft.");
            handle_items_on_shaft(you.x_pos, you.y_pos, false);
        }
        else
            climb_message(stair_find, false, old_level_type);
        break;
    }

    if (!shaft)
        exit_stair_message(stair_find, false);

    if (entered_branch)
    {
        if ( branches[you.where_are_you].entry_message )
            mpr(branches[you.where_are_you].entry_message);
        else
            mprf("Welcome to %s!", branches[you.where_are_you].longname);
    }

    if (stair_find == DNGN_ENTER_HELL)
    {
        mpr("Welcome to Hell!");
        mpr("Please enjoy your stay.");

        // Kill -more- prompt if we're traveling.
        if (!you.running && !force_stair)
            more();
    }

    const bool newlevel =
        load(stair_taken, LOAD_ENTER_LEVEL, old_level_type,
             old_level, old_where);

    set_entry_cause(entry_cause, old_level_type);
    entry_cause = you.entry_cause;

    if (newlevel)
    {
        switch(you.level_type)
        {
        case LEVEL_DUNGEON:
            xom_is_stimulated(49);
            break;

        case LEVEL_PORTAL_VAULT:
            // Portal vaults aren't as interesting.
            xom_is_stimulated(25);
            break;

        case LEVEL_LABYRINTH:
            // Finding the way out of a labyrinth interests Xom.
            xom_is_stimulated(98);
            break;

        case LEVEL_ABYSS:
        case LEVEL_PANDEMONIUM:
        {
            // Paranoia
            if (old_level_type == you.level_type)
                break;

            PlaceInfo &place_info = you.get_place_info();

            // Entering voluntarily only stimulates Xom if you've never
            // been there before
            if ((place_info.num_visits == 1 && place_info.levels_seen == 1)
                || entry_cause != EC_SELF_EXPLICIT)
            {
                if (crawl_state.is_god_acting())
                    xom_is_stimulated(255);
                else if (entry_cause == EC_SELF_EXPLICIT)
                {
                    // Entering Pandemonium or the Abyss for the first
                    // time *voluntarily* stimulates Xom much more than
                    // entering a normal dungeon level for the first time.
                    xom_is_stimulated(128, XM_INTRIGUED);
                }
                else if (entry_cause == EC_SELF_RISKY)
                    xom_is_stimulated(128);
                else
                    xom_is_stimulated(255);
            }

            break;
        }

        default:
            ASSERT(false);
        }
    }

    unsigned char pc = 0;
    unsigned char pt = random2avg(28, 3);

    if (shaft)
    {
        you.your_level    = shaft_level;
        you.where_are_you = shaft_dest.branch;
    }
    else if (level_type_exits_up(you.level_type))
        you.your_level++;
    else if (level_type_exits_down(you.level_type)
             && !level_type_exits_down(old_level_type))
        you.your_level--;
    

    switch (you.level_type)
    {
    case LEVEL_ABYSS:
        grd[you.x_pos][you.y_pos] = DNGN_FLOOR;

        init_pandemonium();     /* colours only */

        if (player_in_hell())
        {
            you.where_are_you = BRANCH_MAIN_DUNGEON;
            you.your_level = you.hell_exit - 1;
        }
        break;

    case LEVEL_PANDEMONIUM:
        if (old_level_type == LEVEL_PANDEMONIUM)
        {
            init_pandemonium();
            for (pc = 0; pc < pt; pc++)
                pandemonium_mons();
        }
        else
        {
            init_pandemonium();

            for (pc = 0; pc < pt; pc++)
                pandemonium_mons();

            if (player_in_hell())
            {
                you.where_are_you = BRANCH_MAIN_DUNGEON;
                you.hell_exit = 26;
                you.your_level = 26;
            }
        }
        break;

    default:
        break;
    }

#ifdef USE_TILE
    TileNewLevel(newlevel);
#endif

    you.turn_is_over = true;

    save_game_state();

    new_level();

    // clear list of beholding monsters
    if (you.duration[DUR_BEHELD])
    {
        you.beheld_by.clear();
        you.duration[DUR_BEHELD] = 0;
    }
    
    viewwindow(1, true);

    if (you.skills[SK_TRANSLOCATIONS] > 0 && !allow_control_teleport( true ))
        mpr( "You sense a powerful magical force warping space.", MSGCH_WARN );

    trackers_init_new_level(true);
    if (collect_travel_data)
    {
        // Update stair information for the stairs we just descended, and the
        // upstairs we're currently on.
        level_id  new_level_id    = level_id::current();

        if (can_travel_interlevel())
        {
            LevelInfo &new_level_info = 
                            travel_cache.get_level_info(new_level_id);
            new_level_info.update();

            // First we update the old level's stair.
            level_pos lp;
            lp.id  = new_level_id;
            lp.pos.x = you.x_pos; 
            lp.pos.y = you.y_pos;

            old_level_info.update_stair(stair_x, stair_y, lp);

            // Then the new level's stair, assuming arbitrarily that going
            // upstairs will land you on the same downstairs you took to begin 
            // with (not necessarily true).
            lp.id = old_level_id;
            lp.pos.x = stair_x;
            lp.pos.y = stair_y;
            new_level_info.update_stair(you.x_pos, you.y_pos, lp, true);
        }
    }
    request_autopickup();
}                               // end down_stairs()

void trackers_init_new_level(bool transit)
{
    travel_init_new_level();
    if (transit)
        stash_init_new_level();
}

std::string level_description_string()
{
    if (you.level_type == LEVEL_PANDEMONIUM)
        return "- Pandemonium";

    if (you.level_type == LEVEL_ABYSS)
        return "- The Abyss";

    if (you.level_type == LEVEL_LABYRINTH)
        return "- a Labyrinth";

    if (you.level_type == LEVEL_PORTAL_VAULT)
    {
         if (you.level_type_name == "bazaar")
             return "- a Bazaar";
             
         return "- a Portal Chamber";
    }

    // level_type == LEVEL_DUNGEON
    char buf[200];
    const int youbranch = you.where_are_you;
    if ( branches[youbranch].depth == 1 )
        snprintf(buf, sizeof buf, "- %s", branches[youbranch].longname);
    else
    {
        const int curr_subdungeon_level = player_branch_depth();
        snprintf(buf, sizeof buf, "%d of %s", curr_subdungeon_level,
                 branches[youbranch].longname);
    }
    return buf;
}


void new_level(void)
{
    textcolor(LIGHTGREY);

    cgotoxy(7, 12, GOTO_STAT);

#if DEBUG_DIAGNOSTICS
    cprintf( "(%d) ", you.your_level + 1 );
#endif

    take_note(Note(NOTE_DUNGEON_LEVEL_CHANGE));
    cprintf("%s", level_description_string().c_str());

    clear_to_end_of_line();
#ifdef DGL_WHEREIS
    whereis_record();
#endif
}

static std::string weird_colour()
{
    int temp_rand;             // for probability determinations {dlb}
    std::string result;

    temp_rand = random2(25);
    result =
        (temp_rand ==  0) ? "red" :
        (temp_rand ==  1) ? "purple" :
        (temp_rand ==  2) ? "orange" :
        (temp_rand ==  3) ? "green" :
        (temp_rand ==  4) ? "magenta" :
        (temp_rand ==  5) ? "black" :
        (temp_rand ==  6) ? "blue" :
        (temp_rand ==  7) ? "grey" :
        (temp_rand ==  8) ? "umber" :
        (temp_rand ==  9) ? "charcoal" :
        (temp_rand == 10) ? "bronze" :
        (temp_rand == 11) ? "silver" :
        (temp_rand == 12) ? "gold" :
        (temp_rand == 13) ? "pink" :
        (temp_rand == 14) ? "yellow" :
        (temp_rand == 15) ? "white" :
        (temp_rand == 16) ? "brown" :
        (temp_rand == 17) ? "aubergine" :
        (temp_rand == 18) ? "ochre" :
        (temp_rand == 19) ? "leaf green" :
        (temp_rand == 20) ? "mauve" :
        (temp_rand == 21) ? "azure" :
        (temp_rand == 22) ? "lime green" :
        (temp_rand == 23) ? "scarlet" :
        (temp_rand == 24) ? "chartreuse"
                          : "colourless";

    return result;
}

std::string weird_writing()
{
    int temp_rand;             // for probability determinations {dlb}
    std::string result;

    temp_rand = random2(15);
    result =
        (temp_rand == 0) ? "writhing" :
        (temp_rand == 1) ? "bold" :
        (temp_rand == 2) ? "faint" :
        (temp_rand == 3) ? "spidery" :
        (temp_rand == 4) ? "blocky" :
        (temp_rand == 5) ? "angular" :
        (temp_rand == 6) ? "shimmering" :
        (temp_rand == 7) ? "glowing"
                         : "";

    if (!result.empty())
        result += ' ';

    result += weird_colour();

    result += ' ';

    temp_rand = random2(14);

    result +=
        (temp_rand == 0) ? "writing" :
        (temp_rand == 1) ? "scrawl" :
        (temp_rand == 2) ? "sigils" :
        (temp_rand == 3) ? "runes" :
        (temp_rand == 4) ? "hieroglyphics" :
        (temp_rand == 5) ? "scrawl" :
        (temp_rand == 6) ? "print-out" :
        (temp_rand == 7) ? "binary code" :
        (temp_rand == 8) ? "glyphs" :
        (temp_rand == 9) ? "symbols"
                         : "text";

    return result;
}

bool scramble(void)
{
    if (you.attribute[ATTR_TRANSFORMATION] == TRAN_STATUE)
        return (false);
    int max_carry = carrying_capacity();

    if ((max_carry / 2) + random2(max_carry / 2) <= you.burden)
        return false;
    else
        return true;
}                               // end scramble()

std::string weird_glow_colour()
{
    int temp_rand;             // for probability determinations {dlb}
    std::string result;

    // Must start with a consonant!
    temp_rand = random2(8);
    result =
        (temp_rand ==  0) ? "brilliant" :
        (temp_rand ==  1) ? "pale" :
        (temp_rand ==  2) ? "mottled" :
        (temp_rand ==  3) ? "shimmering" :
        (temp_rand ==  4) ? "bright" :
        (temp_rand ==  5) ? "dark" :
        (temp_rand ==  6) ? "shining"
                          : "faint";

    result += ' ';

    result += weird_colour();

    return result;
}

bool go_berserk(bool intentional)
{
    if (!you.can_go_berserk(intentional))
        return (false);

    if (Options.tutorial_left)
        Options.tut_berserk_counter++;
    
    mpr("A red film seems to cover your vision as you go berserk!");
    mpr("You feel yourself moving faster!");
    mpr("You feel mighty!");

    you.duration[DUR_BERSERKER] += 20 + random2avg(19, 2);

    calc_hp();
    you.hp *= 15;
    you.hp /= 10;

    deflate_hp(you.hp_max, false);

    if (!you.duration[DUR_MIGHT])
        modify_stat( STAT_STRENGTH, 5, true, "going berserk" );

    you.duration[DUR_MIGHT] += you.duration[DUR_BERSERKER];
    haste_player( you.duration[DUR_BERSERKER] );

    if (you.berserk_penalty != NO_BERSERK_PENALTY)
        you.berserk_penalty = 0;

    return true;
}                               // end go_berserk()

bool is_damaging_cloud(cloud_type type)
{
    switch (type)
    {
    // always harmful
    case CLOUD_FIRE:
    case CLOUD_COLD:
        return (true);
    // only harmful if the player doesn't have the necessary resistances
    // Takes into account what the player can *know* and what s/he can
    // also expect to be the case a few turns later (ignores spells).
    case CLOUD_STINK:
    case CLOUD_POISON:
        return (!player_res_poison(false, false));
    case CLOUD_STEAM:
        return (player_res_steam(false, false) <= 0);
    case CLOUD_MIASMA:
        return (player_prot_life(false, false) <= 2);
    // smoke, never harmful
    default:
        return (false);
    }
}

std::string cloud_name(cloud_type type)
{
    switch (type)
    {
    case CLOUD_FIRE:
        return "flame";
    case CLOUD_STINK:
        return "noxious fumes";
    case CLOUD_COLD:
        return "freezing vapour";
    case CLOUD_POISON:
        return "poison gases";
    case CLOUD_GREY_SMOKE:
        return "grey smoke";
    case CLOUD_BLUE_SMOKE:
        return "blue smoke";
    case CLOUD_PURP_SMOKE:
        return "purple smoke";
    case CLOUD_STEAM:
        return "steam";
    case CLOUD_MIASMA:
        return "foul pestilence";
    case CLOUD_BLACK_SMOKE:
        return "black smoke";
    case CLOUD_MIST:
        return "thin mist";
    default:
        return "buggy goodness";
    }
}

bool mons_is_safe(const struct monsters *mon, bool want_move)
{
    bool is_safe = mons_friendly(mon) ||
        mons_class_flag(mon->type, M_NO_EXP_GAIN);

#ifdef CLUA_BINDINGS
    bool moving = ((!you.delay_queue.empty() &&
                   is_run_delay(you.delay_queue.front().type) &&
                   you.delay_queue.front().type != DELAY_REST) ||
                   you.running < RMODE_NOT_RUNNING || want_move);

    int  dist   = grid_distance(you.x_pos, you.y_pos,
                                mon->x, mon->y);
    bool result = is_safe;

    if (clua.callfn("ch_mon_is_safe", "Mbbd>b",
                    mon, is_safe, moving, dist,
                    &result))
        is_safe = result;
#endif

    return is_safe;
}

bool i_feel_safe(bool announce, bool want_move)
{
    /* This is probably unnecessary, but I'm not sure that
       you're always at least 9 away from a wall */
    int ystart = you.y_pos - 9, xstart = you.x_pos - 9;
    int yend = you.y_pos + 9, xend = you.x_pos + 9;
    if ( xstart < 0 ) xstart = 0;
    if ( ystart < 0 ) ystart = 0;
    if ( xend >= GXM ) xend = GXM;
    if ( ystart >= GYM ) yend = GYM;

    if (in_bounds(you.x_pos, you.y_pos)
        && env.cgrid[you.x_pos][you.y_pos] != EMPTY_CLOUD)
    {
        const cloud_type type =
            env.cloud[ env.cgrid[you.x_pos][you.y_pos] ].type;
        if (is_damaging_cloud(type))
        {
            if (announce)
                mprf(MSGCH_WARN, "You're standing in a cloud of %s!",
                     cloud_name(type).c_str());
            return (false);
        }
    }
    
    // no monster will attack you inside a sanctuary,
    // so presence of monsters won't matter
    if (is_sanctuary(you.x_pos, you.y_pos))
        return (true);

    std::vector<const monsters *> mons;
    /* monster check */
    for ( int y = ystart; y < yend; ++y )
    {
        for ( int x = xstart; x < xend; ++x )
        {
            // if you can see a nonfriendly monster then you feel unsafe
            if ( see_grid(x,y) )
            {
                const unsigned short targ_monst = mgrd[x][y];
                if ( targ_monst != NON_MONSTER )
                {
                    const monsters *mon = &menv[targ_monst];
                    if ( player_monster_visible(mon) &&
                         !mons_is_submerged(mon) &&
                         !mons_is_mimic(mon->type) &&
                         !mons_is_safe(mon, want_move))
                    {
                        if (announce)
                            mons.push_back(mon);
                        else
                        {
                            tutorial_first_monster(*mon);
                            return false;
                        }
                    }
                }
            }
        }
    }

    if (announce)
    {
        // Announce the presence of monsters (Eidolos).
        if (mons.size() == 1)
        {
            const monsters &m = *mons[0];
            mprf(MSGCH_WARN, "Not with %s in view!",
                 m.name(DESC_NOCAP_A).c_str());
        }
        else if (mons.size() > 1)
        {
            mprf(MSGCH_WARN, "Not with these monsters around!");
        }
        return (mons.empty());
    }
    
    return true;
}

static const char *shop_types[] = {
    "weapon",
    "armour",
    "antique weapon",
    "antique armour",
    "antiques",
    "jewellery",
    "wand",
    "book",
    "food",
    "distillery",
    "scroll",
    "general"
};

int str_to_shoptype(const std::string &s)
{
    if (s == "random" || s == "any")
        return (SHOP_RANDOM);
    
    for (unsigned i = 0; i < sizeof(shop_types) / sizeof (*shop_types); ++i)
    {
        if (s == shop_types[i])
            return (i);
    }
    return (-1);
}

/* Decides whether autoprayer Right Now is a good idea. */
static bool should_autopray()
{
    if ( Options.autoprayer_on == false ||
         you.religion == GOD_NO_GOD ||
         you.religion == GOD_NEMELEX_XOBEH ||
         you.duration[DUR_PRAYER] ||
         grid_altar_god( grd[you.x_pos][you.y_pos] ) != GOD_NO_GOD ||
         !i_feel_safe() )
    {
        return false;
    }

    // We already know that we're not praying now. So if you
    // just autoprayed, there's a problem.
    if ( you.just_autoprayed )
    {
        mpr("Autoprayer failed, deactivating.", MSGCH_WARN);
        Options.autoprayer_on = false;
        return false;
    }

    return true;
}

/* Actually performs autoprayer. */
bool do_autopray()
{
    if ( you.turn_is_over )     // can happen with autopickup, I think
        return false;

    if ( should_autopray() )
    {
        pray();
        you.just_autoprayed = true;
        return true;
    }
    else
    {
        you.just_autoprayed = false;
        return false;
    }
}

// general threat = sum_of_logexpervalues_of_nearby_unfriendly_monsters
// highest threat = highest_logexpervalue_of_nearby_unfriendly_monsters
void monster_threat_values(double *general, double *highest)
{
    double sum = 0;
    int highest_xp = -1;

    monsters *monster = NULL;
    for (int it = 0; it < MAX_MONSTERS; it++)
    {
        monster = &menv[it];

        if (monster->alive() && mons_near(monster) && !mons_friendly(monster))
        {
            const int xp = exper_value(monster);
            const double log_xp = log((double)xp);
            sum += log_xp;
            if (xp > highest_xp)
            {
                highest_xp = xp;
                *highest   = log_xp;
            }
        }
    }

    *general = sum;
}

bool player_in_a_dangerous_place()
{
    const double logexp = log((double)you.experience);
    double gen_threat = 0.0, hi_threat = 0.0;
    monster_threat_values(&gen_threat, &hi_threat);

    return (gen_threat > logexp * 1.3 || hi_threat > logexp / 2);
}

////////////////////////////////////////////////////////////////////////////
// Living breathing dungeon stuff.
//

static std::vector<coord_def> sfx_seeds;

void setup_environment_effects()
{
    sfx_seeds.clear();

    for (int x = X_BOUND_1; x <= X_BOUND_2; ++x)
    {
        for (int y = Y_BOUND_1; y <= Y_BOUND_2; ++y)
        {
            if (!in_bounds(x, y))
                continue;

            const int grid = grd[x][y];
            if (grid == DNGN_LAVA 
                    || (grid == DNGN_SHALLOW_WATER
                        && you.where_are_you == BRANCH_SWAMP))
            {
                const coord_def c(x, y);
                sfx_seeds.push_back(c);
            }
        }
    }
#ifdef DEBUG_DIAGNOSTICS
    mprf(MSGCH_DIAGNOSTICS, "%u environment effect seeds", sfx_seeds.size());
#endif
}

static void apply_environment_effect(const coord_def &c)
{
    const int grid = grd[c.x][c.y];
    if (grid == DNGN_LAVA)
        check_place_cloud( CLOUD_BLACK_SMOKE, 
                           c.x, c.y, random_range( 4, 8 ), KC_OTHER );
    else if (grid == DNGN_SHALLOW_WATER)
        check_place_cloud( CLOUD_MIST, 
                           c.x, c.y, random_range( 2, 5 ), KC_OTHER );
}

static const int Base_Sfx_Chance = 5;
void run_environment_effects()
{
    if (!you.time_taken)
        return;

    dungeon_events.fire_event(DET_TURN_ELAPSED);

    // Each square in sfx_seeds has this chance of doing something special
    // per turn.
    const int sfx_chance = Base_Sfx_Chance * you.time_taken / 10;
    const int nseeds = sfx_seeds.size();

    // If there are a large number of seeds, speed things up by fudging the
    // numbers.
    if (nseeds > 50)
    {
        int nsels = div_rand_round( sfx_seeds.size() * sfx_chance, 100 );
        if (one_chance_in(5))
            nsels += random2(nsels * 3);

        for (int i = 0; i < nsels; ++i)
            apply_environment_effect( sfx_seeds[ random2(nseeds) ] );
    }
    else
    {
        for (int i = 0; i < nseeds; ++i)
        {
            if (random2(100) >= sfx_chance)
                continue;

            apply_environment_effect( sfx_seeds[i] );
        }
    }

    run_corruption_effects(you.time_taken);
}

coord_def pick_adjacent_free_square(int x, int y)
{
    int num_ok = 0;
    coord_def result(-1, -1);
    for ( int ux = x-1; ux <= x+1; ++ux )
    {
        for ( int uy = y-1; uy <= y+1; ++uy )
        {
            if ( ux == x && uy == y )
                continue;

            if ( ux >= 0 && ux < GXM && uy >= 0 && uy < GYM &&
                 grd[ux][uy] == DNGN_FLOOR && mgrd[ux][uy] == NON_MONSTER )
            {
                ++num_ok;
                if ( one_chance_in(num_ok) )
                    result.set(ux, uy);
            }
        }
    }
    return result;
}

// Converts a movement speed to a duration. i.e., answers the
// question: if the monster is so fast, how much time has it spent in
// its last movement?
// 
// If speed is 10 (normal),    one movement is a duration of 10.
// If speed is 1  (very slow), each movement is a duration of 100.
// If speed is 15 (50% faster than normal), each movement is a duration of
// 6.6667.
int speed_to_duration(int speed)
{
    if (speed < 1)
        speed = 10;
    else if (speed > 100)
        speed = 100;
    
    return div_rand_round(100, speed);
}

void reveal_secret_door(int x, int y)
{
    ASSERT(grd[x][y] == DNGN_SECRET_DOOR);

    dungeon_feature_type door = grid_secret_door_appearance(x, y);
    grd[x][y] = grid_is_opaque(door) ?
        DNGN_CLOSED_DOOR : DNGN_OPEN_DOOR;
}

// Is there a monster in a circle with the given radius, centered on the
// player's position?  If so, return the distance to it.  Otherwise,
// return -1.
int mons_inside_circle(int posx, int posy, int radius)
{
    if (!inside_level_bounds(posx, posy))
        return -1;

    int dist = distance(posx, posy, you.x_pos, you.y_pos);
    if (dist > radius*radius)
        return -1;

    if (mgrd[posx][posy] != NON_MONSTER)
        return dist;

    return -1;
}
