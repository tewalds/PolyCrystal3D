
#ifndef _RAY_H_
#define _RAY_H_

struct Ray {
	Coord3f loc, dir;

	Ray(){ }
	Ray(Coord3f l, Coord3f d){
		loc = l;
		dir = d;
	}

	void scale(){
		dir.scale();
	}

	void rotx(double t){
		dir.rotx(t);
	}

	void roty(double t){
		dir.roty(t);
	}

	void rotz(double t){
		rotz(t);
	}

	void incr(){
		loc += dir;
	}

	void decr(){
		loc -= dir;
	}

	bool incr(int zmin){
		loc.z += dir.z;

		if(loc.z < zmin){
			loc.z -= dir.z;
			return false;
		}

		loc.x += dir.x;
		loc.y += dir.y;
		return true;
	}
};

#endif

