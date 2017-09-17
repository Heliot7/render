#include <iostream>

#include <GL/glew.h>
#include <SOIL.h>

#include "modelling/Texture.hpp"

using namespace std;

Texture::Texture()
{
}

Texture::~Texture()
{
}

unsigned int Texture::loadEmptyTexture(int w, int h)
{
	unsigned int emptyTex;
	glGenTextures(1, &emptyTex);
	glBindTexture(GL_TEXTURE_2D, emptyTex);

	// vector<unsigned char> pixels(w*h*4, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 /*pixels.data()*/);
	cout << "Texture (ID " << emptyTex << ") has been loaded. - empty" << endl;	

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	return emptyTex;
}

// GL_SRGB_ALPHA instead of GL_RGBA to undo gamma correction and re-do it after illumination shading computation
Texture Texture::loadTexture(const std::string& fileName)
{
	Texture tex;
	glGenTextures(1, &tex.getId());
	glBindTexture(GL_TEXTURE_2D, tex.getId());

	if(fileName == "")
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		cout << "Texture (ID " << tex.getId() << ") has been loaded. - empty" << endl;
	}
	else
	{
		tex.setPath(fileName);

		int width, height;
		unsigned char* image = SOIL_load_image(tex.getPath().c_str(), &width, &height, 0, SOIL_LOAD_RGBA);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		if(image == 0)
			cout << "Texture (ID " << tex.getId() << ") could NOT be loaded!" << endl;
		else
			cout << "Texture (ID " << tex.getId() << ") has been loaded." << endl;
		SOIL_free_image_data(image);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	return tex;
}
