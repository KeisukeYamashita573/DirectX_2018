Texture2D<float4> tex:register(t0);
Texture2D<float> depth:register(t1);
SamplerState smp:register(s0);
struct Output {
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};

Output vs(float4 pos : POSITION, float2 uv : TEXCOORD) {
	Output output;
	output.svpos = pos;
	output.uv = uv;
	return output;
}

float4 ps(Output input) : SV_Target{
	float w,h,level;
	tex.GetDimensions(0, w, h, level);
	float dx = 1.0f / w;
	float dy = 1.0 / h;
	float dep = pow(depth.Sample(smp, input.uv),100);
	//return float4(dep, dep, dep, 1);
    float4 color = float4(0, 0, 0, 0);
    color += tex.Sample(smp, input.uv, int2(-2, 2)) * 1 / 256;
    color += tex.Sample(smp, input.uv, int2(-1, 2)) * 4 / 256;
    color += tex.Sample(smp, input.uv, int2(0, 2)) * 6 / 256;
    color += tex.Sample(smp, input.uv, int2(1, 2)) * 4 / 256;
    color += tex.Sample(smp, input.uv, int2(2, 2)) * 1 / 256;

    color += tex.Sample(smp, input.uv, int2(-2, 1)) * 4 / 256;
    color += tex.Sample(smp, input.uv, int2(-1, 1)) * 16 / 256;
    color += tex.Sample(smp, input.uv, int2(0, 1)) * 24 / 256;
    color += tex.Sample(smp, input.uv, int2(1, 1)) * 16 / 256;
    color += tex.Sample(smp, input.uv, int2(2, 1)) * 4 / 256;

    color += tex.Sample(smp, input.uv, int2(-2, 0)) * 6 / 256;
    color += tex.Sample(smp, input.uv, int2(-1, 0)) * 24 / 256;

    color += tex.Sample(smp, input.uv, int2(1, 0)) * 24 / 256;
    color += tex.Sample(smp, input.uv, int2(2, 0)) * 6 / 256;

    color += tex.Sample(smp, input.uv, int2(-2, -1)) * 4 / 256;
    color += tex.Sample(smp, input.uv, int2(-1, -1)) * 16 / 256;
    color += tex.Sample(smp, input.uv, int2(0, -1)) * 24 / 256;
    color += tex.Sample(smp, input.uv, int2(1, -1)) * 16 / 256;
    color += tex.Sample(smp, input.uv, int2(2, -1)) * 4 / 256;

    color += tex.Sample(smp, input.uv, int2(-2, -2)) * 1 / 256;
    color += tex.Sample(smp, input.uv, int2(-1, -2)) * 4 / 256;
    color += tex.Sample(smp, input.uv, int2(0, -2)) * 6 / 256;
    color += tex.Sample(smp, input.uv, int2(1, -2)) * 4 / 256;
    color += tex.Sample(smp, input.uv, int2(2, -2)) * 1 / 256;

  //  return color;
	return tex.Sample(smp,input.uv);
	float4 col = tex.Sample(smp,input.uv);
	/*col = col * 4 - tex.Sample(smp, input.uv + float2(-dx, 0)) -
		tex.Sample(smp, input.uv + float2(dx, 0)) -
		tex.Sample(smp, input.uv + float2(0, dy)) -
		tex.Sample(smp, input.uv + float2(0, -dy));*/
	//float b = dot(float3(0.298912f, 0.586611f, 0.114478f), 1 - col.rgb);
	//b = pow(b, 4);
	float b = dot(float3(0.298912f,0.586611f,0.114478f),col.rgb);
	//return float4(1 - col.rgb, col.a);	// îΩì]
	//return float4(b,b,b,1);	// ÉÇÉmÉNÉç
	//return float4(1 - b, 1 - b, 1 - b, 1);	// É\ÉâÉäÉ[Å[ÉVÉáÉì
}