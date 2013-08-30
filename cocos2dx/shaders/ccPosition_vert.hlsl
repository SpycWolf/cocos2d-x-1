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
    float4 color:COLOR0;
};
VS_OUTPUT main(float4 pos:POSITION)
{
    VS_OUTPUT o = (VS_OUTPUT)0;
    o.pos = mul(pos,kCCUniformMVPMatrix_s);
    o.color=u_color;
    return o;
}