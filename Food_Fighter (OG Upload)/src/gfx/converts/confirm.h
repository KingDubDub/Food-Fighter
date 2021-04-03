#ifndef confirm_include_file
#define confirm_include_file

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char confirm_tile_0_data[16802];
#define confirm_tile_0 ((gfx_sprite_t*)confirm_tile_0_data)
extern unsigned char confirm_tile_1_data[16802];
#define confirm_tile_1 ((gfx_sprite_t*)confirm_tile_1_data)
#define confirm_num_tiles 2
extern unsigned char *confirm_tiles_data[2];
#define confirm_tiles ((gfx_sprite_t**)confirm_tiles_data)

#ifdef __cplusplus
}
#endif

#endif
