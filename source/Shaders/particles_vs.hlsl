//updates particle positions and passes to geometry shader

#define PI 3.1415f

cbuffer ParticlesBuffer : register(b1) {
	float quadSize;//how big each of the quads should be at most
	float time;
	float maxSpeed;
	float padding;
}

struct VS_IN {
	float3 position : POSITION;
	float4 data : COLOR;//xyz is velocity, w is time decal (between 0 and 1)
};

struct VS_OUT {
	float4 position : POSITION;
	float2 particleData : TEXCOORD0;//x is size of particle
};

VS_OUT main(VS_IN input) {
	VS_OUT output;

	//loop time and add delta for each particle
	float t = time + input.data.w;
	t = t - int(t);

	output.position = float4(input.position.xyz+input.data.xyz/*vel*/ * maxSpeed * t, 1);//apply position as a function of time and velocity

	//size should start at 0, go to 1 at t=0.5 then be at 0 again at t=1
	float size = sin(t * PI);
	output.particleData = float2(quadSize * size, 0);

	return output;
}