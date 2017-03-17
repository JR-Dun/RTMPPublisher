#include "stdafx.h"
#include "ffmpegExtend.h"

#define MAX_NAME_LEN 1024
#define CAP_SCREEN	1
#define CAP_CAMERA	2
#define CAP_JPEG		3
#define CAP_DEVICE	CAP_JPEG

char *inputCamera	= "video=Lenovo EasyCamera";	// ����ͷ
char *inputFile		= "temp\\error.jpg";			// �ļ�·��

// �����ʽ
//1:RTMP����				eg��"rtmp://120.92.3.155:1935/live/stream1"
//2:����Ϊ������Ƶ�ļ�	eg��"C:\\JR_Dun\\git\\RTMPPublisher\\Debug\\stream.mp4"
char *output = ""; // �����ʽ1�� RTMP����

const int fps = 20;			// ��Ƶ���FPS
const int width = 1280;		// ��Ƶwidth
const int height = 720;		// ��Ƶheight

FFMPEGExtend::FFMPEGExtend(char *inputFilePath)
{
	init(inputFilePath);
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

void FFMPEGExtend::init(char *inputFilePath)
{
	if (inputFilePath != NULL && inputFilePath[0] != '\0' && sizeof(inputFilePath) > 0)
	{
		inputFile = inputFilePath;
	}
}

void FFMPEGExtend::start()
{
	vPixelFormatOutput = AV_PIX_FMT_YUV420P;

	av_register_all();
	avformat_network_init();
	avdevice_register_all();

	initInput();
	initFrame();
	initOutput();
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
	sws_freeContext(jpegSWSContext);
	avformat_close_input(&jpegFormatContext);
	av_free(jpegFormatContext);
#else
	sws_freeContext(vSWSContext);
	avformat_close_input(&vFormatContextInput);
	av_free(vFormatContextInput);
#endif

}

void FFMPEGExtend::initInput()
{
#if(CAP_DEVICE == CAP_JPEG)

	jpegFormatContext = avformat_alloc_context();

#else
	vFormatContextInput = avformat_alloc_context();

	AVDictionary *vOptionsInput = NULL;
	char c_fps[8] = { 0 };
	itoa(fps, c_fps, 10);
	av_dict_set(&vOptionsInput, "frame_rate", c_fps, 0);
#if(CAP_DEVICE == CAP_SCREEN) // ¼����Ļ
	av_dict_set(&vOptionsInput, "offset_x", "0", 0);
	av_dict_set(&vOptionsInput, "offset_y", "0", 0);
	av_dict_set(&vOptionsInput, "video_size", "1280x720", 0);

	AVInputFormat *inputFormat = av_find_input_format("gdigrab");
	avformat_open_input(&vFormatContextInput, "desktop", inputFormat, NULL);
#elif(CAP_DEVICE == CAP_CAMERA) // ����ͷ
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
	avcodec_open2(vCodecContextInput, vCodecInput, NULL);

	vSWSContext = sws_getContext(width, height, vCodecContextInput->pix_fmt,
		width, height, vPixelFormatOutput, SWS_BILINEAR, NULL, NULL, NULL);

#endif
}

void FFMPEGExtend::initOutput()
{
	char URL[MAX_NAME_LEN] = { 0 };
	strcpy(URL, output);

	vFormatContextOutput = NULL;
	avformat_alloc_output_context2(&vFormatContextOutput, NULL, "flv", URL);

	avio_open(&(vFormatContextOutput->pb), URL, AVIO_FLAG_WRITE);

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
	//vCodecContextOutput->max_b_frames = 3; //FLV��֧��

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
	avcodec_open2(vCodecContextOutput, vCodecOutput, &vOptionsOutput);

	avformat_write_header(vFormatContextOutput, NULL);
}

void FFMPEGExtend::openCodecContextJPEG()
{
	AVDictionary *vOptionsInput = NULL;
	char c_fps[8] = { 0 };
	itoa(fps, c_fps, 10);
	av_dict_set(&vOptionsInput, "frame_rate", c_fps, 0);
	avformat_open_input(&jpegFormatContext, inputFile, NULL, NULL);
	avformat_find_stream_info(jpegFormatContext, NULL);

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
	avcodec_open2(jpegCodecContext, vCodecInput, NULL);

	jpegSWSContext = sws_getContext(width, height, jpegCodecContext->pix_fmt,
		width, height, vPixelFormatOutput, SWS_BILINEAR, NULL, NULL, NULL);
}

void FFMPEGExtend::closeCodecContextJPEG()
{
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

void FFMPEGExtend::catchVideoFrame(int frameIndex)
{
	AVPacket* packetIn = av_packet_alloc(); // ALLOC_PACKET_IN
	AVFrame* frameIn = av_frame_alloc();    // ALLOC_FRAME_IN

	av_read_frame(vFormatContextInput, packetIn);

	//�滻��Ƶ֡
	//uint8_t *jpegBuffer = getFileBuffer();
	//if (frameIndex > 100 && false)
	//{
	//	packetIn->data = jpegBuffer;
	//}

	int got;
	avcodec_decode_video2(vCodecContextInput, frameIn, &got, packetIn);
	sws_scale(vSWSContext, frameIn->data, frameIn->linesize, 0, height,
		videoFrame->data, videoFrame->linesize);
	videoFrame->format = vCodecContextOutput->pix_fmt;
	videoFrame->width = width;
	videoFrame->height = height;
	videoFrame->pts = frameIndex*(videoStream->time_base.den) / ((videoStream->time_base.num) * fps);

	AVPacket *packetOut = av_packet_alloc(); // ALLOC_PACKET_OUT
	avcodec_encode_video2(vCodecContextOutput, packetOut, videoFrame, &got);

	//��Ƶ֡����ΪͼƬ
	//savePacketToJPEG(packetIn, frameIndex);
	//saveFrameToJPEG(videoFrame, frameIndex);

	packetOut->stream_index = videoStream->index;
	av_write_frame(vFormatContextOutput, packetOut);

	printf("Frame = %05d, PacketOutSize = %d\n", 
		frameIndex, packetOut->size);

	//free(jpegBuffer);
	av_frame_free(&frameIn);   // FREE_FRAME_IN
	av_free_packet(packetIn);  // FREE_PACKET_IN
	av_free_packet(packetOut); // FREE_PACKET_OUT
}

void FFMPEGExtend::catchJPEGFrame(int frameIndex)
{
	openCodecContextJPEG();

	AVPacket* packetIn = av_packet_alloc(); // ALLOC_PACKET_IN
	AVFrame* frameIn = av_frame_alloc();    // ALLOC_FRAME_IN

	if (av_read_frame(jpegFormatContext, packetIn) < 0)
	{
		return;
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

	printf("Frame = %05d, PacketOutSize = %d\n",
		frameIndex, packetOut->size);

	av_frame_free(&frameIn);   // FREE_FRAME_IN
	av_free_packet(packetIn);  // FREE_PACKET_IN
	av_free_packet(packetOut); // FREE_PACKET_OUT

	closeCodecContextJPEG();
	return;
}

void FFMPEGExtend::savePacketToJPEG(AVPacket *packet, int frame)
{
	char *filename = new char[255];
	//�ļ����·���������Լ����޸�  
	sprintf_s(filename, 255, "%s%d.jpg", "temp\\", frame);

	FILE *file = fopen(filename, "wb");
	
	fwrite(packet->data, sizeof(uint8_t), packet->size, file);
	fclose(file);
}

void FFMPEGExtend::saveFrameToJPEG(AVFrame *frame, int index)
{
	char *filename = new char[255];
	//�ļ����·���������Լ����޸�  
	sprintf_s(filename, 255, "%s%d.jpg", "temp\\", index);

	FILE *file = fopen(filename, "wb");

	//����YUV420P��ʽ������
	//fwrite(frame->data[0], frame->width * frame->height, 1, file);
	//fwrite(frame->data[1], frame->width * frame->height / 4, 1, file);
	//fwrite(frame->data[2], frame->width * frame->height / 4, 1, file);

	//����RGB24��ʽ������
	//fwrite(frame->data[0], frame->width * frame->height * 3, 1, file);

	//����UYVY��ʽ������
	//fwrite(frame->data[0], frame->width * frame->height, 2, file);

	fclose(file);
}

void FFMPEGExtend::saveFrameToPPM(AVFrame *frame, int index)
{
	char *filename = new char[255];
	//�ļ����·���������Լ����޸�  
	sprintf_s(filename, 255, "%s%d.ppm", "temp\\", index);

	FILE *file = fopen(filename, "wb");

	//����PPM��ʽ������ 
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
	//�ļ����·���������Լ����޸�  
	sprintf_s(filename, 255, inputFile);

	FILE *file = fopen(filename, "rb");

	/* ��ȡ�ļ���С */
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);

	/* �����ڴ�洢�����ļ� */
	char *buffer = (char*)malloc(sizeof(char) * size);
	/* ���ļ�������buffer�� */
	fread(buffer, 1, size, file);
	/* ���������ļ��Ѿ���buffer�У����ɱ�׼�����ӡ���� */
	//printf("%d\n", sizeof(buffer));

	uint8_t *data = (uint8_t *)buffer;

	fclose(file);

	return data;
}

void FFMPEGExtend::catchVideoStart()
{
	start();
	isWorking = true;
	int frameIndex = 0;
	while (isWorking)
	{
		if (kbhit()) catchVideoStop();

#if(CAP_DEVICE == CAP_JPEG)
		catchJPEGFrame(frameIndex);
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
