#ifndef BOUNDINGBOXCOMPONENT_H
#define BOUNDINGBOXCOMPONENT_H

#include "Component.h"

struct BoundingBoxData;
class Mesh;
class Transform;
namespace component
{
	class BoundingBoxComponent : public Component
	{
	public:
		BoundingBoxComponent(Entity* parent, bool pick = false);
		virtual ~BoundingBoxComponent();

		void Init();
		void Update(double dt);

		void SetMesh(Mesh* mesh);

		Transform* GetTransform() const;
		const Mesh* GetMesh() const;
		const BoundingBoxData* GetBoundingBoxData() const;
		const std::string GetPathOfModel() const;

		bool CanBePicked() const;

		// Renderer calls this function when an entity is picked
		bool& IsPickedThisFrame();

	private:
		std::string m_pPathOfModel = "";
		BoundingBoxData* m_pBbd = nullptr;
		bool createBoundingBox();
		Mesh* m_pMesh = nullptr;

		bool m_CanBePicked = false;

		Transform* m_pTransform = nullptr;
	};
}

#endif
