#pragma once

short palettes[][3] = {
	{ 0x6180, 0x04A6, 0x37F2 },  // 0: brown, green, light green
	{ 0x632C, 0xC4A6, 0xC652 },  // 1: gray, dark sand, sand
	{ 0x3192, 0x64B8, 0xFFFF },  // 2: blue, light blue, white
	{ 0x0000, 0x6180, 0xC4AC },  // 3: black, brown, skin
};

BlockImage bgBlocks[] = {
	{&img_ground, 0},  // grass
	{&img_short_grass, 0},
	{&img_tall_grass, 0},
	{&img_ground, 1},  // sand
	{&img_ground, 2},  // snow
	{&img_ice, 2},
	{&img_water, 2},
};

BlockImage fgBlocks[] = {
	{&img_tree, 0},
	{&img_dead_tree, 0},
	{&img_fallen_tree, 0},
	{&img_cactus, 0},
	{&img_rock, 1},
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