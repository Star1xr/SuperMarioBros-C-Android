#ifndef OPENGLRENDERING_HPP
#define OPENGLRENDERING_HPP

#include <string>
#include <SDL.h>

#ifdef __ANDROID__
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

bool loadOpenGLRendering();
void unloadOpenGLRendering();
bool loadShaderProgram(std::string glslFilePath);
void renderSDLOpenGLBackBuffer(SDL_Window* window, SDL_Texture* backBuffer);

#endif // OPENGLRENDERING_HPP
