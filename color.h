
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
	
	static RGB HSV(double H, double S, double V){
		// HSV Values:Number 0-1
		// RGB Results:Number 0-255

		if(S == 0)
			return RGB(V, V, V);

		double var_H = H * 6.0;
		int    var_i = floor( var_H );
		double var_1 = V * ( 1.0 - S );
		double var_2 = V * ( 1.0 - S * ( var_H - (double)var_i ) );
		double var_3 = V * ( 1.0 - S * (1.0 - ( var_H - (double)var_i ) ) );

		switch(var_i){
			case 0: return RGB(V,     var_3, var_1);
			case 1: return RGB(var_2, V,     var_1);
			case 2: return RGB(var_1, V,     var_3);
			case 3: return RGB(var_1, var_2, V    );
			case 4: return RGB(var_3, var_1, V    );
			case 5: return RGB(V,     var_1, var_2);
		}

		return RGB();
	}
};


