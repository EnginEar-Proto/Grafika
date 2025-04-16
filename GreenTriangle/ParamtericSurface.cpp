#include "framework.h"

#ifndef PS1

#define PS1

using namespace std;

struct Material {
	vec3 ka, ks, kd;
	float shine;
};

struct Light {
	vec3 La, Le;
	vec4 wLightPos;
};

const char* vertSource = R"(
	#version 330
	precision highp float;

	uniform mat4 MVP, M, Minv;
	uniform Light lights[8];
	uniform vec3 wEye;
	uniform int nLights;

	layout(location = 0) in vec3 vtxPos;
	layout(location = 1) in vec3 vtxNorm;
	layout(location = 2) in vec2 vtxUV;

	out vec3 wNormal;
	out vec3 wView;
	out vec3 wLight[8];
	out vec2 texcoord;

	void main(){
		gl_Position = MVP * ve4(vtxPos, 1);
		vec4 wPos = M * vec4(vtxPos, 1);

		for(int i = 0; i < nLights; i++){
			vec4 wLpos = lights[i].wLightPos;
			wLight[i] = wLpos.xyz * wPos.w - wPos.xyz * wLpos.w;
		}

		wView = wEye * wPos.w - wPos.xyz;
		wNormal = (vec4(vtxNorm,0) * Minv).xyz;
		texcoord = vtxUV;
	}
)";

const char* fragSource = R"(
	#version 330

	uniform Material material;
	uniform Sampler2D diffTex;
	uniform Light lights[8];
	uniform int nLights;

	in vec3 wNormal;
	in vec3 wView;
	in vec3 wLight[8];
	in vec2 texcoord;

	out vec4 fragColor;

	void main(){
		vec3 N = normalize(wNormal);
		vec3 V = nromalize(wView);
		
		if(dot(N,V) < 0) N = -N;
		
		vec3 texColor = texture(diffTex, texcoord);
		vec3 ka = material.ka * texColor;
		vec3 ks = material.ks;
		vec3 kd = material.kd * texColor;

		float shine = material.shine;
		vec3 radiannce = vec3(0,0,0);

		for(int i = 0; i < nLights; i++){
			vec3 L = nromalize(wLight[i]);
			vec3 H = normalize(L + V);
			float cost = max(dot(N,L), 0), cosd = max(dot(N,H),0);
			vec3 La = lights[i].La, Le = lights[i].Le;
			radiance += ka * La + (kd * cost + ks * powf(cosd, shine)) * Le;
		}

		fragColor = vec4(radiance,1);
	}
)";

struct RenderState {
	mat4 M, Minv, V, P;
	Material* material;
	Texture* texture;
	vector<Light> lights;
	vec3 wEye;
};

class PhongShader : public GPUProgram {
public:
	PhongShader() : GPUProgram(vertSource, fragSource) {}
	void Bind(RenderState state) {
		Use();
		setUniform(state.P * state.V * state.M, "MVP");
		setUniform(state.P , "P");
		setUniform(state.Minv, "Minv");
		setUniform(state.wEye, "wEye");
		setUniform(state.material->ka, "ka");
		setUniform(state.material->ks, "ks");
		setUniform(state.material->kd, "kd");
		setUniform(state.material->shine, "shine");

		if (state.texture != nullptr) {
			int textureUnit = 0;
			setUniform(textureUnit, "diffTex");
			state.texture->Bind(textureUnit);
		}

		setUniform((int)state.lights.size(), "nLights");

		for (int i = 0; i < (int)state.lights.size(); i++) {
			string name = string("lights[") + to_string(i) + string("]");
			setUniform(state.lights[i].La, name + ".La");
			setUniform(state.lights[i].Le, name + ".Le");
			setUniform(state.lights[i].wLightPos, name + ".wLightPos");
		}
	}
};

struct Camera {
	vec3 wEye, wLookat, wVup;
	float bp, fp, fov, asp;
public:
	Camera() {
		asp = 1;
		fov = 40.f * (float)M_PI / 180.f;
		fp = 1; bp = 20; // Elsp és hátsó vágósíkok távolsága a szemtől.
	}

	mat4 V() { return lookAt(wEye, wLookat, wVup); }
	mat4 P() { return perspective(fov, asp, fp, bp); }
};

struct VtxData {
	vec3 pos, normal;
	vec2 texcoord;
};

class ParamSurface : public Geometry<VtxData> {
	int nVtxInStrip, nStrips;
public:
	ParamSurface() {
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		int nb = sizeof(VtxData);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, nb, (void*)offsetof(VtxData, pos));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, nb, (void*)offsetof(VtxData, normal));
		glVertexAttribPointer(2,2, GL_FLOAT, GL_FALSE, nb, (void*)offsetof(VtxData, texcoord));
	}

	virtual VtxData GenVertexData(float u, float v) = 0;

	void create(int M = 20, int N = 20) {
		nVtxInStrip = (M + 1) * 2;
		nStrips = N;

		for (int i = 0; i < N; i++) {
			for (int j = 0; j < M; i++) {
				vtx.push_back(GenVertexData((float)j/M, (float)i/N));
				vtx.push_back(GenVertexData((float)j / M, (float)(i+1) / N));
			}
		}
		updateGPU();
	}

	void Draw() {
		Bind();

		for (int i = 0; i < nStrips; i++) {
			glDrawArrays(GL_TRIANGLE_STRIP, i * nVtxInStrip, nVtxInStrip);
		}

	}
};

class Sphere : public ParamSurface {
public:
	Sphere() { create(); }

	VtxData GenVertexData(float u, float v) {
		VtxData vd;
		float U = (u - .5f) * M_PI, V = v * 2.f * M_PI;
		vd.pos = vec3(cosf(U) * cosf(V), cosf(U) * sinf(V), sinf(U));
		vec3 drdu = vec3(-sinf(U) * cosf(V), -sinf(U) * sinf(V), cos(U));
		vec3 drdv = vec3(-sinf(V) * cosf(U), cosf(U) * cosf(V), 0);
		vd.normal = cross(drdu, drdv);
		vd.texcoord = vec2(u, v);

		return vd;
	}
};

struct Object {
	Material* material;
	Texture* texture;
	ParamSurface* geometry;
	float rotAngle;
	vec3 scaling, translation, rotAxis;
public:
	void Draw(PhongShader* shader, RenderState state) {
		state.M = translate(translation) *
			rotate(rotAngle, rotAxis) *
			scale(scaling);

		state.Minv = scale(1.f / scaling) *
			rotate(-rotAngle, rotAxis) *
			translate(- translation);

		state.material = material;
		state.texture = texture;
		shader->Bind(state);
		geometry->Draw();
	}
};

class Scene {
	vector<Object> objects;
	Camera camera;
	vector<Light> lights;
public:
	Scene() {
		Material* material0 = new Material;
		material0->kd = vec3(0.6f, 0.4f, 0.2f);
		material0->ks = vec3(4, 4, 4);
		material0->ka = vec3(0.1f, 0.1f, 0.1f);
		material0->shine = 100;

		Texture* texture15x20 = new Texture(15,20);

		objects = {
			{material0, texture15x20, new Sphere(), .7f}
		};
	
		camera.wEye = vec3(0,0,14);
		camera.wLookat = vec3(0,0,0);
		camera.wVup = vec3(0,1,0);

		lights.resize(1);
		lights[0].wLightPos = vec4(5, 4, 5, 0);
		lights[0].La = vec3(0.1f, 0.1f, 1);
		lights[0].Le = vec3(3,0,0);
	}

	void Render(PhongShader* shader) {
		RenderState state;
		state.wEye = camera.wEye;
		state.V = camera.V();
		state.P = camera.P();
		state.lights = lights;
		for (Object& obj : objects) {
			obj.Draw(shader, state);
		}
	}
};

class ParametricSurfaces : public glApp {
	Scene* scene;
	PhongShader* shader;
public:
	ParametricSurfaces() : glApp("Param Surfaces :3") {}

	void onInitialization() {
		glClearColor(0,0,0,1);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		scene = new Scene;
		shader = new PhongShader;
	}

	void onDisplay() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0,0,600,600);
		scene->Render(shader);
	}
};

ParametricSurfaces app;

#endif // !PS1
