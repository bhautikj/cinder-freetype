#include "freetypeTextRender.h"

#include "cinder/app/App.h"
#include "cinder/gl/Texture.h"

namespace
{
std::string fShad = "\
varying mediump vec2 texcoord; \n\
uniform sampler2D tex; \n\
uniform mediump vec4 color; \n\
\n\
void main(void) { \n\
  gl_FragColor = vec4(1, 1, 1, texture2D(tex, texcoord).a) * color; \n\
} \n\
";

std::string vShad = "\
attribute mediump vec4 position; \n\
varying mediump vec2 texcoord; \n\
 \n\
void main(void) { \n\
  gl_Position = vec4(position.xy, 0, 1); \n\
  texcoord = position.zw; \n\
} \n\
";

}

namespace ftTextRender
{

static FT_Library ft;

void initFT()
{
  if (FT_Init_FreeType(&ft)) 
  {
    throw;
  }
}

TextRender::TextRender(const std::string& fontName) :
  mFontName(fontName)
{
  initFT();
  
  cinder::DataSourceRef fontLoc = cinder::app::loadResource(fontName);
  
  if (FT_New_Face(ft, fontLoc->getFilePath().string().c_str(), 0, &mFace))
  {
    throw;
  }
  
  float positions[] = { 0, 0, 0, 1, 0, .01, 0, 1, .01, 0, 0, 1, .01, .01, 0, 1 };
  
  mVbo = cinder::gl::Vbo::create(GL_TRIANGLE_STRIP);
  mVbo->set(cinder::gl::Vbo::Attribute::create("position", 4, GL_FLOAT, GL_DYNAMIC_DRAW)->setData(positions, 16));
  
  try {
    mShader = cinder::gl::GlslProg(vShad.c_str(), fShad.c_str());
    std::cout<<"shader log: "<<mShader.getShaderLog(mShader.getHandle())<<std::endl;
  } catch (cinder::gl::GlslProgCompileExc exc) {
    std::cout<<exc.what()<<std::endl;
    assert(0);
  }
}

TextRender::~TextRender()
{
}

/**
 * Render text using the currently loaded font and currently set font size.
 * Rendering starts at coordinates (x, y), z is always 0.
 * The pixel coordinates that the FreeType2 library uses are scaled by (sx, sy).
 */
void
TextRender::render_text(const char *text, float x, float y, float sx, float sy)
{
	const char *p;
	FT_GlyphSlot g = mFace->glyph;

  cinder::gl::Texture t;

	/* Create a texture that will be used to hold one "glyph" */
	GLuint tex;

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
  //TODO: where is this in relation to mShader->bind()?
  mShader.uniform("tex", 0);

	/* We require 1 byte alignment when uploading texture data */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	/* Clamping to edges is important to prevent artifacts when scaling */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/* Linear filtering usually looks best for text */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
	/* Loop through all characters */
	for (p = text; *p; p++) {
		/* Try to load and render the character */
		if (FT_Load_Char(mFace, *p, FT_LOAD_RENDER))
			continue;

 		/* Upload the "bitmap", which contains an 8-bit grayscale image, as an alpha texture */
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, g->bitmap.width, g->bitmap.rows, 0, GL_ALPHA, GL_UNSIGNED_BYTE, g->bitmap.buffer);

		/* Calculate the vertex and texture coordinates */
		float x2 = x + g->bitmap_left * sx;
		float y2 = -y - g->bitmap_top * sy;
		float w = g->bitmap.width * sx;
		float h = g->bitmap.rows * sy;
    
		float data[16] = {
			x2, -y2, 0, 0,
			x2 + w, -y2, 1, 0,
			x2, -y2 - h, 0, 1,
			x2 + w, -y2 - h, 1, 1
		};
    
    mVbo->get("position")->setData(data, 16);

		/* Draw the character on the screen */
    mVbo->draw();
    
		/* Advance the cursor to the start of the next character */
		x += (g->advance.x >> 6) * sx;
		y += (g->advance.y >> 6) * sy;
	}

	//glDisableVertexAttribArray(attribute_coord);
	glDeleteTextures(1, &tex);
}

void
TextRender::draw()
{
  mShader.bind();
  mVbo->assignLocations(mShader);

  float sx = 0.001;
  float sy = 0.001;
  
	glClear(GL_COLOR_BUFFER_BIT);

	/* Enable blending, necessary for our alpha texture */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* Set font size to 48 pixels, color to black */
	FT_Set_Pixel_Sizes(mFace, 0, 48);
  cinder::Vec4f redV(1,0,0,1);
  mShader.uniform("color", redV);

	/* Effects of alignment */
	render_text("the quick brown fox jumped over the lazy text layout engine", 0, 0, sx, sy);
}

}//namespace ftTextRender