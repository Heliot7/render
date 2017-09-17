#include "modelling/Vertex.hpp"

using namespace std;

Vertex::Vertex()
{
	position[0] = 0.0f; position[1] = 0.0f; position[2] = 0.0f;
	colour[0] = 0.0f; colour[1] = 0.0f; colour[2] = 0.0f; colour[3] = 0.0f;
	texcoord[0] = 0.0f; texcoord[1] = 0.0f;
	normal[0] = 0.0f; normal[1] = 0.0f; normal[2] = 0.0f;
}

Vertex::~Vertex()
{

}
