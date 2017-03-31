#pragma once
#include "FFMPEGExtend.h"

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace System::Collections::Generic;

namespace RTMPPublisher {

	public ref class FFMPEGExtendManager
	{
	public:
		bool isExistExtend(char *courseId);
		bool addExtend(FFMPEGExtend *_extend);
		bool delExtend(char *courseId);
		FFMPEGExtend *getExtend(char *courseId);

		bool threadStart(FFMPEGExtend *extend);
		bool threadStop(FFMPEGExtend *extend);
	};

}