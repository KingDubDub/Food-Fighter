#ifndef ingredients_include_file
#define ingredients_include_file

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char ingredients_tile_0_data[146];
#define ingredients_tile_0 ((gfx_sprite_t*)ingredients_tile_0_data)
extern unsigned char ingredients_tile_1_data[146];
#define ingredients_tile_1 ((gfx_sprite_t*)ingredients_tile_1_data)
extern unsigned char ingredients_tile_2_data[146];
#define ingredients_tile_2 ((gfx_sprite_t*)ingredients_tile_2_data)
#define ingredients_num_tiles 3
extern unsigned char *ingredients_tiles_data[3];
#define ingredients_tiles ((gfx_sprite_t**)ingredients_tiles_data)

#ifdef __cplusplus
}
#endif

#endif
