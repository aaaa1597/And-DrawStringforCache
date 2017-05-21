#ifndef DRAWSTRING_H__
#define DRAWSTRING_H__

#include <string>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

/* シェーダー初期化 */
GLuint createProgramforDrawString(const char *vertexshader, const char *fragmentshader);
GLuint loadShaderorDrawString(int shadertype, const char *sourcestring);
void checkGlErrororDrawString(const char *argstr);

/* 文字列描画 */
void DrawStringbyCache(int x, int y, int fontSize, std::string argstr);

/* freetype */
static FT_Error ftreqface(FTC_FaceID face_id, FT_Library library, FT_Pointer req_data, FT_Face *aface);
void GetBitmapSizeUsingFTCCMapforDrawString(char vertical, int size, wchar_t *utf32String, int utf32Length, FTC_ImageTypeRec *font_type, FT_Face lft_face, FT_Int cmap_index, GLsizei *width, GLsizei *height, FTC_CMapCache ftc_mapcache, FTC_ImageCache ftc_imagecache);

#ifdef __cplusplus
}
#endif

#endif //DRAWSTRING_H__
