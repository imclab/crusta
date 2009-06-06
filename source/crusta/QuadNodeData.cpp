#include <crusta/QuadNodeData.h>

BEGIN_CRUSTA

QuadNodeMainData::
QuadNodeMainData(uint size)
{
    geometry = new Vertex[size*size];
    height   = new DemHeight[size*size];
    color    = new TextureColor[size*size];

    for (int i=0; i<4; ++i)
    {
        childDemTiles[i]   = DemFile::INVALID_TILEINDEX;
        childColorTiles[i] = ColorFile::INVALID_TILEINDEX;
    }
    elevationRange[0] = elevationRange[1] = 0.0;
}
QuadNodeMainData::
~QuadNodeMainData()
{
    delete[] geometry;
    delete[] height;
    delete[] color;
}

QuadNodeVideoData::
QuadNodeVideoData(uint size)
{
    createTexture(geometry, GL_RGB32F_ARB, size);
    createTexture(height, GL_INTENSITY32F_ARB, size);
    createTexture(color, GL_RGB, size);
}
QuadNodeVideoData::
~QuadNodeVideoData()
{
    glDeleteTextures(1, &geometry);
    glDeleteTextures(1, &height);
    glDeleteTextures(1, &color);
}

void QuadNodeVideoData::
createTexture(GLuint& texture, GLint internalFormat, uint size)
{
    glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size, size, 0,
                 GL_RGB, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

END_CRUSTA

#include <crusta/QuadNodeData.hpp>
