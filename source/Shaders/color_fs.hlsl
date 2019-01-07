//Simply return the fragment's color

struct FS_IN {
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

float4 main(FS_IN input) : SV_TARGET {
	return input.color;
}
