#include <av/render.hh>
#include <cstring>
#include <fmt/core.h>

#define DEBUG_CMD_BUF 0

#define FMT_DEBUG(...) if (DEBUG_CMD_BUF) fmt::print(__VA_ARGS__)

namespace av::graphics {
	void CommandBuffer::CmdBindShader(Ref<Shader> shader) {
		FMT_DEBUG(stderr, "CmdBuf/BindShader {}\n", (void*)shader.Get());
		Data_.Resize(Data_.GetCount() + 1 + sizeof(Shader*));
		Data_[Offset_++] = (uint8_t)CommandType::BindShader;
		*(Shader**)(Data_.GetData() + Offset_) = shader.Get();
		Offset_ += sizeof(Shader*);
		Count_ += 1;
	}

	void CommandBuffer::CmdDrawMesh(Ref<Mesh> mesh) {
		FMT_DEBUG(stderr, "CmdBuf/DrawMesh {}\n", (void*)mesh.Get());
		Data_.Resize(Data_.GetCount() + 1 + sizeof(Mesh*));
		Data_[Offset_++] = (uint8_t)CommandType::DrawMesh;
		*(Mesh**)(Data_.GetData() + Offset_) = mesh.Get();
		Offset_ += sizeof(Mesh*);
		Count_ += 1;
	}

	void CommandBuffer::CmdClear(float r, float g, float b, float a) {
		FMT_DEBUG(stderr, "CmdBuf/Clear {} {} {} {}\n", r, g, b, a);
		Data_.Resize(Data_.GetCount() + 1 + sizeof(ClearColor));
		Data_[Offset_++] = (uint8_t)CommandType::Clear;
		*(ClearColor*)(Data_.GetData() + Offset_) = { r, g, b, a };
		Offset_ += sizeof(ClearColor);
		Count_ += 1;
	}

	void CommandBuffer::CmdUniform(const char *name, void *value, DataType dataType, int x, int y) {
		FMT_DEBUG(stderr, "CmdBuf/Uniform '{}' {} {} {}x{}\n", name, value,
			DataTypeToString(dataType), x, y);
		size_t nameSize = strlen(name) + 1;
		size_t dataSize = VertexAttribute::GetElementSize(dataType) * x * y;

		Data_.Resize(Data_.GetCount() + 1 + 4 + nameSize + dataSize);

		Data_[Offset_++] = (uint8_t)CommandType::Uniform; // + 1
		
		Data_[Offset_++] = (uint8_t)dataType; // 1

		Data_[Offset_++] = (x & 0xFF) | (y << 4); // 2
		
		Data_[Offset_++] = nameSize; // 3
		
		CopyItems(Data_.GetData() + Offset_, (uint8_t*)name, nameSize);
		Offset_ += nameSize; // 3 + nameSize

		Data_[Offset_++] = dataSize; // 4 + nameSize
		CopyItems(Data_.GetData() + Offset_, (uint8_t*)value, dataSize);

		Offset_ += dataSize; // 4 + nameSize + dataSize

		Count_ += 1;
	}

	void CommandBuffer::End() {
		FMT_DEBUG(stderr, "CmdBuf/End\n");
		Data_.Resize(Data_.GetCount() + 1);
		Data_[Offset_++] = (uint8_t)CommandType::End;
		Count_ += 1;
	}

	CommandType CommandBufferReader::ReadType() {
		return (CommandType)Data_[Offset_++];
	}

	Mesh *CommandBufferReader::ReadCmdDrawMesh() {
		auto *v = *(Mesh**)(Data_.GetData() + Offset_);
		Offset_ += sizeof(uint64_t);
		FMT_DEBUG(stderr, "CmdBufReader/DrawMesh {}\n", (void*)v);
		return v;
	}

	Shader *CommandBufferReader::ReadCmdBindShader() {
		auto *v = *(Shader**)(Data_.GetData() + Offset_);
		Offset_ += sizeof(uint64_t);
		FMT_DEBUG(stderr, "CmdBufReader/BindShader {}\n", (void*)v);
		return v;
	}

	UniformData CommandBufferReader::ReadCmdUniform() {
		UniformData data;
		data.Type = (DataType)Data_[Offset_++];
		data.SizeX = Data_[Offset_] & 0xFF;
		data.SizeY = Data_[Offset_] >> 4;
		Offset_ += 1;
		size_t nameSize = Data_[Offset_++];
		data.Name = (const char *)Data_.GetData() + Offset_;
		Offset_ += nameSize;
		size_t dataSize = Data_[Offset_++];
		data.Data = { Data_.GetData() + Offset_, dataSize };
		Offset_ += dataSize;
		FMT_DEBUG(stderr, "CmdBufReader/Uniform '{}' {} {} {}x{}\n",
			data.Name, (void*)data.Data.GetData(), DataTypeToString(data.Type),
			(int)data.SizeX, (int)data.SizeY);
		return data;
	}

	ClearColor CommandBufferReader::ReadCmdClear() {
		auto v = (ClearColor*)(Data_.GetData() + Offset_);
		Offset_ += sizeof(ClearColor);
		FMT_DEBUG(stderr, "CmdBufReader/Clear {} {} {} {}\n", v->r, v->g, v->b, v->a);
		return *v;
	}
}
