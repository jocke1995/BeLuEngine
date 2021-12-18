#ifndef BRIGHTBLURTASK_H
#define BRIGHTBLURTASK_H

#include "GraphicsPass.h"

class Bloom;

class BlurComputeTask : public GraphicsPass
{
public:
	BlurComputeTask(Bloom* bloom, unsigned int screenWidth, unsigned int screenHeight);
	virtual ~BlurComputeTask();

	void Execute() override final;
private:
	Bloom* m_pTempBloom = nullptr;

	unsigned int m_HorizontalThreadGroupsX;
	unsigned int m_HorizontalThreadGroupsY;
	unsigned int m_VerticalThreadGroupsX;
	unsigned int m_VerticalThreadGroupsY;
	const unsigned int m_ThreadsPerGroup = 256;
};

#endif