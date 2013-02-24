#pragma once

#include "cinder/gl/GlslProg.h"

#include "Vbo.h"

#include "ft2build.h"
#include FT_FREETYPE_H

#include <string>

namespace ftTextRender
{

class atlas;

class TextRender
{
  public:
    TextRender(const std::string& fontName);
    ~TextRender();
  
    void  render_text(const char *text, float x, float y, float sx = 1.0, float sy = 1.0);
    void  draw();
 
  protected:
    std::string             mFontName;
    FT_Face                 mFace;
    cinder::gl::GlslProg    mShader;
    cinder::gl::VboRef      mVbo;
    atlas*                  mAtlas;
};
  
}//namespace ftTextRender