Texture2D<float> tex : register(t0);
Texture2D<float> depth : register(t1);
SamplerState smp : register(s0);
struct OutPut
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

OutPut vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    OutPut put;
    put.svpos = pos;
    put.uv = uv;
    return put;
}

float4 ps(OutPut input) : SV_Target
{
    float dep = pow(depth.Sample(smp, input.uv),20);
    return float4(dep, dep, dep, 1);

}