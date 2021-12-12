#ifndef RENDERCORE_H
#define RENDERCORE_H

class IGraphicsContext;

// Dont create this class immediatly, use the #define below
class ScopedPIXEvent
{
public:
	ScopedPIXEvent(const char* nameOfTask, IGraphicsContext* graphicsContext);
	~ScopedPIXEvent();

private:
	IGraphicsContext* m_pGraphicsContext = nullptr;
};

#ifdef DEBUG	// This is both for Debug and Release, not for Dist
	#define ScopedPixEvent(name, commandList) ScopedPIXEvent concat(PIX_Event_Marker, __LINE__)(#name, commandList);
#else
	#define ScopedPixEvent(name, commandList);
#endif

#endif