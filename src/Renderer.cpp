//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"
#include "Camera.h"

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
	m_Camera.aspectRatio = static_cast<float>(m_Width) / m_Height;
	m_Camera.CalculateViewMatrix();
	m_Camera.CalculateProjectionMatrix();

	//InitTexture
	//m_pTexture = Texture::LoadFromFile("Resources/uv_grid_2.png");
	m_pTexture = Texture::LoadFromFile("Resources/tuktuk.png");
}

Renderer::~Renderer()
{
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	m_MeshRotationAngle += m_MeshRotationSpeed * pTimer->GetElapsed();
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
	//Render_W1_Part5();
	//Render_W2_Part1();
	//Render_W2_Part2();
	//Render_W2_Part3();
	//Render_W3_Part1();
	Render_W3_Part2();

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}


void Renderer::VertexTransformationFunction(std::vector<Mesh>& meshVector) const
{
	for (Mesh& mesh : meshVector)
	{
		const Matrix worldViewProjectionMatrix{ mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };
		for (const Vertex& vertex : mesh.vertices)
		{

			Vector4 newPos{ worldViewProjectionMatrix.TransformPoint(vertex.position.ToVector4()) };
			newPos.x /= newPos.w;
			newPos.y /= newPos.w;
			newPos.z /= newPos.w;
			Vertex_Out newVertex{ newPos, vertex.color, vertex.uv };
			mesh.vertices_out.emplace_back(newVertex);
		}
	}

}


void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
	const float aspectRatio{ static_cast<float>(m_Width) / m_Height };
	for (const Vertex& v : vertices_in)
	{
		
		const Vector3 cameraToVertex{ m_Camera.origin, v.position };

		Vertex viewVertex{ m_Camera.viewMatrix.TransformPoint(v.position)};

		float projectedX{ viewVertex.position.x / (viewVertex.position.z) };
		float projectedY{ viewVertex.position.y / (viewVertex.position.z) };

		projectedX /= (m_Camera.fov * aspectRatio);
		projectedY /= m_Camera.fov;

		viewVertex.position.x = projectedX;
		viewVertex.position.y = projectedY;

		viewVertex.color = v.color;
		viewVertex.uv = v.uv;
		vertices_out.push_back(viewVertex);
	}
}
void Renderer::ToggleDisplayZBuffer()
{
	m_DisplayZBuffer = !m_DisplayZBuffer;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

bool Renderer::HitTest_Triangle(const std::vector<Vector2>& triangle, const Vector2& pixel, Vector3& barycentricWeights, const int triangleNr) const
{


	return HitTest_Triangle(triangle[triangleNr], triangle[triangleNr + 1], triangle[triangleNr + 2], pixel, barycentricWeights);

	//const Vector2 v0{ triangle[triangleNr], pixel };
	//const Vector2 v1{ triangle[triangleNr + 1], pixel };
	//const Vector2 v2{ triangle[triangleNr + 2], pixel };

	//const Vector2 s0{ triangle[triangleNr], triangle[triangleNr + 1] };
	//const Vector2 s1{ triangle[triangleNr + 1], triangle[triangleNr + 2] };
	//const Vector2 s2{ triangle[triangleNr + 2], triangle[triangleNr] };


	//const float c0{ Vector2::Cross(s1, v1) };
	//const float c1{ Vector2::Cross(s2, v2) };
	//const float c2{ Vector2::Cross(s0, v0) };

	//if ((c1 >= 0 && c2 >= 0 && c0 >= 0) || (c1 <= 0 && c2 <= 0 && c0 <= 0))
	//{
	//	const float triangleArea{ Vector2::Cross(s2, s0) };

	//	barycentricWeights.x = c0 / triangleArea;
	//	barycentricWeights.y = c1 / triangleArea;
	//	barycentricWeights.z = c2 / triangleArea;

	//	return true;
	//}
	//return false;
}

bool Renderer::HitTest_Triangle(const Vector2& vertex0, const Vector2& vertex1, const Vector2& vertex2, const Vector2& pixel, Vector3& barycentricWeights) const
{
	const Vector2 v0{ vertex0, pixel };
	const Vector2 v1{ vertex1, pixel };
	const Vector2 v2{ vertex2, pixel };

	const Vector2 s0{ vertex0, vertex1 };
	const Vector2 s1{ vertex1, vertex2 };
	const Vector2 s2{ vertex2, vertex0 };


	const float c0{ Vector2::Cross(s1, v1) };
	const float c1{ Vector2::Cross(s2, v2) };
	const float c2{ Vector2::Cross(s0, v0) };

	if ((c1 >= 0 && c2 >= 0 && c0 >= 0) || (c1 <= 0 && c2 <= 0 && c0 <= 0))
	{
		const float triangleArea{ Vector2::Cross(s2, s0) };
		if (triangleArea == 0) return false;


		barycentricWeights.x = c0 / triangleArea;
		barycentricWeights.y = c1 / triangleArea;
		barycentricWeights.z = c2 / triangleArea;

		return true;
	}
	return false;
}
void Renderer::RasterizeTriangle(const std::vector<Vertex>& vertices) const
{
	std::vector<Vector2> vetrices_screenSpace{};
	float* depthBufferPixels{ new float[m_Width * m_Height] };
	for (int i{ 0 }; i < m_Width * m_Height; ++i)
	{
		depthBufferPixels[i] = FLT_MAX;
		m_pBackBufferPixels[i] = 0;
	}

	//Convert to ScreenSpace
	for (const Vertex& v : vertices)
	{
		vetrices_screenSpace.push_back(ToScreenSpace(v.position.x, v.position.y));
	}

	for (int triangleNr{ 0 }; triangleNr < vertices.size(); triangleNr += 3)
	{
		//Define triangle bounding box
		Vector2 topLeft
		{
			std::min<float>(vertices[triangleNr].position.x, std::min(vertices[triangleNr + 1].position.x, vertices[triangleNr + 2].position.x)),
			std::max<float>(vertices[triangleNr].position.y, std::max(vertices[triangleNr + 1].position.y, vertices[triangleNr + 2].position.y))
		};
		ClampToNDC(topLeft);
		topLeft = ToScreenSpace(topLeft.x, topLeft.y);

		Vector2 bottomRight
		{
			std::max<float>(vertices[triangleNr].position.x, std::max(vertices[triangleNr + 1].position.x, vertices[triangleNr + 2].position.x)),
			std::min<float>(vertices[triangleNr].position.y, std::min(vertices[triangleNr + 1].position.y, vertices[triangleNr + 2].position.y))
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
				Vector3 barycentrics{};
				if (HitTest_Triangle(vetrices_screenSpace, pixel, barycentrics, triangleNr))
				{
					//depthTest
					const float w0{ barycentrics.x / vertices[triangleNr].position.z};
					const float w1{ barycentrics.y / vertices[triangleNr + 1].position.z };
					const float w2{ barycentrics.z / vertices[triangleNr + 2].position.z };

					float zInterpolated{1 / (w0 + w1 + w2)};


					
					if (zInterpolated < depthBufferPixels[pixelNr])
					{
						depthBufferPixels[pixelNr] = zInterpolated;

						//correctedUV
						const Vector2 uv0{ barycentrics.x * (vertices[triangleNr].uv / vertices[triangleNr].position.z) };
						const Vector2 uv1{ barycentrics.y * (vertices[triangleNr + 1].uv / vertices[triangleNr + 1].position.z) };
						const Vector2 uv2{ barycentrics.z * (vertices[triangleNr + 2].uv / vertices[triangleNr + 2].position.z) };
						const Vector2 interpolatedUV{ (uv0 + uv1 + uv2) * zInterpolated };

						if (interpolatedUV.x >= 0 && interpolatedUV.x <= 1 && interpolatedUV.y >= 0 && interpolatedUV.y <= 1)
						{
							ColorRGB finalColor{};
							finalColor = m_pTexture->Sample(interpolatedUV);

							//Update Color in Buffer
							finalColor.MaxToOne();

							m_pBackBufferPixels[pixelNr] = SDL_MapRGB(m_pBackBuffer->format,
								static_cast<uint8_t>(finalColor.r * 255),
								static_cast<uint8_t>(finalColor.g * 255),
								static_cast<uint8_t>(finalColor.b * 255));
						}
					}
				}
			}
		}
	}

	delete[] depthBufferPixels;
}
void Renderer::RenderAPixel(const int px, const int py, const Vector4& v0, const Vector4& v1, const Vector4& v2, const Mesh& mesh, const int index)
{
	const int pixelNr{ px + (m_Width * py) };
	const Vector2 pixel
	{
		px + 0.5f,
		py + 0.5f
	};

	ColorRGB finalColor{};
	Vector3 barycentrics{};
	if (HitTest_Triangle({ v0.x, v0.y }, { v1.x, v1.y }, { v2.x, v2.y }, pixel, barycentrics))
	{

		//Corrected Depth interpolation
		const float z0{ barycentrics.x / v0.z };
		const float z1{ barycentrics.y / v1.z };
		const float z2{ barycentrics.z / v2.z };

		float zBufferValue{ 1 / (z0 + z1 + z2) };

		if (zBufferValue < 0 || zBufferValue > 1 || zBufferValue >= m_pDepthBufferPixels[pixelNr]) return;

		m_pDepthBufferPixels[pixelNr] = zBufferValue;

		const float w0{ barycentrics.x / v0.w };
		const float w1{ barycentrics.y / v1.w };
		const float w2{ barycentrics.z / v2.w };

		const float wInterpolated{ 1 / (w0 + w1 + w2) };

		//correctedUV
		const Vector2 uv0{ barycentrics.x * (mesh.vertices_out[mesh.indices[index]].uv / v0.w) };
		const Vector2 uv1{ barycentrics.y * (mesh.vertices_out[mesh.indices[index + 1]].uv / v1.w) };
		const Vector2 uv2{ barycentrics.z * (mesh.vertices_out[mesh.indices[index + 2]].uv / v2.w) };
		const Vector2 interpolatedUV{ (uv0 + uv1 + uv2) * wInterpolated };

		if (interpolatedUV.x < 0 || interpolatedUV.x > 1 || interpolatedUV.y < 0 || interpolatedUV.y > 1) return;

		if (!m_DisplayZBuffer)
		{
			finalColor = m_pTexture->Sample(interpolatedUV);
		}
		else
		{
			const float mappedValue{ Remap(zBufferValue,0.8f, 1.0f) };
			finalColor = { mappedValue, mappedValue, mappedValue };
		}
		


		//Update Color in Buffer
		finalColor.MaxToOne();

		m_pBackBufferPixels[pixelNr] = SDL_MapRGB(m_pBackBuffer->format,
			static_cast<uint8_t>(finalColor.r * 255),
			static_cast<uint8_t>(finalColor.g * 255),
			static_cast<uint8_t>(finalColor.b * 255));
	}
}
void Renderer::RenderMeshes(std::vector<Mesh>& meshesWorld)
{
	VertexTransformationFunction(meshesWorld);

	for (const Mesh& mesh : meshesWorld)
	{

		//Rasterize logic
		std::vector<Vector2> vetrices_screenSpace{};
		m_pDepthBufferPixels = new float[m_Width * m_Height];

		for (int i{ 0 }; i < m_Width * m_Height; ++i)
		{
			m_pDepthBufferPixels[i] = FLT_MAX;
			m_pBackBufferPixels[i] = 0;
		}

		//Convert to ScreenSpace
		for (const Vertex_Out& v : mesh.vertices_out)
		{
			vetrices_screenSpace.push_back(ToScreenSpace(v.position.x, v.position.y));
		}



		int incrementIndex{ 1 };
		size_t maxSize{ mesh.indices.size() };

		switch (mesh.primitiveTopology)
		{
		case PrimitiveTopology::TriangleList:
			incrementIndex = 3;
			break;
		case PrimitiveTopology::TriangleStrip:
			maxSize -= 2;
			break;
		}

		//for (int triangleNr{ 0 }; triangleNr < meshesWorld[0].vertices_out.size(); triangleNr += 3)
		for (int index{ 0 }; index < maxSize; index += incrementIndex)
		{
			const Vector4 v0
			{
				vetrices_screenSpace[mesh.indices[index]].x,
				vetrices_screenSpace[mesh.indices[index]].y,
				mesh.vertices_out[mesh.indices[index]].position.z,
				mesh.vertices_out[mesh.indices[index]].position.w
			};

			const Vector4 v1
			{
				vetrices_screenSpace[mesh.indices[index + 1]].x,
				vetrices_screenSpace[mesh.indices[index + 1]].y,
				mesh.vertices_out[mesh.indices[index + 1]].position.z,
				mesh.vertices_out[mesh.indices[index + 1]].position.w
			};

			const Vector4 v2
			{
				vetrices_screenSpace[mesh.indices[index + 2]].x,
				vetrices_screenSpace[mesh.indices[index + 2]].y,
				mesh.vertices_out[mesh.indices[index + 2]].position.z,
				mesh.vertices_out[mesh.indices[index + 2]].position.w
			};



			//Define triangle bounding box
			Vector2 topLeft
			{
				std::min<float>(v0.x, std::min(v1.x, v2.x)),
				std::max<float>(v0.y, std::max(v1.y, v2.y))
			};

			Vector2 bottomRight
			{
				std::max<float>(v0.x, std::max(v1.x, v2.x)),
				std::min<float>(v0.y, std::min(v1.y, v2.y))
			};


			if ((topLeft.x < 0 || bottomRight.x > m_Width - 1) || (bottomRight.y < 0 || topLeft.y > m_Height - 1)) continue;



			//RENDER LOGIC
			for (int px{ static_cast<int>(topLeft.x) }; px < static_cast<int>(bottomRight.x); ++px)
			{
				for (int py{ static_cast<int>(bottomRight.y) }; py < static_cast<int>(topLeft.y); ++py)
				{
					RenderAPixel(px, py, v0, v1, v2, mesh, index);
				}
			}
		}
		delete[] m_pDepthBufferPixels;
		m_pDepthBufferPixels = nullptr;
	}
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
std::vector<Vertex> Renderer::MeshToVetrices(const Mesh& mesh) const
{
	int incrementIndex{1};
	size_t maxSize{ mesh.indices.size() };

	switch (mesh.primitiveTopology)
	{
		case PrimitiveTopology::TriangleList:
			incrementIndex = 3;
			break;
		case PrimitiveTopology::TriangleStrip:
			maxSize -= 2;
			break;
	}

	std::vector<Vertex> vectorToFill{};
	for (int currentIndice{ 0 }; currentIndice < maxSize; currentIndice += incrementIndex)
	{

		vectorToFill.push_back(mesh.vertices[mesh.indices[currentIndice]]);
		vectorToFill.push_back(mesh.vertices[mesh.indices[currentIndice + 1]]);
		vectorToFill.push_back(mesh.vertices[mesh.indices[currentIndice + 2]]);
	}
	return vectorToFill;
}
float Renderer::Remap(const float value, const float min, const float max) const
{
	if (max <= min || value > max || value < min) return 0;


	const float difference{ max - min };
	const float valueDistance{ value - min };
	const float differenceProportion{ valueDistance / difference };

	return differenceProportion;

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
void Renderer::Render_W2_Part1() const
{
	//Define Triangle List
	std::vector<Mesh> meshesWorld
	{
		Mesh
		{
			{
				Vertex{ { -3, 3, -2 },	{1,1,1} },
				Vertex{ { 0, 3, -2 },	{1,1,1} },
				Vertex{ { 3, 3, -2 },	{1,1,1} },
				Vertex{ { -3, 0, -2 },	{1,1,1} },
				Vertex{ { 0, 0, -2 },	{1,1,1} },
				Vertex{ { 3, 0, -2 },	{1,1,1} },
				Vertex{ { -3, -3, -2 },	{1,1,1} },
				Vertex{ { 0, -3, -2 },	{1,1,1} },
				Vertex{ { 3, -3, -2 },	{1,1,1} },
			},

			{
				3,0,1,	1,4,3,	4,1,2,
				2,5,4,	6,3,4,	4,7,6,
				7,4,5,	5,8,7
			},

			PrimitiveTopology::TriangleList
		}
	};

	std::vector<Vertex> vetrices_world{MeshToVetrices(meshesWorld[0])};
	std::vector<Vertex> vetrices_NDC{};

	VertexTransformationFunction(vetrices_world, vetrices_NDC);
	RasterizeTriangle(vetrices_NDC);
}
void Renderer::Render_W2_Part2() const
{
	//Define Triangle List
	std::vector<Mesh> meshesWorld
	{
		Mesh
		{
			{
				Vertex{ { -3, 3, -2 },	{1,1,1} },
				Vertex{ { 0, 3, -2 },	{1,1,1} },
				Vertex{ { 3, 3, -2 },	{1,1,1} },
				Vertex{ { -3, 0, -2 },	{1,1,1} },
				Vertex{ { 0, 0, -2 },	{1,1,1} },
				Vertex{ { 3, 0, -2 },	{1,1,1} },
				Vertex{ { -3, -3, -2 },	{1,0,1} },
				Vertex{ { 0, -3, -2 },	{1,1,1} },
				Vertex{ { 3, -3, -2 },	{1,1,1} },
			},

			{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5
			},

			PrimitiveTopology::TriangleStrip
		}
	};

	std::vector<Vertex> vetrices_world{MeshToVetrices(meshesWorld[0])};
	std::vector<Vertex> vetrices_NDC{};

	VertexTransformationFunction(vetrices_world, vetrices_NDC);
	RasterizeTriangle(vetrices_NDC);
}
void Renderer::Render_W2_Part3() const
{
	//Define Triangle List
	std::vector<Mesh> meshesWorld
	{
		Mesh
		{
			{
				Vertex{ { -3, 3, -2 },	{1,1,1},	{0,0} },
				Vertex{ { 0, 3, -2 },	{1,1,1},	{0.5f,0} },
				Vertex{ { 3, 3, -2 },	{1,1,1},	{1,0} },
				Vertex{ { -3, 0, -2 },	{1,1,1},	{0,0.5f} },
				Vertex{ { 0, 0, -2 },	{1,1,1},	{0.5f,0.5f} },
				Vertex{ { 3, 0, -2 },	{1,1,1},	{1,0.5f} },
				Vertex{ { -3, -3, -2 },	{1,1,1},	{0,1} },
				Vertex{ { 0, -3, -2 },	{1,1,1},	{0.5f,1} },
				Vertex{ { 3, -3, -2 },	{1,1,1},	{1,1} },
			},

			{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5
			},

			PrimitiveTopology::TriangleStrip
		}
	};

	std::vector<Vertex> vetrices_world{ MeshToVetrices(meshesWorld[0]) };
	std::vector<Vertex> vetrices_NDC{};

	VertexTransformationFunction(vetrices_world, vetrices_NDC);
	RasterizeTriangle(vetrices_NDC);
}

void Renderer::Render_W3_Part1()
{
	//Define Triangle List
	std::vector<Mesh> meshesWorld
	{	Mesh
		{
			{
				Vertex{ { -3, 3, -2 },	{1,1,1},	{0,0} },
				Vertex{ { 0, 3, -2 },	{1,1,1},	{0.5f,0} },
				Vertex{ { 3, 3, -2 },	{1,1,1},	{1,0} },
				Vertex{ { -3, 0, -2 },	{1,1,1},	{0,0.5f} },
				Vertex{ { 0, 0, -2 },	{1,1,1},	{0.5f,0.5f} },
				Vertex{ { 3, 0, -2 },	{1,1,1},	{1,0.5f} },
				Vertex{ { -3, -3, -2 },	{1,1,1},	{0,1} },
				Vertex{ { 0, -3, -2 },	{1,1,1},	{0.5f,1} },
				Vertex{ { 3, -3, -2 },	{1,1,1},	{1,1} },
			},

			{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5
			},

			PrimitiveTopology::TriangleStrip
		}
	};

	RenderMeshes(meshesWorld);
}

void Renderer::Render_W3_Part2()
{
	//Define Triangle List
		//InitMeshes
	std::vector<Mesh> meshesWorld{};

	std::vector<Vertex> meshVetrices{};
	std::vector<uint32_t> meshIndices{};
	Utils::ParseOBJ("Resources/tuktuk.obj", meshVetrices, meshIndices);
	meshesWorld.push_back(Mesh{ {meshVetrices}, meshIndices, PrimitiveTopology::TriangleList });

	meshesWorld[0].worldMatrix = Matrix::CreateRotationY(m_MeshRotationAngle);
	RenderMeshes(meshesWorld);
}

void Renderer::Render_W4_Part1()
{

}