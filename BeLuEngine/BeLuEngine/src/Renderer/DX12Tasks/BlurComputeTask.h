#ifndef BRIGHTBLURTASK_H
#define BRIGHTBLURTASK_H

#include "ComputeTask.h"

class Bloom;

class BlurComputeTask : public ComputeTask
{
public:
	BlurComputeTask(
		std::vector<std::pair<std::wstring, std::wstring>> csNamePSOName,
		E_COMMAND_INTERFACE_TYPE interfaceType,
		Bloom* bloom,
		unsigned int screenWidth, unsigned int screenHeight,
		unsigned int FLAG_THREAD
		);
	virtual ~BlurComputeTask();

	void Execute() override final;
private:
	Bloom* m_pTempBloom = nullptr;

	unsigned int m_HorizontalThreadGroupsX;
	unsigned int m_HorizontalThreadGroupsY;
	unsigned int m_VerticalThreadGroupsX;
	unsigned int m_VerticalThreadGroupsY;
	const unsigned int m_ThreadsPerGroup = 256;

	DescriptorHeapIndices m_DhIndices = {};
};

#endif