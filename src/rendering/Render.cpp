// STL Dependencies
#include <iostream>
#include <time.h>
#include <math.h>
#include <iomanip>

// Qt Dependencies
#include <QPainter>
#include <QFileInfo>
#include <QMouseEvent>
#include <QKeyEvent>
#include <qDebug>
#include <QFile>
#include <QDir>

// OpenGL function recognition
#include <GL/glew.h>
// Image Loader
#include <SOIL.h>
// Save images
#include "lodepng.h"

#include "rendering/Render.hpp"

#include "glm/ext.hpp"

using namespace std;

Render::Render(QWidget *parent, const QGLFormat &format, Sampler* pSampler, Sampler* pDepth, string& pathOutput, string& pathObj) : 
	QGLWidget(format, parent), 
	mSampler(pSampler),
	mDepth(pDepth),
	widthRender(766),
	heightRender(766),
	cam(), vuv(glm::vec3(0.0f, 1.0f, 0.0f)),
	mMousePos(glm::vec2(-1,-1)),
	PATH_OUTPUT(pathOutput),
	PATH_OBJ(pathObj)
{
	setMouseTracking(true);

	mKeyPressed[0] = mKeyPressed[1] = mKeyPressed[2] = mKeyPressed[3] = false;
	isSampling = false;
	currentFPS = 30;

	isLabel = false;
}

Render::~Render()
{
	for(unsigned int i = 0; i < programShaders.size(); ++i)
		glDeleteProgram(programShaders[i]);

	for(unsigned int i = 0; i < listModels.size(); ++i)
			delete listModels[i];
}

void Render::initializeGL()
{
	// Initialise GLEW
	// Activation of "Experimental" avoids "access violation "when calling "glGenVertexArrays"
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));

	cout << "OpenGL initialized: version "<< glGetString(GL_VERSION) << endl << "GLSL "<< glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	err = glGetError(); // Ignore ENUM error which is not important

	// For line drawings
	glLineWidth(2);

	// Amount of sampling
	GLint numMSAA;
	glGetIntegerv(GL_MAX_SAMPLES, &numMSAA);
	cout << "Max samples: " << numMSAA << endl;
	int num_samples = numMSAA; // FSAA max number: 32!

	// Define MSAA framebuffer
	glGenRenderbuffers(1, &imgMSAA);
	glBindRenderbuffer(GL_RENDERBUFFER, imgMSAA);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, num_samples, GL_RGBA, widthRender, heightRender);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glGenRenderbuffers(1, &depthMSAA);
	glBindRenderbuffer(GL_RENDERBUFFER, depthMSAA);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, num_samples, GL_RGBA32F, widthRender, heightRender);

	// Depth buffer
	GLuint zBuffer;
	glGenRenderbuffers(1, &zBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, zBuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, num_samples, GL_DEPTH_COMPONENT32F, widthRender, heightRender);

	glGenFramebuffers(1, &fboRender);
	glBindFramebuffer(GL_FRAMEBUFFER, fboRender);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, imgMSAA);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, depthMSAA);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, zBuffer);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "Not properly installed MS-FBO: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Define FBO binary
	glGenRenderbuffers(1, &bufBinary);
	glBindRenderbuffer(GL_RENDERBUFFER, bufBinary);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, widthRender, heightRender);

	glGenFramebuffers(1, &fboBinary);
	glBindFramebuffer(GL_FRAMEBUFFER, fboBinary);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, bufBinary);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "Not properly installed FBO for binary" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Define Depth framebuffer
	glGenRenderbuffers(1, &bufDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, bufDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, widthRender, heightRender);

	glGenFramebuffers(1, &fboDepth);
	glBindFramebuffer(GL_FRAMEBUFFER, fboDepth);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, bufDepth);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "Not properly installed FBO for depth" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Define Depth FBO Visualiser
	glGenRenderbuffers(1, &bufDepthVis);
	glBindRenderbuffer(GL_RENDERBUFFER, bufDepthVis);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, mDepth->width(), mDepth->height());

	glGenFramebuffers(1, &fboDepthVis);
	glBindFramebuffer(GL_FRAMEBUFFER, fboDepthVis);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, bufDepthVis);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "Not properly installed FBO for depth visualisation" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Load shaders
	vector<string> nameShaders;
	nameShaders.push_back("default");
	nameShaders.push_back("phong");
	nameShaders.push_back("depth");
	nameShaders.push_back("ortho");
	nameShaders.push_back("background");
	nameShaders.push_back("labelling");
	loadShaders(nameShaders);
	
	// First of all create an empty texture (transparent) used by material based objs in the shaders
	emptyTex = Texture::loadEmptyTexture();
	texBackground = Texture::loadEmptyTexture();
	
	// Create empty background image with a quad
	backgroundQuad = createQuad(programShaders[TYPE_SHADER::BACKGROUND], DRAW_TYPE::SOLID);
	backgroundQuad->getVisualEntity(0).setTextureId(texBackground);
	// backgroundQuad->setScaling(2.0f, 2.0f, 2.0f);
	// backgroundQuad->setTranslation(0.0f, 0.0f, 1.0f);
	listModels.pop_back();
	listImgBB.pop_back();

	/*
	// - TEST INIT
	// Light Drawing
	vector<Model*> mSphereLights;
	mSphereLights.resize(Lights::MAX_LIGHTS+1);
	// Last light (+1) is a dummy centred object to not remove the last light when loading a real model
	for(unsigned int i = 0; i < Lights::MAX_LIGHTS + 1; ++i)
	{
		mSphereLights[i] = createObj("Sphere", programShaders[TYPE_SHADER::FLAT], 1.0f, 1.0f, 0.8f);
		mSphereLights[i]->setScaling(0.1f, 0.1f, 0.1f);
		//listModels.push_back(mSphereLights[i]);
		//listImgBB.push_back(BB());
	}

	// Draw ground
	Model* ground = createQuad(programShaders[TYPE_SHADER::PHONG], DRAW_TYPE::SOLID, string("M:/PhD/Code/Support/mRender/data/images/dog.png"), true);
	ground->setRotation(90.0f, 0.0f, 0.0f);
	ground->setScaling(2.5f, 2.5f, 2.5f);
	ground->setTranslation(0.0f, -1.0f, -3.0f);

	// Draw basic objects
	Model* offsetSphere = createSphere(programShaders[TYPE_SHADER::PHONG], 0.1f, 0.9f, 0.2f);
	offsetSphere->setObject(true);
	offsetSphere->setTranslation(-2.0f, -0.5f, -3.0f);
	Model* centreSphere = createSphere(programShaders[TYPE_SHADER::PHONG]);
	centreSphere->setObject(true);
	centreSphere->setScaling(1.0f/centreSphere->getBB().getSizeX(), 1.0f/centreSphere->getBB().getSizeY(), 1.0f/centreSphere->getBB().getSizeZ());
	// - TEST END
	*/

	// Draw brush (edit mode)
	brush = createObj("Cylinder", programShaders[TYPE_SHADER::ORTHO]);
	brush->setDrawType(DRAW_TYPE::LINES);
	brush->setTranslation(2.0f,2.0f,2.0f);
	
	// UI Components and their ortho projection
	uiQuad = createQuad(programShaders[TYPE_SHADER::ORTHO], DRAW_TYPE::LINES);
	listModels.pop_back();
	listImgBB.pop_back();
	uiQuad->setPolygonColour(1.0, 1.0, 0.0);
	
	// Control of FPS
	mFPS = new QTimer(this);
	mTimer = new QTimer(this);
	connect(mFPS, SIGNAL(timeout()), this, SLOT(updateGL()));
	connect(mTimer, SIGNAL(timeout()), this, SLOT(computeFPS()));
	mFPS->setTimerType(Qt::PreciseTimer);
	mTimer->setTimerType(Qt::PreciseTimer);
	countFPS = 0;
	mFPS->start(10); // ca. 60FPS
	mTimer->start(1000); // triggered once per second

	// Enable blending
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Render::updateViewMatrix()
{
	if(!isSampling)
		if(isFreeCamera)
			view = glm::lookAt(cam.pos, cam.pos+cam.dir, vuv);
		else
			view = glm::lookAt(cam.fixedPos, glm::vec3(), glm::vec3(0.0f, 1.0f, 0.0f));
	else
	{
		glm::vec3 vuv = glm::rotate(glm::vec3(0.0f, 1.0f, 0.0f), (float)mSampler->getTilt(), glm::vec3(0.0f, 0.0f, 1.0f));;
		view = glm::lookAt(cam.fixedPos, glm::vec3(), vuv);
	}

	GLint uniView = glGetUniformLocation(currentShader, "view");
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
}

void Render::updateProjectionMatrix()
{
	proj = glm::perspective(cam.fov, cam.ar, cam.nearPlane, cam.farPlane);
	GLint uniProj = glGetUniformLocation(currentShader, "proj");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));
}

void Render::setUpLights()
{
	GLuint lightPosShader = glGetUniformLocation(currentShader, "lightPosition");
	// glUniform3fv(lightPosShader, 1, glm::value_ptr(mLights.position[0]));
	glUniform3fv(lightPosShader, Lights::MAX_LIGHTS, (GLfloat*)mLights.position);

	GLint attShader = glGetUniformLocation(currentShader, "lightAttenuation");
	glUniform3fv(attShader, 1, glm::value_ptr(mLights.attenuation[0]));

	GLuint eyeLightShader = glGetUniformLocation(currentShader, "eyeLightPosition");
	glUniform3fv(eyeLightShader, 1, glm::value_ptr(cam.pos));

	GLuint ambientShader = glGetUniformLocation(currentShader, "ambientLight");
	glUniform3fv(ambientShader, 1, glm::value_ptr(mLights.ambient[0]));
	GLuint diffuseShader = glGetUniformLocation(currentShader, "diffuseLight");
	// glUniform3fv(diffuseShader, 1, glm::value_ptr(mLights.diffuse[0]));
	glUniform3fv(diffuseShader, Lights::MAX_LIGHTS, (GLfloat*)(mLights.diffuse));
	GLuint specularShader = glGetUniformLocation(currentShader, "specularLight");
	glUniform3fv(specularShader, 1, glm::value_ptr(mLights.specular[0]));

	//for(unsigned int i = 0; i < Lights::MAX_LIGHTS; ++i)
//		listModels[i]->setTranslation(mLights.position[i].x, mLights.position[i].y, mLights.position[i].z); // Commented when no light bulb is firstly added

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_LINE_SMOOTH);
}

void Render::computeFPS()
{
	// cout << "FPS: " << countFPS << endl;
	emit updateFPS(QString("FPS: %1").arg(countFPS));
	currentFPS = countFPS;
	countFPS = 0;
}

void Render::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
	updateGL();	
}

void Render::paintGL()
{
	// Update camera movement
	if (isFreeCamera)
	{
		// - Translation
		if (mKeyPressed[0])
			cam.pos += (cam.dir * (STEP_TRANS / currentFPS));
		if (mKeyPressed[1])
			cam.pos -= (cam.dir * (STEP_TRANS / currentFPS));
		if (mKeyPressed[2])
			cam.pos -= (glm::cross(cam.dir, vuv) * (STEP_TRANS / currentFPS));
		if (mKeyPressed[3])
			cam.pos += (glm::cross(cam.dir, vuv) * (STEP_TRANS / currentFPS));

		// - Rotation
		cam.dir = (STEP_ROT / currentFPS) * glm::vec3(1.0f, 1.0f, -1.0f) * glm::vec3(
			cos(radian(cam.rot.y)) * sin(radian(cam.rot.x)),
			sin(radian(cam.rot.y)),
			cos(radian(cam.rot.y)) * cos(radian(cam.rot.x))
			);

		// - Update VuV
		if (cam.rot.y >= 90 && cam.rot.y < 270)
			vuv = glm::vec3(0.0f, -1.0f, 0.0f);
		else
			vuv = glm::vec3(0.0f, 1.0f, 0.0f);
	}
	else
	{
		glm::mat4 fixedTrans;
		//fixedTrans = glm::rotate(fixedTrans, cam.azimuth, glm::vec3(0.0f, 1.0f, 0.0f));
		//fixedTrans = glm::rotate(fixedTrans, -cam.elevation, glm::vec3(1.0f, 0.0f, 0.0f));
		cam.fixedPos = glm::vec3(fixedTrans*glm::vec4(0.0f, 0.0f, cam.distance, 1.0f));
	}

	// Call object geometry to render
	if (isAntiAliasing)
		glEnable(GL_MULTISAMPLE);

	// First pass to estimate the optimal 2D bounding box
	computeBB2D();

	// Select and clean main multisampling buffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboRender);
	GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render keypoints (if tab selected) -> LEAVE IT HERE FOR STORING DEPTH IMAGES of KPS
	/*
	if (isSampling)
	{
		renderKps(); 

		// Anti-aliasing post processing (adding framebuffer output into the default window FB = 0)
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fboRender);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, widthRender, heightRender, 0, 0, widthRender, heightRender, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		// Copy depth information into depth-FBO
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fboRender);
		glReadBuffer(GL_COLOR_ATTACHMENT1);		
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboDepth);
		glBlitFramebuffer(0, 0, widthRender, heightRender, 0, 0, widthRender, heightRender, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		// Copy depth into visualisation window
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fboDepth);
		depthKps.clear();
		depthKps.resize(widthRender * heightRender * 4, 0);
		glReadPixels(0, 0, widthRender, heightRender, GL_RGBA, GL_FLOAT, depthKps.data());

		// Select and clean main multisampling buffer
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboRender);
		GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, attachments);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	*/

	// Render background image (if selected)
	renderBackground();

	// Real render pass scene
	glDrawBuffers(2, attachments);
	for (unsigned i = 0; i < listModels.size(); ++i)
		render(listModels[i], listModels[i]->getShader());

	// Render keypoints (if tab selected)
	if (isKpsMode && !listModels.empty())
		renderKps();

	// Render optimal 2D BBs (if selected)
	if (!isSampling)
		renderUI();

	// Render brush when Edit mode is activated)
	if (isEditMode && !isEditPixelMode)
	{
		brush->setScaling(0.01f*brushSize, 0.01f*brushSize, 0.02f*brushSize);
		renderBrush();
	}

	if(isAntiAliasing)
		glDisable(GL_MULTISAMPLE);

	// Anti-aliasing post processing (adding framebuffer output into the default window FB = 0)
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboRender);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, widthRender, heightRender, 0, 0, widthRender, heightRender, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// Copy depth information into depth-FBO
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboRender);
	glReadBuffer(GL_COLOR_ATTACHMENT1);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboDepth);
	glBlitFramebuffer(0, 0, widthRender, heightRender, 0, 0, widthRender, heightRender, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// Copy depth into visualisation window
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboDepth);
	glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
	depth.clear();
	depth.resize(widthRender * heightRender * 4, 0); 
	glReadPixels(0, 0, widthRender, heightRender, GL_RGBA, GL_FLOAT, depth.data());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboDepthVis);
	glBlitFramebuffer(0, 0, widthRender, heightRender, 0, 0, mDepth->width(), mDepth->height(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
	depthByte.clear();
	depthByte.resize(mDepth->width()*mDepth->height() * 4, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboDepthVis);
	glReadPixels(0, 0, mDepth->width(), mDepth->height(), GL_RGBA, GL_UNSIGNED_BYTE, depthByte.data());	
	mDepth->transferViewportImg(depthByte.data(), 0, 0);
	mDepth->updateGL();
	makeCurrent();

	countFPS++;
}

void Render::render(Model* obj, GLuint shader)
{
	GLuint saveShader = obj->getShader();

	// Apply linear transformations to normalise and center the objects at (0,0,0) with max dim size = 1.0
	model = glm::mat4();
	
	// Translate object to desired position
	if(isSampling)
		model  = glm::translate(model , glm::vec3(0.0f, 0.0f, cam.fixedPos.z - mSampler->getDistance()));
	else
		model  = glm::translate(model , glm::vec3(obj->getTX(), obj->getTY(), obj->getTZ()));

	// Rotation
	float rx = obj->getRX();
	float ry = -obj->getRY();
	float rz = obj->getRZ();
	if(isSampling)
	{
		ry = -(float)mSampler->getCurrentAngleY();
		rx = (float)mSampler->getAngleX();
	}
	else if(!isFreeCamera && !isSampling)
	{
		ry -= cam.azimuth;
		rx -= cam.elevation;
	}

	model  = glm::rotate(model, rx, glm::vec3(1.0f, 0.0f, 0.0f));
	model  = glm::rotate(model, ry, glm::vec3(0.0f, 1.0f, 0.0f));
	model  = glm::rotate(model, rz, glm::vec3(0.0f, 0.0f, 1.0f));

	// Scaling
	float scale = max(max(obj->getBB().getSizeX(), obj->getBB().getSizeY()), obj->getBB().getSizeZ());
	model  = glm::scale(model, (1.0f/scale)*glm::vec3(obj->getSX(), obj->getSY(), obj->getSZ()));

	// Bring model to origin
	float *centre = obj->getBB().getCenter();
	model  = glm::translate(model , glm::vec3(-centre[0], -centre[1], -centre[2]));

	if(isLabel && isEditMode)
	{
		currentShader = getShader(LABELLING);
		obj->setShader(currentShader);
		glUseProgram(currentShader);

		GLuint uniModelLabel = glGetUniformLocation(currentShader, "model");
		glUniformMatrix4fv(uniModelLabel, 1, GL_FALSE, glm::value_ptr(model));
		setUpLights();
		updateViewMatrix();
		updateProjectionMatrix();

		obj->renderLabelling();
	}

	// Loop throughout all models and visualise them!
	currentShader = shader;
	obj->setShader(currentShader);
	glUseProgram(currentShader); // Now this shader program is used in the rendering

	GLint uniModel = glGetUniformLocation(currentShader, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	// Lighting
	setUpLights();
	// Camera information
	// - Camera coordinate transformation
	updateViewMatrix();
	// - Screen coordinate transformation
	updateProjectionMatrix();

	if(isEditMode)
		obj->renderParent();
	else
		obj->render();
	
	currentShader = saveShader;
	obj->setShader(currentShader);
}

void Render::renderUI()
{	
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	// Draw BB in main window
	if(isViewBoundingBox)
	{
		for(unsigned int i = 0; i < listImgBB.size(); ++i)
			if(listModels[i]->isObjectScenario())
			{
				BB bb = listImgBB[i];

				float newSx = bb.getSizeX() / widthRender * 2.0f;
				float newSy = bb.getSizeY() / heightRender * 2.0f;
				uiQuad->setScaling(newSx, newSy, 1.0f);

				float newX = (bb.getCenter()[0] - (widthRender/2.0f)) / (widthRender/2.0f);
				float newY = (bb.getCenter()[1] - (heightRender/2.0f)) / (heightRender/2.0f);
				uiQuad->setTranslation(newX, newY, 0.0f);

				currentShader = uiQuad->getShader();
				glUseProgram(currentShader);

				model = glm::mat4();
				model = glm::translate(model, glm::vec3(uiQuad->getTX(), uiQuad->getTY(), uiQuad->getTZ()));
				model = glm::rotate(model, uiQuad->getRX(), glm::vec3(1.0f, 0.0f, 0.0f));
				model = glm::rotate(model, uiQuad->getRY(), glm::vec3(0.0f, 1.0f, 0.0f));
				model = glm::rotate(model, uiQuad->getRZ(), glm::vec3(0.0f, 0.0f, 1.0f));
				model = glm::scale(model, glm::vec3(uiQuad->getSX(), uiQuad->getSY(), uiQuad->getSZ()));
				GLint uniModel = glGetUniformLocation(currentShader, "model");
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

				uiQuad->render();
			}
	}
}

void Render::renderBackground()
{
	glDisable(GL_DEPTH_TEST);
	// glDisable(GL_CULL_FACE);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	currentShader = backgroundQuad->getShader();
	glUseProgram(currentShader);

	GLint uniOrtho = glGetUniformLocation(currentShader, "proj");
	orthoProj = glm::ortho(0.0f, (float)width(), 0.0f, (float)height());
	glUniformMatrix4fv(uniOrtho, 1, GL_FALSE, glm::value_ptr(orthoProj));

	model = glm::mat4();

	// Back to the corner
	model = glm::translate(model, glm::vec3(0.5f*(float)width(), 0.5f*(float)height(), 0.0f));

	model = glm::translate(model, glm::vec3(backgroundQuad->getTX(), backgroundQuad->getTY(), backgroundQuad->getTZ()));
	model = glm::rotate(model, backgroundQuad->getRX(), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, backgroundQuad->getRY(), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, backgroundQuad->getRZ(), glm::vec3(0.0f, 0.0f, 1.0f));

	float ratioWindow = 1.0f;
	if(backgroundWidth >= backgroundHeight)
		// ratioWindow = (float)width() / (float)backgroundWidth;
		ratioWindow = (float)height() / (float)backgroundHeight;
	else
		ratioWindow = (float)width() / (float)backgroundWidth;
		// ratioWindow = (float)height() / (float)backgroundHeight;
	model = glm::scale(model, glm::vec3(ratioWindow*backgroundQuad->getSX()*(float)backgroundWidth, ratioWindow*backgroundQuad->getSY()*(float)backgroundHeight, 1.0f));

	// Move to the center
	// model = glm::translate(model, glm::vec3(-0.5f, -0.5f, 0.0f));

	GLint uniModel = glGetUniformLocation(currentShader, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	backgroundQuad->render();

	glEnable(GL_DEPTH_TEST);
	// glEnable(GL_CULL_FACE);
}

void Render::renderBrush()
{
	glDisable(GL_DEPTH_TEST);
	// glDisable(GL_CULL_FACE);

	currentShader = brush->getShader();
	glUseProgram(currentShader);

	// Apply linear transformations to normalise and center the objects at (0,0,0) with max dim size = 1.0
	model = glm::mat4();
	model  = glm::translate(model , glm::vec3(brush->getTX(), brush->getTY(), brush->getTZ()));
	model  = glm::scale(model , glm::vec3(brush->getSX(), brush->getSY(), brush->getSZ()));

	GLint uniModel = glGetUniformLocation(currentShader, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	brush->render();

	glEnable(GL_DEPTH_TEST);
}

void Render::renderKps()
{
	if (!isKpsSelfOcc)
	{
		glDisable(GL_DEPTH_TEST);
	}

	Model* obj = listModels.back();
	for (map<string, Model*>::iterator it = model_kps.begin(); it != model_kps.end(); ++it)
	{
		Model* currentKp = it->second;
		model = glm::mat4();
		if (isSampling)
			model = glm::translate(model, glm::vec3(0.0f, 0.0f, cam.fixedPos.z - mSampler->getDistance()));

		float rx = obj->getRX();
		float ry = -obj->getRY();
		float rz = obj->getRZ();
		if (isSampling)
		{
			ry = -(float)mSampler->getCurrentAngleY();
			rx = (float)mSampler->getAngleX();
		}
		else if (!isFreeCamera && !isSampling)
		{
			ry -= cam.azimuth;
			rx -= cam.elevation;
		}
		model = glm::rotate(model, rx, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, ry, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, rz, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::translate(model, glm::vec3(currentKp->getTX(), currentKp->getTY(), currentKp->getTZ()));
		model = glm::scale(model, glm::vec3(currentKp->getSX(), currentKp->getSY(), currentKp->getSZ()));
		float *centre = currentKp->getBB().getCenter();
		model = glm::translate(model, glm::vec3(-centre[0], -centre[1], -centre[2]));

		currentShader = currentKp->getShader();
		glUseProgram(currentShader);
		GLint uniModel = glGetUniformLocation(currentShader, "model");
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

		setUpLights();
		updateViewMatrix();
		updateProjectionMatrix();
		currentKp->render();
	}

	if (!isKpsSelfOcc)
	{
		glEnable(GL_DEPTH_TEST);
	}
}

void Render::computeBB2D()
{
	binaryView.resize(widthRender*heightRender*4);
	BB *bb2D, *bb3D;
	for(unsigned i = 0; i < listModels.size(); ++i)
		if(listModels[i]->isObjectScenario())
		{
			// Store the binary mask into an array
			glBindFramebuffer(GL_FRAMEBUFFER, fboBinary);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			render(listModels[i], programShaders[TYPE_SHADER::DEPTH]);
			glReadPixels(0, 0, widthRender, heightRender, GL_RGBA, GL_UNSIGNED_BYTE, binaryView.data());

			// ProjectModelView matrix 
			glm::mat4 projViewModelMat;
			projViewModelMat = proj*view*model;
			// cout << glm::to_string(proj) << endl << endl;
			// cout << glm::to_string(view) << endl << endl;
			// cout << glm::to_string(model) << endl << endl << endl;

			// Project all 3D BB vertices and get most external X and Y projections to shorten the optimal 2D BB search
			bb3D = &listModels[i]->getBB();
			float limX0, limY0, limX1, limY1;
			limX0 = limY0 =  1.0f;
			limX1 = limY1 = -1.0f;
			glm::vec4 proj2D;
			for(unsigned int z = 0; z < 2; ++z)
				for(unsigned int y = 0; y < 2; ++y)
					for(unsigned int x = 0; x < 2; ++x)
					{
						int idx = z*4 + y*2 + x;
						switch(idx)
						{
							case 0:
								proj2D = projViewModelMat * glm::vec4(bb3D->getX0(), bb3D->getY0(), bb3D->getZ0(), 1);
								break;
							case 1:
								proj2D = projViewModelMat * glm::vec4(bb3D->getX0(), bb3D->getY0(), bb3D->getZ1(), 1);
								break;
							case 2:
								proj2D = projViewModelMat * glm::vec4(bb3D->getX0(), bb3D->getY1(), bb3D->getZ0(), 1);
								break;
							case 3:
								proj2D = projViewModelMat * glm::vec4(bb3D->getX0(), bb3D->getY1(), bb3D->getZ1(), 1);
								break;
							case 4:
								proj2D = projViewModelMat * glm::vec4(bb3D->getX1(), bb3D->getY0(), bb3D->getZ0(), 1);
								break;
							case 5:
								proj2D = projViewModelMat * glm::vec4(bb3D->getX1(), bb3D->getY0(), bb3D->getZ1(), 1);
								break;
							case 6:
								proj2D = projViewModelMat * glm::vec4(bb3D->getX1(), bb3D->getY1(), bb3D->getZ0(), 1);
								break;
							case 7:
								proj2D = projViewModelMat * glm::vec4(bb3D->getX1(), bb3D->getY1(), bb3D->getZ1(), 1);
								break;
						}
						
						if(proj2D.x /  proj2D.w < limX0)
							limX0 = proj2D.x /  proj2D.w;
						else if(proj2D.x /  proj2D.w > limX1)
							limX1 = proj2D.x /  proj2D.w;

						if(proj2D.y / proj2D.w < limY0)
							limY0 = proj2D.y /  proj2D.w;
						else if(proj2D.y /  proj2D.w > limY1)
							limY1 = proj2D.y /  proj2D.w;
					}

			// Estimate bounding box from depth
			bb2D = &listImgBB[i];
			bb2D->reset();
			int pxlX0 = floor(max(0.0f, (widthRender/2.0f)*limX0 + (widthRender/2.0f)));
			int pxlY0 = floor(max(0.0f, (heightRender/2.0f)*limY0 + (heightRender/2.0f)));
			int pxlX1 = ceil(min((float)widthRender, (widthRender/2.0f)*limX1 + (widthRender/2.0f)));
			int pxlY1 = ceil(min((float)heightRender, (heightRender/2.0f)*limY1 + (heightRender/2.0f)));
			for(int row = pxlY0; row < pxlY1; ++row)
				for(int col = pxlX0; col < pxlX1; ++col)
				{
					unsigned int idx = 4*(row*heightRender+ col);
					if(binaryView[idx] != 0)
					{
						if(row < bb2D->getY0())
							bb2D->setY0(row);
						else if(row > bb2D->getY1())
							bb2D->setY1(row);

						if(col < bb2D->getX0())
							bb2D->setX0(col);
						else if(col > bb2D->getX1())
							bb2D->setX1(col);
					}
				}

			bb2D->updateCenter();
		}
}

bool Render::loadModelFromFile(const string& fileName, GLuint shader)
{
	// Remove previous object
	std::map<std::string, Kp> old_kps;
	if(!listModels.empty())
	{
		Model* modelToClean = listModels.back();
		old_kps = modelToClean->getKps();
		listModels.pop_back();
		delete modelToClean;
		listImgBB.pop_back();
	}

	Model* mModel = new Model(shader);
	mModel->setObject(true);

	bool isLoaded = mModel->loadModelFromFile(fileName);

	if(!isLoaded)
	{
		cout << "Failed to load the model from a file" << endl;
		if(!listModels.empty())
		{
			Model* modelToClean = listModels.back();
			listModels.pop_back();
			delete modelToClean;
			listImgBB.pop_back();
		}
		return false;
	}
	else
	{
		cout << "New model successfully loaded" << endl;
		// Resize to the unit and center
		listModels.push_back(mModel);
		listImgBB.push_back(BB());

		// If it has no Kps, keep old ones (remove when all annotated)
		if(mModel->getKps().empty())
			mModel->setKps(old_kps);
	}
	return true;
}

void Render::saveSegmentationToFile(ofstream& segmentationFile, vector<unsigned int>& treePath)
{
	Entity node = *getModel()->getTreeNode(treePath);

	// Store vertices and normals
	for(unsigned int v = 0; v < node.getListVertices().size(); ++v)
	{
		segmentationFile << "v " << setprecision(10) << node.getListVertices()[v].getPosition()[0] << " " << node.getListVertices()[v].getPosition()[1] << " " << node.getListVertices()[v].getPosition()[2] << endl;
		segmentationFile << "n " << setprecision(10) << node.getListVertices()[v].getNormal()[0] << " " << node.getListVertices()[v].getNormal()[1] << " " << node.getListVertices()[v].getNormal()[2] << endl;
		segmentationFile << "t " << setprecision(10) << node.getListVertices()[v].getTexcoord()[0] << " " << node.getListVertices()[v].getTexcoord()[1] << endl;
	}

	// Store faces
	for(unsigned int f = 0; f < node.getListFaceIndices().size(); ++f)
		segmentationFile << "f " << node.getListFaceIndices()[f].getFaceIdx1() << " " << node.getListFaceIndices()[f].getFaceIdx2() << " " << node.getListFaceIndices()[f].getFaceIdx3() << endl;
}

bool Render::loadSegmentationFromFile(ifstream& segmentationFile, vector<unsigned int>& treePath, float r, float g, float b)
{
	if(treePath.empty())
		getModel()->resetTree();
	else
	{
		vector<unsigned int> parentPath(treePath);
		parentPath.pop_back();
		getModel()->addTreePart(parentPath);
	}
	Tree<Entity>::sibling_iterator itTree = getModel()->getTreeNode(treePath);

	// Set color
	itTree->setAmbient(r,g,b);

	// Retrieve vertices, normals and finally faces
	string line;
	bool isFinished = false;
	Vertex v;
	Face f;
	QStringList listPoints;
	QString delimiters(" ");
	while(getline(segmentationFile, line) && !isFinished)
	{
		// cout << line << endl;
		switch(line[0])
		{
			case 'v':
				listPoints = QString(line.erase(0,2).c_str()).split(delimiters);
				v.setPosition(atof(listPoints[0].toStdString().c_str()), atof(listPoints[1].toStdString().c_str()), atof(listPoints[2].toStdString().c_str()));
				break;
			case 'n':
				listPoints = QString(line.erase(0,2).c_str()).split(delimiters);
				v.setNormal(atof(listPoints[0].toStdString().c_str()), atof(listPoints[1].toStdString().c_str()), atof(listPoints[2].toStdString().c_str()));
				break;
			case 't':
				listPoints = QString(line.erase(0,2).c_str()).split(delimiters);
				v.setTexcoord(atof(listPoints[0].toStdString().c_str()), atof(listPoints[1].toStdString().c_str()));
				itTree->getListVertices().push_back(v);
				break;
			case 'f':
				listPoints = QString(line.erase(0,2).c_str()).split(delimiters);
				f.setFace(atoi(listPoints[0].toStdString().c_str()), atoi(listPoints[1].toStdString().c_str()), atoi(listPoints[2].toStdString().c_str()));
				itTree->getListFaceIndices().push_back(f);
				break;
			default:
				isFinished = true;
				getModel()->updateLabels(*itTree);
				break;
		}
	}

	return true;
}

void Render::loadBackgroundImgFromFile(const std::string& fileName)
{
	backgroundQuad->getVisualEntity(0).setTexturePath(fileName);
	glBindTexture(GL_TEXTURE_2D, backgroundQuad->getVisualEntity(0).getTextureId());
	if(fileName != "")
	{
		unsigned char* image = SOIL_load_image(fileName.c_str(), &backgroundWidth, &backgroundHeight, 0, SOIL_LOAD_RGBA);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, backgroundWidth, backgroundHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		SOIL_free_image_data(image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &backgroundWidth);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &backgroundHeight);
		vector<unsigned char> pixels(backgroundWidth*backgroundHeight*4, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, backgroundWidth, backgroundHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

Model* Render::createQuad(GLuint shader, DRAW_TYPE drawType, string texturePath, bool isAbsolute)
{
	// - Geometry: (X, Y, Z,	R, G, B, A		U, V		Nx, Ny, Nz)
	float vertices[] =
	{
		 0.5f,  0.5f, 0.0f,		1.0f, 0.0f, 0.0f, 1.0f,		1.0f, 0.0f,		0.0f, 0.0f, 1.0f,	// Vertex 1
		 0.5f, -0.5f, 0.0f,		0.0f, 1.0f, 0.0f, 1.0f,		1.0f, 1.0f,		0.0f, 0.0f, 1.0f,	// Vertex 2
		-0.5f, -0.5f, 0.0f,		0.0f, 0.0f, 1.0f, 1.0f,		0.0f, 1.0f,		0.0f, 0.0f, 1.0f,	// Vertex 3
		-0.5f,  0.5f, 0.0f,		0.5f, 0.5f, 0.5f, 1.0f,		0.0f, 0.0f,		0.0f, 0.0f, 1.0f	// Vertex 4
	};

	GLuint faces[] =
	{
		0, 1, 2,	// Face 1
		0, 2, 3		// Face 2
	};

	GLuint facesLine[] = { 0, 1, 1, 2, 2, 3, 3, 0};
	// Define our mesh
	Model* newModel = new Model(shader);
	newModel->setAllVertices((Vertex*)vertices, 4);
	Entity raw_entity;
	raw_entity.getListVertices().resize(4);
	for(unsigned int i = 0; i < 4; ++i)
		raw_entity.setVertex(newModel->getListAllVertices()[i], i);
	if(drawType == DRAW_TYPE::SOLID)
		raw_entity.getListFaceIndices().insert(raw_entity.getListFaceIndices().begin(), (Face*)faces, (Face*)faces+2);
	else if(drawType == DRAW_TYPE::LINES)
		raw_entity.getListFaceIndices().insert(raw_entity.getListFaceIndices().begin(), (Face*)facesLine, (Face*)facesLine+2);

	newModel->loadRawEntity(raw_entity);
	if(texturePath == "")
		newModel->attachTexture(emptyTex);
	else
	{
		if(!isAbsolute)
			texturePath = newModel->getFullPath(texturePath);
		Texture tex = Texture::loadTexture(texturePath);
		newModel->attachTexture(tex.getId(), tex.getPath());
	}
	newModel->updateBB();
	newModel->setDrawType(drawType);
	listModels.push_back(newModel);
	listImgBB.push_back(BB());
	return newModel;
}

Model* Render::createObj(const char* objStr, GLuint shader, float r, float g, float b)
{
	// Define our mesh
	Model* newModel = new Model(shader);
	newModel->loadModelFromFile(PATH_OBJ + "/" + objStr + ".obj");
	for(unsigned int i = 0; i < newModel->getVisualEntity(0).getListVertices().size(); ++i)
		newModel->getVisualEntity(0).getListVertices()[i].setColour(r, g, b, 1.0f);
	newModel->updateMeshes();
	newModel->updateBB();
	newModel->attachTexture(emptyTex);
	return newModel;
}

void Render::loadShaders(vector<string> nameShaders)
{
	for(unsigned i = 0; i < nameShaders.size(); ++i)
	{
		string fileName= ":/shaders/";
		fileName.append(nameShaders[i]);

		// - Vertex Shader
		QFile vertexFile, fragmentFile;
		string vertexFileName = fileName;
		vertexFile.setFileName(vertexFileName.append(".vert").c_str());
		if (!vertexFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			qWarning() << "Cannot open vertex shader file!" << endl;
			exit(EXIT_FAILURE);
		}
		QByteArray vertexData = vertexFile.readAll();
		vertexFile.close();
		const GLchar* vertexSource = vertexData.data();
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexSource, NULL);
		glCompileShader(vertexShader);

		// - Fragment Shader
		string fragmentFileName = fileName;
		fragmentFile.setFileName(fragmentFileName.append(".frag").c_str());
		if (!fragmentFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			qWarning() << "Cannot open fragment shader file!" << endl;
			exit(EXIT_FAILURE);
		}
		QByteArray fragmentData = fragmentFile.readAll();
		fragmentFile.close();
		const GLchar* fragmentSource = fragmentData.data();
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
		glCompileShader(fragmentShader);

		// Check whether a shader has successfully been compiled
		GLint statusV, statusF;
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &statusV);
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &statusF);
		if(statusV == GL_TRUE && statusF == GL_TRUE)
			cout << "Shader " << nameShaders[i] << " loaded: OK!" << endl;
		else
		{
			cout << "Shader " << nameShaders[i] << " loaded: NO!" << endl;
			char buffer[512];
			glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
			cout << "REASON: " << buffer << endl;
		}

		// Create final shader workflow
		programShaders.push_back(glCreateProgram());
		glAttachShader(programShaders[i], vertexShader);
		glAttachShader(programShaders[i], fragmentShader);

		// Associate shader input (attribute)
		glBindAttribLocation(programShaders[i], SHADER_IN::position, "position");
		glBindAttribLocation(programShaders[i], SHADER_IN::colour, "colour");
		glBindAttribLocation(programShaders[i], SHADER_IN::texcoord, "texcoord");
		glBindAttribLocation(programShaders[i], SHADER_IN::normal, "normal");
	
		// Associate shader output
		glBindFragDataLocation(programShaders[i], 0, "outColour");
		if (i != TYPE_SHADER::DEPTH)
			glBindFragDataLocation(programShaders[i], 1, "outDepth");

		glLinkProgram(programShaders[i]);
		glUseProgram(programShaders[i]); // only 1 program active at a time

		// Associate variable connection code <-> shader (uniform)
		// GLint uniColor = glGetUniformLocation(shaderProgram, "triangleColor");
		// glUniform3f(uniColor, 1.0f, 0.0f, 0.0f);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
	}
}

void Render::keyPressEvent(QKeyEvent *event)
{
	switch(event->key())
	{
		case Qt::Key_W:
			mKeyPressed[0] = true;
			break;
		case Qt::Key_S:
			mKeyPressed[1] = true;
			break;
		case Qt::Key_A:
			mKeyPressed[2] = true;
			break;
		case Qt::Key_D:
			mKeyPressed[3] = true;
			break;
		default:
			break;
	}
}

void Render::keyReleaseEvent(QKeyEvent *event)
{
	switch(event->key())
	{
		case Qt::Key_W:
			mKeyPressed[0] = false;
			break;
		case Qt::Key_S:
			mKeyPressed[1] = false;
			break;
		case Qt::Key_A:
			mKeyPressed[2] = false;
			break;
		case Qt::Key_D:
			mKeyPressed[3] = false;
			break;
		default:
			break;
	}
}

void Render::mousePressEvent(QMouseEvent *event)
{
	mMousePos.x = event->pos().x();
	mMousePos.y = event->pos().y();

	if(isEditMode && (event->button() == Qt::LeftButton || event->button() == Qt::RightButton))
	{
		// Normalise pixel coordinates
		float normX = (mMousePos.x - widthRender * 0.5) / (widthRender * 0.5);
		float normY = ((heightRender - mMousePos.y) - (heightRender * 0.5)) / (heightRender * 0.5);
		//cout << normX << " " << normY << endl;

		// Ray position
		glm::vec3 rayWorldPos = cam.fixedPos;
		// cout << "Pos Camera: " << rayWorldPos.x << " " << rayWorldPos.y << " " << rayWorldPos.z << endl;

		// Ray direction
		glm::vec4 rayDir = glm::vec4(normX, normY, -1.0f, 1.0f);
		glm::vec4 rayEyeDir = glm::inverse(proj) * rayDir;
		rayEyeDir = glm::vec4(rayEyeDir.x, rayEyeDir.y, -1.0f, 0.0f);
		glm::vec4 rayWorldDir_aux = glm::inverse(view) * rayEyeDir;
		glm::vec3 rayWorldDir = glm::normalize(glm::vec3(rayWorldDir_aux.x, rayWorldDir_aux.y, rayWorldDir_aux.z));
		// cout << "Dir Camera: " << rayWorldDir.x << " " << rayWorldDir.y << " " << rayWorldDir.z << endl;

		Model* obj = listModels.back();
		model = glm::mat4();
		float *centre = obj->getBB().getCenter();
		float scale = max(max(obj->getBB().getSizeX(), obj->getBB().getSizeY()), obj->getBB().getSizeZ());
		model  = glm::translate(model , glm::vec3(obj->getTX(), obj->getTY(), obj->getTZ()));
		model  = glm::rotate(model , obj->getRX(), glm::vec3(1.0f, 0.0f, 0.0f));
		model  = glm::rotate(model , obj->getRY(), glm::vec3(0.0f, 1.0f, 0.0f));
		model  = glm::rotate(model , obj->getRZ(), glm::vec3(0.0f, 0.0f, 1.0f));
		model  = glm::scale(model , (1.0f/scale)*glm::vec3(obj->getSX(), obj->getSY(), obj->getSZ()));
		model  = glm::translate(model , glm::vec3(-centre[0], -centre[1], -centre[2]));

		Entity e = Entity();
		vector<unsigned int> pathParent(obj->getCurrentTreeNode());
		if(!pathParent.empty())
		{
			pathParent.pop_back();
			e = (*obj->getTreeNode(pathParent));
		}
		
		// Check intersection with all mesh triangles
		glm::vec3 v0, v1, v2;  // triangle vertices
		glm::vec3 u, v, n; // triangle vectors
		glm::vec3 dir, w0, w; // ray vectors
		dir = glm::normalize(rayWorldDir);
		float r, a, b; // params to calm ray-plane intersect
		glm::vec3 I;

		vector<unsigned int> faceCandidates; vector<float> faceDist;
		for(unsigned int idxFace = 0; idxFace < e.getListFaceIndices().size(); ++idxFace)
		{
			if(!isEditPixelMode)
			{
				Face f = e.getFace(idxFace);

				// Check if normal place of the face is looking towards the camera (angle cam dir and normal)
				float* n1 = e.getVertex(f.getFaceIdx1()).getNormal();
				float* n2 = e.getVertex(f.getFaceIdx2()).getNormal();
				float* n3 = e.getVertex(f.getFaceIdx3()).getNormal();
				glm::vec3 faceNorm = glm::vec3((n1[0] + n2[0] + n3[0]) / 3.0f, (n1[1] + n2[1] + n3[1]) / 3.0f, (n1[2] + n2[2] + n3[2]) / 3.0f);
				// float normNorm = sqrt(pow(faceNorm.x,2) + pow(faceNorm.y,2) + pow(faceNorm.z,2));
				//float dirNorm = sqrt(pow(dir.x,2) + pow(dir.y,2) + pow(dir.z,2));
				float face2face = acos(glm::dot(dir, faceNorm));
				if(face2face < 1.5f) // && face2face > 3.141592f*2.0f-2.0f) // 
					continue;

				// Check if the centre of the face is within the cylinder
				v0 = glm::vec3(model*glm::vec4(e.getVertex(f.getFaceIdx1()).getPosition()[0], e.getVertex(f.getFaceIdx1()).getPosition()[1], e.getVertex(f.getFaceIdx1()).getPosition()[2], 1.0));
				v1 = glm::vec3(model*glm::vec4(e.getVertex(f.getFaceIdx2()).getPosition()[0], e.getVertex(f.getFaceIdx2()).getPosition()[1], e.getVertex(f.getFaceIdx2()).getPosition()[2], 1.0));
				v2 = glm::vec3(model*glm::vec4(e.getVertex(f.getFaceIdx3()).getPosition()[0], e.getVertex(f.getFaceIdx3()).getPosition()[1], e.getVertex(f.getFaceIdx3()).getPosition()[2], 1.0));
				glm::vec3 centreFace = glm::vec3((v0[0] + v1[0] + v2[0]) / 3.0f, (v0[1] + v1[1] + v2[1]) / 3.0f, (v0[2] + v1[2] + v2[2]) / 3.0f);
			
				glm::vec3 vecV = (cam.fixedPos + 10.0f*dir) - cam.fixedPos;
				glm::vec3 vecW = centreFace- cam.fixedPos;

				float c1 = glm::dot(vecW, vecV);
				float c2 = glm::dot(vecV, vecV);
				float b = c1 / c2;
				glm::vec3 Pb = cam.fixedPos + b * vecV;
				float distNorm = sqrt(pow(centreFace.x - Pb.x,2) + pow(centreFace.y - Pb.y,2) + pow(centreFace.z - Pb.z,2));

				// Potential candidate?
				float scalingValue = 1.0f;
				if (distNorm < 0.005f*brushSize*scalingValue*cam.distance)
				{
					// Append new candidate
					faceCandidates.push_back(idxFace);
					glm::vec3 centerFace = glm::vec3( (v0.x + v1.x + v2.x) / 3.0f, (v0.y + v1.y + v2.y) / 3.0f, (v0.z + v1.z + v2.z) / 3.0f );
					faceDist.push_back(pow(rayWorldPos.x - centerFace.x, 2) + pow(rayWorldPos.y - centerFace.y, 2) + pow(rayWorldPos.z - centerFace.z, 2));
					continue;
				}
			}
			else // Pixel Mode
			{

				// Method for line (not cylinder)
				Face f = e.getFace(idxFace);
				v0 = glm::vec3(model*glm::vec4(e.getVertex(f.getFaceIdx1()).getPosition()[0], e.getVertex(f.getFaceIdx1()).getPosition()[1], e.getVertex(f.getFaceIdx1()).getPosition()[2], 1.0));
				v1 = glm::vec3(model*glm::vec4(e.getVertex(f.getFaceIdx2()).getPosition()[0], e.getVertex(f.getFaceIdx2()).getPosition()[1], e.getVertex(f.getFaceIdx2()).getPosition()[2], 1.0));
				v2 = glm::vec3(model*glm::vec4(e.getVertex(f.getFaceIdx3()).getPosition()[0], e.getVertex(f.getFaceIdx3()).getPosition()[1], e.getVertex(f.getFaceIdx3()).getPosition()[2], 1.0));

				u = v1 - v0;
				v = v2 - v0;

				// Cross Product: n = u * v [n = glm::vec3(v1.y*v0.z - v1.z*v0.y, v1.z*v0.x - v1.x*v0.z, v1.x*v0.y - v1.y*v0.x);]
				n = glm::cross(u, v);

				if(n.length() == 0)
					continue;

				w0 =  rayWorldPos - v0;
				a = -glm::dot(n, w0);
				b = glm::dot(n, dir);

			
				if(fabs(b) < 0.00000001) // ray is parallel to triangle plane
				{
					if(a == 0) // ray lies in triangle plane
						continue;
					else // ray disjoint from plane
						continue;
				}
			
				// get intersect point of ray with triangle plane
				r = a / b;
				if(r < 0.0f)
					continue; // no intersection

				I = rayWorldPos + r*dir;

				// Is the plane intersection within the triangle defined by the vertices?
				float uu, uv, vv, wu, wv, D;
				uu = glm::dot(u,u);
				uv = glm::dot(u,v);
				vv = glm::dot(v,v);
				w = I - v0;
				wu = glm::dot(w,u);
				wv = glm::dot(w,v);
				D = uv * uv - uu * vv;

				// Get and test parametric coords
				float s, t;
				s = (uv * wv - vv * wu) / D;
				if (s < 0.0 || s > 1.0)         // I is outside T
					continue;
				t = (uv * wu - uu * wv) / D;
				if (t < 0.0 || (s + t) > 1.0)  // I is outside T
					continue;
				
				// Append new candidate
				faceCandidates.push_back(idxFace);
				glm::vec3 centerFace = glm::vec3( (v0.x + v1.x + v2.x) / 3.0f, (v0.y + v1.y + v2.y) / 3.0f, (v0.z + v1.z + v2.z) / 3.0f );
				faceDist.push_back(pow(rayWorldPos.x - centerFace.x, 2) + pow(rayWorldPos.y - centerFace.y, 2) + pow(rayWorldPos.z - centerFace.z, 2));
			}
		}

		// Choose the candidate which is closest to the camera
		if(!faceCandidates.empty())
		{
			if(!isEditPixelMode)
			{
				for(unsigned int idxFace = 0; idxFace < faceCandidates.size(); ++idxFace)
				{
					Face addFace = e.getFace(faceCandidates[idxFace]);
					// Add face to the current level-label
					if(event->button() == Qt::LeftButton)
						obj->addTreePartFace(e.getVertex(addFace.getFaceIdx1()), e.getVertex(addFace.getFaceIdx2()), e.getVertex(addFace.getFaceIdx3()));
					else if(event->button() == Qt::RightButton)
						obj->removeTreePartFace(e.getVertex(addFace.getFaceIdx1()), e.getVertex(addFace.getFaceIdx2()), e.getVertex(addFace.getFaceIdx3()));
				}
			}
			else
			{
				Face clickedFace = e.getFace(faceCandidates[0]);
				float minDist = faceDist[0];
				for(unsigned int idxFace = 1; idxFace < faceCandidates.size(); ++idxFace)
				{
					if(faceDist[idxFace] < minDist)
					{
						minDist = faceDist[idxFace];
						clickedFace = e.getFace(faceCandidates[idxFace]);
					}
				}

				// Add face to the current level-label
				if(event->button() == Qt::LeftButton)
					obj->addTreePartFace(e.getVertex(clickedFace.getFaceIdx1()), e.getVertex(clickedFace.getFaceIdx2()), e.getVertex(clickedFace.getFaceIdx3()));
				else if(event->button() == Qt::RightButton)
					obj->removeTreePartFace(e.getVertex(clickedFace.getFaceIdx1()), e.getVertex(clickedFace.getFaceIdx2()), e.getVertex(clickedFace.getFaceIdx3()));
			}

			emit updateCompleteness();
		}
	}
}

void Render::mouseMoveEvent(QMouseEvent *event)
{
	if (event->buttons() == Qt::LeftButton)
	{
		if(event->pos().x() - mMousePos.x > 0)
			cam.rot.x += 1;
		else if(event->pos().x() - mMousePos.x < 0)
			cam.rot.x -= 1;

		if(mMousePos.y - event->pos().y() > 0)
			cam.rot.y += 1;
		else if(mMousePos.y - event->pos().y() < 0)
			cam.rot.y -= 1;

		cam.rot += 360.0f;
		cam.rot.x = fmod(cam.rot.x, 360.0f);
		cam.rot.y = fmod(cam.rot.y, 360.0f);
	}
	else if(event->buttons() == (Qt::LeftButton | Qt::RightButton))
	{
		if(mMousePos.y - event->pos().y() > 0)
			cam.pos.y += (0.25*STEP_TRANS / currentFPS);
		else if(mMousePos.y - event->pos().y() < 0)
			cam.pos.y -= (0.25*STEP_TRANS / currentFPS);
	}

	mMousePos.x = event->pos().x();
	mMousePos.y = event->pos().y();

	// Locate brush on the cursor
	if(isEditMode && !isEditPixelMode)
	{
		setCursor(Qt::BlankCursor);
		float normX = (mMousePos.x - widthRender * 0.5) / (widthRender * 0.5);
		float normY = ((heightRender - mMousePos.y) - (heightRender * 0.5)) / (heightRender * 0.5);
		brush->setTranslation(normX, normY, 1.0f);
	}
	else
		setCursor(Qt::ArrowCursor);
}

void Render::wheelEvent(QWheelEvent* event)
{
	int update = 1;
	if(event->delta() < 0)
		update = -1;
	brushSize = max(1, brushSize+update);
	brushSize = min(10, brushSize);

	// Send signal to update the GUI
	emit updateBrushGUI(brushSize);
}

bool Render::createSamples(bool toSave, string& chosenPath)
{
	if (listModels.empty())
	{
		cout << "No moadels have been loaded!" << endl;
		return false;
	}
	clock_t timePreview = clock();

	// Start instance of a new saved scene
	isSampling = true;
	
	// Create directory with all images and annotations
	string path = PATH_OUTPUT;
	QDir dir(PATH_OUTPUT.c_str());
	string dirAnnotations;
	if(toSave)
	{
		if(chosenPath == "")
		{
			path.append("/Samples");
			QDir dir(path.c_str());
			path.append("/sample");
			int numFiles = dir.count() - 2;
			path.append(mSampler->IntToStr(numFiles+1));
			dirAnnotations = path;
		}
		else
		{
			path = chosenPath;
			dir = QDir(path.c_str());
			dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
			QStringList filters;
			filters << "*.png";
			dir.setNameFilters(filters);
			dirAnnotations = path;
		}
		if(dir.mkdir(QString(path.c_str())))
			cout << "New directory of samples " << path << " created" << endl;
	}

	int size = mSampler->getSizeSample();
	int widthSample = size;
	int heightSample = size;

	// NEW framebuffer for the rendered sample
	// - colour texture for samples
	glGenTextures(1, &texSample);
	glBindTexture(GL_TEXTURE_2D, texSample);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widthSample, heightSample, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// - FBO sample
	glGenFramebuffers(1, &fboSample);
	glBindFramebuffer(GL_FRAMEBUFFER, fboSample);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texSample, 0);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "Not properly installed FBO for samples" << endl;

	// Iterate over all different rendered viewports and portions stored in the texture
	vector<GLubyte> pixels(widthSample*heightSample*4, 0);
	vector<GLubyte> depth(widthSample*heightSample*4, 0);

	// Save enough images to finish a 360° inspection (comment to do it before call in non-random generation)
	// mSampler->resetCurrentAngleY();
	bool isFinished = false;
	string imgPath, nameFile;

	float currentY = 0; // azimuth updates (check)
	while(!isFinished)
	{
		if(toSave)
		{
			nameFile = "img";			
			nameFile.append(mSampler->IntToStr(mSampler->getNumImg()));
			mSampler->updateNumImg();
			imgPath = path;
			imgPath.append("/");
			imgPath.append(nameFile);
			imgPath.append(".png");
		}

		mSampler->makeCurrent();
		mSampler->defineCleanTexture();
		mSampler->updateGL();
		makeCurrent();
		for(int i = 0; i < mSampler->getNumSamples() && !isFinished; ++i)
			for(int j = 0; j < mSampler->getNumSamples() && !isFinished; ++j)
			{
				// Update angle Y-rotation camera in the view (need to refresh twice... why? double buffered?)
				updateGL();
				updateGL();

				// Save the pre render framebuffer in another one fixed to sample viewport
				glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboSample);
				glBlitFramebuffer(0, 0, widthRender, heightRender, 0, 0, widthSample, heightSample, GL_COLOR_BUFFER_BIT, GL_LINEAR);
				glBindFramebuffer(GL_FRAMEBUFFER, fboSample);

				glReadPixels(0, 0, widthSample, heightSample, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
				mSampler->transferViewportImg(pixels.data(), i, j);

				// Keep track of annotations per sample
				if(toSave)
					saveAnnotations(nameFile, i, j);

				// Update camera for next sample
				mSampler->updateCurrentAngleY();

				currentY += mSampler->getAngleY();
				if(currentY >= 360.0f)
					isFinished = true;
			}

		if(toSave)
		{
			// Store image with its samples
			mSampler->saveToImg(imgPath);

			// Store annotations in a txt file
			string line;
			string annotationPath = dirAnnotations;
			annotationPath.append("/annotations.txt");			
			ofstream annotationFile;
			annotationFile.open(annotationPath, fstream::app);
			if(annotationFile.is_open())
			{
				for(unsigned int idxAnn = 0; idxAnn < listAnnotations.size(); ++idxAnn)
					annotationFile << listAnnotations[idxAnn] << endl;
				listAnnotations.clear();
				annotationFile.close();
			}
		}
		else
		{
			mSampler->makeCurrent();
			mSampler->updateGL();
		}
	}

	makeCurrent();

	// glDeleteRenderbuffers(1, &depthBuffer);
	glDeleteTextures(1, &texSample);
	glDeleteFramebuffers(1, &fboSample);

	// Unbind usage of any other FBO rather than the default one
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	timePreview = (float)clock() - (float)timePreview;
	std::cout << "Time for " << 360.0f / mSampler->getAngleY() << " viewports of " 
	<< mSampler->getSizeSample() << " x " << mSampler->getSizeSample() << " pixels: " << timePreview << "ms" << endl;

	isSampling = false;

	return true;
}

void Render::saveAnnotations(std::string& imgName, int posSampleX, int posSampleY)
{
	// Annotations scheme:
	// IMGNAME ROW COL HEIGHT WIDTH AZIMUTH ELEVATION DISTANCE PART
	std::stringstream annotationStr;
	for (unsigned int obj = 0; obj < listImgBB.size(); ++obj)
	{
		// General case (only 1 object in the scenario), although there should be only one
		if (!listModels[obj]->isObjectScenario())
			continue;

		// Image name
		annotationStr << imgName << ".png ";

		// Bounding box information
		BB bb = listImgBB[obj];
		int left, top, width, height;
		// Important: left,top are already 1 pxl to the left, so w and h need +2 pxls to be one outside
		width = ceil(bb.getSizeX() / widthRender * mSampler->getSizeSample());
		height = ceil(bb.getSizeY() / heightRender * mSampler->getSizeSample());
		left = bb.getX0() / widthRender * mSampler->getSizeSample() + posSampleX*mSampler->getSizeSample();
		top = (mSampler->getNumSamples() - 1 - posSampleY)*mSampler->getSizeSample() + mSampler->getSizeSample() - (bb.getY0() / heightRender * mSampler->getSizeSample()) - height;
		annotationStr << top << " " << left << " " << height << " " << width << " ";

		// Azimuth, elevatin and distance
		float azimuth, elevation, distance, tilt;
		azimuth = mSampler->getCurrentAngleY();
		elevation = mSampler->getAngleX();
		tilt = mSampler->getTilt();
		distance = mSampler->getDistance();
		annotationStr << azimuth << " " << elevation << " " << tilt << " " << distance<< " ";

		// Keypoints
		map<string, Kp> list_kps = getModel()->getKps();
		vector<string> kps_names(list_kps.size());
		vector<glm::vec3> kps_pos(list_kps.size());
		vector<string> kps_vis(list_kps.size());
		if (list_kps.size() > 0)
		{
			Kp kp;
			for (unsigned int i = 0; i < list_kps.size(); ++i)
			{
				for (map<string, Kp>::iterator it = list_kps.begin(); it != list_kps.end(); ++it)
				{
					kp = it->second;
					if (kp.pos == i)
						break;
				}
				kps_names[i] = kp.name;
				
				// Project 3D points
				kps_pos[i] = computeKpsProj(model_kps[kp.name]);
				// Check if already out of image resolution / truncated (-1) 
				if (kps_pos[i].x < 0 || kps_pos[i].y < 0 || kps_pos[i].x > mSampler->getSizeSample() || kps_pos[i].y >mSampler->getSizeSample())		
					kps_vis[i] = "-1";
				// (1) visible (0) oocluded/truncated
				else if (isKpsVisible(model_kps[kp.name]))
					kps_vis[i] = "1";
				else
					kps_vis[i] = "0";
			}

			// Update kps for Diningtable (Savarese et al Pascal3D annotation)
			// - Most top left pxl = TopLeftFront (check)
			if (kps_names.size() == 8 && kps_names[1] == "Top_Left_Back")
			{
				glm::vec3 aux_pos;
				string aux_vis;
				if (azimuth > 45 && azimuth <= 135)
				{
					// Top_Left_Front -> Top_Left_Back, Top_Left_Back -> Top_Right_Back, Top_Right_Back -> Top_Right_Front, Top_Right_Front -> Top_Left_Front
					aux_pos = kps_pos[1];
					kps_pos[1] = kps_pos[0];
					kps_pos[0] = kps_pos[2];
					kps_pos[2] = kps_pos[3];
					kps_pos[3] = aux_pos;
					aux_vis = kps_vis[1];
					kps_vis[1] = kps_vis[0];
					kps_vis[0] = kps_vis[2];
					kps_vis[2] = kps_vis[3];
					kps_vis[3] = aux_vis;

					// -> Bottom
					aux_pos = kps_pos[5];
					kps_pos[5] = kps_pos[4];
					kps_pos[4] = kps_pos[6];
					kps_pos[6] = kps_pos[7];
					kps_pos[7] = aux_pos;
					aux_vis = kps_vis[5];
					kps_vis[5] = kps_vis[4];
					kps_vis[4] = kps_vis[6];
					kps_vis[6] = kps_vis[7];
					kps_vis[7] = aux_vis;
				}
				else if (azimuth > 135 && azimuth <= 225)
				{
					// Top_Left_Front -> Top_Right_Back, Top_Right_Back -> Top_Left_Front, Top_Left_Back -> Top_Right_Front, Top_Right_Front -> Top_Left_Back
					aux_pos = kps_pos[3];
					kps_pos[3] = kps_pos[0];
					kps_pos[0] = aux_pos;
					aux_pos = kps_pos[1];
					kps_pos[1] = kps_pos[2];
					kps_pos[2] = aux_pos;
					aux_vis = kps_vis[3];
					kps_vis[3] = kps_vis[0];
					kps_vis[0] = aux_vis;
					aux_vis = kps_vis[1];
					kps_vis[1] = kps_vis[2];
					kps_vis[2] = aux_vis;

					// -> Bottom
					aux_pos = kps_pos[7];
					kps_pos[7] = kps_pos[4];
					kps_pos[4] = aux_pos;
					aux_pos = kps_pos[5];
					kps_pos[5] = kps_pos[6];
					kps_pos[6] = aux_pos;
					aux_vis = kps_vis[7];
					kps_vis[7] = kps_vis[4];
					kps_vis[4] = aux_vis;
					aux_vis = kps_vis[5];
					kps_vis[5] = kps_vis[6];
					kps_vis[6] = aux_vis;
				}
				else if (azimuth > 225 && azimuth <= 315)
				{
					// Top_Left_Front -> Top_Right_Front, Top_Right_Front -> Top_Right_Back, Top_Right_Back -> Top_Left_Back, Top_Left_Back -> Top_Left_Front
					aux_pos = kps_pos[2];
					kps_pos[2] = kps_pos[0];
					kps_pos[0] = kps_pos[1];
					kps_pos[1] = kps_pos[3];
					kps_pos[3] = aux_pos;
					aux_vis = kps_vis[2];
					kps_vis[2] = kps_vis[0];
					kps_vis[0] = kps_vis[1];
					kps_vis[1] = kps_vis[3];
					kps_vis[3] = aux_vis;

					// -> Bottom
					aux_pos = kps_pos[6];
					kps_pos[6] = kps_pos[4];
					kps_pos[4] = kps_pos[5];
					kps_pos[5] = kps_pos[7];
					kps_pos[7] = aux_pos;
					aux_vis = kps_vis[6];
					kps_vis[6] = kps_vis[4];
					kps_vis[4] = kps_vis[5];
					kps_vis[5] = kps_vis[7];
					kps_vis[7] = aux_vis;
				}

			}
	
			// Update for Sail_Left and Sail_Right (boat)
			if (list_kps.find("Mast_Top") != list_kps.end())
				if ((kps_vis[9] == "1" || kps_vis[10] == "1") && kps_pos[9].x > kps_pos[10].x)
				{
					glm::vec3 aux_vec3 = kps_pos[9];
					kps_pos[9] = kps_pos[10];
					kps_pos[10] = aux_vec3;
					string aux_vis = kps_vis[9];
					kps_vis[9] = kps_vis[10];
					kps_vis[10] = aux_vis;
				}
		}
		// Make occluded those who are too close from regions that are closer to the camera
		/*
		float dimBB = (float)max(height, width);
		for (int i = 0; i < kps_names.size(); ++i)
		{
			glm::vec3 piv_pos = kps_pos[i];
			if (kps_vis[i] == "1")
				for (int j = 0; j < kps_names.size(); ++j)
				{
					if (kps_vis[j] == "1")
					{
						glm::vec3 nei_pos = kps_pos[j];
						// less than 5% radius of smaller bb dim AND behind in camera depth
						if (sqrt((piv_pos.x - nei_pos.x)*(piv_pos.x - nei_pos.x) + (piv_pos.y - nei_pos.y)*(piv_pos.y - nei_pos.y)) < dimBB*0.05f && piv_pos.z > nei_pos.z)
						{
							kps_vis[i] = "0";
							break;
						}
					}
				}
		}
		*/

		// Add exact location in grid
		for (int i = 0; i < kps_names.size(); ++i)
		{
			kps_pos[i].x += posSampleX*mSampler->getSizeSample();
			kps_pos[i].y += (mSampler->getNumSamples() - 1 - posSampleY)*mSampler->getSizeSample();
		}

		// Matlab notation [1..size] and row,col
		for (int i = 0; i < kps_names.size(); ++i)
			annotationStr << kps_names[i] << " " << floor(kps_pos[i].y) + 1 << " " << floor(kps_pos[i].x) + 1 << " " << kps_vis[i] << " "; 

		// Parts (not implemented yet)
		// annotationStr << "All";

		listAnnotations.push_back(annotationStr.str());
	}
}

void Render::createKp(Kp kp)
{
	delete model_kps[kp.name];
	model_kps[kp.name] = createObj("Sphere", programShaders[TYPE_SHADER::PHONG], 0.75f, 0.0f, 0.0f);
	model_kps[kp.name]->setDrawType(DRAW_TYPE::LINES);
	model_kps[kp.name]->setTranslation(kp.X, kp.Y, kp.Z);
	model_kps[kp.name]->setScaling(kp.Sx, kp.Sy, kp.Sz);
}

void Render::destroyKp(std::string id)
{
	delete model_kps[id];
	model_kps.erase(id);
}

void Render::drawActiveKps(string id)
{
	float r = 0.75f;
	float g = 0.0f;
	float b = 0.0f;
	for (map<string, Model*>::iterator it = model_kps.begin(); it != model_kps.end(); ++it)
	{
		Model* ball = it->second;
		for (unsigned int i = 0; i < ball->getVisualEntity(0).getListVertices().size(); ++i)
			ball->getVisualEntity(0).getListVertices()[i].setColour(r, g, b, 1.0f);
		ball->updateMeshes();
		ball->updateBB();
		ball->attachTexture(emptyTex);
	}
	Model* ball = model_kps[id];
	g = 0.75f;
	for (unsigned int i = 0; i < ball->getVisualEntity(0).getListVertices().size(); ++i)
		ball->getVisualEntity(0).getListVertices()[i].setColour(r, g, b, 1.0f);
	ball->updateMeshes();
	ball->updateBB();
	ball->attachTexture(emptyTex);
}

void Render::updateKpsX(string id, float X)
{
	Model* ball = model_kps[id];
	ball->setTranslationX(X);
}
void Render::updateKpsY(string id, float Y)
{
	Model* ball = model_kps[id];
	ball->setTranslationY(Y);
}
void Render::updateKpsZ(string id, float Z)
{
	Model* ball = model_kps[id];
	ball->setTranslationZ(Z);
}

void Render::updateKpsSx(std::string id, float Sx)
{
	Model* ball = model_kps[id];
	ball->setSx(Sx);
}
void Render::updateKpsSy(std::string id, float Sy)
{
	Model* ball = model_kps[id];
	ball->setSy(Sy);
}
void Render::updateKpsSz(std::string id, float Sz)
{
	Model* ball = model_kps[id];
	ball->setSz(Sz);
}

glm::vec3 Render::computeKpsProj(Model* currentKp)
{
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, cam.fixedPos.z - mSampler->getDistance()));
	model = glm::rotate(model, (float)mSampler->getAngleX(), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, -(float)mSampler->getCurrentAngleY(), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(getModel()->getSX(), getModel()->getSY(), getModel()->getSZ()));
	if (!isKpsAz)
	{
		model = glm::rotate(model, (float)mSampler->getCurrentAngleY(), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	model = glm::translate(model, glm::vec3(currentKp->getTX(), currentKp->getTY(), currentKp->getTZ()));
	glm::vec4 proj2D = proj*view*model * glm::vec4(0.0f, 0.0f, 0.0f, 1);
	float transPxl = (float)mSampler->getSizeSample() / 2.0f;
	glm::vec3 p2d(proj2D.x / proj2D.w * transPxl + transPxl, -1*proj2D.y / proj2D.w * transPxl + transPxl, proj2D.z);
	return p2d;
}

bool Render::isKpsVisible(Model* currentKp)
{
	float transPxl = widthRender / 2.0f;

	// Visualisation code (leave it for future plots)
	/*
	vector<float> depthObj(widthRender*heightRender, 0);
	vector<GLubyte> depthObj_png(4 * widthRender*heightRender, 0);
	vector<GLubyte> depthObj_png_(4 * widthRender*heightRender, 0);
	vector<GLubyte> depthObj_png2(4 * widthRender*heightRender, 0);
	vector<GLubyte> depthObj_png2_(4 * widthRender*heightRender, 0);
	string path_raw = "Z:/depth.png";
	string path_raw2 = "Z:/depth2.png";
	for (int i = 0; i < depth.size(); ++i)
	{
		depthObj_png[i] = floor(depth[i] * 255.0);
		depthObj_png2[i] = floor(depthKps[i] * 255.0);
	}
	for (int col = 0; col < 4 * heightRender; col += 4)
		for (int row = 0; row < heightRender; row += 1)
		{
			depthObj_png_[row * 4 * heightRender + col + 0] = depthObj_png[(heightRender - 1 - row) * 4 * heightRender + col + 0];
			depthObj_png_[row * 4 * heightRender + col + 1] = depthObj_png[(heightRender - 1 - row) * 4 * heightRender + col + 1];
			depthObj_png_[row * 4 * heightRender + col + 2] = depthObj_png[(heightRender - 1 - row) * 4 * heightRender + col + 2];
			depthObj_png_[row * 4 * heightRender + col + 3] = depthObj_png[(heightRender - 1 - row) * 4 * heightRender + col + 3];
			depthObj_png2_[row * 4 * heightRender + col + 0] = depthObj_png2[(heightRender - 1 - row) * 4 * heightRender + col + 0];
			depthObj_png2_[row * 4 * heightRender + col + 1] = depthObj_png2[(heightRender - 1 - row) * 4 * heightRender + col + 1];
			depthObj_png2_[row * 4 * heightRender + col + 2] = depthObj_png2[(heightRender - 1 - row) * 4 * heightRender + col + 2];
			depthObj_png2_[row * 4 * heightRender + col + 3] = depthObj_png2[(heightRender - 1 - row) * 4 * heightRender + col + 3];
		}
	lodepng::encode(path_raw.c_str(), depthObj_png_.data(), heightRender, widthRender);
	lodepng::encode(path_raw2.c_str(), depthObj_png2_.data(), heightRender, widthRender);
	*/

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, cam.fixedPos.z - mSampler->getDistance()));
	model = glm::rotate(model, (float)mSampler->getAngleX(), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, -(float)mSampler->getCurrentAngleY(), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(getModel()->getSX(), getModel()->getSY(), getModel()->getSZ()));
	if (!isKpsAz)
	{
		model = glm::rotate(model, (float)mSampler->getCurrentAngleY(), glm::vec3(0.0f, 1.0f, 0.0f));
	}
	model = glm::translate(model, glm::vec3(currentKp->getTX(), currentKp->getTY(), currentKp->getTZ()));
	model = glm::scale(model, glm::vec3(currentKp->getSX(), currentKp->getSY(), currentKp->getSZ()));
	float* centre = currentKp->getBB().getCenter();
	model = glm::translate(model, glm::vec3(-centre[0], -centre[1], -centre[2]));
	vector<Vertex> list_vertex = currentKp->getListAllVertices();
	int numOk = 0;
	for (unsigned int i = 0; i < list_vertex.size(); ++i)
	{
		// Vertex v = list_vertex[rand() % list_vertex.size()];
		Vertex v = list_vertex[i];
		float* coord = v.getPosition();
		glm::vec4 proj2D = proj*view*model * glm ::vec4(coord[0], coord[1], coord[2], 1);
		glm::vec2 p2d(proj2D.x / proj2D.w * transPxl + transPxl, proj2D.y / proj2D.w * transPxl + transPxl);
		// Check if at least 1 projection is visible
		int row = floor(p2d.y);
		int col = floor(p2d.x);
		unsigned int idx = 4*(row*widthRender + col);
		if (idx < 1 || idx >= 4 * (widthRender*widthRender))
			continue;
		float valueDepth = depth[idx];
		float valueDepthAlpha = depth[idx + 3];
		float valueV = 1.0f - min(1.0f, proj2D.z / 10.0f);
		// float valueV = depthKps[idx];
		// float valueVAlpha = depth[idx + 3];
		// NOTE: 0.000006 is 32F precision error (try without adding anything "valueV + 00000006")		
		if ((!isKpsSelfOcc && valueDepthAlpha > 0.66f) || (valueV > valueDepth && valueDepth != 0 && valueDepthAlpha > 0.66f)) // && abs(valueV - valueDepth) < 0.01) //  && valueVAlpha == 1.0f)
			numOk++;
	}
	if (numOk > list_vertex.size()*0.025) // Only if more than 1% of vertices are visible
		return true;
	return false;
}