struct VS_OUT
{
	float4 pos      : SV_Position;
};

float4 PS_main(VS_OUT input) : SV_TARGET0
{
	return float4(75/255.0f, 0.0f, 130/255.0f, 1.0f);
}