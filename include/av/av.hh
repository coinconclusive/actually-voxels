#pragma once
#include <cstdio>
#include <cstddef>
#include <cstdint>

namespace av {
	template<typename T>
	class Ref;

	template<typename T>
	class Owned {
	public:
		Owned() : Ptr_(nullptr) {}

		template<typename U>
		Owned(U *raw) : Ptr_(raw) {}

		template<typename U>
		Owned(Owned<U> &&moved_) : Ptr_(moved_.Ptr_) {}

		~Owned() { if (Ptr_) delete Ptr_; }

		T *Get() { return Ptr_; }
		const T *Get() const { return Ptr_; }

		const T &operator*() const { return *Ptr_; }
		T &operator*() { return *Ptr_; }

		const T *operator->() const { return Ptr_; }
		T *operator->() { return Ptr_; }

	private:
		T *Ptr_;
	};

	template<typename T>
	class Ref {
	public:
		template<typename U>
		Ref(U *raw) : Ptr_(raw) {}

		template<typename U>
		Ref(Ref<U> ref) : Ptr_(ref.Ptr_) {}

		template<typename U>
		Ref(Owned<U> &owned) : Ptr_(owned.Get()) {}

		T *Get() { return Ptr_; }
		const T *Get() const { return Ptr_; }

		const T &operator*() const { return *Ptr_; }
		T &operator*() { return *Ptr_; }

		const T *operator->() const { return Ptr_; }
		T *operator->() { return Ptr_; }

	private:
		T *Ptr_;
	};

	template<typename T>
	void CopyItems(T *dst, const T *src, size_t count) {
		__builtin_memcpy(dst, src, count * sizeof(T));
	}

	template<typename T>
	class Span {
	public:
		Span() : Data_(nullptr), Count_(0) {}

		template<typename U>
		Span(const U &o) : Data_(o.GetData()), Count_(o.GetCount()) {}

		template<typename U>
		Span(U &o) : Data_(o.GetData()), Count_(o.GetCount()) {}

		Span(T *data, size_t size) : Data_(data), Count_(size) {}

		constexpr size_t GetByteSize() const { return Count_ * sizeof(T); }
		constexpr size_t GetCount() const { return Count_; }
		constexpr const T *begin() const { return Data_; }
		constexpr const T *end() const { return Data_ + Count_; }
		constexpr T *begin() { return Data_; }
		constexpr T *end() { return Data_ + Count_; }
		constexpr T *GetData() { return Data_; }
		constexpr const T *GetData() const { return Data_; }

		constexpr const T &operator[](size_t index) const { return Data_[index]; }
		constexpr T &operator[](size_t index) { return Data_[index]; }

		constexpr Span<T> PeekUntil(const T &elem) {
			T *p = GetData();
			while (*p != elem && p != GetData() + GetCount()) {
				++p;
			}
			return { GetData(), (size_t)(p - GetData()) };
		}

		template<typename F>
		constexpr Span<T> SkipWhile(F pred) {
			T *p = GetData();
			while (pred(*p) && p != GetData() + GetCount()) {
				++p;
			}
			return { GetData(), (size_t)(p - GetData()) };
		}

		constexpr Span<T> PeekAfter(const T &elem) {
			T *p = GetData();
			while (*p != elem && p != GetData() + GetCount()) {
				++p;
			}
			return { p, GetData() + GetCount() };
		}

	protected:
		T *Data_;
		size_t Count_;
	};
	
	template<typename T>
	class OwningSpan : public Span<T> {
	public:
		OwningSpan() : Span<T>(nullptr, 0) {}
		OwningSpan(size_t size) : Span<T>(new T[size], size) {}
		OwningSpan(T *data, size_t size) : Span<T>(data, size) {}
		OwningSpan(OwningSpan &&other) : Span<T>(other.Data_, other.Count_) {}

		~OwningSpan() { delete Span<T>::Data_; }

		OwningSpan<T> Copy() const {
			auto s = OwningSpan<T>(this->Count_);
			CopyItems(s.Data_, this->Data_, this->Count_);
			return s;
		}

		void Resize(size_t newCount) {
			T *newData = new T[newCount];
			CopyItems(newData, this->Data_, newCount > this->Count_ ? this->Count_ : newCount);
			delete this->Data_;
			this->Data_ = newData;
			this->Count_ = newCount;
		}
	};
}

namespace av::fs {
	size_t GetFileSize(const char *filename);
	size_t GetFileSize(FILE *fptr);
}

namespace av::graphics {
	enum class DataType : uint8_t {
		Float32 = 0x00, Float64 = 0x01,
		Int8 = 0x02, Int16 = 0x03, Int32 = 0x04, Int64 = 0x05,
		UInt8 = 0x06, UInt16 = 0x07, UInt32 = 0x08, UInt64 = 0x09,
		Uniform_Sampler2D = 0x0A, Uniform_SamplerCube = 0x0B
	};

	constexpr const char *DataTypeToString(DataType type) {
		switch (type) {
			case DataType::Float32: return "Float32";
			case DataType::Float64: return "Float64";
			case DataType::Int8: return "Int8";
			case DataType::Int16: return "Int16";
			case DataType::Int32: return "Int32";
			case DataType::Int64: return "Int64";
			case DataType::UInt8: return "UInt8";
			case DataType::UInt16: return "UInt16";
			case DataType::UInt32: return "UInt32";
			case DataType::UInt64: return "UInt64";
			case DataType::Uniform_Sampler2D: return "Sampler2D";
			case DataType::Uniform_SamplerCube: return "SamplerCube";
			default: return "Unknown";
		}
	}

	struct VertexAttribute {
		DataType Type;
		size_t Dimension;
		static size_t GetElementSize(DataType type) {
			switch (type) {
			case DataType::Float32: return 4;
			case DataType::Float64: return 8;
			case DataType::Int64: return 8;
			case DataType::Int32: return 4;
			case DataType::Int16: return 2;
			case DataType::Int8: return 1;
			case DataType::UInt64: return 8;
			case DataType::UInt32: return 4;
			case DataType::UInt16: return 2;
			case DataType::UInt8: return 1;
			case DataType::Uniform_Sampler2D: return 4;
			case DataType::Uniform_SamplerCube: return 4;
			}
		}
		size_t GetElementSize() const { return GetElementSize(Type); }
		size_t GetPackedSize() const { return GetElementSize() * Dimension; }
	};

	struct VertexSpecification {
		size_t PackedSize() const {
			size_t total = 0;
			for (const auto &attribute : Attributes) {
				total += attribute.GetPackedSize();
			}
			return total;
		}

		OwningSpan<VertexAttribute> Attributes;
		DataType IndexType;

		VertexSpecification Copy() const {
			return VertexSpecification { Attributes.Copy(), IndexType };
		}
	};

	class Mesh {
		bool Indexed_;
		size_t VertexCount_, IndexCount_;
		VertexSpecification VertexSpec_;

	public:
		Mesh(bool indexed, size_t vertexCount, size_t indexCount, const VertexSpecification &spec)
			: Indexed_(indexed),
			  VertexCount_(vertexCount),
				IndexCount_(indexCount),
				VertexSpec_(spec.Copy()) {
		}

		virtual ~Mesh() = default;

		bool IsIndexed() const { return Indexed_; }
		size_t GetVertexCount() const { return VertexCount_; }
		size_t GetIndexCount() const { return IndexCount_; }
		const VertexSpecification &GetVertexSpec() const { return VertexSpec_; }
	};

	class Shader { };

	class CommandBuffer {
	public:
		void CmdClear(float r, float g, float b, float a);
		void CmdBindShader(Ref<Shader> shader);
		void CmdDrawMesh(Ref<Mesh> mesh);
		void CmdUniform(const char *name, void *value, DataType type, int sizeX = 1, int sizeY = 1);
		
		void CmdUniform(const char *name, float x) {
			CmdUniform(name, &x, DataType::Float32, 1, 1);
		}
		
		void CmdUniform(const char *name, float x, float y) {
			float v[2] = { x, y };
			CmdUniform(name, v, DataType::Float32, 2, 1);
		}
		
		void CmdUniform(const char *name, float x, float y, float z) {
			float v[3] = { x, y, z };
			CmdUniform(name, v, DataType::Float32, 3, 1);
		}

		void End();

		Span<const uint8_t> GetData() const { return { Data_.GetData(), Data_.GetCount() }; }
		size_t GetCount() const { return Count_; }
	private:
		size_t Count_ = 0, Offset_ = 0;
		OwningSpan<uint8_t> Data_;
	};

	class Renderer {
	public:
		virtual Owned<Mesh> CreateMesh(
			Span<uint8_t> vertexData,
			Span<uint8_t> indexData,
			const VertexSpecification &spec
		) = 0;

		virtual Owned<Mesh> CreateMesh(
			Span<uint8_t> vertexData,
			const VertexSpecification &spec
		) = 0;

		virtual Owned<Shader> CreateShader(const char *vertexSource, const char *fragmentSource) = 0;

		virtual void DestroyMesh(Owned<Mesh> &&mesh) = 0;
		virtual void DestroyShader(Owned<Shader> &&shader) = 0;

		virtual void FlushCommandBuffer(Ref<CommandBuffer>) = 0;

		virtual void Initialize() = 0;
		virtual void DeInitialize() = 0;
	};
}
