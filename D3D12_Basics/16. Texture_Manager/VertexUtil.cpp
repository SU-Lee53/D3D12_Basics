#include "pch.h"
#include "VertexUtil.h"

DWORD AddVertex(std::vector<BasicVertex>& VertexList, DWORD dwMaxVertexCount, DWORD& refdwInOutVertexCount, const BasicVertex& pVertex)
{
	DWORD dwFoundIndex = -1;
	DWORD dwExistVertexCount = refdwInOutVertexCount;
	for (DWORD i = 0; i < dwExistVertexCount; i++)
	{
		const BasicVertex* pExistVertex = &VertexList[i];
		if (!memcmp(pExistVertex, &pVertex, sizeof(BasicVertex)))
		{
			dwFoundIndex = i;
			return dwFoundIndex;
		}
	}
	if (dwExistVertexCount + 1 > dwMaxVertexCount)
	{
		__debugbreak();
		return dwFoundIndex;
	}

	dwFoundIndex = dwExistVertexCount;
	VertexList[dwFoundIndex] = pVertex;
	refdwInOutVertexCount = dwExistVertexCount + 1;

	return dwFoundIndex;
}

DWORD VertexUtil::CreateBoxMesh(std::vector<BasicVertex>& prefOutVertexList, WORD* refOutIndexList, DWORD dwMaxBufferCount, float fHalfBoxLen)
{
	const DWORD INDEX_COUNT = 36;
	if (dwMaxBufferCount < INDEX_COUNT)
	{
		__debugbreak();
		return -1;
	}

	const WORD pIndexList[INDEX_COUNT] =
	{
		// +z
		3, 0, 1,
		3, 1, 2,

		// -z
		4, 7, 6,
		4, 6, 5,

		// -x
		0, 4, 5,
		0, 5, 1,

		// +x
		7, 3, 2,
		7, 2, 6,

		// +y
		0, 3, 7,
		0, 7, 4,

		// -y
		2, 1, 5,
		2, 5, 6
	};

	TVERTEX pTexCoordList[INDEX_COUNT] =
	{
		// +z
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

		// -z
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

		// -x
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

		// +x
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

		// +y
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

		// -y
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
	};

	XMFLOAT3 pWorldPosList[8];
	pWorldPosList[0] = { -fHalfBoxLen, fHalfBoxLen, fHalfBoxLen };
	pWorldPosList[1] = { -fHalfBoxLen, -fHalfBoxLen, fHalfBoxLen };
	pWorldPosList[2] = { fHalfBoxLen, -fHalfBoxLen, fHalfBoxLen };
	pWorldPosList[3] = { fHalfBoxLen, fHalfBoxLen, fHalfBoxLen };
	pWorldPosList[4] = { -fHalfBoxLen, fHalfBoxLen, -fHalfBoxLen };
	pWorldPosList[5] = { -fHalfBoxLen, -fHalfBoxLen, -fHalfBoxLen };
	pWorldPosList[6] = { fHalfBoxLen, -fHalfBoxLen, -fHalfBoxLen };
	pWorldPosList[7] = { fHalfBoxLen, fHalfBoxLen, -fHalfBoxLen };

	const DWORD MAX_WORKING_VERTEX_COUNT = 65536;
	std::vector<BasicVertex> WorkingVertexList(MAX_WORKING_VERTEX_COUNT, {{0.f,0.f,0.f}, {0.f,0.f,0.f,0.f}, {0.f,0.f}});
	DWORD dwBasicVertexCount = 0;

	for (DWORD i = 0; i < INDEX_COUNT; i++)
	{
		BasicVertex v;
		v.color = { 1.f, 1.f, 1.f, 1.f };
		v.position = { pWorldPosList[pIndexList[i]].x, pWorldPosList[pIndexList[i]].y, pWorldPosList[pIndexList[i]].z };
		v.texCoord = { pTexCoordList[i].u, pTexCoordList[i].v };

		refOutIndexList[i] = AddVertex(WorkingVertexList, MAX_WORKING_VERTEX_COUNT, dwBasicVertexCount, v);
	}
	std::vector<BasicVertex> pNewVertexList(WorkingVertexList);

	prefOutVertexList = pNewVertexList;

	return dwBasicVertexCount;
}

void VertexUtil::DeleteBoxMesh(std::shared_ptr<BasicVertex> pVertexList)
{
	pVertexList.reset();
}
