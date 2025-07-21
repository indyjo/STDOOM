#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// 768 bytes
extern unsigned char PLAYPAL[];

float colors[768];

//unsigned char subset[] = {0,      93, 241, 202, 253, 250, 38, 222, 216, 140, 131, 158, 116,     58, 249, 4};
//unsigned char subset[] =   {0, 90, 101, 241, 202,    252,   38,    219,   144, 136, 158, 120, 72, 58, 249, 4};
unsigned char subset[] =   { 0, 88, 102, 205,  184,    6,   42,    217,   144, 136, 159, 121, 72, 56, 249, 4 };
//unsigned char subset[] =   {0, 166, 156, 210};

typedef float vec3[3];

float veclength(vec3 v) {
	return sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
}

void vecdiff(float *r, const float *a, const float *b) {
	for (int i=0; i<3; i++) r[i] = a[i]-b[i];
}

// Finds an optimal approximation of a given color vector using a weighted sum of palette colors.
// weight_left: how many weight units are still to be distruibuted?
// delta: distance vector to target color.
// fixed_palette_weights: number of palette entries whose weights have already been fixed.
// remaining_colors: maximum number of remaining colors to choose from.
// retrace: whether or not to output the result to the console.
float find_best_dist(int weight_left, vec3 delta, const float *target, float accum_dev, int fixed_palette_weights, int remaining_colors, char retrace) {
	if (fixed_palette_weights == sizeof(subset)) {
		if (weight_left > 0) {
			//printf("Error: no more palette weights remaining and weight left: %d\n", weight_left);
			return -1;
		}
		return 2*veclength(delta) + 3*accum_dev;
	}

	int best_weight = 0;
	float best_dist = -1;
	float *colorvec = colors + 3*subset[fixed_palette_weights];

	vec3 color_delta;
	vecdiff(color_delta, target, colorvec);
	// The value this color would contribute to the deviation if chosen.
	float dev_contrib = veclength(color_delta);

	if (remaining_colors > 0) {
		for (int w = weight_left; w >= 0; w--) {
			vec3 remaining_delta;
			for (int c=0; c<3; c++) remaining_delta[c] = delta[c] - w*colorvec[c];

			float dist = find_best_dist(weight_left - w,
				remaining_delta,
				target,
				accum_dev + (w==0?0.0f:dev_contrib),
				fixed_palette_weights + 1,
				remaining_colors - (w==0?0:1),
				0);
			if (dist >= 0 && (best_dist < 0 || dist < best_dist)) {
				best_dist = dist;
				best_weight = w;
			}
		}
	}

	if (retrace) {
		// Retrace best result
		printf(" %d,", best_weight);
		fflush(stdout);
	
		for (int c = 0; c<3; c++) delta[c] -= best_weight * colorvec[c];
		find_best_dist(weight_left - best_weight, delta, target,
			accum_dev + (best_weight==0?0.0f:dev_contrib),
			fixed_palette_weights + 1, remaining_colors - (best_weight==0?0:1), 1);
	}
	return best_dist;
}

// Do not try to mix more colors than this
#define MAX_COLORS 4
// 16 is the right number for a 4x4 Bayer matrix
#define MAX_WEIGHT 16

int main(int argc, const char** argv) {
	// Apply gamma to emphasize the darker parts
	for (int i=0; i<768; i++) {
		colors[i] = powf(PLAYPAL[i] / 256.0f, 1.6f);
	}
	// Convert to YUV in order to emphasize Y (luma) over UV (chroma)
	for (int i=0; i<256; i++) {
		float r = colors[3*i + 0];
		float g = colors[3*i + 1];
		float b = colors[3*i + 2];
		float y = 0.299f*r + 0.587f*g + 0.114f*b;
		float u = 0.492f*(b-y);
		float v = 0.877f*(r-y);
		colors[3*i + 0] = 4*y; // emphasize y
		colors[3*i + 1] = u;
		colors[3*i + 2] = v;
	}
	int begin = 0, end = 256;
	if (argc > 1) begin = atoi(argv[1]);
	if (argc > 2) end = atoi(argv[2]);
	for (int target=begin; target<end; target++) {
		printf("{");
		fflush(stdout);
		vec3 delta;
		const float *color = &colors[3*target];
		for (int c=0; c<3; c++) delta[c]=color[c] * MAX_WEIGHT;
		float residual = find_best_dist(MAX_WEIGHT, delta, color, 0.0f, 0, MAX_COLORS, 1);
		printf("}, // %d: %f\n", target, residual);
	}
	return 0;
}


