
#include "tqueue.h"

double unitrand(){
	return (double)rand()/RAND_MAX;
}

int time_msec(){
	struct timeval time;
	gettimeofday(&time, NULL);
	return (time.tv_sec*1000 + time.tv_usec/1000);
}

enum ReqType {
	RT_COUNT_THREATS,
	RT_ADDFLUX,
	RT_THREAT_POINTS
};

struct Request {
	ReqType type;
	int a, b, c;

	Request(ReqType T, int A = 0, int B = 0, int C = 0){
		type = T;
		a = A;
		b = B;
		c = C;
	}
};

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


class Growth {
public:

	int num_steps;
	double growth_factor;
	int max_memory;
	int end_grains;

	int ray_step;
	double ray_ratio;
	double ray_angle;
	double start_angle;

	double diffusion_probability;

	vector<Grain> grains;
	Grid grid;

	tqueue<Request> request;
	tqueue<int> response;
	int num_threads;
	pthread_t thread[MAX_THREADS];

	Growth(int threads){
		max_memory = 0;
		num_threads = threads;

		num_steps = 200;
		growth_factor = 1;
		end_grains = 20;

		start_angle = 0;
		ray_step = 10;
		ray_ratio = 1.0;
		ray_angle = 45;		
	
		diffusion_probability = 0.95;
	
		//define a blank grain
		grains.push_back(Grain());

		//start the threads
		if(num_threads > 1)
			for(int i = 0; i < num_threads; i++)
				pthread_create(&(thread[i]), NULL, (void* (*)(void*)) threadRunner, this);
	}

	static void * threadRunner(void * blah){
		Growth * g = (Growth *) blah;
        while(1){
                Request *req = g->request.pop();
                
                int ret = 0;

                switch(req->type){
                	case RT_COUNT_THREATS:
                		g->count_threats(req->a);
                		break;
					case RT_ADDFLUX:
						g->addflux(req->a);
						break;
					case RT_THREAT_POINTS:
						ret = g->run_layer(req->a, req->b, req->c);
						break;
				}
				g->response.push((int *)ret);

				delete req;
        }
        return NULL;
	}

	void init(int num_grains, double min_space, Shape & shape, bool load){
		int starttime = time_msec();

		echo("Initializing %d grains of shape %s, to be run %d times in %.2f increments ... ", num_grains, shape.name, num_steps, growth_factor);
		fflush(stdout);

		FILE *fd = NULL;

		if(opts.stats){
			fd = fopen("levelstats", "w");
			fprintf(fd, "level,num grains,mean area,porousity\n");
			fclose(fd);

			fd = fopen("timestats", "w");
			fprintf(fd, "time,num grains,mean height,rms roughness\n");
			fclose(fd);
		}

		grid.output(0, grains);
		grid.cleangrid(0, grains);
		
		double min_space_squared = min_space*min_space;

		if(load)
			fd = fopen("graininit", "r");

	//generate initial grain information
	//grain number 0 is special as non-existent
		for(int i = 1; i <= num_grains; i++){
			Grain g;
			
			if(load){
				g.load(fd, shape.faces, shape.num_faces);
			}else{
				double dist = FIELD;
	
				do{
					g.x = rand() % FIELD;
					g.y = rand() % FIELD;

					for(int j = 1; j < i; j++){
						dist = periodic_dist_sq(g.x, g.y, grains[j].x, grains[j].y);
						if(dist < min_space_squared)
							break;
					}
				}while(dist < min_space_squared);

				g.add_faces(shape.faces, shape.num_faces);

				g.rotate(2.0*M_PI*unitrand(), 2.0*M_PI*unitrand(), acos(pow(unitrand(), 1.0/(1.0 + start_angle))));
			}

			g.color = (double)i/num_grains;

			grid.set_point(g.x, g.y, 0, 1, i, 0);
			grains.push_back(g);
		}

		if(load)
			fclose(fd);

		if(!load && opts.graininit){
			fd = fopen("graininit", "w");
			for(int i = 1; i <= num_grains; i++)
				grains[i].dump(fd, i);
			fclose(fd);
		}

		if(opts.voronei){
			echo("Finding closest grains ... ");
			fflush(stdout);
			grid.voroneimap(grains);
		}

		grid.output(1, grains);
		grid.cleangrid(1, grains);

		echo("done in %d msec\n", time_msec() - starttime);
	}

	void run(){
		int start = time_msec();
		int starttime;
		int remain = grains.size() - 1;

		for(int t = 2; t <= num_steps && !opts.interrupt; t++){
			echo("Step %d, layers %d-%d, %d grains, %d Mb ... ", t, grid.zmin, grid.zmax, remain, grid.memory_usage()/(1024*1024));
			fflush(stdout);

			starttime = time_msec();

		//reset grain and face stats
			for(unsigned int i = 0; i < grains.size(); i++){
				grains[i].growth = 0;
				grains[i].threats = 0;

				for(unsigned int j = 0; j < grains[i].faces.size(); j++){
					Face * face = &(grains[i].faces[j]);
					face->growth = 0;
					face->threats = 0;
					face->flux = 0;
				}
			}

			if(opts.fluxmap)
				grid.resetflux();

			int growth = 0;
			int raycount = (grid.zmin == 0 ? grid.planes[0]->taken : FIELD*FIELD);
			raycount *= ray_ratio;

			//grow the grains
			if(ray_step == 0 || t <= ray_step){
//			if(grid.zmin == 0){
//			if(true){
				for(unsigned int i = 0; i < grains.size(); i++)
					grains[i].grow_faces(growth_factor);
			}else{
				if(num_threads > 1){
				//count threats
					for(int z = grid.zmin; z < grid.zmax; z++)
						request.push(new Request(RT_COUNT_THREATS, z));

					for(int z = grid.zmin; z < grid.zmax; z++)
						response.pop();

				//ray trace
					for(int i = 0; i < num_threads*10; i++)
						request.push(new Request(RT_ADDFLUX, raycount/(num_threads*10)));

					for(int i = 0; i < num_threads*10; i++)
						response.pop();
				}else{
					for(int z = grid.zmin; z < grid.zmax; z++)
						count_threats(z);

					addflux(raycount);
				}

			//add flux
				for(unsigned int i = 0; i < grains.size(); i++){
					for(unsigned int j = 0; j < grains[i].faces.size(); j++){
						Face * face = &(grains[i].faces[j]);
						double amnt = face->fluxamnt();
						face->grow(growth_factor * amnt / ray_ratio);
					}
				}
			}

			echo("added flux in %d msec ... ", time_msec() - starttime);
			fflush(stdout);


			starttime = time_msec();

			//fill in the controller of the point if there is a new one
			int thisgrowth;
			int count = 0;
			bool mem = true;
			do{
				thisgrowth = 0;
				if(num_threads > 1){
					for(int z = grid.zmin; z < grid.zmax; z++)
						request.push(new Request(RT_THREAT_POINTS, z, t, count));

					for(int z = grid.zmin; z < grid.zmax; z++)
						thisgrowth += (uint64_t)response.pop();
				}else{
					for(int z = grid.zmin; z < grid.zmax; z++)
						thisgrowth += run_layer(z, t, count);
				}
				mem = grid.growgrid();

				growth += thisgrowth;
				count++;
			}while(mem && count < 5 && thisgrowth && thisgrowth > growth/1000.0);


			echo("grew %d points in %d runs in %d msec ... ", growth, count, time_msec() - starttime);
			fflush(stdout);


			starttime = time_msec();

			//output and finished data and images
			grid.cleangrid(t, grains);
			grid.output(t, grains);

			echo("output in %d msec\n", time_msec() - starttime);

			if(!mem){
				echo("Couldn't allocate more memory, current usage ~ %d Mb\n", grid.memory_usage()/(1024*1024));
				break;
			}

			remain = grid.graincount(grains.size());
			if(remain <= end_grains){
				echo("Hit the grain limit: %d grains left\n", remain);
				break;
			}
			
			if(max_memory && grid.memory_usage()/(1024*1024) > max_memory){
				echo("Hit the memory limit: %d Mb\n", max_memory);
				break;
			}
		}

		grid.dump(grains);
		echo("Finished in %d sec\n", (time_msec() - start)/1000);
	}

	void count_threats(int z){
		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){

			//point isn't threatened
				if(grid.get_grain(x,y,z) != THREAT)
					continue;

				Threat threats[27];
				Threat * threats_end = grid.check_face_threats(threats, x, y, z);

				//add to only one of the grains+faces, choosing which randomly
				if(threats != threats_end){ //needed in case the bottom drops off for being inactive
					Threat * i = threats + (rand() % (threats_end - threats));
					INCR(grains[i->grain].threats);
					INCR(grains[i->grain].faces[i->face].threats);
					grid.set_diffprob(x, y, z, (uint8_t)(diffusion_probability * grains[i->grain].faces[i->face].P * 255));
				}
			}
		}
	}

	int run_layer(int z, int t, bool onlynewthreats){
		int growth = 0;

		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){

			//point isn't threatened
				Point * p = grid.get_point(x, y, z);

				if(p->grain == FULLPOINT) //skip this whole sector if it's full
					break;

				if(p->grain != THREAT || (onlynewthreats && p->time != t))
					continue;

				uint16_t threats[27];
				uint16_t * threats_end = grid.check_grain_threats(threats, x, y, z);

			//check how many took this point this time step, moving valid ones down and ignoring ones that are only a threat
				for(uint16_t * a = threats; a != threats_end; ){
					if(grains[*a].check_point(x, y, z)){
						a++;
					}else{
						threats_end--;
						*a = *threats_end;
					}
				}

			//no threats actually took this point
				if(threats == threats_end)
					continue;

			//figure out which grain and face got it first
				uint16_t best = threats[0];
				FaceDist min_dist = grains[best].find_distance(x, y, z);

				for(uint16_t * a = threats + 1; a != threats_end; a++){
					FaceDist temp = grains[*a].find_distance(x, y, z);
					if(temp.dist < min_dist.dist){
						min_dist = temp;
						best = *a;
					}
				}

				//save this point
				grid.set_point(x, y, z, t, best, min_dist.face);
				INCR(grains[best].growth);
				INCR(grains[best].size);
				INCR(grains[best].faces[min_dist.face].growth);
				growth++;
			}
		}

		return growth;
	}

	void addflux(int num){
		double costheta, sintheta, phi;
		
		double cutoffcos = cos(85.0 * M_PI/180); //cutoff angle of 85 degrees
		double raypow = 1.0/(1.0 + ray_angle);

		for(int i = 0; i < num; i++){
			Ray ray;
			ray.x = unitrand() * FIELD;
			ray.y = unitrand() * FIELD;
			ray.z = grid.zmax-1;

			do{
				costheta = pow(unitrand(), raypow);
			}while(costheta < cutoffcos);
			sintheta = sqrt(1 - costheta*costheta);
			phi = unitrand()*2*M_PI;

			ray.a = cos(phi)*sintheta;
			ray.b = sin(phi)*sintheta;
			ray.c = -costheta;

			Coord c = raytrace(ray); //trace the ray until it hits a threat or the substrate
			
			int grain = grid.get_grain(c.x, c.y, c.z);

			if(opts.fluxmap)
				grid.incrflux(c.x, c.y);
			
			if(grain == THREAT){
				if(diffusion_probability > 0)
					c = face_random_walk(c.x, c.y, c.z); //walk along the threats for random length
			}else if(grain == 0 && c.z == 0){ // hit the substrate
				c = substrate_random_walk(c.x, c.y); //walk till it hits a threat
			}else{
				continue; //hit nothing, likely down a deep crevase to points that were already dropped
			}
			
			//add to only one of the grains+faces, choosing which randomly
			Threat threats[27];
			Threat * threats_end = grid.check_face_threats(threats, c.x, c.y, c.z);
			Threat * i = threats + (rand() % (threats_end - threats));
			INCR(grains[i->grain].faces[i->face].flux);
		}
	}

	Coord raytrace(Ray ray){
		while(ray.incr(grid.zmin) && grid.get_grain(ray.X(), ray.Y(), ray.Z()) != THREAT); //empty body

		return Coord(ray.X(), ray.Y(), ray.Z());
	}

	Coord substrate_random_walk(int x, int y){	
		do{
			switch(rand() % 4){
				case 0: x++; break;
				case 1: x--; break;
				case 2: y++; break;
				case 3: y--; break;
			}
		}while(grid.get_grain(x, y, 0) != THREAT);

		return Coord(x, y, 0);
	}

	Coord face_random_walk(int x, int y, int z){
		Point * p = grid.get_point(x, y, z);

		while((rand() % 256) < p->diffprob){
retry: //used to retry on when the random choice below is invalid, without re-checking the probability
			int dir = rand() % 6;
			switch(dir){
				case 0: x++; break;
				case 1: x--; break;
				case 2: y++; break;
				case 3: y--; break;
				case 4: if(z == grid.zmax - 1){ goto retry; } else { z++; break; }
				case 5: if(z == grid.zmin    ){ goto retry; } else { z--; break; }
			}

			p = grid.get_point(x, y, z);

			//undo invalid moves			
			if(p->grain != THREAT){
				switch(dir){
					case 0: x--; break;
					case 1: x++; break;
					case 2: y--; break;
					case 3: y++; break;
					case 4: z--; break;
					case 5: z++; break;
				}

				goto retry;
			}
		}

		return Coord(x, y, z);
	}

/*
//face walk that tries to be faster and more fair than the random walk
	Coord face_walk(int x, int y, int z){
		//choose length to travel
		double length = 1.0; //fix to be the correct random distribution later

		//choose direction to travel
		Ray ray;
		ray.x = x;
		ray.y = y;
		ray.z = z;
		
		double theta = unitrand()*2*M_PI;
		double k = unitrand()*2.0 - 1;
		
		double i = sqrt(1 - k*k)*cos(theta);
		double j = sqrt(1 - k*k)*sin(theta);
		
		ray.(a,b,c) = face.(a,b,c) x (i,j,k);
		ray.(a,b,c) = ray.(a,b,c) / (length of ray.(a,b,c))

		while(length > 0){
			if(!ray.incr(grid.zmin))
				break;
				
			Point * p = grid.get_point(x, y, z);

			//move one unit in the direction
			//substract face diffusion length factor from length
			//if no longer on this face
				//figure out which face it is moving to
				//find the spine
				//set the new direction
		}

		return Coord(x, y, z);
	}
*/
};


