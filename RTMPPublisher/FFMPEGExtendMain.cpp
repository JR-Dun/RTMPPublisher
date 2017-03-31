#include "stdafx.h"
#include "FFMPEGExtendManager.h"

using namespace RTMPPublisher;
using namespace System;
using namespace System::Runtime::InteropServices;



void main()
{
	FFMPEGExtendManager ffmpegExtendManager;

	FFMPEGExtend *extend1 = ffmpegExtendManager.getExtend("10086");
	FFMPEGExtend *extend3 = ffmpegExtendManager.getExtend("10086");
	ffmpegExtendManager.threadStart(extend1);

	while (true)
	{
		if (kbhit()) {
			extend3->catchVideoStop();
		};
	}
}