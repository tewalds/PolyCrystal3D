
struct HSV;

struct RGB {
	unsigned char r, g, b;
	
	RGB(){
		r = g = b = 0;
	}
	
	RGB(unsigned char R, unsigned char G, unsigned char B){
		r = R;
		g = G;
		b = B;
	}

	RGB(double R, double G, double B){
		r = R*255;
		g = G*255;
		b = B*255;
	}

	RGB(HSV);
};

struct HSV {
	double h, s, v;

	HSV(){
		h = s = v = 0;
	}

	HSV(double H, double S, double V){
		h = H;
		s = S;
		v = V;
	}
	
	HSV(RGB rgb);

	double min3(double a, double b, double c){
		return min(min(a, b), min(a, c));
	}

	double max3(double a, double b, double c){
		return max(max(a, b), max(a, c));
	}
};

RGB::RGB(HSV hsv){
	// HSV Values:Number 0-1
	// RGB Results:Number 0-255

	if(hsv.s == 0){
		r = g = b = hsv.v;
		return;
	}

	double var_H = hsv.h * 6.0;
	int    var_i = floor( var_H );
	double var_1 = hsv.v * ( 1.0 - hsv.s );
	double var_2 = hsv.v * ( 1.0 - hsv.s * ( var_H - (double)var_i ) );
	double var_3 = hsv.v * ( 1.0 - hsv.s * (1.0 - ( var_H - (double)var_i ) ) );

	switch(var_i){
		case 0: r = hsv.v; g = var_3; b = var_1; return;
		case 1: r = var_2; g = hsv.v; b = var_1; return;
		case 2: r = var_1; g = hsv.v; b = var_3; return;
		case 3: r = var_1; g = var_2; b = hsv.v; return;
		case 4: r = var_3; g = var_1; b = hsv.v; return;
		case 5: r = hsv.v; g = var_1; b = var_2; return;
	}

	r = g = b = 0;
	return;
}

HSV::HSV(RGB rgb){
	double r = rgb.r/255.0;
	double g = rgb.g/255.0;
	double b = rgb.b/255.0;

	double rgb_min, rgb_max;
	rgb_min = min3(r, g, b);
	rgb_max = max3(r, g, b);

	v = rgb_max;
	if(v == 0){
		h = s = 0;
		return;
	}

	// Normalize value to 1
	r /= v;
	g /= v;
	b /= v;
	rgb_min = min3(r, g, b);
	rgb_max = max3(r, g, b);

	s = rgb_max - rgb_min;
	if(s == 0){
		h = 0;
		return;
	}

	// Normalize saturation to 1
	r = (r - rgb_min)/(rgb_max - rgb_min);
	g = (g - rgb_min)/(rgb_max - rgb_min);
	b = (b - rgb_min)/(rgb_max - rgb_min);
	rgb_min = min3(r, g, b);
	rgb_max = max3(r, g, b);

	// Compute hue
	if(rgb_max == r){
		h = 0.0 + 60.0/360.0*(g - b);
		if(h < 0.0)
			h += 1.0;
	}else if(rgb_max == g){
		h = 120.0/360.0 + 60.0/360.0*(b - r);
	}else{ // rgb_max == b
		h = 240.0/360.0 + 60.0/360.0*(r - g);
	}
}

