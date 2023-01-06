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
	//�õ�������������
	int nverts();
	int nfaces();
	//�õ���i������
	Vec3f vert(int i);
	//face�õ���idx������������飬��Ӧ�����ε���������
	std::vector<int> face(int idx);
};

#endif //__MODEL_H__