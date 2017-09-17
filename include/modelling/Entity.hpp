#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <vector>

#include "modelling/Texture.hpp"
#include "modelling/Face.hpp"
#include "modelling/Vertex.hpp"

class Entity
{
	public:

		Entity();
		~Entity();

		// Getters
		unsigned int getVAO() { return vao; }
		unsigned int* getPointerVAO() { return &vao; }
		unsigned int getVBO() { return vbo; }
		unsigned int* getPointerVBO() { return &vbo; }
		unsigned int getEBO() { return ebo; }
		unsigned int* getPointerEBO() { return &ebo; }

		std::vector<Vertex>& getListVertices() { return listVertices; }
		Vertex& getVertex(unsigned int idxVertex) { return listVertices[idxVertex]; }
		std::vector<Face>& getListFaceIndices() { return listFaceIndices; }
		Face& getFace(unsigned int idxFace) { return listFaceIndices[idxFace]; }
		std::vector<std::vector<unsigned int>>& getListFacesOfVertex() { return listFacesOfVertex; }
		std::vector<unsigned int>& getFacesOfVertex(unsigned int idxVertex) { return listFacesOfVertex[idxVertex]; }
		Texture& getTexture() { return tex; }
		unsigned int& getTextureId() { return tex.getId(); }
		std::string& getTexturePath() { return tex.getPath(); }
		float* getAmbient() { return ambient; }
		float* getDiffuse() { return diffuse; }
		float* getSpecular() { return specular; }
		float getShininess() { return shininess; }
		
		// Setters
		void setVertex(Vertex& vertex, unsigned int position) { listVertices[position] = vertex; }
		void setFaceToVertex(unsigned int idxVertex, unsigned int idxFace) { listFacesOfVertex[idxVertex].push_back(idxFace); }
		void setTexture(Texture& newTex) { tex = newTex; }
		void setTextureId(unsigned int id) { tex.setId(id); }
		void setTexturePath(const std::string& path) { tex.setPath(path); }
		void setAmbient(float r, float g, float b) { ambient[0] = r; ambient[1] = g; ambient[2] = b; }
		void setDiffuse(float r, float g, float b) { diffuse[0] = r; diffuse[1] = g; diffuse[2] = b; }
		void setSpecular(float r, float g, float b) { specular[0] = r; specular[1] = g; specular[2] = b; }
		void setShininess(float aShine) { shininess = aShine; }

	private:

		unsigned int vao, vbo, ebo;

		std::vector<Vertex> listVertices;
		std::vector<Face> listFaceIndices;
		std::vector<std::vector<unsigned int>> listFacesOfVertex;
		Texture tex;
		float ambient[3], diffuse[3], specular[3];
		float shininess;
		
};

#endif