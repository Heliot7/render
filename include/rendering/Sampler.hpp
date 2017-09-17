#ifndef SAMPLER_HPP
#define SAMPLER_HPP

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include <QGLWidget>
#include <QTimer>

struct RangeView
{
	RangeView(int minA, int maxA, int minE, int maxE) { minAzimuth = minA; maxAzimuth = maxA; minElevation = minE; maxElevation = maxE; }
	int minAzimuth, maxAzimuth;
	int minElevation, maxElevation;
};

class Sampler : public QGLWidget
{
    Q_OBJECT

    public:

        Sampler(QWidget *parent, const QGLFormat &format);
        ~Sampler();
		void transferViewportImg(GLubyte* viewportImg, int x, int y);
		void transferViewportDepth(GLubyte* viewportDepth);
		void defineCleanTexture();
		void saveToImg(std::string& imgPath);
		
		// Getters
		int getSizeSample() { return sizeSample; }
		int getNumSamples() { return numSamples; }
		int getWindowSize() { return windowSize; }
		double getAngleX() { return angleX; }
		double getAngleY() { return angleY; }
		double getDistance() { return distance; }
		double getTilt() { return tilt; }
		double getCurrentAngleY() { return currentAngleY; }
		int numViews() { return listViews.size(); }
		RangeView getRangeView(int i) { return listViews[i]; }
		bool isAzimuth() { return bAzimuth; }
		bool isElevation() { return bElevation; }
		unsigned int getNumImg() { return numImg; }

		// Setters
		void setSizeSample(int newValue) { sizeSample = newValue; windowSize = sizeSample*numSamples; }
		void updateSizeSample(int newValue) { sizeSample = newValue; updateTexture(); }
		void setNumSamples(int newValue) { numSamples = newValue; windowSize = sizeSample*numSamples; }
		void updateNumSamples(int newValue) { numSamples = newValue; updateTexture(); }
		void setAngleX(double newValue) { angleX = newValue; }
		void setAngleY(double newValue) { angleY = newValue; }
		void setDistance(double newValue) { distance = newValue; }
		void setTilt(double newValue) { tilt = newValue; }
		void updateCurrentAngleY() { currentAngleY += angleY; }
		void resetCurrentAngleY() { currentAngleY = 0.0f; }
		void setCurrentAngleY(double angle) { currentAngleY = angle; }
		void resetViews() { listViews.clear(); }
		void addView(int minA, int maxA, int minE, int maxE) { listViews.push_back(RangeView(minA, maxA, minE, maxE)); }
		void setIsAzimuth(bool isA) { bAzimuth = isA; }
		void setIsElevation(bool isE) { bElevation = isE; }
		void setNumImg(unsigned int num) { numImg = num; }
		void updateNumImg() { numImg++; }

		std::string IntToStr(int number)
		{
		   std::stringstream ss;
		   ss << number;
		   return ss.str();
		}

    protected:

		// OpenGL oriented calls
        virtual void initializeGL();
        virtual void paintGL();
        virtual void resizeGL(int width, int height);

	protected slots:		

    private:

		// Render
		GLuint vao, vbo, ebo;
		GLuint mShader;
		void loadShader();
		void loadSampler();
		
		// Views
		std::vector<RangeView> listViews;
		bool bAzimuth, bElevation;

		// Output options
		int sizeSample;
		int numSamples;
		int windowSize;
		float angleY;
		float angleX;
		float currentAngleY;
		float distance;
		float tilt;
		unsigned int numImg;

		// Output image
		void createQuad();
		void updateTexture();
		GLuint outTex;
		std::vector<GLubyte> arrayTex;
	
	signals:

		void updateFPS(const QString& text);
};

#endif
