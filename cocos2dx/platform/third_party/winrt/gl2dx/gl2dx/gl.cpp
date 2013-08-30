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
#include "gl2dx.h"
#include "OpenGL.h"
#include <map>
using namespace gl2dx;
using namespace std;

OpenGL* _openGL;

void gl2dxAssert(bool condition, char* message)
{
   if (condition == false)
   {
      throw new GL2DXException(message);
   }
}

void gl2dxNotSupported(char* message)
{
   throw new GL2DXException(message);
}

void gl2dxInit(ID3D11Device* device, ID3D11DeviceContext* context)
{
    _openGL = new OpenGL(device, context);
}
void gl2dxInitTargetAndStencil(ID3D11RenderTargetView* rView,ID3D11DepthStencilView* dView)
{
    _openGL->SetRenderTargetAndDepthStencil(rView,dView);
}

void glBindTexture(GLenum target, GLuint texture)
{
   _openGL->glBindTexture(target, texture);
}

void glBlendFunc (GLenum sfactor, GLenum dfactor)
{
	_openGL->glBlendFunc(sfactor,dfactor);
}

void glDeleteTextures (GLsizei n, const GLuint *textures)
{
	_openGL->glDeleteTextures(n,textures);
}

void glDisable (GLenum cap)
{
   _openGL->glDisable(cap);
}


void glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
   _openGL->glDrawArrays(mode, first, count);
}

void glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
   _openGL->glDrawElements(mode, count, type, indices);
}

void glEnable (GLenum cap)
{
   _openGL->glEnable(cap);
}
void glGenTextures (GLsizei n, GLuint *textures)
{
   _openGL->glGenTextures(n, textures);
}

void glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
   _openGL->glScissor(x, y, width, height);
}

void glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   _openGL->glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void glTexParameteri (GLenum target, GLenum pname, GLint param)
{
	_openGL->glTexParameteri(target,pname,param);
}

void glGenBuffers(GLsizei n,GLuint* buffers)
{
	_openGL->glGenBuffers(n,buffers);
}
void glGenFramebuffers(GLsizei n,GLuint* buffers)
{
	_openGL->glGenFramebuffers(n,buffers);
}
void glGenRenderbuffers(GLsizei n,GLuint* buffers)
{
	_openGL->glGenRenderbuffers(n,buffers);
}
void glBindBuffer(GLenum target,const GLuint buffer)
{
	_openGL->glBindBuffer(target,buffer);
}
void glBindFramebuffer(GLenum target,const GLuint buffer)
{
	_openGL->glBindFramebuffer(target,buffer);
}
void glBindRenderbuffer(GLenum target,const GLuint buffer)
{
	_openGL->glBindRenderbuffer(target,buffer);
}
void glDeleteRenderbuffers(GLsizei count,const GLuint* buffers)
{
	_openGL->glDeleteRenderbuffers(count,buffers);
}
void glBufferData(GLenum target,GLsizei size,const GLvoid* pData,GLenum streamType)
{
	_openGL->glBufferData(target,size,pData,streamType);
}
void glBufferSubData(GLenum target,GLsizei offset,GLsizei size,const GLvoid* data)
{
	_openGL->glBufferSubData(target,offset,size,data);
}
void glClear(GLsizei clearBits)
{
	_openGL->glClear(clearBits);
}
void glClearColor(GLfloat r,GLfloat g,GLfloat b,GLfloat a)
{
	_openGL->glClearColor(r,g,b,a);
}
void glClearDepth(GLfloat depth)
{
	_openGL->glClearDepth(depth);
}
void glClearStencil(GLubyte value)
{
	_openGL->glClearStencil(value);
}
void glDeleteBuffers(GLsizei n,const GLuint* buffers)
{
	_openGL->glDeleteBuffers(n,buffers);
}
void glDeleteFramebuffers(GLsizei n,const GLuint* buffers)
{
	_openGL->glDeleteFramebuffers(n,buffers);
}
void glDepthFunc(GLenum depthFunc)
{
	_openGL->glDepthFunc(depthFunc);
}
void glDepthMask(GLboolean mask)
{
	_openGL->glDepthMask(mask);
}
void glEnableVertexAttribArray(GLuint index)
{
	_openGL->glEnableVertexAttribArray(index);
}
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	_openGL->glFramebufferRenderbuffer(target,attachment,renderbuffertarget,renderbuffer);
}
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	_openGL->glFramebufferTexture2D(target,attachment,textarget,texture,level);
}
void glGenerateMipmap(GLenum target)
{
	_openGL->glGenerateMipmap(target);
}
GLsizei glGetUniformLocation(GLuint programObj, const char* name)
{
	return _openGL->glGetUniformLocation(programObj,name);
}
bool glIsEnabled(GLenum cap)
{
	return _openGL->glIsEnabled(cap);
}
void glPixelStorei(GLenum pname, GLint param)
{
	_openGL->glPixelStorei(pname,param);
}
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	_openGL->glReadPixels(x,y,width,height,format,type,pixels);
}
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer)
{
	_openGL->glVertexAttribPointer(index,size,type,normalized,stride,pointer);
}
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
	_openGL->glRenderbufferStorage(target,internalformat,width,height);
}
void glGetProgramInfoLog(GLuint program, GLsizei buffSize, GLsizei* length, char* infoLog)
{
	_openGL->glGetProgramInfoLog(program,buffSize,length,infoLog);
}
void glGetProgramiv(GLuint program, GLenum pname, GLint* param)
{
	_openGL->glGetProgramiv(program,pname,param);
}
void glGetShaderInfoLog(GLuint shader, GLsizei buffSize, GLsizei* length, char* infoLog)
{
	_openGL->glGetShaderInfoLog(shader,buffSize,length,infoLog);
}
void glRegisterUniformLocation(GLuint programObj,const GLuint location,const char* name)
{
    _openGL->glRegisterUniformLocation(programObj,location,name);
}
void glRegisterUniformSamplerLocation(GLuint programObj,const GLuint location,const char* name)
{
    _openGL->glRegisterUniformSamplerLocation(programObj,location,name);
}
void glActiveTexture(GLenum texture)
{
	_openGL->glActiveTexture(texture);
}
void glAttachShader(GLuint program, GLuint shader)
{
	_openGL->glAttachShader(program,shader);
}
void glBindAttribLocation(GLuint program, GLuint index, const char* name)
{
	_openGL->glBindAttribLocation(program,index,name);
}

void glBlendEquation(GLenum mode)
{
	_openGL->glBlendEquation(mode);
}
GLenum glCheckFramebufferStatus(GLenum target)
{
	return _openGL->glCheckFramebufferStatus(target);
}
void glCompileShader(GLuint shader)
{
	_openGL->glCompileShader(shader);
}
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
	_openGL->glCompressedTexImage2D(target,level,internalformat,width,height,border,imageSize,data);
}
void glGetShaderiv(GLuint shader, GLenum pname, GLint* param)
{
	_openGL->glGetShaderiv(shader,pname,param);
}
GLuint glCreateShader(GLenum type)
{
	return _openGL->glCreateShader(type);
}
void glDeleteProgram(GLuint program)
{
	_openGL->glDeleteProgram(program);
}
void glDeleteShader(GLuint shader)
{
	_openGL->glDeleteShader(shader);
}
void glDisableVertexAttribArray(GLuint index)
{
	_openGL->glDisableVertexAttribArray(index);
}
void glBuildDxInputLayout(GLuint program)
{
    _openGL->CreateInputLayoutFromShader(program);
}
void glLinkProgram(GLuint program)
{
	_openGL->glLinkProgram(program);
}
void glShaderSource(GLuint shaderObj, GLsizei count, const char ** string, const GLint *length)
{
	_openGL->glShaderSource(shaderObj,count,string,length);
}
GLsizei glCreateProgram()
{
	return _openGL->glCreateProgram();
}
void glBuildDxShader(GLuint shader, GLenum type, const char* source,const GLuint size)
{
    _openGL->glBuildDxShader(shader,type,source,size);
}
void glGetShaderSource(GLsizei obj, GLsizei maxLength, GLsizei* length, char *source)
{
	_openGL->glGetShaderSource(obj,maxLength,length,source);
}
void glUniform1f(GLint location, GLfloat v0)
{
	_openGL->glUniform1f(location,v0);
}
void glUniform1i(GLint location, GLint v0)
{
	_openGL->glUniform1i(location,v0);
}
void glUniform2f(GLint location, GLfloat v0, GLfloat v1)
{
	_openGL->glUniform2f(location,v0,v1);
}
void glUniform2fv(GLint location, GLsizei count, const GLfloat* value)
{
	_openGL->glUniform2fv(location,count,value);
}
void glUniform2i(GLint location, GLint v0, GLint v1)
{
	_openGL->glUniform2i(location,v0,v1);
}
void glUniform2iv(GLint location, GLsizei count, const GLint* value)
{
	_openGL->glUniform2iv(location,count,value);
}
void glUniform3f(GLint location, GLfloat v0, GLfloat v1,GLfloat v2)
{
	_openGL->glUniform3f(location,v0,v1,v2);
}
void glUniform3fv(GLint location, GLsizei count, const GLfloat* value)
{
	_openGL->glUniform3fv(location,count,value);
}
void glUniform3i(GLint location, GLint v0, GLint v1,GLint v2)
{
	_openGL->glUniform3i(location,v0,v1,v2);
}
void glUniform3iv(GLint location, GLsizei count, const GLint* value)
{
	_openGL->glUniform3iv(location,count,value);
}
void glUniform4f(GLint location, GLfloat v0, GLfloat v1,GLfloat v2,GLfloat v3)
{
	_openGL->glUniform4f(location,v0,v1,v2,v3);
}
void glUniform4fv(GLint location, GLsizei count, const GLfloat* value)
{
	_openGL->glUniform4fv(location,count,value);
}
void glUniform4i(GLint location, GLint v0, GLint v1,GLint v2,GLint v3)
{
	_openGL->glUniform4i(location,v0,v1,v2,v3);
}
void glUniform4iv(GLint location, GLsizei count, const GLint* value)
{
	_openGL->glUniform4iv(location,count,value);
}
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	_openGL->glUniformMatrix4fv(location,count,transpose,value);
}
void glUseProgram(GLuint program)
{
	_openGL->glUseProgram(program);
}
void glStencilFunc (GLenum func, GLint ref, GLuint mask)
{
	_openGL->glStencilFunc(func,ref,mask);
}
void glStencilMask (GLuint mask)
{
	_openGL->glStencilMask(mask);
}
void glStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{
	_openGL->glStencilOp(fail,zfail,zpass);
}
void glViewport (GLint x, GLint y, GLsizei width, GLsizei height) 
{
    _openGL->glViewport(x,y,width,height);
}

void glGetFloatv (GLenum pname, GLfloat *params)
{
    _openGL->glGetFloatv(pname,params);
}

void glGetIntegerv (GLenum pname, GLint *params)
{
    _openGL->glGetIntegerv(pname,params);
}

GLenum glGetError (void)
{
    return _openGL->glGetError();
}

const GLubyte * glGetString (GLenum name)
{
    return _openGL->glGetString(name);
}

void glLineWidth (GLfloat width)
{

}

void glFlush (void)
{
	_openGL->glFlush();
}

void glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	_openGL->glColorMask(red,green,blue,alpha);
}

void glGetBooleanv (GLenum pname, GLboolean *params)
{
	_openGL->glGetBooleanv(pname,params);
}