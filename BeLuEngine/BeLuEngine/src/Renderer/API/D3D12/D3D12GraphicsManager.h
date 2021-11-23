#ifndef D3D12GRAPHICSMANAGER_H
#define D3D12GRAPHICSMANAGER_H

#include "../IGraphicsManager.h"

class D3D12GraphicsManager : public IGraphicsManager
{
public:
	D3D12GraphicsManager();
	virtual ~D3D12GraphicsManager();

	void Init() override;
private:

};

#endif