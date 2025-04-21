#include "framework.h"
#include <iostream>
/*


#ifndef PS1

#define PS1

using namespace std;

const char* vertSource = R"(
	#version 330
	precision highp float;

	layout(location = 0) in vec3 vtxPos;
	layout(location = 1) in vec2 vtxUV;

	out vec2 texcoord;

	void main(){
		gl_Position = vec4(vtxPos, 1);
		texcoord = vtxUV;
	}
)";

const char* fragSource = R"(
	#version 330

	uniform sampler2D diffTex;
	uniform float time;
	in vec2 texcoord;
	out vec4 fragColor;

	void main(){
		fragColor = vec4(texture(diffTex, texcoord).rgb, time);
	}
)";

class PlaneTexture {
	unsigned int textureId = 0;
public:
	PlaneTexture(int width, int height) {
		glGenTextures(1, &textureId); // azonosító generálása
		glBindTexture(GL_TEXTURE_2D, textureId);    // ez az aktív innentõl
		// procedurális textúra elõállítása programmal
		const vec3 yellow(1, 1, 0), blue(0, 0, 1);
		std::vector<vec3> image(width * height);
		for (int x = 0; x < width; x++) for (int y = 0; y < height; y++) {
			image[y * width + x] = (x & 1) ^ (y & 1) ? yellow : blue;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &image[0]); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	PlaneTexture(int width, int height, std::vector<vec3>& image) {
		glGenTextures(1, &textureId); // azonosító generálása
		glBindTexture(GL_TEXTURE_2D, textureId);    // ez az aktív innentõl
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &image[0]); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	void Bind(int textureUnit) {
		glActiveTexture(GL_TEXTURE0 + textureUnit); // aktiválás
		glBindTexture(GL_TEXTURE_2D, textureId); // piros nyíl
	}
	~PlaneTexture() {
		if (textureId > 0) glDeleteTextures(1, &textureId);
	}
};

class Plane {
	unsigned int vao;
	unsigned int vbo[2];
	PlaneTexture* texture;
	vector<vec2> vtx;
	vector<vec2> uvs = { vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0,1) };
public:
	Plane() {
		texture = new PlaneTexture(8,8);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(2, &vbo[0]);

		vtx = { vec2(-1, -1),vec2(1, -1),vec2(1,1),vec2(-1,1) };

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec2), &vtx[0], GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), NULL);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), NULL);

	}

	void Animate(float time, GPUProgram* program) {
		program->setUniform(time, "time");

		glBindVertexArray(vao);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec2), &vtx[0], GL_DYNAMIC_DRAW);
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_DYNAMIC_DRAW);
	}

	void Draw(GPUProgram *program) {
		if (vtx.size() <= 0) return;

		glBindVertexArray(vao);

		int samplerUnit = 0;
		program->setUniform(samplerUnit, "diffTex");
		texture->Bind(samplerUnit);

		glDrawArrays(GL_TRIANGLE_FAN, 0, vtx.size());
	}

	~Plane() {
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(2, &vbo[0]);
	}
};

class ParametricSurfaces : public glApp {
	GPUProgram* program;
	Plane* plane;
public:
	ParametricSurfaces() : glApp("Param Surfaces :3") {}

	void onInitialization() {
		program = new GPUProgram(vertSource,fragSource);
		glClearColor(0,0,0,1);
		glEnable(GL_BLEND);
		plane = new Plane();
	}

	void onDisplay() {
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0,0,600,600);
		plane->Draw(program);
	}

	void onTimeElapsed(float startTime, float endTime) {
		plane->Animate((1.f - (float)((int)(endTime * 1000.f) % 1000) / 1000.f), program);
		refreshScreen();
	}
};

ParametricSurfaces app;

#endif // !PS1
*/