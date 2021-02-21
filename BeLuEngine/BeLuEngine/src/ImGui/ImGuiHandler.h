#ifndef IMGUIHANDLER_H
#define IMGUIHANDLER_H

class ImGuiHandler
{
public:
	static ImGuiHandler& GetInstance();

	void NewFrame();
	void UpdateFrame();

private:
	ImGuiHandler();
	~ImGuiHandler();


	void updateMemoryInfo();
};

#endif
