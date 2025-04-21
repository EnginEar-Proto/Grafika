#include "framework.h"
#include <vector>
#include <cmath>
#include <iostream>

/*
#ifndef Hf33

#define Hf33

const char* vertSource = R"(
	#version 330
	precision highp float;

	layout(location = 0) in vec2 cP;
	layout(location = 1) in vec2 tP;

	out vec2 texcoord;

	void main(){
		gl_Position = vec4(cP.x, cP.y,0,1);
		texcoord = tP;
	}
)";

const char* fragSource = R"(
    #version 330

	uniform sampler2D samplerUnit;
	uniform bool useColor;
	uniform vec3 color;

	uniform vec3 sunDirection;

	in vec2 texcoord;
	out vec4 fragmentColor;

	void main() {
		// Kiszámoljuk a felületi normálist a textúra koordinátákból
		// Latitude: -85° .. 85°, Longitude: -180° .. 180°
		float latitude = ((85.f / 180.f) - texcoord.y) * 3.1415926; // 85 fok max
		float longitude = texcoord.x * 2 * 3.1415926 - 3.1415926;

		vec3 normal;
		normal.x = cos(latitude) * cos(longitude);
		normal.y = sin(latitude);
		normal.z = cos(latitude) * sin(longitude);
    
		normal = normalize(normal);

		float dotProduct = dot(normal, normalize(sunDirection));

		float transparency = 1.0;
		if (dotProduct >= 0.0) {
			transparency = 0.5;
		}

		if (useColor) {
			fragmentColor = vec4(color, 1);
		} else {
			vec4 texColor = texture(samplerUnit, texcoord);
			fragmentColor = vec4(texColor.rgb, texColor.a * transparency);
		}
	}
)";

const int nTessVertecies = 100;


class MapTexture {
	unsigned int textureId = 0;
public:
	MapTexture(int width, int height) {
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	MapTexture(int width, int height, std::vector<vec3>& image) {
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
	~MapTexture() {
		if (textureId > 0) glDeleteTextures(1, &textureId);
	}
};

class Map {
	unsigned int vao, vbo[2];
	unsigned int textureId;
	MapTexture *texture;
	std::vector<vec2> vtx;
public:
	Map(std::vector<int> encoded) {

		std::vector<vec3> image = Decode(encoded);
		texture = new MapTexture(64, 64, image);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(2, &vbo[0]);

		vtx = {vec2(-1, -1),vec2(1, -1),vec2(1,1),vec2(-1,1)};

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec2), &vtx[0], GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), NULL);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		std::vector<vec2> uvs = { vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0,1) };
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), NULL);
	}

	std::vector<vec3> Decode(std::vector<int> image) {
		std::vector<vec3> colors;
		for (int i = 0; i < (int)image.size(); i++) {
			int pixNum = (int)(image[i] >> 2);
			int pixColor = (int)(image[i] & 0b00000011);

			for (int j = 0; j <= pixNum; j++) {
				switch (pixColor)
				{
				case 0:
					colors.push_back(vec3(1,1,1));
					break;
				case 1:
					colors.push_back(vec3(0,0,1));
					break;
				case 2:
					colors.push_back(vec3(0,1,0));
					break;
				case 3:
					colors.push_back(vec3(0,0,0));
					break;
				default:
					break;
				}
			}
		}

		return colors;
	}

	void Draw(GPUProgram *program) {
		int sampler = 0;

		program->setUniform(false, "useColor");
		program->setUniform(sampler, "samplerUnit");

		texture->Bind(sampler);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
};

class Route {
	unsigned int pVao, pVbo;
	unsigned int lVao, lVbo;
	std::vector<vec2> vtx;

public:
	Route() {
		glGenVertexArrays(1, &pVao);
		glBindVertexArray(pVao);
		glGenBuffers(1, &pVbo);
		glBindBuffer(GL_ARRAY_BUFFER, pVbo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE, sizeof(vec2), NULL);

		glGenVertexArrays(1, &lVao);
		glBindVertexArray(lVao);
		glGenBuffers(1, &lVbo);
		glBindBuffer(GL_ARRAY_BUFFER, lVbo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), NULL);
	}

	void AddPoint(float cX, float cY) {
		vtx.push_back(vec2(cX,cY));
	}

	float clampf(float value, float minVal, float maxVal) {
		if (value < minVal) return minVal;
		if (value > maxVal) return maxVal;
		return value;
	}

	vec3 slerp(vec3 p, vec3 q, float t) {
		float d = acos(dot(p, q)); // distance
		return p * (sin((1 - t) * d) / sin(d)) + q * (sin(t * d) / sin(d));
	}

	void Draw(GPUProgram* program) {
		if (vtx.size() == 0) return;

		program->setUniform(true, "useColor");

		const float MAX_LAT = 85.0f * M_PI / 180.0f;
		const float MAX_Y = logf(tanf(M_PI / 4.0f + MAX_LAT / 2.0f));

		if (vtx.size() >= 2) {
			std::vector<vec2> vertexData;

			for (int i = 0; i < (int)vtx.size() - 1; i++) {
				// --- Inverz Mercator: síkbeli (x,y) → gömbi (lat, lon) ---
				float lon1 = vtx[i].x * M_PI;  // síkbeli x közvetlenül hosszúság
				float lat1 = 2.0f * atanf(expf(vtx[i].y * MAX_Y)) - M_PI / 2.0f;

				float lon2 = vtx[i + 1].x * M_PI;
				float lat2 = 2.0f * atanf(expf(vtx[i + 1].y * MAX_Y)) - M_PI / 2.0f;

				vec3 p0(cos(lat1) * cos(lon1), sin(lat1), cos(lat1) * sin(lon1));
				vec3 p1(cos(lat2) * cos(lon2), sin(lat2), cos(lat2) * sin(lon2));

				p0 = normalize(p0);
				p1 = normalize(p1);

				for (int j = 0; j < nTessVertecies; j++) {
					float t = (float)j / (nTessVertecies - 1);
					vec3 p = slerp(p0, p1, t);
					p = normalize(p);

					// --- Vissza: gömbi (lat, lon) → Mercator síkbeli (x,y) ---
					float lon = atan2f(p.z, p.x);
					float lat = asinf(p.y);

					lon = (lon > M_PI) ? lon - 2.0f * M_PI : (lon < -M_PI) ? lon + 2.0f * M_PI : lon;
					lat = clampf(lat, -MAX_LAT, MAX_LAT); // Korlátozás 85°-ra

					float x = lon / M_PI;
					float y = logf(tanf(M_PI / 4.0f + lat / 2.0f)) / MAX_Y; // Skalázva vissza [-1,1] közé

					vertexData.push_back(vec2(x, y));
				}
			}

			glBindVertexArray(lVao);
			glBindBuffer(GL_ARRAY_BUFFER, lVbo);
			glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(vec2), &vertexData[0], GL_DYNAMIC_DRAW);

			program->setUniform(vec3(1, 1, 0), "color");
			glLineWidth(3.f);
			glDrawArrays(GL_LINE_STRIP, 0, vertexData.size());
		}

		// --- Piros pontok rajzolása változatlan ---
		if (vtx.size() > 0) {
			glBindVertexArray(pVao);
			glBindBuffer(GL_ARRAY_BUFFER, pVbo);
			glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec2), &vtx[0], GL_DYNAMIC_DRAW);

			program->setUniform(vec3(1, 0, 0), "color");
			glPointSize(10.f);
			glDrawArrays(GL_POINTS, 0, vtx.size());
		}
	}
};

class Hf3 : glApp {
	Route* route;
	Map* map;
	int winWidth, winHeight;
	GPUProgram* program;
	std::vector<int> encoded = {
		252, 252, 252, 252, 252, 252, 252, 252, 252, 0, 9, 80, 1, 148, 13, 72, 13, 140, 25, 60, 21, 132, 41, 12, 1, 28,
25, 128, 61, 0, 17, 4, 29, 124, 81, 8, 37, 116, 89, 0, 69, 16, 5, 48, 97, 0, 77, 0, 25, 8, 1, 8, 253, 253, 253, 253,
101, 10, 237, 14, 237, 14, 241, 10, 141, 2, 93, 14, 121, 2, 5, 6, 93, 14, 49, 6, 57, 26, 89, 18, 41, 10, 57, 26,
89, 18, 41, 14, 1, 2, 45, 26, 89, 26, 33, 18, 57, 14, 93, 26, 33, 18, 57, 10, 93, 18, 5, 2, 33, 18, 41, 2, 5, 2, 5, 6,
89, 22, 29, 2, 1, 22, 37, 2, 1, 6, 1, 2, 97, 22, 29, 38, 45, 2, 97, 10, 1, 2, 37, 42, 17, 2, 13, 2, 5, 2, 89, 10, 49,
46, 25, 10, 101, 2, 5, 6, 37, 50, 9, 30, 89, 10, 9, 2, 37, 50, 5, 38, 81, 26, 45, 22, 17, 54, 77, 30, 41, 22, 17, 58,
1, 2, 61, 38, 65, 2, 9, 58, 69, 46, 37, 6, 1, 10, 9, 62, 65, 38, 5, 2, 33, 102, 57, 54, 33, 102, 57, 30, 1, 14, 33, 2,
9, 86, 9, 2, 21, 6, 13, 26, 5, 6, 53, 94, 29, 26, 1, 22, 29, 0, 29, 98, 5, 14, 9, 46, 1, 2, 5, 6, 5, 2, 0, 13, 0, 13,
118, 1, 2, 1, 42, 1, 4, 5, 6, 5, 2, 4, 33, 78, 1, 6, 1, 6, 1, 10, 5, 34, 1, 20, 2, 9, 2, 12, 25, 14, 5, 30, 1, 54, 13, 6,
9, 2, 1, 32, 13, 8, 37, 2, 13, 2, 1, 70, 49, 28, 13, 16, 53, 2, 1, 46, 1, 2, 1, 2, 53, 28, 17, 16, 57, 14, 1, 18, 1, 14,
1, 2, 57, 24, 13, 20, 57, 0, 2, 1, 2, 17, 0, 17, 2, 61, 0, 5, 16, 1, 28, 25, 0, 41, 2, 117, 56, 25, 0, 33, 2, 1, 2, 117,
52, 201, 48, 77, 0, 121, 40, 1, 0, 205, 8, 1, 0, 1, 12, 213, 4, 13, 12, 253, 253, 253, 141
	};
public:
	Hf3() : glApp("KDDQON") {
		winWidth = 600;
		winHeight = 600;
	}

	void onInitialization() {
		glViewport(0, 0, winWidth, winHeight);
		glClearColor(0,0,0,1);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		program = new GPUProgram(vertSource, fragSource);
		map = new Map(encoded);
		route = new Route();

		constexpr float degToRad = M_PI / 180.0f;
		float tilt = 45.f * degToRad; // 23 fok --> radián

		// Nap irányvektor kiszámítása
		glm::vec3 sunDirection;
		sunDirection.x = cos(tilt) * cos(0.0f); // longitude = 0
		sunDirection.y = sin(tilt);
		sunDirection.z = cos(tilt) * sin(0.0f); // longitude = 0 -> sin(0) = 0
		program->setUniform(sunDirection, "sunDirection");
	}

	void onDisplay() {
		glClear(GL_COLOR_BUFFER_BIT);

		map->Draw(program);
		route->Draw(program);
	}

	void onMousePressed(MouseButton But, int pX, int pY) {
		float cX = (pX - 300.f) / 300.f;
		float cY = (300.f - pY) / 300.f;

		route->AddPoint(cX, cY);
		refreshScreen();
	}

	void onKeyboard(int key) {
		if (key == 'n') {
			// Ide jön majd a órák láptetése. Vagyis legalább az eltolás változtatása. A másik része a fragSource-ban.
			refreshScreen();
		}
	}
};

Hf3 app;
#endif // !Hf33
*/