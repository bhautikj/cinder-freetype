#include "freetypeTextRender.h"

#include "cinder/app/App.h"
#include "cinder/gl/Texture.h"

#include "atlas.h"

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
  
  //mVbo = cinder::gl::Vbo::create(GL_TRIANGLE_STRIP);
  mVbo = cinder::gl::Vbo::create(GL_TRIANGLES);
  mVbo->set(cinder::gl::Vbo::Attribute::create("position", 4, GL_FLOAT, GL_DYNAMIC_DRAW)->setData(positions, 16));
  
  try {
    mShader = cinder::gl::GlslProg(vShad.c_str(), fShad.c_str());
    std::cout<<"shader log: "<<mShader.getShaderLog(mShader.getHandle())<<std::endl;
  } catch (cinder::gl::GlslProgCompileExc exc) {
    std::cout<<exc.what()<<std::endl;
    assert(0);
  }
  
  mAtlas = new atlas(mFace, 48);
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
	const uint8_t *p;

	/* Use the texture containing the atlas */
	glBindTexture(GL_TEXTURE_2D, mAtlas->tex);
	glUniform1i(uniform_tex, 0);

	std::vector<cinder::Vec4f> coords;
  coords.resize(6 * strlen(text));
	int c = 0;

	/* Loop through all characters */
	for (p = (const uint8_t *)text; *p; p++) {
		/* Calculate the vertex and texture coordinates */
		float x2 = x + mAtlas->c[*p].bl * sx;
		float y2 = -y - mAtlas->c[*p].bt * sy;
		float w = mAtlas->c[*p].bw * sx;
		float h = mAtlas->c[*p].bh * sy;

		/* Advance the cursor to the start of the next character */
		x += mAtlas->c[*p].ax * sx;
		y += mAtlas->c[*p].ay * sy;

		/* Skip glyphs that have no pixels */
		if (!w || !h)
			continue;

		coords[c++] = cinder::Vec4f(x2, -y2, mAtlas->c[*p].tx, mAtlas->c[*p].ty);
		coords[c++] = cinder::Vec4f(x2 + w, -y2, mAtlas->c[*p].tx + mAtlas->c[*p].bw / mAtlas->w, mAtlas->c[*p].ty);
		coords[c++] = cinder::Vec4f(x2, -y2 - h, mAtlas->c[*p].tx, mAtlas->c[*p].ty + mAtlas->c[*p].bh / mAtlas->h);
		coords[c++] = cinder::Vec4f(x2 + w, -y2, mAtlas->c[*p].tx + mAtlas->c[*p].bw / mAtlas->w, mAtlas->c[*p].ty);
		coords[c++] = cinder::Vec4f(x2, -y2 - h, mAtlas->c[*p].tx, mAtlas->c[*p].ty + mAtlas->c[*p].bh / mAtlas->h);
		coords[c++] = cinder::Vec4f(x2 + w, -y2 - h, mAtlas->c[*p].tx + mAtlas->c[*p].bw / mAtlas->w, mAtlas->c[*p].ty + mAtlas->c[*p].bh / mAtlas->h);
	}

	/* Draw all the character on the screen in one go */
  mVbo->get("position")->setData(coords);
  mVbo->draw();
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