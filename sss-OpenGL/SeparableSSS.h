#ifndef __sparableSSSH__
#define __sparableSSSH__

#include <vector>

#include "Shader.h"


class SeparableSSS {

public:

	SeparableSSS();
	~SeparableSSS();

	void render(unsigned int texture);

	void createBuffer();

	unsigned int m_quadVBO, m_indexQuad;
	int m_size = 1;
	std::vector<float> m_vertex;

};

#endif // __sparableSSSH__