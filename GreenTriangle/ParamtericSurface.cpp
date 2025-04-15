#include "framework.h";

using namespace std;


struct Material {
	vec3 kd, ks, ka;
	float shininess;
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
	uniform int nLights;
	uniform vec3 wEye;

	layout(location = 0) in vec3 vtxPos;
	layout(locaiton = 1) in vec3 vtxNorm;
	layout(location = 2) in vec3 vtxUV;

	out vec3 wNormal;
	out vec3 wView;
	out vec3 wLights[8];
	out vec2 texCoord;

	void main(){
		gl_Position = MVP * vec4(vtxPos, 1);

		vec4 wPos = M * vec4(vtxPos, 1);
		for(int i = 0; i < nLights; i++){
			vec4 wLpos = lights[i].wLightPos;
			wLight[i] = wLpos.xyz * wPos.w - wPos.xyz * wLpos.w;
		}

		wView = wEye * wPos.w - wPos.xyz;
		wNormal = (vec4(vtxNorm, 0) * Minv).xyz;
		texCoord = vtxUV;
	}
)";

const char* fragSource = R"(
	#version 330

	
)";

class renderState {
	
};

class ParametricSurface: public glApp {
	int winWidht, winHeight;
	GPUProgram* program;
public:
	ParametricSurface() : glApp("Fancy Param Surfaces :3") {
		winWidht = 600;
		winHeight = 600;
	}

	void onInitialization() {
		glViewport(0,0, winWidht, winHeight);
		glClearColor(6.f/255.f, 0x6b / 255.f, 0xd4 / 255.f, 1);
		program = new GPUProgram(vertSource, fragSource);
	}

	void onDisplay() {
		glClear(GL_COLOR_BUFFER_BIT);
	}
};