#include <map>
#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include "CppSurfaceView.h"

std::map<int, CppSurfaceView*> gpSufacesLists;

#ifdef __cplusplus
extern "C" {
#endif

void Java_com_test_cppdrawstringcache_NativeFunc_create(JNIEnv *pEnv, jclass type, jint id) {
    gpSufacesLists[id] = new CppSurfaceView(id);
}

void Java_com_test_cppdrawstringcache_NativeFunc_surfaceCreated(JNIEnv *pEnv, jclass type, jint id, jobject surface) {
    gpSufacesLists[id]->createThread(pEnv, surface);
}

void Java_com_test_cppdrawstringcache_NativeFunc_surfaceChanged(JNIEnv *pEnv, jclass type, jint id, jint width, jint height) {
    gpSufacesLists[id]->isSurfaceCreated = true;
    gpSufacesLists[id]->DspW = width;
    gpSufacesLists[id]->DspH = height;
}

void Java_com_test_cppdrawstringcache_NativeFunc_surfaceDestroyed(JNIEnv *pEnv, jclass type, jint id) {
    gpSufacesLists[id]->mStatus = CppSurfaceView::STATUS_FINISH;
    gpSufacesLists[id]->destroy();
}

#ifdef __cplusplus
}
#endif

/******************/
/* CppSurfaceView */
/******************/
CppSurfaceView::CppSurfaceView(int id) : mId(id), mStatus(CppSurfaceView::STATUS_NONE), mThreadId(-1) {
}

void CppSurfaceView::createThread(JNIEnv *pEnv, jobject surface) {
    mStatus = CppSurfaceView::STATUS_INITIALIZING;
    mWindow = ANativeWindow_fromSurface(pEnv, surface);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&mThreadId, &attr, CppSurfaceView::draw_thread, this);
}

#include <unistd.h>
void *CppSurfaceView::draw_thread(void *pArg) {
    if(pArg == NULL) return NULL;
    CppSurfaceView *pSurface = (CppSurfaceView*)pArg;

    pSurface->initEGL();
    pSurface->mStatus = CppSurfaceView::STATUS_DRAWING;

    pSurface->predrawGL();
    struct timespec req = {0, (int)(16.66*1000000)};
    for(;pSurface->mStatus==CppSurfaceView::STATUS_DRAWING;) {
        clock_t s = clock();

        /* SurfaceCreated()が動作した時は、画面サイズ変更を実行 */
        if(pSurface->isSurfaceCreated) {
            pSurface->isSurfaceCreated = false;
            glViewport(0,0,pSurface->DspW,pSurface->DspH);
        }

        /* 通常の描画処理 */
        pSurface->drawGL();
        clock_t e = clock();
        if( (e-s) < 16667) {
            clock_t waittime = 16667 - (e-s);
            usleep(waittime);
        }
    }

    pSurface->finEGL();

    return NULL;
}

void CppSurfaceView::initEGL() {
    EGLint major, minor;
    mEGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(mEGLDisplay, &major, &minor);

    /* 設定取得 */
    const EGLint configAttributes[] = {
            EGL_LEVEL, 0,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            /* 透過設定 */
            EGL_ALPHA_SIZE, EGL_OPENGL_BIT,
            /*EGL_BUFFER_SIZE, 32 */  /* ARGB8888用 */
            EGL_DEPTH_SIZE, 16,
            EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(mEGLDisplay, configAttributes, &config, 1, &numConfigs);

    /* context生成 */
    const EGLint contextAttributes[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };
    mEGLContext = eglCreateContext(mEGLDisplay, config, EGL_NO_CONTEXT, contextAttributes);

    /* ウィンドウバッファサイズとフォーマットを設定 */
    EGLint format;
    eglGetConfigAttrib(mEGLDisplay, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(mWindow, 0, 0, format);

    /* surface生成 */
    mEGLSurface = eglCreateWindowSurface(mEGLDisplay, config, mWindow, NULL);
    if(mEGLSurface == EGL_NO_SURFACE) {
        __android_log_print(ANDROID_LOG_ERROR, "CppSurfaceView", "%d surface生成 失敗!!");
        return;
    }

    /* context再生成 */
    mEGLContext = eglCreateContext(mEGLDisplay, config, EGL_NO_CONTEXT, contextAttributes);

    /* 作成したsurface/contextを関連付け */
    if(eglMakeCurrent(mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext) == EGL_FALSE) {
        __android_log_print(ANDROID_LOG_ERROR, "CppSurfaceView", "%d surface/contextの関連付け 失敗!!");
        return;
    }

    /* チェック */
    EGLint w,h;
    eglQuerySurface(mEGLDisplay, mEGLSurface, EGL_WIDTH, &w);
    eglQuerySurface(mEGLDisplay, mEGLSurface, EGL_HEIGHT,&h);
    glViewport(0,0,w,h);
}

void CppSurfaceView::predrawGL() {
    glClearColor(0.1, 0.2, 0.3, 0.25);
}

#include "DrawString.h"
void CppSurfaceView::drawGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawStringbyCache(10,   0, 16, "ありがとう");
    DrawStringbyCache(10,  80, 32, "おかえり");
    DrawStringbyCache(10, 240, 72, "夜遅くまで");
    DrawStringbyCache(10, 600,144, "お疲れさま");

    eglSwapBuffers(mEGLDisplay, mEGLSurface);
}

void CppSurfaceView::finEGL() {
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa%d CppSurfaceView::finEGL()", mId);
    ANativeWindow_release(mWindow);
    mWindow = NULL;
}

void CppSurfaceView::destroy() {
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa%d CppSurfaceView::destroy()", mId);
    pthread_join(mThreadId, NULL);
}

CppSurfaceView::~CppSurfaceView() {
}
