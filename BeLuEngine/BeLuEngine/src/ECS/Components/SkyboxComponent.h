#ifndef SKYBOXMCOMPONENT_H
#define SKYBOXMCOMPONENT_H

#include "Component.h"
class IGraphicsTexture;

namespace component
{
    class SkyboxComponent : public Component
    {
    public:
        SkyboxComponent(Entity* parent);
        virtual ~SkyboxComponent();

        void Update(double dt) override;
        void OnInitScene() override;
        void OnUnInitScene() override;

        void Reset() override;

        void SetSkyboxTexture(IGraphicsTexture* texture);
        IGraphicsTexture* GetSkyboxTexture() const;
    private:
        IGraphicsTexture* m_pSkyBoxTexture;
    };
}

#endif

