#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <math.h>

class Vertex
{
	public:

		Vertex();
		~Vertex();

		// Getters
		float* getPosition() { return position; }
		float* getColour() { return colour; }
		float* getTexcoord() { return texcoord; }
		float* getNormal() { return normal; }

		// Setters
		void setPosition(float x, float y, float z) { position[0] = x; position[1] = y; position[2] = z; }
		void setColour(float r, float g, float b, float a) { colour[0] = r; colour[1] = g; colour[2] = b; colour[3] = a;}
		void setTexcoord(float s, float t) { texcoord[0] = s; texcoord[1] = t; }
		void setNormal(float nx, float ny, float nz) { normal[0] = nx; normal[1] = ny; normal[2] = nz; }

		static bool isSameVertex(Vertex& v1, Vertex& v2)
		{
			return (v1.getPosition()[0] == v2.getPosition()[0] && v1.getPosition()[1] == v2.getPosition()[1] && v1.getPosition()[2] == v2.getPosition()[2]);
		}

	private:

		float position[3];
		float colour[4];
		float texcoord[2];
		float normal[3];
		
};

#endif