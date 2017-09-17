#ifndef RENDER_HPP
#define RENDER_HPP

#include <vector>
#include <string>

#include <math.h>
#include <fstream>

#include <QGLWidget>
#include <QTimer>

// Arithmetic operations
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "modelling/Model.hpp"
#include "rendering/Sampler.hpp"

#define STEP_TRANS 10.0f
#define STEP_ROT 10.0f

enum TYPE_SHADER { FLAT, PHONG, DEPTH, ORTHO, BACKGROUND, LABELLING };

struct Camera
{
	glm::vec3 pos, dir, fixedPos;
	glm::vec2 rot;
	float fov, ar, nearPlane, farPlane;
	float azimuth, elevation, distance;
	Camera(float x = 0.0f, float y = 0.0f, float z = 2.0f)
	{
		// Free Camera:
		pos = glm::vec3(x, y, z);
		dir = glm::vec3(-x, -y, -z);
		// Be careful, z should not be 0
		float angle = atan(y/z)*180.0f/3.1416f;
		rot = glm::vec2(0.0f, -angle); // in degrees

		// Fixed Camera (sphere)
		azimuth = x;
		elevation = y;
		distance = fabs(z);

		fov = 45.0f;
		ar = 1.0f;
		nearPlane = 0.01f;
		farPlane = 100.0f;
	}
};

struct Lights
{
	static const int MAX_LIGHTS = 8;

	bool onLight[MAX_LIGHTS];
	glm::vec3 position[MAX_LIGHTS];
	glm::vec3 ambient[MAX_LIGHTS], diffuse[MAX_LIGHTS], specular[MAX_LIGHTS];
	glm::vec3 attenuation[MAX_LIGHTS];
	Lights()
	{
		for(unsigned int i = 0; i < MAX_LIGHTS; ++i)
		{
			onLight[i] = true;
			position[i] = glm::vec3(0.0f, 10.0f, -1.0f);
			ambient[i] = glm::vec3(0.05f, 0.05f, 0.05f);
			diffuse[i] = glm::vec3(0.5f/MAX_LIGHTS, 0.5f/MAX_LIGHTS, 0.4f/MAX_LIGHTS); // should not change anything
			specular[i] = glm::vec3(0.022f, 0.022f, 0.0175f);
			attenuation[i] = glm::vec3(1.0f, 0.00f, 0.2f);
		}
	}
};

class Render : public QGLWidget
{
    Q_OBJECT

    public:

        Render(QWidget *parent, const QGLFormat &format, Sampler* pSampler, Sampler* pDepth, std::string& outputPath, std::string& objPath);
        ~Render();

		bool loadModelFromFile(const std::string& fileName, GLuint shader);
		void saveSegmentationToFile(std::ofstream& segmentationFile, std::vector<unsigned int>& treePath);
		bool loadSegmentationFromFile(std::ifstream& segmentationFile, std::vector<unsigned int>& treePath, float r, float g, float b);
		void loadBackgroundImgFromFile(const std::string& fileName);
		void setAntiAliasing(bool isAA) { isAntiAliasing = isAA; }
		void setViewBoundingBox(bool isBB) { isViewBoundingBox = isBB; }		
		void setFreeCamera(bool isFree) { isFreeCamera = isFree; }
		void resetCamera() { cam = Camera(); }
		bool previewSamples() { return createSamples(false); }
		bool saveViewToImage(std::string& path = std::string()) { return createSamples(true, path); }
		void runScript();

		// I/O calls
		virtual void keyPressEvent(QKeyEvent *event);
		virtual void keyReleaseEvent(QKeyEvent *event);
		virtual void mousePressEvent(QMouseEvent *event);
        virtual void mouseMoveEvent(QMouseEvent *event);
		virtual void wheelEvent(QWheelEvent *event);

		static float degree(float radian) { return (radian*180.0f) / 3.1416f; }
		static float radian(float degree) { return (degree*3.1416f) / 180.0f; }

		// Change values from GUI
		// - Getters
		Lights& getLights() { return mLights; }
		Model* getModel() { return listModels.back(); }
		bool isModel() { return !listModels.empty(); }

		// - Setters
		void setFixedDistance(float value) { cam.distance = value; }
		void setFixedAzimuth(float value) { cam.azimuth = value; }
		void setFixedElevation(float value) { cam.elevation = value; }

		void setPosObj(float newX, float newY, float newZ)
		{
			for(unsigned i = 0; i < listModels.size(); ++i)
				listModels[i]->setTranslation(newX, newY, newZ);
		}
		void setRotObj(float newRX, float newRY, float newRZ)
		{
			for(unsigned i = 0; i < listModels.size(); ++i)
				listModels[i]->setRotation(newRX, newRY, newRZ);
		}
		void setScaleObj(float newSX, float newSY, float newSZ)
		{
			for(unsigned i = 0; i < listModels.size(); ++i)
				listModels[i]->setScaling(newSX, newSY, newSZ);
		}

		bool getIsLabel() { return isLabel; }
		void setIsLabel(bool label) { isLabel = label; }
		void setIsEditMode(bool isEdit) { isEditMode = isEdit; }
		void setEditPixelMode(bool isEditPixel) { isEditPixelMode = isEditPixel; }
		void setIsKpsMode(bool isKps) { isKpsMode = isKps; }

		void setBrushSize(int newValue) { brushSize = newValue; }

		GLuint getShader(TYPE_SHADER type) { return programShaders[type]; }

		// Keypoints
		void createKp(Kp kp);
		void destroyKp(std::string id);
		void drawActiveKps(std::string id);
		void updateKpsX(std::string id, float X);
		void updateKpsY(std::string id, float Y);
		void updateKpsZ(std::string id, float Z);
		void updateKpsSx(std::string id, float Sx);
		void updateKpsSy(std::string id, float Sy);
		void updateKpsSz(std::string id, float Sz);
		glm::vec3 computeKpsProj(Model* currentKp);
		bool isKpsVisible(Model* currentKp);
		void setIsKpsAz(bool isNo) { isKpsAz = isNo; }
		void setIsKpsSelfOcc(bool isNo) { isKpsSelfOcc = isNo; }

    protected:

		// OpenGL oriented calls
        virtual void initializeGL();
        virtual void paintGL();
        virtual void resizeGL(int width, int height);

	protected slots:

		void computeFPS();

    private:

		// FPS
		QTimer *mTimer, *mFPS;
		int countFPS;
		float currentFPS;

		// Interaction parameters
		bool mKeyPressed[4];
		glm::vec2 mMousePos;

		// Camera information
		bool isFreeCamera;
		Camera cam;
		glm::vec3 vuv;
		glm::mat4 model, view, proj, orthoProj;
		void updateViewMatrix();
		void updateProjectionMatrix();

		// Models in the scenario
		bool isRenderLabelling;
		void render(Model* obj, GLuint shader);
		void renderUI();
		void renderBackground();
		void renderBrush();
		void renderKps();
		void computeBB2D();
		GLuint fboRender, imgMSAA, depthMSAA, fboDepth, bufDepth, fboDepthVis, bufDepthVis, fboBinary, bufBinary;
		int widthRender, heightRender;
		std::vector<Model*> listModels;
		std::vector<BB> listImgBB;
		Model* uiQuad;
		Model* backgroundQuad;
		int backgroundWidth, backgroundHeight;
		Model* brush;
		int brushSize;
		std::map<std::string, Model*> model_kps;
		GLuint emptyTex, texBackground;

        // Shading
		void loadShaders(std::vector<std::string> nameShaders);
		std::vector<GLuint> programShaders;
		unsigned int currentShader;

		// Light conditions
		void setUpLights();
		Lights mLights;

		// Predefined Models
		Model* createQuad(GLuint shader, DRAW_TYPE drawType = DRAW_TYPE::SOLID, std::string texturePath = "", bool isAbsolute = false);
		Model* createObj(const char* objStr, GLuint shader, float r = 1.0f, float g = 0.0f, float b = 0.0f);

		// Output image
		std::string PATH_OUTPUT, PATH_OBJ;
		bool isAntiAliasing;
		bool isViewBoundingBox;
		bool isSampling;
		bool isLabel, isEditMode, isEditPixelMode, isKpsMode;
		Sampler *mSampler, *mDepth;
		GLuint fboSample, texSample;
		std::vector<GLubyte> binaryView, depthByte; // depth from main view		
		std::vector<GLfloat> depth, depthKps; // depth from depth view
		bool createSamples(bool toSave, std::string& path = std::string());
		std::vector<std::string> listAnnotations;
		void saveAnnotations(std::string& imgName, int posSampleX, int posSampleY);
	
		// Keypoints
		bool isKpsAz;
		bool isKpsSelfOcc;

	signals:

		void updateFPS(const QString& text);
		void updateBrushGUI(int newValue);
		void updateCompleteness();
};

#endif
