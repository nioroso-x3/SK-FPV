#include <stereokit.hlsli>

//--color:color = 1,1,1,1
//--tex_scale = 1
//--diffuse = white
//--mapX = gray
//--mapY = gray

float4 color;
float tex_scale;
Texture2D diffuse : register(t0);
SamplerState diffuse_s : register(s0);

Texture2D mapX : register(t1);
Texture2D mapY : register(t2);

SamplerState mapX_s : register(s1);
SamplerState mapY_s : register(s2);

struct vsIn {
float4 pos : SV_Position;
float3 norm : NORMAL0;
float2 uv : TEXCOORD0;
float4 col : COLOR0;
};
struct psIn {
float4 pos : SV_POSITION;
float2 uv : TEXCOORD0;
float4 color : COLOR0;
uint view_id : SV_RenderTargetArrayIndex;
};

psIn vs(vsIn input, uint id : SV_InstanceID) {
psIn o;
o.view_id = id % sk_view_count;
id = id / sk_view_count;


float4 world = mul(input.pos, sk_inst[id].world);
o.pos        = mul(world,     sk_viewproj[o.view_id]);

o.uv    = input.uv * tex_scale;
o.color = input.col * color * sk_inst[id].color;
return o;
}

float4 ps(psIn input) : SV_TARGET {
float new_u = mapX.Sample(mapX_s, input.uv).r;
float new_v = mapY.Sample(mapY_s, input.uv).r;

float2 remap_uv = float2(new_u, new_v);

return diffuse.Sample(diffuse_s, remap_uv) * input.color;
}


