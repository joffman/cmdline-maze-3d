#include <math.h>
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
	/* Init ncurses. */
	initscr();
	cbreak();

	/* Init player pose. */
	float player_xpos = 5.;
	float player_ypos = 5.;
	float player_alpha = 0.;

	/* Initialize the map. */
	char map[] =
		"################"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"#..............#"
		"################";


	/* Game loop. */
	while (1) {
		int col;
		for (col = 0; col < SCREEN_WIDTH; ++col) {
			/* Compute geometry of ray. */
			float ray_angle = (player_alpha + FOV / 2.0)
				- (col / SCREEN_WIDTH * FOV);
			float eye_x = sinf(ray_angle);
			float eye_y = cosf(ray_angle);

			/* Distance to wall for that angle. */
			float distance_to_wall = 0.;
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

			/* Draw. */
			int row;
			for (row = 0; row < SCREEN_HEIGHT; ++row) {
				move(row, col);
				if (row < ceiling)
					addch(' ');
				else if (row > floor)
					addch(' ');
				else
					addch('#');
			}
		}
		refresh();
	}

	/* Clean up and exit. */
	endwin();
	return 0;
}
