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
#pragma once

#include "gl2dx.h"
#include <map>
#include <stack>
#include <list>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) {if(p){p->Release();p=NULL;}}
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) {if(p){delete p;p=NULL;}}
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) {if(p){delete[] p;p=NULL;}}
#endif


#define MAX_COLOR_ATTACHEMENT GL_COLOR_ATTACHMENT15
#define MAX_VERTEX_ATTRIBUTES 3

#define SHADER_VARIABLE_NONE -1
#define SHADER_VARIABLE_VALUE 0
#define SHADER_VARIABLE_SINGLE 1
#define SHADER_VARIABLE_ARRAY2D 2
#define SHADER_VARIABLE_ARRAY3D 3
#define SHADER_VARIABLE_ARRAY4D 4
#define SHADER_VARIABLE_MATRIX 5
#define SHADER_VARIABLE_POINTER_FLOAT 6
#define SHADER_VARIABLE_POINTER_INT 7
#define SHADER_VARIABLE_MATRIX_POINTER 8
#define SHADER_VARIABLE_SAMPLER 9
namespace gl2dx
{
    struct FastIndexLocator
    {
        std::stack<int> indices_;
    };
    struct ShaderVariable
    {
        int id;                   //variable id.
        int type;                 //variable type.
        int count;
        union
        {
            int value;             //x
            float single;          //x
            float array2d[2];      //xy
            float array3d[3];      //xyz
            float array4d[4];      //xyzw
            float matrix[16];      //matrix element.
            float* pointerArrayFloat;   //Buffer of any size.
            int* pointerArrayInt;
        };
        ShaderVariable():id(-1),type(SHADER_VARIABLE_NONE),count(0)
        {
        }
    };
    struct AttributeInfo
    {
        GLuint size;
        GLuint stride;
        GLenum type;
        GLvoid* data;
        char bufferName[64];
        AttributeInfo():size(0),stride(0),data(NULL)
        {
            memset(bufferName,0,sizeof(bufferName));
        }
    };
    struct States
    {
        GLboolean enableTexture2D_;
        GLboolean enableScissorTest_;
        GLboolean enableBlend_;
        States():enableTexture2D_(false),enableScissorTest_(false),enableBlend_(false)
        {

        }
    };
   struct FboAttachement
   {
	   GLuint colorAttachement[MAX_COLOR_ATTACHEMENT];
	   GLenum colorAttachementType[MAX_COLOR_ATTACHEMENT];
	   GLuint depthAttachement;
       GLenum depthAttachementType;
	   GLuint stencilAttachement;
       FboAttachement():depthAttachement(0),stencilAttachement(0)
       {
           memset(colorAttachement,0,sizeof(GLuint)*MAX_COLOR_ATTACHEMENT);
           memset(colorAttachementType,0,sizeof(GLuint)*MAX_COLOR_ATTACHEMENT);
       }
   };

   struct BufferInfo
   {
	   GLenum type_;
	   ID3D11Buffer* buffer_;
       BufferInfo():buffer_(NULL)
       {

       }
   };
   struct RenderBufferInfo
   {
       ID3D11DepthStencilView* depthStencilView_;
       RenderBufferInfo():depthStencilView_(NULL)
       {

       }
   };
   struct Texture2DInfo
   {
       GLenum type_;
       ID3D11Texture2D* texture2D_;
       ID3D11RenderTargetView* renderTargetView_;
       ID3D11DepthStencilView* depthStencilView_;
       ID3D11ShaderResourceView* texture_;
       ID3D11SamplerState* samplerState_;
       D3D11_SAMPLER_DESC samplerDesc_;
       Texture2DInfo():
       renderTargetView_(NULL),depthStencilView_(NULL),
       texture_(NULL),texture2D_(NULL),
       samplerState_(NULL)
       {
           memset(&samplerDesc_,0,sizeof(samplerDesc_));
           samplerDesc_.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP;
           samplerDesc_.AddressV=D3D11_TEXTURE_ADDRESS_CLAMP;
           samplerDesc_.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
           samplerDesc_.ComparisonFunc=D3D11_COMPARISON_ALWAYS;
           samplerDesc_.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;
           samplerDesc_.MaxLOD=FLT_MAX;
       }
   };
   struct ShaderInfo
   {
	   GLenum type;
	   GLuint codeLength;
	   char* codeBuffer;
       ShaderInfo():codeLength(0),codeBuffer(NULL)
       {

       }
   }; 
   //**********************************************************
   //Shader linkage just holds the vertex/pixel shader object.
   //**********************************************************
   struct ShaderLinkage
   {
	   ID3D11VertexShader* vertexShader_;
	   ID3D11PixelShader* pixelShader_;
       ID3D11InputLayout* inputLayout_;
       ID3D11Buffer* uniformBuffer;
       ShaderLinkage():vertexShader_(NULL),pixelShader_(NULL),inputLayout_(NULL),uniformBuffer(NULL)
       {
       }
   };
   //**********************************************************
   //Shader program definition.
   //**********************************************************
   struct ShaderProgram
   {
	   GLuint vertexShader_;
	   GLuint pixelShader_;
       ShaderLinkage shaderLinkage_;
       AttributeInfo attributes_[3];
       std::map<std::string,ShaderVariable> shaderVariables_;
       std::map<GLuint,ShaderVariable> shaderVariableByIndex_;
       ShaderProgram()
       {
           vertexShader_=0;
           pixelShader_=0;
           memset(attributes_,0,sizeof(attributes_));
       }
   };
   struct InternalVBO
   {
       InternalVBO();
       ~InternalVBO();
       ID3D11Buffer* vbo;
       unsigned int previousLocation;
       unsigned int location;
       void CreateInternalBuffer(ID3D11Device* device,const int size);
       void Map(ID3D11DeviceContext* context,const int size,void** output);
       void Unmap(ID3D11DeviceContext* context);
       bool firstTimeInit;
   };
#define MAX_INTERNAL_VBO 1
#define VBO_POSITION 0
#define VBO_POSITION_COLOR 1
#define VBO_POSITION_COLOR_TEXTURE 2
#define VBO_SIZE 1024*1024*3  //Times it by 3 in order for it to fit into any type of primitive topology.

   // Manages the conversion of native OpenGL calls into their DirectX equivalents.
   class OpenGL
   {
   protected:
       InternalVBO internalVbos_[MAX_INTERNAL_VBO];
       void glUpdatePosition(GLenum mode, GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,char* vboPositionMemory,GLuint stride);
       void glUpdatePositionFan(GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,float* vboPositionMemory,GLuint stride);
       void glUpdateColor(GLenum mode, GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,char* vboPositionMemory,GLuint stride);
       void glUpdateColorFan(GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,float* vboPositionMemory,GLuint stride);
       void glUpdateTexCoord(GLenum mode, GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,char* vboPositionMemory,GLuint stride);
       void glUpdateTexCoordFan(GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,float* vboPositionMemory,GLuint stride);
       bool glUpdatePositionIndices(GLenum mode, GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,char* vboPositionMemory,short* vboIndices,GLuint stride,GLuint& failCount);
       void glUpdateColorIndices(GLenum mode, GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,char* vboPositionMemory,short* vboIndices,GLuint stride);
        void glUpdateTexCoordIndices(GLenum mode, GLint first, GLsizei count,AttributeInfo* attInfo,char* byteArray,char* vboPositionMemory,short* vboIndices,GLuint stride);
      
   public:
        OpenGL(ID3D11Device* device, ID3D11DeviceContext* context);
       ~OpenGL();
       void glBlendFunc(GLenum sfactor, GLenum dfactor);
	   void glDeleteTextures (GLsizei n, const GLuint *textures);
	   void glTexParameteri (GLenum target, GLenum pname, GLint param);
	   void glGetBooleanv (GLenum pname, GLboolean *params);
	   void glFlush (void);
	   void glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
        void glBindTexture(GLenum target, GLuint texture);
        void glDisable(GLenum cap);
        void glDrawArrays(GLenum mode, GLint first, GLsizei count);
        void RenderAsFan(GLint first,GLsizei count);
        void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
        void glEnable(GLenum cap);
        void glGenTextures(GLsizei n, GLuint *textures);
        void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
        void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
        void glGenBuffers(GLsizei n,GLuint* buffers);
        void glGenFramebuffers(GLsizei n,GLuint* buffers);
        void glGenRenderbuffers(GLsizei n,GLuint* buffers);
        void glBindBuffer(GLenum target,const GLuint buffer);
        void glBindFramebuffer(GLenum target,const GLuint buffer);
        void glBindRenderbuffer(GLenum target,const GLuint buffer);
        void glDeleteRenderbuffers(GLsizei count,const GLuint* buffers);
        void glBufferData(GLenum target,GLsizei size,const GLvoid* pData,GLenum streamType);
        void glBufferSubData(GLenum target,GLsizei offset,GLsizei size,const GLvoid* data);
        void glClear(GLsizei clearBits);
        void glClearColor(GLfloat r,GLfloat g,GLfloat b,GLfloat a);
        void glClearDepth(GLfloat depth);
        void glClearStencil(GLubyte value);
        void glDeleteBuffers(GLsizei n,const GLuint* buffers);
        void glDeleteFramebuffers(GLsizei n,const GLuint* buffers);
        void glDepthFunc(GLenum depthFunc);
        void glDepthMask(GLboolean mask);
        void glEnableVertexAttribArray(GLuint index);
        void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
        void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
        void glGenerateMipmap(GLenum target);
        void glRegisterUniformLocation(GLuint programObj,const GLuint location,const char* name);
        void glRegisterUniformSamplerLocation(GLuint programObj,const GLuint location,const char* name);
        GLint glGetUniformLocation(GLuint programObj, const char* name);
        bool glIsEnabled(GLenum cap);
        void glPixelStorei(GLenum pname, GLint param);
        void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
        void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
        void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
        void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog);
        void glGetProgramiv(GLuint program, GLenum pname, GLint* param);
        void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog);
        void glActiveTexture(GLenum texture);
        void glAttachShader(GLuint program, GLuint shader);
        void glBindAttribLocation(GLuint program, GLuint index, const char* name);
        void glBlendEquation(GLenum mode);
        GLenum glCheckFramebufferStatus(GLenum target);
        void glCompileShader(GLuint shader);
        void glBuildDxShader(GLuint shader, GLenum type, const char* source,const GLuint size);
        void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data);
        void glGetShaderiv(GLuint shader, GLenum pname, GLint* param);
        GLuint glCreateShader(GLenum type);
        void glDeleteProgram(GLuint program);
        void glDeleteShader(GLuint shader);
        void glDisableVertexAttribArray(GLuint);
        void glLinkProgram(GLuint program);
        void glShaderSource(GLuint shaderObj, GLsizei count, const char ** string, const GLint *length);
        GLuint glCreateProgram();
        void glGetShaderSource(GLsizei obj, GLsizei maxLength, GLsizei* length, char *source);
        void glUniform1f(GLint location, GLfloat v0);
        void glUniform1i(GLint location, GLint v0);
        void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
        void glUniform2fv(GLint location, GLsizei count, const GLfloat* value);
        void glUniform2i(GLint location, GLint v0, GLint v1);
        void glUniform2iv(GLint location, GLsizei count, const GLint* value);
        void glUniform3f(GLint location, GLfloat v0, GLfloat v1,GLfloat v2);
        void glUniform3fv(GLint location, GLsizei count, const GLfloat* value);
        void glUniform3i(GLint location, GLint v0, GLint v1,GLint v2);
        void glUniform3iv(GLint location, GLsizei count, const GLint* value);
        void glUniform4f(GLint location, GLfloat v0, GLfloat v1,GLfloat v2,GLfloat v3);
        void glUniform4fv(GLint location, GLsizei count, const GLfloat* value);
        void glUniform4i(GLint location, GLint v0, GLint v1,GLint v2,GLint v3);
        void glUniform4iv(GLint location, GLsizei count, const GLint* value);
        void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
        void glUseProgram(GLuint program);
        void glStencilFunc (GLenum func, GLint ref, GLuint mask);
        void glStencilMask (GLuint mask);
        void glStencilOp (GLenum fail, GLenum zfail, GLenum zpass);
        void glViewport (GLint x, GLint y, GLsizei width, GLsizei height);
        GLenum glGetError (void);
        void glGetFloatv (GLenum pname, GLfloat *params);
        void glGetIntegerv (GLenum pname, GLint *params);
        const GLubyte * glGetString (GLenum name);
        void SetRenderTargetAndDepthStencil(ID3D11RenderTargetView* rView,ID3D11DepthStencilView* dView)
        {
            backBufferRenderTargetView_=rView;
            backBufferDepthStencilView_=dView;
        }
   public:
      void UpdateScissorState(bool enabled);
      void CreateInputLayoutFromShader(GLuint program);
      void glBuildUniformConstantBuffer(GLuint program);
      void DrawVerticesWithProgram(GLenum mode,GLsizei first,ShaderProgram* currentProgram,const GLuint offset,const GLuint count,const GLuint stride);
      void SetShaderTextures(ShaderProgram* program);
      void UnSetShaderTextures(ShaderProgram* program);
      void CheckCurrentTarget();
   public:
      GLuint maskRef;
      FastIndexLocator textureIndices_;
      FastIndexLocator rboIndices_;
      FastIndexLocator fboIndices_;
      FastIndexLocator vboIndices_;
      std::map<GLuint,Texture2DInfo> textures_;          //2D Textures.
	  std::map<GLuint,BufferInfo> buffers_;              //VBO/IBO no PBO support.
      std::map<GLuint,RenderBufferInfo> rbos_;           //RBO
	  std::map<GLuint,FboAttachement> fboAttachements_;  //FBO
	  std::map<GLuint,ShaderInfo> shaders_;              //Shaders info.
	  std::map<GLuint,ShaderProgram> shaderPrograms_;    //Shader programs.
      D3D11_INPUT_ELEMENT_DESC vertexAttribPointer[MAX_VERTEX_ATTRIBUTES];
      ID3D11RasterizerState* rasterizerState;
      D3D11_RASTERIZER_DESC rasterizerDesc_;
      D3D11_DEPTH_STENCIL_DESC depthStencilDesc_;
      D3D11_BLEND_DESC blendDesc_;
      ID3D11Device* device_;
      ID3D11DeviceContext* context_;
      ID3D11BlendState* blendState_;
      ID3D11RenderTargetView* backBufferRenderTargetView_;
      ID3D11DepthStencilView* backBufferDepthStencilView_;
      ID3D11DepthStencilState* depthStencilState_; 
      D3D11_RECT scissorRect_;
      // The current set of vertices to render with the next DrawBatch call
	  GLuint blendMask_;
	  GLint vertexAttribArray[16];                     
      GLint currentTexture_[GL_TEXTURE31-GL_TEXTURE0];
      GLint currentActiveTexture_;
      GLint currentProgram_;
	  GLint currentFrameBuffer_;
      GLint oldFrameBuffer_;
	  GLint currentRenderBuffer_;
	  GLint currentVertexBuffer_;
	  GLint currentIndexBuffer_;
	  GLint currentTextureUnit_;
	  GLfloat clearColor_[4];
	  GLfloat depthValue_;
	  GLenum generateMipmaps_;
	  GLubyte stencilValue_;
      States states_;

      static const D3D11_INPUT_ELEMENT_DESC positionElements[1];
      static const D3D11_INPUT_ELEMENT_DESC positionColorElements[2];
      static const D3D11_INPUT_ELEMENT_DESC positionColorUVElements[3];
   };
}
