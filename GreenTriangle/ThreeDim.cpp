#include "framework.h"

#ifndef ColumnThreeD

#define ColumnThreeD

using namespace std;

const char* vertSource = R"(
	#version 330
	precision highp float;

	uniform mat4 view;
	uniform mat4 projection;
	uniform mat4 model;
	out vec2 texcoord;
	out vec3 Normal; 
	out vec3 fragPos;

	layout(location = 0) in vec3 cP;
	layout(location = 1) in vec2 vtxUV;
	layout(location = 2) in vec3 cNorm;

	void main(){
		gl_Position = projection * view * model * vec4(cP, 1.f);
		texcoord = vtxUV;
		Normal = cNorm;
		fragPos = vec3((model * vec4(cP, 1.f)).xyz);
	}
)";

const char* fragSource = R"(
	#version 330

	uniform sampler2D sampler;
	uniform vec3 objectColor;
	uniform bool useColor;
	uniform vec3 lightColor;
	uniform vec3 lightPos;

	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor;

	in vec2 texcoord;
	in vec3 Normal;
	in vec3 fragPos;
	out vec4 fragmentColor;

	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - fragPos);

	float diff = max(dot(norm, lightDir),0.f); //cos+
	vec3 diffuse = diff * lightColor;
	vec3 result = ambient + diffuse;

	void main(){
		if(useColor){
			fragmentColor = vec4(result * objectColor,1);
		}else{
			fragmentColor = texture(sampler, texcoord) * vec4(result, 1);
		}
	}
)";

class Camera {
	vec3 camPos, target, direction;
public:
	Camera() {
		camPos = vec3(-5.f,-5.f,10);
		target = vec3(0.5,0,1.f);
		direction = normalize(camPos - target);
	}

	mat4 V() { return lookAt(camPos, target, vec3(0,0,1)); }
	mat4 P() { return perspective(radians(45.f), 1.f, 0.1f, 100.f); }

	mat4 Vinv() {}
	mat4 Pinv() {}
};

Camera *camera = new Camera;

class Intersectable{
public:
	unsigned int vao, vbo[2];
	vector<vec3> vtx;
	vector<vec3> norms;

	Intersectable() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(2, vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), NULL);


		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), NULL);
	}

	void updateGPU() {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec3), &vtx[0], GL_DYNAMIC_DRAW);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, norms.size() * sizeof(vec3), &norms[0], GL_DYNAMIC_DRAW);
	}

	virtual void create(float u, float v) = 0;
	virtual void Draw(GPUProgram *program) = 0;
};

class Plane: Intersectable {
	int width = 20, height = 20;
	unsigned int textureId = 0;
	unsigned int texVbo;
	Texture* texture;
public:
	Plane(): Intersectable() {
		create(width, height);

		vtx = { vec3(-10, -10, -1),vec3(10, -10, -1),vec3(10,10,-1),vec3(-10,10,-1) };
		
		for (int i = 0; i < (int)vtx.size(); i++) {
			norms.push_back(vec3(0,0,1));
		}

		glGenBuffers(1, &texVbo);
		glBindBuffer(GL_ARRAY_BUFFER, texVbo);
		std::vector<vec2> uvs = { vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0,1) };
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), NULL);

		updateGPU();
	}

	void create(float u, float v) {
		glGenTextures(1, &textureId); // azonosító generálása
		glBindTexture(GL_TEXTURE_2D, textureId);    // ez az aktív innentõl
		// procedurális textúra elõállítása programmal
		const vec3 white(1, 1, 1), blue(0, 0, 1);
		vector<vec3> image(u*v);

		for (int x = 0; x < u; x++) for (int y = 0; y < v; y++) {
			image[y * u + x] = (x & 1) ^ (y & 1) ? white : blue;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &image[0]); // To GPU
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	void Bind(int textureUnit) {
		glActiveTexture(GL_TEXTURE0 + textureUnit); // aktiválás
		glBindTexture(GL_TEXTURE_2D, textureId); // piros nyíl

		updateGPU();
	}

	void Draw(GPUProgram* program) {
		int sampler = 0;

		program->setUniform(false, "useColor");
		program->setUniform(sampler, "sampler");
		program->setUniform(camera->P(), "projection");
		program->setUniform(camera->V(), "view");
		program->setUniform(mat4(1.f), "model");

		this->Bind(sampler);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	~Plane() {
		if (textureId > 0) glDeleteTextures(1, &textureId);
	}
};

class Cylinder: Intersectable {
	vec3 vDir;
public:
	Cylinder(): Intersectable() {
		vDir = vec3(0,0,1);
	}

	void create(float u, float v) {
		for (int j = 0; j < 34; j++) {
			vec3 vert;
			if (j % 2 == 0) {
				vert = vec3(u * cosf((float)j * (M_PI / 16.f)), u * sinf((float)j * (M_PI / 16.f)), 0);
			}
			else {
				vert = vec3(u * cosf((float)j * (M_PI / 16.f)), u * sinf((float)j * (M_PI / 16.f)), v);
			}
			vtx.push_back(vert);
			norms.push_back(vert);
		}

		updateGPU();
	}

	void Draw(GPUProgram *program) {
		if (vtx.size() <= 0) return;

		mat4 M = translate(vec3(5.f,0,-0.5f)) * rotate(radians(30.f), vec3(0,1,1));

		glBindVertexArray(vao);
		program->setUniform(true,"useColor");
		program->setUniform(vec3(.2,0.3,0.1), "objectColor");
		program->setUniform(camera->V(), "view");
		program->setUniform(camera->P(), "projection");
		program->setUniform(M, "model");
		glDrawArrays(GL_TRIANGLE_STRIP,0,vtx.size());
	}
};

class Cube : Intersectable {
public:
	Cube() : Intersectable() {
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

		norms = { cross(vtx[1], vtx[2]), cross(vtx[5], vtx[6]), cross(vtx[9], vtx[10]), cross(vtx[13], vtx[14]), cross(vtx[17], vtx[18]), cross(vtx[21], vtx[22])};

		updateGPU();
	}

	void create(float u, float v) {
		return;
	}

	void Draw(GPUProgram *program) {
		if (vtx.size() <= 0) return;

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec3), &vtx[0], GL_DYNAMIC_DRAW);

		program->setUniform(camera->P(), "projection");
		program->setUniform(camera->V(), "view");
		program->setUniform(mat4(1.f), "model");
		program->setUniform(true, "useColor");
		program->setUniform(vec3(0.2,0.4,0.7), "objectColor");
		glDrawArrays(GL_TRIANGLE_STRIP, 0, vtx.size());
	}
};

class LightCube: Intersectable {
	vec3 ligthPos;
public:
	LightCube() : Intersectable() {
		ligthPos = vec3(1.2f, 1.f, 2.f);
		create(0,0);
		updateGPU();
	}

	void create(float u, float v) {
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

		norms = { cross(vtx[1], vtx[2]), cross(vtx[5], vtx[6]), cross(vtx[9], vtx[10]), cross(vtx[13], vtx[14]), cross(vtx[17], vtx[18]), cross(vtx[21], vtx[22]) };
	}

	void Draw(GPUProgram *program) {
		if (vtx.size() <= 0) return;

		program->setUniform(vec3(1, 1, 1), "lightColor");
		program->setUniform(vec3(1, 1, 1), "objectColor");
		program->setUniform(true, "useColor");
		program->setUniform(ligthPos, "lightPos");

		const char* lightShader = R"(
			#version 330

			out vec4 fragmentColor;

			void main(){
				fragmentColor = vec4(1,1,1,1);
			}
		)";

		GPUProgram* ligthProgram = new GPUProgram(vertSource, lightShader);
		ligthProgram->Use();

		program->setUniform(camera->P(), "projection");
		program->setUniform(camera->V(), "view");
		program->setUniform(translate(mat4(1.f), ligthPos), "model");

		updateGPU();

		glDrawArrays(GL_TRIANGLE_STRIP, 0, vtx.size());
	}
};

class ThreeDim : public glApp {
	GPUProgram *program;
	Cube* cube;
	Plane* plane;
	Cylinder *c;
	LightCube* lSource;
public:
	ThreeDim() : glApp(":3 Dimension") {
	
	}

	void onInitialization() {
		glClearColor(0.89, 0.95, 1, 1);
		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		program = new GPUProgram(vertSource, fragSource);
		plane = new Plane();
		c = new Cylinder();
		c->create(.5f, 3.f);
		lSource = new LightCube();
	}

	void onDisplay() {
		glViewport(0,0,600,600);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		lSource->Draw(program);
		program->Use();
		c->Draw(program);
		plane->Draw(program);
	}
};

ThreeDim Yiff;

#endif // !ColumnThreeD
