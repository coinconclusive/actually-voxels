#include <av/av.hh>
#include <av/opengl.hh>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader.hh>

/// Adds a nul at the end.
av::OwningSpan<char> ReadFile(const char *filename) {
	FILE *f = fopen(filename, "r");
	size_t fsz = av::fs::GetFileSize(f);
	av::OwningSpan<char> owning;
	owning.Resize(fsz + 1);
	fread(owning.GetData(), fsz, 1, f);
	owning[owning.GetCount() - 1] = '\0';
	return owning;
}

av::Owned<av::graphics::Shader> CreateShaderFromFiles(
	av::Ref<av::graphics::Renderer> renderer,
	const char *fnameVert,
	const char *fnameFrag
) {
	auto vertexSource = ReadFile(fnameVert);
	auto fragmentSource = ReadFile(fnameFrag);
	return renderer->CreateShader(vertexSource.GetData(), fragmentSource.GetData());
}

const char *ConvertGLDebugSourceToString(GLenum source) {
	switch (source) {
	case GL_DEBUG_SOURCE_API: return "OpenGL API";
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "Window system API";
	case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader compiler";
	case GL_DEBUG_SOURCE_THIRD_PARTY: return "Third-party";
	case GL_DEBUG_SOURCE_APPLICATION: return "Application";
	case GL_DEBUG_SOURCE_OTHER: return "Other";
	default: return "Unknown";
	}
}

const char *ConvertGLDebugSeverityToString(GLenum severity) {
	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH: return "High";
	case GL_DEBUG_SEVERITY_MEDIUM: return "Medium";
	case GL_DEBUG_SEVERITY_LOW: return "Low";
	case GL_DEBUG_SEVERITY_NOTIFICATION: return "Notification";
	default: return "Unknown";
	}
}

const char *ConvertGLDebugMessageTypeToString(GLenum type) {
	switch (type) {
	case GL_DEBUG_TYPE_ERROR: return "Error";
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated Behaviour";
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "Undefined Behaviour";
	case GL_DEBUG_TYPE_PORTABILITY: return "Portability";
	case GL_DEBUG_TYPE_PERFORMANCE: return "Performance";
	case GL_DEBUG_TYPE_MARKER: return "Marker";
	case GL_DEBUG_TYPE_PUSH_GROUP: return "Push Group";
	case GL_DEBUG_TYPE_POP_GROUP: return "Pop Group";
	case GL_DEBUG_TYPE_OTHER: return "Other";
	default: return "Unknown";
	}
}

av::Owned<av::graphics::Mesh> CreateMeshFromObjFile(av::graphics::Renderer *renderer, const char *fname) {
	tinyobj::ObjReaderConfig config;
	config.triangulate = true;

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(fname, config)) {
		if (!reader.Error().empty()) {
			fmt::print(stderr, "Error loading obj file: {}", reader.Error());
		}
		exit(1);
	}
	
	if (!reader.Warning().empty()) {
		fmt::print(stderr, "Warning loading obj file: {}", reader.Error());
	}
	
	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 norm;
		glm::vec2 tex;
	};

	av::graphics::VertexSpecification vertexSpec;
	vertexSpec.IndexType = av::graphics::DataType::Int16;
	vertexSpec.Attributes.Resize(3);
	vertexSpec.Attributes[0].Type = av::graphics::DataType::Float32;
	vertexSpec.Attributes[0].Dimension = 3;
	vertexSpec.Attributes[1].Type = av::graphics::DataType::Float32;
	vertexSpec.Attributes[1].Dimension = 3;
	vertexSpec.Attributes[2].Type = av::graphics::DataType::Float32;
	vertexSpec.Attributes[2].Dimension = 2;

	size_t totalVertexCount = 0;
	for (size_t s = 0; s < shapes.size(); ++s) {
		totalVertexCount = shapes[s].mesh.num_face_vertices.size() * 3;
	}

	av::OwningSpan<Vertex> vertices(totalVertexCount);

	size_t vertexIndex = 0;
	for (size_t s = 0; s < shapes.size(); ++s) {
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
			for (size_t v = 0; v < 3; ++v) {
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
				tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
				tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];

				tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
				tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
				tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];

				tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
				tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
				
				vertices[vertexIndex++] = { { vx, vy, vz }, { nx, ny, nz }, { tx, ty } };
			}

			index_offset += 3;
		}
	}

	Vertex vertices2[6] = {
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { +1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { -1.0f, +1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },

		{ { +1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { -1.0f, +1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
		{ { +1.0f, +1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
	};

	return renderer->CreateMesh({ (uint8_t*)vertices2, sizeof(vertices2) }, vertexSpec);
}

class Transform {
public:
	[[nodiscard]] const glm::vec3 &Position() const { return position_; }
	[[nodiscard]] const glm::quat &Rotation() const { return rotation_; }
	[[nodiscard]] const glm::vec3 &Scale() const { return scale_; }

	void Position(const glm::vec3 &v) { position_ = v; dirty_ = true; }
	void Rotation(const glm::quat &v) { rotation_ = v; dirty_ = true; }
	void Scale(const glm::vec3 &v) { scale_ = v; dirty_ = true; }

	[[nodiscard]] glm::mat4 Matrix() const {
		if (dirty_) {
			glm::mat4 result;
			ComputeMatrix_(result);
			return result;
		}
		return matrix_;
	}

	[[nodiscard]] const glm::mat4 &Matrix() {
		if (dirty_) ComputeMatrix_(matrix_);
		return matrix_;
	}

private:
	void ComputeMatrix_(glm::mat4 &result) const {
		result = glm::mat4(1.0);
		result = glm::translate(matrix_, position_);
		result = matrix_ * glm::mat4_cast(rotation_);
		result = glm::scale(matrix_, scale_);
	}

	glm::vec3 position_;
	glm::quat rotation_ = glm::identity<glm::quat>();
	glm::vec3 scale_;
	glm::mat4 matrix_;
	bool dirty_;
};

class Camera {
public:
	Camera(float fov, float aspectRatio, float near, float far)
		: FOV_(fov), AspectRatio_(aspectRatio), Near_(near), Far_(far) {}

	glm::mat4 ComputeMatrix() {
		glm::mat4 p = glm::perspective(FOV_, AspectRatio_, Near_, Far_);
		glm::mat4 v = glm::lookAt(
			Trans_.Position(),
			Trans_.Position() + glm::vec3(0.0f, 0.0f, -1.0f) * Trans_.Rotation(),
			{ 0.0f, 1.0f, 0.0f }
		);
		return p * v;
	}

	float Near() { return Near_; }
	float Far() { return Far_; }
	float AspectRatio() { return AspectRatio_; }
	float FOV() { return FOV_; }

	Transform &GetTransform() { return Trans_; }
	const Transform &GetTransform() const { return Trans_; }
	
private:
	float FOV_;
	float AspectRatio_;
	float Near_, Far_;
	Transform Trans_;
};


int main() {
	glfwSetErrorCallback([](int error, const char *message) {
		fmt::print(stderr, "GLFW error: {} {}\n", error, message);
	});

	if (!glfwInit()) return 1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	GLFWwindow *window = glfwCreateWindow(640, 480, "Window", nullptr, nullptr);
	
	if (window == nullptr) return 1;

	glfwMakeContextCurrent(window);
	
	if (gl3wInit() < 0) {
		fmt::print(stderr, "GL3W failed to initialize!\n");
		return 1;
	}

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback([](
		GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
		const GLchar *message, const void *userParam
	) {
		fmt::print(stderr, "\033[1;31mGL Debug Message: source={} type={} id={} severity={} {}\033[m\n",
			ConvertGLDebugSourceToString(source),
			ConvertGLDebugMessageTypeToString(type),
			id,
			ConvertGLDebugSeverityToString(severity),
			message
		);
	}, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);

	av::graphics::OpenGL_Renderer renderer;

	renderer.Initialize();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	auto mesh = CreateMeshFromObjFile(&renderer, "./data/meshes/cube.obj");
	auto shader = CreateShaderFromFiles(&renderer, "./data/shaders/main.vert", "./data/shaders/main.frag");

	Camera cam(90.0f, width /(float) height, 0.01f, 100.0f);
	cam.GetTransform().Position({ 0, 1.0f, 5.0f });
	cam.GetTransform().Rotation(glm::quatLookAt(
		glm::normalize(glm::vec3(0.f, 1.f/5.f, -1.f)),
		{ 0.f, 1.f, 0.f }
	));

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		auto mat = cam.ComputeMatrix();

		av::graphics::CommandBuffer buffer;
		buffer.CmdClear(0.2f, 0.1, 0.3f, 1.0f);
		buffer.CmdBindShader(shader);
		buffer.CmdUniform("uTransform", glm::value_ptr(mat), av::graphics::DataType::Float32, 4, 4);
		buffer.CmdDrawMesh(mesh);
		buffer.End();
		renderer.FlushCommandBuffer(&buffer);

		glfwSwapBuffers(window);
	}

	renderer.DestroyShader(std::move(shader));
	renderer.DestroyMesh(std::move(mesh));
	renderer.DeInitialize();

	glfwDestroyWindow(window);
	glfwTerminate();
}
