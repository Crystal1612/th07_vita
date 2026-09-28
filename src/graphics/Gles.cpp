#include "Gles.hpp"

#include "AnmManager.hpp"
#include "GameWindow.hpp"
#include "Supervisor.hpp"

const char *vertexShaderSource =
    "struct VertexInput {\n"
    "    half3 a_Position : POSITION;\n"
    "    half4 a_Color : COLOR0;\n"
    "    half2 a_TexCoord : TEXCOORD0;\n"
    "};\n"
    "\n"
    "struct VertexOutput {\n"
    "    half4 position : POSITION;\n"
    "    half4 v_Color : COLOR0;\n"
    "    half2 v_TexCoord : TEXCOORD0;\n"
    "    half v_FogFragCoord : TEXCOORD1;\n"
    "};\n"
    "\n"
    "VertexOutput main(\n"
    "    VertexInput input,\n"
    "    uniform float4x4 u_Model,\n"
    "    uniform float4x4 u_View,\n"
    "    uniform float4x4 u_Proj,\n"
    "    uniform float4x4 u_TextureMatrix,\n"
    "    uniform bool u_ScreenSpace,\n"
    "    uniform half4 u_Viewport\n"
    ") {\n"
    "    VertexOutput output;\n"
    "    output.v_Color = input.a_Color;\n"
    "\n"
    "    if (u_ScreenSpace) {\n"
    "        half x = (input.a_Position.x - u_Viewport.x) / u_Viewport.z * 2.0 - 1.0;\n"
    "        half y = 1.0 - (input.a_Position.y - u_Viewport.y) / u_Viewport.w * 2.0;\n"
    "        output.position = half4(x, y, input.a_Position.z, 1.0);\n"
    "        output.v_TexCoord = input.a_TexCoord;\n"
    "        output.v_FogFragCoord = input.a_Position.z;\n"
    "    } else {\n"
    "        half4 worldPos = mul(u_Model, half4(input.a_Position, 1.0));\n"
    "        half4 viewPos = mul(u_View, worldPos);\n"
    "        output.position = mul(u_Proj, viewPos);\n"
    "        output.v_TexCoord = mul(u_TextureMatrix, half4(input.a_TexCoord, 1.0, 0.0)).xy;\n"
    "        output.v_FogFragCoord = length(viewPos.xyz);\n"
    "    }\n"
    "\n"
    "    return output;\n"
    "}\n";

const char *fragmentShaderSource =
    "struct VertexOutput {\n"
    "    half4 v_Color : COLOR0;\n"
    "    half2 v_TexCoord : TEXCOORD0;\n"
    "    half v_FogFragCoord : TEXCOORD1;\n"
    "};\n"
    "\n"
    "half4 main(\n"
    "    VertexOutput input,\n"
    "    uniform sampler2D u_Texture : TEXUNIT0,\n"
    "    uniform bool u_UseTexture,\n"
    "    uniform int u_ColorOpRgb,\n"
    "    uniform int u_ColorOpAlpha,\n"
    "    uniform int u_TexArg,\n"
    "    uniform half4 u_TextureFactor,\n"
    "    uniform bool u_AlphaTest,\n"
    "    uniform half u_AlphaRef,\n"
    "    uniform bool u_FogEnabled,\n"
    "    uniform half4 u_FogColor,\n"
    "    uniform half u_FogNear,\n"
    "    uniform half u_FogFar\n"
    ") : COLOR {\n"
    "    half4 texColor = half4(1.0, 1.0, 1.0, 1.0);\n"
    "    if (u_UseTexture) {\n"
    "        texColor = tex2D(u_Texture, input.v_TexCoord);\n"
    "    }\n"
    "\n"
    "    half4 argColor = input.v_Color.bgra;\n"
    "    if (u_TexArg == 1) {\n"
    "        argColor = half4(1.0, 1.0, 1.0, 1.0);\n"
    "    } else if (u_TexArg == 2) {\n"
    "        argColor = u_TextureFactor;\n"
    "    }\n"
    "\n"
    "    half4 finalColor = input.v_Color;\n"
    "\n"
    "    if (u_UseTexture) {\n"
    "        if (u_ColorOpRgb == 0) {\n"
    "            finalColor.rgb = texColor.rgb * argColor.rgb;\n"
    "        } else if (u_ColorOpRgb == 1) {\n"
    "            finalColor.rgb = min(texColor.rgb + argColor.rgb, half3(1.0, 1.0, 1.0));\n"
    "        } else if (u_ColorOpRgb == 2) {\n"
    "            finalColor.rgb = texColor.rgb;\n"
    "        } else if (u_ColorOpRgb == 3) {\n"
    "            finalColor.rgb = argColor.rgb;\n"
    "        }\n"
    "\n"
    "        if (u_ColorOpAlpha == 0) {\n"
    "            finalColor.a = texColor.a * argColor.a;\n"
    "        } else if (u_ColorOpAlpha == 1) {\n"
    "            finalColor.a = min(texColor.a + argColor.a, 1.0);\n"
    "        } else if (u_ColorOpAlpha == 2) {\n"
    "            finalColor.a = texColor.a;\n"
    "        } else if (u_ColorOpAlpha == 3) {\n"
    "            finalColor.a = argColor.a;\n"
    "        }\n"
    "    } else {\n"
    "        finalColor = argColor;\n"
    "    }\n"
    "\n"
    "    if (u_AlphaTest && finalColor.a < u_AlphaRef) {\n"
    "        discard;\n"
    "    }\n"
    "\n"
    "    if (u_FogEnabled) {\n"
    "        half f = (u_FogFar - input.v_FogFragCoord) / (u_FogFar - u_FogNear);\n"
    "        f = clamp(f, 0.0, 1.0);\n"
    "        finalColor.rgb = lerp(u_FogColor.rgb, finalColor.rgb, f);\n"
    "    }\n"
    "\n"
    "    return finalColor;\n"
    "}\n";

const char *screenVsSource =
    "struct VertexInput {\n"
    "    half2 a_Position : POSITION;\n"
    "    half2 a_TexCoord : TEXCOORD0;\n"
    "};\n"
    "struct VertexOutput {\n"
    "    half4 position : POSITION;\n"
    "    half2 v_TexCoord : TEXCOORD0;\n"
    "};\n"
    "VertexOutput main(VertexInput input) {\n"
    "    VertexOutput output;\n"
    "    output.position = half4(input.a_Position, 0.0, 1.0);\n"
    "    output.v_TexCoord = input.a_TexCoord;\n"
    "    return output;\n"
    "}\n";

const char *screenFsSource =
    "struct VertexOutput {\n"
    "    half2 v_TexCoord : TEXCOORD0;\n"
    "};\n"
    "half4 main(VertexOutput input, uniform sampler2D u_ScreenTex : TEXUNIT0) : COLOR {\n"
    "    half4 c = tex2D(u_ScreenTex, input.v_TexCoord);\n"
    "    return half4(c.rgb, 1.0);\n"
    "}\n";

ZunGraphics *GlesGraphics::Init()
{
    GlesGraphics *gfx = new GlesGraphics;

    SDL_GLContext ctx = SDL_GL_CreateContext(g_GameWindow.window);
    if (!ctx)
    {
        delete gfx;
        Supervisor::DebugPrint("gles renderer create failed: %s\n", SDL_GetError());
        return nullptr;
    }
    gfx->ctx = ctx;

    SDL_GL_MakeCurrent(g_GameWindow.window, ctx);

    if (SDL_GL_SetSwapInterval(1) < 0)
    {
        Supervisor::DebugPrint("SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError());
    }

    RenderVertexInfo unitQuadData[4] = {{{-128.0f, -128.0f, 0.0f}, {0.0f, 0.0f}},
                                        {{128.0f, -128.0f, 0.0f}, {1.0f, 0.0f}},
                                        {{-128.0f, 128.0f, 0.0f}, {0.0f, 1.0f}},
                                        {{128.0f, 128.0f, 0.0f}, {1.0f, 1.0f}}};

    glGenBuffers(1, &gfx->unitQuadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, gfx->unitQuadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(unitQuadData), unitQuadData, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &gfx->vbo);

    u32 vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    u32 fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (vertexShader == 0 || fragmentShader == 0)
    {
        return nullptr;
    }

    gfx->shaderProgram = glCreateProgram();
    glAttachShader(gfx->shaderProgram, vertexShader);
    glAttachShader(gfx->shaderProgram, fragmentShader);

    glBindAttribLocation(gfx->shaderProgram, 0, "a_Position");
    glBindAttribLocation(gfx->shaderProgram, 1, "a_Color");
    glBindAttribLocation(gfx->shaderProgram, 2, "a_TexCoord");

    glLinkProgram(gfx->shaderProgram);
    glUseProgram(gfx->shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    gfx->u_Model = glGetUniformLocation(gfx->shaderProgram, "u_Model");
    gfx->u_View = glGetUniformLocation(gfx->shaderProgram, "u_View");
    gfx->u_Proj = glGetUniformLocation(gfx->shaderProgram, "u_Proj");
    gfx->u_TextureMatrix = glGetUniformLocation(gfx->shaderProgram, "u_TextureMatrix");
    gfx->u_ScreenSpace = glGetUniformLocation(gfx->shaderProgram, "u_ScreenSpace");
    gfx->u_Viewport = glGetUniformLocation(gfx->shaderProgram, "u_Viewport");
    gfx->u_UseTexture = glGetUniformLocation(gfx->shaderProgram, "u_UseTexture");
    gfx->u_Texture = glGetUniformLocation(gfx->shaderProgram, "u_Texture");
    gfx->u_ColorOpRgb = glGetUniformLocation(gfx->shaderProgram, "u_ColorOpRgb");
    gfx->u_ColorOpAlpha = glGetUniformLocation(gfx->shaderProgram, "u_ColorOpAlpha");
    gfx->u_TexArg = glGetUniformLocation(gfx->shaderProgram, "u_TexArg");
    gfx->u_TextureFactor = glGetUniformLocation(gfx->shaderProgram, "u_TextureFactor");
    gfx->u_AlphaTest = glGetUniformLocation(gfx->shaderProgram, "u_AlphaTest");
    gfx->u_AlphaRef = glGetUniformLocation(gfx->shaderProgram, "u_AlphaRef");
    gfx->u_FogEnabled = glGetUniformLocation(gfx->shaderProgram, "u_FogEnabled");
    gfx->u_FogColor = glGetUniformLocation(gfx->shaderProgram, "u_FogColor");
    gfx->u_FogNear = glGetUniformLocation(gfx->shaderProgram, "u_FogNear");
    gfx->u_FogFar = glGetUniformLocation(gfx->shaderProgram, "u_FogFar");

    for (i32 i = 0; i < 4; i++)
    {
        gfx->transforms[i].Identity();
    }

    glUniform1i(gfx->u_Texture, 0);

    Supervisor::DebugPrint("using gles 2.0 (pib) rendering.\n");

    gfx->windowed = g_Supervisor.cfg.windowed;

    gfx->InitFBO();
    return gfx;
}

void GlesGraphics::Exit()
{
    glDeleteFramebuffers(1, &this->fbo);
    glDeleteTextures(1, &this->fboColorTex);
    glDeleteRenderbuffers(1, &this->fboDepthRb);
    glDeleteBuffers(1, &this->screenQuadVbo);
    glDeleteProgram(this->screenProgram);

    SDL_GL_DeleteContext(this->ctx);
}

void GlesGraphics::SetFogRange(f32 nearPlane, f32 farPlane)
{
    fogNear = nearPlane;
    fogFar = farPlane;
}

void GlesGraphics::SetFogColor(ZunColor color)
{
    fogColor = color;
}

void GlesGraphics::SetColorOp(TextureOpComponent component, ColorOp op)
{
    if (component == COMPONENT_RGB)
    {
        colorOpRgb = op;
    }
    else
    {
        colorOpAlpha = op;
    }
}

void GlesGraphics::SetTextureFactor(ZunColor factor)
{
    textureFactor = factor;
}

void GlesGraphics::SetTextureArg(TextureArg arg)
{
    texArg = arg;
}

void GlesGraphics::SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix)
{
    transforms[type] = matrix;
}

void GlesGraphics::SetTextureFilter()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void GlesGraphics::GetViewport(ZunViewport &viewport)
{
    viewport = this->viewport;
}

void GlesGraphics::SetViewport(const ZunViewport &viewport)
{
    this->viewport = viewport;
    GLint vy = 480 - (viewport.y + viewport.height);

    glViewport(viewport.x, vy, viewport.width, viewport.height);

    glEnable(GL_SCISSOR_TEST);
    glScissor(viewport.x, vy, viewport.width, viewport.height);
}

void GlesGraphics::Enable(Capabilities cap)
{
    switch (cap)
    {
    case CAPS_BLEND:
        glEnable(GL_BLEND);
        break;
    case CAPS_DEPTH_TEST:
        glEnable(GL_DEPTH_TEST);
        break;
    case CAPS_ALPHA_TEST:
        alphaTestEnabled = true;
        break;
    case CAPS_FOG:
        fogEnabled = true;
        break;
    }
}

void GlesGraphics::Disable(Capabilities cap)
{
    switch (cap)
    {
    case CAPS_BLEND:
        glDisable(GL_BLEND);
        break;
    case CAPS_DEPTH_TEST:
        glDisable(GL_DEPTH_TEST);
        break;
    case CAPS_ALPHA_TEST:
        alphaTestEnabled = false;
        break;
    case CAPS_FOG:
        fogEnabled = false;
        break;
    }
}

void GlesGraphics::SetBlendMode(BlendMode srcMode, BlendMode dstMode)
{
    Flush();

    GLenum glSrcMode = GL_SRC_ALPHA;
    switch (srcMode)
    {
    case BLEND_ALPHA:
        glSrcMode = GL_SRC_ALPHA;
        break;
    case BLEND_ONE:
        glSrcMode = GL_ONE;
        break;
    case BLEND_NONE:
        glSrcMode = GL_ONE;
        break;
    }

    GLenum glDstMode = GL_ONE_MINUS_SRC_ALPHA;
    switch (dstMode)
    {
    case BLEND_ALPHA:
        glDstMode = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case BLEND_ONE:
        glDstMode = GL_ONE;
        break;
    case BLEND_NONE:
        glDstMode = GL_ZERO;
        break;
    }
    glBlendFunc(glSrcMode, glDstMode);
}
void GlesGraphics::SetDepthMask(bool enable)
{
    Flush();

    depthMaskEnabled = enable;
    glDepthMask(enable);
}

void GlesGraphics::SetDepthFunc(DepthFunc func)
{
    Flush();

    switch (func)
    {
    case DEPTH_FUNC_LEQUAL:
        glDepthFunc(GL_LEQUAL);
        break;
    case DEPTH_FUNC_ALWAYS:
        glDepthFunc(GL_ALWAYS);
        break;
    }
}

void GlesGraphics::SetClearDepth(f32 depth)
{
    glClearDepthf(depth);
}

void GlesGraphics::SetClearColor(ZunColor color)
{
    glClearColor(color.bytes.r / 255.0f, color.bytes.g / 255.0f, color.bytes.b / 255.0f,
                 color.bytes.a / 255.0f);
}

void GlesGraphics::SetAlphaTestRef(u8 ref)
{
    alphaRef = ref;
}

void GlesGraphics::Clear(u32 clearBits)
{
    GLbitfield bits = 0;
    if (clearBits & CLEAR_COLOR_BUFFER)
    {
        bits |= GL_COLOR_BUFFER_BIT;
    }
    if (clearBits & CLEAR_DEPTH_BUFFER)
    {
        bits |= GL_DEPTH_BUFFER_BIT;
        if (!depthMaskEnabled)
        {
            glDepthMask(GL_TRUE);
        }
    }
    glClear(bits);
    if ((clearBits & CLEAR_DEPTH_BUFFER) && !depthMaskEnabled)
    {
        glDepthMask(GL_FALSE);
    }
}

GfxTextureHandle GlesGraphics::CreateTexture()
{
    GLuint tex;
    glGenTextures(1, &tex);
    return GfxTextureHandle(tex);
}

void GlesGraphics::BindTexture(GfxTextureHandle handle)
{
    glBindTexture(GL_TEXTURE_2D, handle.id);
}

void GlesGraphics::DeleteTexture(GfxTextureHandle handle)
{
    GLuint tex = handle.id;
    glDeleteTextures(1, &tex);
}

void GlesGraphics::SetTextureImage(u32 width, u32 height, PixelFormat fmt, PixelDataType type,
                                   const void *data)
{
    GLenum internalformat;
    GLenum format;
    GLenum datatype;

    switch (fmt)
    {
    case PIXEL_RGB:
        internalformat = GL_RGB;
        format = GL_RGB;
        break;
    case PIXEL_RGBA:
    default:
        internalformat = GL_RGBA;
        format = GL_RGBA;
        break;
    }

    switch (type)
    {
    case PIXEL_UNSIGNED_BYTE:
        datatype = GL_UNSIGNED_BYTE;
        break;
    case PIXEL_UNSIGNED_SHORT_5_5_5_1:
        datatype = GL_UNSIGNED_SHORT_5_5_5_1;
        break;
    case PIXEL_UNSIGNED_SHORT_5_6_5:
        datatype = GL_UNSIGNED_SHORT_5_6_5;
        break;
    case PIXEL_UNSIGNED_SHORT_4_4_4_4:
        datatype = GL_UNSIGNED_SHORT_4_4_4_4;
        break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, datatype, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void GlesGraphics::SetTextureSubImage(i32 xoffset, i32 yoffset, i32 width, i32 height,
                                      const void *data)
{
    glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                    data);
}

void GlesGraphics::ReadPixels(i32 x, i32 y, i32 width, i32 height, void *pixels)
{
    glReadPixels(x, 480 - (y + height), width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    u32 rowSize = width * 4;
    u8 *p = (u8 *)pixels;
    u8 *tempRow = new u8[rowSize];
    for (i32 i = 0; i < height / 2; ++i)
    {
        u8 *top = p + i * rowSize;
        u8 *bottom = p + (height - 1 - i) * rowSize;
        memcpy(tempRow, top, rowSize);
        memcpy(top, bottom, rowSize);
        memcpy(bottom, tempRow, rowSize);
    }
    delete[] tempRow;
}

void GlesGraphics::DrawPrimitive(PrimitiveType type, i32 startVertex, i32 primitiveCount)
{
    i32 vertexCount = 0;
    GLenum glMode = GL_TRIANGLES;

    if (type == PRIM_TRIANGLES)
    {
        vertexCount = primitiveCount * 3;
        glMode = GL_TRIANGLES;
    }
    else if (type == PRIM_TRIANGLE_STRIP)
    {
        vertexCount = primitiveCount + 2;
        glMode = GL_TRIANGLE_STRIP;
    }
    else if (type == PRIM_TRIANGLE_FAN)
    {
        vertexCount = primitiveCount + 2;
        glMode = GL_TRIANGLE_FAN;
    }

    glBindBuffer(GL_ARRAY_BUFFER, unitQuadVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertexInfo),
                          (void *)offsetof(RenderVertexInfo, pos));

    glDisableVertexAttribArray(1);
    glVertexAttrib4f(1, 1.0f, 1.0f, 1.0f, 1.0f);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(RenderVertexInfo),
                          (void *)offsetof(RenderVertexInfo, textureUV));

    glUniform1i(u_ScreenSpace, false);
    glUniform1i(u_UseTexture, true);

    glUniform4f(u_Viewport, (f32)viewport.x, (f32)viewport.y, (f32)viewport.width,
                (f32)viewport.height);

    GLfloat glModel[16];
    GLfloat glView[16];
    GLfloat glProj[16];
    GLfloat glTexMat[16];

    TransposeMatrix(glModel, transforms[MATRIX_MODEL]);
    TransposeMatrix(glView, transforms[MATRIX_VIEW]);
    TransposeMatrix(glProj, transforms[MATRIX_PROJECTION]);
    TransposeMatrix(glTexMat, transforms[MATRIX_TEXTURE]);

    glUniformMatrix4fv(u_Model, 1, GL_FALSE, glModel);
    glUniformMatrix4fv(u_View, 1, GL_FALSE, glView);
    glUniformMatrix4fv(u_Proj, 1, GL_FALSE, glProj);
    glUniformMatrix4fv(u_TextureMatrix, 1, GL_FALSE, glTexMat);

    glUniform1i(u_ColorOpRgb, colorOpRgb);
    glUniform1i(u_ColorOpAlpha, colorOpAlpha);
    glUniform1i(u_TexArg, texArg);

    glUniform4f(u_TextureFactor, textureFactor.bytes.r / 255.0f, textureFactor.bytes.g / 255.0f,
                textureFactor.bytes.b / 255.0f, textureFactor.bytes.a / 255.0f);

    glUniform1i(u_AlphaTest, alphaTestEnabled);
    glUniform1f(u_AlphaRef, alphaRef / 255.0f);

    glUniform1i(u_FogEnabled, fogEnabled);
    glUniform4f(u_FogColor, fogColor.bytes.r / 255.0f, fogColor.bytes.g / 255.0f,
                fogColor.bytes.b / 255.0f, fogColor.bytes.a / 255.0f);
    glUniform1f(u_FogNear, fogNear);
    glUniform1f(u_FogFar, fogFar);

    glDrawArrays(glMode, startVertex, vertexCount);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlesGraphics::DrawPrimitiveUP(PrimitiveType type, i32 primitiveCount, const void *vertexData,
                                   i32 vertexStride)
{
    i32 vertexCount = 0;
    GLenum glMode = GL_TRIANGLES;

    if (type == PRIM_TRIANGLES)
    {
        vertexCount = primitiveCount * 3;
        glMode = GL_TRIANGLES;
    }
    else if (type == PRIM_TRIANGLE_STRIP)
    {
        vertexCount = primitiveCount + 2;
        glMode = GL_TRIANGLE_STRIP;
    }
    else if (type == PRIM_TRIANGLE_FAN)
    {
        vertexCount = primitiveCount + 2;
        glMode = GL_TRIANGLE_FAN;
    }

    bool isScreenSpace = false;
    bool hasTex = false;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * vertexStride, vertexData, GL_DYNAMIC_DRAW);

    if (vertexStride == sizeof(VertexTex1DiffuseXyzrhw))
    {
        isScreenSpace = true;
        hasTex = true;

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride,
                              (void *)offsetof(VertexTex1DiffuseXyzrhw, pos));
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertexStride,
                              (void *)offsetof(VertexTex1DiffuseXyzrhw, diffuse));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexStride,
                              (void *)offsetof(VertexTex1DiffuseXyzrhw, textureUV));
    }
    else if (vertexStride == sizeof(VertexTex1DiffuseXyz))
    {
        isScreenSpace = false;
        hasTex = true;

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride,
                              (void *)offsetof(VertexTex1DiffuseXyz, pos));
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertexStride,
                              (void *)offsetof(VertexTex1DiffuseXyz, diffuse));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexStride,
                              (void *)offsetof(VertexTex1DiffuseXyz, textureUV));
    }
    else if (vertexStride == sizeof(VertexDiffuseXyzrhw))
    {
        isScreenSpace = true;
        hasTex = false;

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDisableVertexAttribArray(2);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride,
                              (void *)offsetof(VertexDiffuseXyzrhw, pos));
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertexStride,
                              (void *)offsetof(VertexDiffuseXyzrhw, diffuse));
    }

    glUniform1i(u_ScreenSpace, isScreenSpace);
    glUniform1i(u_UseTexture, hasTex);

    glUniform4f(u_Viewport, (f32)viewport.x, (f32)viewport.y, (f32)viewport.width,
                (f32)viewport.height);

    glUniformMatrix4fv(u_Model, 1, GL_FALSE, (GLfloat *)&transforms[MATRIX_MODEL]);
    glUniformMatrix4fv(u_View, 1, GL_FALSE, (GLfloat *)&transforms[MATRIX_VIEW]);
    glUniformMatrix4fv(u_Proj, 1, GL_FALSE, (GLfloat *)&transforms[MATRIX_PROJECTION]);
    glUniformMatrix4fv(u_TextureMatrix, 1, GL_FALSE, (GLfloat *)&transforms[MATRIX_TEXTURE]);

    glUniform1i(u_ColorOpRgb, colorOpRgb);
    glUniform1i(u_ColorOpAlpha, colorOpAlpha);
    glUniform1i(u_TexArg, texArg);

    glUniform4f(u_TextureFactor, textureFactor.bytes.r / 255.0f, textureFactor.bytes.g / 255.0f,
                textureFactor.bytes.b / 255.0f, textureFactor.bytes.a / 255.0f);

    glUniform1i(u_AlphaTest, alphaTestEnabled);
    glUniform1f(u_AlphaRef, alphaRef / 255.0f);

    glUniform1i(u_FogEnabled, fogEnabled);
    glUniform4f(u_FogColor, fogColor.bytes.r / 255.0f, fogColor.bytes.g / 255.0f,
                fogColor.bytes.b / 255.0f, fogColor.bytes.a / 255.0f);
    glUniform1f(u_FogNear, fogNear);
    glUniform1f(u_FogFar, fogFar);

    glDrawArrays(glMode, 0, vertexCount);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlesGraphics::SwapBuffers()
{
    glDisable(GL_SCISSOR_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 960, 544);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!windowed)
    {
        const float scale = 544.0f / 480.0f;
        const GLsizei targetW = (GLsizei)(640.0f * scale);
        const GLsizei targetH = 544;
        const GLint offsetX = (960 - targetW) / 2;
        glViewport(offsetX, 0, targetW, targetH);
    }
    else
    {
        glViewport((960 - 640) / 2, (544 - 480) / 2, 640, 480);
    }

    glDisable(GL_DEPTH_TEST);

    glUseProgram(screenProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboColorTex);
    glUniform1i(u_ScreenTex, 0);

    glBindBuffer(GL_ARRAY_BUFFER, screenQuadVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

    glDisableVertexAttribArray(2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

    SDL_GL_SwapWindow(g_GameWindow.window);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glUseProgram(shaderProgram);

    glViewport(0, 0, 640, 480);
    glScissor(0, 0, 640, 480);
    if (this->viewport.width > 0 && this->viewport.height > 0)
    {
        this->SetViewport(this->viewport);
    }
}

void GlesGraphics::InitFBO()
{
    glGenTextures(1, &fboColorTex);
    glBindTexture(GL_TEXTURE_2D, fboColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 640, 480, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenRenderbuffers(1, &fboDepthRb);
    glBindRenderbuffer(GL_RENDERBUFFER, fboDepthRb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 640, 480);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboColorTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fboDepthRb);

    struct ScreenVertex {
        float x, y;
        float u, v;
    };
    ScreenVertex quadVerts[4] = {
        {-1.0f, -1.0f, 0.0f, 0.0f},
        { 1.0f, -1.0f, 1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f, 1.0f},
        { 1.0f,  1.0f, 1.0f, 1.0f}
    };
    glGenBuffers(1, &screenQuadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, screenQuadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    u32 vs = CompileShader(GL_VERTEX_SHADER, screenVsSource);
    u32 fs = CompileShader(GL_FRAGMENT_SHADER, screenFsSource);
    screenProgram = glCreateProgram();
    glAttachShader(screenProgram, vs);
    glAttachShader(screenProgram, fs);
    glBindAttribLocation(screenProgram, 0, "a_Position");
    glBindAttribLocation(screenProgram, 1, "a_TexCoord");
    glLinkProgram(screenProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    u_ScreenTex = glGetUniformLocation(screenProgram, "u_ScreenTex");

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}