#ifndef IMGUIHANDLER_H
#define IMGUIHANDLER_H

struct MouseClick;
class Entity;

class ImGuiHandler
{
public:
	static ImGuiHandler& GetInstance();

	void NewFrame();
	void EndFrame();
	void UpdateFrame();

private:
	ImGuiHandler();
	~ImGuiHandler();

	// On clicked event
	void onEntityClicked(MouseClick* event);
	Entity* m_pSelectedEntity = nullptr;
	void drawSelectedEntityInfo();

	void updateMemoryInfo();
	void resetThreadInfos();
};

#endif
