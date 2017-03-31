#include "stdafx.h"
#include "FFMPEGExtend.h"

using namespace RTMPPublisher;
using namespace System;
using namespace System::Runtime::InteropServices;


#define MAX_NAME_LEN 1024
#define CAP_SCREEN	1
#define CAP_CAMERA	2
#define CAP_JPEG	3
#define CAP_DEVICE	CAP_SCREEN

char *inputCamera = "video=Lenovo EasyCamera";	// 摄像头

// 输出方式
//1:RTMP推流				eg："rtmp://120.92.3.155:1935/live/stream1"
//2:保存为本地视频文件	eg："C:\\JR_Dun\\git\\RTMPPublisher\\Debug\\stream.mp4"
//char *output = "rtmp://120.92.3.155:1935/live/stream1";

char *outputDomain = "rtmp://120.92.3.155:1935/live/";//也可以是本地文件路径

const int fps = 20;			// 视频输出FPS
const int width = 800;		// 视频width
const int height = 600;		// 视频height

FFMPEGExtend::FFMPEGExtend()
{
	//inputFile = "temp\\error.jpg";
	//output = "rtmp://120.92.3.155:1935/live/stream1"
}

FFMPEGExtend::FFMPEGExtend(char *_courseId)
{
	courseId = _courseId;
	char *inputPath = new char[255];
	char *outputPath = new char[255];
	sprintf_s(inputPath, 255, "%s%s.jpg", "temp\\", courseId);
	sprintf_s(outputPath, 255, "%s%s", outputDomain, courseId);

	isWorking = false;
	init(inputPath, outputPath);
}

FFMPEGExtend::FFMPEGExtend(char *inputPath, char *outputPath)
{
	isWorking = false;
	init(inputPath, outputPath);
}

FFMPEGExtend::~FFMPEGExtend()
{
	dealloc();
}

void FFMPEGExtend::getMp3Info()
{
	AVFormatContext *fmt_ctx = NULL;
	AVDictionaryEntry *tag = NULL;
	int ret = 0;

	av_register_all();
	if ((ret = avformat_open_input(&fmt_ctx, "test.mp3", NULL, NULL)))
	{
		return;
	}

	while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
	{
		printf("%s=%s\n", tag->key, tag->value);
	}

	avformat_close_input(&fmt_ctx);

	system("pause");
}

void FFMPEGExtend::init(char *inputPath, char *outputPath)
{
	if (inputPath != NULL && inputPath[0] != '\0' && sizeof(inputPath) > 0)
	{
		inputFile = inputPath;
	}

	if (outputPath != NULL && outputPath[0] != '\0' && sizeof(outputPath) > 0)
	{
		output = outputPath;
	}
}

int FFMPEGExtend::start()
{
	vPixelFormatOutput = AV_PIX_FMT_YUV420P;

	av_register_all();
	avformat_network_init();
	avdevice_register_all();

	if (initInput() < 0)
	{
#if(CAP_DEVICE == CAP_JPEG)
		avformat_close_input(&jpegFormatContext);
		av_free(jpegFormatContext);
#else
		sws_freeContext(vSWSContext);
		avformat_close_input(&vFormatContextInput);
		av_free(vFormatContextInput);
#endif
		return -1;
	}

	initFrame();

	if (initOutput() < 0)
	{
		av_frame_free(&videoFrame);

		avcodec_close(vCodecContextOutput);
		avio_close(vFormatContextOutput->pb);
		avformat_free_context(vFormatContextOutput);
		return -1;
	}

	return 0;
}

void FFMPEGExtend::dealloc()
{
}

void FFMPEGExtend::stop()
{
	av_write_trailer(vFormatContextOutput);

	av_frame_free(&videoFrame);

	avcodec_close(vCodecContextOutput);
	avio_close(vFormatContextOutput->pb);
	avformat_free_context(vFormatContextOutput);

#if(CAP_DEVICE == CAP_JPEG)
	avformat_close_input(&jpegFormatContext);
	av_free(jpegFormatContext);
#else
	sws_freeContext(vSWSContext);
	avformat_close_input(&vFormatContextInput);
	av_free(vFormatContextInput);
#endif

}

int FFMPEGExtend::initInput()
{
#if(CAP_DEVICE == CAP_JPEG)

	jpegFormatContext = avformat_alloc_context();

#else
	vFormatContextInput = avformat_alloc_context();

	AVDictionary *vOptionsInput = NULL;
	char c_fps[8] = { 0 };
	itoa(fps, c_fps, 10);
	av_dict_set(&vOptionsInput, "frame_rate", c_fps, 0);
#if(CAP_DEVICE == CAP_SCREEN) // 录制屏幕
	av_dict_set(&vOptionsInput, "offset_x", "0", 0);
	av_dict_set(&vOptionsInput, "offset_y", "0", 0);
	av_dict_set(&vOptionsInput, "video_size", "800x600", 0);

	AVInputFormat *inputFormat = av_find_input_format("gdigrab");
	avformat_open_input(&vFormatContextInput, "desktop", inputFormat, NULL);
#elif(CAP_DEVICE == CAP_CAMERA) // 摄像头
	inputFormat = av_find_input_format("dshow");
	avformat_open_input(&vFormatContextInput, inputCamera, inputFormat, NULL);
#endif

	avformat_find_stream_info(vFormatContextInput, NULL);

	int videoIndex = -1;
	for (int i = 0; i < vFormatContextInput->nb_streams; ++i)
	{
		if (vFormatContextInput->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoIndex = i;
			break;
		}
	}

	vCodecContextInput = vFormatContextInput->streams[videoIndex]->codec;
	AVCodec *vCodecInput = avcodec_find_decoder(vCodecContextInput->codec_id);
	if (avcodec_open2(vCodecContextInput, vCodecInput, NULL) != 0)
	{
		centerError("initInput avcodec_open2 vCodecContextInput");
		return -1;
	}

	vSWSContext = sws_getContext(width, height, vCodecContextInput->pix_fmt,
		width, height, vPixelFormatOutput, SWS_BILINEAR, NULL, NULL, NULL);

#endif

	return 0;
}

int FFMPEGExtend::initOutput()
{
	char URL[MAX_NAME_LEN] = { 0 };
	strcpy(URL, output);

	vFormatContextOutput = NULL;
	if (avformat_alloc_output_context2(&vFormatContextOutput, NULL, "flv", URL) < 0)
	{
		centerError("initOutput avformat_alloc_output_context2 vFormatContextOutput");
		return -1;
	}

	if (avio_open(&(vFormatContextOutput->pb), URL, AVIO_FLAG_WRITE) < 0)
	{
		centerError("initOutput avio_open vFormatContextOutput");
		return -1;
	}

	videoStream = avformat_new_stream(vFormatContextOutput, NULL);

	vCodecContextOutput = videoStream->codec;
	vCodecContextOutput->codec_id = vFormatContextOutput->oformat->video_codec;
	vCodecContextOutput->codec_type = AVMEDIA_TYPE_VIDEO;
	vCodecContextOutput->pix_fmt = vPixelFormatOutput;
	vCodecContextOutput->width = width;
	vCodecContextOutput->height = height;
	vCodecContextOutput->bit_rate = 1 * 1024 * 1024;
	vCodecContextOutput->gop_size = 250;
	vCodecContextOutput->time_base.num = 1;
	vCodecContextOutput->time_base.den = fps;
	vCodecContextOutput->qmin = 10; //?
	vCodecContextOutput->qmax = 51; //?
	//vCodecContextOutput->max_b_frames = 3; //FLV不支持

	AVDictionary *vOptionsOutput = NULL;

	//H.264  
	if (vCodecContextOutput->codec_id == AV_CODEC_ID_H264)
	{
		av_dict_set(&vOptionsOutput, "preset", "slow", 0);
		av_dict_set(&vOptionsOutput, "tune", "zerolatency", 0);
		//av_dict_set(&voutOptions, "profile", "main", 0);  
	}
	//H.265  
	if (vCodecContextOutput->codec_id == AV_CODEC_ID_H265)
	{
		av_dict_set(&vOptionsOutput, "preset", "ultrafast", 0);
		av_dict_set(&vOptionsOutput, "tune", "zero-latency", 0);
	}

	AVCodec *vCodecOutput = avcodec_find_encoder(vCodecContextOutput->codec_id);
	if (avcodec_open2(vCodecContextOutput, vCodecOutput, &vOptionsOutput) != 0)
	{
		centerError("initOutput avcodec_open2 vCodecContextOutput");
		return -1;
	}

	avformat_write_header(vFormatContextOutput, NULL);

	return 0;
}

int FFMPEGExtend::openCodecContextJPEG()
{
	AVDictionary *vOptionsInput = NULL;
	char c_fps[8] = { 0 };
	itoa(fps, c_fps, 10);
	if (av_dict_set(&vOptionsInput, "frame_rate", c_fps, 0) < 0)
	{
		centerError("openCodecContextJPEG av_dict_set vOptionsInput");
		return -1;
	}
	if (avformat_open_input(&jpegFormatContext, inputFile, NULL, NULL) != 0)
	{
		centerError("openCodecContextJPEG avformat_open_input jpegFormatContext");
		return -1;
	}
	if (avformat_find_stream_info(jpegFormatContext, NULL) < 0)
	{
		centerError("openCodecContextJPEG avformat_find_stream_info jpegFormatContext");
		return -1;
	}

	int videoIndex = -1;
	for (int i = 0; i < jpegFormatContext->nb_streams; ++i)
	{
		if (jpegFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoIndex = i;
			break;
		}
	}

	jpegCodecContext = jpegFormatContext->streams[videoIndex]->codec;
	AVCodec *vCodecInput = avcodec_find_decoder(jpegCodecContext->codec_id);
	if (avcodec_open2(jpegCodecContext, vCodecInput, NULL) != 0)
	{
		centerError("openCodecContextJPEG avcodec_open2 jpegCodecContext");
		return -1;
	}

	jpegSWSContext = sws_getContext(width, height, jpegCodecContext->pix_fmt,
		width, height, vPixelFormatOutput, SWS_BILINEAR, NULL, NULL, NULL);

	return 0;
}

void FFMPEGExtend::closeCodecContextJPEG()
{
	sws_freeContext(jpegSWSContext);
	avcodec_close(jpegCodecContext);
	avformat_close_input(&jpegFormatContext);
}

void FFMPEGExtend::initFrame()
{
	int buffLen = avpicture_get_size(vPixelFormatOutput, width, height);
	uint8_t* buffer = (uint8_t *)av_malloc(buffLen);

	videoFrame = av_frame_alloc();
	avpicture_fill((AVPicture*)videoFrame, buffer, vPixelFormatOutput, width, height);
}

int FFMPEGExtend::catchVideoFrame(int frameIndex)
{
	AVPacket* packetIn = av_packet_alloc(); // ALLOC_PACKET_IN
	AVFrame* frameIn = av_frame_alloc();    // ALLOC_FRAME_IN

	if (av_read_frame(vFormatContextInput, packetIn) < 0)
	{
		centerError("catchVideoFrame av_read_frame vFormatContextInput");
		return -1;
	}

	int got;
	avcodec_decode_video2(vCodecContextInput, frameIn, &got, packetIn);
	sws_scale(vSWSContext, frameIn->data, frameIn->linesize, 0, height,
		videoFrame->data, videoFrame->linesize);
	videoFrame->format = vCodecContextOutput->pix_fmt;
	videoFrame->width = width;
	videoFrame->height = height;
	videoFrame->pts = frameIndex * (videoStream->time_base.den) / ((videoStream->time_base.num) * fps);

	AVPacket *packetOut = av_packet_alloc(); // ALLOC_PACKET_OUT
	avcodec_encode_video2(vCodecContextOutput, packetOut, videoFrame, &got);

	//视频帧保存为图片
	//savePacketToJPEG(packetIn, frameIndex);
	//saveFrameToJPEG(videoFrame, frameIndex);

	packetOut->stream_index = videoStream->index;
	av_write_frame(vFormatContextOutput, packetOut);

	printf("courseId = %d, Frame = %05d, PacketOutSize = %d\n",
		courseId, frameIndex, packetOut->size);

	av_frame_free(&frameIn);   // FREE_FRAME_IN
	av_free_packet(packetIn);  // FREE_PACKET_IN
	av_free_packet(packetOut); // FREE_PACKET_OUT

	return 0;
}

int FFMPEGExtend::catchJPEGFrame(int frameIndex)
{
	if (openCodecContextJPEG() < 0)
	{
		return -1;
	}

	AVPacket* packetIn = av_packet_alloc(); // ALLOC_PACKET_IN
	AVFrame* frameIn = av_frame_alloc();    // ALLOC_FRAME_IN

	if (av_read_frame(jpegFormatContext, packetIn) < 0)
	{
		centerError("catchJPEGFrame av_read_frame jpegFormatContext");
		return -1;
	}

	int gotPtr;
	avcodec_decode_video2(jpegCodecContext, frameIn, &gotPtr, packetIn);

	sws_scale(jpegSWSContext, frameIn->data, frameIn->linesize, 0, height,
		videoFrame->data, videoFrame->linesize);
	videoFrame->format = vCodecContextOutput->pix_fmt;
	videoFrame->width = width;
	videoFrame->height = height;
	videoFrame->pts = frameIndex*(videoStream->time_base.den) / ((videoStream->time_base.num) * fps);
	
	AVPacket *packetOut = av_packet_alloc(); // ALLOC_PACKET_OUT
	avcodec_encode_video2(vCodecContextOutput, packetOut, videoFrame, &gotPtr);

	packetOut->stream_index = videoStream->index;
	av_write_frame(vFormatContextOutput, packetOut);

	printf("courseId = %s, Frame = %05d, PacketOutSize = %d\n",
		courseId, frameIndex, packetOut->size);

	av_frame_free(&frameIn);   // FREE_FRAME_IN
	av_free_packet(packetIn);  // FREE_PACKET_IN
	av_free_packet(packetOut); // FREE_PACKET_OUT

	closeCodecContextJPEG();
	return 0;
}

void FFMPEGExtend::savePacketToJPEG(AVPacket *packet, int frame)
{
	char *filename = new char[255];
	//文件存放路径，根据自己的修改  
	sprintf_s(filename, 255, "%s%d.jpg", "temp\\", frame);

	FILE *file = fopen(filename, "wb");

	fwrite(packet->data, sizeof(uint8_t), packet->size, file);
	fclose(file);
}

void FFMPEGExtend::saveFrameToJPEG(AVFrame *frame, int index)
{
	char *filename = new char[255];
	//文件存放路径，根据自己的修改  
	sprintf_s(filename, 255, "%s%d.jpg", "temp\\", index);

	FILE *file = fopen(filename, "wb");

	//保存YUV420P格式的数据
	//fwrite(frame->data[0], frame->width * frame->height, 1, file);
	//fwrite(frame->data[1], frame->width * frame->height / 4, 1, file);
	//fwrite(frame->data[2], frame->width * frame->height / 4, 1, file);

	//保存RGB24格式的数据
	//fwrite(frame->data[0], frame->width * frame->height * 3, 1, file);

	//保存UYVY格式的数据
	//fwrite(frame->data[0], frame->width * frame->height, 2, file);

	fclose(file);
}

void FFMPEGExtend::saveFrameToPPM(AVFrame *frame, int index)
{
	char *filename = new char[255];
	//文件存放路径，根据自己的修改  
	sprintf_s(filename, 255, "%s%d.ppm", "temp\\", index);

	FILE *file = fopen(filename, "wb");

	//保存PPM格式的数据 
	// Write header
	fprintf(file, "P6\n%d %d\n255\n", width, height);
	// Write pixel data
	for (int i = 0; i < height; i++)
	{
		fwrite(frame->data[0] + i * frame->linesize[0], 1, width * 3, file);
	}

	fclose(file);
}

uint8_t *FFMPEGExtend::getFileBuffer()
{
	char *filename = new char[255];
	//文件存放路径，根据自己的修改  
	sprintf_s(filename, 255, inputFile);

	FILE *file = fopen(filename, "rb");

	/* 获取文件大小 */
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);

	/* 分配内存存储整个文件 */
	char *buffer = (char*)malloc(sizeof(char) * size);
	/* 将文件拷贝到buffer中 */
	fread(buffer, 1, size, file);
	/* 现在整个文件已经在buffer中，可由标准输出打印内容 */
	//printf("%d\n", sizeof(buffer));

	uint8_t *data = (uint8_t *)buffer;

	fclose(file);

	return data;
}

void FFMPEGExtend::centerError(char *error)
{
	printf("%s\n", error);
}

void FFMPEGExtend::catchVideoStart()
{
	if (isWorking == true) return;

	if (start() < 0) return;

	isWorking = true;
	int frameIndex = 0;
	while (isWorking)
	{
		//if (kbhit()) catchVideoStop();

#if(CAP_DEVICE == CAP_JPEG)
		catchJPEGFrame(frameIndex);
		Sleep((unsigned int)(1000.0f / 20.0f));
#else
		catchVideoFrame(frameIndex);
#endif
		++frameIndex;
	}

	stop();
}

void FFMPEGExtend::catchVideoStop()
{
	isWorking = false;
}