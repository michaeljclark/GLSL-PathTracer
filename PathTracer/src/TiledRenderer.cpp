#include <TiledRenderer.h>

void TiledRenderer::init()
{
	renderCompleted = false;

	sampleCounter = new float*[numTilesX];
	for (int i = 0; i < numTilesX; i++)
		sampleCounter[i] = new float[numTilesY];

	for (int j = 0; j < numTilesY; j++)
		for (int i = 0; i < numTilesX; i++)
			sampleCounter[i][j] = 0;

	tileWidth  = (int)screenSize.x / numTilesX;
	tileHeight = (int)screenSize.y / numTilesY;
	tileX = 0;
	tileY = numTilesY - 1;

	//----------------------------------------------------------
	// Shaders
	//----------------------------------------------------------
	pathTraceShader  = loadShaders("src/shaders/Tiled/PathTraceVert.glsl", "src/shaders/Tiled/PathTraceFrag.glsl");
	accumShader      = loadShaders("src/shaders/Tiled/AccumVert.glsl", "src/shaders/Tiled/AccumFrag.glsl");
	tileOutputShader = loadShaders("src/shaders/Tiled/TileOutputVert.glsl", "src/shaders/Tiled/TileOutputFrag.glsl");
	outputShader     = loadShaders("src/shaders/Tiled/OutputVert.glsl", "src/shaders/Tiled/OutputFrag.glsl");

	//----------------------------------------------------------
	// FBO Setup
	//----------------------------------------------------------
	//Create FBOs for path trace shader
	glGenFramebuffers(1, &pathTraceFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBO);

	//Create Texture for FBO
	glGenTextures(1, &pathTraceTexture);
	glBindTexture(GL_TEXTURE_2D, pathTraceTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, tileWidth, tileHeight, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTraceTexture, 0);

	//Create FBOs for screen shader
	glGenFramebuffers(1, &accumFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);

	//Create Texture for FBO
	glGenTextures(1, &accumTexture);
	glBindTexture(GL_TEXTURE_2D, accumTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenSize.x, screenSize.y, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTexture, 0);

	//Create FBOs for output shader
	glGenFramebuffers(1, &outputFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);

	//Create Texture for FBO
	glGenTextures(1, &tileOutputTexture);
	glBindTexture(GL_TEXTURE_2D, tileOutputTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenSize.x, screenSize.y, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tileOutputTexture, 0);

	GLuint shaderObject;

	pathTraceShader->use();
	shaderObject = pathTraceShader->object();

	glUniform3fv(glGetUniformLocation(shaderObject, "camera.position"), 1, glm::value_ptr(scene->camera->position));
	glUniform3fv(glGetUniformLocation(shaderObject, "camera.right"), 1, glm::value_ptr(scene->camera->right));
	glUniform3fv(glGetUniformLocation(shaderObject, "camera.up"), 1, glm::value_ptr(scene->camera->up));
	glUniform3fv(glGetUniformLocation(shaderObject, "camera.forward"), 1, glm::value_ptr(scene->camera->forward));
	glUniform1f(glGetUniformLocation(shaderObject, "camera.fov"), scene->camera->fov);
	glUniform1f(glGetUniformLocation(shaderObject, "camera.focalDist"), scene->camera->focalDist);
	glUniform1f(glGetUniformLocation(shaderObject, "camera.aperture"), scene->camera->aperture);
	
	glUniform1i(glGetUniformLocation(shaderObject,  "maxDepth"), maxDepth);
	glUniform2fv(glGetUniformLocation(shaderObject, "screenResolution"), 1, glm::value_ptr(screenSize));
	glUniform1i(glGetUniformLocation(shaderObject, "numOfLights"), numOfLights);
	glUniform1f(glGetUniformLocation(shaderObject, "invTileWidth"), 1.0f / numTilesX);
	glUniform1f(glGetUniformLocation(shaderObject, "invTileHeight"), 1.0f / numTilesY);

	glUniform1i(glGetUniformLocation(shaderObject, "accumTexture"), 0);
	glUniform1i(glGetUniformLocation(shaderObject, "BVH"), 1);
	glUniform1i(glGetUniformLocation(shaderObject, "triangleIndicesTex"), 2);
	glUniform1i(glGetUniformLocation(shaderObject, "verticesTex"), 3);
	glUniform1i(glGetUniformLocation(shaderObject, "normalsTexCoordsTex"), 4);
	glUniform1i(glGetUniformLocation(shaderObject, "materialsTex"), 5);
	glUniform1i(glGetUniformLocation(shaderObject, "lightsTex"), 6);
	glUniform1i(glGetUniformLocation(shaderObject, "albedoTextures"), 7);
	glUniform1i(glGetUniformLocation(shaderObject, "metallicTextures"), 8);
	glUniform1i(glGetUniformLocation(shaderObject, "roughnessTextures"), 9);
	glUniform1i(glGetUniformLocation(shaderObject, "normalTextures"), 10);

	pathTraceShader->stopUsing();

}

void TiledRenderer::render()
{
	if (!renderCompleted)
	{
		GLuint shaderObject;
		pathTraceShader->use();

		shaderObject = pathTraceShader->object();
		glUniform1i(glGetUniformLocation(shaderObject, "tileX"), tileX);
		glUniform1i(glGetUniformLocation(shaderObject, "tileY"), tileY);
		pathTraceShader->stopUsing();

		glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBO);
		glViewport(0, 0, tileWidth, tileHeight);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, accumTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER, BVHTexture);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_BUFFER, triangleIndicesTexture);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_BUFFER, verticesTexture);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_BUFFER, normalsTexCoordsTexture);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_BUFFER, materialsTexture);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_BUFFER, lightsTexture);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D_ARRAY, albedoTextures);
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D_ARRAY, metallicTextures);
		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_2D_ARRAY, roughnessTextures);
		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D_ARRAY, normalTextures);

		quad->Draw(pathTraceShader);

		glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);
		glViewport(tileWidth * tileX, tileHeight * tileY, tileWidth, tileHeight);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pathTraceTexture);

		quad->Draw(accumShader);

		glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
		glViewport(tileWidth * tileX, tileHeight * tileY, tileWidth, tileHeight);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pathTraceTexture);

		quad->Draw(outputShader);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screenSize.x, screenSize.y);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tileOutputTexture);
	quad->Draw(tileOutputShader);

	if (sampleCounter[tileX][tileY] > maxSamples)
	{
		tileX++;
		if (tileX >= numTilesX)
		{
			tileX = 0;
			tileY--;
			if (tileY < 0)
				renderCompleted = true;
		}
	}
}

void TiledRenderer::update(float secondsElapsed)
{
	if (!renderCompleted)
	{
		sampleCounter[tileX][tileY] += 1;

		float r1 = ((float)rand() / (RAND_MAX));
		float r2 = ((float)rand() / (RAND_MAX));
		float r3 = ((float)rand() / (RAND_MAX));

		GLuint shaderObject;

		pathTraceShader->use();
		shaderObject = pathTraceShader->object();
		glUniform3fv(glGetUniformLocation(shaderObject, "randomVector"), 1, glm::value_ptr(glm::vec3(r1, r2, r3)));
		glUniform1i(glGetUniformLocation(shaderObject, "tileX"), tileX);
		glUniform1i(glGetUniformLocation(shaderObject, "tileY"), tileY);
		pathTraceShader->stopUsing();

		outputShader->use();
		shaderObject = outputShader->object();
		glUniform1f(glGetUniformLocation(shaderObject, "invSampleCounter"), 1.0f / sampleCounter[tileX][tileY]);
		outputShader->stopUsing();

		tileOutputShader->use();
		shaderObject = tileOutputShader->object();
		glUniform1f(glGetUniformLocation(shaderObject, "invSampleCounter"), 1.0f);
		tileOutputShader->stopUsing();
	}
}