#ifndef IGRAPHICSTEXTURE_H
#define IGRAPHICSTEXTURE_H


class IGraphicsTexture
{
public:
	virtual ~IGraphicsTexture();

	static IGraphicsTexture* Create();

protected:
};

#endif