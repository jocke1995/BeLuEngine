#ifndef MERGERENDERTASK_H
#define MERGERENDERTASK_H

#include "RenderTask.h"
class Mesh;

class ShaderResourceView;

class MergeRenderTask : public RenderTask
{
public:
	MergeRenderTask(
		ID3D12Device5* device,
		RootSignature* rootSignature,
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		const std::wstring& psoName,
		unsigned int FLAG_THREAD);
	virtual ~MergeRenderTask();
	
	void AddSRVToMerge(const ShaderResourceView* srvToMerge);
	void SetFullScreenQuad(Mesh* mesh);
	void CreateSlotInfo();

	void Execute() override final;
private:
	Mesh* m_pFullScreenQuadMesh = nullptr;
	std::vector<const ShaderResourceView*> m_SRVs;

	SlotInfo m_Info;
	unsigned int m_NumIndices;

};

#endif
