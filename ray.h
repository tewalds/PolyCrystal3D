
#ifndef _RAY_H_
#define _RAY_H_

struct Ray {
	Coord3f loc, dir;

	Ray(){ }
	Ray(Coord3f l, Coord3f d){
		loc = l;
		dir = d;
	}

	void scale(double l = 1){
		dir.scale(l);
	}

	void rotx(double t){
		dir.rotx(t);
	}

	void roty(double t){
		dir.roty(t);
	}

	void rotz(double t){
		dir.rotz(t);
	}

	void incr(){
		loc += dir;
	}

	void decr(){
		loc -= dir;
	}

	bool incr(int zmin){
		loc += dir;

		if(loc.z < zmin){
			loc -= dir;
			return false;
		}

		return true;
	}
};

#endif

