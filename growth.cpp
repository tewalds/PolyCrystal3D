
#include "atomic.h"
#include "coord.h"
#include "ray.h"
#include "worker.h"

#include "stats.h"

double unitrand(){
	return (double)rand()/RAND_MAX;
}

int time_msec(){
	struct timeval time;
	gettimeofday(&time, NULL);
	return (time.tv_sec*1000 + time.tv_usec/1000);
}

class Growth {
	struct CountThreatsReq : WorkRequest {
		Growth * g;
		int z;
		CountThreatsReq(Growth * G, int Z) : g(G), z(Z) { }
		int64_t run(){
			g->count_threats(z);
			return 0;
		}
	};

	struct AddFluxReq : WorkRequest {
		Growth * g;
		int z;
		AddFluxReq(Growth * G, int Z) : g(G), z(Z) { }
		int64_t run(){
			g->addflux(z);
			return 0;
		}
	};

	struct RunLayerReq : WorkRequest {
		Growth * g;
		int z, t;
		bool n;
		RunLayerReq(Growth * G, int Z, int T, bool N) : g(G), z(Z), t(T), n(N) { }
		int64_t run(){
			return g->run_layer(z, t, n);
		}
	};


public:

	int num_steps;
	double growth_factor;
	int max_memory;
	int end_grains;

	int ray_step;
	double ray_ratio;
	double ray_angle;
	double ray_cutoff;
	double start_angle;

	bool substrate_diffusion;
	double diffusion_probability;

	vector<Grain> grains;
	Grid * grid;

	Worker * worker;
	int num_threads;

	Growth(int threads){
		max_memory = 0;
		num_threads = threads;

		num_steps = 200;
		growth_factor = 1;
		end_grains = 20;

		start_angle = 0;
		ray_step = 10;
		ray_ratio = 1.0;
		ray_angle = 0;
		ray_cutoff = 85;
	
		diffusion_probability = 0.95;
		substrate_diffusion = true;

		grid = new Grid;

		//define a blank grain
		grains.push_back(Grain());

		worker = new Worker(num_threads);
	}

	~Growth(){
		delete grid;
		delete worker;
	}

	void init(int num_grains, double min_space, Shape & shape, bool load){
		int starttime = time_msec();

		echo("Initializing %d grains of shape %s, to be run %d times in %.2f increments ... ", num_grains, shape.name, num_steps, growth_factor);
		fflush(stdout);

		FILE * fd = NULL;

		if(opts.layerstats){
			fd = fopen("layerstats.csv", "w");
			fprintf(fd, "layer,num grains,mean area,porousity\n");
			fclose(fd);
		}
		if(opts.timestats){
			fd = fopen("timestats.csv", "w");
			fprintf(fd, "time,num grains,mean height,rms roughness\n");
			fclose(fd);
		}

		Stats::timestats(0, grid, grains);
		grid->cleangrid(0, grains);
		
		double min_space_squared = min_space*min_space;

		if(load){
			fd = fopen("grains.csv", "r");
			char buf[100];
			if(fgets(buf, 99, fd)); //ignore the header
		}

	//generate initial grain information
	//grain number 0 is special as non-existent
		for(int i = 1; i <= num_grains; i++){
			Grain g;
			
			if(load){
				g.load(fd, shape.faces, shape.num_faces);
			}else{
				double dist = FIELD*FIELD;

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

			if(opts.randcolor)
				g.set_color((double)i/num_grains);
			else
				g.directional_color();

			grid->set_point(g.x, g.y, 0, 1, i, 0);
			grains.push_back(g);
		}

		if(load)
			fclose(fd);

		if(!load && opts.graininit){
			fd = fopen("grains.csv", "w");
			fprintf(fd, "grain,x,y,theta1,theta2,phi\n");
			for(int i = 1; i <= num_grains; i++)
				grains[i].dump(fd, i);
			fclose(fd);
		}

		if(opts.voronei){
			echo("Finding closest grains ... ");
			fflush(stdout);
			Stats::voroneimap(grains);
		}

		Stats::timestats(1, grid, grains);
		grid->cleangrid(1, grains);

		echo("done in %d msec\n", time_msec() - starttime);
	}

	void run(){
		int start = time_msec();
		int starttime;
		int remain = grains.size() - 1;

		for(int t = 2; t <= num_steps && !opts.interrupt; t++){
			echo("Step %d, layers %d-%d, %d grains, %d Mb ... ", t, grid->zmin, grid->zmax, remain, grid->memory_usage()/(1024*1024));
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

			if(opts.fluxdump)
				grid->resetflux();

			int growth = 0;
			int raycount = (grid->zmin == 0 ? grid->planes[0]->taken : FIELD*FIELD);
			raycount *= ray_ratio;

			//grow the grains
			if(ray_step == 0 || t <= ray_step){
				for(unsigned int i = 0; i < grains.size(); i++)
					grains[i].grow_faces(growth_factor);
			}else{
				for(int z = grid->zmin; z < grid->zmax; z++)
					worker->add(new CountThreatsReq(this, z));

				worker->wait();

				while(raycount > 0){
					worker->add(new AddFluxReq(this, min(raycount, 1000)));
					raycount -= 1000;
				}

				worker->wait();

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
				for(int z = grid->zmin; z < grid->zmax; z++)
					worker->add(new RunLayerReq(this, z, t, count));

				thisgrowth = worker->wait();

				mem = grid->growgrid();

				growth += thisgrowth;
				count++;
			}while(mem && count < 5 && thisgrowth && thisgrowth > growth/1000.0);


			echo("grew %d points in %d runs in %d msec ... ", growth, count, time_msec() - starttime);
			fflush(stdout);


			starttime = time_msec();

			//output and finished data and images
			Stats::timestats(worker, t, grid, grains);
			grid->cleangrid(t, grains);

			echo("output in %d msec\n", time_msec() - starttime);

			if(!mem){
				echo("Couldn't allocate more memory, current usage ~ %d Mb\n", grid->memory_usage()/(1024*1024));
				break;
			}

			remain = grid->graincount(grains.size());
			if(remain <= end_grains){
				echo("Hit the grain limit: %d grains left\n", remain);
				break;
			}
			
			if(max_memory && grid->memory_usage()/(1024*1024) > max_memory){
				echo("Hit the memory limit: %d Mb\n", max_memory);
				break;
			}
		}

		grid->dump(grains);
		echo("Finished in %d sec\n", (time_msec() - start)/1000);
	}

	void count_threats(int z){
		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){

			//point isn't threatened
				if(grid->get_grain(x,y,z) != THREAT)
					continue;

				Threat threats[27];
				Threat * threats_end = grid->check_face_threats(threats, x, y, z);

				//add to only one of the grains+faces, choosing which randomly
				if(threats != threats_end){ //needed in case the bottom drops off for being inactive
					Threat * i = threats + (rand() % (threats_end - threats));
					INCR(grains[i->grain].threats);
					INCR(grains[i->grain].faces[i->face].threats);
					grid->set_diffprob(x, y, z, (uint8_t)(diffusion_probability * grains[i->grain].faces[i->face].P * 255));
				}
			}
		}
	}

	int run_layer(int z, int t, bool onlynewthreats){
		int growth = 0;

		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){

			//point isn't threatened
				Point * p = grid->get_point(x, y, z);

				if(p->grain == FULLPOINT) //skip this whole sector if it's full
					break;

				if(p->grain != THREAT || (onlynewthreats && p->time != t))
					continue;

				uint16_t threats[27];
				uint16_t * threats_end = grid->check_grain_threats(threats, x, y, z);

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
				grid->set_point(x, y, z, t, best, min_dist.face);
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
		
		double cutoffcos = cos(ray_cutoff * M_PI/180); //cutoff angle of 85 degrees
		double raypow = 1.0/(1.0 + ray_angle);

		for(int i = 0; i < num; i++){
			Ray ray;
			ray.loc.x = unitrand() * FIELD;
			ray.loc.y = unitrand() * FIELD;
			ray.loc.z = grid->zmax-1;

			do{
				costheta = pow(unitrand(), raypow);
			}while(costheta < cutoffcos);
			sintheta = sqrt(1 - costheta*costheta);
			phi = unitrand()*2*M_PI;

			ray.dir.x = cos(phi)*sintheta;
			ray.dir.y = sin(phi)*sintheta;
			ray.dir.z = -costheta;

			Coord3i c = raytrace(ray); //trace the ray until it hits a threat or the substrate
			
			int grain = grid->get_grain(c.x, c.y, c.z);

			if(opts.fluxdump)
				grid->incrflux(c.x, c.y);
			
			if(grain == THREAT){
				if(diffusion_probability > 0)
					c = face_random_walk(c.x, c.y, c.z); //walk along the threats for random length
			}else if(substrate_diffusion && grain == 0 && c.z == 0){ // hit the substrate
				c = substrate_random_walk(c.x, c.y); //walk till it hits a threat
			}else{
				i--; //shoot another ray
				continue; //hit nothing, likely down a deep crevase to points that were already dropped
			}
			
			//add to only one of the grains+faces, choosing which randomly
			Threat threats[27];
			Threat * threats_end = grid->check_face_threats(threats, c.x, c.y, c.z);
			Threat * i = threats + (rand() % (threats_end - threats));
			INCR(grains[i->grain].faces[i->face].flux);
		}
	}

	Coord3i raytrace(Ray ray){
		while(ray.incr(grid->zmin) && grid->get_grain(ray.loc) != THREAT); //empty body

		return Coord3i(ray.loc);
	}

	Coord3i substrate_random_walk(int x, int y){
		do{
			switch(rand() % 4){
				case 0: x++; break;
				case 1: x--; break;
				case 2: y++; break;
				case 3: y--; break;
			}
		}while(grid->get_grain(x, y, 0) != THREAT);

		return Coord3i(x, y, 0);
	}

	Coord3i face_random_walk(int x, int y, int z){
		Point * p = grid->get_point(x, y, z);

		while((rand() % 256) < p->diffprob){
retry: //used to retry on when the random choice below is invalid, without re-checking the probability
			int dir = rand() % 6;
			switch(dir){
				case 0: x++; break;
				case 1: x--; break;
				case 2: y++; break;
				case 3: y--; break;
				case 4: if(z == grid->zmax - 1){ goto retry; } else { z++; break; }
				case 5: if(z == grid->zmin    ){ goto retry; } else { z--; break; }
			}

			p = grid->get_point(x, y, z);

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

		return Coord3i(x, y, z);
	}

/*
//face walk that tries to be faster and more fair than the random walk
	Coord3i face_walk(int x, int y, int z){
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
			if(!ray.incr(grid->zmin))
				break;
				
			Point * p = grid->get_point(x, y, z);

			//move one unit in the direction
			//substract face diffusion length factor from length
			//if no longer on this face
				//figure out which face it is moving to
				//find the spine
				//set the new direction
		}

		return Coord3i(x, y, z);
	}
*/
};


