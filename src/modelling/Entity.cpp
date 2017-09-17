#include "modelling/Entity.hpp"

using namespace std;

Entity::Entity() : vao(0), vbo(0), ebo(0)
{
	setAmbient(0.0f, 0.0f, 0.0f);
	setDiffuse(0.6f, 0.6f, 0.6f);
	setSpecular(0.0f, 0.0f, 0.0f);
	shininess = 0.1f;
	tex.setId(-1);
}

Entity::~Entity()
{
}
