
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <stdint.h>
#include <cmath>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <cstdarg>
#include "gd.h"
#include <pthread.h>
#include <queue>
#include <algorithm>

using namespace std;

//some compile time options
#ifndef FIELD
#define FIELD (1<<10) //1024, size of the grid
#endif

#ifndef MAX_THREADS
#define MAX_THREADS 100 //slight speed improvement by setting to 1
#endif

struct Options {
	bool cmdline;   // output the command line that was used
	bool console;   // output the console output to a file too
	bool timestats; // output time stats
	bool layerstats;// output layer stats
	bool slopemap;  // map of slopes, easiest visualization
	bool isomorphic;// isomorphic visualization
	bool heightmap; // map of heights
	bool heightdump;// dump of heights, may be possible to turn into a 3d model
	bool timemap;   // map of grains as a top down view, may be useful for stats?
	bool fluxdump;  // dump of amount of flux received per x,y coord
	bool peaks;     // dump of the active peaks per timestep: id,x,y,z
	bool layermap;  // map of grains of a layer, may be useful for stats?
	bool voronei;   // voronei diagram of initial grain placements
	bool graininit; // output the initial grain placements
	bool growth;    // dump of growth of each grain/face
	bool datadump;  // full grid data dump, could be read after the fact to generate any stats needed
	bool savemem;   // dump the data grid temporarily to save memory
	bool pockets;   // mark and drop pockets, potentially save more memory and get better data dumps
	bool interrupt; // set by the interrupt handler, meaning finish your current iteration then exit
	bool randcolor; // use random colours instead of directional colours
} opts;

inline void echo(const char *format, ...){
	char buffer[1024];

	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer)-1, format, args);
	va_end(args);

	printf("%s", buffer);
	
	if(opts.console){
		FILE *fd = fopen("console.txt", "a");
		fprintf(fd, "%s", buffer);
		fclose(fd);
	}
}

void interrupt(int sig){
	if(opts.interrupt){
		echo("Second interrupt, exiting ungracefully\n");
		exit(1);
	}
	opts.interrupt = true;
}

#include "shapes.h"
#include "grain.cpp"
#include "grid.cpp"
#include "growth.cpp"

int main(int argc, char **argv){
	srand(time(NULL));

	signal(SIGINT,  interrupt);
	signal(SIGTERM, interrupt);

	opts.cmdline   = true;
	opts.console   = true;
	opts.layerstats= true;
	opts.timestats = true;
	opts.layermap  = true;
	opts.slopemap  = true;
	opts.isomorphic= false;
	opts.heightmap = false;
	opts.heightdump= false;
	opts.timemap   = false;
	opts.fluxdump  = false;
	opts.peaks     = false;
	opts.voronei   = false;
	opts.graininit = false;
	opts.datadump  = false;
	opts.savemem   = false;
	opts.pockets   = false;
	opts.interrupt = false;
	opts.randcolor = false;

	char * dir        = NULL;
	int    max_memory = 0;
	int    threads    = min(5, MAX_THREADS);

	bool   load_data  = false;
	int    num_steps  = 200;
	double growth_factor = 1.0;
	double start_angle = 0.0;
	int    num_grains = 10000;
	int    end_grains = 20;
	double min_dist   = 0.0;
	double ray_angle  = 0;
	double ray_cutoff = 85;
	int    ray_step   = 10;
	double ray_ratio  = 1.0;
	double diffusion  = 0;
	bool   substrate_diffusion = true;
	int    shape_id   = 6;

	for(int i = 1; i < argc; i++){
		char * ptr = argv[i];
		if(strcmp(ptr, "-h") == 0 || strcmp(ptr, "--help") == 0){
			printf("Usage: %s -d <dir> [options]\n"
				"Ex: %s -d data -n 1000 -s 8 -z \"Quick run of Octahedrons\"\n"
				"Basic Options:\n"
				"\t-h --help       Show this help\n"
				"\t-d --dir        Directory to dump output files [./]\n"
				"\t-z              Include a descriptive message to the command line\n"
				"\t-m --memory     Maximum memory usage in Mb [unlimited]\n",
				argv[0], argv[0]);
#if MAX_THREADS > 1
		printf(	"\t-t --threads    Number of worker threads [%d]\n", threads);
#endif
		printf(	"\nOutput Options:\n"
				"\t   --cmdline    Output the command line                to cmdline.txt     - on\n"
				"\t   --console    Copy the console output                to console.txt     - on\n"
				"\t   --timestats  Output stats per timestep              to timestats.csv   - on\n"
				"\t   --layerstats Output stats per layer                 to layerstats.csv  - on\n"
				"\t   --layermap   Output a layer  map  for each layer    to layer.%%05d.png  - on\n"
				"\t   --slopemap   Output a slope  map  for each timestep to slope.%%05d.png  - on\n"
				"\t   --surf3d     Output a surf3d map  for each timestep to surf3d.%%05d.png - off\n"
				"\t   --timemap    Output a time   map  for each timestep to time.%%05d.png   - off\n"
				"\t   --heightmap  Output a height map  for each timestep to height.%%05d.png - off\n"
				"\t   --heightdump Output a height dump for each timestep to height.%%05d.dat - off\n"
				"\t   --fluxdump   Output a flux   dump for each timestep to flux.%%05d.dat   - off\n"
				"\t   --datadump   Output a binary dump for each layer    to data.%%05d.dat   - off\n"
				"\t   --growth     Output a growth list for each timestep to growth.%%05d.csv - off\n"
				"\t   --peaks      Output a peaks  list for each timestep to peaks.%%05d.csv  - off\n"
				"\t   --graininit  Output initial grain placements        to grains.csv      - off\n"
				"\t   --voronei    Output a voronei map of initial grains to voronei.png     - off\n"
				"\t   --pockets    Mark pockets in datadump, saves memory with --savemem     - off\n"
				"\t   --savemem    Dump the data to disk (temporarily) to save memory        - off\n"
				"\t   --randcolor  Use random grain coloring, not directional coloring       - off\n"
				"\t   --dataformat Output a description of the binary format\n"
				"\t-q --quiet      Disable all output, though they can be re-enabled individually\n"
				"\t-v --verbose    Enable all output options\n"
				"\t-S --stats      Equivalent to --timestats --layerstats\n");
		printf(	"\nRun Options:\n"
				"\t-l --load       Load location and rotation data from graininit, still needs shapes and grain count\n"
 				"\t-n --steps      Number of time steps to run [%d]\n"
				"\t-g --grains     Number of Grains to simulate [%d]\n"
				"\t-e --endgrains  Stop the simulation when this many are left [%d]\n"
				"\t-S --startangle Angle constant to center the grain rotation distribution around [%.2f]\n"
				"\t   --sep        Minimum grain separation [auto]\n"
				"\t-f --factor     Growth Factor (0,1], can slow down the sim for greater accuracy [%.2f]\n"
				"\t-r --raystep    Step to start using ray tracing to add flux, 0 to disable rays [%d]\n"
				"\t-R --rayratio   Number of rays to generate per occupied point [%.2f]\n"
				"\t-a --angleconst Angle constant to center the ray distribution around (evap: 50+, sputter: 1-2, lpcvd: 0) [%.2f]\n"
				"\t-c --cutoff     Cutoff angle for rays as measured from vertical, anything bigger is ignored [%.1f]\n"
				"\t-D --diffusion  Probability of each diffusion step [0,1), only useful with raytracing [%.2f]\n"
				"\t   --no-subdiff Turn off substrate diffusion, shoot extra rays until they hit a grain\n"
				"\t-s --shape      Shape (4,5,6,7,8,9,12,13,14,20,26,252) [%d]\n"
				"\t   --shapes     List the available shapes\n",
				num_steps, num_grains, end_grains, start_angle, growth_factor, ray_step, ray_ratio, ray_angle, ray_cutoff, diffusion, shape_id);
			exit(255);
		} else if(strcmp(ptr, "--shapes") == 0){
			printf("List of chooseable shapes\n"
				"\t 4 - Tetrahedron\n"
				"\t 5 - Stretched Cube\n"
				"\t 6 - Cube\n"
				"\t 7 - Twin Tetrahedron\n"
				"\t 8 - Octahedron\n"
				"\t 9 - Stretched Hex\n"
				"\t12 - Dodecahedron\n"
				"\t13 - Rhombic Dodecahedron\n"
				"\t14 - Cuboctahedron\n"
				"\t20 - Icosahedron\n"
				"\t26 - Rhombic Cuboctahedron\n"
				"\t252 - 252 sided near-sphere\n"
			);
			exit(255);
		} else if(strcmp(ptr, "-d") == 0 || strcmp(ptr, "--dir") == 0) {
			dir = argv[++i];
			if(dir == NULL) { printf("Please specify output directory\n"); exit(1); }
		} else if(strcmp(ptr, "-z") == 0 || strcmp(ptr, "--message") == 0) {
			ptr = argv[++i]; //ignore the next argument
			opts.cmdline = true;
		} else if(strcmp(ptr, "-t") == 0 || strcmp(ptr, "--threads") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify Maximum thread count\n"); exit(1); }
			threads = atoi(ptr);
			if(threads < 1 || threads > MAX_THREADS){ printf("Thread count out of range, max: %d\n", MAX_THREADS); exit(1); }
		} else if(strcmp(ptr, "-m") == 0 || strcmp(ptr, "--memory") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify Maximum memory\n"); exit(1); }
			max_memory = atoi(ptr);
			if(max_memory < 1){ printf("Max memory out of range\n"); exit(1); }
		} else if(strcmp(ptr, "-v") == 0 || strcmp(ptr, "--verbose") == 0) {
			opts.cmdline   = true;
			opts.console   = true;
			opts.layerstats= true;
			opts.timestats = true;
			opts.slopemap  = true;
			opts.isomorphic= true;
			opts.heightmap = true;
			opts.heightdump= true;
			opts.timemap   = true;
			opts.fluxdump  = true;
			opts.peaks     = true;
			opts.layermap  = true;
			opts.voronei   = true;
			opts.graininit = true;
			opts.growth    = true;
			opts.datadump  = true;
			opts.savemem   = true;
			opts.pockets   = true;
		} else if(strcmp(ptr, "-q") == 0 || strcmp(ptr, "--quiet") == 0) {
			opts.cmdline   = false;
			opts.console   = false;
			opts.layerstats= false;
			opts.timestats = false;
			opts.slopemap  = false;
			opts.isomorphic= false;
			opts.heightmap = false;
			opts.heightdump= false;
			opts.timemap   = false;
			opts.fluxdump  = false;
			opts.peaks     = false;
			opts.layermap  = false;
			opts.voronei   = false;
			opts.graininit = false;
			opts.growth    = false;
			opts.datadump  = false;
			opts.savemem   = false;
			opts.pockets   = false;
		} else if(strcmp(ptr, "-S") == 0 || strcmp(ptr, "--stats") == 0){
			opts.timestats  = true;
			opts.layerstats = true;
		} else if(strcmp(ptr, "--cmdline") == 0) {
			opts.cmdline = true;
		} else if(strcmp(ptr, "--console") == 0) {
			opts.console = true;
		} else if(strcmp(ptr, "--layerstats") == 0) {
			opts.layerstats = true;
		} else if(strcmp(ptr, "--timestats") == 0) {
			opts.timestats = true;
		} else if(strcmp(ptr, "--slopemap") == 0) {
			opts.slopemap = true;
		} else if(strcmp(ptr, "--surf3d") == 0) {
			opts.isomorphic = true;
		} else if(strcmp(ptr, "--heightmap") == 0) {
			opts.heightmap = true;
		} else if(strcmp(ptr, "--heightdump") == 0) {
			opts.heightdump = true;
		} else if(strcmp(ptr, "--timemap") == 0) {
			opts.timemap = true;
		} else if(strcmp(ptr, "--fluxdump") == 0) {
			opts.fluxdump = true;
		} else if(strcmp(ptr, "--peaks") == 0) {
			opts.peaks = true;
		} else if(strcmp(ptr, "--layermap") == 0) {
			opts.layermap = true;
		} else if(strcmp(ptr, "--voronei") == 0) {
			opts.voronei = true;
		} else if(strcmp(ptr, "--graininit") == 0) {
			opts.graininit = true;
		} else if(strcmp(ptr, "--growth") == 0) {
			opts.growth = true;
		} else if(strcmp(ptr, "--datadump") == 0) {
			opts.datadump = true;
		} else if(strcmp(ptr, "--savemem") == 0) {
			opts.savemem = true;
		} else if(strcmp(ptr, "--pockets") == 0) {
			opts.pockets = true;
		} else if(strcmp(ptr, "--randcolor") == 0) {
			opts.randcolor = true;
		} else if(strcmp(ptr, "--dataformat") == 0) {
			Point point;
			printf("The binary format is a one file per layer, in a large array Point structures\n");
			printf("Each array entry is %d bytes with elements:\n", (int)sizeof(point));
			printf("\tTime     - uint16_t - %d bytes - timestep this point was taken\n", (int)sizeof(point.time));
			printf("\tGrain    - uint16_t - %d bytes - grain this point was taken by\n", (int)sizeof(point.grain));
			printf("\tFace     - uint8_t  - %d bytes - face on the grain that took it\n", (int)sizeof(point.face));
			printf("\tDiffprob - uint8_t  - %d bytes - diffusion probability at this point when it was a threat\n", (int)sizeof(point.diffprob));
			printf("Anything left is empty space for alignment\n");
			printf("Grain has a couple non-grain special values\n");
			printf("\t0x%X - Threat\n", THREAT);
			printf("\t0x%X - Pocket\n", POCKET);
			printf("\t0x%X - Threatened Pocket\n", TPOCKET);
			printf("\t0x%X - Max grain\n", MAXGRAIN);
			exit(255);
		} else if(strcmp(ptr, "-n") == 0 || strcmp(ptr, "--steps") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify Number of steps\n"); exit(1); }
			num_steps = atoi(ptr);
			if(num_steps < 2 || num_steps > 65000){ printf("Num Steps out of range\n"); exit(2); }
		} else if(strcmp(ptr, "-f") == 0 || strcmp(ptr, "--factor") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify Growth factor\n"); exit(1); }
			growth_factor = atof(ptr);
			if(growth_factor <= 0 || growth_factor > 1.0) { printf("Growth factor out of range\n"); exit(2); }
		} else if(strcmp(ptr, "-l") == 0 || strcmp(ptr, "--load") == 0) {
			load_data = true;
		} else if(strcmp(ptr, "-g") == 0 || strcmp(ptr, "--grains") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify Number of grains\n"); exit(1); }
			num_grains = atoi(ptr);
			if(num_grains < 1 || num_grains > 65000){ printf("Num Grains out of range\n"); exit(2); }
		} else if(strcmp(ptr, "-e") == 0 || strcmp(ptr, "--endgrains") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify Number of end grains\n"); exit(1); }
			end_grains = atoi(ptr);
			if(end_grains < 0 || num_grains > 65000){ printf("Num end Grains out of range\n"); exit(2); }
		} else if(strcmp(ptr, "-S") == 0 || strcmp(ptr, "--startangle") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify Start Angle constant\n"); exit(1); }
			start_angle = atof(ptr);
			if(start_angle < 0){ printf("Start angle constant out of range\n"); exit(2); }
		} else if(strcmp(ptr, "--sep") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify Minimum grain separation\n"); exit(1); }
			min_dist = atof(ptr);
			if(min_dist < 1){ printf("Grain separation out of range\n"); exit(1); }
		} else if(strcmp(ptr, "-r") == 0 || strcmp(ptr, "--raystep") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify step to start using flux\n"); exit(1); }
			ray_step = atoi(ptr);
			if(ray_step < 0){ printf("Ray step out of range\n"); exit(1); }
		} else if(strcmp(ptr, "-R") == 0 || strcmp(ptr, "--rayratio") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify ray ratio\n"); exit(1); }
			ray_ratio = atoi(ptr);
			if(ray_ratio < 1){ printf("Ray ratio out of range\n"); exit(1); }
		} else if(strcmp(ptr, "-a") == 0 || strcmp(ptr, "--angleconst") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify ray angle constant\n"); exit(1); }
			ray_angle = atof(ptr);
			if(ray_angle < 0){ printf("Ray angle constant out of range\n"); exit(1); }
		} else if(strcmp(ptr, "-c") == 0 || strcmp(ptr, "--cutoff") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify ray cutoff angle\n"); exit(1); }
			ray_cutoff = atof(ptr);
			if(ray_cutoff < 1 || ray_cutoff > 89){ printf("Ray cutoff angle out of range\n"); exit(1); }
		} else if(strcmp(ptr, "-D") == 0 || strcmp(ptr, "--diffusion") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify diffusion probability\n"); exit(1); }
			diffusion = atof(ptr);
			if(diffusion < 0 || diffusion >= 1){ printf("Diffusion probability out of range\n"); exit(1); }
		} else if(strcmp(ptr, "-s") == 0 || strcmp(ptr, "--shape") == 0) {
			ptr = argv[++i];
			if(ptr == NULL) { printf("Please specify Shape\n"); exit(1);	}
			shape_id = atoi(ptr);
		} else if(strcmp(ptr, "--no-subdiff") == 0){
			substrate_diffusion = false;
		} else {
			printf("Unknown argument %s\n", ptr);
			exit(1);
		}
	}

	if(dir == NULL || chdir(dir) == -1){
		printf("Couldn't switch directories to %s\n", dir); 
		exit(2);
	}

	if(opts.cmdline){
		FILE *fd = fopen("cmdline.txt", "w");
		for(int i = 0; i < argc; i++)
			fprintf(fd, "%s ", argv[i]);
		fprintf(fd, "\n");
		fclose(fd);
	}

	if(min_dist == 0.0)
		min_dist = sqrt(FIELD*FIELD/(M_PI*num_grains));

	Shape shape;

	switch(shape_id){
		case 4:  shape = Tetrahedron;          break;
		case 5:  shape = Stretched_cube;       break;
		case 6:  shape = Cube;                 break;
		case 7:  shape = Twin_tetrahedron;     break;
		case 8:  shape = Octahedron;           break;
		case 9:  shape = Stretched_hex;        break;
		case 12: shape = Dodecahedron;         break;
		case 13: shape = Rhombic_dodecahedron; break;
		case 14: shape = Cuboctahedron;        break;
		case 20: shape = Icosahedron;          break;
		case 26: shape = Rhombic_Cuboctahedron;break;
		case 252:shape = Sphere252;            break;
		default: printf("Unknown shape\n"); exit(1);
	}



	Growth growth(threads);

	growth.num_steps = num_steps;
	growth.end_grains = end_grains;
	growth.growth_factor = growth_factor;
	
	growth.max_memory = max_memory;

	growth.start_angle = start_angle;
	growth.ray_angle   = ray_angle;
	growth.ray_cutoff  = ray_cutoff;
	growth.ray_step    = ray_step;
	growth.ray_ratio   = ray_ratio;
	
	growth.diffusion_probability = diffusion;
	growth.substrate_diffusion = substrate_diffusion;

	growth.init(num_grains, min_dist, shape, load_data);

	growth.run();

	return 0;
}

