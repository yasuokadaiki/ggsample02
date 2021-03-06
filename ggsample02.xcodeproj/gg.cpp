/*
** ゲームグラフィックス特論用補助プログラム
**

Copyright (c) 2011 Kohe Tokoi. All Rights Reserved.

Permission is hereby granted, free of charge,  to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction,  including without limitation the rights 
to use, copy,  modify, merge,  publish, distribute,  sublicense,  and/or sell 
copies or substantial portions of the Software.

The above  copyright notice  and this permission notice  shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS IS",  WITHOUT WARRANTY OF ANY KIND,  EXPRESS OR 
IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO THE WARRANTIES  OF MERCHANTABILITY, 
FITNESS  FOR  A PARTICULAR PURPOSE  AND NONINFRINGEMENT.  IN  NO EVENT  SHALL 
KOHE TOKOI  BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY,  WHETHER IN 
AN ACTION  OF CONTRACT,  TORT  OR  OTHERWISE,  ARISING  FROM,  OUT OF  OR  IN 
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**
*/

#include <iostream>
#include <fstream>
#include <cstdlib>

#include "gg.h"

/*
** ゲームグラフィックス特論の都合にもとづく初期化
*/
void gg::ggInit(void)
{
  // バージョンチェック
  if (atof(reinterpret_cast<const char *>(glGetString(GL_VERSION))) < 2.1)
  {
    std::cerr << "Error: This program requires OpenGL 2.1 or lator." << std::endl;
    exit(1);
  }

  // Swap Interval の設定
#if defined(X11)
#  if !defined(__APPLE__)
  Display *dpy = glXGetCurrentDisplay();
  GLXDrawable drawable = glXGetCurrentDrawable();
  if (drawable)
  {
    glXSwapIntervalEXT(dpy, drawable, 1);
    return;
  }
#  endif
#elif defined(__APPLE__)
  int swap_interval = 1;
  CGLContextObj cgl_context = CGLGetCurrentContext();
  if (!CGLSetParameter(cgl_context, kCGLCPSwapInterval, &swap_interval))
    return;
#elif defined(WIN32)
  GLenum err = glewInit();
  if (err != GLEW_OK)
  {
    std::cerr <<  "Error: " << glewGetErrorString(err) << std::endl;
    exit(1);
  }
#  if defined(WGLEW_EXT_swap_control)
  if (WGLEW_EXT_swap_control)
  {
    wglSwapIntervalEXT(1);
    return;
  }
#  endif
#else
  std::cerr << "Warning: Could not set swap interval" << std::endl;
#endif
}

/*
** OpenGL のエラーチェック
*/
void gg::ggError(const char *msg)
{
  GLenum error = glGetError();

  if (error != GL_NO_ERROR)
  {
    if (msg) std::cerr << msg << ": ";

    switch (error)
    {
    case GL_INVALID_ENUM:
      std::cerr << "An unacceptable value is specified for an enumerated argument" << std::endl;
      break;
    case GL_INVALID_VALUE:
      std::cerr << "A numeric argument is out of range" << std::endl;
      break;
    case GL_INVALID_OPERATION:
      std::cerr << "The specified operation is not allowed in the current state" << std::endl;
      break;
    case GL_STACK_OVERFLOW:
      std::cerr << "This command would cause a stack overflow" << std::endl;
      break;
    case GL_STACK_UNDERFLOW:
      std::cerr << "This command would cause a a stack underflow" << std::endl;
      break;
    case GL_OUT_OF_MEMORY:
      std::cerr << "There is not enough memory left to execute the command" << std::endl;
      break;
    case GL_TABLE_TOO_LARGE:
      std::cerr << "The specified table exceeds the implementation's maximum supported table size" << std::endl;
      break;
#ifndef GL_INVALID_FRAMEBUFFER_OPERATION
#    define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#endif
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      std::cerr << "The specified operation is not allowed current frame buffer" << std::endl;
      break;
    default:
      std::cerr << "An OpenGL error has occured: " << std::hex << std::showbase << error << std::endl;
      break;
    }
  }
}

/*
** FBO のエラーチェック
*/
void gg::ggFBOError(const char *msg)
{
  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

  if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
  {
    if (msg) std::cerr << msg << ": ";

    switch (status)
    {
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
      std::cerr << "Unsupported framebuffer format" << std::endl;
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
      std::cerr << "Framebuffer incomplete, missing attachment" << std::endl;
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
      std::cerr << "Framebuffer incomplete, duplicate attachment" << std::endl;
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
      std::cerr << "Framebuffer incomplete, attached images must have same dimensions" << std::endl;
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
      std::cerr << "Framebuffer incomplete, attached images must have same format" << std::endl;
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
      std::cerr << "Framebuffer incomplete, missing draw buffer" << std::endl;
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
      std::cerr << "Framebuffer incomplete, missing read buffer" << std::endl;
      break;
    default:
      std::cerr << "Programming error; will fail on all hardware: " << std::hex << std::showbase << status << std::endl;
      break;
    }
  }
}

/*
** シェーダーのソースプログラムをメモリに読み込む
*/
static bool readShaderSource(GLuint shader, const char *name)
{
  bool ret = true; // 終了ステータス（true ならエラー）
  
  // ソースファイルを開く
  std::ifstream file(name, std::ios::binary);

  if (file.fail()) {
    // 開けなかった
    std::cerr << "Error: Can't open source file: " << name << std::endl;
  }
  else {
    // ファイルの末尾に移動し現在位置（＝ファイルサイズ）を得る
    file.seekg(0L, std::ios::end);
    GLsizei length = file.tellg();
    
    // ファイルサイズのメモリを確保
    char *buffer = new GLchar[length];
    
    // ファイルを先頭から読み込む
    file.seekg(0L, std::ios::beg);
    file.read(buffer, length);

    if (file.bad()) {
      // うまく読み込めなかった
      std::cerr << "Error: Could not read souce file: " << name << std::endl;
    }
    else {
      // シェーダのソースプログラムのシェーダオブジェクトへの読み込み
      const GLchar *source[] = { buffer };
      glShaderSource(shader, 1, source, &length);
      ret = false;
    }
    file.close();
    
    // 読み込みに使ったメモリを開放する
    delete[] buffer;
  }
  
  return ret;
}

/*
** シェーダの情報を表示する
*/
static void printShaderInfoLog(GLuint shader)
{
  // シェーダのコンパイル時のログの長さを取得する
  GLsizei bufSize;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &bufSize);
  
  if (bufSize > 1) {
    // シェーダのコンパイル時のログの内容を取得する
    GLchar *infoLog = new GLchar[bufSize];
    GLsizei length;
    glGetShaderInfoLog(shader, bufSize, &length, infoLog);
    std::cerr << "InfoLog:\n" << infoLog << '\n' << std::endl;
    delete[] infoLog;
  }
}

/*
** プログラムの情報を表示する
*/
static void printProgramInfoLog(GLuint program)
{
  // シェーダのリンク時のログの長さを取得する
  GLsizei bufSize;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH , &bufSize);
  
  if (bufSize > 1) {
    // シェーダのリンク時のログの内容を取得する
    GLchar *infoLog = new GLchar[bufSize];
    GLsizei length;
    glGetProgramInfoLog(program, bufSize, &length, infoLog);
    std::cerr << "InfoLog:\n" << infoLog << '\n' << std::endl;
    delete[] infoLog;
  }
}

/*
** シェーダーソースファイルの読み込み
*/
GLuint gg::loadShader(
  const char *vert,       // バーテックスシェーダのソースファイル名
  const char *frag,       // フラグメントシェーダのソースファイル名
  const char *geom,       // ジオメトリシェーダのソースファイル名
  GLenum input,           // ジオメトリシェーダの入力プリミティブ
  GLenum output,          // ジオメトリシェーダの出力プリミティブ
  int nvarying,           // Transform Feedback する varying 変数の数
  const char **varyings   // Transform Feedback する varying 変数のリスト
  )
{
  // プログラムオブジェクトの作成
  GLuint program = glCreateProgram();

  if (program > 0)
  {
    GLint compiled, linked; // コンパイル結果, リンク結果

    // バーテックスシェーダ
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    if (!readShaderSource(vertShader, vert))
    {
      glDeleteShader(vertShader);
      glDeleteProgram(program);
      return 0;
    }
    glCompileShader(vertShader);
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compiled);
    printShaderInfoLog(vertShader);
    if (compiled == GL_FALSE)
    {
      std::cerr << "Error: Could not compile vertex shader source: " << vert << std::endl;
      glDeleteShader(vertShader);
      glDeleteProgram(program);
      return 0;
    }
    glAttachShader(program, vertShader);
    glDeleteShader(vertShader);

    // フラグメントシェーダ
    if (frag)
    {
      GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
      if (!readShaderSource(fragShader, frag))
      {
        glDeleteShader(fragShader);
        glDeleteProgram(program);
        return 0;
      }
      glCompileShader(fragShader);
      glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compiled);
      printShaderInfoLog(fragShader);
      if (compiled == GL_FALSE)
      {
        std::cerr << "Error: Could not compile fragment shader source: " << frag << std::endl;
        glDeleteShader(fragShader);
        glDeleteProgram(program);
        return 0;
      }
      glAttachShader(program, fragShader);
      glDeleteShader(fragShader);
    }

    // ジオメトリシェーダ（オプション）
    if (geom)
    {
      GLuint geomShader = glCreateShader(GL_GEOMETRY_SHADER_EXT);
      if (!readShaderSource(geomShader, geom))
      {
        glDeleteShader(geomShader);
        glDeleteProgram(program);
        return 0;
      }
      glCompileShader(geomShader);
      glGetShaderiv(geomShader, GL_COMPILE_STATUS, &compiled);
      printShaderInfoLog(geomShader);
      if (compiled == GL_FALSE)
      {
        std::cerr << "Error: Could not compile geometry shader source: " << geom << std::endl;
        glDeleteShader(geomShader);
        glDeleteProgram(program);
        return 0;
      }
      glAttachShader(program, geomShader);
      glDeleteShader(geomShader);

      glProgramParameteriEXT(program, GL_GEOMETRY_INPUT_TYPE_EXT, input);
      glProgramParameteriEXT(program, GL_GEOMETRY_OUTPUT_TYPE_EXT, output);

      int vertices;
      glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT, &vertices);
      glProgramParameteriEXT(program, GL_GEOMETRY_VERTICES_OUT_EXT, vertices);
    }

    // feedback に使う varying 変数を指定する
    if (nvarying > 0) glTransformFeedbackVaryings(program, nvarying, varyings, GL_SEPARATE_ATTRIBS);

    // シェーダプログラムのリンク
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    printProgramInfoLog(program);
    if (linked == GL_FALSE)
    {
      std::cerr << "Error: Could not link shader program" << std::endl;
      glDeleteProgram(program);
      return 0;
    }
  }

  return program;
}
