// FFMPEGExtendLibrary.h

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

	extern __declspec(dllexport) void startCatchVideo(char *courseId);

	extern __declspec(dllexport) void stopCatchVideo(char *courseId);


#ifdef __cplusplus
}
#endif


//C#���ö�̬�⣨dll������
//[DllImport("FFMPEGExtendLibrary.dll", CallingConvention = CallingConvention.Cdecl)]
//public static extern void startCatchVideo(string courseId);
//
//[DllImport("FFMPEGExtendLibrary.dll", CallingConvention = CallingConvention.Cdecl)]
//public static extern void stopCatchVideo(string courseId);