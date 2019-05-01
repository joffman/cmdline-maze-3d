#ifndef EDGE_H
#define EDGE_H

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

#endif
