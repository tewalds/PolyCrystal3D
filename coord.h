
#ifndef _COORD_H_
#define _COORD_H_

int periodic_dist_sq(int x1, int y1, int x2, int y2){
	int dx = abs(x1 - x2);
	int dy = abs(y1 - y2);
	
	if(dx > FIELD/2)
		dx = abs(dx - FIELD);
	if(dy > FIELD/2)
		dy = abs(dy - FIELD);

	return dx*dx + dy*dy;
}

double periodic_dist_sq(double x1, double y1, double x2, double y2){
	double dx = fabs(x1 - x2);
	double dy = fabs(y1 - y2);
	
	if(dx > FIELD/2)
		dx = abs(dx - FIELD);
	if(dy > FIELD/2)
		dy = abs(dy - FIELD);

	return dx*dx + dy*dy;
}

struct Coord2i {
	int x, y;
	
	Coord2i(){
		x = y = 0;
	}
	Coord2i(int X, int Y){
		x = X;
		y = Y;
	}
};

struct Coord2f {
	double x, y;
	
	Coord2f(){
		x = y = 0;
	}
	Coord2f(double X, double Y){
		x = X;
		y = Y;
	}
};

struct Coord3i {
	int x, y, z;
	
	Coord3i(){
		x = y = z = 0;
	}
	Coord3i(int X, int Y, int Z){
		x = X;
		y = Y;
		z = Z;
	}
};

struct Coord3f {
	double 	x, y, z;
	
	Coord3f(){
		x = y = z = 0;
	}
	Coord3f(double X, double Y, double Z){
		x = X;
		y = Y;
		z = Z;
	}
};

#endif

