#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <string>

class Texture
{
	public:

		Texture();
		~Texture();
		static unsigned int loadEmptyTexture(int w = 1, int h = 1);
		static Texture loadTexture(const std::string& fileName);

		// Getters
		unsigned int& getId() { return textureId; }
		std::string& getPath() { return path; }

		// Setters
		void setId(unsigned int id) { textureId = id; }
		void setPath(std::string aPath) { path = aPath; }

	private:

		unsigned int textureId;
		std::string path;
		
};

#endif