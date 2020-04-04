#include "SeparableSSS.h"

SeparableSSS::SeparableSSS() {

	createBuffer();
}

SeparableSSS::~SeparableSSS() {

}



void SeparableSSS::createBuffer() {

	m_vertex.push_back(1.0 * m_size); m_vertex.push_back(1.0 * m_size); m_vertex.push_back(0.0 * m_size); m_vertex.push_back(1.0); m_vertex.push_back(1.0);
	m_vertex.push_back(-1.0 * m_size); m_vertex.push_back(1.0 * m_size); m_vertex.push_back(0.0 * m_size); m_vertex.push_back(0.0); m_vertex.push_back(1.0);
	m_vertex.push_back(-1.0 * m_size); m_vertex.push_back(-1.0 * m_size); m_vertex.push_back(0.0 * m_size); m_vertex.push_back(0.0); m_vertex.push_back(0.0);
	m_vertex.push_back(1.0 * m_size); m_vertex.push_back(-1.0 * m_size); m_vertex.push_back(0.0 * m_size); m_vertex.push_back(1.0); m_vertex.push_back(0.0);


	glGenBuffers(1, &m_quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
	glBufferData(GL_ARRAY_BUFFER, m_vertex.size() * sizeof(float), &m_vertex[0], GL_STATIC_DRAW);

	static const GLushort index[] = {

		0, 1, 2,
		0, 2, 3
	};

	glGenBuffers(1, &m_indexQuad);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexQuad);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index), index, GL_STATIC_DRAW);
}