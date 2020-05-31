#include "Depthmap.h"

Depthmap::Depthmap(Camera* camera, int width, int height){

	viewportWidth = width;
	viewportHeight = height;

	//char arrays for creating debug bitmaps
	irradianceData = (unsigned char*)malloc(3 * depthmapWidth * depthmapHeight);
	normalDataColor = (unsigned char*)malloc(3 * depthmapWidth * depthmapHeight);
	normalDataDepth = (unsigned char*)malloc(depthmapWidth * depthmapHeight);
	depthData = (unsigned char*)malloc(depthmapWidth * depthmapHeight);
	singleChannelData = new float[depthmapWidth * depthmapHeight];

	m_camera = camera;
	
	m_irradianceShader = new Shader("shader/irradiance.vert", "shader/irradiance.frag");
	m_normalShader = new Shader("shader/normalMap.vert", "shader/normalMap.frag");
	m_depthMapShader = new Shader("sss/depthMap.vert", "sss/depthMap.frag");
	m_depthMapShader2 = new Shader("shader/depthMap2.vert", "shader/depthMap2.frag");
	m_renderShader = new TextureShader("shader/texture.vert", "shader/texture.frag");

	m_shader = new DepthShader("shader/depth.vert", "shader/depth.frag");

	glUseProgram(m_renderShader->m_program);
		m_renderShader->loadMatrix("u_projection", camera->getProjectionMatrix());
	glUseProgram(0);

	createDepthmapFBO();
	createDepthmapFBO2();
	createBuffer();

	m_biasMatrix = Matrix4f(0.5, 0.0, 0.0, 0.5,
							0.0, 0.5, 0.0, 0.5,
							0.0, 0.0, 0.5, 0.5,
							0.0, 0.0, 0.0, 1.0);

	Bitmap *bitmap = new Bitmap();
	if (bitmap->loadBitmap24("textures/jade.bmp")){

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bitmap->width, bitmap->height, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap->data);

		glGenerateMipmap(GL_TEXTURE_2D);

	}
	delete bitmap;
	bitmap = NULL;

	

}

Depthmap::~Depthmap(){

	if (m_normalShader){
		delete m_normalShader;
		m_normalShader = 0;
	}

	if (m_irradianceShader){
		delete m_irradianceShader;
		m_irradianceShader = 0;
	}

	if (m_depthMapShader){
		delete m_depthMapShader;
		m_depthMapShader = 0;
	}

	if (m_depthMapShader2){
		delete m_depthMapShader2;
		m_depthMapShader2 = 0;
	}

	if (irradianceData){
		free(irradianceData);
		irradianceData = NULL;
	}

	if (normalDataColor){
		free(normalDataColor);
		normalDataColor = NULL;
	}

	if (normalDataDepth){
		free(normalDataDepth);
		normalDataDepth = NULL;
	}

	if (depthData){
		free(depthData);
		depthData = NULL;
	}

	if (singleChannelData){
		free(singleChannelData);
		singleChannelData = NULL;
	}
}

void Depthmap::createDepthmapFBO(){

	////////////////////////////irradianceMap rendertarget//////////////////////////////////////////////
	glGenTextures(1, &irradianceMapTexture);
	glBindTexture(GL_TEXTURE_2D, irradianceMapTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, depthmapWidth, depthmapHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//////////////////////////setup MSAA framebuffer///////////////////////////////////////
	unsigned int colorBuf8;
	glGenRenderbuffers(1, &colorBuf8);
	glBindRenderbuffer(GL_RENDERBUFFER, colorBuf8);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGB, depthmapWidth, depthmapHeight);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, depthmapWidth, depthmapHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	unsigned int depthBuf8;
	glGenRenderbuffers(1, &depthBuf8);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuf8);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, depthmapWidth, depthmapHeight);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, depthmapWidth, depthmapHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glGenFramebuffers(1, &fboIrradianceMSAA);
	glBindFramebuffer(GL_FRAMEBUFFER, fboIrradianceMSAA);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBuf8);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf8);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//////////////////////////end setup MSAA framebuffer///////////////////////////////////////

	//////////////////////////setup normal (no MSAA) FBOs as blit target///////////////////////
	glGenFramebuffers(1, &fboCIrradiance);
	glBindFramebuffer(GL_FRAMEBUFFER, fboCIrradiance);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, irradianceMapTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////


	////////////////////////////normalMap rendertarget//////////////////////////////////////////////
	glGenTextures(1, &normalMapTexture);
	glBindTexture(GL_TEXTURE_2D, normalMapTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, depthmapWidth, depthmapHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	glGenTextures(1, &normalDepthTexture);
	glBindTexture(GL_TEXTURE_2D, normalDepthTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, depthmapWidth, depthmapHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	//////////////////////////setup MSAA framebuffer///////////////////////////////////////
	unsigned int colorBuf1;
	glGenRenderbuffers(1, &colorBuf1);
	glBindRenderbuffer(GL_RENDERBUFFER, colorBuf1);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGB, depthmapWidth, depthmapHeight);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, depthmapWidth, depthmapHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	
	unsigned int depthBuf1;
	glGenRenderbuffers(1, &depthBuf1);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuf1);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, depthmapWidth, depthmapHeight);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, depthmapWidth, depthmapHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);


	glGenFramebuffers(1, &fboNormalMSAA);
	glBindFramebuffer(GL_FRAMEBUFFER, fboNormalMSAA);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBuf1);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//////////////////////////end setup MSAA framebuffer///////////////////////////////////////

	//////////////////////////setup normal (no MSAA) FBOs as blit target///////////////////////
	glGenFramebuffers(1, &fboCNormal);
	glBindFramebuffer(GL_FRAMEBUFFER, fboCNormal);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, normalMapTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	glGenFramebuffers(1, &fboDNormal);
	glBindFramebuffer(GL_FRAMEBUFFER, fboDNormal);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, normalDepthTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	////////////////////////////depthMap rendertarget//////////////////////////////////////////////
	glGenTextures(1, &depthmapTextureMSAA);
	glBindTexture(GL_TEXTURE_2D, depthmapTextureMSAA);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, depthmapWidth, depthmapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	//////////////////////////setup MSAA framebuffer///////////////////////////////////////

	unsigned int depthBuf2;
	glGenRenderbuffers(1, &depthBuf2);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuf2);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, depthmapWidth, depthmapHeight);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, depthmapWidth, depthmapHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glGenFramebuffers(1, &fboDDepthMSAA);
	glBindFramebuffer(GL_FRAMEBUFFER, fboDDepthMSAA);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf2);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	//////////////////////////end setup MSAA framebuffer///////////////////////////////////////

	//////////////////////////setup normal (no MSAA) FBOs as blit target///////////////////////
	glGenFramebuffers(1, &fboDDepth);
	glBindFramebuffer(GL_FRAMEBUFFER, fboDDepth);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthmapTextureMSAA, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	////////////////////////////////////////////////////////////////////////////////////////////////
	
	//////////////////////////setup MSAA lumiance framebuffer///////////////////////////////////////
	glGenTextures(1, &singleChannelTexture);
	glBindTexture(GL_TEXTURE_2D, singleChannelTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, depthmapWidth, depthmapHeight, 0, GL_RED, GL_FLOAT, NULL);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F_ARB, depthmapWidth, depthmapHeight, 0, GL_RGB32F_ARB, GL_FLOAT, NULL);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F_ARB, depthmapWidth, depthmapHeight, 0,GL_RGB, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	unsigned int colorBuf3;
	glGenRenderbuffers(1, &colorBuf3);
	glBindRenderbuffer(GL_RENDERBUFFER, colorBuf3);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RED, depthmapWidth, depthmapHeight);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_LUMINANCE, depthmapWidth, depthmapHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);


	unsigned int depthBuf3;
	glGenRenderbuffers(1, &depthBuf3);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuf3);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, depthmapWidth, depthmapHeight);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, depthmapWidth, depthmapHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);


	glGenFramebuffers(1, &fboSingleChannelMSAA);
	glBindFramebuffer(GL_FRAMEBUFFER, fboSingleChannelMSAA);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBuf3);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf3);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//////////////////////////end setup MSAA framebuffer///////////////////////////////////////

	//////////////////////////setup normal (no MSAA) FBOs as blit target///////////////////////
	glGenFramebuffers(1, &fboCSingleChannel);
	glBindFramebuffer(GL_FRAMEBUFFER, fboCSingleChannel);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, singleChannelTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	///////////////////////////////////////////////////////////////////////////////////////////
}

void Depthmap::createDepthmapFBO2() {


	glGenFramebuffers(1, &depthmapFBO);
	glGenTextures(1, &depthmapTexture);

	glBindTexture(GL_TEXTURE_2D, depthmapTexture);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, depthmapWidth, depthmapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, depthmapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthmapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	glGenFramebuffers(1, &depthmapFBOPCF);
	glGenTextures(1, &depthmapTexturePCF);

	glBindTexture(GL_TEXTURE_2D, depthmapTexturePCF);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, depthmapWidth, depthmapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, depthmapFBOPCF);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthmapTexturePCF, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Depthmap::setViewMatrix(const Vector3f &lightPos, const Vector3f &target, const Vector3f &up){

	
	Matrix4f view;
	view.lookAt(lightPos, target, up);
	m_viewMatrix = view;

	m_eye = lightPos;
	m_target = target;

	m_viewDir = (m_target - m_eye).normalize();
}

void Depthmap::setProjectionMatrix(float fovx, float aspect, float znear, float zfar){


	Matrix4f projection;
	projection.perspectiveD3D(fovx, (GLfloat)depthmapWidth / (GLfloat)depthmapHeight, znear, zfar);
	m_projMatrixD3D = projection;

	projection.perspective(fovx, (GLfloat)depthmapWidth / (GLfloat)depthmapHeight, znear, zfar);
	m_projMatrix = projection;

	projection.linearPerspectiveD3D(fovx, (GLfloat)depthmapWidth / (GLfloat)depthmapHeight, znear, zfar);
	m_linearProjMatrixD3D = projection;

	projection.orthographic(0, (GLfloat)depthmapWidth, (GLfloat)depthmapHeight, 0, 0, 1000);
	m_orthMatrix = projection;

	glUseProgram(m_irradianceShader->m_program);
	m_irradianceShader->loadMatrix("u_projection", m_projMatrix);
	glUseProgram(0);

	glUseProgram(m_normalShader->m_program);
	m_normalShader->loadMatrix("u_projection", m_projMatrix);
	glUseProgram(0);

	glUseProgram(m_depthMapShader->m_program);
	m_depthMapShader->loadMatrix("u_projection", m_linearProjMatrixD3D);
	glUseProgram(0);

	glUseProgram(m_depthMapShader2->m_program);
	m_depthMapShader2->loadMatrix("u_projection", m_projMatrix);
	glUseProgram(0);

	glUseProgram(m_shader->m_program);
	m_shader->loadMatrix("u_projection", m_projMatrix);
	glUseProgram(0);

}



void Depthmap::setOrthMatrix(float left, float right, float bottom, float top, float znear, float zfar){

	Matrix4f projection;
	projection.orthographic(left, right, bottom, top, znear, zfar);
	m_orthMatrix = projection;
}

const Matrix4f &Depthmap::getOrthographicMatrix() const{

	return m_orthMatrix;
}

const Matrix4f &Depthmap::getDepthPassMatrix() const{

	return   m_projMatrix * m_biasMatrix;
}



const Matrix4f &Depthmap::getProjectionMatrix() const{

	return  m_projMatrix;
}

const Matrix4f &Depthmap::getProjectionMatrixD3D() const{

	return  m_projMatrixD3D;
}

const Matrix4f &Depthmap::getLinearProjectionMatrixD3D() const{

	return  m_linearProjMatrixD3D;
}

const Matrix4f &Depthmap::getViewMatrix() const{

	return m_viewMatrix;
}

const Vector3f &Depthmap::getViewDirection() const{

	return m_viewDir;
}

const Vector3f &Depthmap::getPosition() const{

	return m_eye;
}

const Vector3f &Depthmap::getColor() const {
	return m_color;
}

void Depthmap::setColor(const Vector3f &color) {
	m_color = color;
}

void Depthmap::renderToDepth(Object const* object) {
	
	
	glViewport(0, 0, depthViewportWidth, depthViewportHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, depthmapFBO);

		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(0xFF);
		glDepthFunc(GL_LEQUAL);

		glUseProgram(m_shader->m_program);
		m_shader->loadMatrix("u_modelView", object->m_model->getTransformationMatrix() * m_viewMatrix);


		for (int i = 0; i < object->m_model->numberOfMeshes(); i++) {

			glBindBuffer(GL_ARRAY_BUFFER, object->m_model->getMesches()[i]->getVertexName());
			m_shader->bindAttributes(object->m_model->getMesches()[i], NULL);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->m_model->getMesches()[i]->getIndexName());
			glDrawElements(GL_TRIANGLES, object->m_model->getMesches()[i]->getNumberOfTriangles() * 3, GL_UNSIGNED_INT, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			m_shader->unbindAttributes(object->m_model->getMesches()[i]);

			glBindBuffer(GL_ARRAY_BUFFER, 0);

		}
		glUseProgram(0);

		glDisable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	


	/*glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
	glBindFramebuffer(GL_FRAMEBUFFER, depthmapFBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glReadPixels(0, 0, depthmapWidth, depthmapHeight, GL_DEPTH_COMPONENT, GL_FLOAT, singleChannelData);*/



	/*for (int i = 0; i <  depthmapWidth * depthmapHeight ; i++) {

		if (singleChannelData[i] != 1) {
			std::cout << singleChannelData[i] << "  ";
		}
	}*/
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, viewportWidth, viewportHeight);
	
}

void Depthmap::renderToDepthPCF(Object const* object) {

	glViewport(0, 0, depthViewportWidth, depthViewportHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, depthmapFBOPCF);

	glClear(GL_DEPTH_BUFFER_BIT);
	/*glEnable(GL_DEPTH_TEST);
	glDepthMask(0xFF);
	glDepthFunc(GL_LEQUAL);*/

	glUseProgram(m_shader->m_program);
	m_shader->loadMatrix("u_modelView", object->m_model->getTransformationMatrix() * m_viewMatrix);


	for (int i = 0; i < object->m_model->numberOfMeshes(); i++) {

		glBindBuffer(GL_ARRAY_BUFFER, object->m_model->getMesches()[i]->getVertexName());
		m_shader->bindAttributes(object->m_model->getMesches()[i], NULL);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->m_model->getMesches()[i]->getIndexName());
		glDrawElements(GL_TRIANGLES, object->m_model->getMesches()[i]->getNumberOfTriangles() * 3, GL_UNSIGNED_INT, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		m_shader->unbindAttributes(object->m_model->getMesches()[i]);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

	}
	glUseProgram(0);

	/*glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);*/

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, viewportWidth, viewportHeight);
}


void Depthmap::renderToDepthMSAA(Object const* object){

	glViewport(0, 0, depthViewportWidth, depthViewportHeight);
	//glDepthRange(-1.0, 1.0);

	glUseProgram(m_shader->m_program);
	m_shader->loadMatrix("u_modelView", object->m_model->getTransformationMatrix() *m_viewMatrix);

	glBindFramebuffer(GL_FRAMEBUFFER, fboDDepthMSAA);	
	glClear(GL_DEPTH_BUFFER_BIT);
	
	for (int i = 0; i < object->m_model->numberOfMeshes(); i++){

		glBindBuffer(GL_ARRAY_BUFFER, object->m_model->getMesches()[i]->getVertexName());
		m_shader->bindAttributes(object->m_model->getMesches()[i], NULL);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->m_model->getMesches()[i]->getIndexName());
		glDrawElements(GL_TRIANGLES, object->m_model->getMesches()[i]->getNumberOfTriangles() * 3, GL_UNSIGNED_INT, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		m_shader->unbindAttributes(object->m_model->getMesches()[i]);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	glUseProgram(0);

	//glDepthRange(0.0, 1.0);
	glViewport(0, 0, viewportWidth, viewportHeight);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboDDepthMSAA);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboDDepth);
	glBlitFramebuffer(0, 0, depthmapWidth, depthmapHeight, 0, 0, depthmapWidth, depthmapHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	/*glBindFramebuffer(GL_FRAMEBUFFER, fboDDepth);
	glReadPixels(0, 0, depthmapWidth, depthmapHeight, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, depthData);
	Bitmap::saveBitmap8("depth1.bmp", depthData, depthmapWidth, depthmapHeight);*/
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Depthmap::renderToSingleChannel(Object const* object){

	glUseProgram(m_depthMapShader2->m_program);


	glBindFramebuffer(GL_FRAMEBUFFER, fboCSingleChannel);
	glViewport(0, 0, depthViewportWidth, depthViewportHeight);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);

	for (int i = 0; i < object->m_model->numberOfMeshes(); i++){

		m_depthMapShader2->loadMatrix("u_modelView", object->m_model->getTransformationMatrix() * m_viewMatrix);

		glBindBuffer(GL_ARRAY_BUFFER, object->m_model->getMesches()[i]->getVertexName());
		m_normalShader->bindAttributes(object->m_model->getMesches()[i], NULL);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->m_model->getMesches()[i]->getIndexName());
		glDrawElements(GL_TRIANGLES, object->m_model->getMesches()[i]->getNumberOfTriangles() * 3, GL_UNSIGNED_INT, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		m_depthMapShader2->unbindAttributes(object->m_model->getMesches()[i]);

	}
	glDisable(GL_CULL_FACE);
	glViewport(0, 0, viewportWidth, viewportHeight);


	glBindBuffer(GL_ARRAY_BUFFER, 0);


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);


	/*glBindFramebuffer(GL_READ_FRAMEBUFFER, fboSingleChannelMSAA);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboCSingleChannel);
	glBlitFramebuffer(0, 0, depthmapWidth, depthmapHeight, 0, 0, depthmapWidth, depthmapHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);  */                
	
	glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
	glBindFramebuffer(GL_FRAMEBUFFER, fboCSingleChannel);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glReadPixels(0, 0, depthmapWidth, depthmapHeight, GL_RED, GL_FLOAT, singleChannelData);
	
	

	/*for (int i = 0; i <  depthmapWidth * depthmapHeight ; i++) {

		if (singleChannelData[i] != 1) {
			std::cout << singleChannelData[i] << "  ";
		}
	}*/
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//Bitmap::saveBitmap8("depth2.bmp", singleChannelData, depthmapWidth, depthmapHeight);
}


void Depthmap::renderNormalMap(Object const* object){

	glUseProgram(m_normalShader->m_program);

	
	glBindFramebuffer(GL_FRAMEBUFFER, fboNormalMSAA);
	glViewport(0, 0, depthViewportWidth, depthViewportHeight);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	//glEnable(GL_DEPTH_TEST);
	for (int i = 0; i < object->m_model->numberOfMeshes(); i++){

		m_normalShader->loadMatrix("u_modelView", object->m_model->getTransformationMatrix() * m_viewMatrix);
		m_normalShader->loadMatrix("u_normalMatrix", Matrix4f::getNormalMatrix(object->m_model->getTransformationMatrix() * m_viewMatrix));

		glBindBuffer(GL_ARRAY_BUFFER, object->m_model->getMesches()[i]->getVertexName());
		m_normalShader->bindAttributes(object->m_model->getMesches()[i], NULL);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->m_model->getMesches()[i]->getIndexName());
		glDrawElements(GL_TRIANGLES, object->m_model->getMesches()[i]->getNumberOfTriangles() * 3, GL_UNSIGNED_INT, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		m_normalShader->unbindAttributes(object->m_model->getMesches()[i]);
		
	}
	glDisable(GL_CULL_FACE);
	glViewport(0, 0, viewportWidth, viewportHeight);
	

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);


	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboNormalMSAA);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboCNormal);
	glBlitFramebuffer(0, 0, depthmapWidth, depthmapHeight, 0, 0, depthmapWidth, depthmapHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);                           // scale filter


	/*glBindFramebuffer(GL_FRAMEBUFFER, fboCNormal);
	glReadPixels(0, 0, depthmapWidth, depthmapHeight, GL_BGR, GL_UNSIGNED_BYTE, normalData);
	Bitmap::saveBitmap24("normal.bmp", normalData, depthmapWidth, depthmapHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);*/

	

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboNormalMSAA);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboDNormal);
	glBlitFramebuffer(0, 0, depthmapWidth, depthmapHeight, 0, 0, depthmapWidth, depthmapHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	/*glBindFramebuffer(GL_FRAMEBUFFER, fboDNormal);
	glReadPixels(0, 0, depthmapWidth, depthmapHeight, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, normalDataDepth);
	Bitmap::saveBitmap8("depth.bmp", normalDataDepth, depthmapWidth, depthmapHeight);*/
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
}

void Depthmap::renderIrradianceMap(Object const* object){

	glUseProgram(m_irradianceShader->m_program);


	glBindFramebuffer(GL_FRAMEBUFFER, fboIrradianceMSAA);
	glViewport(0, 0, depthViewportWidth, depthViewportHeight);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glFrontFace(GL_CW);
	//glEnable(GL_CULL_FACE);
	//glEnable(GL_DEPTH_TEST);
	for (int i = 0; i < object->m_model->numberOfMeshes(); i++){

		m_irradianceShader->loadMatrix("u_modelView", object->m_model->getTransformationMatrix() * m_viewMatrix);
		m_irradianceShader->loadMatrix("u_normalMatrix", Matrix4f::getNormalMatrix(object->m_model->getTransformationMatrix() * m_viewMatrix));

		glBindBuffer(GL_ARRAY_BUFFER, object->m_model->getMesches()[i]->getVertexName());
		m_irradianceShader->bindAttributes(object->m_model->getMesches()[i], texture);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->m_model->getMesches()[i]->getIndexName());
		glDrawElements(GL_TRIANGLES, object->m_model->getMesches()[i]->getNumberOfTriangles() * 3, GL_UNSIGNED_INT, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		m_irradianceShader->unbindAttributes(object->m_model->getMesches()[i]);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

	}
	//glDisable(GL_CULL_FACE);
	glViewport(0, 0, viewportWidth, viewportHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);


	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboIrradianceMSAA);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboCIrradiance);
	glBlitFramebuffer(0, 0, depthmapWidth, depthmapHeight, 0, 0, depthmapWidth, depthmapHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);


	/*glBindFramebuffer(GL_FRAMEBUFFER, fboCIrradiance);
	glReadPixels(0, 0, depthmapWidth, depthmapHeight, GL_BGR, GL_UNSIGNED_BYTE, irradianceData);
	Bitmap::saveBitmap24("irradiance.bmp", irradianceData, depthmapWidth, depthmapHeight);*/
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}


//debug purpose
void Depthmap::createBuffer(){

	m_vertex.push_back(1.0 * m_size); m_vertex.push_back(1.0 * m_size); m_vertex.push_back(-3.0 * m_size); m_vertex.push_back(1.0); m_vertex.push_back(1.0);
	m_vertex.push_back(-1.0 * m_size); m_vertex.push_back(1.0 * m_size); m_vertex.push_back(-3.0 * m_size); m_vertex.push_back(0.0); m_vertex.push_back(1.0);
	m_vertex.push_back(-1.0 * m_size); m_vertex.push_back(-1.0 * m_size); m_vertex.push_back(-3.0 * m_size); m_vertex.push_back(0.0); m_vertex.push_back(0.0);	
	m_vertex.push_back(1.0 * m_size); m_vertex.push_back(-1.0 * m_size); m_vertex.push_back(-3.0 * m_size); m_vertex.push_back(1.0); m_vertex.push_back(0.0);
	

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


void Depthmap::render(unsigned int texture) {



	glUseProgram(m_renderShader->m_program);
	m_renderShader->loadMatrix("u_modelView", m_camera->getViewMatrix());

	glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
	m_renderShader->bindAttributes(texture);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexQuad);
	glDrawElements(GL_TRIANGLES, 2 * 3, GL_UNSIGNED_SHORT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	m_renderShader->unbindAttributes();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);

}

