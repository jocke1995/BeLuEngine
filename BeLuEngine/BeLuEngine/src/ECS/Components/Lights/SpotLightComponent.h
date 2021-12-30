#ifndef SPOTLIGHTCOMPONENT_H
#define SPOTLIGHTCOMPONENT_H

#include "Light.h"
#include "../Component.h"

namespace component
{
	class SpotLightComponent : public Light, public Component
	{
	public:
		SpotLightComponent(Entity* parent, unsigned int lightFlags = 0);
		virtual ~SpotLightComponent();

		void Update(double dt) override;
		void OnInitScene() override;
		void OnUnInitScene() override;

		void SetCutOff(float degrees);

		// Set functions which modifies the shadowCamera
		void SetPosition(float3 position);
		void SetDirection(float3 direction);
		void SetOuterCutOff(float degrees);

		void* GetLightData() const override;
	
	protected:
		void UpdateLightColor() override;
		void UpdateLightIntensity() override;

	private:
		SpotLight* m_pSpotLight = nullptr;

		void initFlagUsages();
	};
}

#endif