/*
	MIT License

	Copyright (c) 2018 Oleksiy Ryabchun

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include "stdafx.h"
#include "resource.h"
#include "math.h"
#include "Windowsx.h"
#include "Main.h"
#include "Hooks.h"
#include "DirectDraw.h"
#include "DirectDrawSurface.h"
#include "DirectDrawPalette.h"
#include "DirectDrawClipper.h"
#include "FpsCounter.h"
#include "Config.h"

#define VK_I 0x49
#define VK_F 0x46

WNDPROC OldWindowProc;

#pragma region Not Implemented
ULONG DirectDraw::AddRef() { return 0; }
HRESULT DirectDraw::Compact() { return DD_OK; }
HRESULT DirectDraw::EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK) { return DD_OK; }
HRESULT DirectDraw::FlipToGDISurface(void) { return DD_OK; }
HRESULT DirectDraw::GetCaps(LPDDCAPS, LPDDCAPS) { return DD_OK; }
HRESULT DirectDraw::GetDisplayMode(LPDDSURFACEDESC) { return DD_OK; }
HRESULT DirectDraw::GetFourCCCodes(LPDWORD, LPDWORD) { return DD_OK; }
HRESULT DirectDraw::GetGDISurface(LPDIRECTDRAWSURFACE *) { return DD_OK; }
HRESULT DirectDraw::GetMonitorFrequency(LPDWORD) { return DD_OK; }
HRESULT DirectDraw::GetScanLine(LPDWORD) { return DD_OK; }
HRESULT DirectDraw::GetVerticalBlankStatus(LPBOOL) { return DD_OK; }
HRESULT DirectDraw::Initialize(GUID *) { return DD_OK; }
HRESULT DirectDraw::RestoreDisplayMode() { return DD_OK; }
HRESULT DirectDraw::DuplicateSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE *) { return DD_OK; }
HRESULT DirectDraw::WaitForVerticalBlank(DWORD, HANDLE) { return DD_OK; }
HRESULT DirectDraw::QueryInterface(REFIID riid, LPVOID* ppvObj) { return DD_OK; }
HRESULT DirectDraw::EnumDisplayModes(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK) { return DD_OK; }
#pragma endregion

LRESULT __stdcall WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SIZE:
	{
		DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
		if (ddraw && ddraw->width && ddraw->height)
		{
			ddraw->viewport.width = LOWORD(lParam);
			ddraw->viewport.height = HIWORD(lParam) - (ddraw->windowState != WinStateWindowed ? 1 : 0);
			ddraw->viewport.refresh = TRUE;
			SetEvent(ddraw->hDrawEvent);
		}

		return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
	}

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_F2: // FPS counter on/off
		{
			isFpsEnabled = !isFpsEnabled;
			isFpsChanged = TRUE;

			return NULL;
		}

		case VK_F3: // Filtering on/off
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				switch (ddraw->imageFilter)
				{
				case FilterLinear:
					if (glVersion >= GL_VER_2_0)
					{
						ddraw->imageFilter = FilterXRBZ;
						Config::Set("ImageFilter", 2);
					}
					else
					{
						ddraw->imageFilter = FilterNearest;
						Config::Set("ImageFilter", 0);
					}
					break;
				case FilterXRBZ:
					ddraw->imageFilter = FilterNearest;
					Config::Set("ImageFilter", 0);
					break;
				default:
					ddraw->imageFilter = FilterLinear;
					Config::Set("ImageFilter", 1);
					break;
				}

				ddraw->isStateChanged = TRUE;
				SetEvent(ddraw->hDrawEvent);
				ddraw->CheckMenu(NULL);
			}

			return NULL;
		}

		case VK_F5:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				ddraw->imageAspect = !ddraw->imageAspect;
				Config::Set("ImageAspect", ddraw->imageAspect);
				ddraw->viewport.refresh = TRUE;
				SetEvent(ddraw->hDrawEvent);
				ddraw->CheckMenu(NULL);
			}

			return NULL;
		}

		default:
			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}
	}

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MOUSEMOVE:
	{
		DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
		if (ddraw && ddraw->imageAspect)
		{
			POINT p = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ddraw->ScaleMouse(&p);
			lParam = MAKELONG(p.x, p.y);
		}

		return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
	}

	case WM_COMMAND:
	{
		switch (wParam)
		{
		case IDM_ASPECT_RATIO:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				ddraw->imageAspect = !ddraw->imageAspect;
				Config::Set("ImageAspect", ddraw->imageAspect);
				ddraw->viewport.refresh = TRUE;
				SetEvent(ddraw->hDrawEvent);
				ddraw->CheckMenu(NULL);
			}
			return NULL;
		}

		case IDM_FILT_OFF:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				ddraw->imageFilter = FilterNearest;
				Config::Set("ImageFilter", 0);
				ddraw->isStateChanged = TRUE;
				SetEvent(ddraw->hDrawEvent);
				ddraw->CheckMenu(NULL);
			}
			return NULL;
		}

		case IDM_FILT_LINEAR:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				ddraw->imageFilter = FilterLinear;
				Config::Set("ImageFilter", 1);
				ddraw->isStateChanged = TRUE;
				SetEvent(ddraw->hDrawEvent);
				ddraw->CheckMenu(NULL);
			}
			return NULL;
		}

		case IDM_FILT_XRBZ:
		{
			DirectDraw* ddraw = Main::FindDirectDrawByWindow(hWnd);
			if (ddraw)
			{
				if (glVersion >= GL_VER_2_0)
				{
					ddraw->imageFilter = FilterXRBZ;
					Config::Set("ImageFilter", 2);
				}
				else
				{
					ddraw->imageFilter = FilterNearest;
					Config::Set("ImageFilter", 0);
				}
				ddraw->isStateChanged = TRUE;
				SetEvent(ddraw->hDrawEvent);
				ddraw->CheckMenu(NULL);
			}
			return NULL;
		}

		default:
			return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
		}
	}

	default:
		return CallWindowProc(OldWindowProc, hWnd, uMsg, wParam, lParam);
	}
}

VOID DirectDraw::ScaleMouse(LPPOINT p)
{
	p->x = Main::Round((FLOAT)(p->x - this->viewport.rectangle.x) * (FLOAT)this->viewport.width / (FLOAT)this->viewport.rectangle.width);
	p->y = Main::Round((FLOAT)(p->y - this->viewport.rectangle.y) * (FLOAT)this->viewport.height / (FLOAT)this->viewport.rectangle.height);
}

VOID DirectDraw::CheckMenu(HMENU hMenu)
{
	if (!hMenu)
		hMenu = GetMenu(this->hWnd);

	if (!hMenu) return;

	CheckMenuItem(hMenu, IDM_ASPECT_RATIO, MF_BYCOMMAND | (this->imageAspect ? MF_CHECKED : MF_UNCHECKED));
	EnableMenuItem(hMenu, IDM_FILT_XRBZ, MF_BYCOMMAND | (!glVersion || glVersion >= GL_VER_3_0 ? MF_ENABLED : MF_DISABLED));

	switch (this->imageFilter)
	{
	case FilterLinear:
		CheckMenuItem(hMenu, IDM_FILT_OFF, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(hMenu, IDM_FILT_LINEAR, MF_BYCOMMAND | MF_CHECKED);
		CheckMenuItem(hMenu, IDM_FILT_XRBZ, MF_BYCOMMAND | MF_UNCHECKED);
		break;
	case FilterXRBZ:
		CheckMenuItem(hMenu, IDM_FILT_OFF, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(hMenu, IDM_FILT_LINEAR, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(hMenu, IDM_FILT_XRBZ, MF_BYCOMMAND | MF_CHECKED);
		break;
	default:
		CheckMenuItem(hMenu, IDM_FILT_OFF, MF_BYCOMMAND | MF_CHECKED);
		CheckMenuItem(hMenu, IDM_FILT_LINEAR, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(hMenu, IDM_FILT_XRBZ, MF_BYCOMMAND | MF_UNCHECKED);
		break;
	}
}

DWORD __stdcall RenderThread(LPVOID lpParameter)
{
	DirectDraw* ddraw = (DirectDraw*)lpParameter;
	ddraw->hDc = ::GetDC(ddraw->hWnd);
	{
		if (!ddraw->wasPixelSet)
		{
			ddraw->wasPixelSet = TRUE;
			PIXELFORMATDESCRIPTOR pfd = { NULL };
			pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
			pfd.nVersion = 1;
			pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
			pfd.iPixelType = PFD_TYPE_RGBA;
			pfd.cColorBits = 32;
			pfd.cAlphaBits = 0;
			pfd.cAccumBits = 0;
			pfd.cDepthBits = 0;
			pfd.cStencilBits = 0;
			pfd.cAuxBuffers = 0;
			pfd.iLayerType = PFD_MAIN_PLANE;

			DWORD pfIndex;
			if (!GL::PreparePixelFormat(&pfd, &pfIndex, ddraw->hWnd))
			{
				pfIndex = ChoosePixelFormat(ddraw->hDc, &pfd);
				if (pfIndex == NULL)
					Main::ShowError("ChoosePixelFormat failed", __FILE__, __LINE__);
				else if (pfd.dwFlags & PFD_NEED_PALETTE)
					Main::ShowError("Needs palette", __FILE__, __LINE__);
			}

			if (!SetPixelFormat(ddraw->hDc, pfIndex, &pfd))
				Main::ShowError("SetPixelFormat failed", __FILE__, __LINE__);

			memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
			pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
			pfd.nVersion = 1;
			if (DescribePixelFormat(ddraw->hDc, pfIndex, sizeof(PIXELFORMATDESCRIPTOR), &pfd) == NULL)
				Main::ShowError("DescribePixelFormat failed", __FILE__, __LINE__);

			if ((pfd.iPixelType != PFD_TYPE_RGBA) ||
				(pfd.cRedBits < 5) || (pfd.cGreenBits < 5) || (pfd.cBlueBits < 5))
				Main::ShowError("Bad pixel type", __FILE__, __LINE__);
		}

		HGLRC hRc = WGLCreateContext(ddraw->hDc);
		{
			WGLMakeCurrent(ddraw->hDc, hRc);
			{
				GL::CreateContextAttribs(ddraw->hDc, &hRc);

				GLint glMaxTexSize;
				GLGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMaxTexSize);

				if (glVersion >= GL_VER_3_0)
				{
					DWORD maxSize = ddraw->width > ddraw->height ? ddraw->width : ddraw->height;

					DWORD maxTexSize = 1;
					while (maxTexSize < maxSize)
						maxTexSize <<= 1;

					if (maxTexSize > glMaxTexSize)
						glVersion = GL_VER_1_1;
				}

				ddraw->CheckMenu(NULL);
				if (glVersion >= GL_VER_3_0)
					ddraw->RenderNew();
				else
					ddraw->RenderOld(glMaxTexSize);
			}
			WGLMakeCurrent(ddraw->hDc, NULL);
		}
		WGLDeleteContext(hRc);
	}
	::ReleaseDC(ddraw->hWnd, ddraw->hDc);
	ddraw->hDc = NULL;

	return NULL;
}

VOID DirectDraw::RenderOld(DWORD glMaxTexSize)
{
	if (this->imageFilter == FilterXRBZ)
		this->imageFilter = FilterLinear;

	if (glMaxTexSize < 256)
		glMaxTexSize = 256;

	DWORD size;
	if (glCapsClampToEdge == GL_CLAMP_TO_EDGE)
		size = this->width < this->height ? this->width : this->height;
	else
		size = this->width > this->height ? this->width : this->height;

	DWORD maxAllow = 1;
	while (maxAllow < size)
		maxAllow <<= 1;

	DWORD maxTexSize = maxAllow < glMaxTexSize ? maxAllow : glMaxTexSize;

	VOID* pixelBuffer = malloc(maxTexSize * maxTexSize * sizeof(DWORD));
	{
		DWORD framePerWidth = this->width / maxTexSize + (this->width % maxTexSize ? 1 : 0);
		DWORD framePerHeight = this->height / maxTexSize + (this->height % maxTexSize ? 1 : 0);
		DWORD frameCount = framePerWidth * framePerHeight;
		Frame* frames = (Frame*)malloc(frameCount * sizeof(Frame));
		{
			Frame* frame = frames;
			for (DWORD y = 0; y < this->height; y += maxTexSize)
			{
				DWORD height = this->height - y;
				if (height > maxTexSize)
					height = maxTexSize;

				for (DWORD x = 0; x < this->width; x += maxTexSize, ++frame)
				{
					DWORD width = this->width - x;
					if (width > maxTexSize)
						width = maxTexSize;

					frame->rect.x = x;
					frame->rect.y = y;
					frame->rect.width = width;
					frame->rect.height = height;

					frame->vSize.width = x + width;
					frame->vSize.height = y + height;

					frame->tSize.width = width == maxTexSize ? 1.0f : (FLOAT)width / maxTexSize;
					frame->tSize.height = height == maxTexSize ? 1.0f : (FLOAT)height / maxTexSize;


					GLGenTextures(1, &frame->id);
					GLBindTexture(GL_TEXTURE_2D, frame->id);

					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glCapsClampToEdge);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glCapsClampToEdge);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
					GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

					if (this->imageFilter == FilterNearest)
					{
						GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					}
					else
					{
						GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					}

					GLTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

					if (GLColorTable)
						GLTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, maxTexSize, maxTexSize, GL_NONE, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, NULL);
					else
						GLTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, maxTexSize, maxTexSize, GL_NONE, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
				}
			}

			GLClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			this->viewport.refresh = TRUE;

			GLMatrixMode(GL_PROJECTION);
			GLLoadIdentity();
			GLOrtho(0.0, (GLdouble)this->width, (GLdouble)this->height, 0.0, 0.0, 1.0);
			GLMatrixMode(GL_MODELVIEW);
			GLLoadIdentity();

			GLEnable(GL_TEXTURE_2D);

			if (glCapsSharedPalette)
				GLEnable(GL_SHARED_TEXTURE_PALETTE_EXT);

			DWORD fpsQueue[FPS_COUNT];
			DWORD tickQueue[FPS_COUNT];

			DWORD fpsIdx = -1;
			DWORD fpsTotal = 0;
			DWORD fpsCount = 0;
			INT fpsSum = 0;
			memset(fpsQueue, 0, sizeof(fpsQueue));
			memset(tickQueue, 0, sizeof(tickQueue));

			GLClear(GL_COLOR_BUFFER_BIT);
			do
			{
				DirectDrawSurface* surface = (DirectDrawSurface*)this->attachedSurface;
				if (surface)
				{
					DirectDrawPalette* palette = surface->attachedPallete;
					if (palette)
					{
						if (isFpsEnabled)
						{
							DWORD tick = GetTickCount();

							if (isFpsChanged)
							{
								isFpsChanged = FALSE;
								fpsIdx = -1;
								fpsTotal = 0;
								fpsCount = 0;
								fpsSum = 0;
								memset(fpsQueue, 0, sizeof(fpsQueue));
								memset(tickQueue, 0, sizeof(tickQueue));
							}

							++fpsTotal;
							if (fpsCount < FPS_COUNT)
								++fpsCount;

							++fpsIdx;
							if (fpsIdx == FPS_COUNT)
								fpsIdx = 0;

							DWORD diff = tick - tickQueue[fpsTotal != fpsCount ? fpsIdx : 0];
							tickQueue[fpsIdx] = tick;

							DWORD fps = diff ? Main::Round(1000.0f / diff * fpsCount) : 9999;

							DWORD* queue = &fpsQueue[fpsIdx];
							fpsSum -= *queue - fps;
							*queue = fps;
						}

						this->CheckView();

						DWORD glFilter = 0;
						if (this->isStateChanged)
						{
							this->isStateChanged = FALSE;
							glFilter = this->imageFilter == FilterNearest ? GL_NEAREST : GL_LINEAR;
						}

						if (glCapsSharedPalette)
							GLColorTable(GL_TEXTURE_2D, GL_RGBA8, 256, GL_RGBA, GL_UNSIGNED_BYTE, ((DirectDrawSurface*)this->attachedSurface)->attachedPallete);

						DWORD count = frameCount;
						frame = frames;
						while (count--)
						{
							GLBindTexture(GL_TEXTURE_2D, frame->id);

							if (glFilter)
							{
								GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
								GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
							}

							if (GLColorTable)
							{
								if (!glCapsSharedPalette)
									GLColorTable(GL_TEXTURE_2D, GL_RGBA8, 256, GL_RGBA, GL_UNSIGNED_BYTE, palette->entries);

								BYTE* pix = (BYTE*)pixelBuffer;
								for (DWORD y = frame->rect.y; y < frame->vSize.height; ++y)
								{
									BYTE* idx = surface->indexBuffer + y * this->width + frame->rect.x;
									memcpy(pix, idx, frame->rect.width);
									pix += frame->rect.width;
								}

								if (isFpsEnabled && count == frameCount - 1)
								{
									DWORD fps = Main::Round((FLOAT)fpsSum / fpsCount);

									DWORD offset = FPS_X;

									DWORD digCount = 0;
									DWORD current = fps;
									do
									{
										++digCount;
										current = current / 10;
									} while (current);

									DWORD dcount = digCount;
									current = fps;
									do
									{
										DWORD digit = current % 10;
										bool* lpDig = (bool*)counters + FPS_WIDTH * FPS_HEIGHT * digit;

										for (DWORD y = 0; y < FPS_HEIGHT; ++y)
										{
											BYTE* pix = (BYTE*)pixelBuffer + (FPS_Y + y) * (maxTexSize)+
												FPS_X + (FPS_STEP + FPS_WIDTH) * (dcount - 1) + FPS_STEP;

											DWORD width = FPS_WIDTH;
											do
											{
												if (*lpDig++)
													*pix = 0xFF;
												++pix;
											} while (--width);
										}

										current = current / 10;
									} while (--dcount);
								}

								GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->rect.width, frame->rect.height, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, pixelBuffer);
							}
							else
							{
								DWORD* pix = (DWORD*)pixelBuffer;
								for (DWORD y = frame->rect.y; y < frame->vSize.height; ++y)
								{
									BYTE* idx = surface->indexBuffer + y * this->width + frame->rect.x;
									for (DWORD x = frame->rect.x; x < frame->vSize.width; ++x)
										*pix++ = *(DWORD*)&palette->entries[*idx++];
								}

								if (isFpsEnabled && count == frameCount - 1)
								{
									DWORD fps = Main::Round((FLOAT)fpsSum / fpsCount);

									DWORD offset = FPS_X;

									DWORD digCount = 0;
									DWORD current = fps;
									do
									{
										++digCount;
										current = current / 10;
									} while (current);

									DWORD dcount = digCount;
									current = fps;
									do
									{
										DWORD digit = current % 10;
										bool* lpDig = (bool*)counters + FPS_WIDTH * FPS_HEIGHT * digit;

										for (DWORD y = 0; y < FPS_HEIGHT; ++y)
										{
											DWORD* pix = (DWORD*)pixelBuffer + (FPS_Y + y) * maxTexSize +
												FPS_X + (FPS_STEP + FPS_WIDTH) * (dcount - 1) + FPS_STEP;

											DWORD width = FPS_WIDTH;
											do
											{
												if (*lpDig++)
													*pix = 0xFFFFFFFF;
												++pix;
											} while (--width);
										}

										current = current / 10;
									} while (--dcount);
								}

								GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->rect.width, frame->rect.height, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer);
							}

							GLBegin(GL_TRIANGLE_FAN);
							{
								GLTexCoord2f(0.0f, 0.0f);
								GLVertex2s(frame->rect.x, frame->rect.y);

								GLTexCoord2f(frame->tSize.width, 0.0f);
								GLVertex2s(frame->vSize.width, frame->rect.y);

								GLTexCoord2f(frame->tSize.width, frame->tSize.height);
								GLVertex2s(frame->vSize.width, frame->vSize.height);

								GLTexCoord2f(0.0f, frame->tSize.height);
								GLVertex2s(frame->rect.x, frame->vSize.height);

							}
							GLEnd();
							++frame;
						}

						GLFlush();
						SwapBuffers(this->hDc);
						GLClear(GL_COLOR_BUFFER_BIT);

						WaitForSingleObject(this->hDrawEvent, 16);
						ResetEvent(this->hDrawEvent);
					}
				}
			} while (!this->isFinish);
			GLFinish();

			frame = frames;
			DWORD count = frameCount;
			while (count--)
			{
				GLDeleteTextures(1, &frame->id);
				++frame;
			}
		}
		free(frames);
	}
	free(pixelBuffer);
}

VOID DirectDraw::RenderNew()
{
	DWORD maxSize = this->width > this->height ? this->width : this->height;
	DWORD maxTexSize = 1;
	while (maxTexSize < maxSize) maxTexSize <<= 1;
	FLOAT texWidth = this->width == maxTexSize ? 1.0f : (FLOAT)this->width / maxTexSize;
	FLOAT texHeight = this->height == maxTexSize ? 1.0f : (FLOAT)this->height / maxTexSize;

	FLOAT buffer[4][4] = {
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ (FLOAT)this->width, 0.0f, texWidth, 0.0f },
		{ (FLOAT)this->width, (FLOAT)this->height, texWidth, texHeight },
		{ 0.0f, (FLOAT)this->height, 0.0f, texHeight }
	};

	FLOAT mvpMatrix[4][4] = {
		{ 2.0f / this->width, 0.0f, 0.0f, 0.0f },
		{ 0.0f, -2.0f / this->height, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 2.0f, 0.0f },
		{ -1.0f, 1.0f, -1.0f, 1.0f }
	};

	GLuint shProgramLinear = GLCreateProgram();
	{
		//GLuint vLinear = GL::CompileShaderSource(glVersion >= GL_VER_3_0 ? IDR_30_LINEAR_VERTEX : IDR_20_LINEAR_VERTEX, GL_VERTEX_SHADER);
		//GLuint fLinear = GL::CompileShaderSource(glVersion >= GL_VER_3_0 ? IDR_30_LINEAR_FRAGMENT : IDR_20_LINEAR_FRAGMENT, GL_FRAGMENT_SHADER);
		GLuint vLinear = GL::CompileShaderSource(IDR_30_LINEAR_VERTEX, GL_VERTEX_SHADER);
		GLuint fLinear = GL::CompileShaderSource(IDR_30_LINEAR_FRAGMENT, GL_FRAGMENT_SHADER);
		{

			GLAttachShader(shProgramLinear, vLinear);
			GLAttachShader(shProgramLinear, fLinear);
			{
				GLLinkProgram(shProgramLinear);
			}
			GLDetachShader(shProgramLinear, fLinear);
			GLDetachShader(shProgramLinear, vLinear);
		}
		GLDeleteShader(fLinear);
		GLDeleteShader(vLinear);

		GLUseProgram(shProgramLinear);
		{
			GLUniformMatrix4fv(GLGetUniformLocation(shProgramLinear, "mvp"), 1, GL_FALSE, (GLfloat*)mvpMatrix);
			GLUniform1i(GLGetUniformLocation(shProgramLinear, "tex01"), 0);

			GLuint shProgramXRBZ = GLCreateProgram();
			{
				//GLuint vXRBZ = GL::CompileShaderSource(glVersion >= GL_VER_3_0 ? IDR_30_XBRZ_VERTEX : IDR_20_XBRZ_VERTEX, GL_VERTEX_SHADER);
				//GLuint fXRBZ = GL::CompileShaderSource(glVersion >= GL_VER_3_0 ? IDR_30_XBRZ_FRAGMENT : IDR_20_XBRZ_FRAGMENT, GL_FRAGMENT_SHADER);
				GLuint vXRBZ = GL::CompileShaderSource(IDR_30_XBRZ_VERTEX, GL_VERTEX_SHADER);
				GLuint fXRBZ = GL::CompileShaderSource(IDR_30_XBRZ_FRAGMENT, GL_FRAGMENT_SHADER);
				{
					GLAttachShader(shProgramXRBZ, vXRBZ);
					GLAttachShader(shProgramXRBZ, fXRBZ);
					{
						GLLinkProgram(shProgramXRBZ);
					}
					GLDetachShader(shProgramXRBZ, fXRBZ);
					GLDetachShader(shProgramXRBZ, vXRBZ);
				}
				GLDeleteShader(fXRBZ);
				GLDeleteShader(vXRBZ);

				GLUseProgram(shProgramXRBZ);
				{
					GLUniformMatrix4fv(GLGetUniformLocation(shProgramXRBZ, "mvp"), 1, GL_FALSE, (GLfloat*)mvpMatrix);
					GLUniform1i(GLGetUniformLocation(shProgramXRBZ, "tex01"), 0);
					GLUniform2f(GLGetUniformLocation(shProgramXRBZ, "inSize"), (GLfloat)maxTexSize, (GLfloat)maxTexSize);
					GLuint outSize = GLGetUniformLocation(shProgramXRBZ, "outSize");

					GLuint arrayName;

					//if (glVersion >= GL_VER_3_0)
					//{
					GLGenVertexArrays(1, &arrayName);
					GLBindVertexArray(arrayName);
					//}

					{
						GLuint bufferName;
						GLGenBuffers(1, &bufferName);
						{
							GLBindBuffer(GL_ARRAY_BUFFER, bufferName);
							{
								GLBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STATIC_DRAW);

								GLint attrCoordsLoc = GLGetAttribLocation(shProgramXRBZ, "vCoord");
								GLEnableVertexAttribArray(attrCoordsLoc);
								GLVertexAttribPointer(attrCoordsLoc, 2, GL_FLOAT, GL_FALSE, 16, (GLvoid*)0);

								GLint attrTexCoordsLoc = GLGetAttribLocation(shProgramXRBZ, "vTexCoord");
								GLEnableVertexAttribArray(attrTexCoordsLoc);
								GLVertexAttribPointer(attrTexCoordsLoc, 2, GL_FLOAT, GL_FALSE, 16, (GLvoid*)8);

								GLuint textureId;
								GLGenTextures(1, &textureId);
								{
									GLActiveTexture(GL_TEXTURE0);
									GLBindTexture(GL_TEXTURE_2D, textureId);
									GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glCapsClampToEdge);
									GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glCapsClampToEdge);
									GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
									GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
									GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
									GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
									GLTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, maxTexSize, maxTexSize, GL_NONE, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

									GLClearColor(0.0f, 0.0f, 0.0f, 1.0f);
									this->viewport.refresh = TRUE;

									VOID* pixelBuffer = malloc(maxTexSize * maxTexSize * sizeof(DWORD));
									{
										this->isStateChanged = TRUE;

										DWORD fpsQueue[FPS_COUNT];
										DWORD tickQueue[FPS_COUNT];

										DWORD fpsIdx = -1;
										DWORD fpsTotal = 0;
										DWORD fpsCount = 0;
										INT fpsSum = 0;
										memset(fpsQueue, 0, sizeof(fpsQueue));
										memset(tickQueue, 0, sizeof(tickQueue));

										GLClear(GL_COLOR_BUFFER_BIT);
										do
										{
											DirectDrawSurface* surface = (DirectDrawSurface*)this->attachedSurface;
											if (surface)
											{
												DirectDrawPalette* palette = surface->attachedPallete;
												if (palette)
												{
													if (isFpsEnabled)
													{
														DWORD tick = GetTickCount();

														if (isFpsChanged)
														{
															isFpsChanged = FALSE;
															fpsIdx = -1;
															fpsTotal = 0;
															fpsCount = 0;
															fpsSum = 0;
															memset(fpsQueue, 0, sizeof(fpsQueue));
															memset(tickQueue, 0, sizeof(tickQueue));
														}

														++fpsTotal;
														if (fpsCount < FPS_COUNT)
															++fpsCount;

														++fpsIdx;
														if (fpsIdx == FPS_COUNT)
															fpsIdx = 0;

														DWORD diff = tick - tickQueue[fpsTotal != fpsCount ? fpsIdx : 0];
														tickQueue[fpsIdx] = tick;

														DWORD fps = diff ? Main::Round(1000.0f / diff * fpsCount) : 9999;

														DWORD* queue = &fpsQueue[fpsIdx];
														fpsSum -= *queue - fps;
														*queue = fps;
													}

													if (this->CheckView() && this->imageFilter == FilterXRBZ)
														GLUniform2f(outSize, this->viewport.clipFactor.x * maxTexSize, this->viewport.clipFactor.y * maxTexSize);

													if (this->isStateChanged)
													{
														this->isStateChanged = FALSE;

														switch (this->imageFilter)
														{
														case FilterLinear:
															GLUseProgram(shProgramLinear);
															GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
															GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
															break;
														case FilterXRBZ:
															GLUseProgram(shProgramXRBZ);
															GLUniform2f(outSize, this->viewport.clipFactor.x * maxTexSize, this->viewport.clipFactor.y * maxTexSize);
															GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
															GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
															break;
														default:
															GLUseProgram(shProgramLinear);
															GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
															GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
															break;
														}
													}

													BYTE* idx = surface->indexBuffer;
													DWORD* pix = (DWORD*)pixelBuffer;
													DWORD count = this->width * this->height;
													while (count--)
														*pix++ = *(DWORD*)&palette->entries[*idx++];

													if (isFpsEnabled)
													{
														DWORD fps = Main::Round((FLOAT)fpsSum / fpsCount);

														DWORD offset = FPS_X;

														DWORD digCount = 0;
														DWORD current = fps;
														do
														{
															++digCount;
															current = current / 10;
														} while (current);

														DWORD dcount = digCount;
														current = fps;
														do
														{
															DWORD digit = current % 10;
															bool* lpDig = (bool*)counters + FPS_WIDTH * FPS_HEIGHT * digit;

															for (DWORD y = 0; y < FPS_HEIGHT; ++y)
															{
																DWORD* pix = (DWORD*)pixelBuffer + (FPS_Y + y) * this->width +
																	FPS_X + (FPS_STEP + FPS_WIDTH) * (dcount - 1) + FPS_STEP;

																DWORD width = FPS_WIDTH;
																do
																{
																	if (*lpDig++)
																		*pix = 0xFFFFFFFF;
																	++pix;
																} while (--width);
															}

															current = current / 10;
														} while (--dcount);
													}

													GLTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->width, this->height, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer);

													GLDrawArrays(GL_TRIANGLE_FAN, 0, 4);

													GLFlush();
													SwapBuffers(this->hDc);
													GLClear(GL_COLOR_BUFFER_BIT);

													WaitForSingleObject(this->hDrawEvent, INFINITE);
													ResetEvent(this->hDrawEvent);
												}
											}
										} while (!this->isFinish);
										GLFinish();
									}
									free(pixelBuffer);
								}
								GLDeleteTextures(1, &textureId);
							}
							GLBindBuffer(GL_ARRAY_BUFFER, NULL);
						}
						GLDeleteBuffers(1, &bufferName);
					}

					//if (glVersion >= GL_VER_3_0)
					//{
					GLBindVertexArray(NULL);
					GLDeleteVertexArrays(1, &arrayName);
					//}
				}
				GLUseProgram(NULL);
			}
			GLDeleteProgram(shProgramXRBZ);
		}
	}
	GLDeleteProgram(shProgramLinear);
}

BOOL DirectDraw::CheckView()
{
	if (this->viewport.refresh)
	{
		this->viewport.refresh = FALSE;

		this->viewport.rectangle.x = this->viewport.rectangle.y = 0;
		this->viewport.rectangle.width = this->viewport.width;
		this->viewport.rectangle.height = this->viewport.height;

		this->viewport.clipFactor.x = this->viewport.viewFactor.x = (FLOAT)this->viewport.width / this->width;
		this->viewport.clipFactor.y = this->viewport.viewFactor.y = (FLOAT)this->viewport.height / this->height;

		if (this->imageAspect && this->viewport.viewFactor.x != this->viewport.viewFactor.y)
		{
			if (this->viewport.viewFactor.x > this->viewport.viewFactor.y)
			{
				FLOAT fw = this->viewport.viewFactor.y * this->width;
				this->viewport.rectangle.width = Main::Round(fw);
				this->viewport.rectangle.x = Main::Round(((FLOAT)this->viewport.width - fw) / 2.0f);
				this->viewport.clipFactor.x = this->viewport.viewFactor.y;
			}
			else
			{
				FLOAT fh = this->viewport.viewFactor.x * this->height;
				this->viewport.rectangle.height = Main::Round(fh);
				this->viewport.rectangle.y = Main::Round(((FLOAT)this->viewport.height - fh) / 2.0f);
				this->viewport.clipFactor.y = this->viewport.viewFactor.x;
			}
		}

		GLViewport(this->viewport.rectangle.x, this->viewport.rectangle.y, this->viewport.rectangle.width, this->viewport.rectangle.height);

		if (this->imageAspect && this->windowState != WinStateWindowed)
		{
			RECT clipRect;
			GetClientRect(this->hWnd, &clipRect);

			clipRect.left = this->viewport.rectangle.x;
			clipRect.right = clipRect.left + this->viewport.rectangle.width;
			clipRect.bottom = clipRect.bottom - this->viewport.rectangle.y;
			clipRect.top = clipRect.bottom - this->viewport.rectangle.height;

			ClientToScreen(this->hWnd, (POINT*)&clipRect.left);
			ClientToScreen(this->hWnd, (POINT*)&clipRect.right);

			ClipCursor(&clipRect);
		}
		else
			ClipCursor(NULL);

		return TRUE;
	}

	return FALSE;
}

DirectDraw::DirectDraw(DirectDraw* lastObj)
{
	GL::Load();

	this->last = lastObj;

	this->surfaceEntries = NULL;
	this->paletteEntries = NULL;
	this->clipperEntries = NULL;

	this->attachedSurface = NULL;

	this->width = FALSE;
	this->height = FALSE;

	this->isFinish = FALSE;
	this->wasPixelSet = FALSE;

	DWORD filter = Config::Get("ImageFilter", 0);
	switch (filter)
	{
	case 1:
		this->imageFilter = FilterLinear;
		break;

	case 2:
		this->imageFilter = FilterXRBZ;
		break;

	default:
		this->imageFilter = FilterNearest;
		break;
	}

	this->imageAspect = Config::Get("ImageAspect", TRUE);

	this->hDrawEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

DirectDraw::~DirectDraw()
{
	DirectDrawSurface* surfaceEntry = (DirectDrawSurface*)this->surfaceEntries;
	while (surfaceEntry)
	{
		DirectDrawSurface* curr = surfaceEntry->last;
		delete surfaceEntry;
		surfaceEntry = curr;
	}

	DirectDrawPalette* paletteEntry = (DirectDrawPalette*)this->paletteEntries;
	while (paletteEntry)
	{
		DirectDrawPalette* curr = paletteEntry->last;
		delete paletteEntry;
		paletteEntry = curr;
	}

	DirectDrawClipper* clipperEntry = (DirectDrawClipper*)this->clipperEntries;
	while (clipperEntry)
	{
		DirectDrawClipper* curr = clipperEntry->last;
		delete clipperEntry;
		clipperEntry = curr;
	}

	CloseHandle(this->hDrawEvent);
	ClipCursor(NULL);
}

ULONG DirectDraw::Release()
{
	if (ddrawList == this)
		ddrawList = NULL;
	else
	{
		DirectDraw* ddraw = ddrawList;
		while (ddraw)
		{
			if (ddraw->last == this)
			{
				ddraw->last = ddraw->last->last;
				break;
			}

			ddraw = ddraw->last;
		}
	}

	delete this;
	return 0;
}

HRESULT DirectDraw::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
	this->windowState = dwFlags & DDSCL_FULLSCREEN ? WinStateFullScreen : WinStateWindowed;
	this->isStateChanged = TRUE;

	if (hWnd && hWnd != this->hWnd)
	{
		this->hWnd = hWnd;

		if (!OldWindowProc)
			OldWindowProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);

		this->hDc = NULL;
	}

	return DD_OK;
}

HRESULT DirectDraw::CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE* lplpDDPalette, IUnknown* pUnkOuter)
{
	this->paletteEntries = (LPDIRECTDRAWPALETTE)new DirectDrawPalette(this);
	*lplpDDPalette = this->paletteEntries;

	this->paletteEntries->SetEntries(0, 0, 256, lpDDColorArray);

	return DD_OK;
}

HRESULT DirectDraw::CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter)
{
	BOOL isPrimary = lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE;

	this->surfaceEntries = (LPDIRECTDRAWSURFACE)new DirectDrawSurface(this, !isPrimary);
	*lplpDDSurface = this->surfaceEntries;

	if (lpDDSurfaceDesc->dwFlags & DDSD_WIDTH)
		this->width = lpDDSurfaceDesc->dwWidth;

	if (lpDDSurfaceDesc->dwFlags & DDSD_HEIGHT)
		this->height = lpDDSurfaceDesc->dwHeight;

	if (isPrimary)
	{
		this->attachedSurface = this->surfaceEntries;

		if (!this->viewport.width || this->viewport.height)
		{
			RECT rect;
			GetClientRect(this->hWnd, &rect);

			this->viewport.width = rect.right - rect.left;
			this->viewport.height = rect.bottom - rect.top - (this->windowState != WinStateWindowed ? 1 : 0);
		}

		this->isFinish = FALSE;
		this->viewport.refresh = TRUE;

		DWORD threadId;
		SECURITY_ATTRIBUTES sAttribs = { NULL };
		this->hDrawThread = CreateThread(&sAttribs, 262144, RenderThread, this, NORMAL_PRIORITY_CLASS, &threadId);
	}

	return DD_OK;
}

HRESULT DirectDraw::CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpDDClipper, IUnknown* pUnkOuter)
{
	this->clipperEntries = (LPDIRECTDRAWCLIPPER)new DirectDrawClipper(this);
	*lplpDDClipper = this->clipperEntries;

	return DD_OK;
}

HRESULT DirectDraw::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
{
	this->width = dwWidth;
	this->height = dwHeight;

	HMONITOR hMon = MonitorFromWindow(this->hWnd, MONITOR_DEFAULTTONEAREST);

	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfo(hMon, &mi);

	RECT* rect = &mi.rcMonitor;
	rect->top -= 1;

	DWORD dwStyle = GetWindowLong(this->hWnd, GWL_STYLE);
	AdjustWindowRect(rect, dwStyle, FALSE);

	SetWindowPos(this->hWnd, NULL, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, NULL);
	SetForegroundWindow(this->hWnd);

	return DD_OK;
}