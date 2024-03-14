#pragma once

short palettes[][3] = {
	{ 0x6180, 0x04A6, 0x37F2 },  // 0: brown, green, light green
	{ 0x632C, 0xC4A6, 0xC652 },  // 1: gray, dark sand, sand
	{ 0x3192, 0x64B8, 0xFFFF },  // 2: blue, light blue, white
	{ 0x0000, 0x6180, 0xC4AC },  // 3: black, brown, skin
	{ 0x0000, 0x632C, 0x0000 },  // 4: black, gray, unused
};

// Background blocks (generated as part of the world, cannot be placed or broken)
BlockInfo bgBlocks[] = {
	{&img_ground, 0, 0, 0},  // grass
	{&img_short_grass, 0, 0, 0},
	{&img_tall_grass, 0, 0, 0},
	{&img_ground, 1, 0, 0},  // sand
	{&img_ground, 2, 0, 0},  // snow
	{&img_ice, 2, 0, 0},
	{&img_water, 2, 0, 0},
};

// Items and foreground blocks
BlockInfo fgBlocks[] = {
	//                 .- palette
	//                 |  .- dropped item id
	//                 |  |   .- dropped item count  
	//                 |  |   |  .- can be placed
	//                 |  |   |  |  .- requires pickaxe to break
	// image           |  |   |  |  |  .- can be placed on water
	{&img_tree,        0, 6,  4, 1, 0, 0},
	{&img_dead_tree,   0, 6,  2, 1, 0, 0},
	{&img_fallen_tree, 0, 6,  2, 1, 0, 0},
	{&img_cactus,      0, 4,  1, 1, 0, 0},
	{&img_rock,        1, 5,  1, 1, 1, 1},
	{&img_planks,      3, 6,  1, 1, 0, 1},
	{&img_workbench,   3, 7,  1, 1, 0, 0},
	{&img_pickaxe,     3, 0,  0, 0, 0, 0},
	{&img_stick,       3, 0,  0, 0, 0, 0},
	{&img_bridge,      4, 10, 1, 1, 1, 1},
	{&img_hole,        3, 0,  0, 0, 0, 0},
};

Image *playerImages[] = {
	&img_player_n,
	&img_player_ne,
	&img_player_e,
	&img_player_se,
	&img_player_s,
	&img_player_sw,
	&img_player_w,
	&img_player_nw,
};

Recipe recipes[] = {
	{{8, 1}, {{6, 48}, {0, 0}}},
	{{5, 2}, {{6, 1}, {6, 1}, {6, 1}, {6, 1}}},
	{{10, 1}, {{5, 2}, {0, 0}}},
	{0}
};