#ifndef BRIGHTBLURTASK_H
#define BRIGHTBLURTASK_H

#include "../GraphicsPass.h"

class IGraphicsTexture;

class BloomComputePass : public GraphicsPass
{
public:
	BloomComputePass(unsigned int screenWidth, unsigned int screenHeight);
	virtual ~BloomComputePass();

	void Execute() override final;
private:
	// The compute shader will read and write in a "Ping-Pong"-order to these objects.
	std::array<IGraphicsTexture*, 2> m_PingPongTextures = {nullptr, nullptr};

	unsigned int m_ScreenWidth = 0;
	unsigned int m_ScreenHeight = 0;

	unsigned int m_HorizontalThreadGroupsX;
	unsigned int m_HorizontalThreadGroupsY;
	unsigned int m_VerticalThreadGroupsX;
	unsigned int m_VerticalThreadGroupsY;
	const unsigned int m_ThreadsPerGroup = 64;
};

#endif