#include <android/log.h>
#include <GLES2/gl2.h>
#include "DrawString.h"

#define DISPLAY_WIDTH   1888.f
#define DISPLAY_HEIGHT  1356.f
#define CHAR_RESOLUTION 72
#define ADJUST_SIZE 7

#ifdef __cplusplus
extern "C" {
#endif

const static char *VERTEXSHADER_FOR_STRING =
        "attribute vec4 a_Position;\n"
        "attribute vec2 a_Texcoord;\n"
        "varying vec2 texcoordVarying;\n"
        "void main() {\n"
        "    gl_Position = a_Position;\n"
        "    texcoordVarying = a_Texcoord;\n"
        "}\n";

const static char *FRAGMENTSHADER_FOR_STRING =
        "precision mediump float;\n"
        "varying vec2 texcoordVarying;\n"
        "uniform sampler2D u_Sampler;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(u_Sampler, texcoordVarying);\n"
        "}\n";

#include <common/unicode/unistr.h>
void DrawStringbyCache(int x, int y, int fontSize, std::string argstr) {
    /* 初期化 ここから  */
    FT_Library library;
    FT_Error ret = FT_Init_FreeType(&library);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Init_FreeType() ret=%d", ret);

    FT_Int major=0, minor=0, patch=0;
    FT_Library_Version(library, &major, &minor, &patch);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Library_Version() ver %d.%d.%d", major, minor, patch);

    FT_Face fst_face;
    ret = FT_New_Face(library, "/system/fonts/NotoSansJP-Regular.otf", 0, &fst_face);
//    ret = FT_New_Face(library, "/data/local/rounded-mgenplus-1pp-regular.ttf", 0, &fst_face);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_New_Face() ret=%d", ret);

    FTC_Manager ft_manager;
    ret = FTC_Manager_New(library, 0, 0, 2400000, ftreqface, (FT_Pointer) fst_face, &ft_manager);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FTC_Manager_New() ret=%d", ret);

    FTC_CMapCache ft_mapcache;
    ret = FTC_CMapCache_New(ft_manager, &ft_mapcache);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FTC_CMapCache_New() ret=%d", ret);

    FTC_ImageCache ft_imagecache;
    ret = FTC_ImageCache_New(ft_manager, &ft_imagecache);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FTC_ImageCache_New() ret=%d", ret);
    /* 初期化 ここまで  */

    /* 準初期化 ここから  */
    FT_Face snd_face;
    ret = FTC_Manager_LookupFace(ft_manager, (FTC_FaceID)1, &snd_face);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FTC_Manager_LookupFace() ret=%d", ret);

    FT_Int cmap_index = FT_Get_Charmap_Index(snd_face->charmap);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Get_Charmap_Index() cmap_index=%d", cmap_index);
    /* 準初期化 ここまで  */

    FTC_ImageTypeRec font_type;
    font_type.face_id = (FTC_FaceID)1;
    font_type.width = fontSize;
    font_type.height= fontSize;
    font_type.flags = FT_LOAD_RENDER | FT_LOAD_NO_BITMAP;
    FTC_ScalerRec scaler;
    scaler.face_id  = font_type.face_id;
    scaler.width    = font_type.width<<6;
    scaler.height   = font_type.height<<6;
    scaler.pixel    = 0;
    scaler.x_res    = CHAR_RESOLUTION;
    scaler.y_res    = CHAR_RESOLUTION;
    FT_Size font_size;
    FTC_Manager_LookupSize(ft_manager, &scaler, &font_size);

    /* UTF-8 → UTF-32変換 */
    icu::UnicodeString utf8Str(argstr.c_str(), "UTF-8");
    wchar_t *utf32String = new wchar_t[argstr.length()+1/*BOM*/];
    int utf32length = utf8Str.extract(0, utf8Str.length(), (char*) utf32String, "UTF-32");
    utf32length /= 4;
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa utf32length=%d 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x", utf32length, utf32String[0], utf32String[1], utf32String[2], utf32String[3], utf32String[4]);
                                    /* UTF-32LE BOM */            /* UTF-32BE BOM */
    int text_image_width = 0, text_image_height = 0;
    char isVertical = false;
    GetBitmapSizeUsingFTCCMapforDrawString(isVertical, fontSize, utf32String, utf32length, &font_type, snd_face, cmap_index,  &text_image_width, &text_image_height, ft_mapcache, ft_imagecache);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa GetBitmapSizeUsingFTCCMapforDrawString() width=%d height=%d", text_image_width, text_image_height);

    GLvoid *image = calloc(text_image_width * text_image_height, sizeof(GLbyte)*1); /* GL_ALPHA */
    GLubyte *pImage = (GLubyte*)image;
    /* ベースライン算出 */
    int baseline = (snd_face->height + snd_face->descender) * snd_face->size->metrics.y_ppem / snd_face->units_per_EM + 1;
    int xoffset = 0;
    int loopstart = (utf32String[0]==0x0000feff || utf32String[0]==0xfffe0000) ? 1 : 0;
    int loopend   = (utf32String[utf32length-1]=='\0') ? (utf32length-1) : utf32length;
    for(int lpct = loopstart; lpct < loopend; lpct++) {
        FT_UInt iGhyhIndex = FTC_CMapCache_Lookup(ft_mapcache, font_type.face_id, cmap_index, utf32String[lpct]);
        if(!iGhyhIndex) continue;

        FT_Glyph glyph;
        FT_Error ret = FTC_ImageCache_Lookup(ft_imagecache, &font_type, iGhyhIndex, &glyph, NULL);
        if(ret) continue;

        if(glyph->format != FT_GLYPH_FORMAT_BITMAP) continue;

        FT_BitmapGlyph glyph_bitmap = (FT_BitmapGlyph)glyph;
        for(int dsthct = baseline-glyph_bitmap->top, srchct = 0; dsthct < (baseline-glyph_bitmap->top+glyph_bitmap->bitmap.rows); dsthct++, srchct++) {
            for(int dstwct = glyph_bitmap->left, srcwct = 0; dstwct < (glyph_bitmap->left+glyph_bitmap->bitmap.width); dstwct++, srcwct++) {
                if(dstwct < 0 || dsthct < 0 || dstwct >= text_image_width || dsthct >= text_image_height)
                    continue;
                pImage[(dsthct*text_image_width)+dstwct+xoffset] |= glyph_bitmap->bitmap.buffer[(srchct*glyph_bitmap->bitmap.width)+srcwct];
            }
        }

        /* 次の文字のx座標を計算.advanceの分解能は1/65536[pixel] */
        /* advenceはフォント毎に決まっている隣接文字描画時に移動する幅 */
        if(isVertical == true) {
            /* 縦書き */
            if(glyph_bitmap->root.advance.y)
                baseline += glyph_bitmap->root.advance.y >> 16;
            else
                baseline += font_type.width;   /* グリフが縦書きに対応していない場合は、フォントサイズで対応する */
        }
        else {
            /* 横書き */
            xoffset += glyph_bitmap->root.advance.x >> 16;
        }
    }

    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa width=%d height=%d", text_image_width, text_image_height);
    delete[] utf32String;

    /* シェーダ初期化 */
    int programid = createProgramforDrawString(VERTEXSHADER_FOR_STRING, FRAGMENTSHADER_FOR_STRING);
    if(programid == -1)
        __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa createProgramforDrawString() failed=%d", programid);

    /* 頂点座標点 */
    float vertexs[] = {
        (2*x-DISPLAY_WIDTH                             )/DISPLAY_WIDTH,(DISPLAY_HEIGHT-2*y                              )/DISPLAY_HEIGHT,
        (2*x-DISPLAY_WIDTH                             )/DISPLAY_WIDTH,(DISPLAY_HEIGHT-2*y-text_image_height*ADJUST_SIZE)/DISPLAY_HEIGHT,
        (2*x-DISPLAY_WIDTH+text_image_width*ADJUST_SIZE)/DISPLAY_WIDTH,(DISPLAY_HEIGHT-2*y                              )/DISPLAY_HEIGHT,
        (2*x-DISPLAY_WIDTH+text_image_width*ADJUST_SIZE)/DISPLAY_WIDTH,(DISPLAY_HEIGHT-2*y-text_image_height*ADJUST_SIZE)/DISPLAY_HEIGHT,
    };
    int positionid = glGetAttribLocation(programid, "a_Position");
    glEnableVertexAttribArray(positionid);
    glVertexAttribPointer(positionid, 2, GL_FLOAT, GL_FALSE, 0, vertexs);

    /* UV座標点 */
    float texcoords[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };
    int texcoordid = glGetAttribLocation(programid, "a_Texcoord");
    glEnableVertexAttribArray(texcoordid);
    glVertexAttribPointer(texcoordid, 2, GL_FLOAT, GL_FALSE, 0, texcoords);

    /* テクスチャ初期化 */
    GLuint textureid = -1;
    glGenTextures(1, &textureid);
    glBindTexture(GL_TEXTURE_2D, textureid);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /* サンプラ取得 */
    int samplerid = glGetUniformLocation(programid, "u_Sampler");

    /* プログラム有効化 */
    glUseProgram(programid);

    GLint format = GL_ALPHA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, text_image_width, text_image_height, 0, format, GL_UNSIGNED_BYTE, image);
    free(image);

    int texno = GL_TEXTURE7;
    glActiveTexture(texno);
    glBindTexture(GL_TEXTURE_2D, textureid);
    glUniform1i(samplerid, texno-GL_TEXTURE0);

    /* 描画実行 */
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

//    ret = FT_Done_Face(fst_face);
//    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Done_Face() ret=%d", ret);
    FTC_Manager_Done(ft_manager);
    ret = FT_Done_FreeType(library);
    __android_log_print(ANDROID_LOG_DEBUG, "aaaaa", "aaaaa FT_Done_FreeType() ret=%d", ret);
}

static FT_Error ftreqface(FTC_FaceID face_id, FT_Library library, FT_Pointer req_data, FT_Face *aface) {
    *aface = (FT_Face)req_data;
    return 0;
}

void GetBitmapSizeUsingFTCCMapforDrawString(char isvertical, int fontsize, wchar_t *utf32String, int utf32Length, FTC_ImageTypeRec *font_type, FT_Face lft_face, FT_Int cmap_index, GLsizei *width, GLsizei *height, FTC_CMapCache ftc_mapcache, FTC_ImageCache ftc_imagecache) {
    /* ベースライン算出 */
    int baseline = 0;
    if(lft_face->size != NULL) {
        baseline = (lft_face->height + lft_face->descender) * lft_face->size->metrics.y_ppem / lft_face->units_per_EM + 1;
    }

    int loopstart = (utf32String[0]==0x0000feff || utf32String[0]==0xfffe0000) ? 1 : 0;
    int loopend   = (utf32String[utf32Length-1]=='\0') ? (utf32Length-1) : utf32Length;
    int start_x = 0;
    int x_max = 0, y_max = 0;
    GLsizei text_image_w = 0, text_image_h = 0;
    for(int lpct = loopstart; lpct < loopend; lpct++) {
        FT_UInt iGhyhIndex = FTC_CMapCache_Lookup(ftc_mapcache, font_type->face_id, cmap_index, utf32String[lpct]);
        if (!iGhyhIndex) continue;

        FT_Glyph glyph;
        FT_Error ret = FTC_ImageCache_Lookup(ftc_imagecache, font_type, iGhyhIndex, &glyph, NULL);
        if (ret) continue;

        if (glyph->format != FT_GLYPH_FORMAT_BITMAP) continue;

        FT_BitmapGlyph glyph_bitmap = (FT_BitmapGlyph)glyph;
        x_max = start_x + glyph_bitmap->left + glyph_bitmap->bitmap.width;
        y_max = baseline- glyph_bitmap->top  + glyph_bitmap->bitmap.rows;
        if(x_max > text_image_w) text_image_w = x_max;
        if(y_max > text_image_h) text_image_h = y_max;

        /* 次の文字のx座標を計算.advanceの分解能は1/65536[pixel] */
        /* advenceはフォント毎に決まっている隣接文字描画時に移動する幅 */
        if(isvertical == true) {
            /* 縦書き */
            if(glyph_bitmap->root.advance.y)
                baseline += glyph_bitmap->root.advance.y >> 16;
            else
                baseline += fontsize;   /* グリフが縦書きに対応していない場合は、フォントサイズで対応する */
        }
        else {
            /* 横書き */
            start_x += glyph_bitmap->root.advance.x >> 16;
        }
    }

    text_image_w = (text_image_w + 7) & ~7; /* テクスチャ幅制限(8ピクセル) */
    *width  = text_image_w;
    *height = text_image_h;
    return;
}

GLuint createProgramforDrawString(const char *vertexshader, const char *fragmentshader) {
    GLuint vhandle = loadShaderorDrawString(GL_VERTEX_SHADER, vertexshader);
    if(vhandle == GL_FALSE) return GL_FALSE;

    GLuint fhandle = loadShaderorDrawString(GL_FRAGMENT_SHADER, fragmentshader);
    if(fhandle == GL_FALSE) return GL_FALSE;

    GLuint programhandle = glCreateProgram();
    if(programhandle == GL_FALSE) {
        checkGlErrororDrawString("glCreateProgram");
        return GL_FALSE;
    }

    glAttachShader(programhandle, vhandle);
    checkGlErrororDrawString("glAttachShader(VERTEX_SHADER)");
    glAttachShader(programhandle, fhandle);
    checkGlErrororDrawString("glAttachShader(FRAGMENT_SHADER)");

    glLinkProgram(programhandle);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(programhandle, GL_LINK_STATUS, &linkStatus);
    if(linkStatus != GL_TRUE) {
        GLint bufLen = 0;
        glGetProgramiv(programhandle, GL_INFO_LOG_LENGTH, &bufLen);
        if(bufLen) {
            char *logstr = (char*)malloc(bufLen);
            glGetProgramInfoLog(programhandle, bufLen, NULL, logstr);
            __android_log_print(ANDROID_LOG_ERROR, "aaaaa", "glLinkProgram() Fail!!\n %s", logstr);
            free(logstr);
        }
        glDeleteProgram(programhandle);
        programhandle = GL_FALSE;
    }

    return programhandle;
}

GLuint loadShaderorDrawString(int shadertype, const char *sourcestring) {
    GLuint shaderhandle = glCreateShader(shadertype);
    if(!shaderhandle) return GL_FALSE;

    glShaderSource(shaderhandle, 1, &sourcestring, NULL);
    glCompileShader(shaderhandle);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shaderhandle, GL_COMPILE_STATUS, &compiled);
    if(!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shaderhandle, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen) {
            char *logbuf = (char*)malloc(infoLen);
            if(logbuf) {
                glGetShaderInfoLog(shaderhandle, infoLen, NULL, logbuf);
                __android_log_print(ANDROID_LOG_ERROR, "aaaaa", "shader failuer!!\n%s", logbuf);
                free(logbuf);
            }
        }
        glDeleteShader(shaderhandle);
        shaderhandle = GL_FALSE;
    }

    return shaderhandle;
}

void checkGlErrororDrawString(const char *argstr) {
    for(GLuint error = glGetError(); error; error = glGetError())
        __android_log_print(ANDROID_LOG_ERROR, "aaaaa", "after %s errcode=%d", argstr, error);
}

#ifdef __cplusplus
}
#endif
