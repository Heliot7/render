#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>

#include <GL/glew.h>
#include <SOIL.h>

#include "modelling/Model.hpp"
#include "modelling/Vertex.hpp"

using namespace std;

Model::Model(GLuint pShader) : mShader(pShader), mDrawType(SOLID), isObject(false), currentTreeNode(vector<unsigned int>())
{
    Tx = Ty = Tz = 0.0;
    Rx = Ry = Rz = 0.0;
    Sx = Sy = Sz = 1.0;

    isFirstBind = true;
}

Model::~Model()
{
    for(unsigned int i = 0; i < visualEntities.size(); ++i)
    {
        if(visualEntities[i].getTextureId() != 1)
        {
            GLuint texId = visualEntities[i].getTextureId();
            glDeleteTextures(1, &texId);
        }

        glDeleteBuffers(1, visualEntities[i].getPointerEBO());
        glDeleteBuffers(1, visualEntities[i].getPointerVBO());
        glDeleteVertexArrays(1, visualEntities[i].getPointerVAO());
    }	
}

void Model::setAllVertices(Vertex* vertices, unsigned int numAllVertices)
{
    for(unsigned int i = 0; i < numAllVertices; ++i)
    {
        Vertex v;
        v.setPosition(vertices[i].getPosition()[0], vertices[i].getPosition()[1], vertices[i].getPosition()[2]);
        v.setColour(vertices[i].getColour()[0], vertices[i].getColour()[1], vertices[i].getColour()[2], vertices[i].getColour()[3]);
        v.setTexcoord(vertices[i].getTexcoord()[0], vertices[i].getTexcoord()[1]);
        v.setNormal(vertices[i].getNormal()[0], vertices[i].getNormal()[1], vertices[i].getNormal()[2]);
        listAllVertices.push_back(v);
    }
}

void Model::loadRawEntity(Entity& pEntity)
{
    visualEntities.push_back(pEntity);
    bindToOpenGL();
}

bool Model::loadModelFromFile(const string& pFileName)
{
    fileName = pFileName;
    fileDir = getDir();
    fileExt = getExt();

    if(fileExt != "fbx" && fileExt != "FBX")
    {
        // Load model using "assimp" API
        // - Create an instance of the Importer class
        Assimp::Importer importer;
        mScene = importer.ReadFile(fileName, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

        // If the import failed, report it
        if(!mScene)
        {
            cout << "ERROR: MODEL FILE NOT LOADED!" << endl << "Reason: " << importer.GetErrorString() << endl;
            return false;
        }

        // Now we can access the file's contents. 
        getFileContent(mScene);

		// Load Kps file and load them in the current model
		size_t found = fileName.find_last_of(".");
		string fileNameKps = fileName.substr(0, found) + ".kps";
		loadKps(fileNameKps);
    }
    else
    {
        // getFbxContent();
    }

    bindToOpenGL();

	updateBB();

    return true;
}

string Model::getDir()
{
    const size_t last_slash_idx = fileName.rfind('/');
    if (std::string::npos != last_slash_idx)
        return fileName.substr(0, last_slash_idx+1);
    else
        return " ";
}

string Model::getExt()
{
    const size_t last_slash_idx = fileName.rfind('.');
    if (std::string::npos != last_slash_idx)
        return fileName.substr(last_slash_idx+1, fileName.back());
    else
        return " ";
}

string& Model::getFullPath(std::string relativePath)
{
    string path = getDir();
    return path.append(relativePath);
}

/*
bool Model::getFbxContent()
{
    // Create the scenario
    FbxManager* manager = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);
    FbxScene *scene = FbxScene::Create(manager, "Load model");

    // Create the importer
    int fileFormat = -1;
    FbxImporter* importer = FbxImporter::Create(manager, "");
    manager->GetIOPluginRegistry()->DetectReaderFileFormat(fileName.c_str(), fileFormat);
    bool importerStatus = importer->Initialize(fileName.c_str(), fileFormat);
    
    if(importerStatus)
        if(importer->Import(scene))
        {
            // Get Geomtretry
            FbxGeometryConverter lGeomConverter(manager);
            lGeomConverter.Triangulate(scene, true);
            lGeomConverter.SplitMeshesPerMaterial(scene, true);

            // Get visual info
            const int numMaterials = scene->GetMaterialCount();
            listMaterials.clear();
            listMaterials.resize(numMaterials);
            for (int idxMat = 0; idxMat < numMaterials; ++idxMat)
            {
                FbxSurfaceMaterial* mat = scene->GetMaterial(idxMat);
                if(mat)
                {
                    const FbxDouble3 amb = fbxGetMatProperty(mat, "AmbientColor", "AmbientFactor");
                    listMaterials[idxMat].setAmbient(amb[0], amb[1], amb[2]);
                    
                    FbxDouble3 diff = fbxGetMatProperty(mat, "DiffuseColor", "DiffuseFactor");
                    listMaterials[idxMat].setDiffuse(diff[0], diff[1], diff[2]);

                    FbxDouble3 spec = fbxGetMatProperty(mat, "SpecularColor", "SpecularFactor");
                    listMaterials[idxMat].setSpecular(spec[0], spec[1], spec[2]);
                }
            }

            // Get Texture if any at the current material
            const int numTextures = scene->GetTextureCount();
            for (int idxTex = 0; idxTex < numTextures; ++idxTex)
            {
                FbxTexture* tex = scene->GetTexture(idxTex);
                FbxFileTexture* texFile = (FbxFileTexture*)tex;

                if(texFile)
                {
                    listMaterials.push_back(Material());

                    // Try to load the texture from absolute path
                    try
                    {
                        string texPath = texFile->GetFileName();

                        replace(texPath.begin(), texPath.end(), '\\', '/' );
                        cout << "texture loaded: " << texPath << endl;
                        listMaterials.back().setTexture(Texture::loadTexture(texPath));
                    }
                    catch(int)
                    {
                        listMaterials.back().setTextureId(Texture::loadEmptyTexture());
                    }
                }
            }

            // - Mesh
            listMeshes.clear();
            processChildFbx(scene->GetRootNode());
        }

    // Once loaded, remove the object and the scene
    importer->Destroy();
    importer = NULL;

    return true;
}

void Model::processChildFbx(FbxNode* node)
{
    int numChildren = node->GetChildCount();
    for(int idxChild = 0; idxChild < numChildren; ++idxChild)
    {
        FbxNodeAttribute* lNodeAttribute = node->GetChild(idxChild)->GetNodeAttribute();				
        if(lNodeAttribute)
        {
            if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
            {
                FbxMesh* mesh = node->GetChild(idxChild)->GetMesh();
                if(mesh)
                {
                    listMeshes.push_back(Mesh());
                    bool hasColour = mesh->GetElementVertexColorCount() > 0;
                    bool hasNormals = mesh->GetElementNormalCount() > 0;
                    bool hasUVs = mesh->GetElementUVCount() > 0;
                    const int numFaces = mesh->GetPolygonCount();
                    int numVertices = mesh->GetControlPointsCount();

                    listMeshes.back().getListFaceIndices().resize(numFaces);
                    listMeshes.back().getListVertices().resize(numVertices);

                    // Get Faces
                    int currentVertex = 0;
                    const FbxVector4* arrayVertices = mesh->GetControlPoints();
                    for(int idxFace = 0; idxFace < numFaces; ++idxFace)
                    {
                        int vFace[3] = {0, 0, 0};
                        for(int idxVertex = 0; idxVertex < 3; ++idxVertex)
                        {
                            vFace[idxVertex] = mesh->GetPolygonVertex(idxFace, idxVertex);
                            currentVertex = vFace[idxVertex];

                            // Get Vertices
                            FbxVector4 pos = arrayVertices[currentVertex];
                            listMeshes.back().getListVertices()[currentVertex].setPosition(pos[0], pos[1], pos[2]);

                            listMeshes.back().getListVertices()[idxVertex].setColour(1.0, 1.0, 1.0, 1.0);
                            if(hasColour)
                            {
                                int idxColour = currentVertex;
                                if(mesh->GetElementVertexColor()->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                                    idxColour = mesh->GetElementVertexColor()->GetIndexArray().GetAt(currentVertex);
                                FbxColor col = mesh->GetElementVertexColor()->GetDirectArray().GetAt(idxColour);
                                listMeshes.back().getListVertices()[currentVertex].setColour(col.mRed, col.mGreen, col.mBlue, col.mAlpha);
                            }

                            if(hasNormals)
                            {
                                FbxVector4 n;
                                mesh->GetPolygonVertexNormal(idxFace, idxVertex, n);
                                listMeshes.back().getListVertices()[currentVertex].setNormal(n[0], n[1], n[2]);
                            }

                            if(hasUVs)
                            {
                                FbxVector2 uv;
                                FbxStringList UVSetNameList;
                                bool unmapped = false;
                                mesh->GetUVSetNames(UVSetNameList);
                                mesh->GetPolygonVertexUV(idxFace, idxVertex, UVSetNameList.GetStringAt(0), uv, unmapped);
                                listMeshes.back().getListVertices()[currentVertex].setTexcoord(uv[0], 1.0-uv[1]);
                                // cout << "Vertex: " << currentVertex << " UV: [ " << uv[0] << "," <<  uv[1] << "]" << endl;
                            }
                        }
                        listMeshes.back().getListFaceIndices()[idxFace].setFace(vFace[0], vFace[1], vFace[2]);
                    }

                    // - Assign material
                    FbxLayerElementArrayTemplate<int>* lMaterialIndice = NULL;
                    if(mesh->GetElementMaterial())
                    {
                        lMaterialIndice = &mesh->GetElementMaterial()->GetIndexArray();
                        int numMaterials = lMaterialIndice->GetCount();
                        const int idxMat = lMaterialIndice->GetAt(0);									
                        listMeshes.back().setMaterial(idxMat);
                    }
                }
            }
        }
        else
        {
            processChildFbx(node->GetChild(idxChild));
        }
    }
}
*/

/*
const FbxDouble3 Model::fbxGetMatProperty(FbxSurfaceMaterial* pMaterial, const char* pPropertyName, const char* pFactorPropertyName)
{
    FbxDouble3 col(1.0);

    FbxProperty lProperty = pMaterial->FindProperty(pPropertyName);
    FbxProperty lFactorProperty = pMaterial->FindProperty(pFactorPropertyName);
    if(lProperty.IsValid() && lFactorProperty.IsValid())
    {
        col = lProperty.Get<FbxDouble3>();

        double lFactor = lFactorProperty.Get<FbxDouble>();
        if (lFactor != 1)
        {
            col[0] *= lFactor;
            col[1] *= lFactor;
            col[2] *= lFactor;
        }
    }
    */
    /*
    if (lProperty.IsValid())
    {
        const int lTextureCount = lProperty.GetSrcObjectCount<FbxFileTexture>();
        if(lTextureCount)
        {
            const FbxFileTexture* lTexture = lProperty.GetSrcObject<FbxFileTexture>();
            if(lTexture)
            {
                // GLuint pTextureName = *(static_cast<GLuint *>(lTexture->GetUserDataPtr()));
            }
        }
    }
    */
    /*
    return col;
}
*/

bool Model::getFileContent(const aiScene* mScene)
{
    getVisualInfo(mScene);
    getGeometry(mScene);

    // Recompute normals to have one per vertex
    // computeNormalPerVertex();

    return true;
}

void  Model::getVisualInfo(const aiScene* mScene)
{
    visualEntities.clear();
    visualEntities.resize(mScene->mNumMaterials);
    for(unsigned int iMat = 0; iMat < mScene->mNumMaterials; ++iMat)
    {
        const aiMaterial* mMaterial = mScene->mMaterials[iMat];

        // Get colour information
        aiColor3D color(0.0f, 0.0f, 0.0f);
        mMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color);
        visualEntities[iMat].setAmbient(color.r, color.g, color.b);
        mMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        visualEntities[iMat].setDiffuse(color.r, color.g, color.b);
        mMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color);
        visualEntities[iMat].setSpecular(color.r, color.g, color.b);

        // Get texture data if available
        aiString path;
        if(mMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
        {
            string strPath = string(path.C_Str());
            if(strPath[0] == '/')
                strPath.erase(0,1);
            string pathTex = fileDir;
            pathTex.append(strPath);
            visualEntities[iMat].setTexture(Texture::loadTexture(pathTex));
        }
        else
            visualEntities[iMat].setTextureId(1); // empty texture (1 since it is loaded first)
    }
}

void Model::getGeometry(const aiScene* mScene)
{
    listAllVertices.clear();

    // Estimate number of vertices per entity and globally
    unsigned int numAllVertices = 0;
    vector<int> numVisualEntitiesVertex(visualEntities.size());
    vector<int> numVisualEntitiesFace(visualEntities.size());
    for(unsigned int iMesh = 0; iMesh < mScene->mNumMeshes; ++iMesh)
    {
        unsigned int matId = mScene->mMeshes[iMesh]->mMaterialIndex;
        numVisualEntitiesVertex[matId] += mScene->mMeshes[iMesh]->mNumVertices;
        numVisualEntitiesFace[matId] += mScene->mMeshes[iMesh]->mNumFaces;
        numAllVertices += mScene->mMeshes[iMesh]->mNumVertices;
    }
    for(unsigned int iMesh = 0; iMesh < visualEntities.size(); ++iMesh)
    {
        visualEntities[iMesh].getListVertices().resize(numVisualEntitiesVertex[iMesh]);
        visualEntities[iMesh].getListFaceIndices().resize(numVisualEntitiesFace[iMesh]);
        
        visualEntities[iMesh].getListFacesOfVertex().resize(numVisualEntitiesVertex[iMesh]);
    }
    listAllVertices.resize(numAllVertices);
    
    // Loop all model meshes
    unsigned int idAllVertex = 0;
    vector<int> idVisualEntitiesVertex(visualEntities.size(), 0);
    vector<int> idVisualEntitiesFace(visualEntities.size(), 0);
    for(unsigned int iMesh = 0; iMesh < mScene->mNumMeshes; ++iMesh)
    {
        const aiMesh* mMesh = mScene->mMeshes[iMesh];
        unsigned int matId = mMesh->mMaterialIndex;

        // Vertices information
        unsigned int currentSizeVertices = idVisualEntitiesVertex[matId];
        for(unsigned int iVertex = 0; iVertex < mMesh->mNumVertices; ++iVertex)
        {
            // - Position
            if(mMesh->HasPositions())
            {
                const aiVector3D pos = mMesh->mVertices[iVertex];
                listAllVertices[idAllVertex].setPosition(pos.x, pos.y, pos.z);
            }
            
            // - Colour
            int colChannels = mMesh->GetNumColorChannels();
            if(colChannels > 0)
            {
                const aiColor4D col = mMesh->mColors[colChannels-1][iVertex];
                listAllVertices[idAllVertex].setColour(col.r, col.g, col.b, col.a);
            }

            // - Texture coordinates
            if(mMesh->HasTextureCoords(0))
            {
                const aiVector3D tex = mMesh->mTextureCoords[0][iVertex]; // position 0 when only 1 textured used
                listAllVertices[idAllVertex].setTexcoord(tex.x, 1.0f-tex.y);
            }
            
            // - Normals
            if(mMesh->HasNormals())
            {
                const aiVector3D vNormal = mMesh->mNormals[iVertex];
                listAllVertices[idAllVertex].setNormal(vNormal.x, vNormal.y, vNormal.z);
            }

            visualEntities[matId].setVertex(listAllVertices[idAllVertex], idVisualEntitiesVertex[matId]);

            idAllVertex++;
            idVisualEntitiesVertex[matId]++;
        }

        // Faces information
        if(mMesh->HasFaces())
        {
            // unsigned int currentSizeFaces = idVisualEntitiesFace[matId];
            for(unsigned int iFace = 0; iFace < mMesh->mNumFaces; ++iFace)
            {
                const aiFace face = mMesh->mFaces[iFace];
                visualEntities[matId].getListFaceIndices()[idVisualEntitiesFace[matId]].setFace(face.mIndices[0] + currentSizeVertices, face.mIndices[1] + currentSizeVertices, face.mIndices[2] + currentSizeVertices);
                idVisualEntitiesFace[matId]++;

                // Add the face id to the vertices that form it
                visualEntities[matId].setFaceToVertex(face.mIndices[0] + currentSizeVertices, iFace);
                visualEntities[matId].setFaceToVertex(face.mIndices[1] + currentSizeVertices, iFace);
                visualEntities[matId].setFaceToVertex(face.mIndices[2] + currentSizeVertices, iFace);
            }
        }
    }
 }

void Model::computeNormalPerVertex()
{
    for(unsigned int iMesh = 0; iMesh < visualEntities.size(); ++iMesh)
    {
        Entity e = visualEntities[iMesh];
        vector<float> faceNormalsX; vector<float> faceNormalsY; vector<float> faceNormalsZ;
        faceNormalsX.resize(e.getListFaceIndices().size());
        faceNormalsY.resize(e.getListFaceIndices().size());
        faceNormalsZ.resize(e.getListFaceIndices().size());
        for(unsigned int iFace = 0; iFace < e.getListFaceIndices().size(); iFace++)
        {
            // We get the
            Face f = e.getFace(iFace);
            float* v1 = e.getVertex(f.getFaceIdx1()).getPosition();
            float* v2 = e.getVertex(f.getFaceIdx2()).getPosition();
            float* v3 = e.getVertex(f.getFaceIdx3()).getPosition();

            // We calculate both lines of 3 points
            float Px = v2[0] - v1[0];
            float Py = v2[1] - v1[1];
            float Pz = v2[2] - v1[2];
            float Qx = v3[0] - v1[0];
            float Qy = v3[1] - v1[1];
            float Qz = v3[2] - v1[2];

            // We obtain the normal vector and we normalize it
            faceNormalsX[iFace] = Py * Qz - Pz * Qy;
            faceNormalsY[iFace] = Pz * Qx - Px * Qz;
            faceNormalsZ[iFace] = Px * Qy - Py * Qx;
            float den = sqrt(faceNormalsX[iFace] * faceNormalsX[iFace] + faceNormalsY[iFace] * faceNormalsY[iFace] + faceNormalsZ[iFace] * faceNormalsZ[iFace]);
            faceNormalsX[iFace] /= den;
            faceNormalsY[iFace] /= den;
            faceNormalsZ[iFace] /= den;
        }

        float normal[3];
        for(unsigned int iVertex = 0; iVertex < visualEntities[iMesh].getListFacesOfVertex().size(); ++iVertex)
        {
            vector<unsigned int> listFaces = e.getFacesOfVertex(iVertex);
            
            normal[0] = normal[1] = normal[2] = 0.0f;

            for(unsigned iFace = 0; iFace < listFaces.size(); ++iFace)
            {
                normal[0] += faceNormalsX[iFace];
                normal[1] += faceNormalsY[iFace];
                normal[2] += faceNormalsZ[iFace];
            }
            normal[0] /= listFaces.size();
            normal[1] /= listFaces.size();
            normal[2] /= listFaces.size();
            visualEntities[iMesh].getVertex(iVertex).setNormal(normal[0], normal[1], normal[2]);

            // visualEntities[iMesh].getVertex(iVertex).setNormal(faceNormalsX[listFaces[0]], faceNormalsY[listFaces[0]], faceNormalsZ[listFaces[0]]);
        }
    }
}

void Model::attachTexture(GLuint id, string path)
{
    // Assign material 0 to all meshes
    for(unsigned int i = 0; i < visualEntities.size(); ++i)
    {
        visualEntities[i].setTextureId(id);
        visualEntities[i].setTexturePath(path);
    }
}

void Model::bindToOpenGL()
{
    if(isFirstBind)
    {
        for(unsigned int i = 0; i < visualEntities.size(); ++i)
        {
            glGenVertexArrays(1, visualEntities[i].getPointerVAO());
            glGenBuffers(1, visualEntities[i].getPointerVBO());
            glGenBuffers(1, visualEntities[i].getPointerEBO());
        }

        // Add first semantic level in the tree (root)
        resetTree();
    }
    
    Tree<Entity>::sibling_iterator itRootLabel = getTreeNode(vector<unsigned int>());

    unsigned int initFace = 0;
    for(unsigned int i = 0; i < visualEntities.size(); ++i)
    {
        if(visualEntities[i].getListVertices().empty())
            continue;

        glBindVertexArray(visualEntities[i].getVAO());

        glBindBuffer(GL_ARRAY_BUFFER, visualEntities[i].getVBO());
        // Convert from vector<Vertex> to float* array (with all properties embedded) for storing the data on the GPU
        float* vertexData = (float*)(visualEntities[i].getListVertices().data());
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*visualEntities[i].getListVertices().size()*12, vertexData, GL_STATIC_DRAW);
    
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, visualEntities[i].getEBO());
        // Same conversion for vertex indices (faces)
        unsigned int* faceData = (unsigned int*)visualEntities[i].getListFaceIndices().data();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*visualEntities[i].getListFaceIndices().size()*3, faceData, GL_STATIC_DRAW);

        glEnableVertexAttribArray(SHADER_IN::position);
        glVertexAttribPointer(SHADER_IN::position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

        glEnableVertexAttribArray(SHADER_IN::colour);
        glVertexAttribPointer(SHADER_IN::colour, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)(sizeof(float)*3));

        glEnableVertexAttribArray(SHADER_IN::texcoord);
        glVertexAttribPointer(SHADER_IN::texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)(sizeof(float)*7));

        glEnableVertexAttribArray(SHADER_IN::normal);
        glVertexAttribPointer(SHADER_IN::normal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)(sizeof(float)*9));

        if(isFirstBind)
        {
            for(unsigned int v = 0; v < visualEntities[i].getListVertices().size(); ++v)
                (*itRootLabel).getListVertices().push_back(visualEntities[i].getVertex(v));

            for(unsigned int f = 0; f < visualEntities[i].getListFaceIndices().size(); ++f)
            {
                Face newFace = visualEntities[i].getFace(f);
                newFace.setOffset(initFace);
                (*itRootLabel).getListFaceIndices().push_back(newFace);
            }
            initFace += visualEntities[i].getListFaceIndices().size();
        }
    }

    // Bind root labelling of the semantic tree
    bindLabelToOpenGL(*itRootLabel);
    isFirstBind = false;
}

void Model::bindLabelToOpenGL(Entity part)
{
    glBindVertexArray(part.getVAO());
    glBindBuffer(GL_ARRAY_BUFFER, part.getVBO());
    float* vertexData = (float*)(part.getListVertices().data());
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*part.getListVertices().size()*12, vertexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, part.getEBO());
    unsigned int* faceData = (unsigned int*)part.getListFaceIndices().data();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*part.getListFaceIndices().size() * 3, faceData, GL_STATIC_DRAW);
    glEnableVertexAttribArray(SHADER_IN::position);
    glVertexAttribPointer(SHADER_IN::position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glEnableVertexAttribArray(SHADER_IN::colour);
    glVertexAttribPointer(SHADER_IN::colour, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)(sizeof(float)*3));
    glEnableVertexAttribArray(SHADER_IN::texcoord);
    glVertexAttribPointer(SHADER_IN::texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)(sizeof(float)*7));
    glEnableVertexAttribArray(SHADER_IN::normal);
    glVertexAttribPointer(SHADER_IN::normal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)(sizeof(float)*9));
}

void Model::updateMeshes()
{
    bindToOpenGL();
    updateBB();
}

void Model::updateBB()
{
    boundingBox.reset();
    for(unsigned int i = 0; i < visualEntities.size(); ++i)
        for(unsigned int j = 0; j < visualEntities[i].getListVertices().size(); ++j)
        {
            float *v = visualEntities[i].getListVertices()[j].getPosition();
            for(unsigned int pos = 0; pos < 3; ++pos)
            {
                // X dimension
                if(v[0] < boundingBox.getX0())
                    boundingBox.setX0(v[0]);
                else if(v[0] > boundingBox.getX1())
                    boundingBox.setX1(v[0]);

                // Y dimension
                if(v[1] < boundingBox.getY0())
                    boundingBox.setY0(v[1]);
                else if(v[1] > boundingBox.getY1())
                    boundingBox.setY1(v[1]);

                // Z dimension
                if(v[2] < boundingBox.getZ0())
                    boundingBox.setZ0(v[2]);
                else if(v[2] > boundingBox.getZ1())
                    boundingBox.setZ1(v[2]);
            }

        }

    boundingBox.updateCenter();

}

void Model::setPolygonColour(float r, float g, float b)
{
    for(unsigned int iMesh = 0; iMesh < visualEntities.size(); ++iMesh)
        for(unsigned int iVertex = 0; iVertex < visualEntities[iMesh].getListVertices().size(); ++iVertex)
            visualEntities[iMesh].getListVertices()[iVertex].setColour(r, g, b, 1.0);		

    bindToOpenGL();
}

void Model::render()
{
    // Max number of allowes textures
    GLint maxTexs;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexs);
    // Max number of texture size - recommended (w or h -> squared)
    GLint maxTexSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);

    for(unsigned int i = 0; i < visualEntities.size(); ++i)
    {
        if(visualEntities[i].getListVertices().empty())
            continue;

        glBindVertexArray(visualEntities[i].getVAO());
        glBindBuffer(GL_ARRAY_BUFFER, visualEntities[i].getVBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, visualEntities[i].getEBO());
        
        GLuint idxTex = visualEntities[i].getTextureId();
        if(idxTex < GL_MAX_TEXTURE_UNITS)
        {
            glUniform1i(glGetUniformLocation(mShader, "tex"), idxTex-1);
            glActiveTexture(GL_TEXTURE0+(idxTex-1));
            glBindTexture(GL_TEXTURE_2D, idxTex);
        }

        // Material colours to the fragment shader
        GLuint ambientShader = glGetUniformLocation(mShader, "ambientMaterial");
        glUniform3fv(ambientShader, 1, (const GLfloat*)visualEntities[i].getAmbient());
        GLuint diffuseShader = glGetUniformLocation(mShader, "diffuseMaterial");
        glUniform3fv(diffuseShader, 1, (const GLfloat*)visualEntities[i].getDiffuse());
        GLuint specularShader = glGetUniformLocation(mShader, "specularMaterial");
        glUniform3fv(specularShader, 1, (const GLfloat*)visualEntities[i].getSpecular());
        GLuint shininessShader = glGetUniformLocation(mShader, "shininessMaterial");
        glUniform1f(shininessShader, visualEntities[i].getShininess());

        glDrawElements(mDrawType, visualEntities[i].getListFaceIndices().size()*3, GL_UNSIGNED_INT, 0);
    }
}

void Model::renderLabelling()
{
    vector<unsigned int> pathParent(currentTreeNode);
    if(!currentTreeNode.empty())
    {
        vector<unsigned int> pathParent(currentTreeNode);
        pathParent.pop_back();
        Tree<Entity>::sibling_iterator itParent = getTreeNode(pathParent);
        Tree<Entity>::sibling_iterator itTree = semanticTree.begin(itParent);
        for(unsigned int idxChild = 0; idxChild < itParent.number_of_children(); ++idxChild, ++itTree)
            if(!(*itTree).getListVertices().empty())
            {
                glBindVertexArray((*itTree).getVAO());
                glBindBuffer(GL_ARRAY_BUFFER, (*itTree).getVBO());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (*itTree).getEBO());

                // Material colours to the fragment shader
                GLuint colourShader = glGetUniformLocation(mShader, "labelColour");
                glUniform3fv(colourShader, 1, (const GLfloat*)(*itTree).getAmbient());
                GLuint alphaShader = glGetUniformLocation(mShader, "labelAlpha");
                glUniform1f(alphaShader, 1.0f);

                glDrawElements(GL_TRIANGLES, (*itTree).getListFaceIndices().size()*3, GL_UNSIGNED_INT, 0);
            }
    }
    else // draw root
    {
        Tree<Entity>::sibling_iterator itTree = getTreeNode(pathParent);
        glBindVertexArray((*itTree).getVAO());
        glBindBuffer(GL_ARRAY_BUFFER, (*itTree).getVBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (*itTree).getEBO());

        // Material colours to the fragment shader
        GLuint colourShader = glGetUniformLocation(mShader, "labelColour");
        glUniform3fv(colourShader, 1, (const GLfloat*)(*itTree).getAmbient());
        GLuint alphaShader = glGetUniformLocation(mShader, "labelAlpha");
        glUniform1f(alphaShader, 1.0f);

        glDrawElements(GL_TRIANGLES, (*itTree).getListFaceIndices().size()*3, GL_UNSIGNED_INT, 0);
    }
}

void Model::renderParent()
{
    // Render parent node part to edit
    if(!currentTreeNode.empty())
    {
        vector<unsigned int> pathParent(currentTreeNode);
        pathParent.pop_back();
        Tree<Entity>::sibling_iterator itTreeParent = getTreeNode(pathParent);
        if(!(*itTreeParent).getListVertices().empty())
        {
            glBindVertexArray((*itTreeParent).getVAO());
            glBindBuffer(GL_ARRAY_BUFFER, (*itTreeParent).getVBO());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (*itTreeParent).getEBO());

            GLuint idxTex = visualEntities[0].getTextureId();
            if(idxTex < GL_MAX_TEXTURE_UNITS)
            {
                glUniform1i(glGetUniformLocation(mShader, "tex"), idxTex-1);
                glActiveTexture(GL_TEXTURE0+(idxTex-1));
                glBindTexture(GL_TEXTURE_2D, idxTex);
            }

            // Material colours to the fragment shader
            GLuint ambientShader = glGetUniformLocation(mShader, "ambientMaterial");
            glUniform3fv(ambientShader, 1, (const GLfloat*)visualEntities[0].getAmbient());
            GLuint diffuseShader = glGetUniformLocation(mShader, "diffuseMaterial");
            glUniform3fv(diffuseShader, 1, (const GLfloat*)visualEntities[0].getDiffuse());
            GLuint specularShader = glGetUniformLocation(mShader, "specularMaterial");
            glUniform3fv(specularShader, 1, (const GLfloat*)visualEntities[0].getSpecular());
            GLuint shininessShader = glGetUniformLocation(mShader, "shininessMaterial");
            glUniform1f(shininessShader, visualEntities[0].getShininess());

            glDrawElements(mDrawType, (*itTreeParent).getListFaceIndices().size()*3, GL_UNSIGNED_INT, 0);
        }
    }
    else
        render();
}

// Semantic tree functions
Tree<Entity>::sibling_iterator Model::getTreeNode(vector<unsigned int> pathPart)
{
    Tree<Entity>::sibling_iterator itTree = semanticTree.begin();
    for (unsigned int i = 0; i < pathPart.size(); ++i)
    {
        itTree = semanticTree.begin(itTree);
        itTree += pathPart[i];
    }

    return itTree;
}

void Model::resetTree()
{	
    semanticTree.clear();
    Tree<Entity>::sibling_iterator itTree = semanticTree.begin();
    itTree = semanticTree.insert(itTree, Entity());
    setColourTreePart(vector<unsigned int>(), 1.0f, 1.0f, 0.5f);
    glGenVertexArrays(1, (*itTree).getPointerVAO());
    glGenBuffers(1, (*itTree).getPointerVBO());
    glGenBuffers(1, (*itTree).getPointerEBO());
}

void Model::addTreePart(vector<unsigned int> pathPart)
{
    Tree<Entity>::sibling_iterator itTree = getTreeNode(pathPart);
    itTree = semanticTree.append_child(itTree, Entity());
    glGenVertexArrays(1, (*itTree).getPointerVAO());
    glGenBuffers(1, (*itTree).getPointerVBO());
    glGenBuffers(1, (*itTree).getPointerEBO());
}

void Model::removeTreePart(vector<unsigned int> pathPart)
{
    Tree<Entity>::sibling_iterator itTree = getTreeNode(pathPart);
    glDeleteBuffers(1, (*itTree).getPointerEBO());
    glDeleteBuffers(1, (*itTree).getPointerVBO());
    glDeleteVertexArrays(1, (*itTree).getPointerVAO());
    semanticTree.erase(itTree);
}

void Model::setColourTreePart(vector<unsigned int> pathPart, float r, float g, float b)
{
    Tree<Entity>::sibling_iterator itTree = getTreeNode(pathPart);	
    (*itTree).setAmbient(r,g,b);
}

void Model::addTreePartFace(Vertex& v1, Vertex& v2, Vertex& v3)
{
    if(!currentTreeNode.empty())
    {
        Tree<Entity>::sibling_iterator itTree = getTreeNode(currentTreeNode);
        Face face;

        Vertex listVertices[3];
        listVertices[0] = v1; listVertices[1] = v2; listVertices[2] = v3;

        for(unsigned int v = 0; v < 3; ++v)
        {
            bool isFound = false;
            unsigned int i = 0;
            while(!isFound && i < (*itTree).getListVertices().size())
            {
                isFound = Vertex::isSameVertex((*itTree).getVertex(i), listVertices[v]);
                ++i;
            }

            if(!isFound)
            {
                face.setFace(v, (*itTree).getListVertices().size());
                (*itTree).getListVertices().push_back(listVertices[v]);
            }
            else
                face.setFace(v, i-1);
        }

        bool isRepeatedFace = false;
        for(unsigned int idxFace = 0; idxFace < (*itTree).getListFaceIndices().size(); ++idxFace)
        {
            Face addedFace = (*itTree).getFace(idxFace);
            if(addedFace.getFaceIdx1() == face.getFaceIdx1() || addedFace.getFaceIdx2() == face.getFaceIdx1() || addedFace.getFaceIdx3() == face.getFaceIdx1())
                if(addedFace.getFaceIdx1() == face.getFaceIdx2() || addedFace.getFaceIdx2() == face.getFaceIdx2() || addedFace.getFaceIdx3() == face.getFaceIdx2())
                    if(addedFace.getFaceIdx1() == face.getFaceIdx3() || addedFace.getFaceIdx2() == face.getFaceIdx3() || addedFace.getFaceIdx3() == face.getFaceIdx3())
                    {
                        isRepeatedFace = true;
                        break;
                    }
        }

        if(!isRepeatedFace)
        {
            (*itTree).getListFaceIndices().push_back(face);
            bindLabelToOpenGL(*itTree);

            // Check if the face is already stored in other brothers and removed if necessary			
            unsigned int nodeLevelId = currentTreeNode.back();
            vector<unsigned int> pathParent(currentTreeNode);
            pathParent.pop_back();
            Tree<Entity>::sibling_iterator itParent = getTreeNode(pathParent);
            Tree<Entity>::sibling_iterator itBrother = semanticTree.begin(itParent);
            for(unsigned int idxChild = 0; idxChild < itParent.number_of_children(); ++idxChild, ++itBrother)
            {
                if(idxChild == nodeLevelId) // Same node should not compare to himself (the vertices are there!)
                    continue;
                for(unsigned int idxFace = 0; idxFace < (*itBrother).getListFaceIndices().size(); ++idxFace)
                {
                    Face f = (*itBrother).getFace(idxFace);
                    if(Vertex::isSameVertex((*itBrother).getVertex(f.getFaceIdx1()), v1) || Vertex::isSameVertex((*itBrother).getVertex(f.getFaceIdx2()), v1) || Vertex::isSameVertex((*itBrother).getVertex(f.getFaceIdx3()), v1))
                        if(Vertex::isSameVertex((*itBrother).getVertex(f.getFaceIdx1()), v2) || Vertex::isSameVertex((*itBrother).getVertex(f.getFaceIdx2()), v2) || Vertex::isSameVertex((*itBrother).getVertex(f.getFaceIdx3()), v2))
                            if(Vertex::isSameVertex((*itBrother).getVertex(f.getFaceIdx1()), v3) || Vertex::isSameVertex((*itBrother).getVertex(f.getFaceIdx2()), v3) || Vertex::isSameVertex((*itBrother).getVertex(f.getFaceIdx3()), v3))
                            {
                                // Remove and stop searching
                                (*itBrother).getListFaceIndices().erase((*itBrother).getListFaceIndices().begin() + idxFace);
                                bindLabelToOpenGL(*itBrother);
                                break;
                            }
                }
            }
        }
    }
}

void Model::removeTreePartFace(Vertex& v1, Vertex& v2, Vertex& v3)
{
    if(!currentTreeNode.empty())
    {
        Tree<Entity>::sibling_iterator itTree = getTreeNode(currentTreeNode);
        for(unsigned int idxFace = 0; idxFace < (*itTree).getListFaceIndices().size(); ++idxFace)
        {
            Face f = (*itTree).getFace(idxFace);
            if(Vertex::isSameVertex((*itTree).getVertex(f.getFaceIdx1()), v1) || Vertex::isSameVertex((*itTree).getVertex(f.getFaceIdx2()), v1) || Vertex::isSameVertex((*itTree).getVertex(f.getFaceIdx3()), v1))
                if(Vertex::isSameVertex((*itTree).getVertex(f.getFaceIdx1()), v2) || Vertex::isSameVertex((*itTree).getVertex(f.getFaceIdx2()), v2) || Vertex::isSameVertex((*itTree).getVertex(f.getFaceIdx3()), v2))
                    if(Vertex::isSameVertex((*itTree).getVertex(f.getFaceIdx1()), v3) || Vertex::isSameVertex((*itTree).getVertex(f.getFaceIdx2()), v3) || Vertex::isSameVertex((*itTree).getVertex(f.getFaceIdx3()), v3))
                    {
                        // Remove and stop searching
                        (*itTree).getListFaceIndices().erase((*itTree).getListFaceIndices().begin() + idxFace);
                        bindLabelToOpenGL(*itTree);
                        break;
                    }
        }
    }
}

bool Model::checkTreeCompleteness()
{
    bool isComplete = true;
    Tree<Entity>::iterator itParent = semanticTree.begin();
    while(itParent != semanticTree.end())
    {
        unsigned int numFacesParent = (*itParent).getListFaceIndices().size();
        unsigned int numFacesChildren = 0;
        Tree<Entity>::iterator itTree = semanticTree.begin(itParent);
        if(itParent.number_of_children() > 0)
        {
            for(unsigned int idxChild = 0; idxChild < itParent.number_of_children(); ++idxChild, ++itTree)
                numFacesChildren += (*itTree).getListFaceIndices().size();		
            isComplete = isComplete && (numFacesChildren >= numFacesParent);
        }
        ++itParent;
    }

    return isComplete;
}

bool Model::checkLevelCompleteness()
{
    vector<unsigned int> pathParent(currentTreeNode);
    if(!currentTreeNode.empty())
    {
        pathParent.pop_back();
        Tree<Entity>::sibling_iterator itParent = getTreeNode(pathParent);
        numFacesParent = (*itParent).getListFaceIndices().size();
        Tree<Entity>::sibling_iterator itTree = semanticTree.begin(itParent);
        numFacesChildren = 0;
        for(unsigned int idxChild = 0; idxChild < itParent.number_of_children(); ++idxChild, ++itTree)
            numFacesChildren += (*itTree).getListFaceIndices().size();
        return (numFacesChildren >= numFacesParent);
    }
    else
    {
        numFacesParent = numFacesChildren = getTreeNode(pathParent)->getListFaceIndices().size();
        return true;
    }
}

// Keypoints
void Model::loadKps(std::string filename)
{
	string lineKps, elemKps;
	ifstream fileKps;
	fileKps.open(filename.c_str());
	if (fileKps.is_open())
	{
		list_kps.clear();
		int orderKp = 0;
		while (getline(fileKps, lineKps))
		{
			int pos = 0;
			Kp kp;
			kp.isActive = false;
			istringstream iss(lineKps);
			while (iss)
			{
				string sub;
				iss >> sub;
				if (sub != "")
				{
					// cout << sub << endl;
					switch (pos)
					{
					case 0:
						kp.name = sub;
						kp.pos = orderKp;
						kp.Sx = kp.Sy = kp.Sz = 0.015f;
						++orderKp;
						break;
					case 1:
						kp.X = atof(sub.c_str());
						break;
					case 2:
						kp.Y = atof(sub.c_str());
						break;
					case 3:
						kp.Z = atof(sub.c_str());
						break;
					case 4:
						kp.Sx = atof(sub.c_str());
						break;
					case 5:
						kp.Sy = atof(sub.c_str());
						break;
					case 6:
						kp.Sz = atof(sub.c_str());
						break;
					}
				}
				++pos;
			}
			list_kps[kp.name] = kp;
		}
		fileKps.close();
	}
}

void Model::setKpsActive(string id)
{
	for (map<string, Kp>::iterator it = list_kps.begin(); it != list_kps.end(); ++it)
		it->second.isActive = false;
	list_kps[id].isActive = true;
}

void Model::addKps(string name, float x, float y, float z, float sx, float sy, float sz, int pos)
{
	Kp kp;
	kp.name = name;
	kp.X = x;
	kp.Y = y;
	kp.Z = z;
	kp.Sx = sx;
	kp.Sy = sy;
	kp.Sz = sz;
	kp.pos = pos;
	list_kps[name] = kp;
}

void Model::removeKps(string name)
{
	list_kps.erase(name);
}
void Model::updateKpsX(std::string id, float X)
{
	list_kps[id].X = X;
}
void Model::updateKpsY(std::string id, float Y)
{
	list_kps[id].Y = Y;
}
void Model::updateKpsZ(std::string id, float Z)
{
	list_kps[id].Z = Z;
}
void Model::updateKpsPos(std::string id, int pos)
{
	list_kps[id].pos = pos;
}
void Model::updateKpsSx(std::string id, float Sx)
{
	list_kps[id].Sx = Sx;
}
void Model::updateKpsSy(std::string id, float Sy)
{
	list_kps[id].Sy = Sy;
}
void Model::updateKpsSz(std::string id, float Sz)
{
	list_kps[id].Sz = Sz;
}