// passes input over to hull shader

struct VS_IN {
	float3 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

struct VS_OUT {
	float3 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

VS_OUT main(VS_IN input) {
	VS_OUT output;

	//pass to hull
	output.position = input.position;
	output.tex = input.tex;
	output.normal = input.normal;
	output.tangent = input.tangent;

	return output;
}