#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;


namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;
	//struct Vector_Out;


	enum struct ShadingMode
	{
		ObservedArea,
		Diffuse,
		Specular,
		Combined
	};

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		//Transform vetrices in world space
		void VertexTransformationFunction(std::vector<Mesh>& mesh) const;

		//Display functions
		//Called by input pressure
		void ToggleDisplayZBuffer(); //F4
		void ToggleRotation(); //F5
		void ToggleNormalMap(); //F6
		void CycleShadingMode(); //F7

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};
		float* m_pDepthBufferPixels{};

		Camera m_Camera{ {0,0,0}, 45 };

		//Loaded texture for pixel shading
		Texture* m_pTexture;
		Texture* m_pNormalMap;
		Texture* m_pGlossyMap;
		Texture* m_pSpecularMap;
		
		//To calculate the size of the window
		int m_Width{};
		int m_Height{};

		//Inpute influenced variables
		bool m_DisplayZBuffer{ false };
		bool m_IsRotationEnabled{ false };
		bool m_DisplayNormalMap{ false };
		ShadingMode m_ShadingMode{ ShadingMode::ObservedArea };

		//Mesh transform
		float m_MeshRotationAngle{ 0 };
		const float m_MeshRotationSpeed{ 1.0f };
		const Vector3 m_MeshPosition{ 0,0,0.0f };


		//All the meshes
		std::vector<Mesh> m_MeshesWorld{}; //The vector of mesh on which we apply all the transformation
		std::vector<Mesh> m_NonTransformedMeshes{}; //The meshes before transformation, so we avoid creating a vector each frame

		//Vectors to stors vetrices and indices from ParseOBJ function
		std::vector<Vertex> m_MeshVetrices{};
		std::vector<uint32_t> m_MeshIndices{};

		//To check if a triangle could be visible on a certain pixel
		bool HitTest_Triangle(const Vector2& v0, const Vector2& v1, const Vector2& v2, const Vector2& pixel, Vector3& barycentricWeights) const;

		//Render a certain pixel of a mesh
		void RenderAPixel(const int px, const int py, const Vector4& v0, const Vector4& v1, const Vector4& v2, const Mesh& mesh, const int index);

		//Render all the pixels of a mesh
		void RenderMeshes(std::vector<Mesh>& meshesWorld);

		//Transform X and Y world value into screenspace values
		Vector2 ToScreenSpace(const float x, const float y) const;

		//Remap a value between min and max
		float Remap(const float colorValue, const float min, const float max) const;

		//Shade a pixel with texture values and Input options
		ColorRGB PixelShading(const Vertex_Out& vOut) const;

		void Render_W4_Part1();
	};
}
