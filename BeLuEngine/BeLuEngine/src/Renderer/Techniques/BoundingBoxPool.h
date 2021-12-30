#ifndef BOUNDINGBOXPOOL_H
#define BOUNDINGBOXPOOL_H

class Mesh;
struct Vertex;

struct BoundingBoxData
{
	std::vector<Vertex> boundingBoxVertices;
	std::vector<unsigned int> boundingBoxIndices;
};

class BoundingBoxPool
{
public:
	~BoundingBoxPool();
	static BoundingBoxPool* Get();

	bool BoundingBoxDataExists(std::wstring uniquePath) const;
	bool BoundingBoxMeshExists(std::wstring uniquePath) const;

	BoundingBoxData* GetBoundingBoxData(std::wstring uniquePath);
	BoundingBoxData* CreateBoundingBoxData(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::wstring uniquePath);

	std::pair<Mesh*, bool> CreateBoundingBoxMesh(std::wstring uniquePath);
	
private:
	BoundingBoxPool();
	BoundingBoxPool(BoundingBoxPool const&) = delete;
	void operator=(BoundingBoxPool const&) = delete;

	std::map<std::wstring, BoundingBoxData*> m_BoundingBoxesData;
	std::map<std::wstring, Mesh*> m_BoundingBoxesMesh;
};

#endif
