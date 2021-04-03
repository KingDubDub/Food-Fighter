#ifndef heart_include_file
#define heart_include_file

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char heart_tile_0_data[242];
#define heart_tile_0 ((gfx_sprite_t*)heart_tile_0_data)
extern unsigned char heart_tile_1_data[242];
#define heart_tile_1 ((gfx_sprite_t*)heart_tile_1_data)
#define heart_num_tiles 2
extern unsigned char *heart_tiles_data[2];
#define heart_tiles ((gfx_sprite_t**)heart_tiles_data)

#ifdef __cplusplus
}
#endif

#endif
