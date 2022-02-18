#ifndef IMGUIHANDLER_H
#define IMGUIHANDLER_H

struct MouseClick;
class Entity;

class ImGuiHandler
{
public:
	static ImGuiHandler& GetInstance();

	void BeginFrame();
	void EndFrame();
	void UpdateFrame();

private:
	ImGuiHandler();
	~ImGuiHandler();

	// On clicked event
	void onEntityClicked(MouseClick* event);
	Entity* m_pSelectedEntity = nullptr;
	void drawSceneHierarchy();

	void updateMemoryInfo();
};

#endif
