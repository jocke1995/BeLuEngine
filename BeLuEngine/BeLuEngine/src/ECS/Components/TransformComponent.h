#ifndef TRANSFORMCOMPONENT_H
#define TRANSFORMCOMPONENT_H

#include "Component.h"
class Transform;

namespace component
{
    class TransformComponent : public Component
    {
    public:
        TransformComponent(Entity* parent, bool invertDirection = false);
        virtual ~TransformComponent();

        void Update(double dt) override;
        void OnInitScene() override;
        void OnUnInitScene() override;
        // Resets the transform to its original state
        void Reset() override;

        void SetTransformOriginalState();

        Transform* GetTransform() const;
    private:
        Transform* m_pTransform = nullptr;
        Transform* m_pOriginalTransform = nullptr;
    };
}

#endif

