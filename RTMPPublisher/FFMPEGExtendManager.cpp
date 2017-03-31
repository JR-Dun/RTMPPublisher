
#include "stdafx.h"
#include "FFMPEGExtendManager.h"

using namespace RTMPPublisher;
using namespace System;
using namespace System::Runtime::InteropServices;

//并发线程数
const int currentThread = 10;
FFMPEGExtend *array[currentThread];

bool FFMPEGExtendManager::isExistExtend(char *courseId)
{
	bool result = false;
	for (int i = 0; i < currentThread; i++)
	{
		FFMPEGExtend *extend = array[i];

		if (extend != NULL && strcmp(courseId, extend->courseId) == 0)
		{
			result = true;
			break;
		}
	}
	return result;
}

bool FFMPEGExtendManager::addExtend(FFMPEGExtend *_extend)
{
	if (isExistExtend(_extend->courseId)) return false;

	bool result = false;
	for (int i = 0; i < currentThread; i++)
	{
		FFMPEGExtend *extend = array[i];

		if (extend == NULL)
		{
			array[i] = _extend;
			result = true;
			break;
		}
	}
	return result;
}

bool FFMPEGExtendManager::delExtend(char *courseId)
{
	bool result = false;
	for (int i = 0; i < currentThread; i++)
	{
		FFMPEGExtend *extend = array[i];

		if (extend != NULL && strcmp(courseId, extend->courseId) == 0)
		{
			extend = NULL;
			result = true;
			break;
		}
	}
	return result;
}

FFMPEGExtend *FFMPEGExtendManager::getExtend(char *courseId)
{
	for (int i = 0; i < currentThread; i++)
	{
		FFMPEGExtend *extend = array[i];

		if (extend != NULL && strcmp(courseId, extend->courseId) == 0)
		{
			return extend;
		}
	}

	FFMPEGExtend *extend = new FFMPEGExtend(courseId);
	if (addExtend(extend))
	{
		return extend;
	}

	return NULL;
}


DWORD WINAPI runThread(LPVOID lpParameter)
{
	FFMPEGExtend *extend = (FFMPEGExtend *)lpParameter;
	extend->catchVideoStart();
	return 0;
}

bool FFMPEGExtendManager::threadStart(FFMPEGExtend *extend)
{
	HANDLE handle = CreateThread(NULL, 0, runThread, extend, 0, NULL);
	//只是关闭了一个线程句柄对象，表示我不再使用该句柄，即不对这个句柄对应的线程做任何干预了。并没有结束线程。
	CloseHandle(handle);
	return true;
}

bool FFMPEGExtendManager::threadStop(FFMPEGExtend *extend)
{
	extend->catchVideoStop();
	return true;
}
