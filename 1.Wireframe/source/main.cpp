#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);

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

int main(int argc, char** argv) {
	int width = 500;
	int height = 500;

	Model model = Model("obj/african_head.obj"); //创建model
	TGAImage image(width, height, TGAImage::RGB);
	for (int i = 0; i < model.nfaces(); i++) {
		std::vector<int> face = model.face(i); //得到三角形的索引数组，三角形有三个点，face数组存放三顶点的索引
		for (int j = 0; j < 3; j++) {
			//拿到索引后 用索引得到顶点位置
			//根据顶点v0和v1画线
			//循环三次 0-1 1-2 2-0
			//这样就画完了三角形的三条边
			Vec3f v0 = model.vert(face[j]);
			Vec3f v1 = model.vert(face[(j + 1) % 3]);
			//下面的计算是为了把model移到屏幕中心
			//model 范围[-1,1] 我们把范围转化到[0, width/height]
			//目前不考虑z轴 只做2D的渲染
			int x0 = (v0.x + 1.) * width / 2.;
			int y0 = (v0.y + 1.) * height / 2.;
			int x1 = (v1.x + 1.) * width / 2.;
			int y1 = (v1.y + 1.) * height / 2.;
			//画线
			line(x0, y0, x1, y1, image, white);
		}
	}
	//image.flip_vertically();
	image.write_tga_file("output.tga");
	return 0;
}