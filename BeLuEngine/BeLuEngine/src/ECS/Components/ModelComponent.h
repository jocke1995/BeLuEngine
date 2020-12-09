#ifndef MODELCOMPONENT_H
#define MODELCOMPONENT_H

#include "Core.h"
#include "GPU_Structs.h"
#include "Component.h"

class Mesh;
class Model;
class Material;

namespace component
{
    class ModelComponent : public Component
    {
    public:
        ModelComponent(Entity* parent);
        virtual ~ModelComponent();

        void Update(double dt) override;
        void OnInitScene() override;
        void OnUnInitScene() override;

        // Sets
        void SetModel(Model* model);
        void SetDrawFlag(unsigned int drawFlag);

        // Gets
        Mesh* GetMeshAt(unsigned int index) const;
        Material* GetMaterialAt(unsigned int index) const;
        const SlotInfo* GetSlotInfoAt(unsigned int index) const;
        unsigned int GetDrawFlag() const;
        unsigned int GetNrOfMeshes() const;
        const std::wstring& GetModelPath() const;
        bool IsPickedThisFrame() const;
        Model* GetModel() const;

    private:
        // The boundingBox will update the "m_IsPickedThisFrame"
        friend class BoundingBoxComponent;
        friend class BeLuEngine;
        friend class Renderer;
        bool m_IsPickedThisFrame = false;

        Model* m_pModel = nullptr;
        unsigned int m_DrawFlag = 0;
    };
}
#endif
