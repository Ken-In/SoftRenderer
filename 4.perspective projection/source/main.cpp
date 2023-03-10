#include <vector>
#include <cmath>
#include <cstdlib>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <algorithm>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);

Model* model;

Vec3f light_dir(0.2, 0.15, -1);
Vec3f camera(0, 0, 3);

const int width = 800;
const int height = 800;
const int depth = 255;

void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color)
{
	bool steep = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1))
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}

	if (x0 > x1)
	{
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int dx = x1 - x0;
	int dy = y1 - y0;
	float derror = std::abs(dy / float(dx));
	float error = 0;
	int y = y0;

	for (int x = x0; x <= x1; x++)
	{
		steep ? image.set(y, x, color) : image.set(x, y, color);

		error += derror;
		if (error > .5)
		{
			y += (y1 > y0 ? 1 : -1);
			error -= 1.;
		}
	}
}

//绘制三角形(顶点坐标，uv坐标，tga指针，亮度，zbuffer)
void triangle(Vec3i t0, Vec3i t1, Vec3i t2, Vec2i uv0, Vec2i uv1, Vec2i uv2, TGAImage& image, float intensity, int* zbuffer) {
	if (t0.y == t1.y && t0.y == t2.y) return;
	//分割成两个三角形
	if (t0.y > t1.y) { std::swap(t0, t1); std::swap(uv0, uv1); }
	if (t0.y > t2.y) { std::swap(t0, t2); std::swap(uv0, uv2); }
	if (t1.y > t2.y) { std::swap(t1, t2); std::swap(uv1, uv2); }
	//用高度做循环控制
	int total_height = t2.y - t0.y;
	for (int i = 0; i < total_height; i++) {
		//判断属于哪一部分以确定高度
		bool second_half = i > t1.y - t0.y || t1.y == t0.y;
		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
		//计算当前的比例
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height; // be careful: with above conditions no division by zero here
		//A表示t0与t2之间的点
		//B表示t0与t1之间的点
		Vec3i A = t0 + Vec3f(t2 - t0) * alpha;
		Vec3i B = second_half ? t1 + Vec3f(t2 - t1) * beta : t0 + Vec3f(t1 - t0) * beta;
		//计算UV
		Vec2i uvA = uv0 + (uv2 - uv0) * alpha;
		Vec2i uvB = second_half ? uv1 + (uv2 - uv1) * beta : uv0 + (uv1 - uv0) * beta;
		//保证B在A的右边
		if (A.x > B.x) { std::swap(A, B); }// std::swap(uvA, uvB);}
		//用横坐标作为循环控制，对这一行进行着色
		for (int j = A.x; j <= B.x; j++) {
			//计算当前点在AB之间的比例
			float phi = B.x == A.x ? 1. : (float)(j - A.x) / (float)(B.x - A.x);
			//计算出当前点的坐标,A，B保存了z轴信息
			Vec3i   P = Vec3f(A) + Vec3f(B - A) * phi;
			Vec2i uvP = uvA + (uvB - uvA) * phi;
			if (P.x < width && P.y < height)
			{
				//计算当前zbuffer下标=P.x+P.y*width
				int idx = P.x + P.y * width;
				//当前点的z大于zbuffer信息，覆盖掉，并更新zbuffer
				if (zbuffer[idx] < P.z) {
					zbuffer[idx] = P.z;
					TGAColor color = model->diffuse(uvP);
					image.set(P.x, P.y, TGAColor(color.r * intensity, color.g * intensity, color.b * intensity));
				}
			}
		}
	}
}

//视角矩阵
//将物体x，y坐标(-1,1)转换到屏幕坐标(100,700)    1/8width~7/8width
//zbuffer(-1,1)转换到0~255
Matrix viewport(int x, int y, int w, int h) {
	Matrix m = Matrix::identity(4);
	//第4列表示平移信息
	m[0][3] = x + w / 2.f;
	m[1][3] = y + h / 2.f;
	m[2][3] = depth / 2.f;
	//对角线表示缩放信息
	m[0][0] = w / 2.f;
	m[1][1] = h / 2.f;
	m[2][2] = depth / 2.f;
	return m;
}

//4d-->3d
//除以最后一个分量。（当最后一个分量为0，表示向量）
//不为0，表示坐标
Vec3f m2v(Matrix m) {
	return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

//3d-->4d
//添加一个1表示坐标
Matrix v2m(Vec3f v) {
	Matrix m;
	m[0][0] = v.x;
	m[1][0] = v.y;
	m[2][0] = v.z;
	m[3][0] = 1.f;
	return m;
}

int main(int argc, char** argv) {
	
	model = new Model("obj/african_head.obj"); //创建model

	//创建zbuffer并初始化,设定一个很小的值
	int* zbuffer = new int[width * height];
	for (int i = width * height; i--; zbuffer[i] = std::numeric_limits<float>::min());

	TGAImage image(width, height, TGAImage::RGB);

	Matrix Projection = Matrix::identity(4);

	Matrix Viewport = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

	Projection[3][2] = -1.f / camera.z;


	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i); //得到三角形的索引数组，三角形有三个点，face数组存放三顶点的索引
		Vec3f screen_coords[3];//屏幕坐标数组
		Vec3f world_coords[3];//世界坐标数组
		for(int j = 0; j < 3; j++)
		{
			Vec3f v = model->vert(face[j]);
			screen_coords[j] = m2v(Viewport * Projection * v2m(v));
			world_coords[j] = v;
		}
		//计算法向量 ^运算符是叉乘 可在Vec3f中查看
		Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
		n.normalize();
		float intensity = n * light_dir;
		intensity = std::min(std::abs(intensity), 1.f);


		//去掉背面
		if (intensity > 0)
		{
			Vec2i uv[3];
			for (int k = 0; k < 3; k++)
			{
				uv[k] = model->uv(i, k);
			}

			triangle(screen_coords[0], screen_coords[1], screen_coords[2], uv[0], uv[1], uv[2], image, intensity, zbuffer);
		}
	}
	image.flip_vertically();
	image.write_tga_file("output.tga");

	//输出zbuffer
	{
		TGAImage zbimage(width, height, TGAImage::GRAYSCALE);
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				zbimage.set(i, j, TGAColor(zbuffer[i + j * width], 1));
			}
		}
		zbimage.flip_vertically();
		zbimage.write_tga_file("zbuffer.tga");
	}
	return 0;
}