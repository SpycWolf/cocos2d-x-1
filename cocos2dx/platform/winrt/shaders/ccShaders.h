/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) Microsoft Open Technologies, Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
#ifndef __CCSHADER_H__
#define __CCSHADER_H__

#include "CCGL.h"
#include "platform/CCPlatformMacros.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT) || (CC_TARGET_PLATFORM == CC_PLATFORM_WP8)
#include "shaders/ccPositionColorLengthTexture_vert.hps"
#include "shaders/ccPositionColorLengthTexture_frag.hps"
#include "shaders/ccPositionTextureA8Color_frag.hps"
#include "shaders/ccPositionTexture_frag.hps"
#include "shaders/ccPositionTexture_vert.hps"
#include "shaders/ccPositionColor_frag.hps"
#include "shaders/ccPosition_vert.hps"
#include "shaders/ccPosition_frag.hps"
#include "shaders/ccPositionColor_vert .hps"
#include "shaders/ccPositionTextureColorAlphaTest.hps"
#include "shaders/ccPositionTextureColor_frag.hps"
#include "shaders/ccPositionTextureColor_vert.hps"
#include "shaders/ccShaderEx_SwitchMask_frag.hps"
#else
extern CC_DLL const char * ccPosition_uColor_frag;
extern CC_DLL const char* ccPosition_uColor_vert;

extern CC_DLL const char * ccPositionColor_frag;
extern CC_DLL const char * ccPositionColor_vert;

extern CC_DLL const char * ccPositionTexture_frag;
extern CC_DLL const char * ccPositionTexture_vert;

extern CC_DLL const char * ccPositionTextureA8Color_frag;
extern CC_DLL const char * ccPositionTextureA8Color_vert;

extern CC_DLL const char * ccPositionTextureColor_frag;
extern CC_DLL const char * ccPositionTextureColor_vert;

extern CC_DLL const char * ccPositionTextureColorAlphaTest_frag;

extern CC_DLL const char * ccPositionTexture_uColor_frag;
extern CC_DLL const char * ccPositionTexture_uColor_vert;

extern CC_DLL const char * ccPositionColorLengthTexture_frag;
extern CC_DLL const char * ccPositionColorLengthTexture_vert;

extern CC_DLL const char * ccExSwitchMask_frag;
#endif

NS_CC_BEGIN

/**
 * @addtogroup shaders
 * @{
 */



// end of shaders group
/// @}

NS_CC_END

#endif /* __CCSHADER_H__ */
