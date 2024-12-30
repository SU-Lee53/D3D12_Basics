#include "pch.h"
#include "GameObject.h"
#include "D3D12Renderer.h"
#include "Game.h"

using namespace std;

GameObject::GameObject()
{
	m_matScale = XMMatrixIdentity();
	m_matRot = XMMatrixIdentity();
	m_matTrans = XMMatrixIdentity();
	m_matWorld = XMMatrixIdentity();
}

GameObject::~GameObject()
{
}

BOOL GameObject::Initialize(std::shared_ptr<Game> pGame)
{
	BOOL bResult = FALSE;

	m_pGame = pGame;
	m_pRenderer = pGame->GetRenderer();

	m_pMeshObj = CreateBoxMeshObject();
	if (m_pMeshObj)
	{
		bResult = TRUE;
	}

	return bResult;
}

void GameObject::Run()
{
	if (m_bUpdateTransform)
	{
		UpdateTransform();
		m_bUpdateTransform = FALSE;
	}
}

void GameObject::Render()
{
	if (m_pMeshObj)
	{
		m_pRenderer->RenderMeshObject(m_pMeshObj, m_matWorld);
	}
}

void GameObject::SetPosition(float x, float y, float z)
{
	m_Pos.m128_f32[0] = x;
	m_Pos.m128_f32[1] = y;
	m_Pos.m128_f32[2] = z;

	m_matTrans = XMMatrixTranslation(x, y, z);

	m_bUpdateTransform = TRUE;
}

void GameObject::SetScale(float x, float y, float z)
{
	m_Pos.m128_f32[0] = x;
	m_Pos.m128_f32[1] = y;
	m_Pos.m128_f32[2] = z;

	m_matTrans = XMMatrixScaling(x, y, z);

	m_bUpdateTransform = TRUE;
}

void GameObject::SetRotationY(float fRotY)
{
	m_fRotY = fRotY;
	m_matRot = XMMatrixRotationY(fRotY);

	m_bUpdateTransform = TRUE;
}

std::shared_ptr<void> GameObject::CreateBoxMeshObject()
{
	std::shared_ptr<void> pMeshObj = nullptr;

	// Box Mesh »ý¼º
	WORD pIndexList[36];
	std::vector<BasicVertex> VertexList = {};
	DWORD dwVertexCount = VertexUtil::CreateBoxMesh(VertexList, pIndexList, (DWORD)_countof(pIndexList), 0.25f);

	pMeshObj = m_pRenderer->CreateBasicMeshObject();

	const WCHAR* wchTexFileNameList[6] =
	{
		L"tex_00.dds",
		L"tex_01.dds",
		L"tex_02.dds",
		L"tex_03.dds",
		L"tex_04.dds",
		L"tex_05.dds"
	};

	m_pRenderer->BeginCreateMesh(pMeshObj, VertexList.data(), dwVertexCount, 6);
	for (DWORD i = 0; i < 6; i++)
	{
		m_pRenderer->InsertTriGroup(pMeshObj, pIndexList + i * 6, 2, wchTexFileNameList[i]);
	}
	m_pRenderer->EndCreateMesh(pMeshObj);

	return pMeshObj;
}

std::shared_ptr<void> GameObject::CreateQuadMesh()
{
	std::shared_ptr<void> pMeshObj = nullptr;
	pMeshObj = m_pRenderer->CreateBasicMeshObject();

	BasicVertex pVertexList[] =
	{
		{ { -0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
	};

	WORD pIndexList[] =
	{
		0, 1, 2,
		0, 2, 3
	};


	m_pRenderer->BeginCreateMesh(pMeshObj, pVertexList, (DWORD)_countof(pVertexList), 1);
	m_pRenderer->InsertTriGroup(pMeshObj, pIndexList, 2, L"tex_06.dds");
	m_pRenderer->EndCreateMesh(pMeshObj);
	return pMeshObj;
}

void GameObject::UpdateTransform()
{
	// World = S * R * T
	m_matWorld = XMMatrixMultiply(m_matScale, m_matRot);
	m_matWorld = XMMatrixMultiply(m_matWorld, m_matTrans);
}

void GameObject::CleanUp()
{
	if (m_pMeshObj)
	{
		m_pRenderer->DeleteBasicMeshObject(m_pMeshObj);
		m_pMeshObj = nullptr;
	}
}
