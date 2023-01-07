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
Vec3f camera(2, 1, 3);
Vec3f center(0, 0, 0);

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

void triangle(Vec3i t0, Vec3i t1, Vec3i t2, float ity0, float ity1, float ity2, Vec2i uv0, Vec2i uv1, Vec2i uv2, float dis0, float dis1, float dis2, TGAImage& image, int* zbuffer) {
	//����y�ָ�Ϊ����������
	if (t0.y == t1.y && t0.y == t2.y) return;
	if (t0.y > t1.y) { std::swap(t0, t1); std::swap(ity0, ity1); std::swap(uv0, uv1); }
	if (t0.y > t2.y) { std::swap(t0, t2); std::swap(ity0, ity2); std::swap(uv0, uv2); }
	if (t1.y > t2.y) { std::swap(t1, t2); std::swap(ity1, ity2); std::swap(uv1, uv2); }
	int total_height = t2.y - t0.y;
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > t1.y - t0.y || t1.y == t0.y;
		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height;
		//����A,B���������
		Vec3i A = t0 + Vec3f(t2 - t0) * alpha;
		Vec3i B = second_half ? t1 + Vec3f(t2 - t1) * beta : t0 + Vec3f(t1 - t0) * beta;
		//����A,B����Ĺ���ǿ��
		float ityA = ity0 + (ity2 - ity0) * alpha;
		float ityB = second_half ? ity1 + (ity2 - ity1) * beta : ity0 + (ity1 - ity0) * beta;
		//����UV
		Vec2i uvA = uv0 + (uv2 - uv0) * alpha;
		Vec2i uvB = second_half ? uv1 + (uv2 - uv1) * beta : uv0 + (uv1 - uv0) * beta;
		//�������
		float disA = dis0 + (dis2 - dis0) * alpha;
		float disB = second_half ? dis1 + (dis2 - dis1) * beta : dis0 + (dis1 - dis0) * beta;
		if (A.x > B.x) { std::swap(A, B); std::swap(ityA, ityB); }
		//x������Ϊѭ������
		for (int j = A.x; j <= B.x; j++) {
			float phi = B.x == A.x ? 1. : (float)(j - A.x) / (B.x - A.x);
			//���㵱ǰ��Ҫ���Ƶ�P�����꣬����ǿ��
			Vec3i    P = Vec3f(A) + Vec3f(B - A) * phi;
			float ityP = ityA + (ityB - ityA) * phi;
			ityP = std::min(1.f, std::abs(ityP) + 0.01f);
			Vec2i uvP = uvA + (uvB - uvA) * phi;
			float disP = disA + (disB - disA) * phi;
			int idx = P.x + P.y * width;
			//�߽�����
			if (P.x >= width || P.y >= height || P.x < 0 || P.y < 0) continue;
			if (zbuffer[idx] < P.z) {
				zbuffer[idx] = P.z;
				TGAColor color = model->diffuse(uvP);
				image.set(P.x, P.y, TGAColor(color.bgra[2], color.bgra[1], color.bgra[0]) * ityP * (20.f / std::pow(disP, 2.f)));
				//image.set(P.x, P.y, TGAColor(255,255,255)* ityP);
			}
		}
	}
}

//�ӽǾ���
//������x��y����(-1,1)ת������Ļ����(100,700)    1/8width~7/8width
//zbuffer(-1,1)ת����0~255
Matrix viewport(int x, int y, int w, int h) {
	Matrix m = Matrix::identity(4);
	//��4�б�ʾƽ����Ϣ
	m[0][3] = x + w / 2.f;
	m[1][3] = y + h / 2.f;
	m[2][3] = depth / 2.f;
	//�Խ��߱�ʾ������Ϣ
	m[0][0] = w / 2.f;
	m[1][1] = h / 2.f;
	m[2][2] = depth / 2.f;
	return m;
}

Matrix lookat(Vec3f eye, Vec3f center, Vec3f up)
{
	Vec3f z = (eye - center).normalize();
	Vec3f x = (up ^ z).normalize();
	Vec3f y = (z ^ x).normalize();
	Matrix rotation = Matrix::identity(4);
	Matrix translation = Matrix::identity(4);

	//��ƽ�ƾ���
	for (int i = 0; i < 3; i++)
	{
		translation[i][3] = -center[i];
	}

	//����ת
	for (int i = 0; i < 3; i++)
	{
		rotation[0][i] = x[i];
		rotation[1][i] = y[i];
		rotation[2][i] = z[i];
	}

	Matrix res = rotation * translation;
	return res;
}

int main(int argc, char** argv) 
{
	
	model = new Model("obj/african_head.obj"); //����model

	//����zbuffer����ʼ��,�趨һ����С��ֵ
	int* zbuffer = new int[width * height];
	for (int i = width * height; i--; zbuffer[i] = std::numeric_limits<float>::min());

	TGAImage image(width, height, TGAImage::RGB);

	Matrix ModelView = lookat(camera, center, Vec3f(0, 1, 0));

	Matrix Projection = Matrix::identity(4);
	Projection[3][2] = -1.f / (camera - center).norm();

	Matrix Viewport = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i); //�õ������ε��������飬�������������㣬face�����������������
		Vec3f screen_coords[3];//��Ļ��������
		float intensity[3];//�������ǿ��
		float distance[3];
		for(int j = 0; j < 3; j++)
		{
			Vec3f v = model->vert(face[j]);
			Matrix m_v = ModelView * Matrix(v);
			screen_coords[j] = Vec3f(Viewport * Projection * m_v);
			intensity[j] = model->norm(i, j) * light_dir;
			Vec3f new_v = Vec3f(m_v);
			distance[j] = std::pow((std::pow(new_v.x - camera.x, 2.0f) + std::pow(new_v.y - camera.y, 2.0f) + std::pow(new_v.z - camera.z, 2.0f)), 0.5f);
		}

		Vec2i uv[3];
		for (int k = 0; k < 3; k++)
		{
			uv[k] = model->uv(i, k);
		}

		triangle(screen_coords[0], screen_coords[1], screen_coords[2], intensity[0], intensity[1], intensity[2], uv[0], uv[1], uv[2], distance[0], distance[1], distance[2], image, zbuffer);
	}
	image.flip_vertically();
	image.write_tga_file("output.tga");

	//���zbuffer
	{
		TGAImage zbimage(width, height, TGAImage::GRAYSCALE);
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				zbimage.set(i, j, TGAColor(zbuffer[i + j * width]));
			}
		}
		zbimage.flip_vertically();
		zbimage.write_tga_file("zbuffer.tga");
	}
	return 0;
}