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

#include "ccShaders.h"


NS_CC_BEGIN

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT) || (CC_TARGET_PLATFORM == CC_PLATFORM_WP8)

#else
//
const char * ccPosition_uColor_frag;
const char * ccPosition_uColor_vert;

//
const char * ccPositionColor_frag;
const char * ccPositionColor_vert;

//
const char * ccPositionTexture_frag;/*#include "ccShader_PositionTexture_frag.h"*/
const char * ccPositionTexture_vert;/*#include "ccShader_PositionTexture_vert.h"*/

//
const char * ccPositionTextureA8Color_frag;//#include "ccShader_PositionTextureA8Color_frag.h"
const char * ccPositionTextureA8Color_vert;//#include "ccShader_PositionTextureA8Color_vert.h"

//
const char * ccPositionTextureColor_frag;//#include "ccShader_PositionTextureColor_frag.h"
const char * ccPositionTextureColor_vert;//#include "ccShader_PositionTextureColor_vert.h"

//
const char * ccPositionTextureColorAlphaTest_frag;/*#include "ccShader_PositionTextureColorAlphaTest_frag.h"*/

//
const char * ccPositionTexture_uColor_frag;/*#include "ccShader_PositionTexture_uColor_frag.h"*/
const char * ccPositionTexture_uColor_vert;/*#include "ccShader_PositionTexture_uColor_vert.h"*/

const char * ccExSwitchMask_frag;/*#include "ccShaderEx_SwitchMask_frag.h"*/

//
const char * ccPositionColorLengthTexture_frag;/*#include "ccShader_PositionColorLengthTexture_frag.h"*/
const char * ccPositionColorLengthTexture_vert;/*#include "ccShader_PositionColorLengthTexture_vert.h"*/
#endif
NS_CC_END
