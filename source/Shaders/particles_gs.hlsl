//Creates billboards from points and animates them

cbuffer MatrixBuffer : register(b0) {
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
}

struct GS_IN {
	float4 position : POSITION;
	float2 particleData : TEXCOORD0;// x is size of particle
};

struct GS_OUT{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

[maxvertexcount(6)]
void main(point GS_IN input[1], inout TriangleStream<GS_OUT> stream){
	GS_OUT output;

	float2 decals[6];//by how much each of the 6 verts should be displaced to form the quads:
	decals[0] = float2(-0.5f, 0.5f);
	decals[1] = float2(-0.5f, -0.5f);
	decals[2] = float2(0.5f, -0.5f);
	decals[3] = float2(0.5f, -0.5f);
	decals[4] = float2(0.5f, 0.5f);
	decals[5] = float2(-0.5f, 0.5f);

	[unroll]//2 triangles:
	for (int i = 0; i < 2; ++i) {
		[unroll]//3 vertices per tri:
		for (int j = 0; j < 3; ++j) {

			output.position = mul(input[0].position, worldMatrix);
			output.position = mul(output.position, viewMatrix);
			output.position = output.position + float4(decals[i * 3 + j] * input[0].particleData.x, 0, 0);//by applying the decals after world-view and before projection (instead of before all 3), we orient them towards the camera automatically
			output.position = mul(output.position, projectionMatrix);
			output.tex = decals[i * 3 + j] + 0.5f;
			stream.Append(output);
		}

		//next tri!
		stream.RestartStrip();
	}
}