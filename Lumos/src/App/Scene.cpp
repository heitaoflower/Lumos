#include "lmpch.h"
#include "Scene.h"
#include "Core/OS/Input.h"
#include "Application.h"
#include "Scripting/LuaManager.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/RenderManager.h"
#include "Graphics/Camera/Camera.h"
#include "Utilities/TimeStep.h"
#include "Audio/AudioManager.h"
#include "Physics/LumosPhysicsEngine/SortAndSweepBroadphase.h"
#include "Physics/LumosPhysicsEngine/Octree.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Scripting/LuaManager.h"

#include <sol/sol.hpp>

namespace Lumos
{
	Scene::Scene(const String& friendly_name) :
		m_SceneName(friendly_name),
		m_pCamera(nullptr), 
		m_EnvironmentMap(nullptr), 
		m_SceneBoundingRadius(0),
		m_ScreenWidth(0),
		m_ScreenHeight(0)
    {
	}

    Scene::~Scene()
    {
        if(m_LuaEnv && m_LuaEnv["OnDestroy"])
            m_LuaEnv["OnDestroy"]();
        
        DeleteAllGameObjects();
    }

	void Scene::OnInit()
	{
        LuaManager::Instance()->GetState().set("registry", &m_Registry);
        LuaManager::Instance()->GetState().set("scene", this);

		m_CurrentScene = true;

		String Configuration;
		String Platform;
		String RenderAPI;
		String dash = " - ";

#ifdef LUMOS_DEBUG
		Configuration = "Debug";
#else
		Configuration = "Release";
#endif

#ifdef LUMOS_PLATFORM_WINDOWS
		Platform = "Windows";
#elif LUMOS_PLATFORM_LINUX
		Platform = "Linux";
#elif LUMOS_PLATFORM_MACOS
		Platform = "MacOS";
#elif LUMOS_PLATFORM_IOS
		Platform = "iOS";
#endif

		switch (Graphics::GraphicsContext::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case Graphics::RenderAPI::OPENGL: RenderAPI = "OpenGL"; break;
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
		case Graphics::RenderAPI::VULKAN: RenderAPI = "Vulkan ( MoltenVK )"; break;
#else
		case Graphics::RenderAPI::VULKAN: RenderAPI = "Vulkan"; break;
#endif
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
		case DIRECT3D: RenderAPI = "Direct3D"; break;
#endif
        default: break;
		}

		std::stringstream Title;
		Title << Platform << dash << RenderAPI << dash << Configuration << dash << m_SceneName << dash << Application::Instance()->GetWindow()->GetTitle();

		Application::Instance()->GetWindow()->SetWindowTitle(Title.str());

		//Default physics setup
		Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetDampingFactor(0.998f);
		Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetIntegrationType(IntegrationType::RUNGE_KUTTA_4);
		Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetBroadphase(Lumos::CreateRef<Octree>(5, 3, Lumos::CreateRef<SortAndSweepBroadphase>()));

		m_SceneBoundingRadius = 400.0f; //Default scene radius of 400m

		m_SceneGraph.Init(m_Registry);
	}

	void Scene::OnCleanupScene()
	{
        if(m_LuaEnv && m_LuaEnv["OnCleanUp"])
            m_LuaEnv["OnCleanUp"]();
		DeleteAllGameObjects();

		Application::Instance()->GetRenderManager()->Reset();

		auto audioManager = AudioManager::Create();
		if (audioManager)
		{
			audioManager->ClearNodes();
		}

		m_CurrentScene = false;
	};

	void Scene::DeleteAllGameObjects()
	{
		m_Registry.each([&](auto entity) 
		{
			m_Registry.destroy(entity);
		});
	}

	void Scene::OnUpdate(TimeStep* timeStep)
	{
		const Maths::Vector2 mousePos = Input::GetInput()->GetMousePosition();

		if (m_pCamera && Application::Instance()->GetSceneActive())
		{
			m_pCamera->HandleMouse(timeStep->GetMillis(), mousePos.x, mousePos.y);
			m_pCamera->HandleKeyboard(timeStep->GetMillis());
		}

		m_SceneGraph.Update(m_Registry);
        
        if(m_LuaEnv && m_LuaEnv["OnUpdate"])
            m_LuaEnv["OnUpdate"](timeStep->GetElapsedMillis());
	}

	void Scene::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
 		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Scene::OnWindowResize));
	}

	bool Scene::OnWindowResize(WindowResizeEvent& e)
	{
		if (m_pCamera)
		{
			m_pCamera->SetAspectRatio(static_cast<float>(e.GetWidth()) / static_cast<float>(e.GetHeight()));
		}

		return false;
	}

	nlohmann::json Scene::Serialise()
	{
		nlohmann::json output;
		output["typeID"] = LUMOS_TYPENAME(Scene);
		output["name"] = m_SceneName;

		return output;
	}

	void Scene::Deserialise(nlohmann::json & data)
	{
		m_SceneName = data["name"];
	}

    void Scene::LoadLuaScene(const String &filePath)
    {
        m_LuaEnv = sol::environment(LuaManager::Instance()->GetState(), sol::create, LuaManager::Instance()->GetState().globals());
        m_LuaEnv["scene"] = this;
        m_LuaEnv["reg"] = &m_Registry;
        
        String physicalPath;
        if (!VFS::Get()->ResolvePhysicalPath(filePath, physicalPath))
        {
            Debug::Log::Error("Failed to Load Lua script {0}", filePath );
            return;
        }
        
        auto loadFileResult = LuaManager::Instance()->GetState().script_file(physicalPath, m_LuaEnv, sol::script_pass_on_error);
        if (!loadFileResult.valid())
        {
            sol::error err = loadFileResult;
            Debug::Log::Error("Failed to Execute Lua script {0}" , physicalPath );
            Debug::Log::Error("Error : {0}", err.what());
        }

        if(m_LuaEnv["OnInit"])
            m_LuaEnv["OnInit"]();
    }

    Scene* Scene::LoadFromLua(const String& filePath)
    {
        Scene* scene = new Scene("");
        scene->LoadLuaScene(filePath);
        
        return scene;
    }
}
