#include "ShipController.h"
#include "InputManager.h"
#include "Transform.h"
#include "CompositeObject.h"
#include "TimeManager.h"
#include "GraphicsController.h"
#include "TrackLayout.h"
#include <iostream>
#include <tuple>
#include "MeshLoader.h"

void ShipController::UpdateBase()
{
	Transform* transform = obj->GetComponent<Transform>();
	DirectX::XMVECTOR pos = transform->GetPosition();

	DirectX::XMVECTOR norm, basePos;
	std::tie(norm, basePos, overTri) = TrackLayout::GetNormal(pos);

	base.GetComponent<Transform>()->SetPosition(basePos);
	DirectX::XMStoreFloat3(&curNorm, norm);
}

void ShipController::Update()
{
	UpdateBase();

	DirectX::XMVECTOR facingDir = DirectX::XMLoadFloat3(&facingDirection);
	DirectX::XMVECTOR norm = DirectX::XMLoadFloat3(&curNorm);

	//Project facing direction onto plane of new track segment
	facingDir = DirectX::XMVector3Normalize(
		DirectX::XMVectorSubtract(
			facingDir,
			DirectX::XMVectorMultiply(
				DirectX::XMVector3Dot(
					norm,
					facingDir),
				norm)));

	Transform* transform = obj->GetComponent<Transform>();

	DirectX::XMVECTOR shipPos = transform->GetPosition();
	float height = 0.0f;
	float dist = Time::TimeManager::DeltaT() * 40.0;
	float dpos = 0.0f;
	if (Input::InputManager::KeyIsPressed(Input::KeyW))
	{
		dpos += dist;
	}
	if (Input::InputManager::KeyIsPressed(Input::KeyS))
	{
		dpos -= dist;
	}
	if (Input::InputManager::KeyIsPressed(Input::KeyQ))
	{
		height += dist * 0.4f;
	}
	if (Input::InputManager::KeyIsPressed(Input::KeyE))
	{
		height -= dist * 0.4f;
	}

	float drot = 0.0f;
	if (Input::InputManager::KeyIsPressed(Input::KeyD))
	{
		drot += dist * 0.03;
		curRoll -= dist * rollSpeed;
		if (curRoll < -rollMax)
			curRoll = -rollMax;
	}
	else if (Input::InputManager::KeyIsPressed(Input::KeyA))
	{
		drot -= dist * 0.03;
		curRoll += dist * rollSpeed;
		if (curRoll > rollMax)
			curRoll = rollMax;
	}
	else if(curRoll < 0.0f)
	{
		curRoll += dist * rollSpeed;
		if (curRoll > 0.0f)
			curRoll = 0.0f;
	}
	else if (curRoll > 0.0f)
	{
		curRoll -= dist * rollSpeed;
		if (curRoll < 0.0f)
			curRoll = 0.0f;
	}

	//Apply new velocity vector
	DirectX::XMVECTOR v = 
		DirectX::XMVectorAdd(
			DirectX::XMLoadFloat3(&velocity),
			DirectX::XMVectorScale(transform->GetForwards(), dpos * acceleration)
		);

	//Calculate track repulsion force
	DirectX::XMFLOAT3 basepos;
	DirectX::XMStoreFloat3(&basepos, base.GetComponent<Transform>()->GetPosition());

	if (overTri)
	{
		float vDist = DirectX::XMVectorGetX(
			DirectX::XMVector3Dot(
				DirectX::XMVectorSubtract(
					shipPos,
					base.GetComponent<Transform>()->GetPosition()),
				norm
			)) - 10.0f;
		
		if (vDist > Time::TimeManager::DeltaT() * vertSpeed)
			vDist = Time::TimeManager::DeltaT() * vertSpeed;
		else if (vDist < -Time::TimeManager::DeltaT() * vertSpeed)
			vDist = -Time::TimeManager::DeltaT() * vertSpeed;

		//Add on repulsion force
		shipPos = DirectX::XMVectorAdd(DirectX::XMVectorScale(norm, -vDist), shipPos);
	}

	shipPos = DirectX::XMVectorAdd(
		shipPos,
		DirectX::XMVectorScale(v, Time::TimeManager::DeltaT())
	);
	transform->SetPosition(shipPos);

	//Add drag
	DirectX::XMVECTOR dragV =
		DirectX::XMVectorScale(
			DirectX::XMVectorPow(
				v,
				{ 2.0f, 2.0f, 2.0f }),
			drag * Time::TimeManager::DeltaT()
		);

	DirectX::XMFLOAT3 manualD, manualV;
	DirectX::XMStoreFloat3(&manualD, dragV);
	DirectX::XMStoreFloat3(&manualV, v);

	if(manualV.x < 0.0f)
	{
		manualV.x += std::abs(manualD.x);
		if (manualV.x > 0.0f)
			manualV.x = 0.0f;
	}
	else if(manualV.x > 0.0f)
	{
		manualV.x -= std::abs(manualD.x);
		if (manualV.x < 0.0f)
			manualV.x = 0.0f;
	}
	if (manualV.y < 0.0f)
	{
		manualV.y += std::abs(manualD.y);
		if (manualV.y > 0.0f)
			manualV.y = 0.0f;
	}
	else if (manualV.y > 0.0f)
	{
		manualV.y -= std::abs(manualD.y);
		if (manualV.y < 0.0f)
			manualV.y = 0.0f;
	}
	if (manualV.z < 0.0f)
	{
		manualV.z += std::abs(manualD.z);
		if (manualV.z > 0.0f)
			manualV.z = 0.0f;
	}
	else if (manualV.z > 0.0f)
	{
		manualV.z -= std::abs(manualD.z);
		if (manualV.z < 0.0f)
			manualV.z = 0.0f;
	}

	v = DirectX::XMLoadFloat3(&manualV);
	DirectX::XMStoreFloat3(&velocity, v);

	//Rotate around normal
	facingDir = DirectX::XMVector3TransformNormal(
		facingDir,
		DirectX::XMMatrixRotationQuaternion(
			DirectX::XMQuaternionRotationAxis(
				norm,
				drot)));

	DirectX::XMStoreFloat3(&facingDirection, facingDir);

	//Get current normal
	DirectX::XMVECTOR currentNormal = transform->GetUp();

	//Interpolate to find desired normal for this frame
	float angle = DirectX::XMVectorGetX(DirectX::XMVector3AngleBetweenNormals(norm, currentNormal));
	float interpolate = alignSpeed * Time::TimeManager::DeltaT() / angle;
	if (interpolate > 1.0f)
		interpolate = 1.0f;
	currentNormal = DirectX::XMVectorLerp(currentNormal, norm, interpolate);

	//Project facing direction onto plane of current normal
	facingDir = DirectX::XMVector3Normalize(
		DirectX::XMVectorSubtract(
			facingDir,
			DirectX::XMVectorMultiply(
				DirectX::XMVector3Dot(
					currentNormal,
					facingDir),
				currentNormal)));

	//Compute quaternion to map {0,1,0} to current normal
	DirectX::XMVECTOR qa = DirectX::XMVector3Normalize(DirectX::XMVector3Cross({ 0.0f, 1.0f, 0.0f }, currentNormal));
	DirectX::XMVECTOR dot = DirectX::XMVector3Dot(
		DirectX::XMVector3Normalize(currentNormal),
		{ 0.0f, 1.0f, 0.0f }
	);
	float x = DirectX::XMVectorGetX(dot);
	if (x > 1.0f)
		x = 1.0f;
	else if (x < -1.0f)
		x = -1.0f;
	float a = std::acos(x) / 2;
	qa = DirectX::XMVectorMultiply(
		DirectX::XMVectorSetW(qa, 1.0f),
		{ std::sin(a), std::sin(a), std::sin(a), std::cos(a) }
	);

	//Compute quaternion to map {0,0,1} to forwards direction
	DirectX::XMVECTOR va = DirectX::XMVector3Rotate(
		{ 0.0f, 0.0f, 1.0f },
		qa
	);
	DirectX::XMVECTOR qb = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(va, facingDir));
	dot = DirectX::XMVector3Dot(
		DirectX::XMVector3Normalize(facingDir),
		va
	);
	x = DirectX::XMVectorGetX(dot);
	if (x > 1.0f)
		x = 1.0f;
	else if (x < -1.0f)
		x = -1.0f;
	a = std::acos(x) / 2;
	qb = DirectX::XMVectorMultiply(
		DirectX::XMVectorSetW(qb, 1.0f),
		{ std::sin(a), std::sin(a), std::sin(a), std::cos(a) }
	);

	DirectX::XMVECTOR rot = DirectX::XMQuaternionMultiply(qa, qb);
	transform->SetRotation(rot);
	
	DirectX::XMVECTOR camPos =
		DirectX::XMVector3TransformCoord(
	{ 0.0f, 3.0f, -20.0f },
			transform->GetTransform());
	cam.GetComponent<Transform>()->SetPosition(camPos);
	cam.GetComponent<Transform>()->SetRotation(transform->GetRotation());

	model.GetComponent<Transform>()->SetRotation(DirectX::XMQuaternionRotationAxis({0.0f, 0.0f, 1.0f}, curRoll));

	//shipRot = DirectX::XMVectorSetByIndex(shipRot, roll, 2);
	//transform->SetRotation(shipRot);
}

void ShipController::Create()
{
	//Init camera
	cam.AttachComponent<Camera>();
	cam.GetComponent<Transform>()->SetScale({ 1.0f,1.0f,1.0f });
	GraphicsController::instance->SetCamera(cam.GetComponent<Camera>());

	//Init model child object
	MeshData* mesh = new MeshData;
	FbxNode* fbxNode = MeshLoader::LoadFBX("test.fbx");
	MeshLoader::ApplyFBX(mesh, fbxNode, "");

	Transform* t = model.GetComponent<Transform>();
	t->SetPosition({ 0.0f, 0.0f, 0.0f });
	t->SetRotation(DirectX::XMQuaternionIdentity());
	t->SetScale({ 1.0f, 1.0f, 1.0f });
	t->SetParent(obj->GetComponent<Transform>());

	D3D11_INPUT_ELEMENT_DESC iLayout[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, tex), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	Material* mat = new Material;
	mat->passes.push_back(RenderPass());
	mat->passes[0].LoadVS(L"shaders.hlsl", "VShader", iLayout, 3);
	mat->passes[0].LoadPS(L"shaders.hlsl", "PShaderTex");
	mat->LoadTGA("test.tga");

	Renderer* r = model.AttachComponent<Renderer>();
	MaterialGroup m;
	m.AddMaterial(mat);
	r->Init(m, mesh);
	GraphicsController::instance->AddRenderer(r);

	facingDirection = { 0.0f, 0.0f, 1.0f };
	velocity = { 0.0f, 0.0f, 0.0f };
}

ShipController::ShipController()
{
}

ShipController::~ShipController()
{
}
