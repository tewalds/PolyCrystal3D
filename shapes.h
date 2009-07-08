
const double sqrt2 = sqrt(2);
const double sqrt3 = sqrt(3);
const double gr = (1.0 + sqrt(5))/2.0;
const double gr2 = gr*gr;

struct Shape{
	const char * name;
	int    num_faces;
	double faces[50 * 5]; //must be big enough to hold the shape with the most faces, A, B, C, D -> (0,1], P -> [0,1]
};

Shape Tetrahedron = {
	"Tetrahedron", 4,
	{
		 1, 1, 1, 1/sqrt3, 1,
		 1,-1,-1, 1/sqrt3, 1,
		-1, 1,-1, 1/sqrt3, 1,
		-1,-1, 1, 1/sqrt3, 1,
	}
};

Shape Twin_tetrahedron = {
	"Twin Tetrahedron", 7,
	{
		2.0*sqrt2/3*cos(0.0*M_PI/3), 2.0*sqrt2/3*sin(0.0*M_PI/3), 1.0/3, 2*sqrt2/3, 1,
		2.0*sqrt2/3*cos(1.0*M_PI/3), 2.0*sqrt2/3*sin(1.0*M_PI/3), 1.0/3, 2*sqrt2/3, 1,
		2.0*sqrt2/3*cos(2.0*M_PI/3), 2.0*sqrt2/3*sin(2.0*M_PI/3), 1.0/3, 2*sqrt2/3, 1,
		2.0*sqrt2/3*cos(3.0*M_PI/3), 2.0*sqrt2/3*sin(3.0*M_PI/3), 1.0/3, 2*sqrt2/3, 1,
		2.0*sqrt2/3*cos(4.0*M_PI/3), 2.0*sqrt2/3*sin(4.0*M_PI/3), 1.0/3, 2*sqrt2/3, 1,
		2.0*sqrt2/3*cos(5.0*M_PI/3), 2.0*sqrt2/3*sin(5.0*M_PI/3), 1.0/3, 2*sqrt2/3, 1,
		0, 0, -1, 1, 1	
	}
};

Shape Cube = {
	"Cube", 6,
	{
		 1, 0, 0, 1, 1,
		 0, 1, 0, 1, 1,
		 0, 0, 1, 1, 1,
		-1, 0, 0, 1, 1,
		 0,-1, 0, 1, 1,
		 0, 0,-1, 1, 1,
	 }
};

Shape Stretched_cube = {
	"Stretched Cube", 6,
	{
		 1, 0, 0, 0.2, 1,
		 0, 1, 0, 0.2, 1,
		 0, 0, 1, 1.0, 1,
		-1, 0, 0, 0.2, 1,
		 0,-1, 0, 0.2, 1,
		 0, 0,-1, 1.0, 1,
	}
};

Shape Octahedron = {
	"Octahedron", 8,
	{
		 1, 1, 1, 1/sqrt3, 1,
		 1, 1,-1, 1/sqrt3, 1,
		 1,-1, 1, 1/sqrt3, 1,
		 1,-1,-1, 1/sqrt3, 1,
		-1, 1, 1, 1/sqrt3, 1,
		-1, 1,-1, 1/sqrt3, 1,
		-1,-1, 1, 1/sqrt3, 1,
		-1,-1,-1, 1/sqrt3, 1,
	}
};

Shape Stretched_hex = {
	"Stretched Hex", 8,
	{
		 0.5, sqrt3/2, 0, 0.2, 1,
		 0.5,-sqrt3/2, 0, 0.2, 1,
		-0.5, sqrt3/2, 0, 0.2, 1,
		-0.5,-sqrt3/2, 0, 0.2, 1,
		  -1,       0, 0, 0.2, 1,
		   0,      -1, 0, 0.2, 1,
		 0, 0, 1, 1, 1,
		 0, 0,-1, 1, 1,
	}
};

Shape Rhombic_dodecahedron = {
	"Rhombic Dodecahedron", 12,
	{
		 1, 1, 0, 1/sqrt2, 1,
		 1,-1, 0, 1/sqrt2, 1,
		-1, 1, 0, 1/sqrt2, 1,
		-1,-1, 0, 1/sqrt2, 1,
		 1, 0, 1, 1/sqrt2, 1,
		 1, 0,-1, 1/sqrt2, 1,
		-1, 0, 1, 1/sqrt2, 1,
		-1, 0,-1, 1/sqrt2, 1,
		 0, 1, 1, 1/sqrt2, 1,
		 0, 1,-1, 1/sqrt2, 1,
		 0,-1, 1, 1/sqrt2, 1,
		 0,-1,-1, 1/sqrt2, 1,
	}
};

Shape Cuboctahedron = {
	"Cuboctahedron", 14,
	{
		 1, 1, 1, 1/sqrt3, 1,
		 1, 1,-1, 1/sqrt3, 1,
		 1,-1, 1, 1/sqrt3, 1,
		 1,-1,-1, 1/sqrt3, 1,
		-1, 1, 1, 1/sqrt3, 1,
		-1, 1,-1, 1/sqrt3, 1,
		-1,-1, 1, 1/sqrt3, 1,
		-1,-1,-1, 1/sqrt3, 1,
		 1, 0, 0, 1, 1,
		 0, 1, 0, 1, 1,
		 0, 0, 1, 1, 1,
		-1, 0, 0, 1, 1,
		 0,-1, 0, 1, 1,
		 0, 0,-1, 1, 1,
	}
};

Shape Dodecahedron = {
	"Dodecahedron", 12,
	{
		  0,  1, gr, 1/sqrt(1 + gr2), 1,
		  0,  1,-gr, 1/sqrt(1 + gr2), 1,
		  0, -1, gr, 1/sqrt(1 + gr2), 1,
		  0, -1,-gr, 1/sqrt(1 + gr2), 1,
		  1, gr,  0, 1/sqrt(1 + gr2), 1,
		  1,-gr,  0, 1/sqrt(1 + gr2), 1,
		 -1, gr,  0, 1/sqrt(1 + gr2), 1,
		 -1,-gr,  0, 1/sqrt(1 + gr2), 1,
		 gr,  0,  1, 1/sqrt(1 + gr2), 1,
		 gr,  0, -1, 1/sqrt(1 + gr2), 1,
		-gr,  0,  1, 1/sqrt(1 + gr2), 1,
		-gr,  0, -1, 1/sqrt(1 + gr2), 1,
	}
};

Shape Icosahedron = {
	"Icosahedron", 20,
	{
		 1, 1, 1, 1/sqrt3, 1,
		 1, 1,-1, 1/sqrt3, 1,
		 1,-1, 1, 1/sqrt3, 1,
		 1,-1,-1, 1/sqrt3, 1,
		-1, 1, 1, 1/sqrt3, 1,
		-1, 1,-1, 1/sqrt3, 1,
		-1,-1, 1, 1/sqrt3, 1,
		-1,-1,-1, 1/sqrt3, 1,
			0,  1/gr,    gr, 1/sqrt(gr2 + 1/gr2), 1,
			0,  1/gr,   -gr, 1/sqrt(gr2 + 1/gr2), 1,
			0, -1/gr,    gr, 1/sqrt(gr2 + 1/gr2), 1,
			0, -1/gr,   -gr, 1/sqrt(gr2 + 1/gr2), 1,
		 1/gr,    gr,     0, 1/sqrt(gr2 + 1/gr2), 1,
		 1/gr,   -gr,     0, 1/sqrt(gr2 + 1/gr2), 1,
		-1/gr,    gr,     0, 1/sqrt(gr2 + 1/gr2), 1,
		-1/gr,   -gr,     0, 1/sqrt(gr2 + 1/gr2), 1,
		   gr,     0,  1/gr, 1/sqrt(gr2 + 1/gr2), 1,
		   gr,     0, -1/gr, 1/sqrt(gr2 + 1/gr2), 1,
		  -gr,     0,  1/gr, 1/sqrt(gr2 + 1/gr2), 1,
		  -gr,     0, -1/gr, 1/sqrt(gr2 + 1/gr2), 1,
	}
};

