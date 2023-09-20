//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"

#include "Math/Matrix.h"
#include "Misc/Material.h"
#include "Misc/Scene.h"
#include "Misc/Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render(Scene* scenePtr) const
{
	Camera& camera = scenePtr->GetCamera();
	auto& materials = scenePtr->GetMaterials();
	auto& lights = scenePtr->GetLights();

	const float aspectRatio = (float)m_Width / (float)m_Height;

#pragma omp parallel for schedule(guided) default(none) shared(aspectRatio, scenePtr, materials)
	for (int pixelX{}; pixelX < m_Width; ++pixelX)
	{
		for (int pixelY{}; pixelY < m_Height; ++pixelY)
		{

			Vector3 rayDirection
			{
				(2.0f * (static_cast<float>(pixelX) + 0.5f) / static_cast<float>(m_Width) - 1.0f) * aspectRatio,
				1.0f - 2.0f * ((float)pixelY + 0.5f) / static_cast<float>(m_Height),
				1
			};
			rayDirection.Normalize();


			Ray viewRay{ camera.origin,rayDirection };

			HitRecord closestHit{};
			scenePtr->GetClosestHit(viewRay, closestHit);

			ColorRGB finalColor{};

			if (closestHit.didHit)
			{
				float scaled_t = closestHit.t / 250.0f;
				scaled_t = 1.0f - scaled_t;

				finalColor = materials[closestHit.materialIndex]->Shade() * scaled_t;
			}

			finalColor.MaxToOne();

			m_pBufferPixels[pixelX + (pixelY * m_Width)] = SDL_MapRGB(m_pBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}