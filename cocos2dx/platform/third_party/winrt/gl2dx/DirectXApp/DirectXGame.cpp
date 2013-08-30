//-----------------------------------------------------------------------------------------------
// Copyright (c) 2012 Andrew Garrison
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
#include "pch.h"

#include <wrl/client.h>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <memory>
#include "DirectXGame.h"
#include <DirectXMath.h>
#include "DirectXHelper.h"

#include "Utility.h"
#include "gl2dx.h"
#include "Bitmap.h"

using namespace Microsoft::WRL;
using namespace ::Windows::Foundation;
using namespace ::Windows::UI::Core;
using namespace DirectX;
using namespace Windows::UI::ViewManagement;

namespace DirectXApp
{
	struct Vertex
	{
		float x,y,z;
        float r,g,b,a;
		float u,v;
	};
   DirectXGame::DirectXGame()
   {
      _textureId = 0;
   }

   unsigned int DirectXGame::LoadTexture(const std::string& fileName)
   {
      unsigned int textureId = 0;
      Bitmap bitmap;
      bitmap.Load(fileName.c_str());
      glEnable(GL_TEXTURE_2D);
      glGenTextures(1, &textureId);
      glBindTexture(GL_TEXTURE_2D, textureId);
      int imageWidth  = bitmap.GetWidth();
      int imageHeight = bitmap.GetHeight();
      unsigned char* data = bitmap.GetData();
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      return textureId;
   }

   DirectXGame::~DirectXGame()
   {
   }

   void DirectXGame::CreateDeviceResources()
   {
      DirectXBase::CreateDeviceResources();
      gl2dxInit(m_d3dDevice.Get(),m_d3dContext.Get());
      _textureId = LoadTexture("Assets\\texture.bmp");	  	  
      Vertex vertex[]=
	  {
		  {-0.5f,-0.5f,0, 1.0f,1.0f,1.0f,1.0f ,0.0f,0.0f},
		  {0.5f,-0.5f,0,  1.0f,1.0f,1.0f,1.0f ,1.0f,0.0f},
		  {0.5f,0.5f,0,   1.0f,1.0f,1.0f,1.0f, 1.0f,1.0f},
          {-0.5f,0.5f,0,  1.0f,1.0f,1.0f,1.0f ,0.0f,1.0f},
	  };
	  short Indices[]={0,1,2,0,2,3};

      Vertex vertex2[]=
	  {
		  {-0.5f,-0.5f,0, 1.0f,1.0f,1.0f,1.0f ,0.0f,1-0.0f},
		  {0.5f,-0.5f,0,  1.0f,1.0f,1.0f,1.0f ,1.0f,1-0.0f},
		  {0.5f,0.5f,0,   1.0f,1.0f,1.0f,1.0f, 1.0f,1-1.0f},
          {-0.5f,0.5f,0,  1.0f,1.0f,1.0f,1.0f ,0.0f,1-1.0f},
	  };
	  short Indices2[]={0,1,2,0,2,3};

      GLuint bufferId[2];
	  glGenBuffers(2,&bufferId[0]);
	  glBindBuffer(GL_ARRAY_BUFFER,bufferId[0]);
	  glBufferData(GL_ARRAY_BUFFER,sizeof(vertex),vertex,GL_STATIC_DRAW);
	  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,bufferId[1]);
	  glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(Indices),Indices,GL_STATIC_DRAW);


      GLuint bufferId2[2];
	  glGenBuffers(2,&bufferId2[0]);
	  glBindBuffer(GL_ARRAY_BUFFER,bufferId2[0]);
	  glBufferData(GL_ARRAY_BUFFER,sizeof(vertex2),vertex2,GL_STATIC_DRAW);
	  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,bufferId2[1]);
	  glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(Indices2),Indices2,GL_STATIC_DRAW);

      //Create an fbo texture.
      GLuint fbo=0;
      GLuint fboTexture=0;
      GLuint renderbuffer=0;


      glGenTextures(1,&fboTexture);
      glBindTexture(GL_TEXTURE_2D,fboTexture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1680, 1050, 0, GL_RGBA, GL_UNSIGNED_BYTE,0);
      glGenFramebuffers(1,&fbo);
      glGenRenderbuffers(1,&renderbuffer);
      glBindFramebuffer(GL_FRAMEBUFFER,fbo);
      glBindRenderbuffer(GL_RENDERBUFFER,renderbuffer);
      glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_STENCIL,1680,1050);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,renderbuffer);
      glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,fboTexture,0);
   }

   void DirectXGame::CreateWindowSizeDependentResources()
   {
      DirectXBase::CreateWindowSizeDependentResources();
      gl2dxInitTargetAndStencil(m_renderTargetView.Get(),m_depthStencilView.Get());
      glOrtho(-1,1,-1,1,-1,1);
   }

   void DirectXGame::Render()
   {
       glBindFramebuffer(GL_FRAMEBUFFER,1);
       glClearColor(1,0,0,1);
       glBindBuffer(GL_ARRAY_BUFFER,1);
       glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,2);
       glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
       glEnable(GL_TEXTURE_2D);
       glBindTexture(GL_TEXTURE_2D,1);
       glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,0);

       glBindFramebuffer(GL_FRAMEBUFFER,0);
       glBindBuffer(GL_ARRAY_BUFFER,3);
       glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,4);
       glClearColor(0,0,0,1);
       glEnable(GL_TEXTURE_2D);
       glBindTexture(GL_TEXTURE_2D,2);
       glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
       glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,0);       
   }
}
