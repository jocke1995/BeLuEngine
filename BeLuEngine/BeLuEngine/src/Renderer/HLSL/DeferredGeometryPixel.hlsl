struct VS_OUT
{
	float4 pos      : SV_Position;
};

struct PS_OUTPUT
{
	float4 AlbedoColor	: SV_TARGET0;
	float4 NormalColor	: SV_TARGET1;
	float4 MatColor		: SV_TARGET2;
};

PS_OUTPUT PS_main(VS_OUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;

	output.AlbedoColor = float4(1.0f, 0.0f, 0.0f, 1.0f);
	output.NormalColor = float4(0.0f, 1.0f, 0.0f, 1.0f);
	output.MatColor = float4(0.5f, 0.2f, 0.0f, 1.0f);

	return output;
}