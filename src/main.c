#define FNL_IMPL

#include "common.h"
#include "images.h"
#include "blocks.h"

// MRE graphics variables
VMINT layer_hdl;
char *layer_buf;
const VMINT screen_width = SCREEN_WIDTH;
const VMINT screen_height = SCREEN_HEIGHT;
vm_graphic_color color;

// Application variables
State state;
fnl_state fnlState;
VMFILE world_file;
VMFILE player_file;
int blockCacheIndex = -16;
Block blockCache[16];

Player player;

int inventoryX;
int inventoryY;

void setState(State newState) {
	state = newState;
	switch (state) {
		case ST_INVENTORY: {
			inventoryX = player.selectedItem % 8;
			inventoryY = player.selectedItem / 8;
			break;
		}
	}
}

void drawImage(Image *img, short palette[3], int x, int y) {
	// Image metadata byte: 0b0000wwhh
	// Image width/height are defined as multiples of 4 pixels:
	// 0b00 = 4, 0b01 = 8, 0b10 = 12, 0b11 = 16
	int width = (img->metadata >> 2) + 1;
	int height = ((img->metadata & 0b11) + 1)*4;
	
	for (int sy = 0; sy < height; sy++) {
		for (int sx = 0; sx < width; sx++) {
			// Draw byte (in 2bpp, one byte = 4 pixels)
			char byte = img->data[sy*width + sx];
			for (int i = 3; i >= 0; i--) {
				char pixel = (byte >> (i*2)) & 0b11;
				if (!pixel) continue;

				int destX = x + sx*8 + 6 - i*2;
				int destY = y + sy*2;
				if (destX < 0 || destY < 0) continue;
				if (destX >= SCREEN_WIDTH || destY >= SCREEN_HEIGHT) continue;

				int offset = destY*SCREEN_WIDTH + destX;

				((VMUINT16 *)layer_buf)[offset] = palette[pixel - 1];
				((VMUINT16 *)layer_buf)[offset + 1] = palette[pixel - 1];
				((VMUINT16 *)layer_buf)[offset + SCREEN_WIDTH] = palette[pixel - 1];
				((VMUINT16 *)layer_buf)[offset + SCREEN_WIDTH + 1] = palette[pixel - 1];
			}
		}
	}
}

// Checks if the coordinates are out of world bounds
int outOfBounds(int x, int y) {
	return x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT;
}

// Gets the block at the given coordinates.
// This directly reads from the world file (or its cache) without performing any
// checks; those checks should be performed by the caller function.
Block getBlock(int x, int y) {
	int index = y*WORLD_WIDTH + x;

	// If this block is not in cache, refill the cache with this block and some to the right of it
	if (index < blockCacheIndex || index > blockCacheIndex + 15) {
		int unused;
		vm_file_seek(world_file, index*sizeof(Block), BASE_BEGIN);
		vm_file_read(world_file, blockCache, 16*sizeof(Block), &unused);
		blockCacheIndex = index;
	}
	
	return blockCache[index - blockCacheIndex];
}

// Sets the block at the given coordinates.
// This directly writes to the world file without performing any checks.
void setBlock(int x, int y, Block block) {
	int index = y*WORLD_WIDTH + x;
	int unused;
	vm_file_seek(world_file, index*sizeof(Block), BASE_BEGIN);
	vm_file_write(world_file, (void *)&block, sizeof(Block), &unused);

	// Write to cache if this block is in cache
	if (index >= blockCacheIndex && index < blockCacheIndex + 16) {
		blockCache[index - blockCacheIndex] = block;
	}
}

// Draws a block on the screen.
void drawBlock(int x, int y) {
	char numStr[4];
	short numStrUcs[4];

	int screenX = screen_width/2 - BLOCK_WIDTH/2 + (x - player.x)*BLOCK_WIDTH;
	int screenY = screen_height/2 - BLOCK_HEIGHT/2 + (y - player.y)*BLOCK_HEIGHT;

	if (screenX + BLOCK_WIDTH < 0 || screenY + BLOCK_HEIGHT < 0) return;
	if (screenX > screen_width || screenY > screen_height) return;
	
	color.vm_color_565 = VM_COLOR_BLACK;
	vm_graphic_setcolor(&color);
	vm_graphic_fill_rect_ex(layer_hdl, screenX, screenY, BLOCK_WIDTH - 1, BLOCK_HEIGHT - 1);

	Block block;
	if (outOfBounds(x, y)) block = (Block) {6, 0}; // water
	else block = getBlock(x, y);

	drawImage(
		bgBlocks[block.background].image,
		palettes[bgBlocks[block.background].paletteIndex],
		screenX, screenY
	);

	if (!block.foreground) return;
	drawImage(
		fgBlocks[block.foreground - 1].image,
		palettes[fgBlocks[block.foreground - 1].paletteIndex],
		screenX, screenY
	);
}

// Draws the block that the player is standing on, then the player
void drawPlayer() {
	drawBlock(player.x, player.y);
	drawImage(
		playerImages[player.direction], palettes[3],
		screen_width/2 - BLOCK_WIDTH/2,
		screen_height/2 - BLOCK_HEIGHT/2
	);
}

void drawInventoryItem(InventoryItem item, int x, int y) {
	char itemCount[4];
	short itemCountUcs[4];

	drawImage(
		fgBlocks[item.id - 1].image,
		palettes[fgBlocks[item.id - 1].paletteIndex],
		x + 3, y + 3
	);

	if (item.count == 1) return;
	sprintf(itemCount, "%d", item.count);
	vm_ascii_to_ucs2(itemCountUcs, 8, itemCount);

	// Draw item count under the item
	color.vm_color_565 = VM_COLOR_BLACK;
	vm_graphic_setcolor(&color);
	vm_graphic_textout_to_layer(layer_hdl, x + 2, y + 26, itemCountUcs, 255);
}

void drawHUD() {
	const InventoryItem selItem = player.items[player.selectedItem];

	// Draw frame around selected item
	color.vm_color_565 = VM_COLOR_BLACK;
	vm_graphic_setcolor(&color);
	vm_graphic_fill_rect_ex(layer_hdl, 0, 274, 36, 46);
	color.vm_color_565 = VM_COLOR_WHITE;
	vm_graphic_setcolor(&color);
	vm_graphic_fill_rect_ex(layer_hdl, 0, 276, 34, 44);

	// Draw selected item
	if (selItem.count < 1) return;
	drawInventoryItem(selItem, 4, 276);
}

// Draws the world (all the blocks around the player).
void drawWorld() {
	for (int y = -7; y <= 7; y++) {
		for (int x = -5; x <= 5; x++) {
			// if (x == 0 && y == 0) continue;
			drawBlock(player.x + x, player.y + y);
		}
	}
	drawPlayer();
	drawHUD();
}

// Moves the player in the specified direction (delta X/Y)
void movePlayer(int dX, int dY) {
	int newX = player.x + dX;
	int newY = player.y + dY;

	if (outOfBounds(newX, newY)) return;
	if (getBlock(newX, newY).foreground) return;

	player.x = newX;
	player.y = newY;
	drawWorld();
}

// Gives an item to the player. Returns 1 if succeeded, 0 if failed (inventory full).
// If dry is 1, it only checks if the player has enough space for the item.
int giveItem(int id, int count, int dry) {
	// Make a new inventory where the item will be added. If adding the item fails
	// at any point (inventory fills up), we simply discard the new inventory.
	InventoryItem newItems[32];
	memcpy(&newItems, &player.items, sizeof(newItems));

	for (int i = 0; i < count; i++) {
		// Try to add item to existing slot with that item
		for (int s = 0; s < 32; s++) {
			if (newItems[s].id == id && newItems[s].count > 0 && newItems[s].count < 99) {
				newItems[s].count++;
				continue;
			}
		}

		// If that didn't work, try to add item to any empty slot
		for (int s = 0; s < 32; s++) {
			if (newItems[s].count < 1) {
				newItems[s].id = id;
				newItems[s].count = 1;

				// If player's selected slot is empty, switch to the newly filled slot
				if (newItems[player.selectedItem].count < 1) {
					player.selectedItem = s;
				}
				continue;
			}
		}

		// Inventory is full
		return 0;
	}

	// Adding the item succeeded, copy the new inventory to the player's inventory
	if (!dry) memcpy(&player.items, &newItems, sizeof(newItems));
	return 1;
}

// Removes an item from the player's inventory. Works similarly to giveItem.
int takeItem(int id, int count, int dry) {
	InventoryItem newItems[32];
	memcpy(&newItems, &player.items, sizeof(newItems));

	for (int i = 0; i < count; i++) {
		for (int s = 0; s < 32; s++) {
			if (newItems[s].id == id && newItems[s].count > 0) {
				if (!dry) newItems[s].count--;
				continue;
			}
		}
		return 0;
	}
	
	if (!dry) memcpy(&player.items, &newItems, sizeof(newItems));
	return 1;
}

// Places or removes a block in the specified direction (delta X/Y).
// If there is already a block, it is removed. Else the currently selected block is placed.
void placeBlock(int dX, int dY) {
	int newX = player.x + dX;
	int newY = player.y + dY;
	if (outOfBounds(newX, newY)) return;

	Block old_block = getBlock(newX, newY);
	if (old_block.foreground) {
		BlockInfo info = fgBlocks[old_block.foreground - 1];
		if (!giveItem(info.dropItem, info.dropItemCount, 0)) return;
		setBlock(newX, newY, (Block) {old_block.background, 0});
	} else {
		if (player.items[player.selectedItem].count > 0) {
			setBlock(newX, newY, (Block) {old_block.background, player.items[player.selectedItem].id});
			player.items[player.selectedItem].count--;
		}
	}
	drawBlock(newX, newY);
	drawHUD();
}

// Saves the world changes to the world file. Run at a regular interval and when exiting the app.
void save_world(VMINT tid) {
	vm_file_commit(world_file);

	int unused;
	vm_file_seek(player_file, 0, BASE_BEGIN);
	vm_file_write(player_file, &player, sizeof(Player), &unused);
	vm_file_commit(player_file);
}

void handle_sysevt(VMINT message, VMINT param);

void handle_keyevt_ingame(VMINT keycode) {
	switch (keycode) {
		case VM_KEY_UP: movePlayer(0, -1); player.direction = DIR_NORTH; break;
		case VM_KEY_DOWN: movePlayer(0, 1); player.direction = DIR_SOUTH; break;
		case VM_KEY_LEFT: movePlayer(-1, 0); player.direction = DIR_WEST; break;
		case VM_KEY_RIGHT: movePlayer(1, 0); player.direction = DIR_EAST; break;

		case VM_KEY_NUM1: placeBlock(-1, -1); player.direction = DIR_NORTHWEST; break;
		case VM_KEY_NUM2: placeBlock(0, -1); player.direction = DIR_NORTH; break;
		case VM_KEY_NUM3: placeBlock(1, -1); player.direction = DIR_NORTHEAST; break;
		case VM_KEY_NUM4: placeBlock(-1, 0); player.direction = DIR_WEST; break;
		case VM_KEY_NUM6: placeBlock(1, 0); player.direction = DIR_EAST; break;
		case VM_KEY_NUM7: placeBlock(-1, 1); player.direction = DIR_SOUTHWEST; break;
		case VM_KEY_NUM8: placeBlock(0, 1); player.direction = DIR_SOUTH; break;
		case VM_KEY_NUM9: placeBlock(1, 1); player.direction = DIR_SOUTHEAST; break;

		case VM_KEY_OK: setState(ST_INVENTORY); handle_sysevt(VM_MSG_PAINT, 0); return;
	}
	drawPlayer();
	vm_graphic_flush_layer(&layer_hdl, 1);
}

void handle_keyevt_inventory(VMINT keycode) {
	switch (keycode) {
		case VM_KEY_UP: if (inventoryY > 0) inventoryY--; break;
		case VM_KEY_DOWN: if (inventoryY < 3) inventoryY++; break;
		case VM_KEY_LEFT: if (inventoryX > 0) inventoryX--; break;
		case VM_KEY_RIGHT: if (inventoryX < 7) inventoryX++; break;

		case VM_KEY_OK: player.selectedItem = inventoryY*8 + inventoryX; // fall through
		case VM_KEY_RIGHT_SOFTKEY: setState(ST_INGAME); break;

		case VM_KEY_LEFT_SOFTKEY: {
			if (takeItem(6, 16, 0)) giveItem(7, 1, 0);
			break;
		}
	}
	handle_sysevt(VM_MSG_PAINT, 0);
}

void drawInventory() {
	// Clear background
	color.vm_color_565 = VM_COLOR_WHITE;
	vm_graphic_setcolor(&color);
	vm_graphic_fill_rect_ex(layer_hdl, 0, 0, screen_width, screen_height);

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 8; x++) {
			// Tile background, also serves as a highlight for the currently selected slot
			color.vm_color_565 = (inventoryX == x && inventoryY == y) ? 0xC658 : 0xE75C;
			vm_graphic_setcolor(&color);
			vm_graphic_fill_rect_ex(layer_hdl, 1 + x*30, 81 + y*43, 27, 40);

			InventoryItem item = player.items[y*8 + x];
			if (item.count < 1) continue;
			drawInventoryItem(item, x*30, 80 + y*43);
		}

		color.vm_color_565 = VM_COLOR_BLACK;
		vm_graphic_setcolor(&color);
		vm_graphic_textout_to_layer(layer_hdl, 200, 300, u"Back", 255);
		if (takeItem(6, 16, 1)) vm_graphic_textout_to_layer(layer_hdl, 3, 300, u"Workbench", 255);
	}
}

void handle_keyevt(VMINT event, VMINT keycode) {
	if (event != VM_KEY_EVENT_DOWN && event != VM_KEY_EVENT_REPEAT) return;

	switch (state) {
		case ST_INGAME: handle_keyevt_ingame(keycode); break;
		case ST_INVENTORY: handle_keyevt_inventory(keycode); break;
	}
}

// void handle_penevt(VMINT event, VMINT x, VMINT y) {
// }

void handle_sysevt(VMINT message, VMINT param) {
	switch (message) {
		case VM_MSG_CREATE:
		case VM_MSG_ACTIVE:
			// screen_width = vm_graphic_get_screen_width();
			// screen_height = vm_graphic_get_screen_height();
			layer_hdl = vm_graphic_create_layer(0, 0, screen_width, screen_height, VM_NO_TRANS_COLOR);
			vm_graphic_set_clip(0, 0, screen_width, screen_height);
			layer_buf = vm_graphic_get_layer_buffer(layer_hdl);
			break;
			
		case VM_MSG_PAINT: {
			switch (state) {
				case ST_INGAME: drawWorld(); drawHUD(); break;
				case ST_INVENTORY: drawInventory(); break;
			}
    		vm_graphic_flush_layer(&layer_hdl, 1);
			break;
		}
			
		case VM_MSG_INACTIVE:
		case VM_MSG_QUIT:
			if( layer_hdl != -1 ) {
				vm_graphic_delete_layer(layer_hdl);
			}
			save_world(0);
			break;
	}
}

Block generateBlock(int x, int y) {
	// Rivers
    fnlState.frequency = 0.03f;
    fnlState.fractal_type = FNL_FRACTAL_RIDGED;
	if (fnlGetNoise2D(&fnlState, x, y) > 0.8f) return (Block) {6, 0}; // water

	// Biomes
    fnlState.frequency = 0.015f;
    fnlState.fractal_type = FNL_FRACTAL_NONE;
	float noiseValue = fnlGetNoise2D(&fnlState, x, y);
	int decoValue = rand()%100;

	// Desert biome
	if (noiseValue < -0.22f) {
		if (decoValue < 90) return (Block) {3, 0}; // sand
		return (Block) {3, 4}; // cactus
	}

	// Plains biome
    if (noiseValue < 0.1f) {
		if (decoValue < 45) return (Block) {0, 0}; // grass
		if (decoValue < 75) return (Block) {1, 0}; // short grass
		if (decoValue < 90) return (Block) {2, 0}; // tall grass
		if (decoValue < 97) return (Block) {0, 1}; // tree
		return (Block) {0, 3}; // fallen tree
    }

	// Forest biome
    if (noiseValue < 0.28f) {
		if (decoValue < 5) return (Block) {0, 0}; // grass
		if (decoValue < 10) return (Block) {1, 0}; // short grass
		if (decoValue < 15) return (Block) {0, 3}; // fallen tree
		return (Block) {0, 1}; // tree
    }

	// Snow biome
	if (decoValue < 96) return (Block) {4, 0}; // snow
	if (decoValue < 97) return (Block) {4, 2}; // dead tree
	return (Block) {4, 3}; // fallen tree
}

void vm_main(void) {
	int unused;

	vm_file_mkdir(u"e:\\mtmine64\\");
	if (vm_file_get_attributes(u"e:\\mtmine64\\world.bin") < 0) {
		// World doesn't exist, initialize and generate it
		world_file = vm_file_open(u"e:\\mtmine64\\world.bin", MODE_CREATE_ALWAYS_WRITE, TRUE);

		fnlState = fnlCreateState();
    	fnlState.octaves = 1;
		fnlState.noise_type = FNL_NOISE_PERLIN;

		for (int i = 0; i < WORLD_WIDTH*WORLD_HEIGHT; i++) {
			Block block = generateBlock(i%WORLD_WIDTH, i/WORLD_HEIGHT);
			vm_file_write(world_file, &block, sizeof(Block), &unused);
		}
		save_world(0);
	} else {
		// World exists, load it
		world_file = vm_file_open(u"e:\\mtmine64\\world.bin", MODE_WRITE, TRUE);
	}

	if (vm_file_get_attributes(u"e:\\mtmine64\\player.bin") < 0) {
		// Player data doesn't exist, initialize it
		player_file = vm_file_open(u"e:\\mtmine64\\player.bin", MODE_CREATE_ALWAYS_WRITE, TRUE);
	} else {
		// Player data exists, load it
		player_file = vm_file_open(u"e:\\mtmine64\\player.bin", MODE_WRITE, TRUE);
		vm_file_read(player_file, &player, sizeof(Player), &unused);
	}
	
	// Initialize layer handle
	layer_hdl = -1;
	
	// Register MRE event handlers
	vm_reg_sysevt_callback(handle_sysevt);
	vm_reg_keyboard_callback(handle_keyevt);
	// vm_reg_pen_callback(handle_penevt);
	vm_create_timer_ex(20000, save_world);

	// vm_kbd_set_mode(VM_KEYPAD_2KEY_NUMBER);
	vm_switch_power_saving_mode(turn_off_mode);

	vm_graphic_set_font(VM_SMALL_FONT);
	// vm_font_set_font_size(10);
}