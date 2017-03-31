#pragma once

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace System::Collections::Generic;

namespace RTMPPublisher {

	typedef struct SwsContext SWSContext;

	public class FFMPEGExtend
	{
	public:
		FFMPEGExtend();
		FFMPEGExtend(char *courseId);
		FFMPEGExtend(char *inputPath, char *outputPath);
		~FFMPEGExtend();

	public:
		void getMp3Info();
		void catchVideoStart();
		void catchVideoStop();


	private:
		void init(char *inputPath, char *outputPath);
		void dealloc();

		int start();
		void stop();
		int initInput();
		int initOutput();
		void initFrame();

		int openCodecContextJPEG();
		void closeCodecContextJPEG();

		int catchVideoFrame(int frameIndex);
		int catchJPEGFrame(int frameIndex);
		void savePacketToJPEG(AVPacket *packet, int frame);
		void saveFrameToJPEG(AVFrame *frame, int index);
		void saveFrameToPPM(AVFrame *frame, int index);

		uint8_t *getFileBuffer();

		void centerError(char *error);

	public:
		char *courseId;
		bool isWorking;

	private:
		char *inputFile;
		char *output;

		AVFormatContext	*vFormatContextInput;
		AVCodecContext	*vCodecContextInput;

		AVFormatContext	*vFormatContextOutput;
		AVCodecContext	*vCodecContextOutput;
		AVPixelFormat	 vPixelFormatOutput;

		AVFormatContext	*jpegFormatContext;
		AVCodecContext	*jpegCodecContext;

		SWSContext		*vSWSContext;
		SWSContext		*jpegSWSContext;

		AVFrame			*videoFrame;
		AVStream		*videoStream;
	};

}