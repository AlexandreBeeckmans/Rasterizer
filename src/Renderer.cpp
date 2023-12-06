//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"
#include "Camera.h"

#include<execution>

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
	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(45.f, { .0f,5.0f,-64.f });
	m_Camera.aspectRatio = static_cast<float>(m_Width) / m_Height;

	//InitTexture
	m_pTexture = Texture::LoadFromFile("Resources/vehicle_diffuse.png");
	m_pNormalMap = Texture::LoadFromFile("Resources/vehicle_normal.png");
	m_pGlossyMap = Texture::LoadFromFile("Resources/vehicle_gloss.png");
	m_pSpecularMap = Texture::LoadFromFile("Resources/vehicle_specular.png");

	//Init shape
	Utils::ParseOBJ("Resources/vehicle.obj", m_MeshVetrices, m_MeshIndices);
	m_NonTransformedMeshes.push_back(Mesh{ {m_MeshVetrices}, m_MeshIndices, PrimitiveTopology::TriangleList });
}
Renderer::~Renderer()
{
	delete m_pTexture;
	delete m_pNormalMap;

	delete m_pGlossyMap;
	delete m_pSpecularMap;

	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	UpdateRotation(pTimer);	
}
void Renderer::UpdateRotation(Timer* pTimer)
{
	//Update the rotation Angle
	if (m_IsRotationEnabled)
	{
		m_MeshRotationAngle += m_MeshRotationSpeed * pTimer->GetElapsed();
	}
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//Functions
	//All the functions from the previous weeks were deleted to keep the code cleaner
	//You can find them on my GitHub page :
	//https://github.com/AlexandreBeeckmans/Rasterizer

	Render_W4_Part1();

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

//Vertex transformation
void Renderer::VertexTransformationFunction(std::vector<Mesh>& meshVector) const
{
	//Calculate this matrix before the for loop to reduce the amount of operation inside of this loop
	const Matrix viewProjectionMatrix{ m_Camera.viewMatrix * m_Camera.projectionMatrix };
	for (Mesh& mesh : meshVector)
	{
		const Matrix worldViewProjectionMatrix{ mesh.worldMatrix *  viewProjectionMatrix };
		for (const Vertex& vertex : mesh.vertices)
		{
			//New position with perspective division
			Vector4 newPos{ worldViewProjectionMatrix.TransformPoint(vertex.position.ToVector4()) };
			newPos.x /= newPos.w;
			newPos.y /= newPos.w;
			newPos.z /= newPos.w;

			//We calculate the transformed other elements of the mesh
			const Vector3 newNormal{ mesh.worldMatrix.TransformVector(vertex.normal).Normalized()};
			const Vector3 newTangent{ mesh.worldMatrix.TransformVector(vertex.tangent).Normalized()};
			Vector3 newViewDirection{ newPos.GetXYZ() - m_Camera.origin};
			newViewDirection.Normalize();

			//All the elements are added to the Vertices_Out vector
			mesh.vertices_out.emplace_back(Vertex_Out{ newPos, vertex.color, vertex.uv, newNormal, newTangent, newViewDirection });
		}
	}
}

void Renderer::ToggleDisplayZBuffer()
{
	//Invert displaying of ZBuffer
	m_DisplayZBuffer = !m_DisplayZBuffer;
}
void Renderer::ToggleRotation()
{
	//Enable / Disable Rotation
	m_IsRotationEnabled = !m_IsRotationEnabled;
}
void Renderer::ToggleNormalMap()
{
	//Display / Hide normal map
	m_DisplayNormalMap = !m_DisplayNormalMap;
}
void Renderer::CycleShadingMode()
{
	//Change Shading mode

	switch (m_ShadingMode)
	{
	case ShadingMode::ObservedArea:
		m_ShadingMode = ShadingMode::Diffuse;
		break;
	case ShadingMode::Diffuse:
		m_ShadingMode = ShadingMode::Specular;
		break;
	case ShadingMode::Specular:
		m_ShadingMode = ShadingMode::Combined;
		break;
	case ShadingMode::Combined:
		m_ShadingMode = ShadingMode::ObservedArea;
		break;
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
bool Renderer::HitTest_Triangle(const Vector2& vertex0, const Vector2& vertex1, const Vector2& vertex2, const Vector2& pixel, Vector3& barycentricWeights) const
{
	//Caculate vertices to pixel
	const Vector2 v0{ vertex0, pixel };
	const Vector2 v1{ vertex1, pixel };
	const Vector2 v2{ vertex2, pixel };

	//Calculate edges
	const Vector2 s0{ vertex0, vertex1 };
	const Vector2 s1{ vertex1, vertex2 };
	const Vector2 s2{ vertex2, vertex0 };

	//Parallelogram areas
	const float c0{ Vector2::Cross(s1, v1) };
	const float c1{ Vector2::Cross(s2, v2) };
	const float c2{ Vector2::Cross(s0, v0) };

	//We check if all the parallelograms area ae poitning the same direction
	if ((c1 >= 0 && c2 >= 0 && c0 >= 0) || (c1 <= 0 && c2 <= 0 && c0 <= 0))
	{
		//calculate triangle areas
		const float triangleArea{ Vector2::Cross(s0, -s2) };
		if (triangleArea == 0) return false;

		//Calculate barycentrics weights
		barycentricWeights.x = c0 / triangleArea;
		barycentricWeights.y = c1 / triangleArea;
		barycentricWeights.z = c2 / triangleArea;
		return true;
	}
	return false;
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

	//Check if the pixel hits a number + attribute the barycentric weights
	if (HitTest_Triangle({ v0.x, v0.y }, { v1.x, v1.y }, { v2.x, v2.y }, pixel, barycentrics))
	{

		//Corrected Depth interpolation
		const float z0{ barycentrics.x / v0.z };
		const float z1{ barycentrics.y / v1.z };
		const float z2{ barycentrics.z / v2.z };

		float zBufferValue{ 1 / (z0 + z1 + z2) };

		//Check if we can use this buffer value
		if (zBufferValue < 0 || zBufferValue > 1 || zBufferValue >= m_pDepthBufferPixels[pixelNr]) return;

		//W value
		const float w0{ barycentrics.x / v0.w };
		const float w1{ barycentrics.y / v1.w };
		const float w2{ barycentrics.z / v2.w };
		const float wInterpolated{ 1 / (w0 + w1 + w2) };

		//interpolated UV
		const Vector2 uv0{ barycentrics.x * (mesh.vertices_out[mesh.indices[index]].uv / v0.w) };
		const Vector2 uv1{ barycentrics.y * (mesh.vertices_out[mesh.indices[index + 1]].uv / v1.w) };
		const Vector2 uv2{ barycentrics.z * (mesh.vertices_out[mesh.indices[index + 2]].uv / v2.w) };
		const Vector2 interpolatedUV{ (uv0 + uv1 + uv2) * wInterpolated };

		//If uv value outside of the uv map, we display nothing
		if (interpolatedUV.x < 0 || interpolatedUV.x > 1 || interpolatedUV.y < 0 || interpolatedUV.y > 1) return;

		//takes really long
		m_pDepthBufferPixels[pixelNr] = zBufferValue;

		if (!m_DisplayZBuffer)
		{
			//calculate interpolated normal
			const Vector3 n0{ barycentrics.x * (mesh.vertices_out[mesh.indices[index]].normal) };
			const Vector3 n1{ barycentrics.y * (mesh.vertices_out[mesh.indices[index + 1]].normal) };
			const Vector3 n2{ barycentrics.z * (mesh.vertices_out[mesh.indices[index + 2]].normal) };
			const Vector3 interpolatedNormal{ (n0 + n1 + n2).Normalized() };

			//calculate interpolated tangent
			const Vector3 t0{ barycentrics.x * (mesh.vertices_out[mesh.indices[index]].tangent) };
			const Vector3 t1{ barycentrics.y * (mesh.vertices_out[mesh.indices[index + 1]].tangent) };
			const Vector3 t2{ barycentrics.z * (mesh.vertices_out[mesh.indices[index + 2]].tangent) };
			const Vector3 interpolatedTangent{ (t0 + t1 + t2).Normalized() };

			//calculate interpolated viewDirection
			const Vector3 view0{ barycentrics.x * (mesh.vertices_out[mesh.indices[index]].viewDirection) };
			const Vector3 view1{ barycentrics.y * (mesh.vertices_out[mesh.indices[index + 1]].viewDirection) };
			const Vector3 view2{ barycentrics.z * (mesh.vertices_out[mesh.indices[index + 2]].viewDirection) };
			const Vector3 interpolatedViewDirection{ (view0 + view1 + view2).Normalized() };
			
			//Get the color value from the texture map
			finalColor = m_pTexture->Sample(interpolatedUV);
			
			//Create a new vertex out with all the interpolated value
			Vertex_Out interpolatedVertex
			{
				Vector4{v0.x, v0.y, zBufferValue, wInterpolated},
				finalColor,
				interpolatedUV,
				interpolatedNormal,
				interpolatedTangent,
				interpolatedViewDirection
			};

			//Shade the pixel
			finalColor = PixelShading(interpolatedVertex);
		}
		else
		{
			//Display Z buffer values
			const float mappedValue{ Remap(zBufferValue,0.985f, 1.0f) };
			finalColor = ColorRGB{ mappedValue,mappedValue,mappedValue };
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
	//Reinit buffer values
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	std::fill_n(m_pBackBufferPixels, m_Width * m_Height, SDL_MapRGB(m_pBackBuffer->format,
		static_cast<uint8_t>(m_AmbiantColor.r * 255),
		static_cast<uint8_t>(m_AmbiantColor.g * 255),
		static_cast<uint8_t>(m_AmbiantColor.b * 255)));

	//slows a lot (find a faster way ?)
	VertexTransformationFunction(meshesWorld);

	for (const Mesh& mesh : meshesWorld)
	{
		//Convert vertices to ScreenSpace
		std::vector<Vector2> vetrices_screenSpace{};
		for (const Vertex_Out& v : mesh.vertices_out)
		{
			vetrices_screenSpace.push_back(ToScreenSpace(v.position.x, v.position.y));
		}


		//Determine how we will iterate through the for loop dending of the mesh topology
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

		for (int index{ 0 }; index < maxSize; index += incrementIndex)
		{
			//Calculate the value of the three vetrices
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

			//If bounding box outise of the screen --> we don't display the triangle
			if ((topLeft.x <= 0 || bottomRight.x > m_Width) || (bottomRight.y <= 0 || topLeft.y > m_Height)) continue;

			//RENDER LOGIC
			for (int px{ static_cast<int>(topLeft.x) - 1 }; px < static_cast<int>(bottomRight.x + 1); ++px)
			{
				for (int py{ static_cast<int>(bottomRight.y) - 1 }; py < static_cast<int>(topLeft.y + 1); ++py)
				{
					RenderAPixel(px, py, v0, v1, v2, mesh, index);
				}
			}
		}
	}
}

Vector2 Renderer::ToScreenSpace(const float x, const float y) const
{
	const float newX = (x + 1) / 2 * m_Width;
	const float newY = (1 - y) / 2 * m_Height;

	return{ newX, newY };
}
float Renderer::Remap(const float value, const float min, const float max) const
{
	if (max <= min || value > max || value < min) return 0;


	const float difference{ max - min };
	const float valueDistance{ value - min };
	const float differenceProportion{ valueDistance / difference };

	return differenceProportion;

}

ColorRGB Renderer::PixelShading(const Vertex_Out& vOut) const
{
	const Vector3 lightDirection{ 0.577f, -0.577f, 0.577f };

	//Tangent space axis
	const Vector3 binormal = Vector3::Cross(vOut.normal, vOut.tangent);
	const Matrix tangentSpaceAxis{ vOut.tangent, binormal, vOut.normal, Vector4{0,0,0,0} };

	//sample the normal from normal map and place in interval [-1;1]
	ColorRGB sampledNormal = m_pNormalMap->Sample(vOut.uv);
	sampledNormal *= 2.0f;
	sampledNormal.r -= 1.0f;
	sampledNormal.g -= 1.0f;
	sampledNormal.b -= 1.0f;

	//Final normal value depends of the input of the player
	const Vector3 finalNormal = m_DisplayNormalMap?
								tangentSpaceAxis.TransformVector({ sampledNormal.r, sampledNormal.g, sampledNormal.b })
								:
								vOut.normal;

	//OA
	// **************
	const float observedArea{ Vector3::Dot(-lightDirection, finalNormal) };

	//Diffuse
	// **************
	const float kd{ 2.0f };
	const ColorRGB diffuseColor{ vOut.color * kd  };

	//Phong
	// **************
	//calculate glossy
	ColorRGB glossyValue = m_pGlossyMap->Sample(vOut.uv);
	const float glossiness{ 25.0f };
	glossyValue *= glossiness;

	//calculate specular
	ColorRGB specularValue = m_pSpecularMap->Sample(vOut.uv);

	const Vector3 reflect{ Vector3::Reflect(-lightDirection, finalNormal)};

	//If cosinue is lower than zero we take a 0 value
	float cosinus{ std::max(0.0f,Vector3::Dot(reflect, vOut.viewDirection)) };

	ColorRGB specularReflection
	{ 
		specularValue.r * powf(cosinus, glossyValue.r),
		specularValue.g * powf(cosinus, glossyValue.r),
		specularValue.b * powf(cosinus, glossyValue.r)
	};

	//avoid negative value for specular reflection
	specularReflection.r = std::max(0.0f, specularReflection.r);
	specularReflection.g = std::max(0.0f, specularReflection.g);
	specularReflection.b = std::max(0.0f, specularReflection.b);

	if (observedArea > 0)
	{
		switch (m_ShadingMode)
		{
		case ShadingMode::ObservedArea:
			return{ observedArea, observedArea, observedArea };
			break;
			
		case ShadingMode::Diffuse:
			return diffuseColor * observedArea;
			break;

		case ShadingMode::Specular:
			return specularReflection * observedArea;
			break;

		case ShadingMode::Combined:
			const ColorRGB diffuseSpecularColor{ diffuseColor + specularReflection };
			return diffuseSpecularColor * observedArea;
			break;
		}
	}

	return{ 0,0,0 };
}

void Renderer::Render_W4_Part1()
{
	//Define Triangle List
	//InitMeshes

	m_MeshesWorld.clear();

	//Find a more optimal solution ?
	m_MeshesWorld = m_NonTransformedMeshes;
	const Matrix meshTransformation{ Matrix::CreateRotationY(m_MeshRotationAngle) * Matrix::CreateTranslation(m_MeshPosition) };


	m_MeshesWorld[0].worldMatrix = meshTransformation;

	RenderMeshes(m_MeshesWorld);
}