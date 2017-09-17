#ifndef MODEL_HPP
#define MODEL_HPP

#include <vector>
#include <string>
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "modelling/Entity.hpp"
#include "modelling/BB.hpp"
#include "modelling/Tree.hpp"

enum SHADER_IN { position, colour, texcoord, normal };
enum DRAW_TYPE { SOLID = GL_TRIANGLES, LINES = GL_LINE_LOOP};

struct Kp
{
	std::string name;
	float X, Y, Z; // coordinate
	float Sx, Sy, Sz; // scales
	int pos;
	bool isActive;
};

class Model
{
	public:
		Model(GLuint pShader = 0);
		~Model();

		void loadRawEntity(Entity& pEntity);
		bool loadModelFromFile(const std::string& fileName);
		void attachTexture(GLuint id, std::string path = "");
		void updateMeshes();
		void updateLabels(Entity part) { bindLabelToOpenGL(part); }
		void updateBB();
		void render();
		void renderLabelling();
		void renderParent();

		// Setters
		void setAllVertices(Vertex* vertices, unsigned int numAllVertices);
		void setShader(GLuint pShader) { mShader = pShader; }
		void setTranslation(float newTX, float newTY, float newTZ) { Tx = newTX; Ty = newTY; Tz = newTZ; }
		void setTranslationX(float newTX) { Tx = newTX; }
		void setTranslationY(float newTY) { Ty = newTY; }
		void setTranslationZ(float newTZ) { Tz = newTZ; }
		void setRotation(float newRX, float newRY, float newRZ) { Rx = newRX; Ry = newRY; Rz = newRZ; }
		void updateRY(float newRY) { Ry += newRY; }
		void setScaling(float newSX, float newSY, float newSZ) { Sx = newSX; Sy = newSY; Sz = newSZ; }
		void setSx(float newSx) { Sx = newSx; }
		void setSy(float newSy) { Sy = newSy; }
		void setSz(float newSz) { Sz = newSz; }
		void setDrawType(DRAW_TYPE type) { mDrawType = type; }
		void setPolygonColour(float r, float g, float b);
		void setObject(bool obj) { isObject = obj; }

		// Labelling
		void resetTree();
		Tree<Entity>::sibling_iterator getTreeNode(std::vector<unsigned int> pathPart);
		bool checkTreeCompleteness();
		bool checkLevelCompleteness();
		

		void addTreePart(std::vector<unsigned int> pathPart);
		void removeTreePart(std::vector<unsigned int> pathPart);
		void setColourTreePart(std::vector<unsigned int> pathPart, float r, float g, float b);
		void addTreePartFace(Vertex& v1, Vertex& v2, Vertex& v3);
		void removeTreePartFace(Vertex& v1, Vertex& v2, Vertex& v3);
		void setCurrentTreeNode(std::vector<unsigned int> treeNode) { currentTreeNode = treeNode; }

		// Keypoints
		std::map<std::string, Kp> getKps() { return list_kps; }
		void setKps(std::map<std::string, Kp> new_kps) { list_kps = new_kps; }
		Kp getKpsId(std::string id) { return list_kps[id]; }
		bool isKpsActive(std::string id) { return list_kps[id].isActive; }
		void loadKps(std::string filename);
		void setKpsActive(std::string id);
		void addKps(std::string name, float x, float y, float z, float sx, float sy, float sz, int pos);
		void removeKps(std::string name);
		void updateKpsX(std::string id, float X);
		void updateKpsY(std::string id, float Y);
		void updateKpsZ(std::string id, float Z);
		void updateKpsPos(std::string id, int pos);
		void updateKpsSx(std::string id, float Sx);
		void updateKpsSy(std::string id, float Sy);
		void updateKpsSz(std::string id, float Sz);

		// Getters
		std::vector<Vertex>& getListAllVertices() { return listAllVertices; }
		std::string& getFullPath(std::string relativePath);
		GLuint getShader() { return mShader; }
		float getTX() { return Tx; } float getTY() { return Ty; } float getTZ() { return Tz; }
		float getRX() { return Rx; } float getRY() { return Ry; } float getRZ() { return Rz; }
		float getSX() { return Sx; } float getSY() { return Sy; } float getSZ() { return Sz; }
		Entity& getVisualEntity(unsigned int pos) { return visualEntities[pos]; }
		unsigned int getNumVisualEntities() { return visualEntities.size(); }
		BB& getBB() { return boundingBox; }
		DRAW_TYPE getDrawType() { return mDrawType; }
		bool isObjectScenario() { return isObject; }
		unsigned int getCurrentTreeLevel() { return currentTreeNode.size(); }
		std::vector<unsigned int> getCurrentTreeNode() { return currentTreeNode; }
		unsigned int getFacesParent() { return numFacesParent; } 
		unsigned int getFacesChildren() { return numFacesChildren; } 		

	private:

		// Assimp scene
		const aiScene* mScene;

		// Object information
		bool isObject;

		// IO
		bool getFileContent(const aiScene* mScene);
		// bool getFbxContent();
		std::string fileName;
		std::string getDir();
		std::string fileDir;
		std::string getExt();
		std::string fileExt;

		// OpenGL connections
		void bindToOpenGL();
		void bindLabelToOpenGL(Entity part);
		bool isFirstBind;

		// Geometry
		std::vector<Vertex> listAllVertices;
		void getGeometry(const aiScene* mScene);
		void computeNormalPerVertex();
		std::vector<Entity> visualEntities;
		Tree<Entity> semanticTree;
		std::vector<unsigned int> currentTreeNode;
		// - Linear transformations
		// glm::mat4 model;
		float Tx, Ty, Tz, Rx, Ry, Rz, Sx, Sy, Sz;
		BB boundingBox;
		// - Labelling Completeness
		unsigned int numFacesParent;
		unsigned int numFacesChildren;

		// Visualisation
		void getVisualInfo(const aiScene* mScene);
		//const FbxDouble3 fbxGetMatProperty(FbxSurfaceMaterial* pMaterial, const char* pPropertyName, const char* pFactorPropertyName);
		//void processChildFbx(FbxNode* node);
		DRAW_TYPE mDrawType;

		// Shading
		GLuint mShader;

		// Keypoints
		std::map<std::string, Kp> list_kps;
};

#endif