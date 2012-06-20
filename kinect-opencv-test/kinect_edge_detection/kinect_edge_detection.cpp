#include "stdafx.h"

#include <Windows.h>
#include "libkinect/inc/NuiApi.h"

#include "opencv2\opencv.hpp"
#include "opencv2\highgui\highgui.hpp"
#include "opencv2\imgproc\imgproc.hpp"

#pragma comment(lib, "libkinect/lib/x86/Kinect10.lib")
#pragma comment(lib, "opencv_core240.lib")
#pragma comment(lib, "opencv_imgproc240.lib")
#pragma comment(lib, "opencv_highgui240.lib")

DWORD WINAPI RGBImage( LPVOID lpParam );
DWORD WINAPI DepthImage( LPVOID lpParam );
RGBQUAD Nui_ShortToQuad( unsigned short s );

IplImage *cvRGBImage = cvCreateImage( cvSize( 640, 480 ), IPL_DEPTH_8U, 4 );
IplImage *cvDepthImage = cvCreateImage( cvSize( 320, 240 ), IPL_DEPTH_8U, 4 );
IplImage *DepthEdges = cvCreateImage( cvSize( 320, 240 ), IPL_DEPTH_8U, 1 );

IplImage *DepthEdges2 = cvCreateImage( cvSize( 320, 240 ), IPL_DEPTH_8U, 1 );

RGBQUAD	RGBDepth[ 320 * 240 ];

HANDLE hEvents[2] = { CreateEvent( NULL, TRUE, FALSE, NULL ), CreateEvent( NULL, TRUE, FALSE, NULL ) };
HANDLE hStreams[2] = { NULL, NULL };

int main(int argc, char* argv[])
{
	NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_SKELETON );

	NuiImageStreamOpen( NUI_IMAGE_TYPE::NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION::NUI_IMAGE_RESOLUTION_640x480, NULL, 2, hEvents[0], &hStreams[0] );
	NuiImageStreamOpen( NUI_IMAGE_TYPE::NUI_IMAGE_TYPE_DEPTH, NUI_IMAGE_RESOLUTION::NUI_IMAGE_RESOLUTION_320x240, NULL, 2, hEvents[1], &hStreams[1] );

	HANDLE hThreads[2] = {
		CreateThread( NULL, 0, RGBImage, NULL, NULL, NULL),
		CreateThread( NULL, 0, DepthImage, NULL, NULL, NULL)
	};

	WaitForMultipleObjects( 2, hThreads, FALSE, INFINITE );

	cvDestroyAllWindows();
	NuiShutdown();

	return 0;
}

DWORD WINAPI RGBImage( LPVOID lpParam )
{
	cvNamedWindow( "RGB" );

	while ( true )
	{
		WaitForSingleObject( hEvents[0], INFINITE);
		printf( "[+] Got RGB Frame!\n" );

		const NUI_IMAGE_FRAME *RGBFrame;
		NuiImageStreamGetNextFrame( hStreams[0], NULL, &RGBFrame );

		NUI_LOCKED_RECT lockedRGB;
		RGBFrame->pFrameTexture->LockRect( NULL, &lockedRGB, NULL, NULL );
		
		cvSetData( cvRGBImage, (unsigned char*)lockedRGB.pBits, lockedRGB.Pitch );
		cvShowImage( "RGB", cvRGBImage );
		if ( cvWaitKey( 1 ) == 'x' ){ break; }

		NuiImageStreamReleaseFrame( hStreams[0], RGBFrame );
	}

	return 0;
}

DWORD WINAPI DepthImage( LPVOID lpParam )
{
	cvNamedWindow( "Depth" );

	while ( true )
	{
		WaitForSingleObject( hEvents[1], INFINITE);
		printf( "[+] Got Depth Frame!\n" );

		const NUI_IMAGE_FRAME *DepthFrame;
		NuiImageStreamGetNextFrame( hStreams[1], NULL, &DepthFrame );
		
		NUI_LOCKED_RECT lockedDepth;
		DepthFrame->pFrameTexture->LockRect( NULL, &lockedDepth, NULL, NULL );
		
		unsigned char* Buffer = (unsigned char*) lockedDepth.pBits;
		
		RGBQUAD* rgbLoop =  RGBDepth;
		unsigned short* BufferLoop = (unsigned short*) Buffer;

		for ( int y = 0; y < 240; y++ )
		{
			for ( int x = 0; x < 320; x++ )
			{
				RGBQUAD Quad = Nui_ShortToQuad( *BufferLoop );
				BufferLoop++;

				*rgbLoop = Quad;
				rgbLoop++;
			}
		}

		cvSetData( cvDepthImage, (unsigned char*) RGBDepth, cvDepthImage->widthStep );
		//cvCvtColor( cvDepthImage, DepthEdges, CV_RGB2GRAY);
		cvCanny(  cvDepthImage, DepthEdges2, 100, 100, 3 );
		
		cvShowImage( "Depth",  /*cvDepthImage*/ DepthEdges2 );
		if ( cvWaitKey( 1 ) == 'x' ){ break; }

		NuiImageStreamReleaseFrame( hStreams[1], DepthFrame );
	}

	return 0;
}

RGBQUAD Nui_ShortToQuad( unsigned short s )
{
    USHORT RealDepth = s & 0xffff;
    USHORT Player =  0;

    // transform 13-bit depth information into an 8-bit intensity appropriate
    // for display (we disregard information in most significant bit)
    BYTE l = 255 - (BYTE)(256*RealDepth/0x0fff);

    RGBQUAD q;
    q.rgbRed = q.rgbBlue = q.rgbGreen = 0;

    switch( Player )
    {
    case 0:
        q.rgbRed = l / 2;
        q.rgbBlue = l / 2;
        q.rgbGreen = l / 2;
        break;
    case 1:
        q.rgbRed = l;
        break;
    case 2:
        q.rgbGreen = l;
        break;
    case 3:
        q.rgbRed = l / 4;
        q.rgbGreen = l;
        q.rgbBlue = l;
        break;
    case 4:
        q.rgbRed = l;
        q.rgbGreen = l;
        q.rgbBlue = l / 4;
        break;
    case 5:
        q.rgbRed = l;
        q.rgbGreen = l / 4;
        q.rgbBlue = l;
        break;
    case 6:
        q.rgbRed = l / 2;
        q.rgbGreen = l / 2;
        q.rgbBlue = l;
        break;
    case 7:
        q.rgbRed = 255 - ( l / 2 );
        q.rgbGreen = 255 - ( l / 2 );
        q.rgbBlue = 255 - ( l / 2 );
    }

    return q;
}