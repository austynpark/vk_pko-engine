#ifndef RENDER_TYPE_H
#define RENDER_TYPE_H

#include "defines.h"

enum RenderPacketType
{
	RENDERPACKET_PLATFORM_DATA = 0,
	RENDERPACKET_MESH_DATA = 1,
	RENDERPACKET_MAX_COUNT,
};

struct RenderPacket
{
	void* memory;
	u64 size;
	RenderPacketType type;
} typedef RenderPacket;

struct RendererSettings
{
} typedef RendererSettings;

enum ShaderStage
{
	VERTEX_STAGE = 1,
	FRAGMENT_STAGE = 2,
	COMPUTE_STAGE = 4,
	MAX_SHADER_STAGE, 
};

enum PipelineType
{
	PIPELINE_TYPE_GRAPHICS,
	PIPELINE_TYPE_COMPUTE
};

struct ShaderLoadDesc
{
	const char* pFileName;
	const char* pEntryPointName;
};

struct DepthLoadDesc
{

};

struct PipelineLoadDesc
{
	DepthLoadDesc depthLoadDesc;
};

struct BufferLoadDesc
{

} typedef BufferLoadDesc;

struct TextureLoadDesc
{

} typedef TextureLoadDesc;

#endif