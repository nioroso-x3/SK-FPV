#include <stereokit.hlsli>

//--color:color = 1,1,1,1
//--tex_scale = 1
//--darken_factor = 1
//--diffuse = white
//--mapX = gray
//--mapY = gray
//--over = gray

float4 color;
float tex_scale;
float darken_factor;
Texture2D diffuse : register(t0);
SamplerState diffuse_s : register(s0);

Texture2D mapX : register(t1);
Texture2D mapY : register(t2);
Texture2D over : register(t3);


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


float new_u = mapX.Sample(diffuse_s, input.uv).r;
float new_v = mapY.Sample(diffuse_s, input.uv).r;

float2 remap_uv = float2(new_u, new_v);

if (new_u < 0.0 || new_u > 1.0 || new_v < 0.0 || new_v > 1.0)
    return float4(0, 0, 0, 1); // Black fill for out-of-bounds areas

float4 ov_color = over.Sample(diffuse_s, remap_uv);
if (ov_color.a >= 0.9){
    return ov_color * input.color * darken_factor;
}

return diffuse.Sample(diffuse_s, remap_uv) * input.color * darken_factor;
}


