#include <iostream>
#include <time.h>
#include <QFile>
#include <QPainter>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Save images
#include "lodepng.h"

#include "rendering/Sampler.hpp"

using namespace std;

Sampler::Sampler(QWidget *parent, const QGLFormat &format) : QGLWidget(format, parent)
{

}

Sampler::~Sampler()
{
	glDeleteProgram(mShader);

	glDeleteTextures(1, &outTex);

	glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void Sampler::initializeGL()
{
	// Initialise GLEW
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));

	loadShader();
	glGenTextures(1, &outTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, outTex);
	loadSampler();
	createQuad();

	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(), glm::vec3(0.0f, 1.0f, 0.0f));
	GLint uniView = glGetUniformLocation(mShader, "view");
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

	glm::mat4 proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -2.0f, 2.0f);
	GLint uniProj = glGetUniformLocation(mShader, "ortho");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

	glm::mat4 trans;
	trans = glm::rotate(trans, 180.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	GLint uniTrans = glGetUniformLocation(mShader, "model");
	glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(trans));
}

void Sampler::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    updateGL();	
}

void Sampler::paintGL()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Sampler::loadShader()
{
	QFile vertexFile, fragmentFile;

	// - Vertex Shader
	vertexFile.setFileName(":/shaders/base.vert");	
	vertexFile.open(QIODevice::ReadOnly | QIODevice::Text);
	QByteArray vertexData = vertexFile.readAll();
	vertexFile.close();
	const GLchar* vertexSource = vertexData.data();
	string str = vertexSource;
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	// - Fragment Shader
	fragmentFile.setFileName(":/shaders/base.frag");
	fragmentFile.open(QIODevice::ReadOnly | QIODevice::Text);
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

	// Create final shader workflow
	mShader = glCreateProgram();
	glAttachShader(mShader, vertexShader);
	glAttachShader(mShader, fragmentShader);

	glBindAttribLocation(mShader, 0, "position");
	glBindAttribLocation(mShader, 1, "colour");
	glBindAttribLocation(mShader, 2, "texcoord");
	glBindFragDataLocation(mShader, 0, "outColour");

	glLinkProgram(mShader);
	glUseProgram(mShader);
	
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void Sampler::loadSampler()
{
	defineCleanTexture();
	
	glUniform1i(glGetUniformLocation(mShader, "tex"), 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void Sampler::defineCleanTexture()
{
	vector<GLuint> emptyTextureTemplate(windowSize*windowSize, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowSize, windowSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, emptyTextureTemplate.data());
}

void Sampler::createQuad()
{
	// Define quad to render the texture
	GLfloat vertices[] =
    {
		 1.0f,  1.0f,		1.0f, 0.0f, 0.0f,		1.0f, 0.0f,
         1.0f, -1.0f,		0.0f, 1.0f, 0.0f,		1.0f, 1.0f,
        -1.0f, -1.0f, 		0.0f, 0.0f, 1.0f,		0.0f, 1.0f,
		-1.0f,  1.0f,		0.5f, 0.5f, 0.5f,		0.0f, 0.0f
    };

	GLuint faces[] = { 0, 1, 2, 0, 2, 3 };

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);		

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(faces), faces, GL_STATIC_DRAW);

	glEnableVertexAttribArray((GLuint)0);
	glVertexAttribPointer((GLuint)0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*7, 0);

	glEnableVertexAttribArray((GLuint)1);
	glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*7, (const GLvoid*)(sizeof(GLfloat)*2));

	glEnableVertexAttribArray((GLuint)2);
	glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*7, (const GLvoid*)(sizeof(GLfloat)*5));
}

void Sampler::transferViewportImg(GLubyte* viewportImg, int x, int y)
{
	// First of all, context need to be activated to render in this window
	makeCurrent();
	// Redefine an already existing 2D texture only in the specified subregion
	glTexSubImage2D(GL_TEXTURE_2D, 0, x*sizeSample, y*sizeSample, sizeSample, sizeSample, GL_RGBA, GL_UNSIGNED_BYTE, viewportImg);
}

void Sampler::transferViewportDepth(GLubyte* viewportDepth)
{
	// makeCurrent();
	// glTexSubImage2D(GL_TEXTURE_2D, 0, x*sizeSample, y*sizeSample, sizeSample, sizeSample, GL_DEPTH_COMPONENT, GL_FLOAT, viewportDepth);

	unsigned int max = viewportDepth[0];
	unsigned int min = viewportDepth[0];
	for(unsigned int pix = 1; pix < 256*256; pix++)
	{
		unsigned int val = viewportDepth[pix];
		if(val > max)
			max = val;
		if(val < min)
			min = val;
	}

	vector<GLuint> colouredDepth(766*766*3, 0);
	int nPix = 0;
	for(unsigned int pix = 0; pix < 766*766; pix++)
	{
		colouredDepth[nPix+0] = (float)(viewportDepth[pix]-min)/(max-min)*255;
		colouredDepth[nPix+1] = (float)(viewportDepth[pix]-min)/(max-min)*255;
		colouredDepth[nPix+2] = (float)(viewportDepth[pix]-min)/(max-min)*255;
		nPix+=3;
	}
}

void Sampler::saveToImg(string& imgPath)
{
	makeCurrent();
	updateGL();
	updateGL();

	// Get texture information with transparency (glReadPixels does not provide transparency but background colour)
	vector<GLubyte> texDataRGBA(windowSize*windowSize*4);
	vector<GLubyte> revTexDataRGBA(windowSize*windowSize*4);
	glReadPixels(0, 0, windowSize, windowSize, GL_RGBA, GL_UNSIGNED_BYTE, texDataRGBA.data());
	//glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, texDataRGBA.data());

	for(int col = 0; col < 4*windowSize; col += 4)
		for(int row = 0; row < windowSize; row += 1)
		{
			revTexDataRGBA[row*4*windowSize + col + 0] = texDataRGBA[(windowSize-1 - row)*4*windowSize + col + 0];
			revTexDataRGBA[row*4*windowSize + col + 1] = texDataRGBA[(windowSize-1 - row)*4*windowSize + col + 1];
			revTexDataRGBA[row*4*windowSize + col + 2] = texDataRGBA[(windowSize-1 - row)*4*windowSize + col + 2];
			revTexDataRGBA[row*4*windowSize + col + 3] = texDataRGBA[(windowSize-1 - row)*4*windowSize + col + 3];
		}

	lodepng::encode(imgPath.c_str(), revTexDataRGBA.data(), windowSize, windowSize);
}

void Sampler::updateTexture()
{
	windowSize = sizeSample*numSamples;
	loadSampler();
	parentWidget()->setFixedWidth(windowSize);
	parentWidget()->setFixedHeight(windowSize);
	updateGL();
}

