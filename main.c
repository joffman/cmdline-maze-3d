#define _POSIX_C_SOURCE 199309L	/* timespec, clock_gettime */

#include <locale.h>	/* setlocale */
#include <math.h>
#include <stdlib.h>	/* qsort */
#include <string.h>
#include <time.h>

#include <ncurses.h>

#include "edge.h"

#define SCREEN_WIDTH 132
#define SCREEN_HEIGHT 43

#define MAP_WIDTH 16
#define MAP_HEIGHT 16
#define MAX_DISTANCE 23

#define PI 3.14159265359

#define FOV (PI / 4)


/********************************************************************************
 * Structs and typedefs.
 ********************************************************************************/
struct player {
	double x;
	double y;
	double alpha;
};

struct vec2d {
	double x;
	double y;
};

struct coordinates {
	int x;
	int y;
};


/********************************************************************************
 * Static variables.
 ********************************************************************************/
static const char map[] =
	"################"
	"#..............#"
	"#..............#"
	"#.........######"
	"#..............#"
	"#.......##.....#"
	"#.......##.....#"
	"###............#"
	"##.............#"
	"#.......###..###"
	"#.......#......#"
	"#.......#......#"
	"#..............#"
	"#.......########"
	"#..............#"
	"################";


/********************************************************************************
 * Free functions.
 ********************************************************************************/
double dot_product(struct vec2d v1, struct vec2d v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}

void init_ncurses(void)
{
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, 1);
	notimeout(stdscr, 1);
}

void move_player(struct player *player, long elapsed_nsec)
{
	struct vec2d player_unit_vec;
	player_unit_vec.x = sin(player->alpha);
	player_unit_vec.y = cos(player->alpha);

	int cmd = getch();
	struct player old_player = *player;
	const double movement_incr_factor = 0.0000001;
	switch (cmd) {
		case 'w':	/* forward */
			player->x += elapsed_nsec * movement_incr_factor * player_unit_vec.x;
			player->y += elapsed_nsec * movement_incr_factor * player_unit_vec.y;
			break;
		case 's':	/* backward */
			player->x -= elapsed_nsec * movement_incr_factor * player_unit_vec.x;
			player->y -= elapsed_nsec * movement_incr_factor * player_unit_vec.y;
			break;
		case 'a':	/* left */
			player->alpha += elapsed_nsec * movement_incr_factor;
			break;
		case 'd':	/* right */
			player->alpha -= elapsed_nsec * movement_incr_factor;
			break;
	}

	/* Collision detection. */
	if (map[(int) player->y * MAP_WIDTH + (int) player->x] == '#') {
		player->x = old_player.x;
		player->y = old_player.y;
	}
}

/*
 * Compute coordinates of the block that is hit by the given ray.
 */
double compute_block_distance(struct player player, struct vec2d ray_unit_vec) {
	double block_distance = 0.;
	const double step_size = 0.1;

	int block_reached = 0;
	while (!block_reached && block_distance < MAX_DISTANCE) {
		block_distance += step_size;
		int x = player.x + ray_unit_vec.x * block_distance;
		int y = player.y + ray_unit_vec.y * block_distance;

		if (x < 0 || x > MAP_WIDTH
				|| y < 0 || y > MAP_HEIGHT
				|| map[y * MAP_WIDTH + x] == '#')
			block_reached = 1;
	}

	return block_distance;
}

void compute_block_edges(struct edge edges[4], struct coordinates block_coords,
	   	struct player player)
{
	int dx, dy;
	for (dx = 0; dx < 2; ++dx) {
		for (dy = 0; dy < 2; ++dy) {
			struct edge e;
			e.x = block_coords.x + dx;
			e.y = block_coords.y + dy;
			e.distance = sqrt((e.x - player.x) * (e.x - player.x)
					+ (e.y - player.y) * (e.y - player.y));
			edges[dx*2 + dy] = e;
		}
	}
}

/* Check if we hit the boundary of the given block.
 * For that we will look at the 2 closest edges of the block,
 * because we can allways see at least two edges of a block.
 * If the angle between the current ray (given by ray_unit_vec)
 * and the ray to one of the two edges is below a fixed threshold,
 * the ray is approximately pointing at an edge.
 */
int check_ray_hits_edge(struct player player, struct vec2d ray_unit_vec,
		struct coordinates block_coords)
{
	struct edge block_edges[4];
	compute_block_edges(block_edges, block_coords, player);

	/* Sort the edges, because we only want to look at the two closest. */
	qsort(block_edges, sizeof(block_edges) / sizeof(block_edges[0]),
			sizeof(block_edges[0]), compare_edges);
	int i;
	const double cos_threshold = 0.999985;
	for (i = 0; i < 2; ++i) {
		struct vec2d edge_unit_vec;
		edge_unit_vec.x = (block_edges[i].x - player.x) / block_edges[i].distance;
		edge_unit_vec.y = (block_edges[i].y - player.y) / block_edges[i].distance;

		/* The dot product of two unit vectors equals the cosine between them.
		 * Big cosine (close to 1) means small angle between the two vectors.
		 */
		double cosine = dot_product(ray_unit_vec, edge_unit_vec);
		if (cosine > cos_threshold)
			return 1;
	}

	return 0;
}

void compute_block_shade(char *block_shade, int size, double block_distance)
{	
	if (block_distance < MAX_DISTANCE / 4.)
		strncpy(block_shade, "\u2588", size);
	else if (block_distance < MAX_DISTANCE / 3.)
		strncpy(block_shade, "\u2593", size);
	else if (block_distance < MAX_DISTANCE / 2.)
		strncpy(block_shade, "\u2592", size);
	else if (block_distance < MAX_DISTANCE)
		strncpy(block_shade, "\u2591", size);
	else
		strncpy(block_shade, " ", size);
}

char compute_floor_shade(int row)
{
	/* Compute distance-ratio. */
	float half_screen_height = SCREEN_HEIGHT / 2.;
	float dr = 1. - (row - half_screen_height) / half_screen_height;

	/* Determine floor-shade. */
	char fshade;
	if (dr < 0.15)
		fshade = '#';
	else if (dr < 0.3)
		fshade = 'x';
	else if (dr < 0.6)
		fshade = '.';
	else if (dr < 0.9)
		fshade = '-';
	else
		fshade = ' ';
	return fshade;
}

void draw_3d_view(struct player player)
{
	int col;
	for (col = 0; col < SCREEN_WIDTH; ++col) {	/* for every ray */
		/* Compute ray unit vector. */
		double ray_angle = (player.alpha + FOV / 2.0)
			- (col * FOV / SCREEN_WIDTH);
		struct vec2d ray_unit_vec;
		ray_unit_vec.x = sin(ray_angle);
		ray_unit_vec.y = cos(ray_angle);

		/* Compute distance and coordinates of the block that is hit by this ray. */
		double block_distance =
			compute_block_distance(player, ray_unit_vec);
		struct coordinates block_coords;
		block_coords.x = (int) (player.x + ray_unit_vec.x * block_distance);
		block_coords.y = (int) (player.y + ray_unit_vec.y * block_distance);

		int ray_hits_edge =
			check_ray_hits_edge(player, ray_unit_vec, block_coords);

		/* Calculate beginning of ceiling and floor. */
		int ceiling = SCREEN_HEIGHT / 2.0
			- SCREEN_HEIGHT / block_distance;
		int floor = SCREEN_HEIGHT - ceiling;

		/* Determine block-shade. */
		char block_shade[10];
		compute_block_shade(block_shade, 10, block_distance);

		/* Draw the 3D view. */
		int row;
		for (row = 0; row < SCREEN_HEIGHT; ++row) {
			move(row, col);
			if (row < ceiling) {
				addch(' ');
			} else if (row >= ceiling && row <= floor) {	/* drawing the wall */
				if (ray_hits_edge)
					addch(' ');
				else 
					printw("%s", block_shade);
			} else {	/* Shade floor based on distance. */
				addch(compute_floor_shade(row));
			}
		}
	}
}

void draw_stats(struct player player, long elapsed_nsec)
{
		/* Draw the stats. */
		move(0, 0);
		printw("X:%3.2f, Y:%3.2f, A:%3.2f, FPS:%3.2f ",
				player.x, player.y, player.alpha,
				1000000000. / elapsed_nsec);
}

void draw_map(struct player player)
{
	/* Draw the map. */
	const int map_offset = 1;
	int row;
	for (row = 0; row < MAP_HEIGHT; ++row) {
		move(map_offset + row, 0);
		int col;
		for (col = 0; col < MAP_WIDTH; ++col)
			addch(map[row * MAP_WIDTH + col]);
	}
	/* Add the player. */
	move(map_offset + player.y, player.x);
	addch('P');
}

/********************************************************************************
 * main.
 ********************************************************************************/
int main()
{
	/* Initialization. */
	setlocale(LC_ALL, "");	/* XXX this has to come first */
	init_ncurses();

	struct player player;
	player.x = 5.;
	player.y = 5.;
	player.alpha = 0.;

	struct timespec oldtime, newtime;
	clock_gettime(CLOCK_REALTIME, &oldtime); 

	/* Game loop. */
	while (1) {
		/* Compute the passed time. */
		clock_gettime(CLOCK_REALTIME, &newtime); 
		long elapsed_nsec = (newtime.tv_sec - oldtime.tv_sec) * 1000000000
			+ (newtime.tv_nsec - oldtime.tv_nsec);
		oldtime = newtime;

		/* Move the player. */
		move_player(&player, elapsed_nsec);

		/* Draw. */
		draw_3d_view(player);
		draw_stats(player, elapsed_nsec);
		draw_map(player);
		refresh();
	}

	/* Clean up and exit. */
	endwin();
	return 0;
}
