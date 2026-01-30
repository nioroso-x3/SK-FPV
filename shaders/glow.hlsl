#include <stereokit.hlsli>

//--color:color = 1,1,1,1
//--tex_scale   = 1
//--diffuse     = white

float4       color;
float        tex_scale;
Texture2D    diffuse   : register(t0);
SamplerState diffuse_s : register(s0);

struct vsIn {
    float4 pos  : SV_POSITION;
    float3 norm : NORMAL0;
    float2 uv   : TEXCOORD0;
    float4 col  : COLOR0;
};
struct psIn {
    float4 pos   : SV_POSITION;
    float2 uv    : TEXCOORD0;
    float4 color : COLOR0;
    uint view_id : SV_RenderTargetArrayIndex;
};

psIn vs(vsIn input, uint id : SV_InstanceID) {
    psIn o;
    o.view_id = id % sk_view_count;
    id        = id / sk_view_count;

    float4 world = mul(input.pos, sk_inst[id].world);
    o.pos        = mul(world,     sk_viewproj[o.view_id]);

    o.uv    = input.uv * tex_scale;
    o.color = input.col * color * sk_inst[id].color;
    return o;
}

float4 ps(psIn input) : SV_TARGET {
    // Sample the texture
    float4 col = diffuse.Sample(diffuse_s, input.uv);

    // Gaussian blur kernel
    float2 texelSize = float2(1.0 / 1200.0, 1.0 / 800.0); // Assuming 1024x1024 texture size
    float4 blur = float4(0, 0, 0, 0);

    // 11x11 Gaussian kernel weights (approximated)
    float kernel[121] = {
        1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1,
        1, 2, 2, 3, 3, 3, 3, 3, 2, 2, 1,
        2, 2, 3, 4, 4, 4, 4, 4, 3, 2, 2,
        2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2,
        2, 3, 4, 5, 6, 6, 6, 5, 4, 3, 2,
        2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2,
        2, 3, 4, 5, 6, 6, 6, 5, 4, 3, 2,
        2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2,
        2, 2, 3, 4, 4, 4, 4, 4, 3, 2, 2,
        1, 2, 2, 3, 3, 3, 3, 3, 2, 2, 1,
        1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1
    };
    float kernelSum = 379.0;
    for (int i = 0; i < 121; i++) {
        kernelSum += kernel[i];
    }

    int index = 0;
    for (int y = -5; y <= 5; y++) {
        for (int x = -5; x <= 5; x++) {
            float2 offset = float2(x, y) * texelSize;
            blur += diffuse.Sample(diffuse_s, input.uv + offset) * kernel[index];
            index++;
        }
    }

    blur /= kernelSum;

    // Combine the original color with the blurred color
    float4 finalColor = lerp(blur, col, col.a);

    return finalColor * input.color;
}

