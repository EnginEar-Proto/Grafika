#include "framework.h"

#ifndef ColumnThreeD

#define ColumnThreeD

using namespace std;

const char* vertSource = R"(
	#version 330
	precision highp float;

	uniform mat4 MVP;
	out vec2 texcoord;

	layout(location = 0) in vec3 cP;
	layout(location = 1) in vec2 vtxUV;

	void main(){
		gl_Position = MVP * vec4(cP, 1);
		texcoord = vtxUV;
	}
)";

const char* fragSource = R"(
	#version 330

	uniform sampler2D sampler;
	uniform vec3 color;
	uniform bool useColor;

	in vec2 texcoord;
	out vec4 fragmentColor;

	void main(){
		if(useColor){
			fragmentColor = vec4(color,1);
		}else{
			fragmentColor = texture(sampler, texcoord);
		}
	}
)";

class Camera {
	vec3 camPos, target, direction;
public:
	Camera() {
		camPos = vec3(2.f,-2.f,2.5);
		target = vec3(5.f,5,2.f);
		direction = normalize(camPos - target);
	}

	mat4 V() { return lookAt(camPos, target, vec3(0,0,1)); }
	mat4 P() { return perspective(radians(45.f), 1.f, 0.1f, 100.f); }

	mat4 Vinv() {}
	mat4 Pinv() {}
};

Camera *camera = new Camera;

class Plane {
	int width = 20, height = 20;
	unsigned int textureId = 0;
	unsigned int vao, vbo[2];
	vector<vec3> vtx;
	Texture* texture;
public:
	Plane() {
		glGenTextures(1, &textureId); // azonosító generálása
		glBindTexture(GL_TEXTURE_2D, textureId);    // ez az aktív innentõl
		// procedurális textúra elõállítása programmal
		const vec3 white(1, 1, 1), blue(0, 0, 1);
		std::vector<vec3> image(width * height);

		for (int x = 0; x < width; x++) for (int y = 0; y < height; y++) {
			image[y * width + x] = (x & 1) ^ (y & 1) ? white : blue;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &image[0]); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(2, &vbo[0]);

		vtx = { vec3(-10, -10, -1),vec3(10, -10, -1),vec3(10,10,-1),vec3(-10,10,-1) };

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec3), &vtx[0], GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), NULL);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		std::vector<vec2> uvs = { vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0,1) };
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), NULL);
	}

	void Bind(int textureUnit) {
		glActiveTexture(GL_TEXTURE0 + textureUnit); // aktiválás
		glBindTexture(GL_TEXTURE_2D, textureId); // piros nyíl
	}

	void Draw(GPUProgram* program) {
		int sampler = 0;

		program->setUniform(false, "useColor");
		program->setUniform(sampler, "sampler");
		program->setUniform(camera->P() * camera->V(), "MVP");

		this->Bind(sampler);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	~Plane() {
		if (textureId > 0) glDeleteTextures(1, &textureId);
	}
};

class Cylinder {
	unsigned int vao, vbo;
	vector<vec3> vtx;
public:
	Cylinder() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0,3,GL_FLOAT, GL_FALSE, sizeof(vec3), NULL);
	}

	void create(float u, float v) {
		for (int i = 0; i < 32; i++) {
			for (int j = 0; j < v; j++) {
				vtx.push_back(vec3(u * cosf((float)i * (M_PI / 32.f)), u * sinf((float)i * (M_PI / 32.f)), (float)j));
			}
		}

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER,vtx.size() * sizeof(vec3), &vtx[0], GL_DYNAMIC_DRAW);
	}

	void Draw(GPUProgram *program) {
		if (vtx.size() <= 0) return;

		mat4 M = translate(vec3(5.f,5.f,0));
		
		glBindVertexArray(vao);
		program->setUniform(true,"useColor");
		program->setUniform(vec3(.9,0.3,0.1), "color");
		program->setUniform(camera->P() * camera->V() * M, "MVP");
		glDrawArrays(GL_TRIANGLE_STRIP,0,vtx.size());
	}
};

class Cube {
	unsigned int vao, vbo;
	vector<vec3> vtx;
public:
	Cube() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0,3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

		vtx = {
			vec3(0.5f,  0.5f, 0.0f),
			vec3(0.5f, -0.5f, 0.0f),
			vec3(-0.5f,  0.5f, 0.0f),
			vec3(-0.5f, -0.5f, 0.0f),

			vec3(-0.5f,  -0.5f, 0.0f),
			vec3(-0.5f, 0.5f, 0.0f),
			vec3(-0.5f,  0.5f, 1.0f),
			vec3(-0.5f, -0.5f, 1.0f),

			vec3(-0.5f,  0.5f, 0.0f),
			vec3(0.5f, 0.5f, 0.0f),
			vec3(-0.5f,  0.5f, 1.0f),
			vec3(0.5f, 0.5f, 1.0f),

			vec3(0.5f,  -0.5f, 0.0f),
			vec3(0.5f, 0.5f, 0.0f),
			vec3(0.5f,  -0.5f, 1.0f),
			vec3(0.5f, 0.5f, 1.0f),

			vec3(-0.5f,  -0.5f, 0.0f),
			vec3(0.5f, -0.5f, 0.0f),
			vec3(-0.5f, -0.5f, 1.0f),
			vec3(0.5f,  -0.5f, 1.0f),

			vec3(0.5f,  0.5f, 1.0f),
			vec3(0.5f, -0.5f, 1.0f),
			vec3(-0.5f,  0.5f, 1.0f),
			vec3(-0.5f, -0.5f, 1.0f),
		};

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec3), &vtx[0], GL_DYNAMIC_DRAW);
	}

	void Draw(GPUProgram *program) {
		if (vtx.size() <= 0) return;

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec3), &vtx[0], GL_DYNAMIC_DRAW);

		program->setUniform(camera->P() * camera->V(), "MVP");
		program->setUniform(true, "useColor");
		program->setUniform(vec3(0.2,0.4,0.7), "color");
		glDrawArrays(GL_TRIANGLE_STRIP, 0, vtx.size());
	}
};

class ThreeDim : public glApp {
	GPUProgram *program;
	Cube* cube;
	Plane* plane;
	Cylinder *c;
public:
	ThreeDim() : glApp(":3 Dimension") {
	
	}

	void onInitialization() {
		glClearColor(0.89, 0.95, 1, 1);
		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		program = new GPUProgram(vertSource, fragSource);
		cube = new Cube();
		plane = new Plane();
		c = new Cylinder();
		c->create(.5f, 3.f);
	}

	void onDisplay() {
		glViewport(0,0,600,600);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		cube->Draw(program);
		c->Draw(program);
		plane->Draw(program);
	}
};

ThreeDim Yiff;

#endif // !ColumnThreeD
