Texture2D shaderTexture;
SamplerState SampleType;
float4 main(float4 pos:SV_POSITION,float2 tex:TEXCOORD0,float4 col:COLOR0):SV_TARGET
{
    float4 color = shaderTexture.Sample(SampleType,tex);
    return color*col;
}