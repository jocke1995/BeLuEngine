#ifndef DX12TASK_H
#define DX12TASK_H
#include "Core.h"

#include "../../Misc/MultiThreading/MultiThreadedTask.h"
class CommandInterface;
class Resource;
class DescriptorHeap;

enum E_COMMAND_INTERFACE_TYPE;
enum class E_DESCRIPTOR_HEAP_TYPE;
enum D3D12_RESOURCE_STATES;

// DX12 Forward Declarations
struct ID3D12GraphicsCommandList5;

// These renderTasks will execute on "all objects"
enum E_RENDER_TASK_TYPE
{
	DEPTH_PRE_PASS,
	DEFERRED_LIGHT,
	DEFERRED_GEOMETRY,
	OPACITY,
	WIREFRAME,
	OUTLINE,
	MERGE,
	IMGUI,
	DOWNSAMPLE,
	NR_OF_RENDERTASKS
};

enum E_COMPUTE_TASK_TYPE
{
	BLUR,
	NR_OF_COMPUTETASKS
};

enum E_COPY_TASK_TYPE
{
	COPY_PER_FRAME,
	COPY_PER_FRAME_MATRICES,
	COPY_ON_DEMAND,
	NR_OF_COPYTASKS
};

enum E_DXR_TASK_TYPE
{
	REFLECTIONS,
	NR_OF_DXRTASKS
};

// These tasks are for GPU tasks which are very barebone, such as only submitting specific data (such as TLAS:es and BLAS:es)
enum E_DX12_TASK_TYPE
{
	BLAS,
	TLAS,
	NR_OF_DX12TASKS
};

// Dont create this class immediatly, use the #define below
struct ID3D12GraphicsCommandList;
class ScopedPIXEvent
{
public:
	ScopedPIXEvent(const char* nameOfTask, ID3D12GraphicsCommandList* cl);
	~ScopedPIXEvent();

private:
	ID3D12GraphicsCommandList* m_pCommandList = nullptr;
};

#ifdef DEBUG	// This is both for Debug and Release, not for Dist
	#define ScopedPixEvent(name, commandList) ScopedPIXEvent concat(PIX_Event_Marker, __LINE__)(#name, commandList);
#else
	#define ScopedPixEvent(name, commandList);
#endif

namespace component
{
	class ModelComponent;
	class TransformComponent;
}

struct RenderComponent
{
public:
	RenderComponent(component::ModelComponent* mc, component::TransformComponent* tc)
		:mc(mc), tc(tc) {};

	component::ModelComponent* mc = nullptr;
	component::TransformComponent* tc = nullptr;
};

class DX12Task : public MultiThreadedTask
{
public:
	DX12Task(
		ID3D12Device5* device,
		E_COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int FLAG_THREAD,
		const std::wstring& clName);
	virtual ~DX12Task();

	static void SetBackBufferIndex(int backBufferIndex);
	static void SetCommandInterfaceIndex(int index);

	void AddResource(std::string id, Resource* resource);

	CommandInterface* const GetCommandInterface() const;
protected:
	std::map<std::string, Resource*> m_Resources;

	CommandInterface* m_pCommandInterface = nullptr;
	inline static int m_BackBufferIndex = -1;
	inline static int m_CommandInterfaceIndex = -1;
};

#endif