#ifndef LIGHT_H
#define LIGHT_H

#include "EngineMath.h"

class BaseCamera;

class ShaderResource;

enum F_LIGHT_FLAGS
{
	// Set flag to make the light position inherit the position of the corresponding m_pMesh
	USE_TRANSFORM_POSITION = BIT(1),

	// Option to make the light cast shadows or not. (CURRENTLY NO SHADOWS, will be used again when raytracing is implemented)
	CAST_SHADOW = BIT(2),

	// etc..
	NUM_FLAGS_LIGHT = 2
};

static unsigned int s_LightIdCounter = 0;

class Light
{
public:
	Light(unsigned int lightFlags = 0);
	virtual ~Light();

	bool operator== (const Light& other);

	virtual void Update(double dt) = 0;

	void SetColor(float3 color);
	void SetIntensity(float intensity);

	// Gets
	unsigned int GetLightFlags() const;
	virtual void* GetLightData() const = 0;

protected:
	BaseLight* m_pBaseLight = nullptr;
	unsigned int m_LightFlags = 0;
	unsigned int m_Id = 0;

	virtual void UpdateLightColor() = 0;

	// For usage in the lightRawBuffer (memory on GPU)
private:
	friend class Renderer;
	friend class ImGuiHandler;

	static inline ShaderResource* m_pLightsRawBuffer = nullptr;
	static inline unsigned char* m_pRawData			 = nullptr;
	unsigned int m_LightOffsetInArray = 0;
};

#endif