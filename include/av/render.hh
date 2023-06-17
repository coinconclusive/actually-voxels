#include <av/av.hh>

namespace av::graphics {
	enum class CommandType : uint8_t {
		DrawMesh = 0x00, BindShader = 0x01, Uniform = 0x02, Clear = 0x03,
		End = 0xFF
	};

	struct UniformData {
		DataType Type;
		uint8_t SizeX : 4, SizeY : 4;
		const char *Name;
		Span<const uint8_t> Data;
	};

	struct ClearColor {
		float r, g, b, a;
	};

	class CommandBufferReader {
	public:
		CommandBufferReader(Span<const uint8_t> data) : Data_(data) {}

		CommandType ReadType();
		Mesh *ReadCmdDrawMesh();
		Shader *ReadCmdBindShader();
		UniformData ReadCmdUniform();
		ClearColor ReadCmdClear();

	private:
		size_t Offset_ = 0;
		Span<const uint8_t> Data_;
	};
}
