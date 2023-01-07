#include <vector>
#include <iostream>
#include <algorithm>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

Model* model = NULL;

Vec3f light_dir(0, 1, 1);
Vec3f camera(1, 0.5, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

const int width = 800;
const int height = 800;

//高洛德着色器
struct GouraudShader : public IShader
{
	Vec3f varying_intensity;
	mat<2, 3, float> varying_uv;

	//顶点着色器
	virtual Vec4f vertex(int iface, int nthvert)
	{
		//根据面序号和顶点序号读取模型对应顶点，并扩展为4维 
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));

		mat<4, 4, float> uniform_M = Projection * ModelView;
		mat<4, 4, float> uniform_MIT = (Projection * ModelView).invert_transpose();
		gl_Vertex = Viewport * uniform_M * gl_Vertex;

		Vec3f normal = model->normal(iface, nthvert).normalize();
		varying_intensity[nthvert] = std::max(0.1f, model->normal(iface, nthvert) * light_dir);
		return gl_Vertex;
	}

	//bar 是插值
	virtual bool fragment(Vec3f bar, TGAColor& color)
	{
		Vec2f uv = varying_uv * bar;
		TGAColor c = model->diffuse(uv);
		float intensity = varying_intensity * bar;
		color = c * intensity;
		return false;
	}
};

//将一定阈值内的光照强度给替换为一种
struct ToonShader : public IShader {
	mat<3, 3, float> varying_tri;
	Vec3f          varying_ity;

	virtual ~ToonShader() {}

	virtual Vec4f vertex(int iface, int nthvert) {
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		gl_Vertex = Projection * ModelView * gl_Vertex;
		varying_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));

		varying_ity[nthvert] = model->normal(iface, nthvert) * light_dir;

		gl_Vertex = Viewport * gl_Vertex;
		return gl_Vertex;
	}

	virtual bool fragment(Vec3f bar, TGAColor& color) {
		float intensity = varying_ity * bar;
		if (intensity > .85) intensity = 1;
		else if (intensity > .60) intensity = .80;
		else if (intensity > .45) intensity = .60;
		else if (intensity > .30) intensity = .45;
		else if (intensity > .15) intensity = .30;
		else intensity = .15;
		color = TGAColor(255, 155, 0) * intensity;
		return false;
	}
};

//不对法向量进行插值，法向量来源于三角形边的叉积
struct FlatShader : public IShader {
	//三个点的信息
	mat<3, 3, float> varying_tri;

	virtual ~FlatShader() {}

	virtual Vec4f vertex(int iface, int nthvert) {
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		gl_Vertex = Projection * ModelView * gl_Vertex;
		varying_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
		gl_Vertex = Viewport * gl_Vertex;
		return gl_Vertex;
	}

	virtual bool fragment(Vec3f bar, TGAColor& color) {

		Vec3f n = cross(varying_tri.col(1) - varying_tri.col(0), varying_tri.col(2) - varying_tri.col(0)).normalize();
		float intensity = n * light_dir;
		color = TGAColor(255, 255, 255) * intensity;
		return false;
	}
};

//Phong氏着色
struct PhongShader : public IShader {
	mat<2, 3, float> varying_uv;  // same as above
	mat<4, 4, float> uniform_M = Projection * ModelView;
	mat<4, 4, float> uniform_MIT = ModelView.invert_transpose();
	virtual Vec4f vertex(int iface, int nthvert) {
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
		return Viewport * Projection * ModelView * gl_Vertex; // transform it to screen coordinates
	}
	virtual bool fragment(Vec3f bar, TGAColor& color) {
		Vec2f uv = varying_uv * bar;
		Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();
		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
		Vec3f r = (n * (n * l * 2.f) - l).normalize();   // reflected light
		float spec = pow(std::max(r.z, 0.0f), model->specular(uv));
		float diff = std::max(0.f, n * l);
		TGAColor c = model->diffuse(uv);
		color = c;
		for (int i = 0; i < 3; i++) color[i] = std::min<float>(5 + c[i] * (diff + .6 * spec), 255);
		return false;
	}
};

int main(int argc, char** argv) 
{
	model = new Model("obj/african_head.obj");

	lookat(camera, center, up);
	projection(-1.f / (camera - center).norm());
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	light_dir.normalize();

	TGAImage image(width, height, TGAImage::RGB);
	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

	PhongShader shader;
	for (int i = 0; i < model->nfaces(); i++)
	{
		Vec4f screen_coords[3];
		for (int j = 0; j < 3; j++)
		{
			screen_coords[j] = shader.vertex(i, j);
		}
		triangle(screen_coords, shader, image, zbuffer);
	}

	image.flip_vertically();
	zbuffer.flip_vertically();
	image.write_tga_file("output.tga");
	zbuffer.write_tga_file("zbuffer.tga");

	delete model;
	return 0;
}