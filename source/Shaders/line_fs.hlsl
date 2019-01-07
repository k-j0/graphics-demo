
SamplerState sampler0 : register(s0);

struct FS_IN {
	float4 position : SV_POSITION;
};

float4 main(FS_IN input) : SV_TARGET{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}