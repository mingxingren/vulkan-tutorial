#include "gpu_shader.h"

GpuShader::GpuShader(GpuResource* device)
{
	vk_resource_ = device;
}

GpuShader::~GpuShader()
{

}

bool GpuShader::Init(const char* vertex_shader, const char* pixel_shader)
{
	return true;
}