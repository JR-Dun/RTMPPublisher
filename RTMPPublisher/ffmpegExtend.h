#pragma once

typedef struct SwsContext SWSContext;

class FFMPEGExtend
{
public:
	FFMPEGExtend(char *inputFilePath);
	~FFMPEGExtend();

public:
	void getMp3Info();
	void catchVideoStart();
	void catchVideoStop();


private:
	void init(char *inputFilePath);
	void dealloc();

	void start();
	void stop();
	void initInput();
	void initOutput();
	void initFrame();

	void openCodecContextJPEG();
	void closeCodecContextJPEG();

	void catchVideoFrame(int frameIndex);
	void catchJPEGFrame(int frameIndex);
	void savePacketToJPEG(AVPacket *packet, int frame);
	void saveFrameToJPEG(AVFrame *frame, int index);
	void saveFrameToPPM(AVFrame *frame, int index);

	uint8_t *getFileBuffer();

public:
	bool isWorking;

private:
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
	AVStream			*videoStream;
};