#include "modelling/Face.hpp"

using namespace std;

Face::Face()
{ 
	i1 = 0;
	i2 = 0;
	i3 = 0;
}

Face::Face(unsigned int p1, unsigned int p2, unsigned int p3)
{
	i1 = p1; i2 = p2; i3 = p3;
}

Face::~Face()
{

}

