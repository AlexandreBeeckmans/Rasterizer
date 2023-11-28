#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

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

		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const;
		void VertexTransformationFunction(std::vector<Mesh>& mesh) const;
		void ToggleDisplayZBuffer();

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};
		float* m_pDepthBufferPixels{nullptr};

		Camera m_Camera{ {0,0,-10}, 60 };

		Texture* m_pTexture;

		int m_Width{};
		int m_Height{};

		bool m_DisplayZBuffer{ false };
		float m_MeshRotationAngle{ 0 };
		const float m_MeshRotationSpeed{ 2 };


		bool HitTest_Triangle(const std::vector<Vector2>& triangle, const Vector2& pixel, Vector3& barycentricWeights, const int triangleNr) const;
		bool HitTest_Triangle(const Vector2& v0, const Vector2& v1, const Vector2& v2, const Vector2& pixel, Vector3& barycentricWeights) const;

		void RasterizeTriangle(const std::vector<Vertex>& triangle) const;
		void RenderAPixel(const int px, const int py, const Vector4& v0, const Vector4& v1, const Vector4& v2, const Mesh& mesh, const int index);
		void RenderMeshes(std::vector<Mesh>& meshesWorld);
		Vector2 ToScreenSpace(const float x, const float y) const;
		void ClampToNDC(Vector2& point) const;
		std::vector<Vertex> MeshToVetrices(const Mesh& mesh) const;

		float Remap(const float colorValue, const float min, const float max) const;


		void Render_W1_Part1() const;
		void Render_W1_Part2() const;
		void Render_W1_Part3() const;
		void Render_W1_Part4() const;
		void Render_W1_Part5() const;
		void Render_W2_Part1() const; //TriangleList
		void Render_W2_Part2() const; //TriangleStrip
		void Render_W2_Part3() const; //UV
		
		void Render_W3_Part1(); // Projection Matrix
		void Render_W3_Part2(); //3D model

		void Render_W4_Part1();
	};
}
