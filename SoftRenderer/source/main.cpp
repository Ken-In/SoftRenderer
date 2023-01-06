#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const Vec3f	lightDir = { 0, 0, -1 };

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

void triangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color)
{
	//三角形是线时
	if (t0.y == t1.y && t0.y == t2.y) return;
	//排序
	if (t0.y > t1.y) std::swap(t0, t1);
	if (t0.y > t2.y) std::swap(t0, t2);
	if (t1.y > t2.y) std::swap(t1, t2);
	int total_height = t2.y - t0.y;

	for (int i = 0; i < total_height; i++)
	{
		//是否为第二部分
		bool second_half = i > t1.y - t0.y || t1.y == t0.y;
		//该部分高度
		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
		float alpha = (float)i / total_height;//总比值
		float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height;//部分比值
		//计算A，B两点坐标
		Vec2i A = t0 + (t2 - t0) * alpha;
		Vec2i B = second_half ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;
		if (A.x > B.x) std::swap(A, B);
		for (int j = A.x; j <= B.x; j++)
		{
			image.set(j, t0.y + i, color);
		}
	}
}

int main(int argc, char** argv) {
	int width = 500;
	int height = 500;

	Model model = Model("obj/african_head.obj"); //创建model
	TGAImage image(width, height, TGAImage::RGB);
	for (int i = 0; i < model.nfaces(); i++) {
		std::vector<int> face = model.face(i); //得到三角形的索引数组，三角形有三个点，face数组存放三顶点的索引
		Vec2i screen_coords[3];//屏幕坐标数组
		Vec3f world_coords[3];//世界坐标数组
		for(int j = 0; j < 3; j++)
		{
			Vec3f v = model.vert(face[j]);
			screen_coords[j] = Vec2i((v.x + 1.) * width / 2, (v.y + 1.) * height / 2);
			world_coords[j] = v;
		}
		//计算法向量 ^运算符是叉乘 可在Vec3f中查看
		Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
		n.normalize();
		float intensity = n * lightDir;

		//去掉背面
		if(intensity > 0)
		triangle(screen_coords[0], screen_coords[1], screen_coords[2], image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
	}
	//image.flip_vertically();
	image.write_tga_file("output.tga");
	return 0;
}