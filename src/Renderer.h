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
		void VertexTransformationFunction(Mesh& mesh) const;

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};
		//float* m_pDepthBufferPixels{};

		Camera m_Camera{ {0,0,-10}, 60 };

		Texture* m_pTexture;

		int m_Width{};
		int m_Height{};

		bool HitTest_Triangle(const std::vector<Vector2>& triangle, const Vector2& pixel, Vector3& barycentricWeights, const int triangleNr) const;
		bool HitTest_Triangle(const Vector2& v0, const Vector2& v1, const Vector2& v2, const Vector2& pixel, Vector3& barycentricWeights) const;

		void RasterizeTriangle(const std::vector<Vertex>& triangle) const;
		Vector2 ToScreenSpace(const float x, const float y) const;
		void ClampToNDC(Vector2& point) const;
		std::vector<Vertex> MeshToVetrices(const Mesh& mesh) const;


		void Render_W1_Part1() const;
		void Render_W1_Part2() const;
		void Render_W1_Part3() const;
		void Render_W1_Part4() const;
		void Render_W1_Part5() const;
		void Render_W2_Part1() const; //TriangleList
		void Render_W2_Part2() const; //TriangleStrip
		void Render_W2_Part3() const; //UV
		
		void Render_W3_Part1() const; // Projection Matrix
		void Render_W3_Part2()const;
	};
}
