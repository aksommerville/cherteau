/* shared_symbols.h
 * Consumed by both the game and the tools.
 */

#ifndef SHARED_SYMBOLS_H
#define SHARED_SYMBOLS_H

#define NS_sys_tilesize 16
#define NS_sys_mapw 20
#define NS_sys_maph 10
#define NS_sys_worldw 24
#define NS_sys_worldh 18 /* Top bar is 20 pixels; leave 2 margin. */

#define CMD_map_image     0x20 /* u16:imageid */
#define CMD_map_location  0x40 /* u8:long u8:lat u8:plane u8:reserved */
#define CMD_map_sprite    0x61 /* u16:pos u16:spriteid u32:reserved */
#define CMD_map_door      0x62 /* u16:pos u16:mapid u16:dstpos u16:reserved */

#define CMD_sprite_image  0x20 /* u16:imageid */
#define CMD_sprite_tile   0x21 /* u8:tileid u8:xform */

#define NS_tilesheet_physics     1
#define NS_tilesheet_neighbors   0
#define NS_tilesheet_family      0
#define NS_tilesheet_weight      0

#define NS_physics_vacant 0
#define NS_physics_solid 1

#endif
