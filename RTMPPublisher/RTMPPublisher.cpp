// RTMPPublisher.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "ffmpegExtend.h"


int main()
{
	FFMPEGExtend *extend = new FFMPEGExtend("temp\\course_demo.jpg");
	extend->getMp3Info();
	extend->catchVideoStart();
    return 0;
}

