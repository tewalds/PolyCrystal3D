
#ifndef _RAY_H_
#define _RAY_H_

struct Ray {
	double x, y, z;
	double a, b, c;

	Coord3i loc(){
		return Coord3i(X(), Y(), Z());
	}

	void scale(){
		double factor = sqrt(a*a + b*b + c*c);
		a /= factor;
		b /= factor;
		c /= factor;
	}

	void rotx(double t){
		double B, C;
		B = cos(t)*b + sin(t)*c;
		C = -sin(t)*b + cos(t)*c;
		b = B; c = C;
	}

	void roty(double t){
		double A, C;
		A = cos(t)*a - sin(t)*c;
		C = sin(t)*a + cos(t)*c;
		a = A; c = C;
	}

	void rotz(double t){
		double A, B;
		A = cos(t)*a + sin(t)*b;
		B = -sin(t)*a + cos(t)*b;
		a = A; b = B;
	}

	int round(double v){
//		return lroundl(v);
		return v;
	}

	void incr(){
		x += a;
		y += b;
		z += c;
	}

	bool incr(int zmin){
		z += c;

		if(round(z) < zmin){
			z -= c;
			return false;
		}

		x += a;
		y += b;
		return true;
	}
	int X(){ return round(x); }
	int Y(){ return round(y); }
	int Z(){ return round(z); }
};

#endif

