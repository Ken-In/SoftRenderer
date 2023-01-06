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
const Vec3f	lightDir = { 0, 0, -1 };

const int width = 800;
const int height = 800;

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

Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P)
{
	Vec3f s[2];
	//����[AB,AC,PA]��x��y����
	for (int i = 2; i--; )
	{
		s[i][0] = B[i] - A[i];
		s[i][1] = C[i] - A[i];
		s[i][2] = A[i] - P[i];
	}
	//[u,v,1]��[AB,AC,PA]��Ӧ��x��y��������ֱ�����Բ��
	Vec3f u = cross(s[0], s[1]);
	//���㹲��ʱ���ᵼ��u[2]Ϊ0����ʱ����(-1,1,1)
	if (std::abs(u[2]) > 1e-2)
		return Vec3f(1.f - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);
	return Vec3f(-1, 1, 1);
}

void triangle(Vec3f* pts, float* zbuffer, TGAImage& image, TGAColor color)
{
	Vec2f bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
			bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
		}
	}
	Vec3f P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
	{
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
		{
			//bc_screen������������
			Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
			//�и�ֵ ��������������
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
			P.z = 0;
			//����P��zֵ
			for (int i = 0; i < 3; i++) P.z += pts[i][2] * bc_screen[i];
			if (zbuffer[int(P.x + P.y * width)] < P.z)
			{
				zbuffer[int(P.x + P.y * width)] = P.z;
				image.set(P.x, P.y, color);
			}
		}
	}
}

int main(int argc, char** argv) {
	
	Model model = Model("obj/african_head.obj"); //����model

	//����zbuffer����ʼ��,�趨һ����С��ֵ
	float* zbuffer = new float[width * height];
	for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max());

	TGAImage image(width, height, TGAImage::RGB);

	for (int i = 0; i < model.nfaces(); i++) {
		std::vector<int> face = model.face(i); //�õ������ε��������飬�������������㣬face�����������������
		Vec3f screen_coords[3];//��Ļ��������
		Vec3f world_coords[3];//������������
		for(int j = 0; j < 3; j++)
		{
			Vec3f v = model.vert(face[j]);
			screen_coords[j] = Vec3f(int((v.x + 1.) * width / 2 + .5), int((v.y + 1.) * height / 2. + .5), v.z);
			world_coords[j] = v;
		}
		//���㷨���� ^������ǲ�� ����Vec3f�в鿴
		Vec3f n = cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
		n.normalize();
		float intensity = n * lightDir;

		//ȥ������
		if(intensity > 0)
			triangle(screen_coords, zbuffer,image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
	}
	image.flip_vertically();
	image.write_tga_file("output.tga");
	return 0;
}