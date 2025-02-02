﻿#include "Gameplay.h"

#define USE_NETWORKMODE
#ifdef USE_NETWORKMODE
#include <shellapi.h>
#include "Networking.h"
#endif

#include "Controllers.h"
#include "ColliderCallback.h"
#include "LazyCamera.h"
#include "Engine/DebugShape/DebugShape.h"
#include "Engine/CollisionDetection/ColliderBase.h"
#include "Engine/General/BasicShapes.h"
#include "Engine/Common/Interfaces.h"
#include "Engine/Core/Entirety/World.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/Graphics.h"
#include "Engine/Graphics/Common/MeshRender.h"
#include "Engine/Graphics/Common/Material.h"
#include "Engine/Graphics/Common/ImageRender.h"
#include "Engine/Graphics/Common/CanvasRender.h"
#include "Engine/Graphics/Common/Image.h"
#include "Engine/Graphics/Common/Camera.h"
#include "Engine/Graphics/Common/Screen.h"
#include "Engine/Graphics/Common/BinaryFileLoader.h"
#include "Engine/Graphics/Common/DebugMesh.h"
#include "Engine/Graphics/Common/Text.h"
#include "Engine/Graphics/Common/RectTransform.h"
#include "Engine/Graphics/Common/GUI.h"
#include "Engine/Graphics/Common/BasicMeshes.h"
#include "Engine/CollisionDetection/RigidBody.h"
#include "Engine/CollisionDetection/MeshCollider.h"
#include "Engine/SpatialPartition/Octree.h"

namespace 
{
	// variables.
	float _windowWidth = 1200.0f;
	float _windowHeight = 800.0f;
	float _resetCDRemain = 0.0f;
	// function declears
	bool InitLevel();
	bool ResetLevel();

  void CreateFlag(EAE_Engine::Math::Vector3 flagPos, const char* pName, EAE_Engine::Math::Vector4 color);
	void CreatePlayer(EAE_Engine::Math::Vector3 playerPos);
	void CreateCamera();
	void CreateSprite();
	void CreateDebugMenu();
}

const char* const pathGround = "data/Meshes/warehouse.aosmesh";
const char* const pathPlayer = "data/Meshes/sphere.aosmesh";
const char* const pathFlag = "data/Meshes/flag.aosmesh";

EAE_Engine::Physics::RigidBody* g_pRigidBody = nullptr;
EAE_Engine::Common::IGameObj* g_pPlayerObj = nullptr;
EAE_Engine::Controller::Controller* g_pPlayerController = nullptr;

EAE_Engine::Common::IGameObj* pCameraObj = nullptr;
EAE_Engine::Common::ICamera* pCamera = nullptr;
EAE_Engine::Controller::Controller* pCamController = nullptr;
EAE_Engine::Controller::Controller* pFlyCamController = nullptr;


EAE_Engine::Graphics::ImageRender* pNumberSpriteRender = nullptr;

EAE_Engine::Graphics::Text* pFrameText = nullptr;
EAE_Engine::Graphics::Button* pControlBtn = nullptr;
EAE_Engine::Graphics::Slider* pSlider = nullptr;
EAE_Engine::Graphics::Toggle* pToggle = nullptr;
EAE_Engine::Graphics::Toggle* pDrawSegmentToggle = nullptr;

bool g_drawDebugMenu = true;

void btnCallBack(void*) 
{
	if (pSlider)
		pSlider->_handleValue = 20.0f;
}

EAE_Engine::Core::CompleteOctree* g_pCompleteOctree = nullptr;

///////////////////////////////////////////////////////////////////////////////

bool GameplayInit(float windowWidth, float windowHeight)
{
#ifdef USE_NETWORKMODE
  LPWSTR *szArglist;
  int nArgs;
  szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
  if (nArgs < 3)
    return false;
  bool isServe = false;
  if (szArglist[1][0] == 's' || szArglist[1][0] == 'S')
  {
    isServe = true;
  }
  wchar_t* wchars = szArglist[2];

  //convert from wide char to narrow char array
  char ch[260];
  char DefChar = ' ';
  WideCharToMultiByte(CP_ACP, 0, wchars, -1, ch, 260, &DefChar, NULL);
  //A std:string  using the char* constructor.
  std::string ipaddress(ch);
  NetworkPeer::GetInstance()->Init(isServe, ipaddress);
#endif  


  bool result = true;
	result = EAE_Engine::Graphics::LoadMeshFromBinary(pathGround);
	if (!result)
	{
		return false;
	}
	result = EAE_Engine::Graphics::LoadMeshFromBinary(pathPlayer);
	if (!result)
	{
		return false;
	}
  result = EAE_Engine::Graphics::LoadMeshFromBinary(pathFlag);
  if (!result)
  {
    return false;
  }
	EAE_Engine::Graphics::LoadMaterial("data/Materials/white.material");
	EAE_Engine::Graphics::LoadMaterial("data/Materials/lambert.material");
	EAE_Engine::Graphics::LoadMaterial("data/Materials/floor.material");
	EAE_Engine::Graphics::LoadMaterial("data/Materials/railing.material");
	EAE_Engine::Graphics::LoadMaterial("data/Materials/ceiling.material");
	EAE_Engine::Graphics::LoadMaterial("data/Materials/metal.material");
	EAE_Engine::Graphics::LoadMaterial("data/Materials/cement.material");
	EAE_Engine::Graphics::LoadMaterial("data/Materials/walls.material");
	EAE_Engine::Graphics::LoadMaterial("data/Materials/phongShading.material");
	_windowWidth = windowWidth;
	_windowHeight = windowHeight;
	result = InitLevel();
	
  g_pCompleteOctree = EAE_Engine::Core::OctreeManager::GetInstance()->GetOctree();
	return true;
}
 
void GameplayUpdate()
{
  //ResetLevel();
	_resetLevelController = false;
	_resetLevel = false;
	float elpasedTime = EAE_Engine::Time::GetSecondsElapsedThisFrame();
	_resetCDRemain -= elpasedTime;
	//DebugInformations
		
	EAE_Engine::Math::Vector3 yellow(1.0f, 1.0f, 0.0f);
	EAE_Engine::Math::Vector3 red(1.0f, 0.0f, 0.0f);
	EAE_Engine::Math::Vector3 green(0.0f, 1.0f, 0.0f);
	EAE_Engine::Math::Vector3 blue(0.0f, 0.0f, 1.0f);
	EAE_Engine::Debug::CleanDebugShapes();
  static bool bUseFlyCam = false;
	if (EAE_Engine::UserInput::Input::GetInstance()->GetKeyState('C') == EAE_Engine::UserInput::KeyState::OnPressed)
	{
    bUseFlyCam = !bUseFlyCam;
    if (bUseFlyCam)
    {
      g_pPlayerController->SetActive(false);
      pCamController->SetActive(false);
      pFlyCamController->SetActive(true);
    }
    else 
    {
      dynamic_cast<LazyCamera*>(pCamController)->ResetCamera(g_pPlayerObj->GetTransform());
      g_pPlayerController->SetActive(true);
      pCamController->SetActive(true);
      pFlyCamController->SetActive(false);
    }
	}
  // Enable/Disable DebugMenu
  static bool s_EnableMenuOrNot = false;
  if (EAE_Engine::UserInput::Input::GetInstance()->GetKeyState(VK_TAB) == EAE_Engine::UserInput::KeyState::OnPressed)
  {
    s_EnableMenuOrNot = !s_EnableMenuOrNot;
    EAE_Engine::Graphics::UIElementManager::GetInstance()->SetEnable(s_EnableMenuOrNot);
  }
  // Switch the Octree Info
	static uint32_t levelIndex = 3;
	static EAE_Engine::Math::Vector3 octreeColor = EAE_Engine::Math::Vector3(0.0f, 1.0f, 1.0f);
	if (EAE_Engine::UserInput::Input::GetInstance()->GetKeyState('1') == EAE_Engine::UserInput::KeyState::OnPressed)
	{
		levelIndex = 0;
		octreeColor = EAE_Engine::Math::Vector3(1.0f, 0.0f, 0.0f);
	}
	else if (EAE_Engine::UserInput::Input::GetInstance()->GetKeyState('2') == EAE_Engine::UserInput::KeyState::OnPressed)
	{
		levelIndex = 1;
		octreeColor = EAE_Engine::Math::Vector3(0.0f, 1.0f, 0.0f);
	}
	else if (EAE_Engine::UserInput::Input::GetInstance()->GetKeyState('3') == EAE_Engine::UserInput::KeyState::OnPressed)
	{
		levelIndex = 2;
		octreeColor = EAE_Engine::Math::Vector3(0.0f, 0.0f, 1.0f);
	}
	else if (EAE_Engine::UserInput::Input::GetInstance()->GetKeyState('4') == EAE_Engine::UserInput::KeyState::OnPressed)
	{
		levelIndex = 3;
		octreeColor = EAE_Engine::Math::Vector3(0.0f, 1.0f, 1.0f);
	}
	char text[20];
	float fps = 1.0f / elpasedTime;
	sprintf_s(text, "FPS:%.2f", fps);
	pFrameText->_value = text;
	static float radisu = 1.0f;
	EAE_Engine::Math::Vector3 start = g_pPlayerObj->GetTransform()->GetPos();
	start._y += 1.5f;
	EAE_Engine::Math::Vector3 end = start + g_pPlayerObj->GetTransform()->GetForward() * 150;
  {
    EAE_Engine::Debug::DebugShapes::GetInstance().AddCircle(start, 2.0f, yellow);
  }
	if (pToggle->_checked)
	{
		EAE_Engine::Debug::AddSegment(start, end, yellow);
		EAE_Engine::Math::Quaternion rotation = EAE_Engine::Math::Quaternion::Identity;.0f;
		std::vector<EAE_Engine::Core::OctreeNode*> list = g_pCompleteOctree->GetNodesCollideWithSegment(start, end, levelIndex);
		for (uint32_t index = 0; index < list.size(); ++index)
		{
			EAE_Engine::Core::OctreeNode* pNode = list[index];
			EAE_Engine::Debug::DebugShapes::GetInstance().AddBox(pNode->_extent, pNode->_pos, rotation, octreeColor);
		}
	}
	if (pDrawSegmentToggle->_checked)
	{
		EAE_Engine::Debug::AddSegment(start, end, yellow);
    std::vector<EAE_Engine::Mesh::TriangleIndex> triangles;
    EAE_Engine::Physics::Physics::GetInstance()->RayCast(start, end, triangles);
		std::vector<uint32_t> triangleIndices;
		for (uint32_t index = 0; index < triangles.size(); ++index)
		{
			triangleIndices.push_back(triangles[index]._index0);
			triangleIndices.push_back(triangles[index]._index1);
			triangleIndices.push_back(triangles[index]._index2);
		}
		EAE_Engine::Mesh::AOSMeshData* pData = EAE_Engine::Mesh::AOSMeshDataManager::GetInstance()->GetAOSMeshData("collisionData");
		EAE_Engine::Debug::DebugShapes::GetInstance().AddMesh(pData->GetVertexPoses(triangleIndices), red);
	}
#ifdef USE_NETWORKMODE
  NetworkPeer::GetInstance()->Update(g_pPlayerObj->GetTransform());
#endif
}

void GameplayExit()
{
#ifdef USE_NETWORKMODE
  NetworkPeer::Destroy();
#endif
}


void CreateOtherPlayer(const char* pname, EAE_Engine::Math::Vector3 pos, EAE_Engine::Math::Quaternion rotation)
{
  EAE_Engine::Math::Vector3 playerinitPos = pos;
  EAE_Engine::Common::IGameObj* pOtherObj = EAE_Engine::Core::World::GetInstance().AddGameObj(pname, playerinitPos);
  EAE_Engine::Math::Quaternion player_rotation = rotation;
  pOtherObj->GetTransform()->SetRotation(player_rotation);
  EAE_Engine::Math::Vector3 playerRenderPos(0.0f, 2.0f, 0.0f);
  EAE_Engine::Common::IGameObj* pOthersRenderObj = EAE_Engine::Core::World::GetInstance().AddGameObj("otherRender", playerRenderPos);
  pOthersRenderObj->GetTransform()->SetParent(pOtherObj->GetTransform());
  EAE_Engine::Graphics::AOSMeshRender* pOtherRender = EAE_Engine::Graphics::AOSMeshRenderManager::GetInstance().AddMeshRender(pathPlayer, pOthersRenderObj->GetTransform());
  /*
  {
    EAE_Engine::Graphics::SphereSOAMesh sphere(5, 5, 1.0f);
    EAE_Engine::Mesh::sSubMesh subMesh(0, (uint32_t)sphere._indices.size() - 1);
    std::vector<EAE_Engine::Mesh::sVertex> vertices;
    vertices.resize(sphere._vertices.size());
    for (uint32_t index = 0; index < (uint32_t)sphere._vertices.size(); ++index)
    {
      vertices[index].x = sphere._vertices[index]._x;
      vertices[index].y = sphere._vertices[index]._y;
      vertices[index].z = sphere._vertices[index]._z;
      vertices[index].nx = sphere._normals[index]._x;
      vertices[index].ny = sphere._normals[index]._y;
      vertices[index].nz = sphere._normals[index]._z;
      vertices[index].u = sphere._uvs[index]._x;
      vertices[index].v = sphere._uvs[index]._y;
      vertices[index].r = 1.0f;
      vertices[index].g = 0.0f;
      vertices[index].b = 0.0f;
      vertices[index].a = 1.0f;
    }
    pOtherRender->GetMesh()->ChangeWholeBuffers(&vertices[0], (uint32_t)vertices.size(),
      &sphere._indices[0], (uint32_t)sphere._indices.size(), &subMesh, 1);
  }
  */
  pOtherRender->AddMaterial("phongShading");
}


namespace
{
	bool InitLevel()
	{
		EAE_Engine::Math::Vector3 zero = EAE_Engine::Math::Vector3::Zero;
		EAE_Engine::Math::Vector3 boardPos = EAE_Engine::Math::Vector3(0.0f, 0.0f, 3.5f);

    EAE_Engine::Common::IGameObj* pActorGround = EAE_Engine::Core::World::GetInstance().AddGameObj("ground", zero);
		EAE_Engine::Math::Vector3 groundScale(1.0f, 1.0f, 1.0f);
		pActorGround->GetTransform()->SetLocalScale(groundScale);

    EAE_Engine::Graphics::AOSMeshRender* pRenderGround = EAE_Engine::Graphics::AOSMeshRenderManager::GetInstance().AddMeshRender(pathGround, pActorGround->GetTransform());
		pRenderGround->AddMaterial("lambert");
		pRenderGround->AddMaterial("floor");
		pRenderGround->AddMaterial("railing");
		pRenderGround->AddMaterial("ceiling");
		pRenderGround->AddMaterial("metal");
		pRenderGround->AddMaterial("cement");
		pRenderGround->AddMaterial("walls");
    
    {
      EAE_Engine::Math::Vector3 flagpos0(130.0f, -25.0f, -100.0f);
      EAE_Engine::Math::Vector4 red(1.0f, 0.0f, 0.0f, 1.0f);
      CreateFlag(flagpos0, "0", red);
      EAE_Engine::Math::Vector3 flagpos1(-130.0f, -25.0f, 100.0f);
      EAE_Engine::Math::Vector4 blue(0.0f, 0.0f, 1.0f, 1.0f);
      CreateFlag(flagpos1, "1", blue);
      EAE_Engine::Math::Vector3 playerinitPos = flagpos0 + EAE_Engine::Math::Vector3(0.0f, 0.0f, 0.0f);
#ifdef USE_NETWORKMODE
      if (!NetworkPeer::GetInstance()->IsServer())
      {
        playerinitPos = flagpos1 + EAE_Engine::Math::Vector3(0.0f, 0.0f, 0.0f);
      }
#endif
      CreatePlayer(playerinitPos);
    }
		CreateCamera();
		CreateSprite();
		CreateDebugMenu();

		EAE_Engine::Common::IGameObj* pCollisionDataObj = EAE_Engine::Core::World::GetInstance().AddGameObj("collisionData", zero);
		//EAE_Engine::Graphics::AOSMeshRender* pTempRender = EAE_Engine::Graphics::AddMeshRender(pathCollisionData, pCollisionDataObj->GetTransform());
		//pTempRender->AddMaterial("lambert");
		EAE_Engine::Collider::MeshCollider* pMeshCollider = new EAE_Engine::Collider::MeshCollider(pCollisionDataObj->GetTransform());
		pMeshCollider->Init("collisionData");
		EAE_Engine::Collider::ColliderManager::GetInstance()->AddToColliderList(pMeshCollider);
		return true;
	}


	bool ResetLevel()
	{
		EAE_Engine::Controller::ControllerManager::CleanInstance();
		EAE_Engine::Collider::ColliderManager::CleanInstance();
		EAE_Engine::Graphics::CleanBeforeRestart();
		EAE_Engine::Core::World::CleanInstance();

		bool result = InitLevel();
		return result;
	}


  void CreateFlag(EAE_Engine::Math::Vector3 flagPos, const char* pName, EAE_Engine::Math::Vector4 color)
  {
    EAE_Engine::Common::IGameObj* pFlagObj = EAE_Engine::Core::World::GetInstance().AddGameObj(pName, flagPos);
    pFlagObj->GetTransform()->SetLocalScale(EAE_Engine::Math::Vector3(0.2f, 0.2f, 0.2f));
    EAE_Engine::Graphics::AOSMeshRender* pFlagRender = EAE_Engine::Graphics::AOSMeshRenderManager::GetInstance().AddMeshRender(pathFlag, pFlagObj->GetTransform());
    pFlagRender->AddMaterial("lambert");
    EAE_Engine::Graphics::MaterialDesc* pMaterial = pFlagRender->GetMaterial();
    pMaterial->ChangeUniformVariable("g_RGBColor", &color);
  }

	void CreatePlayer(EAE_Engine::Math::Vector3 playerinitPos)
	{
		// Player
		{
      g_pPlayerObj = EAE_Engine::Core::World::GetInstance().AddGameObj("player", playerinitPos);
		//	EAE_Engine::Math::Vector3 axis(0.0f, 1.0f, 0.0f);
		//	float radian = EAE_Engine::Math::ConvertDegreesToRadians(20.0f);
		//	EAE_Engine::Math::Quaternion player_rotation(radian, axis);
		//	pPlayerObj->GetTransform()->SetRotation(player_rotation);
			EAE_Engine::Math::Vector3 playerRenderPos(0.0f, 2.0f, 0.0f);
			EAE_Engine::Common::IGameObj* pPlayerRenderObj = EAE_Engine::Core::World::GetInstance().AddGameObj("playerRenderObj", playerRenderPos);
			pPlayerRenderObj->GetTransform()->SetParent(g_pPlayerObj->GetTransform());
			EAE_Engine::Graphics::AOSMeshRender* pPlayerRender = EAE_Engine::Graphics::AOSMeshRenderManager::GetInstance().AddMeshRender(pathPlayer, pPlayerRenderObj->GetTransform());
			{
				EAE_Engine::Graphics::CylinderSOAMesh cylinder(1.0f, 1.0f, 4.0f, 20, 1);
				EAE_Engine::Mesh::sSubMesh subMesh(0, (uint32_t)cylinder._indices.size() - 1);
				std::vector<EAE_Engine::Mesh::sVertex> vertices;
				vertices.resize(cylinder._vertices.size());
				for (uint32_t index = 0; index < (uint32_t)cylinder._vertices.size(); ++index)
				{
					vertices[index].x = cylinder._vertices[index]._x;
					vertices[index].y = cylinder._vertices[index]._y;
					vertices[index].z = cylinder._vertices[index]._z;
					vertices[index].nx = cylinder._normals[index]._x;
					vertices[index].ny = cylinder._normals[index]._y;
					vertices[index].nz = cylinder._normals[index]._z;
					vertices[index].u = cylinder._uvs[index]._x;
					vertices[index].v = cylinder._uvs[index]._y;
					vertices[index].r = cylinder._colors[index]._x;
					vertices[index].g = cylinder._colors[index]._y;
					vertices[index].b = cylinder._colors[index]._z;
					vertices[index].a = cylinder._colors[index]._w;
				}
        //EAE_Engine::Graphics::AOSMesh* pNewAOSMesh = EAE_Engine::Graphics::CreateAOSMesh();
				pPlayerRender->GetSharedMesh()->ChangeWholeBuffers(&vertices[0], (uint32_t)vertices.size(),
					&cylinder._indices[0], (uint32_t)cylinder._indices.size(), &subMesh, 1);
			}
			pPlayerRender->AddMaterial("phongShading");

			g_pRigidBody = EAE_Engine::Physics::Physics::GetInstance()->AddRigidBody(g_pPlayerObj->GetTransform());
      g_pPlayerObj->AddComponent({ g_pRigidBody, g_pRigidBody->GetTypeID() });
			EAE_Engine::Common::ICompo* pCompo = g_pPlayerObj->GetTransform()->GetComponent(getTypeID<EAE_Engine::Physics::RigidBody>());
			EAE_Engine::Physics::RigidBody* pRBA = dynamic_cast<EAE_Engine::Physics::RigidBody*>(pCompo);
			
			//Set Collider
			//EAE_Engine::Math::Vector3 playerSize = EAE_Engine::Math::Vector3(0.3f, 1.0f, 0.3f);
			//pPlayerCollider = EAE_Engine::Collider::CreateOBBCollider(pPlayerObj->GetTransform(), playerSize);
			//pPlayerCollider->RegistOnCollideCallback(OnCollideRestart);
			//Set Controller
		//	pPlayerController = new PlayerController(pPlayerObj->GetTransform());
		//	EAE_Engine::Controller::ControllerManager::GetInstance().AddController(pPlayerController);
      g_pPlayerController = new RelativeScreenInput(g_pPlayerObj->GetTransform());
      EAE_Engine::Controller::ControllerManager::GetInstance().AddController(g_pPlayerController);
		}
	}

	void CreateCamera() 
	{
		EAE_Engine::Math::Vector3 camera_pos = EAE_Engine::Math::Vector3(0.0f, 0.0f, 0.0f);
		EAE_Engine::Math::Vector3 axis(1.0f, 0.0f, 0.0f);
		float radian = EAE_Engine::Math::ConvertDegreesToRadians(0.0f);
		EAE_Engine::Math::Quaternion camera_rotation(radian, axis);
		//EAE_Engine::Math::Quaternion camera_rotation = EAE_Engine::Math::Quaternion::Identity;
		pCamera = EAE_Engine::Engine::CreateCamera("mainCamera", camera_pos, camera_rotation,
			_windowWidth, _windowHeight);
		pCameraObj = pCamera->GetTransform()->GetGameObj();
		//pCamera->GetTransform()->SetParent(pPlayerObj->GetTransform());
		//Camera Controller
    /*
    {
      pCamController = new CameraController(pCamera);
		pCamController->ResetCamera(pPlayerObj->GetTransform());
		pCamController->SetActive(false);
		EAE_Engine::Controller::ControllerManager::GetInstance().AddController(pCamController);
    }
    */
    {
      pCamController = new LazyCamera(pCamera);
      pFlyCamController = new FlyCameraController(pCamera);
      ((LazyCamera*)pCamController)->ResetCamera(g_pPlayerObj->GetTransform());
      EAE_Engine::Controller::ControllerManager::GetInstance().AddController(pCamController);
      EAE_Engine::Controller::ControllerManager::GetInstance().AddController(pFlyCamController);
      pFlyCamController->SetActive(false);
    }
	}

	void CreateSprite() 
	{
	}

	void CreateDebugMenu()
	{
		// Add widgets
		EAE_Engine::Math::Vector3 textPos = EAE_Engine::Math::Vector3::Zero;
		EAE_Engine::Common::IGameObj* pTextObj = EAE_Engine::Core::World::GetInstance().AddGameObj("textObj", textPos);
		{
			pFrameText = EAE_Engine::Graphics::UIElementManager::GetInstance()->AddText("test", pTextObj->GetTransform());
			pFrameText->_rectTransform.SetAnchor({ 0.0f, 0.0f, 1.0f, 1.0f });
			pFrameText->_rectTransform.SetRect({ 32.0f, -32.0f, 64.0f, 64.0f });
		}
		EAE_Engine::Common::IGameObj* pBtnObj = EAE_Engine::Core::World::GetInstance().AddGameObj("btnObj", textPos);
		{
			pControlBtn = EAE_Engine::Graphics::UIElementManager::GetInstance()->AddButton(btnCallBack, pBtnObj->GetTransform());
			pControlBtn->_rectTransform.SetAnchor({ 0.0f, 0.0f, 1.0f, 1.0f });
			pControlBtn->_rectTransform.SetRect({ 32.0f, -64.0f, 64.0f, 64.0f });
			pControlBtn->_backgroundImage._rectTransform.SetAnchor({ 0.0f, 0.0f, 1.0f, 1.0f });
			pControlBtn->_backgroundImage._rectTransform.SetRect({ 32.0f, -64.0f, 64.0f, 64.0f });
			pControlBtn->_text._rectTransform.SetAnchor({ 0.0f, 0.0f, 1.0f, 1.0f });
			pControlBtn->_text._rectTransform.SetRect({ 32.0f, -64.0f, 64.0f, 64.0f });
		}
		EAE_Engine::Common::IGameObj* pSliderObj = EAE_Engine::Core::World::GetInstance().AddGameObj("sliderObj", textPos);
		{
			pSlider = EAE_Engine::Graphics::UIElementManager::GetInstance()->AddSlider(50.0f, 200.0f, pSliderObj->GetTransform());
			pSlider->_rectTransform.SetAnchor({ 0.0f, 0.0f, 1.0f, 1.0f });
			pSlider->_rectTransform.SetRect({ 64.0f, -128.0f, 64.0f, 64.0f });
			pSlider->_backgroundImage._rectTransform.SetAnchor({ 0.0f, 0.0f, 1.0f, 1.0f });
			pSlider->_backgroundImage._rectTransform.SetRect({ 64.0f, -128.0f, 128.0f, 16.0f });
			pSlider->SetHandle(50.0f);
		}
		EAE_Engine::Common::IGameObj* pToggleObj = EAE_Engine::Core::World::GetInstance().AddGameObj("toggleObj", textPos);
		{
			pToggle = EAE_Engine::Graphics::UIElementManager::GetInstance()->AddToggle(false, pToggleObj->GetTransform());
			pToggle->_rectTransform.SetAnchor({ 0.0f, 0.0f, 1.0f, 1.0f });
			pToggle->_rectTransform.SetRect({ 16.0f, -168.0f, 16.0f, 16.0f });
			pToggle->_backgroundImage._rectTransform.SetAnchor({ 0.0f, 0.0f, 1.0f, 1.0f });
			pToggle->_backgroundImage._rectTransform.SetRect({ 16.0f, -168.0f, 16.0f, 16.0f });
		}
		EAE_Engine::Common::IGameObj* pSegToggleObj = EAE_Engine::Core::World::GetInstance().AddGameObj("segtoggleObj", textPos);
		{
			pDrawSegmentToggle = EAE_Engine::Graphics::UIElementManager::GetInstance()->AddToggle(true, pSegToggleObj->GetTransform());
			pDrawSegmentToggle->_rectTransform.SetAnchor({ 0.0f, 0.0f, 1.0f, 1.0f });
			pDrawSegmentToggle->_rectTransform.SetRect({ 16.0f, -200.0f, 16.0f, 16.0f });
			pDrawSegmentToggle->_backgroundImage._rectTransform.SetAnchor({ 0.0f, 0.0f, 1.0f, 1.0f });
			pDrawSegmentToggle->_backgroundImage._rectTransform.SetRect({ 16.0f, -200.0f, 16.0f, 16.0f });
		}
	}

}

