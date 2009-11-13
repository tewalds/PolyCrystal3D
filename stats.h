
#ifndef _STATS_H_
#define _STATS_H_

#include <pthread.h>
#include "color.h"
#include "worker.h"
#include "coord.h"
#include "ray.h"

struct Stats {
	struct TimeStatsReq : WorkRequest {
		void (*func)(int, Grid *, const vector<Grain> &);
		int t;
		Grid * grid;
		const vector<Grain> & grains;

		TimeStatsReq(void (*_func)(int, Grid *, const vector<Grain> &), int _t, Grid * _grid, const vector<Grain> & _grains)
			: func(_func), t(_t), grid(_grid), grains(_grains) { }

		int64_t run(){
			func(t, grid, grains);
			return 0;
		}
	};

	struct RayReq : WorkRequest {
		Grid * grid;
		const vector<Grain> & grains;
		Ray ray;
		Coord3f light;
		int x, y;
		gdImagePtr im;
		pthread_mutex_t * lock;

		RayReq(Grid * _grid, const vector<Grain> & _grains, Ray r, Coord3f _light, int _x, int _y, gdImagePtr _im, pthread_mutex_t * _lock)
			: grid(_grid), grains(_grains), ray(r), light(_light), x(_x), y(_y), im(_im), lock(_lock) { }

		int64_t run(){
			RGB rgb = Stats::shootray(ray, light, grid, grains);

			pthread_mutex_lock(lock);

			int color = gdImageColorAllocate(im, rgb.r, rgb.g, rgb.b);
			gdImageSetPixel(im, x, y, color);

			pthread_mutex_unlock(lock);

			return 0;
		}
	};

	static void timestats(Worker * worker, int t, Grid * grid, const vector<Grain> & grains){
		if(opts.slopemap)
			worker->add(new TimeStatsReq(slopemap,   t, grid, grains));
		if(opts.heightmap)
			worker->add(new TimeStatsReq(heightmap,  t, grid, grains));
		if(opts.heightdump)
			worker->add(new TimeStatsReq(heightdump, t, grid, grains));
		if(opts.timemap)
			worker->add(new TimeStatsReq(timemap,    t, grid, grains));
		if(opts.fluxdump)
			worker->add(new TimeStatsReq(fluxdump,   t, grid, grains));
		if(opts.peaks)
			worker->add(new TimeStatsReq(peaks,      t, grid, grains));
		if(opts.growth)
			worker->add(new TimeStatsReq(growth,     t, grid, grains));
		if(opts.timestats)
			worker->add(new TimeStatsReq(timestats,  t, grid, grains));

		worker->wait();	

		if(opts.isomorphic)
			isomorphic(worker, t, grid, grains);
	}

	static void growth(int t, Grid * grid, const vector<Grain> & grains) {
		char filename[50];
		sprintf(filename, "growth.%05d.csv", t);
		FILE * fd = fopen(filename, "w");

		fprintf(fd, "grain,face,A,B,C,D,F,dF,flux,threats\n");

		for(unsigned int i = 0; i < grains.size(); i++){
			for(unsigned int j = 0; j < grains[i].faces.size(); j++){
				fprintf(fd, "%d,%d,", i, j);
				grains[i].faces[j].dump(fd);
			}
		}

		fclose(fd);
	}

	static void peaks(int t, Grid * grid, const vector<Grain> & grains) {
		vector<Coord3i> peaks(grains.size());

		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){
				int z = grid->heights[y][x];
				int grain = grid->get_grain(x, y, z);
				if(grain && grain < MAXGRAIN && peaks[grain].z < z){
					bool peak = true;
					for(int x1 = -1; x1 <= 1; x1++){
						for(int y1 = -1; y1 <= 1; y1++){
							if(x1 == x && y1 == y)
								continue;
							int g = grid->get_grain(x - x1, y - y1, z);
							if(g != 0 && g != grain && g < MAXGRAIN)
								peak = false;
						}
					}
					if(peak)
						peaks[grain] = Coord3i(x, y, z);
				}
			}
		}

		char filename[50];
		sprintf(filename, "peaks.%05d.csv", t);
		FILE * fd = fopen(filename, "w");
		for(unsigned int i = 0; i < grains.size(); i++){
			if(peaks[i].z)
				fprintf(fd, "%d,%d,%d,%d\n", i, peaks[i].x, peaks[i].y, peaks[i].z);
		}
		fclose(fd);
	}


	static void timestats(int t, Grid * grid, const vector<Grain> & grains) {
/*
- number of surface grains (count of grains in height map)
- mean height (average max z)
- rms roughness = sqrt(sum((h - avgH)^2)/FIELD^2)
*/

		int num = grid->graincount(grains.size());
		double mean = grid->mean_height();

		double totalms = 0;
		for(int y = 0; y < FIELD; y++)
			for(int x = 0; x < FIELD; x++)
				totalms += (grid->heights[y][x] - mean)*(grid->heights[y][x] - mean);

		double rms = sqrt(totalms/(FIELD*FIELD));

		FILE * fd = fopen("timestats.csv", "a");
		fprintf(fd, "%d,%d,%f,%f\n", t, num, mean, rms);
		fclose(fd);
	}


	static void fluxdump(int t, Grid * grid, const vector<Grain> & grains) {
		char filename[50];
		sprintf(filename, "flux.%05d.dat", t);
		FILE * fd = fopen(filename, "wb");

		for(int y = 0; y < FIELD; y++)
			if(fwrite(grid->flux[y], sizeof(uint8_t), FIELD, fd));

		fclose(fd);
	}

	static void heightdump(int t, Grid * grid, const vector<Grain> & grains) {
		char filename[50];
		sprintf(filename, "height.%05d.dat", t);
		FILE * fd = fopen(filename, "wb");

		for(int y = 0; y < FIELD; y++)
			if(fwrite(grid->heights[y], sizeof(uint16_t), FIELD, fd));

		fclose(fd);
	}

	static void heightmap(int t, Grid * grid, const vector<Grain> & grains) {
	//generate a png height map
		double diffheight = grid->zmax - grid->zmin;

		gdImagePtr im = gdImageCreateTrueColor(FIELD, FIELD);
		gdImageFill(im, 0, 0, gdImageColorAllocate(im, 0, 0, 0));

		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){
				if(grid->heights[y][x]){
					int grain = grid->get_grain(x, y, grid->heights[y][x]);
					RGB rgb = RGB(HSV(grains[grain].color, 1.0 - ((double)(grid->heights[y][x] - grid->zmin)/diffheight), 1.0));
					int color = gdImageColorAllocate(im, rgb.r, rgb.g, rgb.b);
					gdImageSetPixel(im, x, y, color);
				}
			}
		}

		char filename[50];
		sprintf(filename, "height.%05d.png", t);
		FILE * fd = fopen(filename, "wb");
		gdImagePng(im, fd);
		fclose(fd);
		gdImageDestroy(im);
	}

	static void slopemap(int t, Grid * grid, const vector<Grain> & grains) {
	//generate a png slope map
		gdImagePtr im = gdImageCreateTrueColor(FIELD, FIELD);
		gdImageFill(im, 0, 0, gdImageColorAllocate(im, 0, 0, 0));

		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){
				if(grid->heights[y][x]){
					Point * p = grid->get_point(x, y, grid->heights[y][x]);
					RGB rgb = grains[p->grain].get_face_color(p->face);
					int color = gdImageColorAllocate(im, rgb.r, rgb.g, rgb.b);
					gdImageSetPixel(im, x, y, color);
				}
			}
		}

		char filename[50];
		sprintf(filename, "slope.%05d.png", t);
		FILE * fd = fopen(filename, "wb");
		gdImagePng(im, fd);
		fclose(fd);
		gdImageDestroy(im);
	}

	static void timemap(int t, Grid * grid, const vector<Grain> & grains) {
	//generate a png time map, ie non-shaded heightmap
		gdImagePtr im = gdImageCreateTrueColor(FIELD, FIELD);
		gdImageFill(im, 0, 0, gdImageColorAllocate(im, 0, 0, 0));

		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){
				if(grid->heights[y][x]){
					int grain = grid->get_grain(x, y, grid->heights[y][x]);
					RGB rgb = RGB(HSV(grains[grain].color, 1.0, 1.0));
					int color = gdImageColorAllocate(im, rgb.r, rgb.g, rgb.b);
					gdImageSetPixel(im, x, y, color);
				}
			}
		}

		char filename[50];
		sprintf(filename, "time.%05d.png", t);
		FILE * fd = fopen(filename, "wb");
		gdImagePng(im, fd);
		fclose(fd);
		gdImageDestroy(im);
	}

	static void voroneimap(vector<Grain> & grains) {
	//generate a png voronei map
		gdImagePtr im = gdImageCreateTrueColor(FIELD, FIELD);
		gdImageFill(im, 0, 0, gdImageColorAllocate(im, 0, 0, 0));

		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){
				int mindist = FIELD*FIELD;
				int grain = 0;

				for(unsigned int i = 1; i < grains.size(); i++){
					Grain * g = &(grains[i]);
					int dist = periodic_dist_sq(g->x, g->y, x, y);

					if(dist < mindist){
						mindist = dist;
						grain = i;
					}
				}

				RGB rgb;
				if(x != grains[grain].x || y != grains[grain].y)
					rgb = RGB(HSV(grains[grain].color, 1.0, 1.0));
				int color = gdImageColorAllocate(im, rgb.r, rgb.g, rgb.b);
				gdImageSetPixel(im, x, y, color);
			}
		}

		FILE * fd = fopen("voronei.png", "wb");
		gdImagePng(im, fd);
		fclose(fd);
		gdImageDestroy(im);
	}

	static void isomorphic(Worker * worker, int t, Grid * grid, const vector<Grain> & grains) {
		const double scale = 1.0;
		const int width = scale*FIELD*1.5;
		const int height= scale*FIELD;
		int dist = FIELD*0.7;

		Ray init;
		init.dir = Coord3f(1, 1, -1).scale();
		init.loc = Coord3f(FIELD/2, FIELD/2, grid->mean_height()) - init.dir * dist;

		Coord3f light = Coord3f(1, -1, -1).scale();


		gdImagePtr im = gdImageCreateTrueColor(width, height);
		gdImageFill(im, 0, 0, gdImageColorAllocate(im, 0, 0, 0));

		Coord3f shiftx = Coord3f(0, 0, 1).cross(init.dir);
		Coord3f shifty = shiftx.cross(init.dir);

		shiftx.scale(1/scale);
		shifty.scale(1/scale);

		pthread_mutex_t lock;
		pthread_mutex_init(&lock, NULL);

		for(int y = 0; y < height; y++){
			for(int x = 0; x < width; x++){
				Ray ray = init;

				ray.loc += shiftx * (x - width/2) + shifty * (y - height/2);

//				shootray(ray, light, grid, grains);
				worker->add(new RayReq(grid, grains, ray, light, x, y, im, &lock));
			}
		}
		worker->wait();

		char filename[50];
		sprintf(filename, "isomorphic.%05d.png", t);
		FILE * fd = fopen(filename, "wb");
		gdImagePng(im, fd);
		fclose(fd);
		gdImageDestroy(im);
	}

	static RGB shootray(Ray ray, Coord3f light, Grid * grid, const vector<Grain> & grains){
		while(1){
			ray.incr();
			Coord3i c = ray.loc;

			if(c.x >= FIELD || c.y >= FIELD || c.z < grid->zmin) //outside the boundaries
				return RGB();

			if(c.x >= 0 && c.y >= 0 && c.z < grid->zmax){ //inside
				Point * p = grid->get_point(c.x, c.y, c.z);

				if(p->grain != 0 && p->grain < MAXGRAIN){ //on the surface
					double hue, dot;
					Coord3f vec;

					if(c.x == 0)      vec = Coord3f(-1, 0, 0);
					else if(c.y == 0) vec = Coord3f(0, -1, 0);
					else              vec = grains[p->grain].faces[p->face].vec;

					dot = light.dot(vec);
					hue = grains[p->grain].color;

					if(dot < 0) return RGB(HSV(hue, 1.0 + dot, 1.0));
					else        return RGB(HSV(hue, 1.0, 1.0 - dot));
				}
			}
		}
	}

};

#endif

