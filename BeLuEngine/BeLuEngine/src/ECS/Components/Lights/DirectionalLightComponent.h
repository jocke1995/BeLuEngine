#ifndef DIRECTIONALLIGHTCOMPONENT_H
#define DIRECTIONALLIGHTCOMPONENT_H

#include "Light.h"
#include "../Component.h"

namespace component
{
	class DirectionalLightComponent :  public Light, public Component
	{
	public:
		DirectionalLightComponent(Entity* parent, unsigned int lightFlags = 0);
		virtual ~DirectionalLightComponent();

		void Update(double dt) override;
		void OnInitScene() override;
		void OnUnInitScene() override;

		// Set functions which modifies the shadowCamera
		void SetDirection(float3 direction);

		void* GetLightData() const override;
	
	protected:
		void UpdateLightColor() override;

	private:
		float m_Distance = 30.0f;
		DirectionalLight* m_pDirectionalLight = nullptr;

		void initFlagUsages();
	};
}
#endif
