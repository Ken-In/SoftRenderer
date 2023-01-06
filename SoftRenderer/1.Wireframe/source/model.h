#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<std::vector<int> > faces_;
public:
	Model(const char* filename);
	~Model();
	//得到顶点数和面数
	int nverts();
	int nfaces();
	//得到第i个顶点
	Vec3f vert(int i);
	//face得到第idx个面的索引数组，对应三角形的三个顶点
	std::vector<int> face(int idx);
};

#endif //__MODEL_H__