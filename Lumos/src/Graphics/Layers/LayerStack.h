#pragma once

#include "Core/Core.h"
#include "Layer.h"

namespace Lumos
{
	class TimeStep;
	class Scene;
	class Event;

	class LUMOS_EXPORT LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);
		void Clear();

		void OnRender(Scene* scene);
		void OnUpdate(TimeStep* timeStep, Scene* scene);
		void OnEvent(Event& e);
		void OnImGui();

		u32 GetCount() const { return (u32)m_Layers.size(); }
        
        std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
        std::vector<Layer*>::iterator end() { return m_Layers.end(); }

	private:
        std::vector<Layer*> m_Layers;
		size_t m_LayerCount = 0;
	};
}
