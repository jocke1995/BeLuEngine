#ifndef IGRAPHICSMANAGER_H
#define IGRAPHICSMANAGER_H

enum E_GLOBAL_ROOTSIGNATURE
{
	dtSRV,
	dtCBV,
	dtUAV,
	Constants_SlotInfo_B0,
	Constants_DH_Indices_B1,
	RootParam_CBV_B2,
	RootParam_CBV_B3,
	RootParam_CBV_B4,
	RootParam_CBV_B5,
	RootParam_CBV_B6,
	RootParam_CBV_B7,
	RootParam_SRV_T0,
	RootParam_SRV_T1,
	RootParam_SRV_T2,
	RootParam_SRV_T3,
	RootParam_SRV_T4,
	RootParam_SRV_T5,
	RootParam_UAV_U0,
	RootParam_UAV_U1,
	RootParam_UAV_U2,
	RootParam_UAV_U3,
	RootParam_UAV_U4,
	RootParam_UAV_U5,
	RootParam_UAV_U6,
	NUM_PARAMS
};

enum E_GRAPHICS_API
{
	D3D12_API,
	VULKAN_API,		// One bright day.. :)
	NUM_PARAMS
};

class IGraphicsManager
{
public:
	static IGraphicsManager* GetInstance();
	virtual ~IGraphicsManager();

	static IGraphicsManager* Create(const E_GRAPHICS_API graphicsApi);

	virtual void Init() = 0;
private:
	static IGraphicsManager* m_sInstance;
};

#endif