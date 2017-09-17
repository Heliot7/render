#ifndef FACE_HPP
#define FACE_HPP

class Face
{
	public:

		Face();
		Face(unsigned int p1, unsigned int p2, unsigned int p3);
		~Face();

		// Getters
		int getFaceIdx1() { return i1; }
		int getFaceIdx2() { return i2; }
		int getFaceIdx3() { return i3; }
		int getFaceIdx(int idx)
		{
			switch(idx)
			{
				case 1:
				default:
					return i1;
				case 2:
					return i2;
				case 3:
					return i3;			
			}
		}

		// Setters
		void setFace(unsigned int id, unsigned int p) { if(id == 0) i1 = p; if(id == 1) i2 = p; if(id == 2) i3 = p;}
		void setFace(unsigned int p1, unsigned int p2, unsigned int p3) { i1 = p1; i2 = p2; i3 = p3; }
		void setOffset(int offset) { i1 += offset; i2 += offset; i3 += offset; };

	private:

		unsigned int i1, i2, i3;

};

#endif