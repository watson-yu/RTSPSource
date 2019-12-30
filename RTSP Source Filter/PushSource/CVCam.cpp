#pragma warning(disable:4244)
#pragma warning(disable:4711)
#include "stdafx.h"

#include <streams.h>
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include "CVCam.h"

//////////////////////////////////////////////////////////////////////////
//  CVCam is the source filter which masquerades as a capture device
//////////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CVCam::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);
    CUnknown *punk = new CVCam(lpunk, phr);
    return punk;
}

CVCam::CVCam(LPUNKNOWN lpunk, HRESULT *phr) : 
    CSource(NAME("Virtual Cam"), lpunk, CLSID_VirtualCam)
{
    ASSERT(phr);
    CAutoLock cAutoLock(&m_cStateLock);
    // Create the one and only output pin
    m_paStreams = (CSourceStream **) new CVCamStream*[1];
    m_paStreams[0] = new CVCamStream(phr, this, L"Virtual Cam");

	//LPCOLESTR url = OLESTR("rtspsource://192.168.1.1/h264?w=848&h=480");
	//m_transportUrl=new TransportUrl(url);

}

HRESULT CVCam::QueryInterface(REFIID riid, void **ppv)
{
    //Forward request for IAMStreamConfig & IKsPropertySet to the pin
    if(riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
        return m_paStreams[0]->QueryInterface(riid, ppv);
    else
        return CSource::QueryInterface(riid, ppv);
}

//////////////////////////////////////////////////////////////////////////
// CVCamStream is the one and only output pin of CVCam which handles 
// all the stuff.
//////////////////////////////////////////////////////////////////////////
CVCamStream::CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName) :
    CSourceStream(NAME("Virtual Cam"),phr, pParent, pPinName), m_pParent(pParent)
{
	m_currentVideoState = NoVideo;
	m_streamMedia = NULL;

    // Set the default media type as 320x240x24@15
    GetMediaType(4, &m_mt);
}

CVCamStream::~CVCamStream()
{
} 

HRESULT CVCamStream::QueryInterface(REFIID riid, void **ppv)
{   
    // Standard OLE stuff
    if(riid == _uuidof(IAMStreamConfig))
        *ppv = (IAMStreamConfig*)this;
    else if(riid == _uuidof(IKsPropertySet))
        *ppv = (IKsPropertySet*)this;
    else
        return CSourceStream::QueryInterface(riid, ppv);

    AddRef();
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//  This is the routine where we create the data being output by the Virtual
//  Camera device.
//////////////////////////////////////////////////////////////////////////

HRESULT CVCamStream::FillBuffer(IMediaSample *pSample)
{
    TRACE_DEBUG("FillBuffer, pSample=%d", pSample->GetSize());

	if (FALSE) {
		return NO_ERROR;
	}

	bool syncPoint;
	HRESULT hr = S_OK;
    REFERENCE_TIME rtStop = 0, rtDuration = 0;
	bool rc=true;

	//if (TRUE) return NOERROR;

	CheckPointer(pSample, E_POINTER);
	//CAutoLock cAutoLockShared(&CPushPinRTSP);
	//if (TRUE) return NOERROR;

	try
	{
		// Copy the DIB bits over into our filter's output buffer.
		//This is where the magic happens, call the pipeline to fill our buffer
		rc = ProcessVideo(pSample);
		TRACE_DEBUG("processVideo rc=%d", rc);
		//if (TRUE) return NOERROR;

		if (rc)
		{
			// Set the timestamps that will govern playback frame rate.
			REFERENCE_TIME rtStop  = m_rtStart + m_framerate;
			pSample->SetTime(&m_rtStart, &rtStop);
			pSample->SetDiscontinuity(FALSE);
			m_rtStart=rtStop;
			pSample->SetSyncPoint(TRUE);
		}else{
			hr=E_FAIL;
		}
	}
	catch(...)
	{
		//TRACE_ERROR(  "--Exception---------------------");
		//TRACE_ERROR(  "FillBuffer...");
		//TRACE_ERROR(  "--------------------------------");
		
		hr=E_FAIL;//This will cause the filter to stop
	}

	return hr;
}

bool CVCamStream::ProcessVideo(IMediaSample *pSample)
{
	if (TRUE) {
		REFERENCE_TIME rtNow;
		REFERENCE_TIME avgFrameTime = ((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame;

		rtNow = m_rtLastTime;
		m_rtLastTime += avgFrameTime;
		pSample->SetTime(&rtNow, &m_rtLastTime);
		pSample->SetSyncPoint(TRUE);

		BYTE *pData;
			long lDataLen;
			pSample->GetPointer(&pData);
			lDataLen = pSample->GetSize();
			//for(int i = 0; i < lDataLen; ++i) pData[i] = rand();
			bool rc = m_streamMedia->GetFrame(pData, lDataLen);
			if (rc) m_currentVideoState = Playing;

			return TRUE;
	}

	bool rc=true;
	long cbData;
	BYTE *pData;
	//if (TRUE) return TRUE;

	// Access the sample's data buffer
	pSample->GetPointer(&pData);
	cbData = pSample->GetSize();
	long bufferSize=cbData;

	//if (TRUE) return TRUE;

	for(int i = 0; i < pSample->GetSize(); ++i)
		pData[i] = rand();
	
	rc=m_streamMedia->GetFrame(pData, bufferSize);
	TRACE_DEBUG("getFrame rc=%d", rc);

	//if (rc) return TRUE;

	if(rc)
	{
		m_lostFrameBufferCount=0;
		m_currentVideoState=Playing;
	}else{
		//paint black video to indicate a lose
		/*
		int count=((CVCam*)this->m_pFilter)->m_transportUrl->get_LostFrameCount();
		TRACE_DEBUG("lostFrameCount=%d", count);
		if(m_lostFrameBufferCount>count)
		{
			if(!(m_currentVideoState==VideoState::Lost))
			{
				//TRACE_INFO("Lost frame count (%d) over limit {%d). Paint Black Frame",m_lostFrameBufferCount, count );
				//HelpLib::TraceHelper::WriteInfo("Lost frame count over limit. Paint Black Frame and shutdown.");
				memset(pData,0, bufferSize);
				m_currentVideoState=VideoState::Lost;
				rc=true;
			} else{
				//TRACE_INFO("Shutting Down");
				rc=false;
			}
		
			
		}else{
			m_lostFrameBufferCount++;
			rc = true;
			//if(m_currentVideoState==VideoState::Lost) Sleep(1000);

		}
		*/
	}
	return rc;
}

//
// Notify
// Ignore quality management messages sent from the downstream filter
STDMETHODIMP CVCamStream::Notify(IBaseFilter * pSender, Quality q)
{
    return E_NOTIMPL;
} // Notify

//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////
HRESULT CVCamStream::SetMediaType(const CMediaType *pmt)
{
    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
    HRESULT hr = CSourceStream::SetMediaType(pmt);
    return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CVCamStream::GetMediaType(int iPosition, CMediaType *pmt)
{

    if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 8) return VFW_S_NO_MORE_ITEMS;
	TRACE_DEBUG("position=%d", iPosition);

	TransportUrl *url=((CVCam*)this->m_pFilter)->m_transportUrl;
	QueryVideo(url);

    if(iPosition == 0) 
    {
        *pmt = m_mt;
        return S_OK;
    }

    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = 24;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = 80 * iPosition;
    pvi->bmiHeader.biHeight     = 60 * iPosition;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    pvi->AvgTimePerFrame = 1000000;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);
    
    return NOERROR;

} // GetMediaType

int CVCamStream::QueryVideo(TransportUrl * url)
{
	int ret=-1;
	TRACE_DEBUG("videoState=%d", m_currentVideoState);

	if(!(m_currentVideoState==VideoState::NoVideo||m_currentVideoState==VideoState::Lost)) 
	{
		TRACE_INFO("Video already configured, exiting");
		return 0;
	}
		//if (True) return 0;

	//if(!url->hasUrl())
	//{
		//TRACE_INFO("Missing the URL");
		LPCOLESTR str = OLESTR("rtspsource://192.168.1.1/h264?w=640&h=360&Width=640&Height=360&fps=30&br=400000");
		url = new TransportUrl(str);
		((CVCam*)this->m_pFilter)->m_transportUrl = url;

		TRACE_DEBUG("RTSP URL: %s", url->get_RtspUrl());
		//if (! url->hasUrl()) return 0;//ret;
	//}
	//if (True) return 0;

	try
	{
		//TRACE_INFO("Try to open the RTSP video stream");
		if (m_streamMedia==NULL)
			m_streamMedia=new CstreamMedia();
		TRACE_DEBUG("CstreamMedia=%d", m_streamMedia);

		TRACE_DEBUG("before rtspClientOpenStream");
		ret = m_streamMedia->rtspClientOpenStream((const char *)url->get_RtspUrl());
		TRACE_DEBUG("OpenStream result=%d", ret);
		if (ret < 0)
		{
			//TRACE_ERROR( "Unable to open rtsp video stream ret=%d", ret);
			return E_FAIL;
		}
		//if (m_currentVideoState != Playing)
		//	m_streamMedia->rtspClientPlayStream(url->get_RtspUrl());
		//m_currentVideoState = Playing;
	}
	catch(...)
	{
		//TRACE_CRITICAL( "QueryVideo Failed");
		m_currentVideoState=VideoState::NoVideo;
		throw false;
	}
	//if (True) return 0;

	//attempt to get the media info from the stream
	//we know that in 7.2 this does not work, but we are
	//hoping that 7.5 will enable width and height
	MediaInfo videoMediaInfo;
	try{
		//TRACE_INFO("Get Media Info");
		ret= m_streamMedia->rtspClientGetMediaInfo(CODEC_TYPE_VIDEO, videoMediaInfo);
		if(ret < 0)
		{	
			//TRACE_CRITICAL( "Unable to get media info from RTSP stream.  ret=%d (url=%s)", ret,url->get_Url());
			return VFW_S_NO_MORE_ITEMS;
		}
	}
	catch(...)
	{
		//TRACE_CRITICAL( "QueryVideo Failed from ");
		m_currentVideoState=VideoState::NoVideo;
		throw false;
	}
	//if (True) return 0;

	//TRACE_INFO( "Format: %d",videoMediaInfo.i_format);
	//TRACE_INFO( "Codec: %s",videoMediaInfo.codecName);
	if(videoMediaInfo.video.width>0)
	{
		//TRACE_INFO( "Using video information directly from the stream");
		m_videoWidth = videoMediaInfo.video.width;
		m_videoHeight = videoMediaInfo.video.height;
		m_bitCount = videoMediaInfo.video.bitrate;
		if(videoMediaInfo.video.fps>0)
			m_framerate=(REFERENCE_TIME)(10000000/videoMediaInfo.video.fps);
	}else{
		//TRACE_WARN( "No video info in stream, using defaults from url");
		m_videoWidth = url->get_Width();
		m_videoHeight = url->get_Height();
		//m_videoWidth = 352;
		//m_videoHeight = 240;
		m_bitCount = 1;
		if(url->get_Framerate()>0)
			m_framerate=(REFERENCE_TIME)(10000000/url->get_Framerate());
	}

	//TRACE_INFO( "Width: %d",m_videoWidth);
	//TRACE_INFO( "Height: %d",m_videoHeight);
	//TRACE_INFO( "FPS: %d",10000000/m_framerate);
	//TRACE_INFO( "Bitrate: %d",m_bitCount);
	m_currentVideoState=VideoState::Reloading;
		
	return ret;
}

// This method is called to see if a given output format is supported
HRESULT CVCamStream::CheckMediaType(const CMediaType *pMediaType)
{
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pMediaType->Format());
    if(*pMediaType != m_mt) 
        return E_INVALIDARG;
    return S_OK;
} // CheckMediaType

// This method is called after the pins are connected to allocate buffers to stream data
HRESULT CVCamStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);

    if(FAILED(hr)) return hr;
    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;

    return NOERROR;
} // DecideBufferSize

// Called when graph is run
HRESULT CVCamStream::OnThreadCreate()
{
    m_rtLastTime = 0;
	//return NOERROR;

	HRESULT hr = S_OK;
	hr = CSourceStream::OnThreadCreate();
	TransportUrl *url = ((CVCam*)this->m_pFilter)->m_transportUrl;

	TRACE_INFO("Open Stream");
	if (m_streamMedia == NULL) m_streamMedia = new CstreamMedia();
	int ret = m_streamMedia->rtspClientOpenStream(url->get_RtspUrl());

	if (ret != 0) {
		TRACE_INFO("Unable to open stream ret=%d", ret);
		hr = E_FAIL;
	}

	TRACE_INFO("OnThreadCreate. HRESULT = %#x", hr);
	return hr;
} // OnThreadCreate

/**
* FillBuffer is about to get called for the first time
* do any prep work here
*/
HRESULT CVCamStream::OnThreadStartPlay(void)
{
	TRACE_INFO("OnThreadStartPlay");

	HRESULT hr = S_OK;
	hr = CSourceStream::OnThreadStartPlay();
	TRACE_INFO("Play Stream");
	TransportUrl *url=((CVCam*)this->m_pFilter)->m_transportUrl;
	int ret = m_streamMedia->rtspClientPlayStream(url->get_RtspUrl());
	if (ret != 0) {
		TRACE_ERROR("Unable to play stream ret=%d", ret);
		hr=E_FAIL;
	}

	TRACE_DEBUG( "OnThreadStartPlay. HRESULT = %#x", hr);
	return hr;
}

//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CVCamStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
    DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.pbFormat);
    m_mt = *pmt;
    IPin* pin; 
    ConnectedTo(&pin);
    if(pin)
    {
        IFilterGraph *pGraph = m_pParent->GetGraph();
        pGraph->Reconnect(this);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    *ppmt = CreateMediaType(&m_mt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    *piCount = 8;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
    *pmt = CreateMediaType(&m_mt);
    DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);

    if (iIndex == 0) iIndex = 4;

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = 24;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = 80 * iIndex;
    pvi->bmiHeader.biHeight     = 60 * iIndex;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    (*pmt)->majortype = MEDIATYPE_Video;
    (*pmt)->subtype = MEDIASUBTYPE_RGB24;
    (*pmt)->formattype = FORMAT_VideoInfo;
    (*pmt)->bTemporalCompression = FALSE;
    (*pmt)->bFixedSizeSamples= FALSE;
    (*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
    (*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);
    
    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);
    
    pvscc->guid = FORMAT_VideoInfo;
    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = 640;
    pvscc->InputSize.cy = 360;
    pvscc->MinCroppingSize.cx = 80;
    pvscc->MinCroppingSize.cy = 45;
    pvscc->MaxCroppingSize.cx = 640;
    pvscc->MaxCroppingSize.cy = 360;
    pvscc->CropGranularityX = 80;
    pvscc->CropGranularityY = 45;
    pvscc->CropAlignX = 0;
    pvscc->CropAlignY = 0;

    pvscc->MinOutputSize.cx = 80;
    pvscc->MinOutputSize.cy = 45;
    pvscc->MaxOutputSize.cx = 640;
    pvscc->MaxOutputSize.cy = 360;
    pvscc->OutputGranularityX = 0;
    pvscc->OutputGranularityY = 0;
    pvscc->StretchTapsX = 0;
    pvscc->StretchTapsY = 0;
    pvscc->ShrinkTapsX = 0;
    pvscc->ShrinkTapsY = 0;
    pvscc->MinFrameInterval = 200000;   //50 fps
    pvscc->MaxFrameInterval = 50000000; // 0.2 fps
    pvscc->MinBitsPerSecond = (80 * 45 * 3 * 8) / 5;
    pvscc->MaxBitsPerSecond = 640 * 360 * 3 * 8 * 50;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////


HRESULT CVCamStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, 
                        DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
    return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CVCamStream::Get(
    REFGUID guidPropSet,   // Which property set.
    DWORD dwPropID,        // Which property in that set.
    void *pInstanceData,   // Instance data (ignore).
    DWORD cbInstanceData,  // Size of the instance data (ignore).
    void *pPropData,       // Buffer to receive the property data.
    DWORD cbPropData,      // Size of the buffer.
    DWORD *pcbReturned     // Return the size of the property.
)
{
    if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
    if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;
    
    if (pcbReturned) *pcbReturned = sizeof(GUID);
    if (pPropData == NULL)          return S_OK; // Caller just wants to know the size. 
    if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;// The buffer is too small.
        
    *(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
    return S_OK;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT CVCamStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
    // We support getting this property, but not setting it.
    if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET; 
    return S_OK;
}
