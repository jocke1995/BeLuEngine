#ifndef SKYBOXMCOMPONENT_H
#define SKYBOXMCOMPONENT_H

#include "Component.h"
class IGraphicsTexture;
class Model;

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
        friend class Renderer;
        friend class SkyboxPass;
        Model* m_pCube = nullptr;

        IGraphicsTexture* m_pSkyBoxTexture = nullptr;
    };
}

#endif

