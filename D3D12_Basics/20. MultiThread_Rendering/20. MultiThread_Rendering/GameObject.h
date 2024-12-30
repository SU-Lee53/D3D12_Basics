#pragma once

class Game;
class D3D12Renderer;

class GameObject
{
public:
	GameObject();
	~GameObject();

	BOOL Initialize(std::shared_ptr<Game> pGame);
	void Run();
	void Render();
	
	void SetPosition(float x, float y, float z);
	void SetScale(float x, float y, float z);
	void SetRotationY(float fRotY);

private:
	std::shared_ptr<void> CreateBoxMeshObject();
	std::shared_ptr<void> CreateQuadMesh();

	void UpdateTransform();
	void CleanUp();

private:
	std::weak_ptr<Game> m_pGame;
	std::shared_ptr<D3D12Renderer> m_pRenderer;
	std::shared_ptr<void> m_pMeshObj = nullptr;

	XMVECTOR m_Scale = { 1.f, 1.f, 1.f, 0.f };
	XMVECTOR m_Pos = {};
	float m_fRotY = 0.0f;

	XMMATRIX m_matScale = {};
	XMMATRIX m_matRot = {};
	XMMATRIX m_matTrans = {};
	XMMATRIX m_matWorld = {};
	BOOL m_bUpdateTransform = FALSE;
};


