// テクスチャとサンプラ
Texture2D<float4> tex:register(t0);// テクスチャ0番
Texture2D<float4> sph:register(t1);//sph
Texture2D<float4> spa:register(t2);//spa
Texture2D<float4> clut:register(t3);
SamplerState smp:register(s0);// サンプラ0番
cbuffer mat : register(b0) {
	matrix world;
    matrix view;
	matrix wvp;
    matrix lvp;
    float4 peye;
};

cbuffer Material : register(b1) {
	float4 diffuse;	// マテリアル用
	float4 specular;
	float4 ambient;
}

cbuffer bones : register(b2) {
	matrix bonemats[512];
}

struct Out {
	float4 svpos : SV_POSITION;
	float4 pos : POSITION;
	float4 posSM : POSITION_SM;
	float2 uv : TEXCOORD;
	float3 normal: NORMAL;
	min16uint2 boneno : BONENO;
	min16uint weight : WEIGHT;
	float3 color : COLOR;
};

struct ShadowInput {
	float4 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

// 頂点シェーダ
Out vs( float4 pos : POSITION,float3 normal:NORMAL,float2 uv:TEXCOORD, min16uint2 boneno:BONENO, min16uint weight:WEIGHT, float3 color:COLOR)
{	
	float w = weight / 100.f;
	matrix n = bonemats[boneno.x] * w + bonemats[boneno.y] * (1 - w);
	pos = mul(n, pos);

	Out o;
	pos = mul(wvp,pos);
	normal = mul(world, normal);

	o.uv = uv;
	o.normal = normal;
	o.boneno = boneno;
	o.weight = weight;
	o.svpos = o.pos = pos;
	return o;
}

//ピクセルシェーダ
float4 ps(Out o) : SV_Target{
	float3 eye = (0,20,-15);// 視点
	float3 ray = eye - o.pos;// 視線ベクトル
	float3 rayReverse = -ray;//逆視線ベクトル
	rayReverse = normalize(reflect(rayReverse, o.normal));
	float3 light = normalize(float3(-1,1,-1));
    float Blightness = dot(o.normal,light);
	float ref = reflect(light, o.normal.xyz);
	float b = saturate(dot(-light, o.normal.xyz)) + 0.2f;
	//float3 toon = clut.Sample(smp, float2(0, 1 - b)).xyz;
	float spec = saturate(pow(dot(ref, normalize(ref)), specular.a));
	//float2 RayUv = rayReverse.xy;
	float2 RayUv = o.normal.xy;
	RayUv = float2(1, -1) * (RayUv + float2(1, -1)) / 2;
	float3 matcolor = diffuse * b;

    float4 toon = clut.Sample(smp, float2(0.f,1.f - Blightness));
	/*float spec = saturate(dot(reflect(-light, o.normal), 1.f));
	spec = pow(spec, 10);
	float Blightness = saturate(dot(light, o.normal.xyz));*/
	//return float4(o.normal, 1.f);
	//return float4(matcolor, 1.f)
	//return float4(o.color, 1);
    return float4(spa.Sample(smp, o.uv) + sph.Sample(smp, o.uv) * tex.Sample(smp, o.uv).rgb * saturate(saturate(toon.rgb * diffuse.rgb * spec/1.5 + ambient.rgb)), 1);
}

float4 vsShadow(float4 pos : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint weight : WEIGHT, float3 color : COLOR) : SV_POSITION
{
    float w = weight / 100.f;
    matrix n = bonemats[boneno.x] * w + bonemats[boneno.y] * (1 - w);
    pos = mul(n, pos);
    float4 Pos = mul(world, pos);
    Pos = mul(lvp, Pos);
    return Pos;
}
