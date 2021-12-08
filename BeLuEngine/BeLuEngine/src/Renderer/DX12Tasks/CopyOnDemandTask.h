#ifndef COPYONDEMANDTASK_H
#define COPYONDEMANDTASK_H

#include "CopyTask.h"
class Texture;
class Mesh;

class CopyOnDemandTask : public CopyTask
{
public:
	CopyOnDemandTask(
		ID3D12Device5* device,
		E_COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int FLAG_THREAD,
		const std::wstring& clName);
	virtual ~CopyOnDemandTask();

	// Removal
	void Clear() override;

	void Execute() override final;

private:
	std::vector<IGraphicsTexture*> m_Textures;
	void copyTexture(ID3D12GraphicsCommandList5* commandList, IGraphicsTexture* texture);
};

#endif
