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
		COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int FLAG_THREAD,
		const std::wstring& clName);
	virtual ~CopyOnDemandTask();

	void SubmitTexture(Texture* texture);

	// Removal
	void Clear() override;

	void Execute() override final;

private:
	std::vector<Texture*> m_Textures;
	void copyTexture(ID3D12GraphicsCommandList5* commandList, Texture* texture);
};

#endif
