#pragma once

#include "vmsys.h"
#include "vmgraph.h"
#include "vmio.h"
#include "vmtimer.h"
#include "vmchset.h"
#include "vmstdlib.h"

#include "FastNoiseLite.h"

#include <stdio.h>
#include <stdlib.h>

// Defines
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define BLOCK_WIDTH 24
#define BLOCK_HEIGHT 24
#define WORLD_WIDTH 256
#define WORLD_HEIGHT 256

typedef enum Direction {
    DIR_NORTH,
    DIR_NORTHEAST,
    DIR_EAST,
    DIR_SOUTHEAST,
    DIR_SOUTH,
    DIR_SOUTHWEST,
    DIR_WEST,
    DIR_NORTHWEST
} Direction;

// Game state, i.e. currently visible screen/menu
typedef enum State {
    ST_INGAME,
    ST_INVENTORY
} State;

typedef struct InventoryItem {
    char id;
    char count;
} InventoryItem;

// Data stored in the player info file
typedef struct Player {
    int x;
    int y;
    int depth;
    Direction direction;

    InventoryItem items[32];
    int selectedItem;
} Player;

// Data of one block stored in the world file
typedef struct Block {
	char background;
	char foreground;
} Block;

// 2bpp image structure. Palettes are defined separately and are supplied in drawImage calls.
typedef struct Image {
	char metadata;
	char data[];
} Image;

// Image data used to draw a block (2bpp image paired with a palette).
// Some blocks reuse the same image but with a different palette.
typedef struct BlockImage {
    Image *image;
    char paletteIndex;
} BlockImage;