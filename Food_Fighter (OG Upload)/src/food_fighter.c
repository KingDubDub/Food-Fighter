/* Food Fighter - By King Dub Dub

For the TI-84 CE Plus graphing calculators.

My CC25 entry, Lord have mercy there's a lot of lore and I'm sure as hecc not spending time writing it here.
Play the game, read through the comments, maybe read through the massive amount of text dialogue if I get
around to adding it... Point is, I'm not making it easy for you. Unless you want to read/steal the code, in
which case I've been overcome with my Obsessive Commenting Disorder and it should be super easy to figure
out what's what. Have as much fun as won't kill you!

Oh, and welcome to nested-if hell. It's wonderfully intricate and painfully intense, but a few layers of
redundancy never hurt anyone (although I have removed a lot and changed the code structure to avoid it).

BASIC GOALS :
-Map sprites
-Character sprites
-Enemy sprites
-Tilemap engine and manuevering system
-Fighting stuff, getting ingredients, making food
-Dialogue and cutscene interrupts (may not be fully implemented but at least started)
-Funny ha-ha comments
-Learn from my mistakes with my other (ongoing) projects...
-Get at least 3rd place
-Play everyone else's CC25 and chillax for the first time in ~10 months

*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <keypadc.h>
#include <graphx.h>
#include <fileioc.h>
#include <compression.h>

//dialogue strings:
#include "strings.c"

//sprites:
#include "gfx/converts/gfx.h"
#include "sprites_optimized.h"

//tilemaps:
#include "gfx/hub_room/hr_tilemap.h"
#include "gfx/storeroom_arena/sr_tilemap.h"

//that looks like gibberish:
#define APPVAR_NAME "FDFGHT"
const uint8_t APPVAR_VERSION = 1;
uint8_t save_version;

#define TILE_WIDTH  32
#define TILE_HEIGHT 32

//max tiles of any tileset:
#define MAX_TILES 39

//defined hitbox values for Jerry becase I keep needing to change them:
#define J_HB_WIDTH  24
#define J_HB_HEIGHT 12

#define SLAM_HEIGHT 12
#define SLAM_WIDTH  28
#define SLAM_SPEED  2

//FPS to run the game at, don't change unless you want to redo LOTS of math:
#define FPS_LOCK 20

//defined as seconds and multiplied by FPS lock at compilation, 120 seconds is 2 minutes:
#define GAME_TIME 120

#define MAX_ENTITIES 8

//number of times Jerry flashes when he takes damage:
#define FLICKER_TIME 8

#define MAX_HEALTH 3

//Movin' at the speed of W A L K:
#define SPEED 4

typedef struct
{
    uint32_t score;
    uint32_t highscore;
    uint32_t kills;
    uint32_t games;
}
stat_t;

//for the score data and stuff, may add more later:
stat_t stats;

//the only thing that doesn't seem clear in context here is the sprite_ptr, it's for a neat little sprite-layering
//engine that checks the y's of all entities passed to it:
typedef struct
{
    int24_t x;
    int24_t y;

    int8_t health;
    uint8_t temperature;

    uint8_t direction;
    bool moving;

    int8_t animation;
    int8_t animationToggle;

    int8_t damageFlicker;

    bool canFire;   //stores interaction value for either firing or touching object tiles
    bool last_2nd;  //debouncing fun

    uint8_t inventory;

    gfx_sprite_t **sprite_ptr;
}
avatar_t;

//This is Jerry.
//Jerry has a flamethrower.
//Please treat Jerry with respect.
avatar_t Jerry;

//the fire only needs some stuff for hitbox checks, otherwise I have to recalculate lots of stuff:
typedef struct
{
    int24_t x;
    int24_t y;
    uint8_t width;
    uint8_t height;

    int8_t animation;
}
fire_t;

fire_t fire;

//struct for holding enemies and their data:
typedef struct
{
    int24_t x;
    int24_t y;
    uint8_t width;
    uint8_t height;
    int8_t health;

    int8_t animToggle;
    int8_t animation;

    uint8_t floor_offset;

    int8_t delay;

    gfx_sprite_t **sprite_ptr;
}
entity_t;

//4 enemies and 4 ingredients, although I suppose they can get imbalanced:
entity_t entities[8];

//number of entities in play, global because I might use it in multiple contexts:
uint8_t numEntities;

//keeps track of forced delay between spawning periods:
uint8_t spawnDelay;

//super-neat trick: put everything into a struct so you can put lots of data under single pointers, it also
//helps for clarifying the purpose of variables:
typedef struct
{
    //I've organized the tilesets so all the tiles Jerry can walk on come first, so the first tile is the start
    //of the ones he can walk on, and the number is defined by the initialization function/appvar:
    uint8_t walkables;

    //start of weird walkables, everything before it is a normal tile that I don't need to do real collision for:
    uint8_t normie_walkables;

    //keeps track of door-type tiles, since they can be at different positions in the tileset sometimes:
    uint8_t doors;

    //starting tile index of tiles that can be interacted with and the number of them:
    uint8_t interactable_tiles;

    //tilemap drawing offsets:
    int24_t camera_x;
    int24_t camera_y;

    //for the "camera" aka tilemap displacement for side-scrolling:
    int24_t camera_x_limit;
    int24_t camera_y_limit;
}
room_t;

room_t room;

//stores whether Jerry enters a door or not, not saved in appvar:
bool transport;

//the room Jerry is in and would go to if he transported:
uint8_t current_room;
uint8_t target_room;

//used for checking if Jerry is playing the game or menuing:
bool gaming;

//I was going to make a complex timer system, but then I realized that lag messes it up, so we count loops:
int24_t time;

//OH BOY HERE COMES THE CRAZY STUFF THAT I JUST LEARNED:
gfx_tilemap_t arena;

gfx_sprite_t *decompressed_tiles[MAX_TILES];

//a list of the offset hitboxes for different walkable tiles with funky collision, measured from tile edges
//and assumed to be square, they can also be negative for extra width/height for failsafes:
const int8_t tile_offset_x[]      = {32, -1, -1};
const int8_t tile_offset_y[]      = {32, 16, 16};
const int8_t tile_offset_width[]  = { 0, 34, 34};
const int8_t tile_offset_height[] = { 0, 16, 16};
//Do I need to keep these? Yes? No? Quite possibly?
//I need to put those in appvars at some point, but until then they are nasty nasty globals.

//appvar slot for save data:
ti_var_t appvar;

//should only be called twice:
void SaveGame(void)
{
    //create a new appvar, which erases the old one:
    appvar = ti_Open(APPVAR_NAME, "w");

    //appvar version, should help with debuggin' in future releases:
    ti_Write(&APPVAR_VERSION, sizeof(APPVAR_VERSION), 1, appvar);

    //all the beautiful custom datatypes that are incredibly dense, the names give a loose idea of what they hold:
    ti_Write(&Jerry, sizeof(Jerry), 1, appvar);
    ti_Write(&fire, sizeof(fire), 1, appvar);
    ti_Write(&entities, sizeof(entities), 1, appvar);
    ti_Write(&room, sizeof(room), 1, appvar);
    ti_Write(&stats, sizeof(stats), 1, appvar);

    //all the nasty single variables whose compilation I couldn't justify:
    ti_Write(&numEntities, sizeof(numEntities), 1, appvar);
    ti_Write(&spawnDelay, sizeof(spawnDelay), 1, appvar);
    ti_Write(&current_room, sizeof(current_room), 1, appvar);
    ti_Write(&target_room, sizeof(target_room), 1, appvar);
    ti_Write(&gaming, sizeof(gaming), 1, appvar);
    ti_Write(&time, sizeof(time), 1, appvar);

    ti_SetArchiveStatus(true, appvar);
}

void SetupMenu(void)
{
    //decompress the tileset:
    for(uint8_t i = 0; i < hr_tileset_num_tiles; ++i)
    {
        zx7_Decompress(decompressed_tiles[i], hr_tileset_tiles_compressed[i]);
    }

    //set a bunch of stuff the main menu needs to be true:
    arena.map    = hub_room;
    arena.width  = 10;
    arena.height = 8;

    //index values of various tiles with different collision systems:
    room.normie_walkables = 1;
    room.doors = 3;
    room.walkables = 3;
    
    //no interactables:
    room.interactable_tiles = 255;

    //number of pixels the camera can't go past:
    room.camera_x_limit = 0;
    room.camera_y_limit = 0;

    /* Just marking the desired coords and camera position for starting in the hub room:
    room.camera_x = 0;
    room.camera_y = 0;

    Jerry.x = 228;
    Jerry.y = 74;*/

    //Jerry is just a regular Jerry now:
    Jerry.sprite_ptr = Jerry_sprite;

    target_room = 1;
    current_room = 0;

    //Jerry can walk away this time...
    gaming = true;
};

void SetupStoreroom(void)
{
    for(uint8_t i = 0; i < sr_tileset_num_tiles; ++i)
    {
        zx7_Decompress(decompressed_tiles[i], sr_tileset_tiles_compressed[i]);
    }

    arena.map    = sr_arena;
    arena.width  = 14;
    arena.height = 12;

    room.normie_walkables = 7;
    room.doors = 9;
    room.walkables = 9;

    //for ingredient slot:
    room.interactable_tiles = 9;

    //now it can only go four tiles out, plus half a tile because of the LCD height not being a multiple of 32:
    room.camera_x_limit = TILE_WIDTH * 4;
    room.camera_y_limit = (TILE_HEIGHT * 4) + 16;

    /* making sure these are marked here too:
    room.camera_x = (TILE_WIDTH * 2);
    room.camera_y = (TILE_HEIGHT * 2) + 26;

    Jerry.x = (TILE_WIDTH * 6) + 20;
    Jerry.y = (TILE_HEIGHT * 6) + 26;*/

    //Jerry gets his cool backpack:
    Jerry.sprite_ptr = Jerry_weaponized_sprite;

    target_room = 0;
    current_room = 1;

    //Jerry can't move when the battle starts:
    gaming = false;
}

//pop up screen that asks if you want to go to the next room or not:
void ConfirmTransport(void)
{
    //move Jerry away from the door he enters if he tries to quit:
    int8_t scooch;
    int8_t update_direction;

    //copy the screen to the buffer without disrupting the screen:
    gfx_SwapDraw();
    gfx_BlitBuffer();

    //set text color to white and scale:
    gfx_SetTextFGColor(2);
    gfx_SetTextScale(3, 3);

    //draw the base menu:
    gfx_Sprite_NoClip(confirm_tile_0, 20, 60);
    gfx_Sprite_NoClip(confirm_tile_1, 160, 60);


    if(arena.map == hub_room)
    {
        gfx_PrintStringXY("Go to work?", 48, 90);

        scooch = -4;
        update_direction = 2;
    }
    else
    {
        gfx_PrintStringXY("Quit round?", 45, 90);

        scooch = 4;
        update_direction = 0;
    }

    gfx_SetTextScale(1, 1);

    //put menu on screen and put old screen back to the buffer:
    gfx_SwapDraw();

    Jerry.y += scooch;
    Jerry.direction = update_direction;

    //wait until all keys are released:
    do {kb_Scan();} while(kb_AnyKey() && !(kb_Data[6] & kb_Clear));

    //wait for a left or right press:
    do
    {
        kb_Scan();

        if((kb_Data[7] & kb_Left) && !(kb_Data[7] & kb_Right))
        {
            transport = true;
        }
        else if(!(kb_Data[7] & kb_Left) && (kb_Data[7] & kb_Right))
        {
            transport = false;
        }
    }
    while(!(kb_Data[7] & kb_Left) && !(kb_Data[7] & kb_Right) && !(kb_Data[6] & kb_Clear));

    do {kb_Scan();} while(((kb_Data[7] & kb_Left) || (kb_Data[7] & kb_Right)) && !(kb_Data[6] & kb_Clear));
}

//for spawning enemies, uses sprite pointers as identifiers for enemy types with coords as well, list needs
//to be defragged for speed and ease of use, spawning shifts all entries up when neccesary and fills in "blank"
//spots that are out of play (AKA greater than or equal to Y-10000):
void SpawnEnemies(void)
{
    //The coords enemy are randomly set by starting from the corner of each spawn zone, adding a value that may be
    //zero or a big incrementer to shift it between upper/lower and left/right zones, and then a random offset.
    //Hard to explain, but trust me it gets great results.

    if(!spawnDelay)
    {
        //if a random chance is zero, the entity limit hasn't been hit yet, and the time isn't zero, make some enemies:
        if(!randInt(0, 50) && (numEntities != MAX_ENTITIES) && time)
        {
            //the y to set the new enemy to:
            int24_t targetY = 120 + (randInt(0, 1) * 128) + randInt(0, 16);
            uint8_t targetOffset = 40;

            for(uint8_t i = 0; (i < MAX_ENTITIES); ++i)
            {
                //if the entity is supposed to go ahead of the new one, shift the list:
                if((entities[i].y + entities[i].floor_offset) >= (targetY + targetOffset))
                {
                    //j needs to start at the 2nd-to-last entry and move it up, and then continue until it moves the
                    //entry we want to edit:
                    for(int8_t j = (MAX_ENTITIES - 2); (j >= i); --j)
                    {
                        entities[j + 1] = entities[j];
                    }

                    //set x, y, health, and a positive animation incrementor:
                    entities[i].x = 88 + (randInt(0, 1) * 224) + randInt(0, 16);
                    entities[i].y = targetY;
                    entities[i].health = 5;

                    entities[i].animToggle = -1;
                    entities[i].animation  = 0;

                    //distance from entity to the floor, 40 pixels for the slamwhich:
                    entities[i].floor_offset = 39;

                    //enemies have to wait a second after spawning to be able to move and hit Jerry:
                    entities[i].delay = (1 * FPS_LOCK);

                    entities[i].sprite_ptr = slamwhich_tiles;

                    entities[i].width  = SLAM_WIDTH;
                    entities[i].height = SLAM_HEIGHT;

                    ++numEntities;

                    //exit out of the spawning loop:
                    i = MAX_ENTITIES;
                }
            }
            //wait 20 frames before attempting spawns again:
            spawnDelay = 19;
        }
    }
    else
    {
        --spawnDelay;
    }
}

//take the entities list and sort it in ascending order:
void DefragEntities(void)
{
    //temporary entity slot for the var we want to shift:
    entity_t temp;

    //run through every entry in the list and check for discrepencies, but not the first one since we can't check it:
    for(int8_t i = (numEntities - 1); i > 0; --i)
    {
        for(int8_t j = i; j < (MAX_ENTITIES - 1); ++j)
        {
            if((entities[j].y + entities[j].floor_offset) > (entities[j+1].y + entities[j+1].floor_offset))
            {
                temp = entities[j];

                //copy the lower entity back:
                entities[j] = entities[j+1];

                //put the current entity higher up:
                entities[j+1] = temp;
            }
            else
            {
                //quit early if we can, otherwise we shift the current entity to the top of the list:
                break;
            }
        }       //for some reason, I wrote this whole thing in one go and it sorted in descending order the first try.
    }           //Not what I wanted, but still pretty neat.
}

//set the Y-coords to "something huge"
void ClearEntities(void)
{
    for(uint8_t i = 0; i < MAX_ENTITIES; ++i)
    {
        entities[i].y = 10000;
    }

    numEntities = 0;
}

//@EverydayCode here's my hitbox checking for tilemaps; it's optimized to use my globals, but ctrl-h should fix
//all those naming issues for your 'Mungus clone.

//OOH, FAHN-CY HITBOX CHECKS! This uses some globals of course but it's definitely smaller than 4 of itself in
//the main loop, returns false if Jerry isn't touching anything, true if he does, and messes with some globals
//for menuing triggers that have to run after the tilemap code. Gotta see about fixing them if possible:
uint8_t CornerCollision(uint8_t corner_tile_1, uint8_t corner_tile_2, uint8_t orientation)
{
    //some quick 'n dirty locals for hitbox checks:
    uint8_t x;
    uint8_t y;

    if((corner_tile_1 < room.normie_walkables) && (corner_tile_2 < room.normie_walkables))
    {
        //if this is gonna be easy for once, return true, no tile collisions:
        return false;
    }
    else if((corner_tile_1 < room.doors) && (corner_tile_2 < room.doors))
    {
        //OOH A MAGICAL EXCEPTION SINCE I'M TOO LAZY FOR ANYTHING ELSE RIGHT NOW!

        transport = true;

        return true;
    }
    else if((corner_tile_1 < room.walkables) && (corner_tile_2 < room.walkables))
    {
        //if both corners are walkable but not easy, then here comes some weird stuff:

        if(kb_Data[7] & kb_Up) //north
        {
            y = trunc((Jerry.y - SPEED) / 32) * 32;

            //rounds Jerry's y so he touches the tile's bottom edge, note that his head sticks over it though:
            if((tile_offset_y[corner_tile_1]) < (tile_offset_y[corner_tile_2]))
            {
                x = round(Jerry.x / 32) * 32;

                if(gfx_CheckRectangleHotspot(Jerry.x, Jerry.y - SPEED, J_HB_WIDTH - 1, J_HB_HEIGHT - 1, x + tile_offset_x[corner_tile_1], y + tile_offset_y[corner_tile_1], tile_offset_width[corner_tile_1], tile_offset_height[corner_tile_1]))
                {
                    Jerry.y = y + tile_offset_y[corner_tile_1] + tile_offset_height[corner_tile_1] + 1;

                    return true;
                }
            }
            else
            {
                x = round((Jerry.x + J_HB_WIDTH - 1) / 32) * 32;

                if(gfx_CheckRectangleHotspot(Jerry.x, Jerry.y - SPEED, J_HB_WIDTH - 1, J_HB_HEIGHT - 1, x + tile_offset_x[corner_tile_2], y + tile_offset_y[corner_tile_2], tile_offset_width[corner_tile_2], tile_offset_height[corner_tile_2]))
                {
                    Jerry.y = y + tile_offset_y[corner_tile_2] + tile_offset_height[corner_tile_2] + 1;

                    return true;
                }
            }
        }
        else if(kb_Data[7] & kb_Down) //south
        {
                y = round((Jerry.y + J_HB_HEIGHT - 1 + SPEED) / 32) * 32;

                //rounds Jerry's y so he touches the funky tile's top edge if he can't walk far enough:
                if((tile_offset_y[corner_tile_1]) < (tile_offset_y[corner_tile_2]))
                {
                    x = round(Jerry.x / 32) * 32;

                    if(gfx_CheckRectangleHotspot(Jerry.x, Jerry.y + SPEED, J_HB_WIDTH - 1, J_HB_HEIGHT - 1, x + tile_offset_x[corner_tile_1], y + tile_offset_y[corner_tile_1], tile_offset_width[corner_tile_1], tile_offset_height[corner_tile_1]))
                    {
                        Jerry.y = y + tile_offset_y[corner_tile_1] - J_HB_HEIGHT;

                        return true;
                    }
                }
                else
                {
                    x = round((Jerry.x + J_HB_WIDTH - 1) / 32) * 32;

                    if(gfx_CheckRectangleHotspot(Jerry.x, Jerry.y + SPEED, J_HB_WIDTH - 1, J_HB_HEIGHT - 1, x + tile_offset_x[corner_tile_2], y + tile_offset_y[corner_tile_2], tile_offset_width[corner_tile_2], tile_offset_height[corner_tile_2]))
                    {
                        Jerry.y = y + tile_offset_y[corner_tile_2] - J_HB_HEIGHT;

                        return true;
                    }
                }
        }

        if(kb_Data[7] & kb_Left) //west
        {
                x = round((Jerry.x - SPEED) / 32) * 32;

                //rounds Jerry's so he touches the funky tile's left edge:
                if((tile_offset_x[corner_tile_1]) < (tile_offset_x[corner_tile_2]))
                {
                    y = round(Jerry.y / 32) * 32;

                    if(gfx_CheckRectangleHotspot(Jerry.x - SPEED, Jerry.y, J_HB_WIDTH, J_HB_HEIGHT - 1, x + tile_offset_x[corner_tile_1], y + tile_offset_y[corner_tile_1], tile_offset_width[corner_tile_1], tile_offset_height[corner_tile_1]))
                    {
                        Jerry.x = x + tile_offset_x[corner_tile_1] + tile_offset_width[corner_tile_1];

                        return true;
                    }
                }
                else
                {
                    y = round((Jerry.y + J_HB_HEIGHT - 1) / 32) * 32;

                    if(gfx_CheckRectangleHotspot(Jerry.x - SPEED, Jerry.y, J_HB_WIDTH, J_HB_HEIGHT - 1, x + tile_offset_x[corner_tile_2], y + tile_offset_y[corner_tile_2], tile_offset_width[corner_tile_2], tile_offset_height[corner_tile_2]))
                    {
                        Jerry.x = x + tile_offset_x[corner_tile_2] + tile_offset_width[corner_tile_2];

                        return true;
                    }
                }
        }
        else if(kb_Data[7] & kb_Right) //east
        {
            x = trunc((Jerry.x + J_HB_WIDTH - 1 + SPEED) / 32) * 32;

            if((tile_offset_x[corner_tile_1]) < (tile_offset_x[corner_tile_2]))
            {
                y = round(Jerry.y / 32) * 32;

                if(gfx_CheckRectangleHotspot(Jerry.x + SPEED, Jerry.y, J_HB_WIDTH - 1, J_HB_HEIGHT - 1, x + tile_offset_x[corner_tile_1], y + tile_offset_y[corner_tile_1], tile_offset_width[corner_tile_1], tile_offset_height[corner_tile_1]))
                {
                    Jerry.x = x + tile_offset_x[corner_tile_1] - J_HB_WIDTH;

                    return true;
                }
            }
            else
            {
                y = round((Jerry.y + J_HB_HEIGHT - 1) / 32) * 32;

                if(gfx_CheckRectangleHotspot(Jerry.x + SPEED, Jerry.y, J_HB_WIDTH - 1, J_HB_HEIGHT - 1, x + tile_offset_x[corner_tile_2], y + tile_offset_y[corner_tile_2], tile_offset_width[corner_tile_2], 32 - tile_offset_y[corner_tile_2]))
                {
                    Jerry.x = x + tile_offset_x[corner_tile_2] - J_HB_WIDTH;

                    return true;
                }
            }
        }

        //one small step for Jerry, a big stupid logistical nightmare for me:
        return false;
    }
    else //they aren't walkable tiles, they aren't doors, and they aren't weird tiles, they must be a regular wall:
    {
        //if either corner hits an easy tile with a normal hibox, I can breathe easy:
        switch(orientation)
        {
            case 0: //south
                    //rounds Jerry's y so he touches the tile's bottom edge:
                    y = round(Jerry.y / 32) + 1;
                    Jerry.y = (y * 32) - J_HB_HEIGHT;
            break;

            case 1: //west
                    //rounds Jerry's x to the tile's left edge:
                    x = trunc(Jerry.x / 32);
                    Jerry.x = x * 32;
            break;

            case 2: //north
                    //rounds Jerry's y so he touches the tile's top edge, note that his head sticks over it though:
                    y = round(Jerry.y / 32);
                    Jerry.y = (y * 32);
            break;

            case 3: //east
                    //rounds Jerry's x to the tile's right edge:
                    x = trunc(Jerry.x / 32) + 1;
                    Jerry.x = (x * 32) - J_HB_WIDTH;
            break;
        }
    }
    //Jerry shall not walk this day.
    return true;
}

//my function for checking if Jerry can to interact with a tile or not, uses globals because of size cut:
uint8_t WannaTouchIt(void)
{
    //the tile that's 16 pixels away from the center of Jerry's hitbox:
    uint8_t target_tile;
    //of course, his hitbox is comprised of even numbers so it has some bugs, but not noticable ones in my case.

    //figure out which tile to check, very similar to all the other functions:
    switch(Jerry.direction)
    {
        case 0:
                target_tile = gfx_GetTile(&arena, Jerry.x + (J_HB_WIDTH/2), Jerry.y + J_HB_HEIGHT - 1 + 8);
        break;

        case 1:
                target_tile = gfx_GetTile(&arena, Jerry.x - 8, Jerry.y + (J_HB_HEIGHT/2));
        break;

        case 2:
                target_tile = gfx_GetTile(&arena, Jerry.x + (J_HB_WIDTH/2), Jerry.y - 8);
        break;

        case 3:
                target_tile = gfx_GetTile(&arena, Jerry.x + J_HB_WIDTH - 1 + 8, Jerry.y + (J_HB_HEIGHT/2));
        break;
    }

    //if the tile is within range of the interactable tile types:
    if(room.interactable_tiles <= target_tile)
    {
        //we got a weird tile to do things with:
        return target_tile;
    }
    else
    {
        //nothing odd here, the same output as reading tile zero:
        return 0;
    }
}

//I do it in two different cases, therefore it gets a function and some fun local variables:
void DrawJerry()
{
    //So I have this crazy idea, take a variable that toggles Jerry's drawing based on it's positivity
    //that also increments, and after it's positive and incremented a certain amount of times it shouldn't
    //toggle until reset. This sounds really clean and I can't wait for it to fall apart:
    if(Jerry.damageFlicker > -1)
    {
        if(Jerry.damageFlicker < (FLICKER_TIME + 1))
        {
            ++Jerry.damageFlicker;
        }

        //Jerry's coords are relative to the map, so here's some precalculated vars:
        uint24_t x_pos = (Jerry.x - room.camera_x);
        uint8_t y_pos  = (Jerry.y - (46 - J_HB_HEIGHT) - room.camera_y);

        //if Jerry's firing, do some funky stuff:
        if((kb_Data[1] & kb_2nd) && (arena.map != hub_room) && gaming && Jerry.canFire)
        {        
            //I get so excited when I use a switch case, it's just such a rare occurrence:
            switch(Jerry.direction)
            {
                case 0:
                    //and this is Jerry in any other position; I don't like how I set this up:
                    gfx_TransparentSprite_NoClip(Jerry.sprite_ptr[(Jerry.animation / 3) + (Jerry.direction * 3)], x_pos, y_pos);

                    //calculate all the stuff for hitbox checks, all width and heights are one pixel less than the sprite ones:
                    fire.x = Jerry.x;
                    fire.y = Jerry.y - 4;
                    fire.width  = 23;
                    fire.height = 29;

                    //drawing the fire, layered over Jerry of course:
                    gfx_TransparentSprite(fire_sprite[fire.animation / 2], fire.x - room.camera_x, fire.y - room.camera_y);
                break;

                case 1:
                    //the sprite for left-facing firing Jerry are 4 pixels wider than normal and thus drawn 4 pixels to the left:
                    gfx_TransparentSprite_NoClip(Jerry.sprite_ptr[(Jerry.animation / 3) + 3], x_pos - 4, y_pos);

                    fire.x = Jerry.x - 36;
                    fire.y = Jerry.y - 30;
                    fire.width  = 31;
                    fire.height = 29;

                    gfx_TransparentSprite(fire_sprite[(fire.animation / 2) + 3], fire.x - room.camera_x, fire.y - room.camera_y);
                break;

                case 2:
                    fire.x = Jerry.x;
                    fire.y = Jerry.y - 40;
                    fire.width  = 23;
                    fire.height = 29;

                    gfx_TransparentSprite(fire_sprite[(fire.animation / 2) + 6], fire.x - room.camera_x, fire.y - room.camera_y);

                    //this time Jerry goes over the fire since he's shooting "above" him from our perspective.
                    gfx_TransparentSprite_NoClip(Jerry.sprite_ptr[(Jerry.animation / 3) + (Jerry.direction * 3)], x_pos, y_pos);
                break;

                case 3:
                    gfx_TransparentSprite_NoClip(Jerry.sprite_ptr[(Jerry.animation / 3) + (Jerry.direction * 3)], x_pos, y_pos);

                    fire.x = Jerry.x + 28;
                    fire.y = Jerry.y - 30;
                    fire.width  = 31;
                    fire.height = 29;

                    gfx_TransparentSprite(fire_sprite[(fire.animation / 2) + 9], fire.x - room.camera_x, fire.y - room.camera_y);
                break;
            }
        }
        else //if it's a normal sprite and I don't need to worry:
        {
            //I'm drawing Jerry with a working hitbox of 24x12, so I draw his sprite 21 pixels above the actual y:
            gfx_TransparentSprite_NoClip(Jerry.sprite_ptr[(Jerry.animation / 3) + (Jerry.direction * 3)], x_pos, y_pos);
        }
    }

    if(Jerry.damageFlicker < (FLICKER_TIME + 1))
    {
        Jerry.damageFlicker *= -1;
    }
}

//death screen, more like Death Note heh heh (I highly reccomend watching it even though I don't like anime):
void DeathScreen(void)
{
    //opening text and a counter for the number of loop times:
    uint8_t text_address = randInt(0, 3) * 2;
    uint8_t press_count = 0;

    Jerry.health = 3;

    gfx_SetTextFGColor(1);
    gfx_SetTextScale(2, 2);

    //wait a second so the idiocy of your death can sink in, you were either killed by a sandwhich or by blowing yourself up:
    delay(100);

    //a simple loop just because:
    do
    {
        //this is purely so I can use SwapDraw
        gfx_FillScreen(2);

        //Spoopy Jerry will visit you at 2 AM if you do not brush your teeth.
        gfx_TransparentSprite_NoClip(Jerry_ded_sprite[Jerry.direction], 160 - (J_HB_WIDTH / 2), 97);

        //print some centered text:
        gfx_PrintStringXY(ded_speech[text_address], 160 - (gfx_GetStringWidth(ded_speech[text_address]) / 2), 40);
        gfx_PrintStringXY(ded_speech[text_address + 1], 160 - (gfx_GetStringWidth(ded_speech[text_address + 1]) / 2), 60);

        //actually, I changed my mind, this is less likely to leave artifacts:
        gfx_BlitBuffer();

        text_address = randInt(4, 7) * 2;

        //cheap debouncing trick that I love to use:
        do {kb_Scan();} while((kb_Data[1] & kb_2nd) && !(kb_Data[6] & kb_Clear));
        do {kb_Scan();} while(!(kb_Data[1] & kb_2nd) && !(kb_Data[6] & kb_Clear));

        ++press_count;
    }
    while(!(kb_Data[6] & kb_Clear) && (press_count != 2));

    //show's over, go home folks:
    transport = true;
}

//this is a painful loss of bytes, but it's for proper failchecks with shells, so I have to use it:
int main(void)
{
    //used to tell if someone has ever played the game before or not:
    bool noob;

    //stores a sucsessful game save and is used to load or ignore savedata values:
    bool app_success = false;

    //prepare the tilesets:
    for(uint8_t i = 0; i < MAX_TILES; ++i)
    {
        decompressed_tiles[i] = gfx_MallocSprite(TILE_WIDTH, TILE_HEIGHT);
    }

    //make sure there's no funny business:
    ti_CloseAll();

    //if we don't have a savedata appvar, make one:
    if(!(appvar = ti_Open(APPVAR_NAME, "r")))
    {
        SaveGame();

        //HA, UR NOOB!!!
        noob = true;
    }
    else //read what's in the detected appvar to the game vars:
    {
        ti_Read(&save_version, sizeof(save_version), 1, appvar);

        if(save_version == APPVAR_VERSION)
        {
            //and now we read the variables into the volatile program ones:
            ti_Read(&Jerry, sizeof(Jerry), 1, appvar);
            ti_Read(&fire, sizeof(fire), 1, appvar);
            ti_Read(&entities, sizeof(entities), 1, appvar);
            ti_Read(&room, sizeof(room), 1, appvar);
            ti_Read(&stats, sizeof(stats), 1, appvar);

            //all the nasty single variables whose compilation I couldn't justify:
            ti_Read(&numEntities, sizeof(numEntities), 1, appvar);
            ti_Read(&spawnDelay, sizeof(spawnDelay), 1, appvar);
            ti_Read(&current_room, sizeof(current_room), 1, appvar);
            ti_Read(&target_room, sizeof(target_room), 1, appvar);
            ti_Read(&gaming, sizeof(gaming), 1, appvar);
            ti_Read(&time, sizeof(time), 1, appvar);

            app_success = true;
        }
        else
        {
            SaveGame();
        }
    }

    //the RNG is absolute cheese without this:
    srand(rtc_Time());

    //handy little FPS var for locking timings and such:
    uint8_t FPS;

    //these are always true for all the room tilemaps, so they're only set once:
    arena.type_width  = gfx_tile_32_pixel;
    arena.type_height = gfx_tile_32_pixel;
    arena.tile_width  = 32;
    arena.tile_height = 32;

    //these need to be one higher than normal because of how the tilemap function draws:
    arena.draw_width  = 11;
    arena.draw_height = 9;

    arena.x_loc       = 0;
    arena.y_loc       = 0;

    //initialize the graphx libraries and VRAM:
    gfx_Begin();
    gfx_SetDrawBuffer();

    gfx_SetPalette(main_palette, sizeof(main_palette), 0);
    //gfx_SetTransparentColor(1);

    //gfx_SetTextConfig(gfx_text_clip);

    //fast scan mode:
    kb_SetMode(MODE_3_CONTINUOUS);

    //set up timer one for FPS monitoring:
    timer_Control = TIMER1_ENABLE | TIMER1_32K | TIMER1_UP;

    //come here when I need to partially reset after death or something:
    GAMESTART:

    //reload that tileset pointer:
    arena.tiles  = decompressed_tiles;

    //if we didn't load an initial appvar save or the game's restarting, set the defaults:
    if(!app_success)
    {
        //stuff to reset if we don't have a save to load them from:
        Jerry.health = MAX_HEALTH;
        Jerry.temperature = 0;
        Jerry.canFire = true;
        Jerry.inventory = 0;

        fire.animation = 0;

        //reset all entity values to something stupidly large so I know to ignore them:
        ClearEntities();
    }
    else
    {
        //don't load the data if we return back to GAMESTART:
        app_success = false;
    }

    //if we were in the middle of leaving last save, reset the coordinates for the room type:
    if(current_room == target_room)
    {
        Jerry.health = MAX_HEALTH;
        Jerry.direction = 0;
        Jerry.animation = 3;
        Jerry.animationToggle = 1;
        Jerry.damageFlicker = FLICKER_TIME + 1;

        //2 minutes 'n 3 seconds to kick some booty:
        time = (GAME_TIME + 4) * FPS_LOCK;

        //choose room starting coordinates:
        switch(current_room)
        {
            case 0:
                room.camera_x = 0;
                room.camera_y = 0;

                Jerry.x = 228;
                Jerry.y = 74;
            break;

            case 1:
                room.camera_x = (TILE_WIDTH * 2);
                room.camera_y = (TILE_HEIGHT * 2) + 26;

                Jerry.x = (TILE_WIDTH * 6) + 20;
                Jerry.y = (TILE_HEIGHT * 6) + 26;
            break;
        }
    }

    //choose room:
    switch(current_room)
    {
        case 0:
            SetupMenu();

            //bugs exist, so let's make it look like they don't:
            ClearEntities();
            Jerry.inventory = 0;
        break;

        case 1:
            SetupStoreroom();
        break;
    }

    //extra redundancy for odd things with loading saved entity lists:
    DefragEntities();

    //extra extra redundancy to fight data corruption and a bug that should be fixed:
    for(uint8_t i = 0; i < MAX_ENTITIES; ++i)
    {
        //if they're a slamwhich, set them to the default animation:
        if(entities[i].sprite_ptr == slamwhich_tiles)
        {
            entities[i].animToggle = -1;
            entities[i].animation = 0;
        }
        else if(entities[i].sprite_ptr == ingredients_tiles)
        {
            //ingredients just get their lock enforced:
            entities[i].animToggle = 0;
        }
        else
        {
            //and if it's something weird, turn it into cheese:
            entities[i].sprite_ptr = ingredients_tiles;
            entities[i].animation = 6;
            entities[i].animToggle = 0;

            entities[i].x += 10;
            entities[i].y += 28;

            entities[i].health = 127;

            entities[i].floor_offset = 11;

            //My new go-to solution, just turn the problem into cheese!
        }
    }

    //set initial text color and size:
    gfx_SetTextFGColor(2);
    gfx_SetTextScale(1, 1);

    do{
        //we obviously need to check for keypresses, I don't think anyone's mastered calc-telekinesis-control yet:
        kb_Scan();

        //if Jerry's allowed to move, then run the controls, enemy spawning, and etc.:
        if(gaming)
        {
            //vertical control:
            if((kb_Data[7] & kb_Up) && !(kb_Data[7] & kb_Down))
            {
                //if Jerry isn't firing or can't fire, change which way he's looking:
                if(!(kb_Data[1] & kb_2nd) || !Jerry.canFire) Jerry.direction = 2;

                //check if Jerry's top corners will hit any tiles:
                if(!CornerCollision(gfx_GetTile(&arena, Jerry.x, Jerry.y - SPEED), gfx_GetTile(&arena, Jerry.x + J_HB_WIDTH - 1, Jerry.y - SPEED), 2))
                {
                    Jerry.y -= SPEED;
                    Jerry.moving = true;

                    //move the camera upwards to follow Jerry as long as it's inside the tilemap:
                    if((Jerry.y - SPEED + (J_HB_HEIGHT/2) - room.camera_y) < (LCD_HEIGHT/2))
                    {
                        if((room.camera_y - SPEED) > 0)
                        {
                            room.camera_y -= SPEED;
                        } else {
                            room.camera_y = 0;
                        }
                    }
                }
            }
            else if((kb_Data[7] & kb_Down) && !(kb_Data[7] & kb_Up) && gaming)
            {
                if(!(kb_Data[1] & kb_2nd) || !Jerry.canFire) Jerry.direction = 0;

                //check if Jerry's bottom corners are about to hit any tiles
                if(!CornerCollision(gfx_GetTile(&arena, Jerry.x, Jerry.y + J_HB_HEIGHT + SPEED - 1), gfx_GetTile(&arena, Jerry.x + (J_HB_WIDTH - 1), Jerry.y + J_HB_HEIGHT + SPEED - 1), 0))
                {
                    Jerry.y += SPEED;
                    Jerry.moving = true;

                    //move the camera downwards if it doesn't go past the edge of the tilemap:
                    if((Jerry.y + SPEED + (J_HB_HEIGHT/2) - room.camera_y) > 120)
                    {
                        if((room.camera_y + SPEED) < room.camera_y_limit)
                        {
                            room.camera_y += SPEED;
                        } else {
                            room.camera_y = room.camera_y_limit;
                        }
                    }
                }
            }

            //horizontal control:
            if((kb_Data[7] & kb_Left) && !(kb_Data[7] & kb_Right) && gaming)
            {
                if(!(kb_Data[1] & kb_2nd) || !Jerry.canFire) Jerry.direction = 1;

                //check if the 2 left corners of Jerry's hitbox will hit a tile:
                if(!CornerCollision(gfx_GetTile(&arena, Jerry.x -SPEED, Jerry.y), gfx_GetTile(&arena, Jerry.x -SPEED, Jerry.y + (J_HB_HEIGHT - 1)), 1))
                {
                    Jerry.x -= SPEED;
                    Jerry.moving = true;

                    //move camera to the left, to the left, to the left, now shuffle:
                    if((Jerry.x - room.camera_x - SPEED + J_HB_WIDTH/2) < 160)
                    {
                        if((room.camera_x - SPEED) > 0)
                        {
                            room.camera_x -= SPEED;
                        } else {
                            room.camera_x = 0;
                        }
                    }
                }
            }
            else if((kb_Data[7] & kb_Right) && !(kb_Data[7] & kb_Left) && gaming)
            {
                if(!(kb_Data[1] & kb_2nd) || !Jerry.canFire) Jerry.direction = 3;

                if(!CornerCollision(gfx_GetTile(&arena, Jerry.x + (J_HB_WIDTH + 2), Jerry.y), gfx_GetTile(&arena, Jerry.x + (J_HB_WIDTH + 2), Jerry.y + (J_HB_HEIGHT - 1)), 3))
                {
                    Jerry.x += SPEED;
                    Jerry.moving = true;

                    //move the camera to the right, to the right, to the right, now shuffle:
                    if((Jerry.x - room.camera_x + SPEED + J_HB_WIDTH/2) > 160)
                    {
                        if((room.camera_x + SPEED) < room.camera_x_limit)
                        {
                            room.camera_x += SPEED;
                        } else {
                            room.camera_x = room.camera_x_limit;
                        }
                    }
                }
            }

            //I used to take a dance class once at a week at an ettiquite school, hated every minute of it.
            //Well, except for when we learned to eat soup fancily, we used skittles instead of real soup.

            //if the current keypress is down and the last one was up:
            if((kb_Data[1] & kb_2nd))
            {
                //tile that's being checked by Jerry:
                uint8_t touch_tile = WannaTouchIt();

                //if Jerry doesn't have anything to mess with:
                if(!touch_tile)
                {
                    Jerry.canFire = true;
                }
                else if(!Jerry.last_2nd) //if he can, check what to do with it:
                {
                    //things after tile 9 in the arena are interactables:
                    if((arena.map != hub_room) && (touch_tile >= 9))
                    {
                        //tiles 9 and 10 are for bread input only:
                        if(touch_tile <= 10)
                        {
                            if(Jerry.inventory == 1)
                            {
                                Jerry.inventory = 0;

                                stats.score += 1;
                            }
                        }
                        else //tiles 11 and 12 are for meat:
                        if(touch_tile <= 12)
                        {
                            if(Jerry.inventory == 2)
                            {
                                Jerry.inventory = 0;

                                //since it's hard to reach the slots in the corner, here's double points:
                                stats.score += 2;
                            }
                        }
                        else //tiles 13 and 14 are for cheese:
                        if(touch_tile <= 14)
                        {
                            if(Jerry.inventory == 3)
                            {
                                Jerry.inventory = 0;

                                stats.score += 2;
                            }
                        }
                    }

                    //if Jerry can mess with something and hasn't already he can't shoot:
                    Jerry.canFire = false;
                }
            }

            //update the current 2nd key state for the future:
            Jerry.last_2nd = (kb_Data[1] & kb_2nd);

            //if we're in an arena, spawn stuff and shoot fire:
            if(arena.map != hub_room)
            {
                //spawns enemies, I think that's pretty clear:
                SpawnEnemies();

                if((kb_Data[1] & kb_2nd) && Jerry.canFire)
                {
                    //time to start shootin'
                    Jerry.sprite_ptr = Jerry_firing_sprite;

                    if(fire.animation < 5)
                    {
                        ++fire.animation;
                    } else {
                        fire.animation = 0;
                    }

                    //update Jerry's temp:
                    if((Jerry.temperature + 4) < 100)
                    {
                        Jerry.temperature += randInt(1, 4);
                    } else {
                        Jerry.temperature = 100;
                    }
                }
                else
                {
                    Jerry.sprite_ptr = Jerry_weaponized_sprite;

                    if((Jerry.temperature - 4) > 0)
                    {
                        Jerry.temperature -= randInt(1, 4);
                    } else {
                        Jerry.temperature = 0;
                    }
                }
            }

            //if Jerry's skirting around at a godly speed, then update his walking animation:
            if(Jerry.moving)
            {
                if((Jerry.animation == 0) || (7 < Jerry.animation))
                {
                    Jerry.animationToggle *= -1;
                }

                Jerry.animation += Jerry.animationToggle;

                //reset the moving variable to be re-evaluated by the movement code:
                Jerry.moving = false;
            }
            else
            {
                //as soon as the animation needs to be updated, it goes to the next frame; since otherwise Jerry can
                //slide around with short keypresses. (2 * 3) - 1 = frame 5
                Jerry.animation = 5;
            }

            //Now that Jerry gets his head start, run through the enemy logic and animations:
            for(uint8_t i = 0; i < numEntities; ++i)
            {
                //the enemies target points near Jerry so they can cover his hitbox most effectively:
                int24_t targetX;
                int24_t targetY;

                //if the entity is ded, figure out what it was and if it needs to drop something:
                if(entities[i].health < 1)
                {
                    if(entities[i].sprite_ptr == slamwhich_tiles) //if the dead thing's a slammwhich:
                    {
                        //if a slamwhich died, set up it's drop where the animation is the ingredient type:
                        entities[i].sprite_ptr = ingredients_tiles;
                        entities[i].animation = randInt(0, 2) * 3;

                        //put it at the slamwhiches' feet so I don't have to defrag the list:
                        entities[i].x += 10;
                        entities[i].y += 28;

                        //it loses health as it sits, but immediately dies on pickup as well:
                        entities[i].health = 127;

                        //12 pixel offset from the floor for all ingredients:
                        entities[i].floor_offset = 11;
                    }
                    else
                    {
                        //"pull" all the entities down the list to overwrite the now nonexistant entity:
                        for(uint8_t j = i; j < MAX_ENTITIES; ++j)
                        {
                            if(j < (numEntities - 1))
                            {
                                entities[j] = entities[j + 1];
                            } else {
                                entities[j].y = 10000; //redundancy that also fixes a few bugs
                            }
                        }
                        //update the number of used entity slots:
                        --numEntities;
                    }
                }
                else
                {
                    if(entities[i].sprite_ptr == slamwhich_tiles)
                    {
                        //damage the enemy and defrag the entities list if it dies:
                        if(Jerry.canFire && (kb_Data[1] & kb_2nd) && gfx_CheckRectangleHotspot(fire.x, fire.y, fire.width, fire.height, entities[i].x + 2, entities[i].y + 18, SLAM_WIDTH - 1, SLAM_HEIGHT + 9))
                        {
                            if(entities[i].health > 0)
                            {
                                --entities[i].health;
                            }

                            //normally the enemy would be dead, but the check has already passed, so they get one more frame of existence:
                            if(entities[i].health < 1)
                            {
                                //freeze the animation on the one where they scream in agony as they burn:
                                entities[i].animToggle = 0;
                                entities[i].animation = 6;
                                //YES, BURN! BURN! BURN YOU LITTLE DEMONS WITH THE STUPID HITBOXES THAT WON'T WORK RIGHT!
                            }
                        }

                        if(entities[i].delay < 1)
                        {
                            //do when Jerry isn't using invincibility frames, damage him:
                            if((Jerry.damageFlicker >= (FLICKER_TIME + 1)) && (entities[i].health > 0))
                            {
                                //checks if the slammwhich touched Jerry:
                                if(gfx_CheckRectangleHotspot(Jerry.x, Jerry.y, J_HB_WIDTH - 1, J_HB_HEIGHT - 1, entities[i].x + 2, entities[i].y + 28, SLAM_WIDTH - 1, SLAM_HEIGHT - 1))
                                {
                                    Jerry.damageFlicker = 0;
                                    --Jerry.health;
                                }
                            }

                            //Jerry's x coord, with displacement so the slamwhich is trying to center on him:
                            targetX = Jerry.x + (J_HB_WIDTH - SLAM_WIDTH);
                            targetY = Jerry.y - (40 - SLAM_HEIGHT);

                            //move on x-axis towards the x that will rek Jerry the most:
                            if((entities[i].x + SLAM_SPEED) < targetX)
                            {
                                entities[i].x += SLAM_SPEED;
                            }
                            else if((entities[i].x - SLAM_SPEED) > targetX)
                            {
                                entities[i].x -= SLAM_SPEED;
                            }

                            //move on y-axis, we have to defrag the list too:
                            if((entities[i].y + SLAM_SPEED) < targetY)
                            {
                                entities[i].y += SLAM_SPEED;
                            }
                            else if((entities[i].y - SLAM_SPEED) > targetY)
                            {
                                entities[i].y -= SLAM_SPEED;
                            }
                        }
                    }
                    else if(entities[i].sprite_ptr == ingredients_tiles) //if it's any ingredient type:
                    {
                        //if Jerry touches the ingredient drop and has an empty inventory:
                        if(!Jerry.inventory && gfx_CheckRectangleHotspot(entities[i].x, entities[i].y, 12, 12, Jerry.x, Jerry.y, J_HB_WIDTH - 1, J_HB_HEIGHT - 1))
                        {
                            //mark it as "ded" so it can be handled later:
                            entities[i].health = 0;

                            //and put it's ingredient value in Jerry's inventory plus one, since zero is empty:
                            Jerry.inventory = 1 + (entities[i].animation / 3);
                        }

                        --entities[i].health;
                    }

                    //flip the animation to move in the opposite direction through the array's images:
                    if((entities[i].animation <= 0 ) || (entities[i].animation >= 5))
                    {
                        entities[i].animToggle *= -1;
                    }

                    entities[i].animation += entities[i].animToggle;
                }

                if(entities[i].delay > 0)
                {
                    --entities[i].delay;
                }
            }

            //I know what you're thinking "We didn't organize the lists and now things are funky, what do we do?"
            //Why defrag them by sorting in ascending order of course:
            if(numEntities > 1)
            {
                DefragEntities();
            }
        }

        //update the highscore:
        if(stats.score > stats.highscore)
        {
            stats.highscore = stats.score;
        }

        //FPS counter data collection, time "stops" (being relevant) after this point; Zeroko told me to disable the timer
        //when reading but I'm measuring such a small value I *should* be fine, will do if I use more timers:
        FPS = (32768 / timer_1_Counter);

        //that's gonna bite me in the butt so hard later on, I just know it.

        //if the FPS is greater than what I want, delay long enough to display slow down to the FPS lock:
        if(FPS > FPS_LOCK) delay((1000 / FPS_LOCK) - (1000 / FPS));

        //reset the timer:
        timer_1_Counter = 0;

        //here's a simple local variable used in the upcoming entity drawing to make sure I did Jerry correctly:
        bool JerryDoneBeenDrawn = false;
        //I don't like how I did this, so it stays.

        //draw everything if Jerry's alive, else we go to the void:
        if(Jerry.health > 0)
        {
            //all drawing should go after this:
            gfx_Tilemap(&arena, room.camera_x, room.camera_y);

            //this is the "pause menu" in all it's glory, and I'm happy that I made something simple for once:
            if(kb_Data[1] & kb_Del)
            {
                //put the base room image on the screen:
                gfx_BlitBuffer();

                gfx_SetTextScale(3, 3);

                gfx_SetTextFGColor(1);
                gfx_PrintStringXY("Paused", 69, 109); //Nice.
                gfx_SetTextFGColor(2);
                gfx_PrintStringXY("Paused", 66, 106);

                //now swap it so the buffer is the same as before the blit and the screen has a copy with "Paused" on it:
                gfx_SwapDraw();

                gfx_SetTextScale(1, 1);

                //wait until the key is released, then pressed again, and then released once more:
                do {kb_Scan();} while((kb_Data[1] & kb_Del) && !(kb_Data[6] & kb_Clear));
                do {kb_Scan();} while((!(kb_Data[1] & kb_Del)) && !(kb_Data[6] & kb_Clear));
                do {kb_Scan();} while((kb_Data[1] & kb_Del) && !(kb_Data[6] & kb_Clear));

                //skip the last drawing portion if we don't need to do it:
                if(kb_Data[6] & kb_Clear) break;

                //It doesn't use at least 2 locals and 1 globals so it feels too easy.
            }

            //work from the back entities forwards to layer them properly, interrupting to draw Jerry if necessary:
            for(uint8_t i = 0; i < numEntities; ++i)
            {
                //very cheaty way of drawing the avatar, since all the other ways need so much overhead:
                if(((entities[i].y + entities[i].floor_offset) > (Jerry.y + J_HB_HEIGHT)) && !JerryDoneBeenDrawn)
                {
                    //remember that big-ol' function? This is case one for it:
                    DrawJerry();

                    JerryDoneBeenDrawn = true;
                }
                //draw the entity, be it an enemy or an ingredient:
                gfx_TransparentSprite(entities[i].sprite_ptr[entities[i].animation/3], entities[i].x - room.camera_x, entities[i].y - room.camera_y);
            }

                //if there's nothing to draw but Jerry, then draw him:
            if(!JerryDoneBeenDrawn)
            {
                //and here's the second case, which is just if it fails to draw at all:
                DrawJerry();
            }

            //this just quickly draws the heart sprites, pretty simple stuff:
            for(uint8_t i = 2; i < (MAX_HEALTH * 18); i += 18)
            {
                if(i < (Jerry.health * 18))
                {
                    gfx_TransparentSprite_NoClip(heart_tiles[0], i, 2);
                } else {
                    gfx_TransparentSprite_NoClip(heart_tiles[1], i, 2);
                }
            }

            //things that only need to be done in-battle and not in the hub:
            if(arena.map != hub_room)
            {
                //draw heads-up in lower left corner, for temperature monitoring and timer:
                gfx_TransparentSprite_NoClip(hud, 0, 224);
                gfx_TransparentSprite_NoClip(timer, 269, 224);
                gfx_TransparentSprite_NoClip(scorebar, 102, 0);

                //red color for temp. gauge:
                gfx_SetColor(3);
                gfx_FillRectangle_NoClip(3 + Jerry.temperature, 228, 100 - Jerry.temperature, 9);

                //if Jerry's holding something, draw it in the inventory slot:
                if(Jerry.inventory)
                {
                    gfx_TransparentSprite_NoClip(ingredients_tiles[Jerry.inventory - 1], 306, 226);
                }

                gfx_SetTextFGColor(2);

                gfx_SetTextXY(157, 1);
                gfx_PrintUInt(stats.highscore, 6);
                gfx_SetTextXY(157, 9);
                gfx_PrintUInt(stats.score, 6);

                //get the timer all nice and centered next to the inventory slot:
                gfx_SetTextXY(280, 229);

                //if the time's under the game time, then display it on the HUD, else display the max time and draw the countdown
                //in the middle and go when time is equal to the game time:
                if(time < (GAME_TIME * FPS_LOCK))
                {
                    gfx_PrintUInt(time / FPS_LOCK, 1);

                    //Jerry can move now when the game is running and he's not dead:
                    if(Jerry.health > 0)
                    {
                        gaming = true;
                    }
                }
                else
                {
                    gfx_PrintUInt(GAME_TIME, 1);

                    if((time / FPS_LOCK) != GAME_TIME)
                    {
                        if((time / FPS_LOCK) <= GAME_TIME + 3)
                        {
                            uint8_t countdown = (time / FPS_LOCK) - GAME_TIME;

                            //I have a really smart idea, I can't center the countdown, so I'll just have it travel across the screen!
                            //Now it's more complicated!

                            gfx_SetTextScale(countdown + 2, countdown + 2);

                            gfx_SetTextFGColor(1);
                            gfx_SetTextXY((countdown * 73) + 3, (countdown * 53) + 3);
                            gfx_PrintUInt(countdown, 1);

                            gfx_SetTextFGColor(2);
                            gfx_SetTextXY(countdown * 73, countdown * 53);
                            gfx_PrintUInt(countdown, 1);
                        }
                    }
                    else //if the remaining countdown equals zero:
                    {
                        gfx_SetTextScale(6, 6);

                        gfx_SetTextFGColor(1);
                        gfx_PrintStringXY("Go!", 109, 102);

                        gfx_SetTextFGColor(2);
                        gfx_PrintStringXY("Go!", 106, 99);
                    }
                    gfx_SetTextScale(1, 1);
                }

                //because of the FPS lock, 20 loops is one second, and it's affected by extra lag so cheesing it is harder
                if((time > 0) && Jerry.health)
                {
                    --time;
                }
                else if(!numEntities)
                {
                    //if the time is up and all the entities are gone, add some spawn delay and start over:
                    time = (GAME_TIME + 4) * FPS_LOCK;
                    spawnDelay = 3 * FPS_LOCK;

                    //oh, and here's some regen because I'm a nice guy:
                    if(Jerry.health < MAX_HEALTH)
                    {
                        ++Jerry.health;
                    }
                }
                //but I'm sure someone will figure out a way to mess it up, probably me :P

                //if the temp goes to high, flash a hot color (indices 4-7) for the explosion:
                if(Jerry.temperature >= 100)
                {
                    gfx_FillScreen(randInt(4, 7));
                    
                    //instant death, keep an eye on those temp meters kids:
                    Jerry.health = 0;
                }
            }

            //run a final check on if transport should actually be transport or not:
            if(transport)
            {
                ConfirmTransport();
            }
        }
        else
        {
            if(gaming)
            {
                //Welcome to the void!
                DeathScreen();
            }
        }

        //respawn back at apartment if health drops to zero or player decides to restart:
        if(transport)
        {
            //I honestly don't know if this is a good idea, but here's a static variable:
            static uint8_t scroll;

            //mark the the current room the room to travel too since one could quit mid-transition:
            current_room = target_room;

            while(!(kb_Data[6] & kb_Clear))
            {
                kb_Scan();

                //fill the screen with black:
                gfx_FillScreen(1);

                //blit the rows at a given y and height, makes it look like the black color is moving:
                gfx_BlitLines(gfx_buffer, scroll, 1);
                gfx_BlitLines(gfx_buffer, 239 - scroll, 1);

                if(scroll >= 120)
                {
                    transport = false;

                    //since scroll is static, it needs resetting:
                    scroll = 0;

                    //reset the score:
                    stats.score  = 0;

                    //Ewww, a goto!
                    goto GAMESTART;
                    //it's only 14 bytes though, so I guess it works... for now.
                }
                else
                {
                    scroll += 1;
                }
            }
        }
        else
        {
            //print the FPS, most people don't want to see this so I've commented it out:
            //gfx_SetTextXY(290, 13);
            //gfx_PrintInt(FPS, 1);

            /*//Handy-dandy debuggin' bit!
            for(uint8_t i = 0; i < MAX_ENTITIES; ++i)
            {
                gfx_SetTextXY(280, 13 + i*10);
                gfx_PrintInt(entities[i].animToggle, 1);
            }*/

            //swap the buffer and the screen, IT SWAPS, NOT BLITS! TAKE NOTE 2AM ME! I swear I can be so stupid sometimes...
            gfx_SwapDraw();
            //I spent hours trying to debug buffer artifacts and it was just SwapDraw(); that was a while ago on a different
            //project, and boy did I suffer that night!
        }

        //Most of my characters are somewhat ethics-blind, so respawning after death shouldn't be a big deal except as a
        //security threat and maybe a possible method of torture to them.

    } while(!(kb_Data[6] & kb_Clear));

    gfx_End();

    //save the game:
    SaveGame();

    //Mateo told me this was important for shells and it's a professional convention, so it stays:
    return 0;
}

//In the words of my good buddy Lance: "Cringe."