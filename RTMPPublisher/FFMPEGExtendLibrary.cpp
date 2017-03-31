// 这是主 DLL 文件。

#include "stdafx.h"

#include "FFMPEGExtendLibrary.h"
#include "FFMPEGExtendManager.h"

using namespace RTMPPublisher;
using namespace System;
using namespace System::Runtime::InteropServices;

//FFMPEGExtendManager ffmpegExtendManager;

void startCatchVideo(char *courseId)
{
	//FFMPEGExtend *extend = ffmpegExtendManager.getExtend(courseId);
	//ffmpegExtendManager.threadStart(extend);
}

void stopCatchVideo(char *courseId)
{
	//FFMPEGExtend *extend = ffmpegExtendManager.getExtend(courseId);
	//extend->catchVideoStop();
}