#define _POSIX_C_SOURCE 199309L	/* timespec, clock_gettime */

#include <math.h>
#include <time.h>
#include <locale.h>
#include <ncurses.h>

#define SCREEN_WIDTH 132
#define SCREEN_HEIGHT 43

#define MAP_WIDTH 16
#define MAP_HEIGHT 16
#define MAX_DISTANCE 23

#define PI 3.14159265359

#define FOV (PI / 4)



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
		"#...##.........#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#......#########"
		"#..............#"
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
			int wall_reached = 0;
			while (!wall_reached && distance_to_wall < MAX_DISTANCE) {
				distance_to_wall += 0.1;
				int test_x = (int) (player_xpos + eye_x * distance_to_wall);
				int test_y = (int) (player_ypos + eye_y * distance_to_wall);

				if (test_x < 0 || test_x > MAP_WIDTH
						|| test_y < 0 || test_y > MAP_HEIGHT) {
					wall_reached = 1;
				} else if (map[test_y * MAP_WIDTH + test_x] == '#') {
					wall_reached = 1;
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

			/* Draw. */
			int row;
			for (row = 0; row < SCREEN_HEIGHT; ++row) {
				move(row, col);
				if (row < ceiling)
					addch(' ');
				else if (row >= ceiling && row <= floor)
					printw("%s", shade);
				else {	/* Shade floor based on distance. */
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
		refresh();
	}

	/* Clean up and exit. */
	endwin();
	return 0;
}
