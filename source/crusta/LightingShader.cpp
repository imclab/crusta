/***********************************************************************
LightingShader - Class to maintain a GLSL point-based lighting
shader that tracks the current OpenGL lighting state.
Copyright (c) 2008 Oliver Kreylos

This file is part of the LiDAR processing and analysis package.

The LiDAR processing and analysis package is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The LiDAR processing and analysis package is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the LiDAR processing and analysis package; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/
#include <crusta/LightingShader.h>

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <Misc/ThrowStdErr.h>

///\todo integrate properly (VIS 2010)
#include <crusta/Crusta.h>

///\todo remove debug
#include <sys/stat.h>
#include <fstream>


BEGIN_CRUSTA


/*************************************************
Static elements of class LightingShader:
*************************************************/

const char* LightingShader::applyLightTemplate=
    "\
    vec4 applyLight<lightIndex>(const vec4 vertexEc,const vec3 normalEc,const vec4 ambient,const vec4 diffuse,const vec4 specular)\n\
        {\n\
        vec4 color;\n\
        vec3 lightDirEc;\n\
        float nl;\n\
        \n\
        /* Calculate per-source ambient light term: */\n\
        color=gl_LightSource[<lightIndex>].ambient*ambient;\n\
        \n\
        /* Compute the light direction (works both for directional and point lights): */\n\
        lightDirEc=gl_LightSource[<lightIndex>].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[<lightIndex>].position.w;\n\
        lightDirEc=normalize(lightDirEc);\n\
        \n\
        /* Compute the diffuse lighting angle: */\n\
        nl=dot(normalEc,lightDirEc);\n\
        if(nl>0.0)\n\
            {\n\
            vec3 eyeDirEc;\n\
            float nhv;\n\
            \n\
            /* Calculate per-source diffuse light term: */\n\
            color+=(gl_LightSource[<lightIndex>].diffuse*diffuse)*nl;\n\
            \n\
            /* Compute the eye direction: */\n\
            eyeDirEc=normalize(-vertexEc.xyz);\n\
            \n\
            /* Compute the specular lighting angle: */\n\
            nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);\n\
            \n\
            /* Calculate per-source specular lighting term: */\n\
            color+=(gl_LightSource[<lightIndex>].specular*specular)*pow(nhv,gl_FrontMaterial.shininess);\n\
            }\n\
        \n\
        /* Return the result color: */\n\
        return color;\n\
        }\n\
    \n";

const char* LightingShader::applyAttenuatedLightTemplate=
    "\
    vec4 applyLight<lightIndex>(const vec4 vertexEc,const vec3 normalEc,const vec4 ambient,const vec4 diffuse,const vec4 specular)\n\
        {\n\
        vec4 color;\n\
        vec3 lightDirEc;\n\
        float lightDist,nl,att;\n\
        \n\
        /* Calculate per-source ambient light term: */\n\
        color=gl_LightSource[<lightIndex>].ambient*ambient;\n\
        \n\
        /* Compute the light direction (works both for directional and point lights): */\n\
        lightDirEc=gl_LightSource[<lightIndex>].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[<lightIndex>].position.w;\n\
        lightDist=length(lightDirEc);\n\
        lightDirEc=normalize(lightDirEc);\n\
        \n\
        /* Compute the diffuse lighting angle: */\n\
        nl=dot(normalEc,lightDirEc);\n\
        if(nl>0.0)\n\
            {\n\
            vec3 eyeDirEc;\n\
            float nhv;\n\
            \n\
            /* Calculate per-source diffuse light term: */\n\
            color+=(gl_LightSource[<lightIndex>].diffuse*diffuse)*nl;\n\
            \n\
            /* Compute the eye direction: */\n\
            eyeDirEc=normalize(-vertexEc.xyz);\n\
            \n\
            /* Compute the specular lighting angle: */\n\
            nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);\n\
            \n\
            /* Calculate per-source specular lighting term: */\n\
            color+=(gl_LightSource[<lightIndex>].specular*specular)*pow(nhv,gl_FrontMaterial.shininess);\n\
            }\n\
        \n\
        /* Attenuate the per-source light terms: */\n\
        att=(gl_LightSource[<lightIndex>].quadraticAttenuation*lightDist+gl_LightSource[<lightIndex>].linearAttenuation)*lightDist+gl_LightSource[<lightIndex>].constantAttenuation;\n\
        color*=1.0/att;\n\
        \n\
        /* Return the result color: */\n\
        return color;\n\
        }\n\
    \n";

const char* LightingShader::applySpotLightTemplate=
    "\
    vec4 applyLight<lightIndex>(const vec4 vertexEc,const vec3 normalEc,const vec4 ambient,const vec4 diffuse,const vec4 specular)\n\
        {\n\
        vec4 color;\n\
        vec3 lightDirEc;\n\
        float sl,nl,att;\n\
        \n\
        /* Calculate per-source ambient light term: */\n\
        color=gl_LightSource[<lightIndex>].ambient*ambient;\n\
        \n\
        /* Compute the light direction (works both for directional and point lights): */\n\
        lightDirEc=gl_LightSource[<lightIndex>].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[<lightIndex>].position.w;\n\
        lightDirEc=normalize(lightDirEc);\n\
        \n\
        /* Calculate the spot light angle: */\n\
        sl=-dot(lightDirEc,normalize(gl_LightSource[<lightIndex>].spotDirection));\n\
        \n\
        /* Check if the point is inside the spot light's cone: */\n\
        if(sl>=gl_LightSource[<lightIndex>].spotCosCutoff)\n\
            {\n\
            /* Compute the diffuse lighting angle: */\n\
            nl=dot(normalEc,lightDirEc);\n\
            if(nl>0.0)\n\
                {\n\
                vec3 eyeDirEc;\n\
                float nhv;\n\
                \n\
                /* Calculate per-source diffuse light term: */\n\
                color+=(gl_LightSource[<lightIndex>].diffuse*diffuse)*nl;\n\
                \n\
                /* Compute the eye direction: */\n\
                eyeDirEc=normalize(-vertexEc.xyz);\n\
                \n\
                /* Compute the specular lighting angle: */\n\
                nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);\n\
                \n\
                /* Calculate per-source specular lighting term: */\n\
                color+=(gl_LightSource[<lightIndex>].specular*specular)*pow(nhv,gl_FrontMaterial.shininess);\n\
                }\n\
            \n\
            /* Calculate the spot light attenuation: */\n\
            att=pow(sl,gl_LightSource[<lightIndex>].spotExponent);\n\
            color*=att;\n\
            }\n\
        \n\
        /* Return the result color: */\n\
        return color;\n\
        }\n\
    \n";

const char* LightingShader::applyAttenuatedSpotLightTemplate=
    "\
    vec4 applyLight<lightIndex>(const vec4 vertexEc,const vec3 normalEc,const vec4 ambient,const vec4 diffuse,const vec4 specular)\n\
        {\n\
        vec4 color;\n\
        vec3 lightDirEc;\n\
        float sl,nl,att;\n\
        \n\
        /* Calculate per-source ambient light term: */\n\
        color=gl_LightSource[<lightIndex>].ambient*ambient;\n\
        \n\
        /* Compute the light direction (works both for directional and point lights): */\n\
        lightDirEc=gl_LightSource[<lightIndex>].position.xyz*vertexEc.w-vertexEc.xyz*gl_LightSource[<lightIndex>].position.w;\n\
        lightDirEc=normalize(lightDirEc);\n\
        \n\
        /* Calculate the spot light angle: */\n\
        sl=-dot(lightDirEc,normalize(gl_LightSource[<lightIndex>].spotDirection))\n\
        \n\
        /* Check if the point is inside the spot light's cone: */\n\
        if(sl>=gl_LightSource[<lightIndex>].spotCosCutoff)\n\
            {\n\
            /* Compute the diffuse lighting angle: */\n\
            nl=dot(normalEc,lightDirEc);\n\
            if(nl>0.0)\n\
                {\n\
                vec3 eyeDirEc;\n\
                float nhv;\n\
                \n\
                /* Calculate per-source diffuse light term: */\n\
                color+=(gl_LightSource[<lightIndex>].diffuse*diffuse)*nl;\n\
                \n\
                /* Compute the eye direction: */\n\
                eyeDirEc=normalize(-vertexEc.xyz);\n\
                \n\
                /* Compute the specular lighting angle: */\n\
                nhv=max(dot(normalEc,normalize(eyeDirEc+lightDirEc)),0.0);\n\
                \n\
                /* Calculate per-source specular lighting term: */\n\
                color+=(gl_LightSource[<lightIndex>].specular*specular)*pow(nhv,gl_FrontMaterial.shininess);\n\
                }\n\
            \n\
            /* Calculate the spot light attenuation: */\n\
            att=pow(sl,gl_LightSource[<lightIndex>].spotExponent);\n\
            color*=att;\n\
            }\n\
        \n\
        /* Attenuate the per-source light terms: */\n\
        att=(gl_LightSource[<lightIndex>].quadraticAttenuation*lightDist+gl_LightSource[<lightIndex>].linearAttenuation)*lightDist+gl_LightSource[<lightIndex>].constantAttenuation;\n\
        color*=1.0/att;\n\
        \n\
        /* Return the result color: */\n\
        return color;\n\
        }\n\
    \n";

const char* LightingShader::fetchTerrainColorAsConstant =
"\
    vec4 terrainColor = vec4(0.6); //60% grey\n\
";

const char* LightingShader::fetchTerrainColorFromColorMap =
"\
    float e = sampleHeight(coord);\n\
          e = (e-minColorMapElevation) * colorMapElevationInvRange;\n\
    vec4 mapColor    = texture1D(colorMap, e);\n\
    vec3 texColor    = sampleColor(coord).rgb;\n\
    vec4 terrainColor= texColor==colorNodata ? vec4(colorDefault, 1.0) :\n\
                                               vec4(texColor, 1.0);\n\
    terrainColor     = mix(terrainColor, mapColor, mapColor.w);\
";

const char* LightingShader::fetchTerrainColorFromTexture =
"\
    vec3 texColor     = sampleColor(coord).rgb;\n\
    vec4 terrainColor = texColor==colorNodata ? vec4(colorDefault, 1.0) :\n\
                                                vec4(texColor, 1.0);\
";

/*****************************************
Methods of class LightingShader:
*****************************************/

std::string LightingShader::createApplyLightFunction(const char* functionTemplate,int lightIndex) const
    {
    std::string result;

    /* Create the light index string: */
    char index[10];
    snprintf(index,sizeof(index),"%d",lightIndex);

    /* Replace all occurrences of <lightIndex> in the template string with the light index string: */
    const char* match="<lightIndex>";
    const char* matchStart = NULL;
    int matchLen=0;
    for(const char* tPtr=functionTemplate;*tPtr!='\0';++tPtr)
        {
        if(matchLen==0)
            {
            if(*tPtr=='<')
                {
                matchStart=tPtr;
                matchLen=1;
                }
            else
                result.push_back(*tPtr);
            }
        else if(matchLen<12)
            {
            if(*tPtr==match[matchLen])
                {
                ++matchLen;
                if(matchLen==static_cast<int>(strlen(match)))
                    {
                    result.append(index);
                    matchLen=0;
                    }
                }
            else
                {
                for(const char* cPtr=matchStart;cPtr!=tPtr;++cPtr)
                    result.push_back(*cPtr);
                matchLen=0;
                --tPtr;
                }
            }
        }

    return result;
    }

LightingShader::LightingShader() :
    mustRecompile(true),
    colorMaterial(false),
    linesDecorated(false),
    texturingMode(2),
    lightStates(0),
    vertexShader(0),fragmentShader(0),
    programObject(0)
{
    /* Determine the maximum number of light sources supported by the local OpenGL: */
    glGetIntegerv(GL_MAX_LIGHTS,&maxNumLights);

    /* Initialize the light state array: */
    lightStates=new LightState[maxNumLights];
    updateLightingState();

    /* Create the vertex and fragment shaders: */
    vertexShader=glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
    fragmentShader=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

    /* Create the program object: */
    programObject=glCreateProgramObjectARB();
    glAttachObjectARB(programObject,vertexShader);
    glAttachObjectARB(programObject,fragmentShader);
}

LightingShader::~LightingShader()
{
    glDeleteObjectARB(programObject);
    glDeleteObjectARB(vertexShader);
    glDeleteObjectARB(fragmentShader);
    delete[] lightStates;
}


///\todo remove once code is baked in
static time_t lastFPTime;
static bool
checkFileForChanges(const char* fileName)
{
    struct stat statRes;
    if (stat(fileName, &statRes) == 0)
    {
        if (statRes.st_mtime != lastFPTime)
        {
            lastFPTime      = statRes.st_mtime;
            return true;
        }
        else
            return false;
    }
    else
    {
        Misc::throwStdErr("LightingShader: can't find FP to check (%s)",
                          fileName);
        return false;
    }
}

void LightingShader::
update()
{
    /* Update light states and recompile the shader if necessary: */
    updateLightingState();

///\todo bake this into the code
std::string progFile(CRUSTA_SHARE_PATH);
if (linesDecorated)
    progFile += "/decoratedRenderer.fp";
else
    progFile += "/plainRenderer.fp";
mustRecompile |= checkFileForChanges(progFile.c_str());

    if (mustRecompile)
    {
        compileShader();
        mustRecompile = false;
    }
}

void LightingShader::
updateLightingState()
{
    /* Check the color material flag: */
    bool newColorMaterial=glIsEnabled(GL_COLOR_MATERIAL);
    if(newColorMaterial!=colorMaterial)
        mustRecompile=true;
    colorMaterial=newColorMaterial;

    for(int lightIndex=0;lightIndex<maxNumLights;++lightIndex)
    {
        GLenum light=GL_LIGHT0+lightIndex;

        /* Get the light's enabled flag: */
        bool enabled=glIsEnabled(light);
        if(enabled!=lightStates[lightIndex].enabled)
            mustRecompile=true;
        lightStates[lightIndex].enabled=enabled;

        if(enabled)
        {
            /* Determine the light's attenuation and spot light state: */
            bool attenuated=false;
            bool spotLight=false;

            /* Get the light's position: */
            GLfloat pos[4];
            glGetLightfv(light,GL_POSITION,pos);
            if(pos[3]!=0.0f)
            {
                /* Get the light's attenuation coefficients: */
                GLfloat att[3];
                glGetLightfv(light,GL_CONSTANT_ATTENUATION,&att[0]);
                glGetLightfv(light,GL_LINEAR_ATTENUATION,&att[1]);
                glGetLightfv(light,GL_QUADRATIC_ATTENUATION,&att[2]);

                /* Determine whether the light is attenuated: */
                if(att[0]!=1.0f||att[1]!=0.0f||att[2]!=0.0f)
                    attenuated=true;

                /* Get the light's spot light cutoff angle: */
                GLfloat spotLightCutoff;
                glGetLightfv(light,GL_SPOT_CUTOFF,&spotLightCutoff);
                spotLight=spotLightCutoff<=90.0f;
            }

            if(attenuated!=lightStates[lightIndex].attenuated)
                mustRecompile=true;
            lightStates[lightIndex].attenuated=attenuated;

            if(spotLight!=lightStates[lightIndex].spotLight)
                mustRecompile=true;
            lightStates[lightIndex].spotLight=spotLight;
        }
    }
}

///\todo remove once code is baked in
void readFileToString(const char* fileName, std::string& content)
{
    std::ifstream ifs(fileName);
    content.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

void LightingShader::compileShader()
    {
    std::string vertexShaderUniforms;
    std::string vertexShaderFunctions;
    std::string vertexShaderMain;

    vertexShaderUniforms +=
    "\
        #extension GL_EXT_gpu_shader4   : enable\n\
        #extension GL_EXT_texture_array : enable\n\
        \n\
        uniform sampler2DArray geometryTex;\n\
        uniform sampler2DArray heightTex;\n\
        uniform sampler2DArray colorTex;\n\
        \n\
        uniform sampler1D colorMap;\n\
        \n\
        uniform float colorMapElevationInvRange;\n\
        uniform float minColorMapElevation;\n\
        uniform float texStep;\n\
        uniform float verticalScale;\n\
        uniform vec3  center;\n\
        \n\
        uniform float demNodata;\n\
        uniform vec3  colorNodata;\n\
        uniform float demDefault;\n\
        uniform vec3  colorDefault;\n\
        \n\
        uniform vec3 heightTexOffset;\n\
        uniform vec2 heightTexScale;\n\
        uniform vec3 geometryTexOffset;\n\
        uniform vec2 geometryTexScale;\n\
        uniform vec3 colorTexOffset;\n\
        uniform vec2 colorTexScale;\n\
        \n\
        varying vec3 position;\n\
        varying vec3 normal;\n\
        varying vec2 texCoord;\n\
        \n\
    ";

    vertexShaderFunctions +=
    "\
        vec3 sampleGeometry(in vec2 tc)\n\
        {\n\
            vec3 tc2 = geometryTexOffset + vec3((tc * geometryTexScale),0.0);\n\
            return texture2DArray(geometryTex, tc2).xyz;\n\
        }\n\
        float sampleHeight(in vec2 tc)\n\
        {\n\
            vec3 tc2 = heightTexOffset + vec3((tc * heightTexScale),0.0);\n\
            return texture2DArray(heightTex, tc2).x;\n\
        }\n\
        vec4 sampleColor(in vec2 tc)\n\
        {\n\
            vec3 tc2 = colorTexOffset + vec3((tc * colorTexScale),0.0);\n\
            return texture2DArray(colorTex, tc2);\n\
        }\n\
        \n\
        vec3 surfacePoint(in vec2 coords)\n\
        {\n\
            vec3 res      = sampleGeometry(coords);\n\
            vec3 dir      = normalize(center + res);\n\
            float height  = sampleHeight(coords);\n\
            height        = height==demNodata ? demDefault : height;\n\
            height       *= verticalScale;\n\
            res          += height * dir;\n\
            return res;\n\
        }\n\
    ";

    /* Create the main vertex shader starting boilerplate: */
    vertexShaderMain+=
        "\
        void main()\n\
        {\n\
            vec4 vertexEc;\n\
            vec3 normalEc;\n\
            vec4 ambient,diffuse,specular;\n\
            vec4 color;\n\
            vec2 coord = gl_Vertex.xy;\n\
            position   = surfacePoint(coord);\n\
            \n\
            float minTexCoord = texStep * 0.5;\n\
            float maxTexCoord = 1.0 - (texStep * 0.5);\n\
            \n\
            vec2 ncoord = vec2(coord.x, coord.y + texStep);\n\
            ncoord.y    = clamp(ncoord.y, minTexCoord, maxTexCoord);\n\
            vec3 up     = surfacePoint(ncoord);\n\
            \n\
            ncoord      = vec2(coord.x, coord.y - texStep);\n\
            ncoord.y    = clamp(ncoord.y, minTexCoord, maxTexCoord);\n\
            vec3 down   = surfacePoint(ncoord);\n\
            \n\
            ncoord      = vec2(coord.x - texStep, coord.y);\n\
            ncoord.x    = clamp(ncoord.x, minTexCoord, maxTexCoord);\n\
            vec3 left   = surfacePoint(ncoord);\n\
            \n\
            ncoord      = vec2(coord.x + texStep, coord.y);\n\
            ncoord.x    = clamp(ncoord.x, minTexCoord, maxTexCoord);\n\
            vec3 right  = surfacePoint(ncoord);\n\
            \n\
            normal = cross((right - left), (up - down));\n\
            normal = normalize(normal);\n\
            \n\
            /* Compute the vertex position in eye coordinates: */\n\
            vertexEc = gl_ModelViewMatrix * vec4(position, 1.0);\n\
            \n\
            /* Compute the normal vector in eye coordinates: */\n\
            normalEc=normalize(gl_NormalMatrix*normal);\n\
            \n\
            \n";

    /* Get the material components: */
    if(colorMaterial)
        {
        vertexShaderMain+=
            "\
            /* Get the material properties from the current color: */\n\
            ambient=gl_Color;\n\
            diffuse=gl_Color;\n\
            specular=gl_FrontMaterial.specular;\n\
            \n";
        }
    else
        {
        vertexShaderMain+=
            "\
            /* Get the material properties from the material state: */\n\
            ambient=gl_FrontMaterial.ambient;\n\
            diffuse=gl_FrontMaterial.diffuse;\n\
            specular=gl_FrontMaterial.specular;\n\
            \n";
        }

    vertexShaderMain+=
        "\
        /* Modulate with the texture color: */\n\
        \n";

    switch (texturingMode)
    {
        case 0:
            vertexShaderMain+=fetchTerrainColorAsConstant;
            break;
        case 1:
            vertexShaderMain+=fetchTerrainColorFromColorMap;
            break;
        case 2:
            vertexShaderMain+=fetchTerrainColorFromTexture;
            break;
        default:
            vertexShaderMain+="vec4 terrainColor(1.0, 0.0, 0.0, 1.0);";
    }

    vertexShaderMain+=
        "\
        ambient *= terrainColor;\n\
        diffuse *= terrainColor;\n";

    /* Continue the main vertex shader: */
    vertexShaderMain+=
        "\
        /* Calculate global ambient light term: */\n\
        color  = gl_LightModel.ambient*ambient;\n\
        \n\
        /* Apply all enabled light sources: */\n";

    /* Create light application functions for all enabled light sources: */
    for(int lightIndex=0;lightIndex<maxNumLights;++lightIndex)
        if(lightStates[lightIndex].enabled)
            {
            /* Create the appropriate light application function: */
            if(lightStates[lightIndex].spotLight)
                {
                if(lightStates[lightIndex].attenuated)
                    vertexShaderFunctions+=createApplyLightFunction(applyAttenuatedSpotLightTemplate,lightIndex);
                else
                    vertexShaderFunctions+=createApplyLightFunction(applySpotLightTemplate,lightIndex);
                }
            else
                {
                if(lightStates[lightIndex].attenuated)
                    vertexShaderFunctions+=createApplyLightFunction(applyAttenuatedLightTemplate,lightIndex);
                else
                    vertexShaderFunctions+=createApplyLightFunction(applyLightTemplate,lightIndex);
                }

            /* Call the light application function from the shader's main function: */
            char call[256];
            snprintf(call,sizeof(call),"\t\t\tcolor+=applyLight%d(vertexEc,normalEc,ambient,diffuse,specular);\n",lightIndex);
            vertexShaderMain+=call;
            }

    /* Finish the main function: */
    vertexShaderMain+=
        "\
            \n\
            /* Compute final vertex color: */\n\
            gl_FrontColor = color;\n\
            \n\
            /* Use finalize vertex transformation: */\n\
            gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);\n\
            \n\
            /* Pass the texture coordinates to the fragment program: */\n\
            texCoord = gl_Vertex.xy;\n\
            }\n";

    /* Compile the vertex shader: */
    std::string vertexShaderSource = vertexShaderUniforms  +
                                     vertexShaderFunctions +
                                     vertexShaderMain;
    compileShaderFromString(vertexShader,vertexShaderSource.c_str());

    /* Compile the standard fragment shader: */
    std::string fragmentShaderSource;
    std::string progFile(CRUSTA_SHARE_PATH);
    if (linesDecorated)
        progFile += "/decoratedRenderer.fp";
    else
        progFile += "/plainRenderer.fp";
    readFileToString(progFile.c_str(), fragmentShaderSource);

try{
    compileShaderFromString(fragmentShader,fragmentShaderSource.c_str());
}
catch (std::exception& e){
    std::cerr << e.what() << std::endl;
    compileShaderFromString(fragmentShader, "void main(){gl_FragColor=vec4(1.0);}");
}

    /* Link the program object: */
    glLinkProgramARB(programObject);

    /* Check if the program linked successfully: */
    GLint linkStatus;
    glGetObjectParameterivARB(programObject,GL_OBJECT_LINK_STATUS_ARB,&linkStatus);
    if(!linkStatus)
    {
        /* Get some more detailed information: */
        GLcharARB linkLogBuffer[2048];
        GLsizei linkLogSize;
        glGetInfoLogARB(programObject,sizeof(linkLogBuffer),&linkLogSize,linkLogBuffer);

        std::cerr << linkLogBuffer << std::endl;
        compileShaderFromString(fragmentShader,
                                  "void main(){gl_FragColor=vec4(1.0);}");
        glLinkProgram(programObject);
    }

    glUseProgramObjectARB(programObject);
    //setup "constant uniforms"
    GLint uniform;
    uniform = glGetUniformLocation(programObject, "geometryTex");
    glUniform1i(uniform, 0);
    uniform = glGetUniformLocation(programObject, "heightTex");
    glUniform1i(uniform, 1);
    uniform = glGetUniformLocation(programObject, "colorTex");
    glUniform1i(uniform, 2);

    uniform = glGetUniformLocation(programObject, "colorMap");
    glUniform1i(uniform, 6);

    colorMapElevationInvRangeUniform =
        glGetUniformLocation(programObject, "colorMapElevationInvRange");
    minColorMapElevationUniform =
        glGetUniformLocation(programObject, "minColorMapElevation");
    textureStepUniform  =glGetUniformLocation(programObject,"texStep");
    verticalScaleUniform=glGetUniformLocation(programObject,"verticalScale");
    centroidUniform     =glGetUniformLocation(programObject,"center");

    geometryTexOffsetUniform = glGetUniformLocation(programObject,
                                                       "geometryTexOffset");
    geometryTexScaleUniform = glGetUniformLocation(programObject,
                                                      "geometryTexScale");

    heightTexOffsetUniform = glGetUniformLocation(programObject,
                                                     "heightTexOffset");
    heightTexScaleUniform = glGetUniformLocation(programObject,
                                                    "heightTexScale");

    colorTexOffsetUniform = glGetUniformLocation(programObject,
                                                    "colorTexOffset");
    colorTexScaleUniform = glGetUniformLocation(programObject,
                                                   "colorTexScale");

    demNodataUniform    = glGetUniformLocationARB(programObject,"demNodata");
    colorNodataUniform  = glGetUniformLocationARB(programObject,"colorNodata");
    demDefaultUniform   = glGetUniformLocationARB(programObject,"demDefault");
    colorDefaultUniform = glGetUniformLocationARB(programObject,"colorDefault");


    if (linesDecorated)
    {
        uniform = glGetUniformLocation(programObject, "lineDataTex");
        glUniform1i(uniform, 3);
        uniform = glGetUniformLocation(programObject, "lineCoverageTex");
        glUniform1i(uniform, 4);
        uniform = glGetUniformLocation(programObject, "symbolTex");
        glUniform1i(uniform, 5);

        uniform = glGetUniformLocation(programObject, "lineStartCoord");
        glUniform1f(uniform, crusta::SETTINGS->lineDataStartCoord);
        uniform = glGetUniformLocation(programObject, "lineCoordStep");
        glUniform1f(uniform, crusta::SETTINGS->lineDataCoordStep);

        lineNumSegmentsUniform =
            glGetUniformLocation(programObject, "lineNumSegments");
        lineCoordScaleUniform =
            glGetUniformLocation(programObject, "lineCoordScale");
        lineWidthUniform = glGetUniformLocation(programObject, "lineWidth");

        coverageTexOffsetUniform = glGetUniformLocation(
            programObject, "coverageTexOffset");
        coverageTexScaleUniform = glGetUniformLocation(
            programObject, "coverageTexScale");
        lineDataTexOffsetUniform = glGetUniformLocation(
            programObject, "lineDataTexOffset");
        lineDataTexScaleUniform = glGetUniformLocation(
            programObject, "lineDataTexScale");
    }

    glUseProgramObjectARB(0);
}

void LightingShader::compileShaderFromString(GLhandleARB shaderObject,const char* shaderSource)
{
    /* Determine the length of the source string: */
    GLint shaderSourceLength=GLint(strlen(shaderSource));

    /* Upload the shader source into the shader object: */
    const GLcharARB* ss=reinterpret_cast<const GLcharARB*>(shaderSource);
    glShaderSource(shaderObject,1,&ss,&shaderSourceLength);

    /* Compile the shader source: */
    glCompileShader(shaderObject);

    /* Check if the shader compiled successfully: */
    GLint compileStatus;
    glGetObjectParameterivARB(shaderObject,GL_OBJECT_COMPILE_STATUS_ARB,
                              &compileStatus);
    if(!compileStatus)
    {
        /* Get some more detailed information: */
        GLcharARB compileLogBuffer[2048];
        GLsizei compileLogSize;
        glGetInfoLogARB(shaderObject, sizeof(compileLogBuffer),
                        &compileLogSize, compileLogBuffer);

        /* Signal an error: */
        Misc::throwStdErr("glCompileShaderFromString: Error \"%s\" while compiling shader",compileLogBuffer);
    }
}

void LightingShader::enable()
    {
    /* Enable the shader: */
    glUseProgramObjectARB(programObject);
    }

void LightingShader::disable()
    {
    /* Disable the shader: */
    glUseProgramObjectARB(0);
    }


END_CRUSTA
