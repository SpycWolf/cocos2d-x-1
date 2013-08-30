/****************************************************************************
Copyright (c) 2010 cocos2d-x.org
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
#define __CC_PLATFORM_IMAGE_CPP__
#include "platform/CCImageCommon_cpp.h"
#include "CCEGLView.h"
#include "CCWinRTUtils.h"
#include <memory>

using namespace Windows::UI::Core;

NS_CC_BEGIN

/**
@brief    A memory DC which uses to draw text on bitmap.
*/
class BitmapDC
{
public:
    BitmapDC() :
		m_pBrush(nullptr),
		m_pWICBitmap(nullptr),
		m_pTextFormat(nullptr)

    {
    }

    ~BitmapDC()
    {
		if (m_pBrush)
		{
			m_pBrush->Release();
		}
		if (m_pTextFormat)
		{
			m_pTextFormat->Release();
		}
 		if (m_pWICBitmap)
		{
			m_pWICBitmap->Release();
		}
   }

	wchar_t * utf8ToUtf16(std::string nString)
	{
		wchar_t * pwszBuffer = NULL;
		do 
		{
			if (nString.size() < 0)
			{
				break;
			}
			// utf-8 to utf-16
			int nLen = nString.size();
			int nBufLen  = nLen + 1;			
			pwszBuffer = new wchar_t[nBufLen];
			CC_BREAK_IF(! pwszBuffer);
			memset(pwszBuffer,0,nBufLen);
			nLen = MultiByteToWideChar(CP_UTF8, 0, nString.c_str(), nLen, pwszBuffer, nBufLen);		
			pwszBuffer[nLen] = '\0';
		} while (0);	
		return pwszBuffer;

	}

    bool setFont(const char * pFontName, CCImage::ETextAlign eAlign, int nSize)
    {
		bool bRet = false;
		do
		{
			CCEGLView* view = CCEGLView::sharedOpenGLView();
			Microsoft::WRL::ComPtr<IDWriteFactory1> pDWriteFactory = view->getDWriteFactory2();
			CC_BREAK_IF(! pDWriteFactory);

			// utf-8 to utf-16
			int nLen = strlen(pFontName);
			int nBufLen  = nLen + 1;
			std::unique_ptr<wchar_t> pwszBuffer(new wchar_t[nBufLen]);
			CC_BREAK_IF(! pwszBuffer);
			memset(pwszBuffer.get(), 0, sizeof(wchar_t)*nBufLen);
			nLen = MultiByteToWideChar(CP_UTF8, 0, pFontName, nLen, pwszBuffer.get(), nBufLen);

			nSize = nSize ? nSize : 18;

			if(m_pTextFormat) 
			{
				m_pTextFormat->Release();
				m_pTextFormat = nullptr;
			}

  			HRESULT hr = pDWriteFactory->CreateTextFormat(
				pwszBuffer.get(),           // Font family name.
				NULL,                       // Font collection (NULL sets it to use the system font collection).
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				nSize,
				L"en-us",
				&m_pTextFormat
			);

			CC_BREAK_IF(hr != 0);

			switch(eAlign)
			{
			case CCImage::ETextAlign::kAlignCenter: // Horizontal center and vertical center
			    m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
				m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
				break;

			case CCImage::ETextAlign::kAlignTop: // Horizontal center and vertical top.
			    m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
				break;
			
            case CCImage::ETextAlign::kAlignTopLeft: // Horizontal left and vertical top.
			    m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
				break;

			case CCImage::ETextAlign::kAlignTopRight: // Horizontal right and vertical top.
			    m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
				break;

			case CCImage::ETextAlign::kAlignRight: // Horizontal right and vertical top.
			    m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
				m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
				break;
 
			case CCImage::ETextAlign::kAlignBottomRight: // Horizontal right and vertical bottom.
			    m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
				m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
				break;

			case CCImage::ETextAlign::kAlignBottom: // Horizontal center and vertical bottom.
			    m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
				m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
				break;

			case CCImage::ETextAlign::kAlignBottomLeft: // Horizontal left and vertical bottom.
			    m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
				m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
				break;

			case CCImage::ETextAlign::kAlignLeft: // Horizontal left and vertical center.
			default:
			    m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
				m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
				break;
			}

			bRet = true;

		} while(0);


        return bRet;
    }

    SIZE sizeWithText(const wchar_t * pszText, int nLen, LONG nWidthLimit)
    {
        SIZE tRet = {0};
		do
		{
			CoreWindow^ window = CoreWindow::GetForCurrentThread();
			int width = (int)ConvertDipsToPixels(window->Bounds.Width);
			int height = (int)ConvertDipsToPixels(window->Bounds.Height);

            CC_BREAK_IF(! pszText || nLen <= 0);
			CCEGLView* view = CCEGLView::sharedOpenGLView();
			Microsoft::WRL::ComPtr<IDWriteFactory1> pDWriteFactory = view->getDWriteFactory2();
			IDWriteTextLayout* pTextLayout;
			HRESULT hr = pDWriteFactory->CreateTextLayout(
					pszText,      // The string to be laid out and formatted.
					nLen,  // The length of the string.
					m_pTextFormat,  // The text format to apply to the string (contains font information, etc).
					width,         // The width of the layout box.
					height,        // The height of the layout box.
					&pTextLayout  // The IDWriteTextLayout interface pointer.
			);
			CC_BREAK_IF(hr != 0);

			DWRITE_TEXT_METRICS metrics;

			hr = pTextLayout->GetMetrics(&metrics);
			CC_BREAK_IF(hr != 0);
			tRet.cx = (long)getScaledDPIValue((metrics.widthIncludingTrailingWhitespace)) + 1;
			tRet.cy = (long)getScaledDPIValue((metrics.height)) + 1;

		} while (0);

        return tRet;
    }

    bool prepareBitmap(int nWidth, int nHeight)
    {
		CCEGLView* view = CCEGLView::sharedOpenGLView();
		Microsoft::WRL::ComPtr<IWICImagingFactory2> pWICFactory = view->getWICImagingFactory2();
 
		if(m_pWICBitmap) 
		{
			m_pWICBitmap->Release();
			m_pWICBitmap = nullptr;
		}

		HRESULT hr = pWICFactory->CreateBitmap(
			nWidth,
			nHeight,
			GUID_WICPixelFormat32bppBGR,
			WICBitmapCacheOnLoad,
			&m_pWICBitmap
        );

        return hr == 0;
    }

    bool drawText(const char * pszText, SIZE& tSize, CCImage::ETextAlign eAlign)
    {
        bool nRet = false;
		ID2D1RenderTarget * pRT = nullptr;
		do
		{
            CC_BREAK_IF(! pszText);
			
			int nLen = strlen(pszText);
			int nBufLen  = nLen + 1;
			std::unique_ptr<wchar_t> pwszBuffer(new wchar_t[nBufLen]);
			CC_BREAK_IF(! pwszBuffer);
			memset(pwszBuffer.get(), 0, sizeof(wchar_t)*nBufLen);
			nLen = MultiByteToWideChar(CP_UTF8, 0, pszText, nLen, pwszBuffer.get(), nBufLen);
			
			CCEGLView* view = CCEGLView::sharedOpenGLView();
			Microsoft::WRL::ComPtr<IDWriteFactory1> pDWriteFactory = view->getDWriteFactory2();
			Microsoft::WRL::ComPtr<ID2D1Factory1> pD2DFactory = view->getD2D1Factory1();
			Microsoft::WRL::ComPtr<IWICImagingFactory2> wicFactory = view->getWICImagingFactory2();
 			CC_BREAK_IF(! pDWriteFactory);       
			CC_BREAK_IF(! pD2DFactory);       
			CC_BREAK_IF(! wicFactory);    

            SIZE newSize = sizeWithText(pwszBuffer.get(), nLen, tSize.cx);
			// if content width is 0, use text size as content size
            RECT rcText = {0};
            if (tSize.cx <= 0 || tSize.cy <= 0)
            {
                tSize = newSize;
            }
   
            CC_BREAK_IF(! prepareBitmap(tSize.cx, tSize.cy));

			// draw text
			HRESULT hr = pD2DFactory->CreateWicBitmapRenderTarget(
				m_pWICBitmap,
				D2D1::RenderTargetProperties(),
				&pRT
			);
            CC_BREAK_IF(hr);

			if(!m_pBrush)
			{
				hr = pRT->CreateSolidColorBrush(
					D2D1::ColorF(D2D1::ColorF::White),
					&m_pBrush
				);
				CC_BREAK_IF(hr);
			}

			pRT->BeginDraw();
			pRT->Clear(D2D1::ColorF(D2D1::ColorF::Black));
			pRT->DrawText(
				pwszBuffer.get(),
				nLen,
				m_pTextFormat,
				D2D1::RectF(0, 0, tSize.cx, tSize.cy),
				m_pBrush);
			pRT->EndDraw();
			nRet = true;
		} while(0);

		if(pRT) {
			pRT->Release();
		}

        return nRet;
    }

private:
    friend class CCImage;
	IDWriteTextFormat* m_pTextFormat;
	IWICBitmap* m_pWICBitmap;
	ID2D1SolidColorBrush* m_pBrush;

};

static BitmapDC& sharedBitmapDC()
{
    static BitmapDC s_BmpDC;
    return s_BmpDC;
}

static void copyPixels(unsigned char* dest, unsigned char* src, int width, int rows, int stride) {

	for(int i = 0; i < rows; i++)
	{
		memcpy((void*)&dest[i * width], (void*)&src[i * stride], width);
	}
}


bool CCImage::initWithString(
                               const char *    pText, 
                               int             nWidth/* = 0*/, 
                               int             nHeight/* = 0*/,
                               ETextAlign      eAlignMask/* = kAlignCenter*/,
                               const char *    pFontName/* = nil*/,
                               int             nSize/* = 0*/)
{
    bool bRet = false;
	wchar_t * pwszBuffer = 0;
	do 
	{
		CC_BREAK_IF(! pText);       
        BitmapDC& dc = sharedBitmapDC();
		if (! dc.setFont(pFontName, eAlignMask, nSize))
		{
			CCLog("Can't found font(%s), use system default", pFontName);
		}

       // draw text
        SIZE size = {nWidth, nHeight};
        CC_BREAK_IF(! dc.drawText(pText, size, eAlignMask));

		m_pData = new unsigned char[size.cx * size.cy * 4];
		CC_BREAK_IF(! m_pData);

        m_nWidth    = (short)size.cx;
        m_nHeight   = (short)size.cy;
        m_bHasAlpha = true;
        m_bPreMulti = false;
        m_nBitsPerComponent = 8;
		
		WICRect rcLock = { 0, 0, size.cx, size.cy };
		IWICBitmapLock *pLock = NULL;

        HRESULT hr = dc.m_pWICBitmap->Lock(&rcLock, WICBitmapLockWrite, &pLock);

        if (SUCCEEDED(hr))
        {
            UINT cbBufferSize = 0;
            UINT cbStride = 0;
            BYTE *pv = NULL;

            hr = pLock->GetStride(&cbStride);

            if (SUCCEEDED(hr))
            {
                hr = pLock->GetDataPointer(&cbBufferSize, &pv);
            }

            // copy the image data
			copyPixels(m_pData, pv, size.cx * 4, size.cy, cbStride);


            // Release the bitmap lock.
            pLock->Release();
        }

		// change pixel's alpha value to 255, when it's RGB != 0
        COLORREF * pPixel = NULL;
        for (int y = 0; y < m_nHeight; ++y)
        {
            pPixel = (COLORREF *)m_pData + y * m_nWidth;
            for (int x = 0; x < m_nWidth; ++x)
            {
                COLORREF& clr = *pPixel;
                if (GetRValue(clr) || GetGValue(clr) || GetBValue(clr))
                {
                    clr |= 0xff000000;
                }
				else {
					clr = 0;
				}
                ++pPixel;
            }
        }

        bRet = true;
    } while (0);

	




    return bRet;
}

NS_CC_END