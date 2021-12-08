#ifndef MODELCOMPONENT_H
#define MODELCOMPONENT_H

#include "Core.h"
#include "GPU_Structs.h"
#include "Component.h"

class Mesh;
class Model;
class Material;
struct SlotInfo;

class IGraphicsBuffer;

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

        // Model Stuff
        Mesh* GetMeshAt(unsigned int index) const;
        unsigned int GetNrOfMeshes() const;
        unsigned int GetDrawFlag() const;
        const std::wstring& GetModelPath() const;
        bool IsPickedThisFrame() const;
        Model* GetModel() const;

        // Material
        void SetMaterialAt(unsigned int index, Material* material);
        Material* GetMaterialAt(unsigned int index);
        MaterialData* GetUniqueMaterialDataAt(unsigned int index);
        IGraphicsBuffer* GetMaterialByteAdressBuffer() const;

        const SlotInfo* GetSlotInfoAt(unsigned int index) const;
        IGraphicsBuffer* GetSlotInfoByteAdressBufferDXR() const;

        void UpdateMaterialRawBufferFromMaterial();
    private:
        // The boundingBox will update the "m_IsPickedThisFrame"
        friend class BoundingBoxComponent;
        friend class BeLuEngine;
        friend class Renderer;

        bool m_IsPickedThisFrame = false;
        unsigned int m_DrawFlag = 0;

        Model* m_pModel = nullptr;

        std::vector<Material*> m_Materials;
        std::vector<MaterialData> m_MaterialDataRawBuffer;

        std::vector<SlotInfo> m_SlotInfos;
        void updateSlotInfoBuffer();

        IGraphicsBuffer* m_SlotInfoByteAdressBuffer;
        IGraphicsBuffer* m_MaterialByteAdressBuffer;
    };
}
#endif
