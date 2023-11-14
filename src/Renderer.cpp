//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//Functions
	//Render_W1_Part1(); //Rasyerizer stage only
	//Render_W1_Part2();
	//Render_W1_Part3();
	/*Render_W1_Part4();*/
	Render_W1_Part5();



	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
	for (const Vertex& v : vertices_in)
	{
		const float aspectRatio{ (float)m_Width / m_Height };
		const Vector3 cameraToVertex{ m_Camera.origin, v.position };

		Vertex viewVertex{ m_Camera.viewMatrix.TransformPoint(v.position)};
		viewVertex.position.z = cameraToVertex.Magnitude();

		if (viewVertex.position.z > 0)
		{
			float projectedX{ viewVertex.position.x / (viewVertex.position.z) };
			float projectedY{ viewVertex.position.y / (viewVertex.position.z) };

			projectedX /= (m_Camera.fov * aspectRatio);
			projectedY /= m_Camera.fov;

			viewVertex.position.x = projectedX;
			viewVertex.position.y = projectedY;
		}

		viewVertex.color = v.color;
		vertices_out.push_back(viewVertex);
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

bool Renderer::HitTest_Triangle(const std::vector<Vector2>& triangle, const Vector2& pixel, Vector3& barycentricWeights, const int triangleNr) const
{
	const Vector2 v0{ triangle[triangleNr], pixel };
	const Vector2 v1{ triangle[triangleNr + 1], pixel };
	const Vector2 v2{ triangle[triangleNr + 2], pixel };

	const Vector2 s0{ triangle[triangleNr], triangle[triangleNr + 1] };
	const Vector2 s1{ triangle[triangleNr + 1], triangle[triangleNr + 2] };
	const Vector2 s2{ triangle[triangleNr + 2], triangle[triangleNr] };


	const float c0{ Vector2::Cross(s1, v1) };
	const float c1{ Vector2::Cross(s2, v2) };
	const float c2{ Vector2::Cross(s0, v0) };

	if ((c1 >= 0 && c2 >= 0 && c0 >= 0) || (c1 <= 0 && c2 <= 0 && c0 <= 0))
	{
		const float triangleArea{ Vector2::Cross(s2, s0) };

		barycentricWeights.x = c0 / triangleArea;
		barycentricWeights.y = c1 / triangleArea;
		barycentricWeights.z = c2 / triangleArea;

		return true;
	}
	return false;


	
}
void Renderer::RasterizeTriangle(const std::vector<Vertex>& triangles) const
{
	std::vector<Vector2> vetrices_screenSpace{};
	float* depthBufferPixels{ new float[m_Width * m_Height] };
	for (int i{ 0 }; i < m_Width * m_Height; ++i)
	{
		depthBufferPixels[i] = FLT_MAX;
		m_pBackBufferPixels[i] = 0;
	}


	//Convert to ScreenSpace
	for (const Vertex& v : triangles)
	{
		vetrices_screenSpace.push_back(ToScreenSpace(v.position.x, v.position.y));
	}

	for (int triangleNr{ 0 }; triangleNr < triangles.size(); triangleNr += 3)
	{
		//Define triangle bounding box
		Vector2 topLeft
		{
			std::min<float>(triangles[triangleNr].position.x, std::min(triangles[triangleNr + 1].position.x, triangles[triangleNr + 2].position.x)),
			std::max<float>(triangles[triangleNr].position.y, std::max(triangles[triangleNr + 1].position.y, triangles[triangleNr + 2].position.y))
		};
		ClampToNDC(topLeft);
		topLeft = ToScreenSpace(topLeft.x, topLeft.y);

		Vector2 bottomRight
		{
			std::max<float>(triangles[triangleNr].position.x, std::max(triangles[triangleNr + 1].position.x, triangles[triangleNr + 2].position.x)),
			std::min<float>(triangles[triangleNr].position.y, std::min(triangles[triangleNr + 1].position.y, triangles[triangleNr + 2].position.y))
		};
		ClampToNDC(bottomRight);
		bottomRight = ToScreenSpace(bottomRight.x, bottomRight.y);




		//RENDER LOGIC
		for (int px{int(topLeft.x)}; px < bottomRight.x; ++px)
		{
			for (int py{int(topLeft.y)}; py < bottomRight.y; ++py)
			{
				const Vector2 pixel
				{
					px + 0.5f,
					py + 0.5f
				};
				const int pixelNr{ px + m_Width * py };
				ColorRGB finalColor{ m_pBackBufferPixels[pixelNr]};
				Vector3 barycentrics{};
				if (HitTest_Triangle(vetrices_screenSpace, pixel, barycentrics, triangleNr))
				{
					//depthTest
					float depth{ barycentrics.x * triangles[triangleNr].position.z };
					depth += barycentrics.y * triangles[triangleNr + 1].position.z;
					depth += barycentrics.z * triangles[triangleNr + 2].position.z;
					
					if (depth < depthBufferPixels[pixelNr])
					{
						depthBufferPixels[pixelNr] = barycentrics.z;

						finalColor = barycentrics.x * triangles[triangleNr].color;
						finalColor += barycentrics.y * triangles[triangleNr + 1].color;
						finalColor += barycentrics.z * triangles[triangleNr + 2].color;
					}
				}
				//Update Color in Buffer
				finalColor.MaxToOne();

				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}
		
	}


	delete[] depthBufferPixels;
}

Vector2 Renderer::ToScreenSpace(const float x, const float y) const
{
	const float newX = (x + 1) / 2 * m_Width;
	const float newY = (1 - y) / 2 * m_Height;

	return{ newX, newY };
}

void Renderer::ClampToNDC(Vector2& point) const
{
	if (point.x > 1)
	{
		point.x = 1;
	}
	else if (point.x < -1)
	{
		point.x = -1;
	}

	if (point.y > 1)
	{
		point.y = 1;
	}
	else if (point.y < -1)
	{
		point.y = -1;
	}
}



void Renderer::Render_W1_Part1() const
{

	//Define triangle
	std::vector<Vector3> vetrices_ndc
	{
		{ 0.0f, 0.5f, 1.0f },
		{ 0.5f, -0.5f, 1.0f },
		{ -0.5f, -0.5f, 1.0f }
	};


	std::vector<Vector2> vetrices_screenSpace{};
	//Convert to ScreenSpace
	for (const Vector3& v : vetrices_ndc)
	{

		const float x = (v.x + 1) / 2 * m_Width;
		const float y = (1 - v.y) / 2 * m_Height;

		vetrices_screenSpace.push_back({ x, y });
	}


	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			/*float gradient = px / static_cast<float>(m_Width);
			gradient += py / static_cast<float>(m_Width);
			gradient /= 2.0f;

			ColorRGB finalColor{ gradient, gradient, gradient };*/

			const Vector2 pixel
			{
				px + 0.5f,
				py + 0.5f
			};
			ColorRGB finalColor{ 0, 0, 0 };
			Vector3 barycentrics{};
			if (HitTest_Triangle(vetrices_screenSpace, pixel, barycentrics, 0))
			{
				finalColor = { 1,1,1 };
			}




			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}


}
void Renderer::Render_W1_Part2() const
{

	const std::vector<Vertex> vetrices_world
	{
		Vertex { { 0.0f, 2.0f, 0.0f } },
		Vertex { { 1.0f, 0.0f, 0.0f } },
		Vertex { { -1.0f, 0.0f, 0.0f} }
	};

	std::vector<Vertex> vetrices_NDC{};

	VertexTransformationFunction(vetrices_world, vetrices_NDC);
	RasterizeTriangle(vetrices_NDC);

}
void Renderer::Render_W1_Part3() const
{
	const std::vector<Vertex> vetrices_world
	{
		Vertex { { 0.0f, 2.0f, 0.0f }, { 1, 0, 0 } },
		Vertex { { 1.0f, 0.0f, 0.0f }, { 0, 1, 0 } },
		Vertex { { -1.0f, 0.0f, 0.0f}, { 0, 0, 1 } }
	};

	std::vector<Vertex> vetrices_NDC{};

	VertexTransformationFunction(vetrices_world, vetrices_NDC);
	
	RasterizeTriangle(vetrices_NDC);


}
void Renderer::Render_W1_Part4() const
{
	const std::vector<Vertex> vetrices_world
	{
		//Triangle 0
		Vertex { { 0.0f, 2.0f, 0.0f }, { 1, 0, 0 } },
		Vertex { { 1.5f, -1.0f, 0.0f }, { 1, 0, 0 } },
		Vertex { { -1.5f, -1.0f, 0.0f}, { 1, 0, 0 } },

		//Triangle1
		Vertex { { 0.0f, 4.0f, 2.0f }, { 1, 0, 0 } },
		Vertex { { 3.0f, -2.0f, 2.0f }, { 0, 1, 0 } },
		Vertex { { -3.0f, -2.0f, 2.0f}, { 0, 0, 1 } }
	};

	std::vector<Vertex> vetrices_NDC{};

	VertexTransformationFunction(vetrices_world, vetrices_NDC);

	RasterizeTriangle(vetrices_NDC);
}
void Renderer::Render_W1_Part5() const
{
	const std::vector<Vertex> vetrices_world
	{
		//Triangle 0
		Vertex { { 0.0f, 2.0f, 0.0f }, { 1, 0, 0 } },
		Vertex { { 1.5f, -1.0f, 0.0f }, { 1, 0, 0 } },
		Vertex { { -1.5f, -1.0f, 0.0f}, { 1, 0, 0 } },

		//Triangle1
		Vertex { { 0.0f, 4.0f, 2.0f }, { 1, 0, 0 } },
		Vertex { { 3.0f, -2.0f, 2.0f }, { 0, 1, 0 } },
		Vertex { { -3.0f, -2.0f, 2.0f}, { 0, 0, 1 } }
	};

	std::vector<Vertex> vetrices_NDC{};

	VertexTransformationFunction(vetrices_world, vetrices_NDC);

	RasterizeTriangle(vetrices_NDC);
}