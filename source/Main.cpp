#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <array>

#include <SDL.h>

#ifdef __ANDROID__
#include <jni.h>
#include <SDL_system.h>
#endif

#include "Emulation/Controller.hpp"
#include "SMB/SMBEngine.hpp"
#include "Util/Video.hpp"

#include "Configuration.hpp"
#include "Constants.hpp"
#include "OpenGLRendering.hpp"
#include "TouchControls.hpp"

uint8_t* romImage;
static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* texture;
static SDL_Texture* scanlineTexture;
static SDL_GameController* gameController;
static SMBEngine* smbEngine = nullptr;
static TouchControls* touchControls = nullptr;
static uint32_t renderBuffer[RENDER_WIDTH * RENDER_HEIGHT];

/// <summary>
/// Load the Super Mario Bros. ROM image.
/// </summary>
/// <returns>true if the file was loaded successfully.</returns>
static bool loadRomImage()
{
	SDL_RWops* file = SDL_RWFromFile(Configuration::getRomFileName().c_str(), "rb");
	if (file == nullptr)
	{
		std::cout << "Failed to open the file \"" << Configuration::getRomFileName() << "\". " << SDL_GetError() << "\n";
		return false;
	}

	Sint64 fileSize = SDL_RWsize(file);
	if (fileSize <= 0)
	{
		std::cout << "Failed to get file size for \"" << Configuration::getRomFileName() << "\".\n";
		SDL_RWclose(file);
		return false;
	}

	romImage = new uint8_t[static_cast<size_t>(fileSize)];
	SDL_RWread(file, romImage, sizeof(uint8_t), static_cast<size_t>(fileSize));
	SDL_RWclose(file);

	return true;
}

/// <summary>
/// SDL Audio callback function.
/// </summary>
/// <param name="userdata"></param>
/// <param name="buffer"></param>
/// <param name="len"></param>
static void audioCallback(void* userdata, uint8_t* buffer, int len)
{
	if (smbEngine != nullptr)
	{
		smbEngine->audioCallback(buffer, len);
	}
}

/// <summary>
/// Initialize SDL2 and other libraries for use.
/// </summary>
/// <returns>true if initialization was successful.</returns>
static bool initialize()
{
	// Load the configuration file
	Configuration::initialize(CONFIG_FILE_NAME);

	// Load the SMB ROM image
	if (!loadRomImage())
	{
		return false;
	}

	// Initialize SDL2
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0)
	{
		std::cout << "SDL_Init() failed during initialize(): " << SDL_GetError() << std::endl;
		return false;
	}

	// Create the SDL2 window
	window = SDL_CreateWindow(APP_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		RENDER_WIDTH * Configuration::getRenderScale(), RENDER_HEIGHT * Configuration::getRenderScale(),
		SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
	if (window == nullptr)
	{
		std::cout << "SDL_CreateWindow() failed during initialize(): " << SDL_GetError() << std::endl;
		return false;
	}

	// Create the SDL2 renderer
	// SDL_HINT_RENDER_DRIVER is used to specify which render driver to use. Otherwise the default is Direct3D.
#ifndef __ANDROID__
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED
		| (Configuration::getVsyncEnabled() ? SDL_RENDERER_PRESENTVSYNC : 0));
	if (renderer == nullptr)
	{
		std::cout << "SDL_CreateRenderer() failed during initialize(): " << SDL_GetError() << std::endl;
		return false;
	}

	// Get SDL2 renderer info and make sure it's an OpenGL/GLES renderer
	SDL_RendererInfo rendererInfo;
	SDL_GetRendererInfo(renderer, &rendererInfo);
	if (std::strncmp(rendererInfo.name, "opengl", 6) != 0
		&& std::strncmp(rendererInfo.name, "opengles", 8) != 0)
	{
		std::cout << "SDL_CreateRenderer() failed to create an OpenGL renderer" << std::endl;
		return false;
	}

	// Initialize OpenGL rendering backend.
	if (!loadOpenGLRendering())
	{
		std::cout << "Error: Couldn't initialize OpenGL extensions" << std::endl;
		return false;
	}

	// Uncomment this to use the scale3x shader
	//loadShaderProgram("scale3x.glsl");

	// Create the SDL2 texture that will be used for rendering the PPU
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, RENDER_WIDTH, RENDER_HEIGHT);
	if (texture == nullptr)
	{
		std::cout << "SDL_CreateTexture() failed during initialize(): " << SDL_GetError() << std::endl;
		return false;
	}

	if (Configuration::getScanlinesEnabled())
	{
		scanlineTexture = generateScanlineTexture(renderer);
	}

	// Set up custom palette, if configured
	if (!Configuration::getPaletteFileName().empty())
	{
		const uint32_t* palette = loadPalette(Configuration::getPaletteFileName());
		if (palette)
		{
			paletteRGB = palette;
		}
	}

	// Initialize touch controls
	touchControls = new TouchControls();
#ifdef __ANDROID__
	touchControls->setScale(10);
#else
	touchControls->setScale(Configuration::getRenderScale());
#endif

	if (Configuration::getAudioEnabled())
	{
		// Initialize audio
		SDL_AudioSpec desiredSpec;
		desiredSpec.freq = Configuration::getAudioFrequency();
		desiredSpec.format = AUDIO_S8;
		desiredSpec.channels = 1;
		desiredSpec.samples = 2048;
		desiredSpec.callback = audioCallback;
		desiredSpec.userdata = NULL;

		SDL_AudioSpec obtainedSpec;
		SDL_OpenAudio(&desiredSpec, &obtainedSpec);

		// Start playing audio
		SDL_PauseAudio(0);
	}

	return true;
}

/// <summary>
/// Android vibrate helper - called from touch controls.
/// </summary>
void androidVibrate(int ms)
{
#ifdef __ANDROID__
	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	if (!env) return;

	jclass clazz = env->FindClass("org/libsdl/app/SDLActivity");
	if (!clazz) return;

	jmethodID method = env->GetStaticMethodID(clazz, "vibrate", "(I)V");
	if (!method) return;

	env->CallStaticVoidMethod(clazz, method, ms);
	env->DeleteLocalRef(clazz);
#endif
}

/// <summary>
/// Shutdown libraries for exit.
/// </summary>
static void shutdown()
{
	if (gameController)
		SDL_GameControllerClose(gameController);

	delete touchControls;
	touchControls = nullptr;

	SDL_CloseAudio();

	SDL_DestroyTexture(scanlineTexture);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
}

/// <summary>
/// Main loop of the program runs the game program: CPU, PPU, APU.
/// </summary>
static void mainLoop()
{
	SMBEngine engine(romImage);
	smbEngine = &engine;
	engine.reset();

	bool running = true;
	int progStartTime = SDL_GetTicks();
	int frame = 0;
	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				running = false;
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
				case SDL_WINDOWEVENT_CLOSE:
					running = false;
					break;
				}
				break;

			case SDL_CONTROLLERDEVICEADDED:
				if (!gameController && event.cdevice.which == 0)
				{
					gameController = SDL_GameControllerOpen(0);
				}
				break;

			case SDL_CONTROLLERDEVICEREMOVED:
				if (gameController && event.cdevice.which == 0)
				{
					SDL_GameControllerClose(gameController);
					gameController = 0;
				}
				break;

			case SDL_FINGERDOWN:
			case SDL_FINGERUP:
			case SDL_FINGERMOTION:
				if (smbEngine && touchControls) {
					touchControls->handleEvent(event, smbEngine->getController1());
				}
				break;

			case SDL_APP_WILLENTERBACKGROUND:
				if (smbEngine) {
					SDL_PauseAudio(1);
				}
				break;

			case SDL_APP_DIDENTERFOREGROUND:
				if (smbEngine) {
					SDL_PauseAudio(0);
				}
				break;

			case SDL_APP_TERMINATING:
				running = false;
				break;

			default:
				break;
			}
		}

		const Uint8* keys = SDL_GetKeyboardState(NULL);
		Controller& controller1 = engine.getController1();
		controller1.setButtonState(BUTTON_A, keys[SDL_SCANCODE_X]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_A)));
		controller1.setButtonState(BUTTON_B, keys[SDL_SCANCODE_Z]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_X)));
		controller1.setButtonState(BUTTON_SELECT, keys[SDL_SCANCODE_BACKSPACE]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_BACK)));
		controller1.setButtonState(BUTTON_START, keys[SDL_SCANCODE_RETURN]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_START)));
		controller1.setButtonState(BUTTON_UP, keys[SDL_SCANCODE_UP]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_DPAD_UP)));
		controller1.setButtonState(BUTTON_DOWN, keys[SDL_SCANCODE_DOWN]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_DPAD_DOWN)));
		controller1.setButtonState(BUTTON_LEFT, keys[SDL_SCANCODE_LEFT]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_DPAD_LEFT)));
		controller1.setButtonState(BUTTON_RIGHT, keys[SDL_SCANCODE_RIGHT]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)));

		Controller& controller2 = engine.getController2();
		controller2.setButtonState(BUTTON_A, keys[SDL_SCANCODE_X]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_A)));
		controller2.setButtonState(BUTTON_B, keys[SDL_SCANCODE_Z]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_X)));
		controller2.setButtonState(BUTTON_SELECT, keys[SDL_SCANCODE_BACKSPACE]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_BACK)));
		controller2.setButtonState(BUTTON_START, keys[SDL_SCANCODE_RETURN]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_START)));
		controller2.setButtonState(BUTTON_UP, keys[SDL_SCANCODE_UP]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_DPAD_UP)));
		controller2.setButtonState(BUTTON_DOWN, keys[SDL_SCANCODE_DOWN]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_DPAD_DOWN)));
		controller2.setButtonState(BUTTON_LEFT, keys[SDL_SCANCODE_LEFT]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_DPAD_LEFT)));
		controller2.setButtonState(BUTTON_RIGHT, keys[SDL_SCANCODE_RIGHT]
			|| (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)));

		if (keys[SDL_SCANCODE_R])
		{
			// Reset
			engine.reset();
		}
		if (keys[SDL_SCANCODE_ESCAPE])
		{
			// quit
			running = false;
			break;
		}
		if (keys[SDL_SCANCODE_F])
		{
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}

		engine.update();
		engine.render(renderBuffer);

		// Render touch controls on top of game frame
		if (touchControls) {
			touchControls->render(renderBuffer);
		}

		// Updates the texture with new information from renderBuffer.
		SDL_UpdateTexture(texture, NULL, renderBuffer, sizeof(uint32_t) * RENDER_WIDTH);

		// New function to use OpenGL for presenting the texture as the backbuffer
		renderSDLOpenGLBackBuffer(window, texture);

		// Ensure that the framerate stays as close to the desired FPS as possible. If the frame was rendered faster,
		// then delay. If the frame was slower, reset time so that the game doesn't try to "catch up", going super-speed.
		int now = SDL_GetTicks();
		int delay = progStartTime + int(double(frame) * double(MS_PER_SEC) / double(Configuration::getFrameRate())) - now;
		if (delay > 0)
		{
			SDL_Delay(delay);
		}
		else
		{
			frame = 0;
			progStartTime = now;
		}
		frame++;
	}
}

/// <summary>
/// Entry point to the program.
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
int main(int argc, char** argv)
{
	if (!initialize())
	{
		std::cout << "Failed to initialize. Please check previous error messages for more information. The program will now exit.\n";
		return -1;
	}

	mainLoop();

	shutdown();

	return 0;
}
