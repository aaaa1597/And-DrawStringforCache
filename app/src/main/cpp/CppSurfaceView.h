#ifndef TESTNATIVESURFACE_H
#define TESTNATIVESURFACE_H

#include <jni.h>
#include <android/native_window.h>
#include <pthread.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_com_test_cppdrawstringcache_NativeFunc_create(JNIEnv *env, jclass type, jint id);
JNIEXPORT void JNICALL Java_com_test_cppdrawstringcache_NativeFunc_surfaceCreated(JNIEnv *env, jclass type, jint id, jobject surface);
JNIEXPORT void JNICALL Java_com_test_cppdrawstringcache_NativeFunc_surfaceChanged(JNIEnv *env, jclass type, jint id, jint width, jint height);
JNIEXPORT void JNICALL Java_com_test_cppdrawstringcache_NativeFunc_surfaceDestroyed(JNIEnv *env, jclass type, jint id);

#ifdef __cplusplus
}
#endif

class CppSurfaceView {
public:
    static const int STATUS_NONE   = 0;
    static const int STATUS_INITIALIZING = 1;
    static const int STATUS_DRAWING= 2;
    static const int STATUS_FINISH = 3;
    int mStatus = STATUS_NONE;
    int mId = -1;
    pthread_t mThreadId = -1;
    ANativeWindow *mWindow = NULL;
    EGLDisplay mEGLDisplay = NULL;
    EGLContext mEGLContext = NULL;
    EGLSurface mEGLSurface = NULL;
    bool isSurfaceCreated = false;
    int DspW = 0;
    int DspH = 0;

public:
    CppSurfaceView(int id);
    virtual ~CppSurfaceView();
    static void *draw_thread(void *pArg);
    void createThread(JNIEnv *pEnv, jobject surface);
    void initEGL();
    void finEGL();
    void predrawGL();
    void drawGL();
    void destroy();
};

#endif //TESTNATIVESURFACE_H
