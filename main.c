#define _POSIX_C_SOURCE 199309L	/* timespec, clock_gettime */

#include <math.h>
#include <time.h>
#include <locale.h>
#include <stdlib.h>	/* qsort */
#include <ncurses.h>

#define SCREEN_WIDTH 132
#define SCREEN_HEIGHT 43

#define MAP_WIDTH 16
#define MAP_HEIGHT 16
#define MAX_DISTANCE 23

#define PI 3.14159265359

#define FOV (PI / 4)


struct edge {
	int x;
	int y;
	double distance;	/* distance to player */
};

int compare_edges(const void *p1, const void *p2)
{
	const struct edge *e1 = p1;
	const struct edge *e2 = p2;
	return e1->distance > e2->distance;	/* edges with smaller distance should have smaller index */
}

int main()
{
	setlocale(LC_ALL, "");

	/* Init ncurses. */
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, 1);
	notimeout(stdscr, 1);

	/* Init player pose. */
	double player_xpos = 5.;
	double player_ypos = 5.;
	double player_alpha = 0.;

	/* Initialize the map. */
	char map[] =
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


	/* Get ready for loop. */
	struct timespec oldtime, newtime;
	clock_gettime(CLOCK_REALTIME, &oldtime); 

	/* Game loop. */
	while (1) {
		/* Compute the passed time. */
		clock_gettime(CLOCK_REALTIME, &newtime); 
		long elapsed_nsec = (newtime.tv_sec - oldtime.tv_sec) * 1000000000
			+ (newtime.tv_nsec - oldtime.tv_nsec);
		oldtime = newtime;

		/* Handle controls. */
		double player_eye_x = sin(player_alpha);
		double player_eye_y = cos(player_alpha);
		int cmd = getch();
		float old_px = player_xpos;
		float old_py = player_ypos;
		switch (cmd) {
			case 'w':	/* forward */
				player_xpos += elapsed_nsec * 0.0000001 * player_eye_x;
				player_ypos += elapsed_nsec * 0.0000001 * player_eye_y;
				break;
			case 's':	/* backward */
				player_xpos -= elapsed_nsec * 0.0000001 * player_eye_x;
				player_ypos -= elapsed_nsec * 0.0000001 * player_eye_y;
				break;
			case 'a':	/* left */
				player_alpha += elapsed_nsec * 0.0000001;
				break;
			case 'd':	/* right */
				player_alpha -= elapsed_nsec * 0.0000001;
				break;
		}
		/* Collision detection. */
		if (map[(int) player_ypos * MAP_WIDTH + (int) player_xpos] == '#') {
			player_xpos = old_px;
			player_ypos = old_py;
		}

		/* Handle drawing. */
		int col;
		for (col = 0; col < SCREEN_WIDTH; ++col) {
			/* Compute geometry of ray. */
			double ray_angle = (player_alpha + FOV / 2.0)
				- (col * FOV / SCREEN_WIDTH);
			double eye_x = sin(ray_angle);
			double eye_y = cos(ray_angle);

			/* Distance to wall for that angle. */
			double distance_to_wall = 0.;
			const double step_size = 0.1;
			int wall_reached = 0;
			int is_boundary = 0;
			while (!wall_reached && distance_to_wall < MAX_DISTANCE) {
				distance_to_wall += step_size;
				int test_x = (int) (player_xpos + eye_x * distance_to_wall);
				int test_y = (int) (player_ypos + eye_y * distance_to_wall);

				if (test_x < 0 || test_x > MAP_WIDTH
						|| test_y < 0 || test_y > MAP_HEIGHT) {
					wall_reached = 1;
				} else if (map[test_y * MAP_WIDTH + test_x] == '#') {
					wall_reached = 1;

					/* Check if we hit the boundary of the wall.
					 * For that we will look at the 2 closest edges of the block,
					 * because we can allways see at least two edges of a block.
					 * If the angle between the current ray (given by eye_x and eye_y)
					 * and the ray to one of the two edges is below a fixed threshold,
					 * the current is approximately pointing at an edge.
					 */
					int dx, dy;
					struct edge block_edges[4];
					for (dx = 0; dx < 2; ++dx) {
						for (dy = 0; dy < 2; ++dy) {
							/* Add each edge to block_edges. */
							struct edge e;
							e.x = test_x + dx;
							e.y = test_y + dy;
							e.distance = sqrt((e.x - player_xpos) * (e.x - player_xpos)
								+ (e.y - player_ypos) * (e.y - player_ypos));
							block_edges[dx*2 + dy] = e;
						}
					}

					/* Check if the current ray hits them hits one of the two closest edges. */
					qsort(block_edges, sizeof(block_edges) / sizeof(block_edges[0]),
							sizeof(block_edges[0]), compare_edges);
					int i;
					const double cos_threshold = 0.999985;
					for (i = 0; i < 2; ++i) {	/* look at the two closest edges */
						float vx = (block_edges[i].x - player_xpos) / block_edges[i].distance;
						float vy = (block_edges[i].y - player_ypos) / block_edges[i].distance;
						/* The dot product of two unit vectors equals the cosine between them.
						 * Big cosine (close to 1) means small angle between the two vectors.
						 */
						double cosine = vx * eye_x + vy * eye_y;
						if (cosine > cos_threshold) {
							is_boundary = 1;
							break;
						}
					}
				}
			}

			/* Calculate beginning of ceiling and floor. */
			int ceiling = SCREEN_HEIGHT / 2.0
				- SCREEN_HEIGHT / distance_to_wall;
			int floor = SCREEN_HEIGHT - ceiling;

			/* Determine shade. */
			char *shade;
			if (distance_to_wall < MAX_DISTANCE / 4.)
				shade = "\u2588";
			else if (distance_to_wall < MAX_DISTANCE / 3.)
				shade = "\u2593";
			else if (distance_to_wall < MAX_DISTANCE / 2.)
				shade = "\u2592";
			else if (distance_to_wall < MAX_DISTANCE)
				shade = "\u2591";
			else
				shade = " ";

			/* Draw the 3D view. */
			int row;
			for (row = 0; row < SCREEN_HEIGHT; ++row) {
				move(row, col);
				if (row < ceiling) {
					addch(' ');
				} else if (row >= ceiling && row <= floor) {	/* drawing the wall */
					if (is_boundary)
						addch(' ');
					else 
						printw("%s", shade);
				} else {	/* Shade floor based on distance. */
					float half_screen_height = SCREEN_HEIGHT / 2.;
					float dr = 1. - (row - half_screen_height) / half_screen_height; /* distance-ratio */

					char fshade;	/* floor shade */
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

					addch(fshade);
				}
			}
		}

		/* Draw the stats. */
		move(0, 0);
		printw("X:%3.2f, Y:%3.2f, A:%3.2f, FPS:%3.2f ", player_xpos, player_ypos, player_alpha, 1000000000. / elapsed_nsec);

		/* Draw the map. */
		const int map_offset = 1;
		int i;
		for (i = 0; i < MAP_HEIGHT; ++i) {
			move(map_offset + i, 0);
			int j;
			for (j = 0; j < MAP_WIDTH; ++j)
				addch(map[i * MAP_WIDTH + j]);
		}
		/* Add the player. */
		move(map_offset + player_ypos, player_xpos);
		addch('P');

		refresh();
	}

	/* Clean up and exit. */
	endwin();
	return 0;
}
