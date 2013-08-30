cbuffer uniforms
{
    //Default Uniform are registers in this exact order.
    //do not change any of this.
    float4x4 kCCUniformPMatrix_s;
    float4x4 kCCUniformMVMatrix_s;
    float4x4 kCCUniformMVPMatrix_s;
    float4 kCCUniformTime_s;
    float4 kCCUniformSinTime_s;
    float4 kCCUniformCosTime_s;
    float4 kCCUniformRandom01_s;
    float4 u_color;
    float CC_alpha_value;
};
struct VS_OUTPUT
{
    float4 pos:SV_POSITION;
    float2 tex:TEXCOORD0;
    float4 col:COLOR0;
};
VS_OUTPUT main(float4 pos:POSITION,float2 tex:TEXCOORD0)
{
    VS_OUTPUT output;
    output.pos=mul(pos,kCCUniformMVPMatrix_s);
    output.tex=tex;
    output.col = u_color;
    return output;
}