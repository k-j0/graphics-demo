// Tessellation Hull Shader
// Prepares control points for tessellation

#define ONE_THIRD 0.3333333333f

cbuffer DynamicTessellation : register(b0) {
	matrix worldMatrix;
	float3 cameraPosition;
	float oneOverFarPlane;
	float tessellationMin;//minimum amount to apply (furthest from camera); should default to 1
	float tessellationMax;//maxmimum amount to apply (closest to camera); for example 64
	float tessellationRange;//between 1..infinity, how far we should keep applying tessellation at all (if set to 1, tessellation will happen all the way to far plane; at 2, only up to half distance; at infinity, no tessellation)
	float padding;
}

struct HS_IN{
	float3 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

struct ConstantOutputType{
	float edges[3] : SV_TessFactor;
	float inside : SV_InsideTessFactor;
};

struct HS_OUT{
	float3 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

ConstantOutputType PatchConstantFunction(InputPatch<HS_IN, 3> inputPatch, uint patchId : SV_PrimitiveID){
	ConstantOutputType output;

	//Compute world position of each control point
	float4 wp0 = mul(float4(inputPatch[0].position.xyz, 1.0f), worldMatrix);
	float4 wp1 = mul(float4(inputPatch[1].position.xyz, 1.0f), worldMatrix);
	float4 wp2 = mul(float4(inputPatch[2].position.xyz, 1.0f), worldMatrix);

	//Find distance between camera and cps
	float d0 = length(cameraPosition - wp0.xyz);
	float d1 = length(cameraPosition - wp1.xyz);
	float d2 = length(cameraPosition - wp2.xyz);

	//Put in -infinity..1 range (1 at camera, 0 further down)
	d0 = 1.0f - d0 * oneOverFarPlane * tessellationRange;
	d1 = 1.0f - d1 * oneOverFarPlane * tessellationRange;
	d2 = 1.0f - d2 * oneOverFarPlane * tessellationRange;

	//get tessellation factor for each control point
	float tf0 = d0 <= 0 ? tessellationMin : tessellationMin + d0 * (tessellationMax - tessellationMin);
	float tf1 = d1 <= 0 ? tessellationMin : tessellationMin + d1 * (tessellationMax - tessellationMin);
	float tf2 = d2 <= 0 ? tessellationMin : tessellationMin + d2 * (tessellationMax - tessellationMin);

	//set tessellation factors for the three edges of the tri
	//each edge corresponds to average tess factor of opposite two CPs
	//this way when two triangles are right next to each other they get the same tess factor along the same edge
	// (because each edge is opposite the CP with the same index; CP 0 is the only CP not on edge 0, and so on)
	//		    1
	//		   / \
	//		  2   0       <-- like this
	//		 /     \
	//		0---1---2
	output.edges[0] = (tf1 + tf2) * 0.5f;
	output.edges[1] = (tf0 + tf2) * 0.5f;
	output.edges[2] = (tf0 + tf1) * 0.5f;

	output.inside = (tf0 + tf1 + tf2) * ONE_THIRD;

	return output;
}


[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstantFunction")]
HS_OUT main(InputPatch<HS_IN, 3> patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID){
	HS_OUT output;


	//Pass inputs over to Domain
	output.position = patch[pointId].position;
	output.tex = patch[pointId].tex;
	output.normal = patch[pointId].normal;
	output.tangent = patch[pointId].tangent;

	return output;
}