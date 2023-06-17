#pragma once
#include <av/av.hh>

namespace av::graphics {
	class OpenGL_Renderer : public Renderer {
	public:
		virtual Owned<Mesh> CreateMesh(
			Span<uint8_t> vertexData,
			Span<uint8_t> indexData,
			const VertexSpecification &spec
		) override;

		virtual Owned<Mesh> CreateMesh(
			Span<uint8_t> vertexData,
			const VertexSpecification &spec
		) override;

		Owned<Shader> CreateShader(const char *vertexSource, const char *fragmentSource) override;

		void DestroyMesh(Owned<Mesh> &&mesh) override;
		void DestroyShader(Owned<Shader> &&shader) override;

		void FlushCommandBuffer(Ref<CommandBuffer>) override;

		void Initialize() override;
		void DeInitialize() override;
	};
}
