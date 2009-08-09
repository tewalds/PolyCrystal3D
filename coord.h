
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

struct Coord3f;

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
	Coord3i(Coord3f c);
	Coord3i & operator=(const Coord3f & rhs);
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
	Coord3f(Coord3i c){
		x = c.x;
		y = c.y;
		z = c.z;
	}

	Coord3f & operator=(const Coord3i & rhs) {
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *this;
	}

	Coord3f & operator+=(const Coord3f & rhs) {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	Coord3f operator+(const Coord3f & rhs) const {
		Coord3f tmp = *this;
		tmp += rhs;
		return tmp;
	}

	Coord3f & operator+=(const double rhs) {
		x += rhs;
		y += rhs;
		z += rhs;
		return *this;
	}

	Coord3f operator+(const double rhs) const {
		Coord3f tmp = *this;
		tmp += rhs;
		return tmp;
	}

	Coord3f & operator-=(const Coord3f & rhs) {
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}

	Coord3f operator-(const Coord3f & rhs) const {
		Coord3f tmp = *this;
		tmp -= rhs;
		return tmp;
	}

	Coord3f & operator-=(const double rhs) {
		x -= rhs;
		y -= rhs;
		z -= rhs;
		return *this;
	}

	Coord3f operator-(const double rhs) const {
		Coord3f tmp = *this;
		tmp -= rhs;
		return tmp;
	}

	Coord3f & operator*=(double rhs) {
		x *= rhs;
		y *= rhs;
		z *= rhs;
		return *this;
	}

	Coord3f operator*(double rhs) const {
		Coord3f tmp = *this;
		tmp *= rhs;
		return tmp;
	}

	Coord3f & operator/=(double rhs) {
		x /= rhs;
		y /= rhs;
		z /= rhs;
		return *this;
	}

	Coord3f operator/(double rhs) const {
		Coord3f tmp = *this;
		tmp /= rhs;
		return tmp;
	}

	double len(){
		return sqrt(x*x + y*y + z*z);
	}

	void scale(double l = 1){
		double factor = len() * l;
		x /= factor;
		y /= factor;
		z /= factor;
	}

	double dot(Coord3f c){
		return x*c.x + y*c.y + z*c.z;
	}

	void rotx(double t){
		double Y, Z;
		Y = cos(t)*y + sin(t)*z;
		Z = -sin(t)*y + cos(t)*z;
		y = Y; z = Z;
	}

	void roty(double t){
		double X, Z;
		X = cos(t)*x - sin(t)*z;
		Z = sin(t)*x + cos(t)*z;
		z = X; z = Z;
	}

	void rotz(double t){
		double X, Y;
		X = cos(t)*x + sin(t)*y;
		Y = -sin(t)*x + cos(t)*y;
		x = X; y = Y;
	}
};

Coord3i::Coord3i(Coord3f c){
	x = c.x;
	y = c.y;
	z = c.z;
}
Coord3i & Coord3i::operator=(const Coord3f & rhs){
	x = rhs.x;
	y = rhs.y;
	z = rhs.z;
	return *this;
}


#endif

