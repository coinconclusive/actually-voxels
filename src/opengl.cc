#include <av/av.hh>
#include <av/opengl.hh>
#include <av/render.hh>
#include <GL/gl3w.h>
#include <fmt/core.h>

namespace av::graphics {
	class OpenGL_Mesh : public Mesh {
	public:
		GLuint VAO, VBO, EBO;

		OpenGL_Mesh(bool indexed, size_t vertexCount, size_t indexCount, const VertexSpecification &spec)
			: Mesh(indexed, vertexCount, indexCount, spec) {}

	private:
		friend OpenGL_Renderer;

		void Create_(Span<uint8_t> vertexData, Span<uint8_t> indexData);
		void Destroy_();
	};

	class OpenGL_Shader : public Shader {
	public:
		GLuint Id;

	private:
		friend OpenGL_Renderer;

		void Create_(const char *vertexSource, const char *fragmentSource);
		void Destroy_();
	};

	static GLuint CompileShader_(const char *source, GLenum type) {
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE) {
			OwningSpan<GLchar> log(1024);
			GLsizei length = 0;
			glGetShaderInfoLog(shader, log.GetByteSize(), &length, log.GetData());
			fmt::print(stderr, "Failed to compile {} shader: {}\n",
				type == GL_VERTEX_SHADER ? "vertex" : "fragment",
				log.GetData()
			);
			exit(1);
		}
		return shader;
	}

	void OpenGL_Shader::Create_(const char *vertexSource, const char *fragmentSource) {
		GLuint vertShader = CompileShader_(vertexSource, GL_VERTEX_SHADER);
		GLuint fragShader = CompileShader_(fragmentSource, GL_FRAGMENT_SHADER);

		Id = glCreateProgram();
		glAttachShader(Id, vertShader);
		glAttachShader(Id, fragShader);
		glLinkProgram(Id);
		GLint status = 0;
		glGetProgramiv(Id, GL_LINK_STATUS, &status);
		if (status == GL_FALSE) {
			OwningSpan<GLchar> log(1024);
			GLsizei length = 0;
			glGetProgramInfoLog(Id, log.GetByteSize(), &length, log.GetData());
			fmt::print(stderr, "Failed to link program: {}\n", log.GetData());
			exit(1);
		}

		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
	}

	void OpenGL_Shader::Destroy_() {
		glDeleteProgram(Id);
	}

	GLenum DataTypeToGLenum_(DataType type) {
		switch (type) {
			case DataType::Float32: return GL_FLOAT;
			case DataType::Float64: return GL_DOUBLE;
			case DataType::Int64: return GL_INT64_ARB;
			case DataType::Int32: return GL_INT;
			case DataType::Int16: return GL_SHORT;
			case DataType::Int8: return GL_BYTE;
			case DataType::UInt64: return GL_UNSIGNED_INT64_ARB;
			case DataType::UInt32: return GL_UNSIGNED_INT;
			case DataType::UInt16: return GL_UNSIGNED_SHORT;
			case DataType::UInt8: return GL_UNSIGNED_BYTE;
			case DataType::Uniform_Sampler2D: return GL_SAMPLER_2D;
			case DataType::Uniform_SamplerCube: return GL_SAMPLER_CUBE;
		}
	}

	void OpenGL_Mesh::Create_(Span<uint8_t> vertexData, Span<uint8_t> indexData) {
		glCreateVertexArrays(1, &VAO);
		glCreateBuffers(1, &VBO);
		if (IsIndexed()) glCreateBuffers(1, &EBO);

		glVertexArrayVertexBuffer(VAO, 0, VBO, 0, GetVertexSpec().PackedSize());
		if (IsIndexed()) glVertexArrayElementBuffer(VAO, EBO);

		size_t index = 0, offset = 0;
		for (const auto &attr : GetVertexSpec().Attributes) {
			glVertexArrayAttribFormat(
				VAO, index,
				attr.Dimension,
				DataTypeToGLenum_(attr.Type),
				GL_FALSE,
				offset
			);

			glVertexArrayAttribBinding(VAO, index, 0);

			glEnableVertexArrayAttrib(VAO, index);

			offset += attr.GetPackedSize();
			index += 1;
		}

		glNamedBufferStorage(VBO, vertexData.GetByteSize(), vertexData.GetData(), 0);
		if (IsIndexed()) glNamedBufferStorage(EBO, indexData.GetByteSize(), indexData.GetData(), 0);
	}

	void OpenGL_Mesh::Destroy_() {
		if (IsIndexed()) glDeleteBuffers(1, &EBO);
		glDeleteBuffers(1, &VBO);
		glDeleteVertexArrays(1, &VAO);
	}

	void OpenGL_Renderer::Initialize() {
		// glEnable(GL_DEPTH_TEST);
	}

	void OpenGL_Renderer::DeInitialize() {

	}
	
	Owned<Mesh> OpenGL_Renderer::CreateMesh(
		Span<uint8_t> vertexData,
		Span<uint8_t> indexData,
		const VertexSpecification &spec
	) {
		auto *m = new OpenGL_Mesh(
			true,
			vertexData.GetByteSize() / spec.PackedSize(),
			indexData.GetByteSize(),
			spec
		);
		m->Create_(vertexData, indexData);
		return Owned<Mesh>(m);
	}

	Owned<Mesh> OpenGL_Renderer::CreateMesh(
		Span<uint8_t> vertexData,
		const VertexSpecification &spec
	) {
		auto *m = new OpenGL_Mesh(
			false,
			vertexData.GetByteSize() / spec.PackedSize(),
			0,
			spec
		);
		m->Create_(vertexData, { nullptr, 0 });
		return Owned<Mesh>(m);
	}

	Owned<Shader> OpenGL_Renderer::CreateShader(const char *vertexSource, const char *fragmentSource) {
		auto *shader = new OpenGL_Shader();
		shader->Create_(vertexSource, fragmentSource);
		return Owned<Shader>(shader);
	}

	void OpenGL_Renderer::DestroyMesh(Owned<Mesh> &&mesh) {
		((OpenGL_Mesh*)mesh.Get())->Destroy_();
	}

	void OpenGL_Renderer::DestroyShader(Owned<Shader> &&shader) {
		((OpenGL_Shader*)shader.Get())->Destroy_();
	}

	static void DrawMesh_(Ref<Mesh> mesh_, Ref<Shader> shader_);
	static void Clear_(float r, float g, float b, float a);
	static void SetUniform_(const UniformData &data, OpenGL_Shader *boundShader);

	void OpenGL_Renderer::FlushCommandBuffer(Ref<CommandBuffer> cmdBuf) {
		CommandBufferReader reader(cmdBuf->GetData());
		OpenGL_Shader *boundShader = nullptr;
		while (true) {
			auto type = reader.ReadType();
			if (type == CommandType::End) break;

			switch (type) {
			case CommandType::DrawMesh: {
				auto *mesh = (OpenGL_Mesh*)reader.ReadCmdDrawMesh();
				DrawMesh_(mesh, boundShader);
			} break;
			case CommandType::BindShader: {
				boundShader = (OpenGL_Shader*)reader.ReadCmdBindShader();
			} break;
			case CommandType::Uniform: {
				UniformData data = reader.ReadCmdUniform();
				SetUniform_(data, boundShader);
			} break;
			case CommandType::Clear: {
				ClearColor col = reader.ReadCmdClear();
				Clear_(col.r, col.g, col.b, col.a);
			} break;
			case CommandType::End: break;
			}
		}
	}

	static void SetUniform_(const UniformData &data, OpenGL_Shader *boundShader) {
		auto loc = glGetUniformLocation(boundShader->Id, data.Name);
		if (data.Type == DataType::Float32) {
			auto v = (float*)data.Data.GetData();
			// fmt::print("-----------\n");
			// fmt::print("{} {} {} {}\n", v[0], v[1], v[2], v[3]);
			// fmt::print("{} {} {} {}\n", v[4], v[5], v[6], v[7]);
			// fmt::print("{} {} {} {}\n", v[8], v[9], v[10], v[11]);
			// fmt::print("{} {} {} {}\n", v[12], v[13], v[14], v[15]);
			// fmt::print("-----------\n\n");

			if (data.SizeY == 1 || data.SizeX == data.SizeY)
			switch (data.SizeX * data.SizeY) {
			case  1: glProgramUniform1f(boundShader->Id, loc, v[0]); break;
			case  2: glProgramUniform2f(boundShader->Id, loc, v[0], v[1]); break;
			case  3: glProgramUniform3f(boundShader->Id, loc, v[0], v[1], v[2]); break;
			case  4: glProgramUniform4f(boundShader->Id, loc, v[0], v[1], v[2], v[3]); break;
			case  9: glProgramUniformMatrix3fv(boundShader->Id, loc, 1, GL_FALSE, v); break;
			case 16: glProgramUniformMatrix4fv(boundShader->Id, loc, 1, GL_FALSE, v); break;
			}
		} else {
			fmt::print(stderr, "SetUniform_ data type not yet supported! {}",
				DataTypeToString(data.Type));
			exit(1);
		}
	}

	static void DrawMesh_(Ref<Mesh> mesh_, Ref<Shader> shader_) {
		auto *shader = (OpenGL_Shader*)shader_.Get();
		auto *mesh = (OpenGL_Mesh*)mesh_.Get();

		glUseProgram(shader->Id);
		glBindVertexArray(mesh->VAO);

		if (mesh->IsIndexed()) {
			glDrawElements(
				GL_TRIANGLES,
				mesh->GetIndexCount(),
				DataTypeToGLenum_(mesh->GetVertexSpec().IndexType),
				nullptr
			);
		} else {
			glDrawArrays(
				GL_TRIANGLES,
				0,
				mesh->GetVertexCount()
			);
		}
	}

	static void Clear_(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}
