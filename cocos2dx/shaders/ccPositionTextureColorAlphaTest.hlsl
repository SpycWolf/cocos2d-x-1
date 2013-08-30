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
Texture2D shaderTexture;
SamplerState SampleType;
float4 main(float4 pos:SV_POSITION,float4 v_fragmentColor:COLOR0,float2 v_texCoord:TEXCOORD0) : SV_TARGET
{
   float4 texColor = shaderTexture.Sample(SampleType,v_texCoord);
   if(texColor.a<=CC_alpha_value)
   {
       discard;
   }
   return texColor*v_fragmentColor;
}
