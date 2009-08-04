
struct FaceDist {
	int face;
	double dist;
	
	FaceDist(){
	}
	
	FaceDist(int f, double d){
		face = f;
		dist = d;
	}
};

struct Face {
	double A;
	double B;
	double C;
	double D;  // speed of growth
	double F;  // flux
	double dF; // amount of flux received last timestep
	double K;  // K = D*F, minor optimization
	double P;  // probability of diffusion from this face
	
	int flux;
	int threats;
	int growth;

	RGB rgb;

	Face(double a, double b, double c, double d, double f, double p){
		A = a;
		B = b;
		C = c;
		D = d;
		F = f;
		dF = 0;
		K = D*F;
		P = p;
		
		flux = 0;
		threats = 0;
	}

	void output() const {
		printf("\t%.3f*x + %.3f*y + %.3f*z + %.3f*f = 0, f = %.3f, K = %.3f\n", A, B, C, D, F, K);
	}

	void dump(FILE * fd) const {
		fprintf(fd, "%.10f,%.10f,%.10f,%.10f,%.10f,%.10f,%d,%d\n", A, B, C, D, F, dF, flux, threats);
	}

	double fluxamnt(){
		double amnt = 0;
		if(flux && threats)
			amnt = (fabs(A) + fabs(B) + fabs(C)) * flux / threats;

		return amnt;
	}

	void grow(double amnt){
		dF = amnt;
		F += amnt;
		
		K = D*F;
	}

	void calc_color(double color){
		double slope = atan(A/C)*2/M_PI;
		if(slope < 0) rgb = RGB(HSV(color, slope + 1.0, 1.0));
		else          rgb = RGB(HSV(color, 1.0, 1.0 - slope));
	}
	
	void rotate(double t1, double t2, double phi){
		double a, b, c;
		
		a = cos(t1)*A + sin(t1)*B;
		b = -sin(t1)*A + cos(t1)*B;
		
		A = a; B = b;
		
		b = cos(phi)*B + sin(phi)*C;
		c = -sin(phi)*B + cos(phi)*C;
		
		B = b; C = c;
		
		a = cos(t2)*A + sin(t2)*B;
		b = -sin(t2)*A + cos(t2)*B;
		
		A = a; B = b;
	}
	
	bool under(int x, int y, int z) const {
		return (A*x + B*y + C*z < K);
	}
	
	double rel_dist(int x, int y, int z) const {
		double f = (A*x + B*y + C*z)/D;
		return (f - (F - dF))/dF;
	}

	double abs_dist(int x, int y, int z) const {
		return K - (A*x + B*y + C*z);
	}
};

class Grain {
public:
	vector<Face> faces;
	int x, y, z;
	double theta1, theta2, phi;

	double color;
	
	int size;
	int growth;
	int threats;

	Grain(){
		x = y = z = 0;
		theta1 = theta2 = phi = 0;

		size = 1;
		growth = 0;
		threats = 17;
		color = (double)rand()/RAND_MAX;
	}

	void set_color(double c){
		color = c;
		fix_face_colors();
	}

	void fix_face_colors(){
		for(vector<Face>::iterator fit = faces.begin(); fit != faces.end(); ++fit)
			fit->calc_color(color);
	}

	void directional_color(){
		double vec[3];
		vec[0] = fabs(sin(phi)*sin(theta1));
		vec[1] = fabs(cos(theta1)*sin(phi));
		vec[2] = fabs(cos(phi));

		sort(vec, vec+3);

		vec[0] /= vec[2];
		vec[1] /= vec[2];
		vec[2] /= vec[2];

		HSV hsv = HSV(RGB(vec[0], vec[1] - vec[0], vec[2] - vec[1]));
		color = hsv.h;

		fix_face_colors();
	}

	void add_faces(const double *faces, int num_faces){
		for(int i = 0; i < num_faces*5; i += 5)
			add_face(faces[i+0], faces[i+1], faces[i+2], faces[i+3], faces[i+4]);
	}
	void add_face(double a, double b, double c, double d, double p){
		add_face(Face(a, b, c, d, 0, p));
	}
	void add_face(Face face){
		face.calc_color(color);
		faces.push_back(face);
	}

	void output(int i){
		printf("Grain %d, %d faces, size %d, centered at %d,%d\n", i, (int)faces.size(), size, x, y);
		for(vector<Face>::iterator fit = faces.begin(); fit != faces.end(); ++fit)
			fit->output();
	}

	void dump(FILE * fd, int i){
		fprintf(fd, "%d,%d,%d,%f,%f,%f\n", i, x, y, theta1, theta2, phi);
	}
	
	void load(FILE * fd, const double *faces, int num_faces){
		int i;
		if(fscanf(fd, "%d,%d,%d,%lf,%lf,%lf\n", &i, &x, &y, &theta1, &theta2, &phi));
		add_faces(faces, num_faces);
		rotate(theta1, theta2, phi);
	}

	void grow_faces(double amnt){
		for(vector<Face>::iterator fit = faces.begin(); fit != faces.end(); ++fit)
			fit->grow(amnt);
	}

	void rotate(double t1, double t2, double p){
		theta1 = t1;
		theta2 = t2;
		phi	= p;

		for(vector<Face>::iterator fit = faces.begin(); fit != faces.end(); ++fit)
			fit->rotate(t1, t2, p);

		fix_face_colors();
	}

	RGB get_face_color(int face) const {
		return faces[face].rgb;
	}

	void fix_period(int & X, int & Y) const {
		X = ((X - x + 255*FIELD/2) % FIELD) - FIELD/2;
		Y = ((Y - y + 255*FIELD/2) % FIELD) - FIELD/2;
	}

	//return the face that is abs closest, but the relative growth distance of the face that took it
	//between grains, use the relative distance, giving it to the grain that swallowed it first
	//between faces, use the absolute distance, since there may be faces that swallowed it several timesteps ago
	FaceDist find_distance(int X, int Y, int Z) const {
		fix_period(X, Y);

		double absdist = FIELD;
		double reldist = FIELD;
		int    absface = 0;
		int    relface = 0;

		double dist;
		for(unsigned int i = 0; i < faces.size(); i++){
			dist = faces[i].rel_dist(X, Y, Z);
			if(dist < reldist){
				reldist = dist;
				relface = i;
			}

			dist = faces[i].abs_dist(X, Y, Z);
			if(dist < absdist){
				absdist = dist;
				absface = i;
			}
		}

		return FaceDist(absface, reldist);
	}

	//check if a point in space is within this grain	
	bool check_point(int X, int Y, int Z) const {
		fix_period(X, Y);

		for(vector<Face>::const_iterator fit = faces.begin(); fit != faces.end(); ++fit)
			if(!fit->under(X, Y, Z))
				return false;

		return true;
	}
};

