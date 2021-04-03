/*  NEEDLESS OPTIMIZATION

The contest has a category for well-written code, I'm going to TANK that.
Thankfully I can still optimzize what I know to optimize, and one way is to shave down my sprites
and remove any extra rows/columns I can, and then add them to their own arrays. It's harder than
just using convimg, but I don't have enough humanity left in me to care at this point.

Note that this will most likely mess with compressed sizes, but the overall RAM usage to store the
decompressed sprites will be greatly reduced.

*/

#include <stddef.h>

#include <graphx.h>

//LET THE OPTIMIZATION COMMENCE!

//Jerry's avatar array:
extern unsigned char
    Jerry0_data[],
    Jerry1_data[],
    Jerry2_data[],
    Jerry3_data[],
    Jerry4_data[],
    Jerry5_data[],
    Jerry6_data[],
    Jerry7_data[],
    Jerry8_data[],
    Jerry9_data[],
    Jerry10_data[],
    Jerry11_data[];

unsigned char *Jerry_sprite_data[] =
{
    Jerry0_data,
    Jerry1_data,
    Jerry2_data,
    Jerry3_data,
    Jerry4_data,
    Jerry5_data,
    Jerry6_data,
    Jerry7_data,
    Jerry8_data,
    Jerry9_data,
    Jerry10_data,
    Jerry11_data
};

#define Jerry_sprite ((gfx_sprite_t**)Jerry_sprite_data)

//Jerry with his snazzy new backpack flamethrower that gives me Luigi's Mansion vibes:
extern unsigned char
    Jerry_weaponized0_data[],
    Jerry_weaponized1_data[],
    Jerry_weaponized2_data[],
    Jerry_weaponized3_data[],
    Jerry_weaponized4_data[],
    Jerry_weaponized5_data[],
    Jerry_weaponized6_data[],
    Jerry_weaponized7_data[],
    Jerry_weaponized8_data[],
    Jerry_weaponized9_data[],
    Jerry_weaponized10_data[],
    Jerry_weaponized11_data[];

unsigned char *Jerry_weaponized_sprite_data[] =
{
    Jerry_weaponized0_data,
    Jerry_weaponized1_data,
    Jerry_weaponized2_data,
    Jerry_weaponized3_data,
    Jerry_weaponized4_data,
    Jerry_weaponized5_data,
    Jerry_weaponized6_data,
    Jerry_weaponized7_data,
    Jerry_weaponized8_data,
    Jerry_weaponized9_data,
    Jerry_weaponized10_data,
    Jerry_weaponized11_data
};

#define Jerry_weaponized_sprite ((gfx_sprite_t**)Jerry_weaponized_sprite_data)

//OH MITOSIS GUYS HE HAS A FRIGGEN GUN AND ANGRY EYES:
extern unsigned char
    Jerry_firing0_data[],
    Jerry_firing1_data[],
    Jerry_firing2_data[],
    Jerry_firing3_data[],
    Jerry_firing4_data[],
    Jerry_firing5_data[],
    Jerry_firing6_data[],
    Jerry_firing7_data[],
    Jerry_firing8_data[],
    Jerry_firing9_data[],
    Jerry_firing10_data[],
    Jerry_firing11_data[];

unsigned char *Jerry_firing_sprite_data[] =
{
    Jerry_firing0_data,
    Jerry_firing1_data,
    Jerry_firing2_data,
    Jerry_firing3_data,
    Jerry_firing4_data,
    Jerry_firing5_data,
    Jerry_firing6_data,
    Jerry_firing7_data,
    Jerry_firing8_data,
    Jerry_firing9_data,
    Jerry_firing10_data,
    Jerry_firing11_data
};

#define Jerry_firing_sprite ((gfx_sprite_t**)Jerry_firing_sprite_data)

//Jerry is ded and looks like something out of a creepypasta:
extern unsigned char
    Jerry_ded0_data[],
    Jerry_ded1_data[],
    Jerry_ded2_data[],
    Jerry_ded3_data[];

unsigned char *Jerry_ded_sprite_data[] =
{
    Jerry_ded0_data,
    Jerry_ded1_data,
    Jerry_ded2_data,
    Jerry_ded3_data,
};

#define Jerry_ded_sprite ((gfx_sprite_t**)Jerry_ded_sprite_data)

//Consider my pyromania and I absolutely triggered:
extern unsigned char
    fire0_data[],
    fire1_data[],
    fire2_data[],
    fire3_data[],
    fire4_data[],
    fire5_data[],
    fire6_data[],
    fire7_data[],
    fire8_data[],
    fire9_data[],
    fire10_data[],
    fire11_data[];

unsigned char *fire_sprite_data[] =
{
    fire0_data,
    fire1_data,
    fire2_data,
    fire3_data,
    fire4_data,
    fire5_data,
    fire6_data,
    fire7_data,
    fire8_data,
    fire9_data,
    fire10_data,
    fire11_data
};

#define fire_sprite ((gfx_sprite_t**)fire_sprite_data)