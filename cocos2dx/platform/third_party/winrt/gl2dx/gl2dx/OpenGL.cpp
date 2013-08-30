//-----------------------------------------------------------------------------------------------
// Copyright (c) 2012 Andrew Garrison
// Copyright (c) Microsoft Open Technologies, Inc.
//-----------------------------------------------------------------------------------------------
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
// and associated documentation files (the "Software"), to deal in the Software without 
// restriction, including without limitation the rights to use, copy, modify, merge, publish, 
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or 
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//-----------------------------------------------------------------------------------------------
#include "OpenGL.h"
#include <d3d11_1.h>
#include <DirectXMath.h>

using namespace DirectX;
namespace gl2dx
{

    //*****************************************************************
    //Constructor.
    //*****************************************************************
    InternalVBO::InternalVBO()
    {
        vbo = NULL;
        location = 0;
        firstTimeInit=true;
    }
    //*****************************************************************
    //Destructor.
    //*****************************************************************
    InternalVBO::~InternalVBO()
    {
        SAFE_RELEASE(vbo);
        location=0;
    }
    //*****************************************************************
    //Internal buffer.
    //*****************************************************************
    void InternalVBO::CreateInternalBuffer(ID3D11Device* device,const int size)
    {
        SAFE_RELEASE(vbo);
        D3D11_BUFFER_DESC bufferDesc={};
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.ByteWidth = size;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        device->CreateBuffer(&bufferDesc,NULL,&vbo);
        location=0;
    }

    //*****************************************************************
    //Map an buffer.
    //*****************************************************************
    void InternalVBO::Map(ID3D11DeviceContext* context,const int size,void** output)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if(firstTimeInit)
        {
            context->Map(vbo,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
            *output = mappedResource.pData;
            previousLocation=location=0;
            firstTimeInit=false;
        }
        else
        {
            if((location+size)>=VBO_SIZE)
            {                
                context->Map(vbo,0,D3D11_MAP_WRITE_DISCARD,0,&mappedResource);
                *output = mappedResource.pData;
                previousLocation=0;
                location=size;
            }
            else
            {
                context->Map(vbo,0,D3D11_MAP_WRITE_NO_OVERWRITE,0,&mappedResource);
                *output = (void*)(((char*)mappedResource.pData)+location);
                previousLocation=location;
                location+=size;
            }
        }
    }

    //*****************************************************************
    //Unmap the buffer.
    //*****************************************************************
    void InternalVBO::Unmap(ID3D11DeviceContext* context)
    {
        context->Unmap(vbo,0);
    }

    //*****************************************************************
    //Constructor.
    //*****************************************************************
    OpenGL::OpenGL(ID3D11Device* device, ID3D11DeviceContext* context):
    backBufferRenderTargetView_(NULL),backBufferDepthStencilView_(NULL)
    {
        currentProgram_=0;
        for(int i=0;i<MAX_INTERNAL_VBO;i++)
        {
            internalVbos_[i].CreateInternalBuffer(device,VBO_SIZE);
        }
        memset(vertexAttribArray,0,sizeof(vertexAttribArray));
        memset(currentTexture_,0,sizeof(currentTexture_));
        memset(&states_,0,sizeof(states_));
        states_.enableTexture2D_=true;

        currentFrameBuffer_=0;
        currentRenderBuffer_=0;
        currentVertexBuffer_=0;
        currentIndexBuffer_=0;
        currentTextureUnit_=0;
        currentActiveTexture_=0;
        currentProgram_=0;
        oldFrameBuffer_=-1; // Must be -1 in case the user did not set the default RenderTarget View

        memset(clearColor_,0,sizeof(clearColor_));
        depthValue_=0;
        generateMipmaps_=GL_TEXTURE_2D;
        stencilValue_=0;

        device_ = device;
        context_ = context;
        device_->AddRef();
        context_->AddRef();

        memset(&scissorRect_,0,sizeof(scissorRect_));
        memset(&rasterizerDesc_,0,sizeof(rasterizerDesc_));

        //Set up default raster state.
        rasterizerDesc_.FillMode = D3D11_FILL_SOLID;
      rasterizerDesc_.CullMode = D3D11_CULL_NONE;
      rasterizerDesc_.FrontCounterClockwise = true;
      rasterizerDesc_.DepthClipEnable = true;
      rasterizerDesc_.ScissorEnable = false;
      rasterizerDesc_.MultisampleEnable = false;
      rasterizerDesc_.AntialiasedLineEnable = false;      
      device_->CreateRasterizerState(&rasterizerDesc_, &rasterizerState);
      context_->RSSetState(rasterizerState);


      //Depth stencil for back buffer.
      depthStencilDesc_.DepthEnable=false;
      depthStencilDesc_.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
      depthStencilDesc_.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;
      depthStencilDesc_.StencilEnable=false;
      depthStencilDesc_.StencilReadMask=0xff;
      depthStencilDesc_.StencilWriteMask=0xff;
      depthStencilDesc_.FrontFace.StencilDepthFailOp=D3D11_STENCIL_OP_KEEP;
      depthStencilDesc_.FrontFace.StencilFailOp=D3D11_STENCIL_OP_KEEP;
      depthStencilDesc_.FrontFace.StencilFunc=D3D11_COMPARISON_ALWAYS;
      depthStencilDesc_.FrontFace.StencilPassOp=D3D11_STENCIL_OP_KEEP;
      depthStencilDesc_.BackFace.StencilDepthFailOp=D3D11_STENCIL_OP_KEEP;
      depthStencilDesc_.BackFace.StencilFailOp=D3D11_STENCIL_OP_KEEP;
      depthStencilDesc_.BackFace.StencilFunc=D3D11_COMPARISON_ALWAYS;
      depthStencilDesc_.BackFace.StencilPassOp=D3D11_STENCIL_OP_KEEP;
      device_->CreateDepthStencilState(&depthStencilDesc_,&depthStencilState_);

      //Set the default blend state.
      float BlendFactor[4]={1,1,1,1};
      blendDesc_.AlphaToCoverageEnable=false;
      blendDesc_.IndependentBlendEnable=false;
      blendDesc_.RenderTarget[0].BlendEnable=false;
      blendDesc_.RenderTarget[0].BlendOp=D3D11_BLEND_OP_ADD;
      blendDesc_.RenderTarget[0].BlendOpAlpha=D3D11_BLEND_OP_ADD;
      blendDesc_.RenderTarget[0].DestBlend=D3D11_BLEND_INV_SRC_ALPHA;
	  blendDesc_.RenderTarget[0].DestBlendAlpha=D3D11_BLEND_ONE;
      blendDesc_.RenderTarget[0].RenderTargetWriteMask=0xf;
      blendDesc_.RenderTarget[0].SrcBlend=D3D11_BLEND_SRC_ALPHA;
	  blendDesc_.RenderTarget[0].SrcBlendAlpha=D3D11_BLEND_ONE;
      device_->CreateBlendState(&blendDesc_,&blendState_);
      maskRef=1;
	  blendMask_=0xffffffff;
   }

   //**********************************************************
   //Destructor.
   //**********************************************************
   OpenGL::~OpenGL()
   {
       SAFE_RELEASE(rasterizerState);
       SAFE_RELEASE(context_);
       SAFE_RELEASE(device_);
   }

   //*********************************************************
   //Bind the texture to the current texture unit.
   //*********************************************************
   void OpenGL::glBindTexture(GLenum target, GLuint texture)
   {
       currentActiveTexture_=texture;
       currentTexture_[currentTextureUnit_] = texture;
   }


   
   //*********************************************************
   //Update the texture sampler parameter.
   //*********************************************************
   void OpenGL::glTexParameteri (GLenum target, GLenum pname, GLint param)
   {
	   std::map<GLuint,Texture2DInfo>::iterator itt = textures_.find(currentTexture_[currentTextureUnit_]);
	   if(itt==textures_.end())
	   {
		   return;
	   }
       Texture2DInfo* currentTexture=&itt->second;
       switch(pname)
       {
       case GL_TEXTURE_MIN_FILTER:
           switch(param)
           {
           case GL_NEAREST:
               currentTexture->samplerDesc_.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
               break;
           case GL_LINEAR:
               currentTexture->samplerDesc_.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
               break;
           case GL_NEAREST_MIPMAP_NEAREST:
               currentTexture->samplerDesc_.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
               break;
           case GL_LINEAR_MIPMAP_NEAREST:
               currentTexture->samplerDesc_.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
               break;
           case GL_LINEAR_MIPMAP_LINEAR:
               currentTexture->samplerDesc_.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
               break;
           }           
           break;
       case GL_TEXTURE_MAG_FILTER:
           switch(param)
           {
           case GL_NEAREST:
               currentTexture->samplerDesc_.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
               break;
           case GL_LINEAR:
               currentTexture->samplerDesc_.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
               break;
           case GL_NEAREST_MIPMAP_NEAREST:
               currentTexture->samplerDesc_.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
               break;
           case GL_LINEAR_MIPMAP_NEAREST:
               currentTexture->samplerDesc_.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
               break;
           case GL_LINEAR_MIPMAP_LINEAR:
               currentTexture->samplerDesc_.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
               break;
           }           
           break;
       case GL_TEXTURE_WRAP_R:
           switch(param)
           {
           case GL_CLAMP_TO_EDGE:
           case GL_CLAMP:
               currentTexture->samplerDesc_.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
               break;
           case GL_CLAMP_TO_BORDER:
               currentTexture->samplerDesc_.AddressW=D3D11_TEXTURE_ADDRESS_BORDER;
               break;
           case GL_REPEAT:
               currentTexture->samplerDesc_.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;
               break;
           }
           break;
       case GL_TEXTURE_WRAP_S:
           switch(param)
           {
           case GL_CLAMP_TO_EDGE:
           case GL_CLAMP:
               currentTexture->samplerDesc_.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP;
               break;
           case GL_CLAMP_TO_BORDER:
               currentTexture->samplerDesc_.AddressU=D3D11_TEXTURE_ADDRESS_BORDER;
               break;
           case GL_REPEAT:
               currentTexture->samplerDesc_.AddressU=D3D11_TEXTURE_ADDRESS_WRAP;
               break;
           }
           break;
       case GL_TEXTURE_WRAP_T:
           switch(param)
           {
           case GL_CLAMP_TO_EDGE:
           case GL_CLAMP:
               currentTexture->samplerDesc_.AddressV=D3D11_TEXTURE_ADDRESS_CLAMP;
               break;
           case GL_CLAMP_TO_BORDER:
               currentTexture->samplerDesc_.AddressV=D3D11_TEXTURE_ADDRESS_BORDER;
               break;
           case GL_REPEAT:
               currentTexture->samplerDesc_.AddressV=D3D11_TEXTURE_ADDRESS_WRAP;
               break;
           }
           break;
       }
       SAFE_RELEASE(currentTexture->samplerState_);
       device_->CreateSamplerState(&currentTexture->samplerDesc_,&currentTexture->samplerState_);
   }

   //*********************************************************
   //Draw the vertices with a specific program.
   //*********************************************************
   void OpenGL::DrawVerticesWithProgram(GLenum mode,GLsizei first,ShaderProgram* currentProgram,const GLuint offset,const GLuint count,const GLuint stride)
   {
       D3D11_PRIMITIVE_TOPOLOGY primitiveTopology;
	   switch(mode)
	   {
        case GL_POINTS:
            primitiveTopology=D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
            break;
		case GL_LINE_STRIP:
			primitiveTopology=D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
			break;
        case GL_LINE_LOOP:
			primitiveTopology=D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
            break;
		case GL_LINES:
			primitiveTopology=D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case GL_TRIANGLE_STRIP:
			primitiveTopology=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			break;
        case GL_TRIANGLE_FAN:
			primitiveTopology=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            break;
		case GL_TRIANGLES:
			primitiveTopology=D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		}
		float blendFactor[4]={1,1,1,1};
		glBuildUniformConstantBuffer(currentProgram_);
		context_->IASetPrimitiveTopology(primitiveTopology);
		context_->IASetInputLayout(currentProgram->shaderLinkage_.inputLayout_);
        context_->IASetVertexBuffers(0,1,&internalVbos_[0].vbo,&stride,&offset);
        context_->VSSetConstantBuffers(0,1,&currentProgram->shaderLinkage_.uniformBuffer);
		context_->OMSetBlendState(blendState_,blendFactor,blendMask_);
		context_->OMSetDepthStencilState(depthStencilState_,maskRef);
        SetShaderTextures(currentProgram);
		context_->Draw(count,first);
        UnSetShaderTextures(currentProgram);
        ID3D11Buffer* buffer[1]={NULL};
   }

   //*********************************************************
   //Set the textures that used.
   //*********************************************************
   void OpenGL::SetShaderTextures(ShaderProgram* program)
   {
       int slotIndex=0;
       std::map<GLuint,ShaderVariable>::iterator itt = program->shaderVariableByIndex_.begin();
       for(;itt!=program->shaderVariableByIndex_.end();++itt)
       {
           ShaderVariable* currentVariable=&itt->second;
           if(currentVariable->type==SHADER_VARIABLE_SAMPLER)
           {
               std::map<GLuint,Texture2DInfo>::iterator textureItt=textures_.find(currentTexture_[currentVariable->value]);
               if(textureItt==textures_.end())
               {
                   continue;
               }
               Texture2DInfo* currentTexture=&textureItt->second;
               context_->PSSetShaderResources(slotIndex,1,&currentTexture->texture_);
               context_->PSSetSamplers(slotIndex,1,&currentTexture->samplerState_);
               ++slotIndex;
           }
       }
   }

   //*********************************************************
   //Set the textures that used.
   //*********************************************************
   void OpenGL::UnSetShaderTextures(ShaderProgram* program)
   {
       int slotIndex=0;
       std::map<GLuint,ShaderVariable>::iterator itt = program->shaderVariableByIndex_.begin();
       for(;itt!=program->shaderVariableByIndex_.end();++itt)
       {
           ShaderVariable* currentVariable=&itt->second;
           if(currentVariable->type==SHADER_VARIABLE_SAMPLER)
           {
               std::map<GLuint,Texture2DInfo>::iterator textureItt=textures_.find(currentTexture_[currentVariable->value]);
               if(textureItt==textures_.end())
               {
                   continue;
               }
               Texture2DInfo* currentTexture=&textureItt->second;
               ID3D11ShaderResourceView* empty[1]={NULL};
               context_->PSSetShaderResources(slotIndex,1,empty);
               ++slotIndex;
           }
       }
   }

   //*********************************************************
   //Update Buffer position.
   //*********************************************************
   void OpenGL::glUpdatePositionFan(GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,float* vboPosition,GLuint stride)
   {
       //Copy all positions.
       for(int j=0;j<count;++j)
       {
        //Copy all positions.
        if(attInfo->stride!=0)
        {
			//Interleaved
            if(j<=2)
            {
                float* position=(float*)(byteArray+(j*attInfo->stride));
				memset(vboPosition,0,sizeof(float)*3);
                memcpy(vboPosition,position,sizeof(float)*attInfo->size);
                vboPosition+=(stride>>2);
            }
            else
            {
                float* position0=(float*)(byteArray+(j*attInfo->stride));
                float* position1=(float*)(byteArray+((j-1)*attInfo->stride));
                float* position2=(float*)(byteArray+(0*attInfo->stride));
                               
                memset(vboPosition,0,sizeof(float)*3);
                memcpy(vboPosition,position0,sizeof(float)*attInfo->size);
                vboPosition+=(stride>>2);
                               
                memset(vboPosition,0,sizeof(float)*3);
                memcpy(vboPosition,position1,sizeof(float)*attInfo->size);
                vboPosition+=(stride>>2);

                memset(vboPosition,0,sizeof(float)*3);
                memcpy(vboPosition,position2,sizeof(float)*attInfo->size);
                vboPosition+=(stride>>2);
            }
        }
        else
        {
			//Non interleaved.
            if(j<=2)
            {
                int xyzOffset=j*sizeof(float)*attInfo->size;
                float* position=(float*)(byteArray+xyzOffset);
                               
                memset(vboPosition,0,sizeof(float)*3);
                memcpy(vboPosition,position,sizeof(float)*attInfo->size);
                vboPosition+=(stride>>2);
            }
            else
            {
                int xyzOffset=j*sizeof(float)*attInfo->size;
                float* position=(float*)(byteArray+xyzOffset);
                               
                memset(vboPosition,0,sizeof(float)*3);
                memcpy(vboPosition,position,sizeof(float)*attInfo->size);
                vboPosition+=(stride>>2);

                xyzOffset=(j-1)*sizeof(float)*attInfo->size;
                position=(float*)(byteArray+xyzOffset);
                               
                memset(vboPosition,0,sizeof(float)*3);
                memcpy(vboPosition,position,sizeof(float)*attInfo->size);
                vboPosition+=(stride>>2);

                xyzOffset=(0)*sizeof(float)*attInfo->size;
                position=(float*)(byteArray+xyzOffset);
                               
                memset(vboPosition,0,sizeof(float)*3);
                memcpy(vboPosition,position,sizeof(float)*attInfo->size);
                vboPosition+=(stride>>2);
            }
        }
       }
   }

   //*********************************************************
   //Update Color position.
   //*********************************************************
   void OpenGL::glUpdateColorFan(GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,float* vboColor,GLuint stride)
   {
       for(int j=0;j<count;++j)
       {
           if(attInfo->type==GL_UNSIGNED_BYTE)
           {
               //Convert from unsigned byte to float.
               assert(attInfo->stride!=0);
               if(j<=2)
               {
                   unsigned int* color = (unsigned int*)(byteArray+(j*attInfo->stride));
                   float a = (float)((*color&0xff000000)>>24);
                   float b = (float)((*color&0x00ff0000)>>16);
                   float g = (float)((*color&0x0000ff00)>>8);
                   float r = (float)((*color&0x000000ff));
                   r/=255.0f;g/=255.0f;b/=255.0f;a/=255.0f;
                   float rgba[4]={r,g,b,a};
                   memset(vboColor,0,sizeof(float)*4);
                   memcpy(vboColor,rgba,sizeof(float)*attInfo->size);
                   vboColor+=(stride>>2);
                }
                else
                {
                    unsigned int* color = (unsigned int*)(byteArray+(j*attInfo->stride));
                    float a = (float)((*color&0xff000000)>>24);
                    float b = (float)((*color&0x00ff0000)>>16);
                    float g = (float)((*color&0x0000ff00)>>8);
                    float r = (float)((*color&0x000000ff));
                    r/=255.0f;g/=255.0f;b/=255.0f;a/=255.0f;
                    float rgba[4]={r,g,b,a};
				    memset(vboColor,0,sizeof(float)*4);
                    memcpy(vboColor,rgba,sizeof(float)*attInfo->size);
                    vboColor+=(stride>>2);

                    color = (unsigned int*)(byteArray+((j-1)*attInfo->stride));
                    a = (float)((*color&0xff000000)>>24);
                    b = (float)((*color&0x00ff0000)>>16);
                    g = (float)((*color&0x0000ff00)>>8);
                    r = (float)((*color&0x000000ff));
                    r/=255.0f;g/=255.0f;b/=255.0f;a/=255.0f;
                    float rgba1[4]={r,g,b,a};
				    memset(vboColor,0,sizeof(float)*4);
                    memcpy(vboColor,rgba1,sizeof(float)*attInfo->size);
                    vboColor+=(stride>>2);

                    color = (unsigned int*)(byteArray+((0)*attInfo->stride));
                    a = (float)((*color&0xff000000)>>24);
                    b = (float)((*color&0x00ff0000)>>16);
                    g = (float)((*color&0x0000ff00)>>8);
                    r = (float)((*color&0x000000ff));
                    r/=255.0f;g/=255.0f;b/=255.0f;a/=255.0f;
                    float rgba2[4]={r,g,b,a};
				    memset(vboColor,0,sizeof(float)*4);
                    memcpy(vboColor,rgba1,sizeof(float)*attInfo->size);
                    vboColor+=(stride>>2);
                }
            }
            else if(attInfo->type==GL_FLOAT)
            {
                //Copy the color as is.
			    assert(attInfo->stride!=0);
                if(j<=2)
                {
                    float* position=(float*)(byteArray+(j*attInfo->stride));
				    memset(vboColor,0,sizeof(float)*4);
                    memcpy(vboColor,position,sizeof(float)*attInfo->size);
                    vboColor+=(stride>>2);
                }
                else
                {
                    float* position=(float*)(byteArray+(j*attInfo->stride));
				    memset(vboColor,0,sizeof(float)*4);
                    memcpy(vboColor,position,sizeof(float)*attInfo->size);
                    vboColor+=(stride>>2);
                    position=(float*)(byteArray+((j-1)*attInfo->stride));
				    memset(vboColor,0,sizeof(float)*4);
                    memcpy(vboColor,position,sizeof(float)*attInfo->size);
                    vboColor+=(stride>>2);
                    position=(float*)(byteArray+((0)*attInfo->stride));
				    memset(vboColor,0,sizeof(float)*4);
                    memcpy(vboColor,position,sizeof(float)*attInfo->size);
                    vboColor+=(stride>>2);
                }
            }
       }
   }

   //*********************************************************
   //Update TexCoord position.
   //*********************************************************
   void OpenGL::glUpdateTexCoordFan(GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,float* vboTexCoord,GLuint stride)
   {
       for(int j=0;j<count;++j)
       {
           //Copy texture coordinate.
           if(attInfo->stride!=0)
           {
               //Interleaved.
               if(j<=2)
               {
                   float* position=(float*)(byteArray+(j*attInfo->stride));						       
                   memset(vboTexCoord,0,sizeof(float)*2);
                   float v[2]={*position,1.0f-(*(position+1))};
                   memcpy(vboTexCoord,v,sizeof(float)*attInfo->size);
                   vboTexCoord+=(stride>>2);
                }
                else
                {
                    float* position=(float*)(byteArray+(j*attInfo->stride));						       
                    memset(vboTexCoord,0,sizeof(float)*2);
                    float v[2]={*position,1.0f-(*(position+1))};
                    memcpy(vboTexCoord,v,sizeof(float)*attInfo->size);
                    vboTexCoord+=(stride>>2);

					position=(float*)(byteArray+((j-1)*attInfo->stride));
					memset(vboTexCoord,0,sizeof(float)*2);
                    {
                    float v[2]={*position,1.0f-(*(position+1))};
                    memcpy(vboTexCoord,v,sizeof(float)*attInfo->size);
                    }
                    vboTexCoord+=(stride>>2);

					position=(float*)(byteArray+(0*attInfo->stride));
					memset(vboTexCoord,0,sizeof(float)*2);
                    {
                    float v[2]={*position,1.0f-(*(position+1))};
                    memcpy(vboTexCoord,v,sizeof(float)*attInfo->size);
                    }
                    vboTexCoord+=(stride>>2);
                }
            }
            else
            {
                if(j<=2)
                {
					int xyzOffset=j*sizeof(float)*attInfo->size;
					float* position=(float*)(byteArray+xyzOffset);						       
                    memset(vboTexCoord,0,sizeof(float)*2);
                    float v[2]={*position,1.0f-(*(position+1))};
                    memcpy(vboTexCoord,v,sizeof(float)*attInfo->size);
                    vboTexCoord+=(stride>>2);
                }
                else
                {
					int xyzOffset=j*sizeof(float)*attInfo->size;
					float* position=(float*)(byteArray+xyzOffset);
						       
                    memset(vboTexCoord,0,sizeof(float)*2);
                    float v[2]={*position,1.0f-(*(position+1))};
                    memcpy(vboTexCoord,v,sizeof(float)*attInfo->size);
                    vboTexCoord+=(stride>>2);

					xyzOffset=(j-1)*sizeof(float)*attInfo->size;
					position=(float*)(byteArray+xyzOffset);
						       
                    memset(vboTexCoord,0,sizeof(float)*2);
                    {
                    float v[2]={*position,1.0f-(*(position+1))};
                    memcpy(vboTexCoord,v,sizeof(float)*attInfo->size);
                    }
                    vboTexCoord+=(stride>>2);

					xyzOffset=(0)*sizeof(float)*attInfo->size;
					position=(float*)(byteArray+xyzOffset);
						       
                    memset(vboTexCoord,0,sizeof(float)*2);
                    {
                    float v[2]={*position,1.0f-(*(position+1))};
                    memcpy(vboTexCoord,v,sizeof(float)*attInfo->size);
                    }
                    vboTexCoord+=(stride>>2);
                }
            }
       }
   }
   //*********************************************************
   //Triangles fans are not supported in DX11.
   //Fan Triangles are converted to list on the fly.
   //Only support glDraw array for now.
   //*********************************************************
   void OpenGL::RenderAsFan(GLint first,GLsizei count)
   {
	   std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
	   if(itt==shaderPrograms_.end())
	   {
		   return;
	   }
	   int positionOffset=0;
	   int uvOffset=0;
	   int colorOffset=0;
	   UINT stride=0;
	   UINT offset=0;
	   ShaderProgram* currentProgram=&itt->second;
	   for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
	   {
		   if(strcmp(currentProgram->attributes_[i].bufferName,"POSITION")==0)
		   {
			   positionOffset=stride;
			   stride+=12;
		   }
		   else if(strcmp(currentProgram->attributes_[i].bufferName,"COLOR")==0)
		   {
			   colorOffset=stride;
			   stride+=16;
		   }
		   else if(strcmp(currentProgram->attributes_[i].bufferName,"TEXCOORD")==0)
		   {
			   uvOffset=stride;
			   stride+=8;
		   }
       }
       int listcount=((count-3)+1)*3;
       int numSplits = 1;
       int totalByteInStride = listcount*stride;
       int totalFanByteInStride = count*stride;
       if(totalByteInStride>=VBO_SIZE)
       {
           numSplits = numSplits+(totalByteInStride/VBO_SIZE);
       }
       char* vboMemory=NULL;       
       bool hasTexture=false;
       int incCountByte=0;
       int incFanCountByte=0;
       int byteArrayOffset[3]={0,0,0};
       if(currentVertexBuffer_!=0)
       {
			//Grab VBO.
	       std::map<GLuint,BufferInfo>::iterator vboItt = buffers_.find(currentVertexBuffer_);
	       if(vboItt==buffers_.end())
	       {
		       return;
	       }

           D3D11_MAPPED_SUBRESOURCE subResource={};
           BufferInfo* cBuffer = &vboItt->second;
           context_->Map(cBuffer->buffer_,0,D3D11_MAP_READ,0,&subResource);
		   int advanceBy=0;
		   char* subResourcePointerInByte=(char*)subResource.pData;
		   char* subResources[3]={NULL,NULL,NULL};
           for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
           {
			   //Compute VBO internal offsets.
               AttributeInfo* attInfo=&currentProgram->attributes_[i];
			   subResources[i]=subResourcePointerInByte+advanceBy;
			   int ByteCount = (attInfo->type==GL_UNSIGNED_BYTE || attInfo->type==GL_BYTE) ? 1:(attInfo->type==GL_UNSIGNED_SHORT|| attInfo->type==GL_SHORT) ? 2:4;
			   advanceBy=advanceBy+ByteCount*attInfo->size;
		   }
           for(int splitIndex=0;splitIndex<numSplits;splitIndex++)
           {
               incCountByte = min(totalByteInStride,VBO_SIZE);
               incFanCountByte = min(totalFanByteInStride,VBO_SIZE);
               internalVbos_->Map(context_,incCountByte,(void**)&vboMemory);
               char* vboPositionMemory=(vboMemory+positionOffset);
               char* vboColorPositionMemory=(vboMemory+colorOffset);
               char* vboTexCoordPositionMemory=(vboMemory+uvOffset);
               int vCount = incCountByte/stride;
               int fCount = incFanCountByte/stride;
               for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
               {
                   AttributeInfo* attInfo=&currentProgram->attributes_[i];
                   char* byteArray=(char*)subResources[i]+byteArrayOffset[i];
                   if(strcmp(attInfo->bufferName,"POSITION")==0)
                   {
                       glUpdatePositionFan(first,fCount,attInfo,byteArray,(float*)vboPositionMemory,stride);
                   }
                   else if(strcmp(attInfo->bufferName,"COLOR")==0)
                   {
                       glUpdateColorFan(first,fCount,attInfo,byteArray,(float*)vboColorPositionMemory,stride);
                   }
                   else if(strcmp(attInfo->bufferName,"TEXCOORD")==0)
                   {
                       glUpdateTexCoordFan(first,fCount,attInfo,byteArray,(float*)vboTexCoordPositionMemory,stride);
                       hasTexture = true;
                   }
                   byteArrayOffset[i] = byteArrayOffset[i]+(fCount*attInfo->stride);
               }
               internalVbos_->Unmap(context_);
               DrawVerticesWithProgram(GL_TRIANGLES,first,currentProgram,internalVbos_[0].previousLocation,vCount,stride);
               totalByteInStride-=VBO_SIZE;
           }
           context_->Unmap(cBuffer->buffer_,0);
       }
       else
       {           
           for(int splitIndex=0;splitIndex<numSplits;splitIndex++)
           {
               incCountByte = min(totalByteInStride,VBO_SIZE);
               incFanCountByte = min(totalFanByteInStride,VBO_SIZE);
               internalVbos_->Map(context_,incCountByte,(void**)&vboMemory);
               char* vboPositionMemory=(vboMemory+positionOffset);
               char* vboColorPositionMemory=(vboMemory+colorOffset);
               char* vboTexCoordPositionMemory=(vboMemory+uvOffset);
               int vCount = incCountByte/stride;
               int fCount = incFanCountByte/stride;
               for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
               {
                   AttributeInfo* attInfo=&currentProgram->attributes_[i];
                   char* byteArray=(char*)attInfo->data+byteArrayOffset[i];
                   if(strcmp(attInfo->bufferName,"POSITION")==0)
                   {
                       glUpdatePositionFan(first,fCount,attInfo,byteArray,(float*)vboPositionMemory,stride);
                   }
                   else if(strcmp(attInfo->bufferName,"COLOR")==0)
                   {
                       glUpdateColorFan(first,fCount,attInfo,byteArray,(float*)vboColorPositionMemory,stride);
                   }
                   else if(strcmp(attInfo->bufferName,"TEXCOORD")==0)
                   {
                       glUpdateTexCoordFan(first,fCount,attInfo,byteArray,(float*)vboTexCoordPositionMemory,stride);
                       hasTexture = true;
                   }
                   byteArrayOffset[i] = byteArrayOffset[i]+(fCount*attInfo->stride);
               }
               internalVbos_->Unmap(context_);
               DrawVerticesWithProgram(GL_TRIANGLES,first,currentProgram,internalVbos_[0].previousLocation,vCount,stride);
               totalByteInStride-=VBO_SIZE;
           }
       }
   }

   //*********************************************************
   //Update Buffer position.
   //*********************************************************
   void OpenGL::glUpdatePosition(GLenum mode, GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,char* vboPositionMemory,GLuint stride)
   {
       for(int j=0;j<count;++j)
       {
            //Copy all positions.
            if(attInfo->stride!=0)
            {
			    //Interleaved
                float* position=(float*)(byteArray+(j*attInfo->stride));
                float* vboPosition=(float*)(vboPositionMemory+(j*stride));
			    memset(vboPosition,0,sizeof(float)*3);
                memcpy(vboPosition,position,sizeof(float)*attInfo->size);

            }
            else
            {
			    //Non interleaved.
                int xyzOffset=j*sizeof(float)*attInfo->size;
                float* position=(float*)(byteArray+xyzOffset);
                float* vboPosition=(float*)(vboPositionMemory+(j*stride));
                memset(vboPosition,0,sizeof(float)*3);
                memcpy(vboPosition,position,sizeof(float)*attInfo->size);
            }
       }
       if(mode==GL_LINE_LOOP)
       {
            //Copy all positions.
            if(attInfo->stride!=0)
            {
			    //Interleaved
                float* position=(float*)(byteArray+(0*attInfo->stride));
                float* vboPosition=(float*)(vboPositionMemory+(count*stride));
			    memset(vboPosition,0,sizeof(float)*3);
                memcpy(vboPosition,position,sizeof(float)*attInfo->size);

            }
            else
            {
			    //Non interleaved.
                int xyzOffset=0*sizeof(float)*attInfo->size;
                float* position=(float*)(byteArray+xyzOffset);
                float* vboPosition=(float*)(vboPositionMemory+(count*stride));
                memset(vboPosition,0,sizeof(float)*3);
                memcpy(vboPosition,position,sizeof(float)*attInfo->size);
            }
       }
   }

   //*********************************************************
   //Update Color position.
   //*********************************************************
   void OpenGL::glUpdateColor(GLenum mode, GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,char* vboColorPositionMemory,GLuint stride)
   {
       for(int j=0;j<count;++j)
       {
           if(attInfo->type==GL_UNSIGNED_BYTE)
           {
               //Convert from unsigned byte to float.
                if(attInfo->stride!=0)
                {
                    unsigned int* color = (unsigned int*)(byteArray+(j*attInfo->stride));
                    float a = (float)((*color&0xff000000)>>24);
                    float b = (float)((*color&0x00ff0000)>>16);
                    float g = (float)((*color&0x0000ff00)>>8);
                    float r = (float)((*color&0x000000ff));
                    r/=255.0f;g/=255.0f;b/=255.0f;a/=255.0f;
                    float rgba[4]={r,g,b,a};
                    float* vboColorPosition=(float*)(vboColorPositionMemory+(j*stride));
				    memset(vboColorPosition,0,sizeof(float)*4);
                    memcpy(vboColorPosition,rgba,sizeof(float)*attInfo->size);
                }
                else
                {
                    int xyzOffset=j*sizeof(unsigned char)*attInfo->size;
                    unsigned int* color = (unsigned int*)(byteArray+xyzOffset);
                    float a = (float)((*color&0xff000000)>>24);
                    float b = (float)((*color&0x00ff0000)>>16);
                    float g = (float)((*color&0x0000ff00)>>8);
                    float r = (float)((*color&0x000000ff));
                    r/=255.0f;g/=255.0f;b/=255.0f;a/=255.0f;
                    float rgba[4]={r,g,b,a};
                    float* vboColorPosition=(float*)(vboColorPositionMemory+(j*stride));
                    memset(vboColorPosition,0,sizeof(float)*4);
                    memcpy(vboColorPosition,rgba,sizeof(float)*attInfo->size);
                }
           }
           else if(attInfo->type==GL_FLOAT)
           {
                //Copy the color as is.
                if(attInfo->stride!=0)
                {
                    float* position=(float*)(byteArray+(j*attInfo->stride));
                    float* vboColorPosition=(float*)(vboColorPositionMemory+(j*stride));
				    memset(vboColorPosition,0,sizeof(float)*4);
                    memcpy(vboColorPosition,position,sizeof(float)*attInfo->size);
                }
                else
                {
                    int xyzOffset=j*sizeof(float)*attInfo->size;
                    float* position=(float*)(byteArray+xyzOffset);
                    float* vboColorPosition=(float*)(vboColorPositionMemory+(j*stride));
                    memset(vboColorPosition,0,sizeof(float)*4);
                    memcpy(vboColorPosition,position,sizeof(float)*attInfo->size);
                }
           }
       }
       if(mode == GL_LINE_LOOP)
       {
           if(attInfo->type==GL_UNSIGNED_BYTE)
           {
               //Convert from unsigned byte to float.
                if(attInfo->stride!=0)
                {
                    unsigned int* color = (unsigned int*)(byteArray+(0*attInfo->stride));
                    float a = (float)((*color&0xff000000)>>24);
                    float b = (float)((*color&0x00ff0000)>>16);
                    float g = (float)((*color&0x0000ff00)>>8);
                    float r = (float)((*color&0x000000ff));
                    r/=255.0f;g/=255.0f;b/=255.0f;a/=255.0f;
                    float rgba[4]={r,g,b,a};
                    float* vboColorPosition=(float*)(vboColorPositionMemory+(count*stride));
				    memset(vboColorPosition,0,sizeof(float)*4);
                    memcpy(vboColorPosition,rgba,sizeof(float)*attInfo->size);
                }
                else
                {
                    int xyzOffset=0*sizeof(unsigned char)*attInfo->size;
                    unsigned int* color = (unsigned int*)(byteArray+xyzOffset);
                    float a = (float)((*color&0xff000000)>>24);
                    float b = (float)((*color&0x00ff0000)>>16);
                    float g = (float)((*color&0x0000ff00)>>8);
                    float r = (float)((*color&0x000000ff));
                    r/=255.0f;g/=255.0f;b/=255.0f;a/=255.0f;
                    float rgba[4]={r,g,b,a};
                    float* vboColorPosition=(float*)(vboColorPositionMemory+(count*stride));
                    memset(vboColorPosition,0,sizeof(float)*4);
                    memcpy(vboColorPosition,rgba,sizeof(float)*attInfo->size);
                }
           }
           else if(attInfo->type==GL_FLOAT)
           {
                //Copy the color as is.
                if(attInfo->stride!=0)
                {
                    float* position=(float*)(byteArray+(0*attInfo->stride));
                    float* vboColorPosition=(float*)(vboColorPositionMemory+(count*stride));
				    memset(vboColorPosition,0,sizeof(float)*4);
                    memcpy(vboColorPosition,position,sizeof(float)*attInfo->size);
                }
                else
                {
                    int xyzOffset=0*sizeof(float)*attInfo->size;
                    float* position=(float*)(byteArray+xyzOffset);
                    float* vboColorPosition=(float*)(vboColorPositionMemory+(count*stride));
                    memset(vboColorPosition,0,sizeof(float)*4);
                    memcpy(vboColorPosition,position,sizeof(float)*attInfo->size);
                }
           }
       }
   }

   //*********************************************************
   //Update TexCoord position.
   //*********************************************************
   void OpenGL::glUpdateTexCoord(GLenum mode, GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,char* vboTexCoordPositionMemory,GLuint stride)
   {
       for(int j=0;j<count;++j)
       {
           //Copy texture coordinate.
           if(attInfo->stride!=0)
           {
               float* position=(float*)(byteArray+(j*attInfo->stride));
               float* texCoordPosition=(float*)(vboTexCoordPositionMemory+(j*stride));
               memset(texCoordPosition,0,sizeof(float)*2);
               float v[2]={*position,1.0f-(*(position+1))};
               memcpy(texCoordPosition,v,sizeof(float)*attInfo->size);
           }
           else
           {
               int xyzOffset=j*sizeof(float)*attInfo->size;
               float* position=(float*)(byteArray+xyzOffset);
               float* texCoordPosition=(float*)(vboTexCoordPositionMemory+(j*stride));
               memset(texCoordPosition,0,sizeof(float)*2);
               float v[2]={*position,1.0f-(*(position+1))};
               memcpy(texCoordPosition,v,sizeof(float)*attInfo->size);
           }
       }
       if(mode == GL_LINE_LOOP)
       {
           //Copy texture coordinate.
           if(attInfo->stride!=0)
           {
               float* position=(float*)(byteArray+(0*attInfo->stride));
               float* texCoordPosition=(float*)(vboTexCoordPositionMemory+(count*stride));
               memset(texCoordPosition,0,sizeof(float)*2);
               float v[2]={*position,1.0f-(*(position+1))};
               memcpy(texCoordPosition,v,sizeof(float)*attInfo->size);
           }
           else
           {
               int xyzOffset=0*sizeof(float)*attInfo->size;
               float* position=(float*)(byteArray+xyzOffset);
               float* texCoordPosition=(float*)(vboTexCoordPositionMemory+(count*stride));
               memset(texCoordPosition,0,sizeof(float)*2);
               float v[2]={*position,1.0f-(*(position+1))};
               memcpy(texCoordPosition,v,sizeof(float)*attInfo->size);
           }
       }
   }

   //*********************************************************
   //Draw the array buffer.
   //*********************************************************
   void OpenGL::glDrawArrays(GLenum mode, GLint first, GLsizei count)
   {
       CheckCurrentTarget();

	   if(count==0)
	   {
		   return;
	   }
       
       if(mode==GL_TRIANGLE_FAN)
       {
           RenderAsFan(first,count);
           return;
       }
	   
       std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
	   if(itt==shaderPrograms_.end())
	   {
		   return;
	   }
	   int positionOffset=0;
	   int uvOffset=0;
	   int colorOffset=0;
	   UINT stride=0;
	   UINT offset=0;
	   ShaderProgram* currentProgram=&itt->second;
	   for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
	   {
		   if(strcmp(currentProgram->attributes_[i].bufferName,"POSITION")==0)
		   {
			   positionOffset=stride;
			   stride+=12;
		   }
		   else if(strcmp(currentProgram->attributes_[i].bufferName,"COLOR")==0)
		   {
			   colorOffset=stride;
			   stride+=16;
		   }
		   else if(strcmp(currentProgram->attributes_[i].bufferName,"TEXCOORD")==0)
		   {
			   uvOffset=stride;
			   stride+=8;
		   }
       }

       int numSplits = 1;
       int totalByteInStride = count*stride;
       int totalByteLoopStride = (count+((mode==GL_LINE_LOOP) ? 1:0))*stride;
       if(totalByteInStride>=VBO_SIZE)
       {
           numSplits = numSplits+(totalByteInStride/VBO_SIZE);
       }
       
       char* vboMemory=NULL;       
       bool hasTexture=false;
       int incCountByte=0;
       int incLoopByte = 0;
       int byteArrayOffset[3] = {0,0,0};
       if(currentVertexBuffer_!=0)
       {
			//Grab VBO.
	       std::map<GLuint,BufferInfo>::iterator vboItt = buffers_.find(currentVertexBuffer_);
	       if(vboItt==buffers_.end())
	       {
		       return;
	       }

           D3D11_MAPPED_SUBRESOURCE subResource={};
           BufferInfo* cBuffer = &vboItt->second;
           context_->Map(cBuffer->buffer_,0,D3D11_MAP_READ,0,&subResource);
		   int advanceBy=0;
		   char* subResourcePointerInByte=(char*)subResource.pData;
		   char* subResources[3]={NULL,NULL,NULL};
           for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
           {
			   //Compute VBO internal offsets.
               AttributeInfo* attInfo=&currentProgram->attributes_[i];
			   subResources[i]=subResourcePointerInByte+advanceBy;
			   int ByteCount = (attInfo->type==GL_UNSIGNED_BYTE || attInfo->type==GL_BYTE) ? 1:(attInfo->type==GL_UNSIGNED_SHORT|| attInfo->type==GL_SHORT) ? 2:4;
			   advanceBy=advanceBy+ByteCount*attInfo->size;
		   }

		   //Copy all the vertices to our own internal buffer.
           for(int splitIndex=0;splitIndex<numSplits;splitIndex++)
           {
               incCountByte = min(totalByteInStride,VBO_SIZE);
               incLoopByte = min(totalByteLoopStride,VBO_SIZE);
               internalVbos_->Map(context_,incLoopByte,(void**)&vboMemory);
               char* vboPositionMemory=(vboMemory+positionOffset);
               char* vboColorPositionMemory=(vboMemory+colorOffset);
               char* vboTexCoordPositionMemory=(vboMemory+uvOffset);
               int loopCount = incLoopByte/stride;
               int vCount = incCountByte/stride;
               for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
               {
                   AttributeInfo* attInfo=&currentProgram->attributes_[i];
                   char* byteArray=(char*)subResources[i]+byteArrayOffset[i];
                   if(strcmp(attInfo->bufferName,"POSITION")==0)
                   {
                       glUpdatePosition(mode,first,vCount,attInfo,byteArray,vboPositionMemory,stride);
                   }
                   else if(strcmp(attInfo->bufferName,"COLOR")==0)
                   {
                       glUpdateColor(mode,first,vCount,attInfo,byteArray,vboColorPositionMemory,stride);
                   }
                   else if(strcmp(attInfo->bufferName,"TEXCOORD")==0)
                   {
                       glUpdateTexCoord(mode,first,vCount,attInfo,byteArray,vboTexCoordPositionMemory,stride);
                       hasTexture = true;
                   }
                   byteArrayOffset[i] = byteArrayOffset[i]+(vCount*attInfo->stride);
               }
               internalVbos_->Unmap(context_);
               DrawVerticesWithProgram(mode,first,currentProgram,internalVbos_[0].previousLocation,loopCount,stride);
               totalByteInStride-=VBO_SIZE;
           }
		   context_->Unmap(cBuffer->buffer_,0);
       }
       else
       {
           for(int splitIndex=0;splitIndex<numSplits;splitIndex++)
           {
               incCountByte = min(totalByteInStride,VBO_SIZE);
               incLoopByte = min(totalByteLoopStride,VBO_SIZE);
               internalVbos_->Map(context_,incLoopByte,(void**)&vboMemory);
               char* vboPositionMemory=(vboMemory+positionOffset);
               char* vboColorPositionMemory=(vboMemory+colorOffset);
               char* vboTexCoordPositionMemory=(vboMemory+uvOffset);
               int loopCount = incLoopByte/stride;
               int vCount = incCountByte/stride;
               for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
               {
                   AttributeInfo* attInfo=&currentProgram->attributes_[i];
                   char* byteArray=(char*)attInfo->data+byteArrayOffset[i];
                   if(strcmp(attInfo->bufferName,"POSITION")==0)
                   {
                       glUpdatePosition(mode,first,vCount,attInfo,byteArray,vboPositionMemory,stride);
                   }
                   else if(strcmp(attInfo->bufferName,"COLOR")==0)
                   {
                       glUpdateColor(mode,first,vCount,attInfo,byteArray,vboColorPositionMemory,stride);
                   }
                   else if(strcmp(attInfo->bufferName,"TEXCOORD")==0)
                   {
                       glUpdateTexCoord(mode,first,vCount,attInfo,byteArray,vboTexCoordPositionMemory,stride);
                       hasTexture = true;
                   }
                   byteArrayOffset[i] = byteArrayOffset[i]+(vCount*attInfo->stride);
               }
               internalVbos_->Unmap(context_);
               DrawVerticesWithProgram(mode,first,currentProgram,internalVbos_[0].previousLocation,loopCount,stride);
               totalByteInStride-=VBO_SIZE;
           }
       }
   }

   //*********************************************************
   //Update Buffer position.
   //*********************************************************
   bool OpenGL::glUpdatePositionIndices(GLenum mode, GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,char* vboPositionMemory,short* vboIndices,GLuint stride,GLuint& failCount)
   {
       for(int j=0;j<count;++j)
       {
           int vIndex=vboIndices[j];
           if(vIndex<0)
           {
               failCount = j;
               return false;
           }
           //Copy all positions.
           if(attInfo->stride!=0)
           {
               float* position=(float*)(byteArray+(vIndex*attInfo->stride));
               float* vboPosition=(float*)(vboPositionMemory+(j*stride));
               memset(vboPosition,0,sizeof(float)*3);
               memcpy(vboPosition,position,sizeof(float)*attInfo->size);
           }
           else
           {
               int xyzOffset=vIndex*sizeof(float)*attInfo->size;
               float* position=(float*)(byteArray+xyzOffset);
               float* vboPosition=(float*)(vboPositionMemory+(j*stride));
               memset(vboPosition,0,sizeof(float)*3);
               memcpy(vboPosition,position,sizeof(float)*attInfo->size);
           }
       }
       return true;
   }

   //*********************************************************
   //Update Color position.
   //*********************************************************
   void OpenGL::glUpdateColorIndices(GLenum mode, GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,char* vboColorPositionMemory,short* iboIndices,GLuint stride)
   {
       for(int j=0;j<count;++j)
       {
           int vIndex=iboIndices[j];
           if(vIndex<0)
           {
               return;
           }
           if(attInfo->type==GL_UNSIGNED_BYTE)
           {
               //Convert from unsigned byte to float.
               if(attInfo->stride!=0)
               {
                   unsigned int* color = (unsigned int*)(byteArray+(vIndex*attInfo->stride));
                   float a = (float)((*color&0xff000000)>>24);
                   float b = (float)((*color&0x00ff0000)>>16);
                   float g = (float)((*color&0x0000ff00)>>8);
                   float r = (float)((*color&0x000000ff));
                   r/=255.0f;g/=255.0f;b/=255.0f;a/=255.0f;
                   float rgba[4]={r,g,b,a};
                   float* vboColorPosition=(float*)(vboColorPositionMemory+(j*stride));
                   memset(vboColorPosition,0,sizeof(float)*4);
                   memcpy(vboColorPosition,rgba,sizeof(float)*attInfo->size);
				}
				else
				{
					int xyzOffset=vIndex*sizeof(float)*attInfo->size;
					unsigned int* color = (unsigned int*)(byteArray+xyzOffset);
					float a = (float)((*color&0xff000000)>>24);
					float b = (float)((*color&0x00ff0000)>>16);
					float g = (float)((*color&0x0000ff00)>>8);
					float r = (float)((*color&0x000000ff));
					r/=255.0f;g/=255.0f;b/=255.0f;a/=255.0f;
					float rgba[4]={r,g,b,a};
					float* vboColorPosition=(float*)(vboColorPositionMemory+(j*stride));
					memset(vboColorPosition,0,sizeof(float)*4);
					memcpy(vboColorPosition,rgba,sizeof(float)*attInfo->size);
				}
            }
            else if(attInfo->type==GL_FLOAT)
            {
                //Copy the color as is.
				if(attInfo->stride!=0)
				{
					float* position=(float*)(byteArray+(vIndex*attInfo->stride));
					float* vboColorPosition=(float*)(vboColorPositionMemory+(j*stride));
					memset(vboColorPosition,0,sizeof(float)*4);
					memcpy(vboColorPosition,position,sizeof(float)*attInfo->size);
				}
				else
				{
					int xyzOffset=vIndex*sizeof(float)*attInfo->size;
					float* position=(float*)(byteArray+xyzOffset);
					float* vboColorPosition=(float*)(vboColorPositionMemory+(j*stride));
					memset(vboColorPosition,0,sizeof(float)*4);
					memcpy(vboColorPosition,position,sizeof(float)*attInfo->size);
				}
            }
       }
   }

   //*********************************************************
   //Update TexCoord position.
   //*********************************************************
   void OpenGL::glUpdateTexCoordIndices(GLenum mode, GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,char* vboTexCoordPositionMemory,short* iboIndices,GLuint stride)
   {
       for(int j=0;j<count;++j)
       {
           int vIndex=iboIndices[j];
           if(vIndex<0)
           {
               return;
           }
           if(attInfo->stride!=0)
           {
               float* position=(float*)(byteArray+(vIndex*attInfo->stride));
               float* texCoordPosition=(float*)(vboTexCoordPositionMemory+(j*stride));
               memset(texCoordPosition,0,sizeof(float)*2);
               float v[2]={*position,(1.0f-*(position+1))};
               memcpy(texCoordPosition,v,sizeof(float)*attInfo->size);
           }
           else
           {
               int xyzOffset=vIndex*sizeof(float)*attInfo->size;
               float* position=(float*)(byteArray+xyzOffset);
               float* texCoordPosition=(float*)(vboTexCoordPositionMemory+(j*stride));
               memset(texCoordPosition,0,sizeof(float)*2);
               float v[2]={*position,1.0f-(*(position+1))};
               memcpy(texCoordPosition,v,sizeof(float)*attInfo->size);
           }
       }
   }
   //*********************************************************
   //Draw the array_element
   //*********************************************************
   void OpenGL::glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
   {
       CheckCurrentTarget();
	   if(count==0)
	   {
		   return;
	   }

	   std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
	   if(itt==shaderPrograms_.end())
	   {
		   return;
	   }
	   int positionOffset=0;
	   int uvOffset=0;
	   int colorOffset=0;
	   UINT stride=0;
	   UINT offset=0;
	   ShaderProgram* currentProgram=&itt->second;
	   for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
	   {
		   if(strcmp(currentProgram->attributes_[i].bufferName,"POSITION")==0)
		   {
			   positionOffset=stride;
			   stride+=12;
		   }
		   else if(strcmp(currentProgram->attributes_[i].bufferName,"COLOR")==0)
		   {
			   colorOffset=stride;
			   stride+=16;
		   }
		   else if(strcmp(currentProgram->attributes_[i].bufferName,"TEXCOORD")==0)
		   {
			   uvOffset=stride;
			   stride+=8;
		   }
		}
       int numSplits = 1;
       int totalByteInStride = count*stride;
       if(totalByteInStride>=VBO_SIZE)
       {
           int q = VBO_SIZE;
           int s=(totalByteInStride/q);
           numSplits = numSplits+s;
       }
       char* vboMemory=NULL;
       bool hasTexture=false;
       int incCountByte=0;
       int byteArrayOffset[3] = {0,0,0};
       if(currentVertexBuffer_!=0 && currentIndexBuffer_!=0)
       {
			std::map<GLuint,BufferInfo>::iterator vboItt = buffers_.find(currentVertexBuffer_);
			if(vboItt==buffers_.end())
			{
				return;
			}
			std::map<GLuint,BufferInfo>::iterator iboItt = buffers_.find(currentIndexBuffer_);
			if(vboItt==buffers_.end())
			{
		       return;
			}
			D3D11_MAPPED_SUBRESOURCE subResource={};
			D3D11_MAPPED_SUBRESOURCE iboResource={};
			BufferInfo* cBuffer = &vboItt->second;
			BufferInfo* ibBuffer = &iboItt->second;
			context_->Map(cBuffer->buffer_,0,D3D11_MAP_READ,0,&subResource);
			context_->Map(ibBuffer->buffer_,0,D3D11_MAP_READ,0,&iboResource);
			
			char* subResourcePointerInByte=(char*)subResource.pData;
			int advanceBy=0;
			char* subResources[3]={NULL,NULL,NULL};
			for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
			{
			   //Compute VBO internal offsets.
               AttributeInfo* attInfo=&currentProgram->attributes_[i];
			   subResources[i]=subResourcePointerInByte+advanceBy;
			   int ByteCount = (attInfo->type==GL_UNSIGNED_BYTE || attInfo->type==GL_BYTE) ? 1:(attInfo->type==GL_UNSIGNED_SHORT|| attInfo->type==GL_SHORT) ? 2:4;
			   advanceBy=advanceBy+ByteCount*attInfo->size;
			}
            int start=0;
            for(int splitIndex=0;splitIndex<numSplits;splitIndex++)
            {
                incCountByte = min(totalByteInStride,VBO_SIZE);
                internalVbos_->Map(context_,incCountByte,(void**)&vboMemory);
                char* vboPositionMemory=(vboMemory+positionOffset);
                char* vboColorPositionMemory=(vboMemory+colorOffset);
                char* vboTexCoordPositionMemory=(vboMemory+uvOffset);
                int vCount = incCountByte/stride;
                short* iboIndices = (short*)iboResource.pData;
                iboIndices+=start;
                GLuint failCount=0;
                bool pass = true;
                for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
                {
                    AttributeInfo* attInfo=&currentProgram->attributes_[i];
                    char* byteArray=(char*)subResources[i];
                    if(strcmp(attInfo->bufferName,"POSITION")==0)
                    {
                        pass = glUpdatePositionIndices(mode,0,vCount,attInfo,byteArray,vboPositionMemory,iboIndices,stride,failCount);
                    }
                    else if(strcmp(attInfo->bufferName,"COLOR")==0)
                    {
                        glUpdateColorIndices(mode,0,vCount,attInfo,byteArray,vboColorPositionMemory,iboIndices,stride);
                    }
                    else if(strcmp(attInfo->bufferName,"TEXCOORD")==0)
                    {
                        glUpdateTexCoordIndices(mode,0,vCount,attInfo,byteArray,vboTexCoordPositionMemory,iboIndices,stride);
                        hasTexture = true;
                    }
                }
                internalVbos_->Unmap(context_);
                if(pass)
                {
                    DrawVerticesWithProgram(mode,0,currentProgram,internalVbos_[0].previousLocation,vCount,stride);
                }
                else
                {
                    DrawVerticesWithProgram(mode,0,currentProgram,internalVbos_[0].previousLocation,failCount-1,stride);
                }
                totalByteInStride-=VBO_SIZE;
                start+=vCount;
            }
            context_->Unmap(cBuffer->buffer_,0);
            context_->Unmap(ibBuffer->buffer_,0);
       }
       else
       {
           int start=0;
           for(int splitIndex=0;splitIndex<numSplits;splitIndex++)
           {
               incCountByte = min(totalByteInStride,VBO_SIZE);
               internalVbos_->Map(context_,incCountByte,(void**)&vboMemory);
               char* vboPositionMemory=(vboMemory+positionOffset);
               char* vboColorPositionMemory=(vboMemory+colorOffset);
               char* vboTexCoordPositionMemory=(vboMemory+uvOffset);
               int vCount = incCountByte/stride;
               short* pIndices=(short*)indices;
               pIndices+=start;
               GLuint failCount=0;
               bool pass = true;
               for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
               {
                   AttributeInfo* attInfo=&currentProgram->attributes_[i];
                   char* byteArray=(char*)attInfo->data;
                   if(strcmp(attInfo->bufferName,"POSITION")==0)
                   {
                       pass = glUpdatePositionIndices(mode,0,vCount,attInfo,byteArray,vboPositionMemory,pIndices,stride,failCount);
                   }
                   else if(strcmp(attInfo->bufferName,"COLOR")==0)
                   {
                       glUpdateColorIndices(mode,0,vCount,attInfo,byteArray,vboColorPositionMemory,pIndices,stride);
                   }
                   else if(strcmp(attInfo->bufferName,"TEXCOORD")==0)
                   {
                       glUpdateTexCoordIndices(mode,0,vCount,attInfo,byteArray,vboTexCoordPositionMemory,pIndices,stride);
                       hasTexture = true;
                   }
               }
               internalVbos_->Unmap(context_);
                if(pass)
                {
                    DrawVerticesWithProgram(mode,0,currentProgram,internalVbos_[0].previousLocation,vCount,stride);
                }
                else
                {
                    DrawVerticesWithProgram(mode,0,currentProgram,internalVbos_[0].previousLocation,failCount-1,stride);
                }               totalByteInStride-=VBO_SIZE;
               start+=vCount;
           }
       }
   }
   //**************************************************************************
   //Mask an specific color.
   //**************************************************************************
   void OpenGL::glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
   {	   
       blendDesc_.RenderTarget[0].RenderTargetWriteMask=(alpha<<3)|(blue<<2)|(green<<1)|red;
       SAFE_RELEASE(blendState_);
       device_->CreateBlendState(&blendDesc_,&blendState_);
   }
   //*********************************************************
   //Enable an open gl state.
   //*********************************************************
   void OpenGL::glEnable(GLenum cap)
   {
      if (cap == GL_TEXTURE_2D)
      {
          states_.enableTexture2D_= true;
      }
      else if (cap == GL_SCISSOR_TEST)
      {  
          states_.enableScissorTest_=true;
      }
      else if(cap==GL_BLEND)
      {
          if(!states_.enableBlend_)
          {
              SAFE_RELEASE(blendState_);
              states_.enableBlend_=true;
              blendDesc_.RenderTarget[0].BlendEnable=states_.enableBlend_;
              device_->CreateBlendState(&blendDesc_,&blendState_);
          }
      }
      else if(cap==GL_DEPTH_TEST)
      {
          if(!depthStencilDesc_.DepthEnable)
          {
              depthStencilDesc_.DepthEnable=true;
              SAFE_RELEASE(depthStencilState_);
              device_->CreateDepthStencilState(&depthStencilDesc_,&depthStencilState_);
          }
      }
      else if (cap==GL_STENCIL_TEST)
      {
          if(!depthStencilDesc_.StencilEnable)
          {
              depthStencilDesc_.StencilEnable=true;
              SAFE_RELEASE(depthStencilState_);
              device_->CreateDepthStencilState(&depthStencilDesc_,&depthStencilState_);
          }
      }
      else if (cap==GL_ALPHA_TEST)
      {
      }
   }
   //*********************************************************
   //Disable an open gl setting.
   //*********************************************************
   void OpenGL::glDisable(GLenum cap)
   {
      if (cap == GL_TEXTURE_2D)
      {
          states_.enableTexture2D_ = false;
      }
      else if (cap == GL_SCISSOR_TEST)
      {
          states_.enableScissorTest_=false;
      }
      else if (cap ==GL_BLEND)
      {
          if(states_.enableBlend_)
          {
              states_.enableBlend_=false;
              blendDesc_.RenderTarget[0].BlendEnable=states_.enableBlend_;
              SAFE_RELEASE(blendState_);
              device_->CreateBlendState(&blendDesc_,&blendState_);
          }
      }
      else if (cap==GL_DEPTH_TEST)
      {
          if(depthStencilDesc_.DepthEnable)
          {
              depthStencilDesc_.DepthEnable=false;
              SAFE_RELEASE(depthStencilState_);
              device_->CreateDepthStencilState(&depthStencilDesc_,&depthStencilState_);
          }
      }
      else if (cap==GL_STENCIL_TEST)
      {
          if(depthStencilDesc_.StencilEnable)
          {
              depthStencilDesc_.StencilEnable=false;
              SAFE_RELEASE(depthStencilState_);
              device_->CreateDepthStencilState(&depthStencilDesc_,&depthStencilState_);
          }
      }
   }
   //*********************************************************
   //Generate an texture id.
   //*********************************************************
   void OpenGL::glGenTextures(GLsizei n, GLuint *textures)
   {
      static GLuint textureId = 1;
      for (GLsizei i = 0; i < n; i++)
      {
          if(textureIndices_.indices_.empty())
          {
              textures[i] = textureId++;
          }
          else
          {
              textures[i]=textureIndices_.indices_.top();
              textureIndices_.indices_.pop();
          }
          Texture2DInfo texture;
          device_->CreateSamplerState(&texture.samplerDesc_,&texture.samplerState_);
          textures_[textures[i]]=texture;
      }
   }

   //*********************************************************
   //Delete n textures.
   //*********************************************************
   void OpenGL::glDeleteTextures (GLsizei n, const GLuint *textures)
   {
	   for(int i=0;i<n;++i)
	   {
		   std::map<GLuint,Texture2DInfo>::iterator itt = textures_.find(textures[i]);
		   if(itt==textures_.end())
		   {
			   continue;
		   }
		   Texture2DInfo* currentTexture=&itt->second;
           SAFE_RELEASE(currentTexture->texture2D_);
		   SAFE_RELEASE(currentTexture->depthStencilView_);
		   SAFE_RELEASE(currentTexture->renderTargetView_);
           SAFE_RELEASE(currentTexture->samplerState_);
		   SAFE_RELEASE(currentTexture->texture_);
           textures_.erase(textures[i]);
           textureIndices_.indices_.push(textures[i]);
	   }
   }

   //*********************************************************
   //Create a 2d texture image.
   //*********************************************************
   void OpenGL::glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, 
                             GLint border, GLenum format, GLenum type, const GLvoid *pixels)
   {
       std::map<GLuint,Texture2DInfo>::iterator itt = textures_.find(currentActiveTexture_);
       if(itt==textures_.end())
       {
           return;
       }
       Texture2DInfo* currentTexture=&itt->second;
       SAFE_RELEASE(currentTexture->depthStencilView_);
       SAFE_RELEASE(currentTexture->renderTargetView_);
       SAFE_RELEASE(currentTexture->texture_);
       unsigned char* pixelBuffer = NULL;
       unsigned char* dstPixelBuffer = (unsigned char*)pixels;
       int srcIndex=0;
       int dstIndex=0;
       pixelBuffer =new unsigned char[width*height*4];
       unsigned int* pixelPtr=(unsigned int*)pixelBuffer;

       if(internalformat==GL_RGB)
       {           
           if(type==GL_UNSIGNED_SHORT_5_6_5)
           {
               for(int y=1;y<=height;y++)
               {
                   int linesize = width*2;
                   unsigned char* pData = dstPixelBuffer+(y-1)*linesize;
                   for(int x=0;x<width;x++)
                   {
                       int nx = x*2;
                       int index = (height-y)*width+x;
                       unsigned short* aPixel=(unsigned short*)&pData[nx];
                       int r = ((*aPixel & 0xF800)>>8)|((*aPixel&0xF800)>>13);
                       int g = ((*aPixel & 0x07E0)>>3)|((*aPixel&0x07E0)>>9);
                       int b = ((*aPixel & 0x001F)<<3)|((*aPixel&0x001F)>>2);
                       pixelPtr[index]=(0xff<<24)|(b<<16)|(g<<8)|r;
                   }
               }
           }
           else if(type==GL_UNSIGNED_BYTE)
           {
               for(int y=1;y<=height;y++)
               {
                   int linesize = width*3;
                   unsigned char* pData = dstPixelBuffer+(y-1)*linesize;
                   for(int x=0;x<width;x++)
                   {
                       int index = (height-y)*width+x;
                       int nx = x*3;
                       unsigned char r = pData[nx];
                       unsigned char g = pData[nx+1];
                       unsigned char b = pData[nx+2];
                       pixelPtr[index]=(0xff<<24)|(b<<16)|(g<<8)|r;
                   }
               }
           }
       }
       else if(internalformat==GL_LUMINANCE)
       {
           for(int y=1;y<=height;y++)
           {
               int linesize = width*1;
               unsigned char* pData = dstPixelBuffer+(y-1)*linesize;
               for(int x=0;x<width;x++)
               {
                   int index = (height-y)*width+x;
                   unsigned char luminance = pData[x];
                   pixelPtr[index]=(0xff<<24)|(luminance<<16)|(luminance<<8)|luminance;
               }
           }
       }
       else if(internalformat==GL_LUMINANCE_ALPHA)
       {
           for(int y=1;y<=height;y++)
           {
               int linesize = width*1;
               unsigned short* pData =reinterpret_cast<unsigned short*>(static_cast<unsigned char*>(dstPixelBuffer)+(y-1)*linesize);
               for(int x=0;x<width;x++)
               {
                   int index = (height-y)*width+x;
                   unsigned short luminance = pData[2*x+0];
                   unsigned short alpha=pData[2*x+1];
                   pixelPtr[index]=(luminance<<24)|(luminance<<16)|(luminance<<8)|luminance;
               }
           }
       }
       else if(internalformat==GL_ALPHA)
       {
           for(int y=1;y<=height;y++)
           {
               int linesize = width*1;
               unsigned char* pData = dstPixelBuffer+(y-1)*linesize;
               for(int x=0;x<width;x++)
               {
                   int index = (height-y)*width+x;
                   int nx = x;
                   unsigned char alpha = pData[nx];
                   pixelPtr[index]=(alpha<<24)|(0<<16)|(0<<8)|0;
               }
           }
       }
       else if(internalformat==GL_RGBA)
       {
           if(type==GL_UNSIGNED_SHORT_4_4_4_4)
           {
               for(int y=1;y<=height;y++)
               {
                   int linesize = width*2;
                   unsigned char* pData = dstPixelBuffer+(y-1)*linesize;
                   for(int x=0;x<width;x++)
                   {
                       int index = (height-y)*width+x;
                       int nx = x*2;
                       unsigned short* aPixel=(unsigned short*)&pData[nx];
                       int r = ((*aPixel&0xf000)>>8)|((*aPixel&0xf000)>>12);
                       int g = ((*aPixel&0x0f00)>>4)|((*aPixel&0x0f00)>>8);
                       int b = ((*aPixel&0x00f0)<<0)|((*aPixel&0x00f0)>>4);
                       int a = ((*aPixel&0x000f)<<4)|((*aPixel&0x000f)>>0);
                       pixelPtr[index]=(a<<24)|(b<<16)|(g<<8)|r;
                   }
               }
           }
           else if(type==GL_UNSIGNED_SHORT_5_5_5_1)
           {
               for(int y=1;y<=height;y++)
               {
                   int linesize = width*2;
                   unsigned char* pData = dstPixelBuffer+(y-1)*linesize;
                   for(int x=0;x<width;x++)
                   {
                       int index = (height-y)*width+x;
                       int nx = x*2;
                       unsigned short* aPixel=(unsigned short*)&pData[nx];
                       int b = ((*aPixel&0x003E)<<2)|((*aPixel&0x003E)>>3);
                       int g = ((*aPixel&0x07C0)>>3)|((*aPixel&0x07C0)>>8);
                       int r = ((*aPixel&0xF800)>>8)|((*aPixel&0xF800)>>13);
                       int a = (*aPixel&0x0001) ? 0xff:0;
                       pixelPtr[index]=(a<<24)|(b<<16)|(g<<8)|r;
                   }
               }
           }
           else if(type==GL_UNSIGNED_BYTE)
           {
               for(int y=1;y<=height;y++)
               {
                   int linesize = width*4;
                   unsigned char* pData = dstPixelBuffer+(y-1)*linesize;
                   for(int x=0;x<width;x++)
                   {
                       int index = (height-y)*width+x;
                       int nx = x*4;
                       unsigned char r = pData[nx];
                       unsigned char g = pData[nx+1];
                       unsigned char b = pData[nx+2];
                       unsigned char a = pData[nx+3];
                       pixelPtr[index]=(a<<24)|(b<<16)|(g<<8)|r;
                   }
               }
           }
       }
       else
       {
           int x=0;
       }
       HRESULT hr=S_OK;
       D3D11_TEXTURE2D_DESC desc={};
       desc.ArraySize=1;
       desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
       desc.CPUAccessFlags=0;
       desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
       desc.Height=height;
       desc.MipLevels=1;
       desc.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;
       desc.SampleDesc.Count=1;
       desc.Usage=D3D11_USAGE_DEFAULT;
       desc.Width=width;

       ID3D11Texture2D* texture2D;
       D3D11_SUBRESOURCE_DATA initialData;
       if(pixels)
       {
           ZeroMemory(&initialData, sizeof(initialData));
           initialData.pSysMem = pixelBuffer;
           initialData.SysMemPitch = width * 4;
           hr = device_->CreateTexture2D(&desc, &initialData, &texture2D);
       }
       else
       {
           hr= device_->CreateTexture2D(&desc,NULL,&texture2D);
       }
       if (FAILED(hr))
       {
            throw new GL2DXException("glTexImage2D failed at CreateTexture2D.");
       }
       
       ID3D11ShaderResourceView* textureView;
       hr = device_->CreateShaderResourceView( texture2D, NULL, &textureView);
       if ( FAILED(hr) )
       {
           throw new GL2DXException("glTexImage2D failed at CreateShaderResourceView.");
       }
       currentTexture->type_=target;
       currentTexture->texture2D_=texture2D;
       currentTexture->texture_=textureView;
       SAFE_DELETE_ARRAY(pixelBuffer);
   }

   //*********************************************************
   //Set the scissor box.
   //*********************************************************
   void OpenGL::glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
   {
       CheckCurrentTarget();
       scissorRect_.left = x;
       scissorRect_.right = x + width;
       scissorRect_.top = y;
       scissorRect_.bottom = y+ height;
       D3D11_RECT rects[1];
       rects[0] = scissorRect_;
       context_->RSSetScissorRects(1, rects);
   }


   //*********************************************************
   //Update the scissor state.
   //*********************************************************
   void OpenGL::UpdateScissorState(bool enabled)
   {
       if(states_.enableScissorTest_)
       {
           SAFE_RELEASE(rasterizerState);
           rasterizerDesc_.ScissorEnable = states_.enableScissorTest_;
           device_->CreateRasterizerState(&rasterizerDesc_, &rasterizerState);
           context_->RSSetState(rasterizerState);
       }
   }

   //****************************************************
   //Generate OpenGL buffers.
   //****************************************************
   void OpenGL::glGenBuffers(GLsizei n,GLuint* buffers)
   {
	   static GLuint bufferId=1;
	   for(GLsizei i=0;i<n;++i)
	   {
           if(vboIndices_.indices_.size()==0)
           {
               buffers[i]=bufferId++;
           }
           else
           {
               buffers[i]=vboIndices_.indices_.top();
               vboIndices_.indices_.pop();
           }
	   }
   }

   //****************************************************
   //Generate OpenGL Render buffers.
   //****************************************************
   void OpenGL::glGenRenderbuffers(GLsizei n,GLuint* buffers)
   {
	   static GLuint bufferId=1;
	   for(GLsizei i=0;i<n;++i)
	   {
           if(rboIndices_.indices_.size()==0)
           {
               buffers[i]=bufferId++;
           }
           else
           {
               buffers[i]=rboIndices_.indices_.top();
               rboIndices_.indices_.pop();
           }
	   }
   }
   //****************************************************
   //Delete render buffers.
   //****************************************************
   void OpenGL::glDeleteRenderbuffers(GLsizei count,const GLuint* buffers)
   {
	   for(GLsizei i=0;i<count;++i)
	   {
		   std::map<GLuint,RenderBufferInfo>::iterator itt = rbos_.find(buffers[i]);
		   if(itt==rbos_.end())
		   {
			   continue;
		   }
           RenderBufferInfo* rBufferInfo=&itt->second;
           ID3D11DepthStencilView* targetView=rBufferInfo->depthStencilView_;
           SAFE_RELEASE(targetView);
		   rbos_.erase(itt);
           rboIndices_.indices_.push(buffers[i]);
	   }
   }

   //****************************************************
   //Bind a vertex or index buffer.
   //****************************************************
   void OpenGL::glBindBuffer(GLenum target,const GLuint buffer)
   {
	   if(target==GL_ARRAY_BUFFER)
	   {
		   currentVertexBuffer_=buffer;
	   }
	   else if(target==GL_ELEMENT_ARRAY_BUFFER)
	   {
		   currentIndexBuffer_=buffer;
	   }
   }

   //****************************************************
   //Generate OpenGL Frame buffers.
   //****************************************************
   void OpenGL::glGenFramebuffers(GLsizei n,GLuint* buffers)
   {
	   static GLuint bufferId=1;
	   for(GLsizei i=0;i<n;++i)
	   {
           if(fboIndices_.indices_.empty())
           {
               buffers[i]=bufferId++;
           }
           else
           {
               buffers[i]=fboIndices_.indices_.top();
               fboIndices_.indices_.pop();
           }
           FboAttachement attachement;
           fboAttachements_[buffers[i]]=attachement;
	   }
   }

   //****************************************************
   //Delete opengl frame buffers.
   //****************************************************
   void OpenGL::glDeleteFramebuffers(GLsizei count,const GLuint* buffers)
   {
	   for(GLsizei i=0;i<count;++i)
	   {
		   std::map<GLuint,FboAttachement>::iterator itt = fboAttachements_.find(buffers[i]);
		   if(itt==fboAttachements_.end())
		   {
			   continue;
		   }
		   fboAttachements_.erase(itt);
           fboIndices_.indices_.push(buffers[i]);
	   }
   }
   //****************************************************
   //Bind the frame buffer.
   //****************************************************
   void OpenGL::glBindFramebuffer(GLenum target,const GLuint buffer)
   {
	   currentFrameBuffer_=buffer;
   }

   //****************************************************
   //Bind the render buffer.
   //****************************************************
   void OpenGL::glBindRenderbuffer(GLenum target,const GLuint buffer)
   {
	   if(target==GL_RENDERBUFFER)
	   {
		   currentRenderBuffer_=buffer;
	   }
   }

   //****************************************************
   //Create buffer and allocate memory for it.
   //****************************************************
   void OpenGL::glBufferData(GLenum target,GLsizei size,const GLvoid* pData,GLenum streamType)
   {
	   if(target==GL_ARRAY_BUFFER)
	   {
           std::map<GLuint,BufferInfo>::iterator itt = buffers_.find(currentVertexBuffer_);
		   if(itt!=buffers_.end())
		   {
			    BufferInfo* bInfo=&itt->second;
                if(bInfo && bInfo->buffer_ != nullptr)
				{
					bInfo->buffer_->Release();
				}
				buffers_.erase(itt);
		   }
		   ID3D11Buffer* buffer=NULL;
		   D3D11_BUFFER_DESC bufferDesc={};
		   D3D11_SUBRESOURCE_DATA subData={};
		   BufferInfo bufferInfo;
		   subData.pSysMem=pData;
		   bufferDesc.BindFlags=0;
		   bufferDesc.ByteWidth=size;
           bufferDesc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE|D3D11_CPU_ACCESS_READ;
           bufferDesc.Usage=D3D11_USAGE_STAGING;
		   if(pData)
		   {
			   device_->CreateBuffer(&bufferDesc,&subData,&buffer);
		   }
		   else
		   {
			   device_->CreateBuffer(&bufferDesc,0,&buffer);
		   }
		   bufferInfo.type_=target;
		   bufferInfo.buffer_=buffer;
		   buffers_[currentVertexBuffer_]=bufferInfo;
	   }
	   else if(target==GL_ELEMENT_ARRAY_BUFFER)
	   {
		   std::map<GLuint,BufferInfo>::iterator itt = buffers_.find(currentIndexBuffer_);
		   if(itt!=buffers_.end())
		   {
			   BufferInfo* bInfo=&itt->second;
			   if(bInfo->buffer_)
				{
					bInfo->buffer_->Release();
			   }
			   buffers_.erase(itt);
		   }
		   ID3D11Buffer* buffer=NULL;
		   D3D11_BUFFER_DESC bufferDesc={};
		   D3D11_SUBRESOURCE_DATA subData={};
		   BufferInfo bufferInfo;
		   subData.pSysMem=pData;
		   bufferDesc.BindFlags=0;
		   bufferDesc.ByteWidth=size;
           bufferDesc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE|D3D11_CPU_ACCESS_READ;
		   bufferDesc.Usage=D3D11_USAGE_STAGING;
		   if(pData)
		   {
			   device_->CreateBuffer(&bufferDesc,&subData,&buffer);
		   }
		   else
		   {
			   device_->CreateBuffer(&bufferDesc,0,&buffer);
		   }
		   bufferInfo.type_=target;
		   bufferInfo.buffer_=buffer;
		   buffers_[currentIndexBuffer_]=bufferInfo;
	   }
   }

   //****************************************************
   //Update an section of the active buffer.
   //****************************************************
   void OpenGL::glBufferSubData(GLenum target,GLsizei offset,GLsizei size,const GLvoid* data)
   {
	   if(target==GL_ARRAY_BUFFER)
	   {
		   std::map<GLuint,BufferInfo>::iterator itt = buffers_.find(currentVertexBuffer_);
		   if(itt==buffers_.end())
		   {
			  return;
		   }
		   BufferInfo* bInfo = &buffers_.find(currentVertexBuffer_)->second;
		   ID3D11Buffer* buffer = bInfo->buffer_;
		   D3D11_MAPPED_SUBRESOURCE mapResource={};
           context_->Map(buffer,0,D3D11_MAP_WRITE,0,&mapResource);
		   char* rPointer = (char*)mapResource.pData;
		   memcpy((rPointer+offset),data,size);
		   context_->Unmap(buffer,0);
	   }
	   else if(target==GL_ELEMENT_ARRAY_BUFFER)
	   {
		   std::map<GLuint,BufferInfo>::iterator itt = buffers_.find(currentIndexBuffer_);
		   if(itt==buffers_.end())
		   {
			  return;
		   }
		   BufferInfo* bInfo = &buffers_.find(currentIndexBuffer_)->second;
		   ID3D11Buffer* buffer = bInfo->buffer_;
		   D3D11_MAPPED_SUBRESOURCE mapResource={};
		   context_->Map(buffer,0,D3D11_MAP_WRITE,0,&mapResource);
		   char* rPointer = (char*)mapResource.pData;
		   memcpy((rPointer+offset),data,size);
		   context_->Unmap(buffer,0);
	   }
   }

   //*******************************************************
   //Check the current target that we are going to draw to.
   //*******************************************************
   void OpenGL::CheckCurrentTarget()
   {
       if(currentFrameBuffer_!=0)
       {
           //if(currentFrameBuffer_!=oldFrameBuffer_)
           {
               std::map<GLuint,FboAttachement>::iterator fboItt = fboAttachements_.find(currentFrameBuffer_);
               if(fboItt!=fboAttachements_.end())
               {               
                   FboAttachement* attachement=&fboItt->second;
                   ID3D11DepthStencilView* depthStencilView=NULL;
                   ID3D11RenderTargetView* d3d11RenderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
                   UINT totalRenderTargets=0;
                   for(int i=0;i<D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;++i)
                   {
                       if(attachement->colorAttachement[i]!=0)
                       {
                           std::map<GLuint,Texture2DInfo>::iterator textureItt = textures_.find(attachement->colorAttachement[i]);
                           if(textureItt!=textures_.end())
                           {
                               Texture2DInfo* TextureTarget=&textureItt->second;
                               if(TextureTarget->renderTargetView_==NULL)
                               {
                                   ID3D11RenderTargetView* renderTargetView=NULL;
                                   D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc={};
                                   renderTargetViewDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
                                   renderTargetViewDesc.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
                                   device_->CreateRenderTargetView(TextureTarget->texture2D_,&renderTargetViewDesc,&renderTargetView);
                                   TextureTarget->renderTargetView_=renderTargetView;
                               }
                               d3d11RenderTargets[i]=TextureTarget->renderTargetView_;
                               totalRenderTargets++;
                           }
                       }
                       else
                       {
                           break;
                       }
                   }
                   if(totalRenderTargets==0)
                   {
                       return;
                   }
                   //Figure the depth texture which is part of the fbo.
                   if(attachement->depthAttachement!=0)
                   {
                       if(attachement->depthAttachementType==GL_RENDERBUFFER)
                       {                   
                           std::map<GLuint,RenderBufferInfo>::iterator rboItt= rbos_.find(attachement->depthAttachement);
                           if(rboItt!=rbos_.end())
                           {
                               depthStencilView=rboItt->second.depthStencilView_;
                           }
                       }
                       else if(attachement->depthAttachement==GL_TEXTURE_2D)
                       {
                           std::map<GLuint,Texture2DInfo>::iterator textureItt=textures_.find(attachement->depthAttachement);
                           if(textureItt!=textures_.end())
                           {
                               Texture2DInfo* TextureTarget=&textureItt->second;
                               depthStencilView=TextureTarget->depthStencilView_;
                           }
                       }
                   }
                   //Now we set our new render target/depth stencil.
                   context_->OMSetRenderTargets(totalRenderTargets,d3d11RenderTargets,depthStencilView);
                   oldFrameBuffer_=currentFrameBuffer_;               
               }
           }
       }
       else
       {
           //Restore the default back buffer/depth stencil.
           if(oldFrameBuffer_!=currentFrameBuffer_)
           {
               context_->OMSetRenderTargets(1,&backBufferRenderTargetView_,backBufferDepthStencilView_);
               oldFrameBuffer_=currentFrameBuffer_;
           }
       }
   }
   //***********************************************************
   //Flush.
   //***********************************************************
   void OpenGL::glFlush()
   {
	   context_->Flush();
   }
   //****************************************************
   //Set the clear color.
   //****************************************************
   void OpenGL::glClearColor(GLfloat r,GLfloat g,GLfloat b,GLfloat a)
   {
	   clearColor_[0]=r;
	   clearColor_[1]=g;
	   clearColor_[2]=b;
	   clearColor_[3]=a;
   }

   //****************************************************
   //Set the clear depth value.
   //****************************************************
   void OpenGL::glClearDepth(GLfloat depth)
   {
	   depthValue_=depth;
   }

   //****************************************************
   //Set stencil clear value.
   //****************************************************
   void OpenGL::glClearStencil(GLubyte value)
   {
	   stencilValue_=value;
   }

   //****************************************************
   //Clear the screen/active fbo.
   //****************************************************
   void OpenGL::glClear(GLsizei clearBits)
   {
       CheckCurrentTarget();
	   ID3D11RenderTargetView* targetView=NULL;
	   ID3D11DepthStencilView* depthStencilView=NULL;
	   UINT ClearFlags=0;
	   context_->OMGetRenderTargets(1,&targetView,&depthStencilView);   //Clear the active target/depth stencil view.
	   if(clearBits & GL_COLOR_BUFFER_BIT)
	   {
           if(targetView)
           {
               context_->ClearRenderTargetView(targetView,clearColor_);
           }
	   }
	   if(clearBits & GL_DEPTH_BUFFER_BIT)
	   {
		   ClearFlags|=D3D11_CLEAR_DEPTH;
	   }
	   if(clearBits & GL_STENCIL_BUFFER_BIT)
	   {
		   ClearFlags|=D3D11_CLEAR_STENCIL;
	   }
       if(ClearFlags!=0)
       {
           if(depthStencilView)
           {
               context_->ClearDepthStencilView(depthStencilView,ClearFlags,depthValue_,stencilValue_);
           }
       }
       SAFE_RELEASE(targetView);
       SAFE_RELEASE(depthStencilView);
   }

   //****************************************************
   //Delete vbo/ibo
   //****************************************************
   void OpenGL::glDeleteBuffers(GLsizei count,const GLuint* buffers)
   {
	   for(GLsizei i=0;i<count;++i)
	   {
		   std::map<GLuint,BufferInfo>::iterator itt = buffers_.find(buffers[i]);
		   if(itt==buffers_.end())
		   {
			   continue;
		   }
		   ID3D11Buffer* buffer=itt->second.buffer_;
           SAFE_RELEASE(buffer);
		   buffers_.erase(itt);
           vboIndices_.indices_.push(buffers[i]);
	   }   
   }

   //****************************************************
   //Set the depth func.
   //****************************************************
   void OpenGL::glDepthFunc(GLenum depthFunc)
   {
        switch(depthFunc)
        {
        case GL_NEVER:
            depthStencilDesc_.DepthFunc=D3D11_COMPARISON_NEVER;
            break;
        case GL_LESS:
            depthStencilDesc_.DepthFunc=D3D11_COMPARISON_LESS;
            break;
        case GL_LEQUAL:
            depthStencilDesc_.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
            break;
        case GL_GREATER:
            depthStencilDesc_.DepthFunc=D3D11_COMPARISON_GREATER;
            break;
        case GL_GEQUAL:
            depthStencilDesc_.DepthFunc=D3D11_COMPARISON_GREATER_EQUAL;
            break;
        case GL_EQUAL:
            depthStencilDesc_.DepthFunc=D3D11_COMPARISON_EQUAL;
            break;
        case GL_NOTEQUAL:
            depthStencilDesc_.DepthFunc=D3D11_COMPARISON_NOT_EQUAL;
            break;
        case GL_ALWAYS:
            depthStencilDesc_.DepthFunc=D3D11_COMPARISON_ALWAYS;
            break;
        }
        SAFE_RELEASE(depthStencilState_);
        device_->CreateDepthStencilState(&depthStencilDesc_,&depthStencilState_);
   }

   //****************************************************
   //Set the depth Mask.
   //****************************************************
   void OpenGL::glDepthMask(GLboolean mask)
   {
       if(mask)
       {
           depthStencilDesc_.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;
       }
       else
       {
           depthStencilDesc_.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;
       }
       SAFE_RELEASE(depthStencilState_);
       device_->CreateDepthStencilState(&depthStencilDesc_,&depthStencilState_);
   }

   //****************************************************
   //Set the vertex attrib we want to turn on.
   //****************************************************
   void OpenGL::glEnableVertexAttribArray(GLuint index)
   {
       if(index>=0 && index<=MAX_VERTEX_ATTRIBUTES-1)
       {
           vertexAttribArray[index]=1;
       }
   }

	//***********************************************
	//attach the frame buffer to an render buffer.
	//***********************************************
	void OpenGL::glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
	{
		if(currentFrameBuffer_==0)
		{
			return;
		}
		std::map<GLuint,FboAttachement>::iterator itt = fboAttachements_.find(currentFrameBuffer_);
		if(itt==fboAttachements_.end())
		{
			FboAttachement fboAtt;
			memset(&fboAtt,0,sizeof(fboAtt));
			if(attachment>=GL_COLOR_ATTACHMENT0 && attachment<=GL_COLOR_ATTACHMENT15)
			{
				int attachIndex=GL_COLOR_ATTACHMENT15-attachment;
				fboAtt.colorAttachement[attachIndex]=renderbuffer;
				fboAtt.colorAttachementType[attachIndex]=renderbuffertarget;
			}
			else if(attachment==GL_DEPTH_ATTACHMENT)
			{
				fboAtt.depthAttachement=renderbuffer;
                fboAtt.depthAttachementType=renderbuffertarget;

			}
			else if(attachment==GL_STENCIL_ATTACHMENT)
			{
				fboAtt.stencilAttachement=renderbuffer;
			}
			fboAttachements_[currentFrameBuffer_]=fboAtt;
		}
		else
		{
			FboAttachement* fboAtt=&itt->second;
			if(attachment>=GL_COLOR_ATTACHMENT0 && attachment<=GL_COLOR_ATTACHMENT15)
			{
				int attachIndex=GL_COLOR_ATTACHMENT15-attachment;
				fboAtt->colorAttachement[attachIndex]=renderbuffer;
			}
			else if(attachment==GL_DEPTH_ATTACHMENT)
			{
				fboAtt->depthAttachement=renderbuffer;
                fboAtt->depthAttachementType=renderbuffertarget;
			}
			else if(attachment==GL_STENCIL_ATTACHMENT)
			{
				fboAtt->stencilAttachement=renderbuffer;
			}
		}
	}

	//****************************************************
	//Attach an texture to an frame buffer object.
	//****************************************************
	void OpenGL::glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
	{
		if(currentFrameBuffer_==0)
		{
			return;
		}
		std::map<GLuint,FboAttachement>::iterator itt = fboAttachements_.find(currentFrameBuffer_);
		if(itt==fboAttachements_.end())
		{
			FboAttachement fboAtt;
			memset(&fboAtt,0,sizeof(fboAtt));
			if(attachment>=GL_COLOR_ATTACHMENT0 && attachment<=GL_COLOR_ATTACHMENT15)
			{
				int attachIndex=GL_COLOR_ATTACHMENT15-attachment;
				fboAtt.colorAttachement[attachIndex]=texture;
				fboAtt.colorAttachementType[attachIndex]=textarget;
			}
			else if(attachment==GL_DEPTH_ATTACHMENT)
			{
				fboAtt.depthAttachement=texture;
                fboAtt.depthAttachementType=textarget;
			}
			else if(attachment==GL_STENCIL_ATTACHMENT)
			{
				fboAtt.stencilAttachement=texture;
			}
			fboAttachements_[currentFrameBuffer_]=fboAtt;
		}
		else
		{
			FboAttachement* fboAtt=&itt->second;
			if(attachment>=GL_COLOR_ATTACHMENT0 && attachment<=GL_COLOR_ATTACHMENT15)
			{
				int attachIndex=15-(GL_COLOR_ATTACHMENT15-attachment);
				fboAtt->colorAttachement[attachIndex]=texture;
				fboAtt->colorAttachementType[attachIndex]=textarget;
			}
			else if(attachment==GL_DEPTH_ATTACHMENT)
			{
				fboAtt->depthAttachement=texture;
                fboAtt->depthAttachementType=textarget;
			}
			else if(attachment==GL_STENCIL_ATTACHMENT)
			{
				fboAtt->stencilAttachement=texture;
			}
		}
	}

	//*****************************************************
	//Set the target we want to have auto mipmap generated.
	//*****************************************************
	void OpenGL::glGenerateMipmap(GLenum target)
	{
		generateMipmaps_=target;
	}
    //****************************************************
    //Build the dx shader.
    //****************************************************
    void OpenGL::glBuildDxShader(GLuint shader, GLenum type, const char* source,const GLuint size)
    {
        std::map<GLuint,ShaderInfo>::iterator itt = shaders_.find(shader);
        if(itt==shaders_.end())
        {
            return;
        }
        ShaderInfo* currentShader=&itt->second;
        currentShader->codeBuffer=(char*)source;
        currentShader->codeLength=size;
    }

    //****************************************************
    //Create an input layout that matches the shader we 
    //are trying to use.
    //****************************************************
    void OpenGL::CreateInputLayoutFromShader(GLuint program)
    {
        GLuint numElements=0;
        D3D11_INPUT_ELEMENT_DESC* ppElements=NULL;
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(program);
        if(itt==shaderPrograms_.end())
        {
            return;
        }
        int totalElements=0;
        ShaderProgram* currentProgram=&itt->second;
        for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
        {
            if(strcmp(currentProgram->attributes_[i].bufferName,"")!=0)
            {
                ++totalElements;
            }
        }


		ppElements = new D3D11_INPUT_ELEMENT_DESC[totalElements];
		memset(ppElements,0,sizeof(D3D11_INPUT_ELEMENT_DESC)*totalElements);
        int currentElementIndex=0;
        int byteOffset=0;
        for(int i=0;i<MAX_VERTEX_ATTRIBUTES;++i)
        {
            if(strcmp(currentProgram->attributes_[i].bufferName,"")!=0)
            {
                AttributeInfo* cAttInfo=&currentProgram->attributes_[i];
                D3D11_INPUT_ELEMENT_DESC* currentDesc=&ppElements[currentElementIndex++];
                currentDesc->SemanticName=currentProgram->attributes_[i].bufferName;
                currentDesc->AlignedByteOffset=byteOffset;
                if(strcmp(currentProgram->attributes_[i].bufferName,"POSITION")==0)
                {
                    currentDesc->Format=DXGI_FORMAT_R32G32B32_FLOAT;
                    currentDesc->AlignedByteOffset=byteOffset;
                    byteOffset+=12;
                }
                else if(strcmp(currentProgram->attributes_[i].bufferName,"COLOR")==0)
                {
                    currentDesc->Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
                    currentDesc->AlignedByteOffset=byteOffset;
                    byteOffset+=16;
                }
                else if(strcmp(currentProgram->attributes_[i].bufferName,"TEXCOORD")==0)
                {
                    currentDesc->Format=DXGI_FORMAT_R32G32_FLOAT;
                    currentDesc->AlignedByteOffset=byteOffset;
                    byteOffset+=8;
                }
                currentDesc->InputSlot=0;
                currentDesc->InputSlotClass=D3D11_INPUT_PER_VERTEX_DATA;
                currentDesc->InstanceDataStepRate=0;
                currentDesc->SemanticIndex=0;
            }
        }

        std::map<GLuint,ShaderInfo>::iterator shaderInfoItt=shaders_.find(currentProgram->vertexShader_);
        ShaderInfo* sInfo=&shaderInfoItt->second;
        device_->CreateInputLayout(ppElements,totalElements,sInfo->codeBuffer,sInfo->codeLength,&currentProgram->shaderLinkage_.inputLayout_);
		SAFE_DELETE_ARRAY(ppElements);
    }

    //****************************************************
    //Register Uniform location with the shader.
    //****************************************************
    void OpenGL::glRegisterUniformLocation(GLuint programObj,const GLuint location,const char* name)
    {
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(programObj);
        if(itt==shaderPrograms_.end())
        {
            return;
        }
        ShaderVariable variable;
        variable.id=location;
		variable.pointerArrayFloat = nullptr;
        ShaderProgram* currentProgram=&itt->second;
        currentProgram->shaderVariables_[name]=variable;
        currentProgram->shaderVariableByIndex_[location]=variable;
    }

    //****************************************************
    //Register sampler location with the shader.
    //****************************************************
    void OpenGL::glRegisterUniformSamplerLocation(GLuint programObj,const GLuint location,const char* name)
    {
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(programObj);
        if(itt==shaderPrograms_.end())
        {
            return;
        }
        ShaderVariable variable;
        variable.id=location;
        variable.type=SHADER_VARIABLE_SAMPLER;
        ShaderProgram* currentProgram=&itt->second;
        currentProgram->shaderVariables_[name]=variable;
        currentProgram->shaderVariableByIndex_[location]=variable;
    }
	//****************************************************
	//Get the location of a variable in a shader.
	//****************************************************
	GLint OpenGL::glGetUniformLocation(GLuint programObj, const char* name)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(programObj);
        if(itt==shaderPrograms_.end())
        {
            return -1;
        }
        ShaderProgram* currentProgram=&itt->second;
        auto shaderVarItt=currentProgram->shaderVariables_.find(name);
        if(shaderVarItt==currentProgram->shaderVariables_.end())
        {
            return -1;
        }
        return shaderVarItt->second.id;
	}

	//****************************************************
	//Get an parameter.
	//****************************************************
	void OpenGL::glGetBooleanv(GLenum pname, GLboolean *params)
	{
		switch(pname)
		{
		case GL_DEPTH_WRITEMASK:
			*params=(depthStencilDesc_.DepthWriteMask==D3D11_DEPTH_WRITE_MASK_ALL) ? GL_TRUE:GL_FALSE;
			break;
		}
	}
	//****************************************************
	//Check if an specific cap is enabled.
	//****************************************************
	bool OpenGL::glIsEnabled(GLenum cap)
	{
		return true;
	}

	//****************************************************
	//Set how you want pixels to be pack when been read.
	//****************************************************
	void OpenGL::glPixelStorei(GLenum pname, GLint param)
	{
        
	}

	//****************************************************
	//Read a block of pixels from the active buffer.
	//****************************************************
	void OpenGL::glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
	{

	}

	//****************************************************
	//Set a vertex attrib pointer.
	//****************************************************
	void OpenGL::glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
        if(itt==shaderPrograms_.end())
        {
            return;
        }
        //Store the data which maps to this attribute.
        ShaderProgram* currentProgram=&itt->second;
        AttributeInfo* current=&currentProgram->attributes_[index];
        current->data=(GLvoid*)pointer;
        current->size=size;
        current->stride=stride;
        current->type=type;

	}

	//****************************************************
	//Allocate memory for a render buffer.
	//****************************************************
	void OpenGL::glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
	{
		ID3D11Texture2D* texture=NULL;
		ID3D11DepthStencilView* depthStencilView=NULL;
        D3D11_TEXTURE2D_DESC textureDesc={};
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc={};
		textureDesc.ArraySize=1;
		textureDesc.BindFlags=D3D11_BIND_DEPTH_STENCIL;
		textureDesc.CPUAccessFlags=0;
		textureDesc.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;
		textureDesc.Height=height;
		textureDesc.MipLevels=1;
		textureDesc.MiscFlags=0;
		textureDesc.SampleDesc.Count=1;
		textureDesc.SampleDesc.Quality=0;
		textureDesc.Usage=D3D11_USAGE_DEFAULT;
		textureDesc.Width=width;
		depthStencilViewDesc.Format=textureDesc.Format;
		depthStencilViewDesc.ViewDimension=D3D11_DSV_DIMENSION_TEXTURE2D;
		device_->CreateTexture2D(&textureDesc,NULL,&texture);
		device_->CreateDepthStencilView(texture,&depthStencilViewDesc,&depthStencilView);
		texture->Release();
        RenderBufferInfo rBufferInfo;
        rBufferInfo.depthStencilView_=depthStencilView;
        rbos_[currentRenderBuffer_]=rBufferInfo;
	}

	//****************************************************
	//Get shader info.
	//****************************************************
	void OpenGL::glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog)
	{
		std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(program);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
		ShaderProgram* sProgram=&itt->second;
		
	}
	//****************************************************
	//Validate an shader.
	//****************************************************
	void OpenGL::glGetProgramiv(GLuint program, GLenum pname, GLint* param)
	{
		if(pname==GL_DELETE_STATUS)
		{
		}
		else if(pname==GL_LINK_STATUS)
		{
		}
		else if(pname==GL_VALIDATE_STATUS)
		{
		}
		else if(pname==GL_INFO_LOG_LENGTH)
		{
		}
		else if(pname==GL_ATTACHED_SHADERS)
		{
		}
		else if(pname==GL_ACTIVE_ATTRIBUTES)
		{
		}
		else if(GL_ACTIVE_ATTRIBUTE_MAX_LENGTH)
		{
		}
		else if(GL_ACTIVE_UNIFORMS)
		{
		}
		else if(GL_ACTIVE_UNIFORM_MAX_LENGTH)
		{

		}
	}

	//******************************************************************
	//Get a log info about an specific shader.
	//******************************************************************
	void OpenGL::glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog)
	{

	}

	//******************************************************************
	//Switch the active texture unit.
	//******************************************************************
	void OpenGL::glActiveTexture(GLenum texture)
	{
		currentTextureUnit_=31-(GL_TEXTURE31-texture);
	}

	//******************************************************************
	//Attach an shader to the program.
	//******************************************************************
	void OpenGL::glAttachShader(GLuint program, GLuint shader)
	{
		std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(program);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
		std::map<GLuint,ShaderInfo>::iterator shaderItt = shaders_.find(shader);
		if(shaderItt==shaders_.end())
		{
			return;
		}
		ShaderProgram* sProgram=&itt->second;
		ShaderInfo* sInfo=&shaderItt->second;
		if(sInfo->type==GL_VERTEX_SHADER)
		{
			sProgram->vertexShader_=shader;
		}
		else if(sInfo->type==GL_FRAGMENT_SHADER)
		{
			sProgram->pixelShader_=shader;
		}
	}

	//******************************************************************
    //Bind an Input Layout index to a specific index.
	//******************************************************************
	void OpenGL::glBindAttribLocation(GLuint program, GLuint index, const char* name)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(program);
        if(itt==shaderPrograms_.end())
        {
            return;
        }
        ShaderProgram* linkage=&itt->second;
        strcpy_s(linkage->attributes_[index].bufferName,64,name);
	}

	//******************************************************************
	//Blend equation.
	//******************************************************************
	void OpenGL::glBlendEquation(GLenum mode)
	{
        switch(mode)
        {
        case GL_FUNC_ADD:
            blendDesc_.RenderTarget[0].BlendOp=D3D11_BLEND_OP_ADD;
            break;
        case GL_FUNC_SUBTRACT:
            blendDesc_.RenderTarget[0].BlendOp=D3D11_BLEND_OP_SUBTRACT;
            break;
        case GL_FUNC_REVERSE_SUBTRACT:
            blendDesc_.RenderTarget[0].BlendOp=D3D11_BLEND_OP_REV_SUBTRACT;
            break;
        case GL_MIN:
            blendDesc_.RenderTarget[0].BlendOp=D3D11_BLEND_OP_MIN;
            break;
        case GL_MAX:
            blendDesc_.RenderTarget[0].BlendOp=D3D11_BLEND_OP_MAX;
            break;
        }
        SAFE_RELEASE(blendState_);
        device_->CreateBlendState(&blendDesc_,&blendState_);
	}

	//******************************************************************
	//Blend states.
	//******************************************************************
	void OpenGL::glBlendFunc(GLenum sfactor, GLenum dfactor)
	{
		D3D11_BLEND srcBlend;
		D3D11_BLEND dstBlend;
		switch(sfactor)
		{
		case GL_ZERO:
			srcBlend=D3D11_BLEND_ZERO;
			break;
		case GL_ONE:
			srcBlend=D3D11_BLEND_ONE;
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			srcBlend=D3D11_BLEND_INV_SRC_COLOR;
			break;
		case GL_DST_COLOR:
			srcBlend=D3D11_BLEND_DEST_COLOR;
			break;
		case GL_ONE_MINUS_DST_COLOR:
			srcBlend=D3D11_BLEND_INV_DEST_COLOR;
			break;
		case GL_SRC_ALPHA:
			srcBlend=D3D11_BLEND_SRC_ALPHA;
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			srcBlend=D3D11_BLEND_INV_SRC_ALPHA;
			break;
		case GL_DST_ALPHA:
			srcBlend=D3D11_BLEND_DEST_ALPHA;
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			srcBlend=D3D11_BLEND_INV_DEST_ALPHA;
			break;
		case GL_SRC_ALPHA_SATURATE:
			srcBlend=D3D11_BLEND_SRC_ALPHA_SAT;
			break;
		case GL_SRC1_ALPHA:
			srcBlend=D3D11_BLEND_SRC1_ALPHA;
			break;
		}
		switch(dfactor)
		{
		case GL_ZERO:
			dstBlend=D3D11_BLEND_ZERO;
			break;
		case GL_ONE:
			dstBlend=D3D11_BLEND_ONE;
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			dstBlend=D3D11_BLEND_INV_SRC_COLOR;
			break;
		case GL_DST_COLOR:
			dstBlend=D3D11_BLEND_DEST_COLOR;
			break;
		case GL_ONE_MINUS_DST_COLOR:
			dstBlend=D3D11_BLEND_INV_DEST_COLOR;
			break;
		case GL_SRC_ALPHA:
			dstBlend=D3D11_BLEND_SRC_ALPHA;
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			dstBlend=D3D11_BLEND_INV_SRC_ALPHA;
			break;
		case GL_DST_ALPHA:
			dstBlend=D3D11_BLEND_DEST_ALPHA;
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			dstBlend=D3D11_BLEND_INV_DEST_ALPHA;
			break;
		case GL_SRC_ALPHA_SATURATE:
			dstBlend=D3D11_BLEND_SRC_ALPHA_SAT;
			break;
		case GL_SRC1_ALPHA:
			dstBlend=D3D11_BLEND_SRC1_ALPHA;
			break;
		}
		SAFE_RELEASE(blendState_);
		blendDesc_.RenderTarget[0].SrcBlend=srcBlend;
		blendDesc_.RenderTarget[0].DestBlend=dstBlend;
		device_->CreateBlendState(&blendDesc_,&blendState_);
	}
	//******************************************************************
	//Check the fbo status.
	//******************************************************************
	GLenum OpenGL::glCheckFramebufferStatus(GLenum target)
	{
		std::map<GLuint,FboAttachement>::iterator itt = fboAttachements_.find(currentFrameBuffer_);
		if(itt==fboAttachements_.end())
		{
			return GL_FRAMEBUFFER_UNDEFINED;
		}
		FboAttachement* attachements=&itt->second;
		if(attachements->colorAttachement[0]>0)
		{
			return GL_FRAMEBUFFER_COMPLETE;
		}
		return GL_FRAMEBUFFER_UNDEFINED;
	}

	//**************************************************************************
	//Load compress texture images.
	//**************************************************************************
	void OpenGL::glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
	{

	}

	//**************************************************************************
	//Retur a parameter from a shader object.
	//**************************************************************************
	void OpenGL::glGetShaderiv(GLuint shader, GLenum pname, GLint* param)
	{
        *param=GL_TRUE;
	}

	//**************************************************************************
	//Create a shader object.
	//**************************************************************************
	GLuint OpenGL::glCreateShader(GLenum type)
	{
		static GLsizei index=1;
		ShaderInfo shaderInfo;
		shaderInfo.type=type;
		shaders_[index]=shaderInfo;
		return index++;
	}

	//**************************************************************************
	//Delete an shader program.
	//**************************************************************************
	void OpenGL::glDeleteProgram(GLuint program)
	{
		std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(program);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
		shaderPrograms_.erase(itt);
	}

	//**************************************************************************
	//Delete an shader.
	//**************************************************************************
	void OpenGL::glDeleteShader(GLuint shader)
	{
		std::map<GLuint,ShaderInfo>::iterator itt = shaders_.find(shader);
		if(itt==shaders_.end())
		{
			return;
		}
		shaders_.erase(itt);
	}

	//**************************************************************************
	//Disable a vertex atrrib pointer.
	//**************************************************************************
	void OpenGL::glDisableVertexAttribArray(GLuint index)
	{
		if(index>=0 && index<=15)
		{
			vertexAttribArray[index]=0;
		}
	}

	//**************************************************************************
	//Create an new shader program.
	//**************************************************************************
	GLuint OpenGL::glCreateProgram()
	{
		static GLsizei index=1;
		ShaderProgram sProgram;
		sProgram.pixelShader_=0;
		sProgram.vertexShader_=0;
		shaderPrograms_[index]=sProgram;
		return index++;
	}

	//**************************************************************************
	//Replaces the source code in a shader object.
	//**************************************************************************
	void OpenGL::glShaderSource(GLuint shader, GLsizei count, const char ** string, const GLint *length)
	{
		std::map<GLuint,ShaderInfo>::iterator itt = shaders_.find(shader);
		if(itt==shaders_.end())
		{
			return;
		}
		ShaderInfo* sInfo=&itt->second;
		int codeLength=0;
		for(GLsizei i=0;i<count;++i)
		{
			codeLength+=strlen(string[i]);
		}
		if(sInfo->codeBuffer)
		{
			delete[] sInfo->codeBuffer;
		}
		sInfo->codeLength=codeLength+1;
		sInfo->codeBuffer=new char[sInfo->codeLength];
		int codeStart=0;
		for(GLsizei i=0;i<count;++i)
		{
			int len = strlen(string[i]);
			memcpy(&sInfo->codeBuffer[codeStart],string[i],sizeof(char)*len);
			codeStart+=len;
		}
		sInfo->codeBuffer[codeLength]='\0';
	}
	//************************************************************
	//Compile the loaded shader code.
	//************************************************************
	void OpenGL::glCompileShader(GLuint shader)
	{
        //DX For WINRT does not support the compiling of shaders.
	}

	//**************************************************************************
	//Link all the shader into an vertex/pixel shader.
	//**************************************************************************
	void OpenGL::glLinkProgram(GLuint program)
	{
		std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(program);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
		ID3D11VertexShader* dx11VertexShader=NULL;
		ID3D11PixelShader* dx11PixelShader=NULL;
		ShaderProgram* pShaderProgram=&itt->second;
		std::map<GLuint,ShaderInfo>::iterator VSItt=shaders_.find(pShaderProgram->vertexShader_);
		std::map<GLuint,ShaderInfo>::iterator PSItt=shaders_.find(pShaderProgram->pixelShader_);
		if(VSItt!=shaders_.end())
		{
			ShaderInfo* sInfo=&VSItt->second;
            device_->CreateVertexShader(sInfo->codeBuffer,sInfo->codeLength,NULL,&dx11VertexShader);
		}
		if(PSItt!=shaders_.end())
		{
			ShaderInfo* sInfo=&PSItt->second;
            device_->CreatePixelShader(sInfo->codeBuffer,sInfo->codeLength,NULL,&dx11PixelShader);
		}
        pShaderProgram->shaderLinkage_.vertexShader_=dx11VertexShader;
		pShaderProgram->shaderLinkage_.pixelShader_=dx11PixelShader;
	}

	//**************************************************************************
	//Returns the source code string from a shader object.
	//**************************************************************************
	void OpenGL::glGetShaderSource(GLsizei shader, GLsizei buffSize, GLsizei* length, char *source)
	{
		std::map<GLuint,ShaderInfo>::iterator itt = shaders_.find(shader);
		if(itt==shaders_.end())
		{
			return;
		}
		ShaderInfo* sInfo=&itt->second;
		GLsizei maxLenght = (buffSize>(GLsizei)sInfo->codeLength) ? sInfo->codeLength:buffSize;
		if(*length)
		{
			*length=maxLenght-1;
		}
		memcpy(source,sInfo->codeBuffer,sizeof(char)*maxLenght);
	}

	//**************************************************************************
	//Bind the program to use.
	//**************************************************************************
	void OpenGL::glUseProgram(GLuint program)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(program);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderLinkage* linkage=&itt->second.shaderLinkage_;
        context_->VSSetShader(linkage->vertexShader_,NULL,0);
		context_->PSSetShader(linkage->pixelShader_,NULL,0);
        currentProgram_=program;
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform1f(GLint location, GLfloat v0)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=1;
        if(variable->type!=SHADER_VARIABLE_SAMPLER)
        {
            variable->type=SHADER_VARIABLE_SINGLE;
        }
        variable->single=v0;
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform1i(GLint location, GLint v0)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=1;
        if(variable->type!=SHADER_VARIABLE_SAMPLER)
        {
            variable->type=SHADER_VARIABLE_VALUE;
        }
        variable->value=v0;
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform2f(GLint location, GLfloat v0, GLfloat v1)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=1;
        variable->type=SHADER_VARIABLE_ARRAY2D;
        variable->array2d[0]=v0;
        variable->array2d[1]=v1;
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform2fv(GLint location, GLsizei count, const GLfloat* values)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=count*2;
        variable->type=SHADER_VARIABLE_POINTER_FLOAT;
        variable->pointerArrayFloat=new float[count*2];
        memcpy(variable->pointerArrayFloat,values,sizeof(float)*count*2);
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform2i(GLint location, GLint v0, GLint v1)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=1;
        variable->type=SHADER_VARIABLE_ARRAY2D;
        variable->array2d[0]=(float)v0;
        variable->array2d[1]=(float)v1;

	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform2iv(GLint location, GLsizei count, const GLint* values)
	{
        //TODO. I need to have access to shader reflection.
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=count*2;
        variable->type=SHADER_VARIABLE_POINTER_INT;
        variable->pointerArrayInt=new int[count*2];
        memcpy(variable->pointerArrayInt,values,sizeof(int)*count*2);
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform3f(GLint location, GLfloat v0, GLfloat v1,GLfloat v2)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=1;
        variable->type=SHADER_VARIABLE_ARRAY3D;
        variable->array3d[0]=v0;
        variable->array3d[1]=v1;
        variable->array3d[2]=v2;
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform3fv(GLint location, GLsizei count, const GLfloat* values)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=count*3;
        variable->type=SHADER_VARIABLE_POINTER_FLOAT;
        variable->pointerArrayFloat=new float[count*3];
        memcpy(variable->pointerArrayFloat,values,sizeof(float)*count*3);
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform3i(GLint location, GLint v0, GLint v1,GLint v2)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=1;
        variable->type=SHADER_VARIABLE_ARRAY3D;
        variable->array3d[0]=(float)v0;
        variable->array3d[1]=(float)v1;
        variable->array3d[2]=(float)v2;
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform3iv(GLint location, GLsizei count, const GLint* values)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=count*3;
        variable->type=SHADER_VARIABLE_POINTER_INT;
        variable->pointerArrayInt=new int[count*3];
        memcpy(variable->pointerArrayInt,values,sizeof(int)*count*3);
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform4f(GLint location, GLfloat v0, GLfloat v1,GLfloat v2,GLfloat v3)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=1;
        variable->type=SHADER_VARIABLE_ARRAY4D;
        variable->array4d[0]=v0;
        variable->array4d[1]=v1;
        variable->array4d[2]=v2;
        variable->array4d[3]=v3;
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform4fv(GLint location, GLsizei count, const GLfloat* values)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=count*4;
        variable->type=SHADER_VARIABLE_POINTER_FLOAT;
        variable->pointerArrayFloat=new float[count*4];
        memcpy(variable->pointerArrayFloat,values,sizeof(float)*count*4);
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform4i(GLint location, GLint v0, GLint v1,GLint v2,GLint v3)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=1;
        variable->type=SHADER_VARIABLE_ARRAY4D;
        variable->array4d[0]=(float)v0;
        variable->array4d[1]=(float)v1;
        variable->array4d[2]=(float)v2;
        variable->array4d[3]=(float)v3;
	}

	//**************************************************************************
	//Bind an uniform value.
	//**************************************************************************
	void OpenGL::glUniform4iv(GLint location, GLsizei count, const GLint* values)
	{
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=count*4;
        variable->type=SHADER_VARIABLE_POINTER_FLOAT;
        variable->pointerArrayInt=new int[count*4];
        memcpy(variable->pointerArrayInt,values,sizeof(int)*count*4);
	}

	//**************************************************************************
	//Bind an uniform value. for now it only works with count 1.
	//**************************************************************************
	void OpenGL::glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* values)
	{
        //TODO Transpose not implemented yet.
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(currentProgram_);
		if(itt==shaderPrograms_.end())
		{
			return;
		}
        ShaderProgram* currentProgram=&itt->second;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt=currentProgram->shaderVariableByIndex_.find(location);
        if(shaderVarItt==currentProgram->shaderVariableByIndex_.end())
        {
            return;
        }
        ShaderVariable* variable=&shaderVarItt->second;
        variable->count=count;
        variable->type=SHADER_VARIABLE_MATRIX_POINTER;

		if(variable->pointerArrayFloat == nullptr) 
		{
			variable->pointerArrayFloat = new float[count*16];
		}

        float* outMatrices = variable->pointerArrayFloat;
        if(transpose)
        {
            //Transpose all the matrices and copy the output.
            for(int i=0;i<count;++i)
            {
                const float* ref = &values[i*16];
                float* output=&outMatrices[i*16];
                output[0]=ref[0];
                output[1]=ref[4];
                output[2]=ref[8];
                output[3]=ref[12];
                output[4]=ref[1];
                output[5]=ref[5];
                output[6]=ref[9];
                output[7]=ref[13];
                output[8]=ref[2];
                output[9]=ref[6];
                output[10]=ref[10];
                output[11]=ref[14];
                output[12]=ref[3];
                output[13]=ref[7];
                output[14]=ref[11];
                output[15]=ref[15];
            }
        }
        else
        {
            for(int i=0;i<count;++i)
            {
                const float* ref = &values[i*16];
                float* output=&outMatrices[i*16];
                output[0]=ref[0];
                output[1]=ref[1];
                output[2]=ref[2];
                output[3]=ref[3];
                output[4]=ref[4];
                output[5]=ref[5];
                output[6]=ref[6];
                output[7]=ref[7];
                output[8]=ref[8];
                output[9]=ref[9];
                output[10]=ref[10];
                output[11]=ref[11];
                output[12]=ref[12];
                output[13]=ref[13];
                output[14]=ref[14];
                output[15]=ref[15];
            }
        }
	}

    //**************************************************************************
    //Store all the shaders uniform into an constant buffer.
    //**************************************************************************
    void OpenGL::glBuildUniformConstantBuffer(GLuint program)
    {
        std::map<GLuint,ShaderProgram>::iterator itt = shaderPrograms_.find(program);
        if(itt==shaderPrograms_.end())
        {
            return;
        }
        ShaderProgram* currentProgram=&itt->second;
        GLuint bufferSize=0;
        std::map<GLuint,ShaderVariable>::iterator shaderVarItt = currentProgram->shaderVariableByIndex_.begin();
        for(;shaderVarItt!=currentProgram->shaderVariableByIndex_.end();++shaderVarItt)
        {
            ShaderVariable* sVariable=&shaderVarItt->second;
            if(sVariable->id==SHADER_VARIABLE_NONE)
            {
                continue;
            }
            switch(sVariable->type)
            {
            case SHADER_VARIABLE_SINGLE:
                bufferSize=bufferSize+(sizeof(float));
                break;
            case SHADER_VARIABLE_VALUE:
                bufferSize=bufferSize+(sizeof(int));
                break;
            case SHADER_VARIABLE_POINTER_INT:
                bufferSize=bufferSize+(sizeof(int)*sVariable->count);
                break;
            case SHADER_VARIABLE_POINTER_FLOAT:
                bufferSize=bufferSize+(sizeof(float)*sVariable->count);
                break;
            case SHADER_VARIABLE_ARRAY2D:
                bufferSize=bufferSize+(sizeof(float)*2)*sVariable->count;
                break;
            case SHADER_VARIABLE_ARRAY3D:
                bufferSize=bufferSize+(sizeof(float)*3)*sVariable->count;
                break;
            case SHADER_VARIABLE_ARRAY4D:
                bufferSize=bufferSize+(sizeof(float)*4)*sVariable->count;
                break;
            case SHADER_VARIABLE_MATRIX:
                bufferSize=bufferSize+(sizeof(float)*16);
                break;
            case SHADER_VARIABLE_MATRIX_POINTER:
                bufferSize=bufferSize+(sizeof(float)*16)*sVariable->count;
                break;
            }
        }
        if(bufferSize==0)
        {
            return;
        }
        //Copy all uniforms into the constant buffer.
        char* memoryBuffer = new char[bufferSize];
        char* pBuffer=memoryBuffer;
        shaderVarItt = currentProgram->shaderVariableByIndex_.begin();
        for(;shaderVarItt!=currentProgram->shaderVariableByIndex_.end();++shaderVarItt)
        {
            ShaderVariable* sVariable=&shaderVarItt->second;
            if(sVariable->id==SHADER_VARIABLE_NONE)
            {
                continue;
            }
            switch(sVariable->type)
            {
            case SHADER_VARIABLE_SINGLE:
                memcpy(pBuffer,&sVariable->single,sizeof(float));
                pBuffer=pBuffer+sizeof(float);
                break;
            case SHADER_VARIABLE_VALUE:
                memcpy(pBuffer,&sVariable->value,sizeof(int));
                pBuffer=pBuffer+sizeof(int);
                break;
            case SHADER_VARIABLE_POINTER_INT:
                memcpy(pBuffer,sVariable->pointerArrayInt,sizeof(int)*sVariable->count);
                pBuffer=pBuffer+sizeof(int)*sVariable->count;
                break;
            case SHADER_VARIABLE_POINTER_FLOAT:
                memcpy(pBuffer,sVariable->pointerArrayFloat,sizeof(float)*sVariable->count);
                pBuffer=pBuffer+sizeof(float)*sVariable->count;
                break;
            case SHADER_VARIABLE_ARRAY2D:
                memcpy(pBuffer,sVariable->array2d,sizeof(float)*2*sVariable->count);
                pBuffer=pBuffer+sizeof(float)*2*sVariable->count;
                break;
            case SHADER_VARIABLE_ARRAY3D:
                memcpy(pBuffer,sVariable->array3d,sizeof(float)*3*sVariable->count);
                pBuffer=pBuffer+sizeof(float)*3*sVariable->count;
                break;
            case SHADER_VARIABLE_ARRAY4D:
                memcpy(pBuffer,sVariable->array4d,sizeof(float)*4*sVariable->count);
                pBuffer=pBuffer+sizeof(float)*4*sVariable->count;
                break;
            case SHADER_VARIABLE_MATRIX:
                memcpy(pBuffer,sVariable->matrix,sizeof(float)*16);
                pBuffer=pBuffer+(sizeof(float)*16);
                break;
            case SHADER_VARIABLE_MATRIX_POINTER:
                {
                    float ss[16];
                    memcpy(ss,sVariable->pointerArrayFloat,sizeof(float)*16*sVariable->count);
                    memcpy(pBuffer,sVariable->pointerArrayFloat,sizeof(float)*16*sVariable->count);
                    pBuffer=pBuffer+(sizeof(float)*16*sVariable->count);
                }
                break;
            }
        }
       
        //Buffer must be aligned to 16byte boundaries.
        if((bufferSize & 15)>0)
        {
            bufferSize=(bufferSize-(bufferSize&15))+16;
        }
        ID3D11Buffer* buffer=NULL;
        D3D11_BUFFER_DESC bufferDesc={};
        D3D11_SUBRESOURCE_DATA subResource={};
        bufferDesc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.ByteWidth=bufferSize;
        bufferDesc.Usage=D3D11_USAGE_DEFAULT;
        subResource.pSysMem=memoryBuffer;
        device_->CreateBuffer(&bufferDesc,&subResource,&buffer);
        SAFE_RELEASE(currentProgram->shaderLinkage_.uniformBuffer);
        currentProgram->shaderLinkage_.uniformBuffer=buffer;
		SAFE_DELETE_ARRAY(memoryBuffer);

    }
	//**************************************************************************
	//Set the stencil func.
	//**************************************************************************
	void OpenGL::glStencilFunc (GLenum func, GLint ref, GLuint mask)
	{
        switch(func)
        {
        case GL_NEVER:
            depthStencilDesc_.FrontFace.StencilFunc=D3D11_COMPARISON_NEVER;
            depthStencilDesc_.BackFace.StencilFunc=D3D11_COMPARISON_NEVER;
            break;
        case GL_LESS:
            depthStencilDesc_.FrontFace.StencilFunc=D3D11_COMPARISON_LESS;
            depthStencilDesc_.BackFace.StencilFunc=D3D11_COMPARISON_LESS;
            break;
        case GL_LEQUAL:
            depthStencilDesc_.FrontFace.StencilFunc=D3D11_COMPARISON_LESS_EQUAL;
            depthStencilDesc_.BackFace.StencilFunc=D3D11_COMPARISON_LESS_EQUAL;
            break;
        case GL_GREATER:
            depthStencilDesc_.FrontFace.StencilFunc=D3D11_COMPARISON_GREATER;
            depthStencilDesc_.BackFace.StencilFunc=D3D11_COMPARISON_GREATER;
            break;
        case GL_GEQUAL:
            depthStencilDesc_.FrontFace.StencilFunc=D3D11_COMPARISON_GREATER_EQUAL;
            depthStencilDesc_.BackFace.StencilFunc=D3D11_COMPARISON_GREATER_EQUAL;
            break;
        case GL_EQUAL:
            depthStencilDesc_.FrontFace.StencilFunc=D3D11_COMPARISON_EQUAL;
            depthStencilDesc_.BackFace.StencilFunc=D3D11_COMPARISON_EQUAL;
            break;
        case GL_NOTEQUAL:
            depthStencilDesc_.FrontFace.StencilFunc=D3D11_COMPARISON_NOT_EQUAL;
            depthStencilDesc_.BackFace.StencilFunc=D3D11_COMPARISON_NOT_EQUAL;
            break;
        case GL_ALWAYS:
            depthStencilDesc_.FrontFace.StencilFunc=D3D11_COMPARISON_ALWAYS;
            depthStencilDesc_.BackFace.StencilFunc=D3D11_COMPARISON_ALWAYS;
            break;
        }
        depthStencilDesc_.StencilWriteMask=mask;
        depthStencilDesc_.StencilReadMask=mask;
        maskRef=ref;
        SAFE_RELEASE(depthStencilState_);
        device_->CreateDepthStencilState(&depthStencilDesc_,&depthStencilState_);
	}

	//**************************************************************************
	//Set the stencil mask.
	//**************************************************************************
	void OpenGL::glStencilMask (GLuint mask)
	{
        depthStencilDesc_.StencilWriteMask=mask;
        depthStencilDesc_.StencilReadMask=mask;
        SAFE_RELEASE(depthStencilState_);
        device_->CreateDepthStencilState(&depthStencilDesc_,&depthStencilState_);
	}

	//**************************************************************************
	//Set the stencil operation.
	//**************************************************************************
	void OpenGL::glStencilOp (GLenum dfail, GLenum zfail, GLenum dpass)
	{
        switch(dfail)
        {
        case GL_KEEP:
            depthStencilDesc_.FrontFace.StencilFailOp=D3D11_STENCIL_OP_KEEP;
            depthStencilDesc_.BackFace.StencilFailOp=D3D11_STENCIL_OP_KEEP;
            break;
        case GL_ZERO:
            depthStencilDesc_.FrontFace.StencilFailOp=D3D11_STENCIL_OP_ZERO;
            depthStencilDesc_.BackFace.StencilFailOp=D3D11_STENCIL_OP_ZERO;
            break;
        case GL_REPLACE:
            depthStencilDesc_.FrontFace.StencilFailOp=D3D11_STENCIL_OP_REPLACE;
            depthStencilDesc_.BackFace.StencilFailOp=D3D11_STENCIL_OP_REPLACE;
            break;
        case GL_INCR:
            depthStencilDesc_.FrontFace.StencilFailOp=D3D11_STENCIL_OP_INCR;
            depthStencilDesc_.BackFace.StencilFailOp=D3D11_STENCIL_OP_INCR;
            break;
        case GL_DECR:
            depthStencilDesc_.FrontFace.StencilFailOp=D3D11_STENCIL_OP_DECR;
            depthStencilDesc_.BackFace.StencilFailOp=D3D11_STENCIL_OP_DECR;
            break;
        case GL_INVERT:
            depthStencilDesc_.FrontFace.StencilFailOp=D3D11_STENCIL_OP_INVERT;
            depthStencilDesc_.BackFace.StencilFailOp=D3D11_STENCIL_OP_INVERT;
            break;
        }

        switch(zfail)
        {
        case GL_KEEP:
            depthStencilDesc_.FrontFace.StencilDepthFailOp=D3D11_STENCIL_OP_KEEP;
            depthStencilDesc_.BackFace.StencilDepthFailOp=D3D11_STENCIL_OP_KEEP;
            break;
        case GL_ZERO:
            depthStencilDesc_.FrontFace.StencilDepthFailOp=D3D11_STENCIL_OP_ZERO;
            depthStencilDesc_.BackFace.StencilDepthFailOp=D3D11_STENCIL_OP_ZERO;
            break;
        case GL_REPLACE:
            depthStencilDesc_.FrontFace.StencilDepthFailOp=D3D11_STENCIL_OP_REPLACE;
            depthStencilDesc_.BackFace.StencilDepthFailOp=D3D11_STENCIL_OP_REPLACE;
            break;
        case GL_INCR:
            depthStencilDesc_.FrontFace.StencilDepthFailOp=D3D11_STENCIL_OP_INCR;
            depthStencilDesc_.BackFace.StencilDepthFailOp=D3D11_STENCIL_OP_INCR;
            break;
        case GL_DECR:
            depthStencilDesc_.FrontFace.StencilDepthFailOp=D3D11_STENCIL_OP_DECR;
            depthStencilDesc_.BackFace.StencilDepthFailOp=D3D11_STENCIL_OP_DECR;
            break;
        case GL_INVERT:
            depthStencilDesc_.FrontFace.StencilDepthFailOp=D3D11_STENCIL_OP_INVERT;
            depthStencilDesc_.BackFace.StencilDepthFailOp=D3D11_STENCIL_OP_INVERT;
            break;
        }

        switch(dpass)
        {
        case GL_KEEP:
            depthStencilDesc_.FrontFace.StencilPassOp=D3D11_STENCIL_OP_KEEP;
            depthStencilDesc_.BackFace.StencilPassOp=D3D11_STENCIL_OP_KEEP;
            break;
        case GL_ZERO:
            depthStencilDesc_.FrontFace.StencilPassOp=D3D11_STENCIL_OP_ZERO;
            depthStencilDesc_.BackFace.StencilPassOp=D3D11_STENCIL_OP_ZERO;
            break;
        case GL_REPLACE:
            depthStencilDesc_.FrontFace.StencilPassOp=D3D11_STENCIL_OP_REPLACE;
            depthStencilDesc_.BackFace.StencilPassOp=D3D11_STENCIL_OP_REPLACE;
            break;
        case GL_INCR:
            depthStencilDesc_.FrontFace.StencilPassOp=D3D11_STENCIL_OP_INCR;
            depthStencilDesc_.BackFace.StencilPassOp=D3D11_STENCIL_OP_INCR;
            break;
        case GL_DECR:
            depthStencilDesc_.FrontFace.StencilPassOp=D3D11_STENCIL_OP_DECR;
            depthStencilDesc_.BackFace.StencilPassOp=D3D11_STENCIL_OP_DECR;
            break;
        case GL_INVERT:
            depthStencilDesc_.FrontFace.StencilPassOp=D3D11_STENCIL_OP_INVERT;
            depthStencilDesc_.BackFace.StencilPassOp=D3D11_STENCIL_OP_INVERT;
            break;
        }
        SAFE_RELEASE(depthStencilState_);
        device_->CreateDepthStencilState(&depthStencilDesc_,&depthStencilState_);
	}
    //**************************************************************************
    //Update the viewport.
    //**************************************************************************
    void OpenGL::glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
    {
        CheckCurrentTarget();
        D3D11_VIEWPORT viewport;
        viewport.TopLeftX=(float)x;
        viewport.TopLeftY=(float)y;
        viewport.Width=(float)width;
        viewport.Height=(float)height;
        viewport.MinDepth=0.0f;
        viewport.MaxDepth=1.0f;
        context_->RSSetViewports(1,&viewport);
    }

    //**************************************************************************
    //Current the error block.
    //**************************************************************************
    GLenum OpenGL::glGetError (void)
    {
        return 0;
    }

    //**************************************************************************
    //Get an open gl params.
    //**************************************************************************
    void OpenGL::glGetFloatv (GLenum pname, GLfloat *params)
    {
		switch(pname)
		{
		case GL_COLOR_CLEAR_VALUE:
			memcpy(params,clearColor_,sizeof(float)*4);
			break;
		case GL_DEPTH_CLEAR_VALUE:
			*params = depthValue_;
			break;
		case GL_STENCIL_CLEAR_VALUE:
			*params = stencilValue_;
			break;
		case GL_ALPHA_TEST_REF:
			*params = 0;
			break;
		}
    }

    //**************************************************************************
    //Get an open gl params.
    //**************************************************************************
    void OpenGL::glGetIntegerv (GLenum pname, GLint *params)
    {
        switch(pname)
        {
        case GL_MAX_TEXTURE_SIZE:
            *params=D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            break;
		case GL_FRAMEBUFFER_BINDING:
            *params=currentFrameBuffer_;
			break;
		case GL_RENDERBUFFER_BINDING:
			*params=currentRenderBuffer_;
			break;
        case GL_LINE_WIDTH:
            *params = 1;
            break;
        case GL_STENCIL_BITS:
            *params = 8;
            break;
        }
    }
    //**************************************************************************
    //Get an open gl param.
    //**************************************************************************
    const GLubyte* OpenGL::glGetString (GLenum name)
    {

        return NULL;
    }
}
