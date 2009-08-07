
#ifndef _RAY_H_
#define _RAY_H_

struct Ray {
	double x, y, z;
	double a, b, c;

	int round(double v){
//		return lroundl(v);
		return v;
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

