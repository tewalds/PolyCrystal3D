

#define FULLPOINT (0xFFFF) //internal value to mean this point was dropped to disk
#define THREAT    (0xFFFE) //this point is threatened by other points, but is empty
#define POCKET    (0xFFFD) //this point is empty space, but can never be taken since it is in a pocket
#define TPOCKET   (0xFFFC) //also in a pocket, but at the edge of the pocket and threatened
#define MARK      (0xFFFB) //marks a threat for the mark/sweep pocket search
#define MAXGRAIN  (0xFFF0) //max amount of grains, anything above is reserved for special values

int periodic_dist_sq(int x1, int y1, int x2, int y2){
	int dx = abs(x1 - x2);
	int dy = abs(y1 - y2);
	
	if(dx > FIELD/2)
		dx = abs(dx - FIELD);
	if(dy > FIELD/2)
		dy = abs(dy - FIELD);

	return dx*dx + dy*dy;
}

struct Threat {
	uint16_t grain;
	uint8_t  face;
};

struct Coord {
	int x, y, z;
	
	Coord(){
		x = y = z = 0;
	}
	Coord(int X, int Y, int Z){
		x = X;
		y = Y;
		z = Z;
	}
};

struct Point {
	uint16_t time;  // time it was taken
	uint16_t grain; // grain that took it
	uint8_t  face;  // face on that grain
	uint8_t  diffprob; //probability of diffusion from the point. Only makes sense if this point is a threat

	Point(){
		time = 0;
		grain = 0;
		face = 0;
		diffprob = 0;
	}
	
	Point(uint16_t t, uint16_t g, uint8_t f, uint8_t d){
		time = t;
		grain = g;
		face = f;
		diffprob = d;
	}
};

Point empty_point;
Point full_point = Point(FULLPOINT, FULLPOINT, 0xFE, 0);

struct Sector {
	uint16_t fullpoints;
	Point * points;

	Sector(){
		fullpoints = 0;
		points = NULL;
	}
	
	~Sector(){
		drop();
	}

	void alloc(){
		if(!points){
			Point * temp = new Point[FIELD];
			CASv(points, NULL, temp);
			if(points != temp) //already set by a different thread
				delete[] temp;
		}
	}

	void mark(int i){
		if(points && points[i].grain == THREAT)
			points[i].grain = MARK;
	}

	bool unmark(int i){
		if(marked(i)){
			points[i].grain = THREAT;
			return true;
		}
		return false;
	}
	
	bool marked(int i){
		return (points && points[i].grain == MARK);
	}

	void set(int i, Point & p){
		alloc();
		points[i] = p;
		INCR(fullpoints);
	}

	void set_threat(int i, int t){
		alloc();

	 //only set it to be threatend if it's currently empty
	 //reset the time of the threat, but only if it's a threat
	 //valid without CAS since if a different thread sets a grain, it'll be the same timestamp
		CASv(points[i].grain, 0, THREAT);
		if(points[i].grain == THREAT)
			points[i].time = t;
	}

	Point * get(int i){
		if(points)
			return &(points[i]);
		if(fullpoints)
			return & full_point;
		return & empty_point;
	}

	bool full(){
		return (fullpoints == FIELD);
	}

	void dump(FILE * fd){
		if(points){ //write out the data
			if(fwrite(points, sizeof(Point), FIELD, fd));
			drop();
		}else if(full()){
			fseek(fd, FIELD*sizeof(Point), SEEK_CUR); //already written, skip ahead the right distance
		}else{ //write 0s
			for(int i = 0; i < FIELD; i++)
				if(fwrite(& empty_point, sizeof(Point), 1, fd));				
		}
	}

	void drop(){
		if(points){
			delete[] points;
			points = NULL;
		}
	}

	void load(FILE * fd){
		if(points){
			fseek(fd, FIELD*sizeof(Point), SEEK_CUR);
		}else{
			alloc();
			if(fread(points, sizeof(Point), FIELD, fd));
		}
	}
};

struct Plane {
	Sector grid[FIELD];
	int taken;
	int time;
	FILE * data_fd;

	Plane(){
		time = 0;
		taken = 0;
		data_fd = NULL;
	}

	~Plane(){
		if(data_fd){
			fclose(data_fd);
			data_fd = NULL;
		}
	}

	long memory_usage(){
		long mem = sizeof(Plane);
		for(int i = 0; i < FIELD; i++)
			if(grid[i].points)
				mem += sizeof(Point)*FIELD;
		return mem;
	}

	Point * get(int x, int y){
		return grid[y].get(x);
	}

	void set(int x, int y, Point & p){
		grid[y].set(x, p);

		INCR(taken);
		time = p.time;
	}
	
	void set_threat(int x, int y, int t){
		grid[y].set_threat(x, t);
	}

	bool marked(int x, int y){
		return grid[y].marked(x);
	}	
	void mark(int x, int y){
		grid[y].mark(x);
	}
	bool unmark(int x, int y){
		return grid[y].unmark(x);
	}

	void dump(int level, int sector = -1){
	//dump to a data file

		if(!data_fd){
			char filename[50];
			sprintf(filename, "grid.%05d", level);

			//create the file if needed... stupid fopen not having a mode to open a file in write mode, 
			//  creating it if needed, and not truncating it if it already exists
			data_fd = fopen(filename, "ab");
			fclose(data_fd);

			//actually open the file now... rb+ since there's no write only without truncating and without appending mode
			data_fd = fopen(filename, "rb+");
		}

		if(sector == -1){
			for(int y = 0; y < FIELD; y++)
				grid[y].dump(data_fd);
		}else{
			fseek(data_fd, sizeof(Point)*FIELD*sector, SEEK_SET);
			grid[sector].dump(data_fd);
		}
	}

	void delfile(int level){
		if(data_fd){
			fclose(data_fd);
			data_fd = NULL;
		}
		
		char filename[50];
		sprintf(filename, "grid.%05d", level);

		remove(filename);			
	}

	bool load(int level){
		char filename[50];
		FILE * fd;

		sprintf(filename, "grid.%05d", level);
		fd = fopen(filename, "rb");

		if(!fd)
			return false;

		for(int y = 0; y < FIELD; y++)
			grid[y].load(fd);

		fclose(fd);

		return true;
	}
	
	void stats(int level, int maxgraincount){
/*
- grain count
- mean grain area
- porousity (fraction that is filled)
*/

		vector<int> counts(maxgraincount, 0);
	
		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){
				Point * p = get(x, y);
				if(p->grain < MAXGRAIN)
					counts[p->grain]++;
			}
		}

		int num = 0;
		for(int i = 1; i < maxgraincount; i++) //start at 1 since 0 is a special grain
			if(counts[i])
				num++;
	
		FILE * fd = fopen("levelstats", "a");
		fprintf(fd, "%d,%d,%f,%f\n", level, num, (double)(FIELD*FIELD - counts[0])/num, (double)counts[0]/(FIELD*FIELD));
		fclose(fd);
	}
	
	void layermap(int level, vector<Grain> & grains){
	//generate a png layermap
		gdImagePtr im = gdImageCreateTrueColor(FIELD, FIELD);
		gdImageFill(im, 0, 0, gdImageColorAllocate(im, 0, 0, 0));

		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){
				Point * p = get(x, y);
				int grain = p->grain;
				if(grain != 0 && grain < MAXGRAIN){
					RGB rgb = RGB::HSV(grains[grain].color, 1.0, 1.0);
					int color = gdImageColorAllocate(im, rgb.r, rgb.g, rgb.b);
					gdImageSetPixel(im, x, y, color);
				}
			}
		}

		char filename[50];
		sprintf(filename, "level.%05d.png", level);
		FILE * fd = fopen(filename, "wb");
		gdImagePng(im, fd);
		fclose(fd);
		gdImageDestroy(im);
	}
};

class Grid {
public:
	Plane * planes[10000]; //better be deep enough...
	uint16_t heights[FIELD][FIELD];
	uint8_t flux[FIELD][FIELD];

	int surfacethreats;

	//quick linear scan, quick because the list will always be tiny
	static Threat * find(Threat * pos, Threat * end, uint16_t grain, uint8_t face){
		while(pos != end && pos->grain != grain && pos->face != face) ++pos;
		return pos;
	}
	static uint16_t * find(uint16_t * pos, uint16_t * end, uint16_t val){
		while(pos != end && *pos != val) ++pos;
		return pos;
	}

	int zmin;
	int zmax;

	Grid(){
		zmin = 0;
		zmax = 3;

		for(int i = zmin; i < zmax; i++)
			planes[i] = new Plane();

		for(int y = 0; y < FIELD; y++)
			for(int x = 0; x < FIELD; x++)
				heights[y][x] = 0;
	}

	long memory_usage(){
		long mem = sizeof(heights) + sizeof(flux);

		for(int i = zmin; i < zmax; i++)
			mem += planes[i]->memory_usage();

		return mem;
	}

	void output(int t, vector<Grain> & grains){
		if(opts.fluxmap)
			fluxmap(t);
		if(opts.slopemap)
			slopemap(t, grains);
		if(opts.heightmap)
			heightmap(t, grains);
		if(opts.timemap)
			timemap(t, grains);
		if(opts.stats)
			stats(t, grains.size());
		if(opts.peaks)
			peaks(t, grains.size());
	}

	void peaks(int t, int maxgraincount){
		vector<Coord> peaks(maxgraincount);
	
		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){
				int z = heights[y][x];
				int grain = get_grain(x, y, z);
				if(grain && grain < MAXGRAIN && peaks[grain].z < z){
					bool peak = true;
					for(int x1 = -1; x1 <= 1; x1++){
						for(int y1 = -1; y1 <= 1; y1++){
							if(x1 == x && y1 == y)
								continue;
							int g = get_grain(x - x1, y - y1, z);
							if(g != 0 && g != grain && g < MAXGRAIN)
								peak = false;
						}
					}
					if(peak)
						peaks[grain] = Coord(x, y, z);
				}
			}	
		}

		char filename[50];
		sprintf(filename, "peaks.%05d", t);
		FILE * fd = fopen(filename, "w");
		for(int i = 0; i < maxgraincount; i++){
			if(peaks[i].z)
				fprintf(fd, "%d,%d,%d,%d\n", i, peaks[i].x, peaks[i].y, peaks[i].z);
		}
		fclose(fd);
	}

	void stats(int t, int maxgraincount){
/*
- number of surface grains (count of grains in height map)
- mean height (average max z)
- rms roughness = sqrt(sum((h - avgH)^2)/FIELD^2)
*/
		vector<int> counts(maxgraincount, 0);
		uint64_t totalheight = 0;

		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){
				int grain = get_grain(x, y, heights[y][x]);
				if(grain != 0 && grain < MAXGRAIN){
					counts[grain]++;
					totalheight += heights[y][x];
				}
			}
		}

		double mean = (double)totalheight/(FIELD*FIELD);
		
		double totalms = 0;
		for(int y = 0; y < FIELD; y++)
			for(int x = 0; x < FIELD; x++)
				totalms += (heights[y][x] - mean)*(heights[y][x] - mean);
		
		double rms = sqrt(totalms/(FIELD*FIELD));

		int num = 0;
		for(int i = 1; i < maxgraincount; i++) //start at 1 since 0 is a special grain
			if(counts[i])
				num++;
	
		FILE * fd = fopen("timestats", "a");
		fprintf(fd, "%d,%d,%f,%f\n", t, num, mean, rms);
		fclose(fd);
	}

	// keep two empty planes on the top
	bool growgrid(){
		if(planes[zmax-2]->taken){
			try{
				planes[zmax] = new Plane();
			}catch(std::bad_alloc){
				return false;
			}
			zmax++;
		}
		return true;
	}

	void pocketsearch(){
		//set all threats to MARK, ie threats that haven't been validated as executable threats
		for(int z = zmin; z < zmax; z++)
			for(int y = 0; y < FIELD; y++)
				for(int x = 0; x < FIELD; x++)
					planes[z]->mark(x, y);

		surfacethreats = 0;

		//reset all surface threats to actual THREATs
		unmark(0, 0, heights[0][0]+1);
		
		//search for still MARKed threats, set them to TPOCKET, fill the internal space with POCKET
		for(int z = zmin; z < zmax; z++)
			for(int y = 0; y < FIELD; y++)
				for(int x = 0; x < FIELD; x++)
					if(planes[z]->marked(x, y))
						sweep_pockets(x, y, z);
	}

	//reset all reachable MARK threats to real THREATs
	void unmark(int x, int y, int z){
		queue<Coord> q;

		q.push(Coord(x, y, z));

		Coord c;
		while(!q.empty()){
			c = q.front();
			q.pop();

			if(c.z < zmin || c.z >= zmax)
				continue;

			fix_period(c.x, c.y);

			if(planes[c.z]->unmark(c.x, c.y)){
				surfacethreats++;

				q.push(Coord(c.x-1, c.y, c.z));
				q.push(Coord(c.x+1, c.y, c.z));
				q.push(Coord(c.x, c.y-1, c.z));
				q.push(Coord(c.x, c.y+1, c.z));
				q.push(Coord(c.x, c.y, c.z-1));
				q.push(Coord(c.x, c.y, c.z+1));
			}
		}
	}

	void sweep_pockets(int x, int y, int z){
		queue<Coord> q;

		q.push(Coord(x, y, z));

		Coord c;
		while(!q.empty()){
			c = q.front();
			q.pop();

			if(c.z < zmin || c.z >= zmax)
				continue;

			fix_period(c.x, c.y);

			Point * p = planes[c.z]->get(c.x, c.y);
			Point n;
			if(p->grain == MARK){
				n = *p;
				n.grain = TPOCKET;
			}else if(!p->grain){ //empty space
				n.grain = POCKET;
			}else{
				continue;
			}
		
			planes[c.z]->set(c.x, c.y, n);
		
			q.push(Coord(c.x-1, c.y, c.z));
			q.push(Coord(c.x+1, c.y, c.z));
			q.push(Coord(c.x, c.y-1, c.z));
			q.push(Coord(c.x, c.y+1, c.z));
			q.push(Coord(c.x, c.y, c.z-1));
			q.push(Coord(c.x, c.y, c.z+1));
		}
	}

	//dump full or inactive planes off the bottom, output layer based data
	void cleangrid(int t, vector<Grain> & grains){
		int minheight = heights[0][0];
		for(int y = 0; y < FIELD; y++)
			for(int x = 0; x < FIELD; x++)
				if(minheight > heights[y][x])
					minheight = heights[y][x];

	//mark all pockets as such
		if(minheight > 0 && (opts.savemem || opts.datadump))
			pocketsearch();

	//drop each sector that is completely surrounded by full or dropped sectors
		if(opts.savemem){
			for(int z = zmin; z < zmax - 5; z++){
				for(int y = 0; y < FIELD; y++){
					if(planes[z]->grid[y].full()){
						bool full = true;
						for(int Z = max(zmin, z - 1); Z <= z + 1; Z++)
							for(int Y = y - 1; Y <= y + 1; Y++)
								if(!planes[Z]->grid[(Y + FIELD) % FIELD].full())
									full = false;
						if(full)
							planes[z]->dump(z, y);
					}
				}
			}
		}

	//find new zmin
		int newmin = zmin;
		for(int i = minheight; i >= zmin; i--){
			if(planes[i]->taken == FIELD*FIELD || planes[i]->time + 25 < t){
				newmin = i;
				break;
			}
		}

	//dump all planes under newmin
		dump(grains, newmin);
	}

	void dump(vector<Grain> & grains, int max = -1){
		if(max == -1)
			max = zmax;

		for(int i = zmin; i < max; i++){
			if(opts.savemem)
				planes[i]->load(i);
			if(opts.layermap)
				planes[i]->layermap(i, grains);
			if(opts.stats)
				planes[i]->stats(i, grains.size());
			if(opts.datadump)
				planes[i]->dump(i);

			if(opts.savemem && !opts.datadump)
				planes[i]->delfile(i);

			drop(i);
		}
		zmin = max;
	}

	void load(int i){
		planes[i] = new Plane();
		planes[i]->load(i);
	}

	void drop(int i){
		delete planes[i];
		planes[i] = NULL;
	}

	void resetflux(){
		for(int y = 0; y < FIELD; y++)
			for(int x = 0; x < FIELD; x++)
				flux[y][x] = 0;
	}
	
	void incrflux(int x, int y){
		fix_period(x, y);
		INCR(flux[y][x]);		
	}

	void fluxmap(int t) const {
	//dump to a data file
		char filename[50];
		FILE* fd;

		sprintf(filename, "fluxmap.%05d", t);
		fd = fopen(filename, "wb");
		
		for(int y = 0; y < FIELD; y++)
			if(fwrite(flux[y], sizeof(uint8_t), FIELD, fd));

		fclose(fd);
	}

	void heightmap(int time, vector<Grain> & grains) const {
	//generate a png height map
		double diffheight = zmax - zmin;

		gdImagePtr im = gdImageCreateTrueColor(FIELD, FIELD);
		gdImageFill(im, 0, 0, gdImageColorAllocate(im, 0, 0, 0));

		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){
				if(heights[y][x]){
					int grain = get_grain(x, y, heights[y][x]);
					RGB rgb = RGB::HSV(grains[grain].color, 1.0 - ((double)(heights[y][x] - zmin)/diffheight), 1.0);
					int color = gdImageColorAllocate(im, rgb.r, rgb.g, rgb.b);
					gdImageSetPixel(im, x, y, color);
				}
			}
		}

		char filename[50];
		sprintf(filename, "height.%05d.png", time);
		FILE * fd = fopen(filename, "wb");
		gdImagePng(im, fd);
		fclose(fd);
		gdImageDestroy(im);
	}

	void slopemap(int time, vector<Grain> & grains) const {
	//generate a png slope map
		gdImagePtr im = gdImageCreateTrueColor(FIELD, FIELD);
		gdImageFill(im, 0, 0, gdImageColorAllocate(im, 0, 0, 0));

		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){
				if(heights[y][x]){
					Point * p = get_point(x, y, heights[y][x]);
					RGB rgb = grains[p->grain].get_face_color(p->face);
					int color = gdImageColorAllocate(im, rgb.r, rgb.g, rgb.b);
					gdImageSetPixel(im, x, y, color);
				}
			}
		}

		char filename[50];
		sprintf(filename, "slope.%05d.png", time);
		FILE * fd = fopen(filename, "wb");
		gdImagePng(im, fd);
		fclose(fd);
		gdImageDestroy(im);
	}

	void timemap(int time, vector<Grain> & grains) const {
	//generate a png time map, ie non-shaded heightmap
		gdImagePtr im = gdImageCreateTrueColor(FIELD, FIELD);
		gdImageFill(im, 0, 0, gdImageColorAllocate(im, 0, 0, 0));

		for(int y = 0; y < FIELD; y++){
			for(int x = 0; x < FIELD; x++){
				if(heights[y][x]){
					int grain = get_grain(x, y, heights[y][x]);
					RGB rgb = RGB::HSV(grains[grain].color, 1.0, 1.0);
					int color = gdImageColorAllocate(im, rgb.r, rgb.g, rgb.b);
					gdImageSetPixel(im, x, y, color);
				}
			}
		}

		char filename[50];
		sprintf(filename, "time.%05d.png", time);
		FILE * fd = fopen(filename, "wb");
		gdImagePng(im, fd);
		fclose(fd);
		gdImageDestroy(im);
	}

	void voroneimap(vector<Grain> & grains) const {
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
					rgb = RGB::HSV(grains[grain].color, 1.0, 1.0);
				int color = gdImageColorAllocate(im, rgb.r, rgb.g, rgb.b);
				gdImageSetPixel(im, x, y, color);
			}
		}

		FILE * fd = fopen("voronei.png", "wb");
		gdImagePng(im, fd);
		fclose(fd);
		gdImageDestroy(im);
	}

	void fix_period(int & x, int & y) const {
			 // use (x + FIELD) % FIELD to wrap values outside the range, making it periodic on x,y
			 // compiler should optimize % to & if FIELD is a power of 2
		x = (x + 256*FIELD) % FIELD; //256 times to guarantee putting it in the positive integer space
		y = (y + 256*FIELD) % FIELD;
	}

	Point * get_point(int x, int y, int z) const {
		fix_period(x, y);
		return planes[z]->get(x, y);
	}

	uint16_t get_grain(int x, int y, int z) const {
		return get_point(x, y, z)->grain;
	}
	uint16_t get_grain(int x, int y, int z, int t) const {
		Point * p = get_point(x, y, z);
		if(p->time < t)
			return p->grain;
		return 0;
	}

	void set_point(int x, int y, int z, Point & p){
		fix_period(x, y);
		planes[z]->set(x, y, p);
	}

	void set_threat(int x, int y, int z, int t){
		fix_period(x, y);
		planes[z]->set_threat(x, y, t);
	}

	void set_diffprob(int x, int y, int z, uint8_t prob){
//		get_point(x, y, z)->diffprob = prob;
	}

	void set_point(int X, int Y, int Z, uint16_t time, uint16_t grain, uint8_t face){
		Point p;
		p.time = time;
		p.grain = grain;
		p.face = face;

		set_point(X, Y, Z, p);

		int curval = heights[Y][X];
		while(curval < Z){
			CASv(heights[Y][X], curval, Z);
			curval = heights[Y][X];
		}

		int Xmin = X - 1, Xmax = X + 1;
		int Ymin = Y - 1, Ymax = Y + 1;
		int Zmin = max(Z - 1, zmin), Zmax = min(Z + 1, zmax - 1);

		for(int z = Zmin; z <= Zmax; z++)
			for(int y = Ymin; y <= Ymax; y++)
				for(int x = Xmin; x <= Xmax; x++)
					if(x != X || y != Y || z != Z)
						set_threat(x, y, z, time);
	}

	//given a point, return a list of nearby grains. Fill the supplied array, returning the number of entries filled.
	uint16_t * check_grain_threats(uint16_t *threats, int X, int Y, int Z) const {
		uint16_t *threatsend = threats;

		int Xmin = X - 1, Xmax = X + 1;
		int Ymin = Y - 1, Ymax = Y + 1;
		int Zmin = max(Z - 1, zmin), Zmax = min(Z + 1, zmax - 1);
		
		for(int z = Zmin; z <= Zmax; z++){
			for(int y = Ymin; y <= Ymax; y++){
				for(int x = Xmin; x <= Xmax; x++){
					if(x == X && y == Y && z == Z)
						continue;

					Point * p = get_point(x, y, z);

					if(p->grain != 0 && p->grain < MAXGRAIN && find(threats, threatsend, p->grain) == threatsend){ //insert only if it isn't found yet
						*threatsend = p->grain;
						++threatsend;
					}
				}
			}
		}
		return threatsend;
	}

	//given a point, return a list of nearby grains and faces. Fill the supplied array, returning the number of entries filled.
	Threat * check_face_threats(Threat *threats, int X, int Y, int Z) const {
		Threat *threatsend = threats;

		int Xmin = X - 1, Xmax = X + 1;
		int Ymin = Y - 1, Ymax = Y + 1;
		int Zmin = max(Z - 1, zmin), Zmax = min(Z + 1, zmax - 1);
		
		for(int z = Zmin; z <= Zmax; z++){
			for(int y = Ymin; y <= Ymax; y++){
				for(int x = Xmin; x <= Xmax; x++){
					if(x == X && y == Y && z == Z)
						continue;

					Point * p = get_point(x, y, z);

					if(p->grain != 0 && p->grain < MAXGRAIN && find(threats, threatsend, p->grain, p->face) == threatsend){
						threatsend->grain = p->grain;
						threatsend->face  = p->face;
						++threatsend;
					}
				}
			}
		}
		return threatsend;
	}
};


