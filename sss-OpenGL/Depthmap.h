#ifndef __depthmapH__
#define __depthmapH__

#include "Camera.h"
#include "Shader.h"
#include "Object.h"

class Depthmap{

public:

	Depthmap(Camera* camera, int width, int height);
	~Depthmap();

	unsigned char *irradianceData;
	unsigned char *normalDataColor;
	unsigned char *normalDataDepth;
	unsigned char *depthData;
	float *singleChannelData;

	unsigned int irradianceMapTexture;
	unsigned int normalMapTexture;
	unsigned int normalDepthTexture;
	unsigned int depthmapTexture;
	unsigned int depthmapTexturePCF;
	unsigned int depthmapTextureMSAA;

	unsigned int singleChannelTexture;
	unsigned int texture;
	unsigned int depthmapFBO;
	unsigned int depthmapFBOPCF;

	void setViewMatrix(const Vector3f &lightPos, const Vector3f &target, const Vector3f &up);
	void setProjectionMatrix(float fovx, float aspect, float znear, float zfar);	
	void setOrthMatrix(float left, float right, float bottom, float top, float znear, float zfar);
	void setColor(const Vector3f &color);

	void renderIrradianceMap(Object const* object);
	void renderNormalMap(Object const* object);
	void renderToDepthMSAA(Object const* object);
	void renderToDepth(Object const* object);
	void renderToDepthPCF(Object const* object);
	void renderToSingleChannel(Object const* object);
	void render(unsigned int texture);
	

	const Matrix4f &getDepthPassMatrix() const;
	const Matrix4f &getProjectionMatrix() const;
	const Matrix4f &getProjectionMatrixD3D() const;
	const Matrix4f &getLinearProjectionMatrixD3D() const;
	const Matrix4f &getOrthographicMatrix() const;
	const Matrix4f &getViewMatrix() const;
	const Vector3f &getPosition() const;
	const Vector3f &getViewDirection() const;
	const Vector3f &getColor() const;
private :

	
	unsigned int normalFBO;
	unsigned int normalFBOMSAA;

	unsigned int fboIrradianceMSAA;
	unsigned int fboCIrradiance;

	unsigned int fboNormalMSAA;
	unsigned int fboCNormal;
	unsigned int fboDNormal;

	unsigned int fboDDepthMSAA;
	unsigned int fboDDepth;

	unsigned int fboSingleChannelMSAA;
	unsigned int fboCSingleChannel;

	unsigned int depthmapWidth = 512*2;
	unsigned int depthmapHeight = 512*2;

	unsigned int viewportWidth = 640;
	unsigned int viewportHeight = 480;

	unsigned int depthViewportWidth = 512*2;
	unsigned int depthViewportHeight = 512*2;

	unsigned int m_quadVBO;
	unsigned int m_indexQuad;
	int m_size = 25;
	std::vector<float> m_vertex;

	Matrix4f m_biasMatrix;
	Matrix4f m_viewMatrix;
	Matrix4f m_projMatrix;
	Matrix4f m_projMatrixD3D;
	Matrix4f m_linearProjMatrixD3D;
	Matrix4f m_orthMatrix;

	Vector3f m_eye;
	Vector3f m_target;
	Vector3f m_viewDir;
	Vector3f m_color;
	Shader *m_irradianceShader;
	Shader *m_normalShader;
	Shader* m_depthMapShader;
	Shader* m_depthMapShader2;
	TextureShader *m_renderShader;
	DepthShader* m_shader;

	Camera* m_camera;

	void createDepthmapFBO();
	void createBuffer();
	void createDepthmapFBO2();
};

#endif // __depthmapH__