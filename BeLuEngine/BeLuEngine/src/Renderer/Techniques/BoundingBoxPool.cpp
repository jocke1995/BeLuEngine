#include "stdafx.h"
#include "BoundingBoxPool.h"

#include "../Geometry/Mesh.h"

BoundingBoxPool::BoundingBoxPool()
{
}

BoundingBoxPool::~BoundingBoxPool()
{
	for (auto& pair : m_BoundingBoxesMesh)
	{
		delete pair.second;
	}

	for (auto& pair : m_BoundingBoxesData)
	{
		delete pair.second;
	}
}

BoundingBoxPool* BoundingBoxPool::Get()
{
	static BoundingBoxPool instance;

	return &instance;
}

bool BoundingBoxPool::BoundingBoxDataExists(std::wstring uniquePath) const
{
	if (m_BoundingBoxesData.count(uniquePath) != 0)
	{
		return true;
	}
	return false;
}

bool BoundingBoxPool::BoundingBoxMeshExists(std::wstring uniquePath) const
{
	if (m_BoundingBoxesMesh.count(uniquePath) != 0)
	{
		return true;
	}
	return false;
}

BoundingBoxData* BoundingBoxPool::GetBoundingBoxData(std::wstring uniquePath)
{
	if (BoundingBoxDataExists(uniquePath) == true)
	{
		return m_BoundingBoxesData.at(uniquePath);
	}
	return nullptr;
}

BoundingBoxData* BoundingBoxPool::CreateBoundingBoxData(
	std::vector<Vertex> vertices,
	std::vector<unsigned int> indices,
	std::wstring uniquePath)
{
	if (BoundingBoxDataExists(uniquePath) == false)
	{
		BoundingBoxData* bbd = new BoundingBoxData();
		bbd->boundingBoxVertices = vertices;
		bbd->boundingBoxIndices = indices;
		m_BoundingBoxesData[uniquePath] = bbd;
	}
	return m_BoundingBoxesData[uniquePath];
}

std::pair<Mesh*, bool> BoundingBoxPool::CreateBoundingBoxMesh(std::wstring uniquePath)
{
	// If it already exists.. return it
	if (BoundingBoxMeshExists(uniquePath) == true)
	{
		// if this happens, do not submit data to CODT
		return std::make_pair(m_BoundingBoxesMesh.at(uniquePath), false);
	}

	// else create it and return it if the data exists
	if (BoundingBoxDataExists(uniquePath) == true)
	{
		BoundingBoxData* bbd = m_BoundingBoxesData[uniquePath];
		m_BoundingBoxesMesh[uniquePath] = new Mesh(&bbd->boundingBoxVertices, &bbd->boundingBoxIndices, uniquePath);
		m_BoundingBoxesMesh[uniquePath]->Init();
		return std::make_pair(m_BoundingBoxesMesh.at(uniquePath), true);
	}

	// else return nullptr
	return std::make_pair(nullptr, false);
}
