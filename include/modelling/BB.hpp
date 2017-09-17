#ifndef BB_HPP
#define BB_HPP

class BB
{
	public:

		BB();
		~BB();

		// Getters
		float* getCenter() { return center; }
		float getX0() { return x0; }
		float getX1() { return x1; }
		float getY0() { return y0; }
		float getY1() { return y1; }
		float getZ0() { return z0; }
		float getZ1() { return z1; }
		float getSizeX() { return x1 - x0; }
		float getSizeY() { return y1 - y0; }
		float getSizeZ() { return z1 - z0; }

		// Setters
		void reset();
		void updateCenter();
		void setX0(float newX0) { x0 = newX0; }
		void setX1(float newX1) { x1 = newX1; }
		void setY0(float newY0) { y0 = newY0; }
		void setY1(float newY1) { y1 = newY1; }
		void setZ0(float newZ0) { z0 = newZ0; }
		void setZ1(float newZ1) { z1 = newZ1; }

	private:

		float x0, y0, z0;
		float x1, y1, z1;
		float center[3];
		
};

#endif