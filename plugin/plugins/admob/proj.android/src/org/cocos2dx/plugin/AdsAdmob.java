/****************************************************************************
Copyright (c) 2012-2013 cocos2d-x.org

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
package org.cocos2dx.plugin;

import java.util.HashSet;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Set;

import org.cocos2dx.plugin.InterfaceAds.AdsAdapter;

import com.google.ads.*;
import com.google.ads.AdRequest.ErrorCode;

import android.app.Activity;
import android.content.Context;
import android.util.Log;
import android.view.WindowManager;

public class AdsAdmob implements AdsAdapter {

	private static final String LOG_TAG = "AdsAdmob";
	private static Activity mContext = null;
	private static boolean bDebug = false;
	private static AdsAdmob mAdapter = null;

	private AdView adView = null;
	private String mPublishID = "";
	private Set<String> mTestDevices = null;
	private WindowManager mWm = null;

	private static final int ADMOB_SIZE_BANNER = 0;
	private static final int ADMOB_SIZE_IABMRect = 1;
	private static final int ADMOB_SIZE_IABBanner = 2;
	private static final int ADMOB_SIZE_IABLeaderboard = 3;

	protected static void LogE(String msg, Exception e) {
		Log.e(LOG_TAG, msg, e);
		e.printStackTrace();
	}

	protected static void LogD(String msg) {
		if (bDebug) {
			Log.d(LOG_TAG, msg);
		}
	}

	public AdsAdmob(Context context) {
		mContext = (Activity) context;
		mAdapter = this;
	}

	@Override
	public void setDebugMode(boolean debug) {
		bDebug = debug;
	}

	@Override
	public String getSDKVersion() {
		return "6.3.1";
	}

	@Override
	public void configDeveloperInfo(Hashtable<String, String> devInfo) {
		try {
			mPublishID = devInfo.get("AdmobID");
			LogD("init AppInfo : " + mPublishID);
		} catch (Exception e) {
			LogE("initAppInfo, The format of appInfo is wrong", e);
		}
	}

	@Override
	public void showAds(int adsType, int sizeEnum, int pos) {
		switch (adsType) {
		case InterfaceAds.ADS_TYPE_BANNER:
			showBannerAd(sizeEnum, pos);
			break;
		case InterfaceAds.ADS_TYPE_FULL_SCREEN:
			LogD("Now not support full screen view in Admob");
			break;
		default:
			break;
		}
	}

	@Override
	public void spendPoints(int points) {
		// do nothing, Admob don't have this function
	}

	@Override
	public void hideAds(int adsType) {
		switch (adsType) {
		case InterfaceAds.ADS_TYPE_BANNER:
			hideBannerAd();
			break;
		case InterfaceAds.ADS_TYPE_FULL_SCREEN:
			break;
		default:
			break;
		}
	}

	private void showBannerAd(int sizeEnum, int pos) {
		final int curPos = pos;
		final int curSize = sizeEnum;

		PluginWrapper.runOnMainThread(new Runnable() {

			@Override
			public void run() {
				// destory the ad view before
				if (null != adView) {
					if (null != mWm) {
						mWm.removeView(adView);
					}
					adView.destroy();
					adView = null;
				}

				AdSize size = AdSize.BANNER;
				switch (curSize) {
				case AdsAdmob.ADMOB_SIZE_BANNER:
					size = AdSize.BANNER;
					break;
				case AdsAdmob.ADMOB_SIZE_IABMRect:
					size = AdSize.IAB_MRECT;
					break;
				case AdsAdmob.ADMOB_SIZE_IABBanner:
					size = AdSize.IAB_BANNER;
					break;
				case AdsAdmob.ADMOB_SIZE_IABLeaderboard:
					size = AdSize.IAB_LEADERBOARD;
					break;
				default:
					break;
				}
				adView = new AdView(mContext, size, mPublishID);
				AdRequest req = new AdRequest();
				
				try {
					if (mTestDevices != null) {
						Iterator<String> ir = mTestDevices.iterator();
						while(ir.hasNext())
						{
							req.addTestDevice(ir.next());
						}
					}
				} catch (Exception e) {
					LogE("Error during add test device", e);
				}
				
				adView.loadAd(req);
				adView.setAdListener(new AdmobAdsListener());

				if (null == mWm) {
					mWm = (WindowManager) mContext.getSystemService("window");
				}
				InterfaceAds.addAdView(mWm, adView, curPos);
			}
		});
	}

	private void hideBannerAd() {
		PluginWrapper.runOnMainThread(new Runnable() {
			@Override
			public void run() {
				if (null != adView) {
					if (null != mWm) {
						mWm.removeView(adView);
					}
					adView.destroy();
					adView = null;
				}
			}
		});
	}

	public void addTestDevice(String deviceID) {
		LogD("addTestDevice invoked : " + deviceID);
		if (null == mTestDevices) {
			mTestDevices = new HashSet<String>();
		}
		mTestDevices.add(deviceID);
	}

	private class AdmobAdsListener implements AdListener {

		@Override
		public void onDismissScreen(Ad arg0) {
			LogD("onDismissScreen invoked");
			InterfaceAds.onAdsResult(mAdapter, InterfaceAds.RESULT_CODE_FullScreenViewDismissed, "Full screen ads view dismissed!");
		}

		@Override
		public void onFailedToReceiveAd(Ad arg0, ErrorCode arg1) {
			int errorNo = InterfaceAds.RESULT_CODE_UnknownError;
			String errorMsg = "Unknow error";
			switch (arg1) {
			case NETWORK_ERROR:
				errorNo =  InterfaceAds.RESULT_CODE_NetworkError;
				errorMsg = "Network error";
				break;
			case INVALID_REQUEST:
				errorNo = InterfaceAds.RESULT_CODE_NetworkError;
				errorMsg = "The ad request is invalid";
				break;
			case NO_FILL:
				errorMsg = "The ad request is successful, but no ad was returned due to lack of ad inventory.";
				break;
			default:
				break;
			}
			LogD("failed to receive ad : " + errorNo + " , " + errorMsg);
			InterfaceAds.onAdsResult(mAdapter, errorNo, errorMsg);
		}

		@Override
		public void onLeaveApplication(Ad arg0) {
			LogD("onLeaveApplication invoked");
		}

		@Override
		public void onPresentScreen(Ad arg0) {
			LogD("onPresentScreen invoked");
			InterfaceAds.onAdsResult(mAdapter, InterfaceAds.RESULT_CODE_FullScreenViewShown, "Full screen ads view shown!");
		}

		@Override
		public void onReceiveAd(Ad arg0) {
			LogD("onReceiveAd invoked");
			InterfaceAds.onAdsResult(mAdapter, InterfaceAds.RESULT_CODE_AdsReceived, "Ads request received success!");
		}
	}
}
