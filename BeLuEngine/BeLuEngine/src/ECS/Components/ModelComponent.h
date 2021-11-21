#ifndef MODELCOMPONENT_H
#define MODELCOMPONENT_H

#include "Core.h"
#include "GPU_Structs.h"
#include "Component.h"

class Mesh;
class Model;
class Material;
struct SlotInfo;

class ShaderResource;

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
        Material* GetMaterialAt(unsigned int index) const;
        void SetMaterialAt(unsigned int index, Material* material);
        ShaderResource* GetMaterialByteAdressBufferDXR() const;

        // SlotInfo
        const SlotInfo* GetSlotInfoAt(unsigned int index) const;
        ShaderResource* GetSlotInfoByteAdressBufferDXR() const;

    private:
        // The boundingBox will update the "m_IsPickedThisFrame"
        friend class BoundingBoxComponent;
        friend class BeLuEngine;
        friend class Renderer;
        bool m_IsPickedThisFrame = false;

        Model* m_pModel = nullptr;

        std::vector<Material*> m_Materials;
        std::vector<SlotInfo> m_SlotInfos;
        std::vector<MaterialData> m_MaterialDataRawBuffer;
        void updateSlotInfo();
        void updateMaterialDataBuffer();

        unsigned int m_DrawFlag = 0;

        // DXR
        ShaderResource* m_SlotInfoByteAdressBuffer;
        ShaderResource* m_MaterialByteAdressBuffer;
    };
}
#endif
