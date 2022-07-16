// This has been adapted from the Vulkan tutorial

#include "MyProject.hpp"

const std::string TRAY_MODEL_PATH = "models/tray.obj";
const std::vector<std::string> PIECES_MODEL_PATHS = { 
	"models/piece1.obj", 
	"models/piece2.obj",
	"models/piece3.obj",
	"models/piece4.obj",
	"models/piece5.obj",
	"models/piece6.obj",
	"models/piece7.obj"
};

const std::string TRAY_TEXTURE_PATH = "textures/texture-background.jpg";
const std::string PIECES_TEXTURE_PATH = "textures/faded-gray-wooden-textured-background.jpg";


const std::vector<Vertex> planeVertices = {
	{
		glm::vec3(-1, 0, -1),
		glm::vec3(0, 1, 0),
		glm::vec2(0, 0)
	},
	{
		glm::vec3(1, 0, -1),
		glm::vec3(0, 1, 0),
		glm::vec2(1, 0)
	},
	{
		glm::vec3(1, 0, 1),
		glm::vec3(0, 1, 0),
		glm::vec2(1, 1)
	},
	{
		glm::vec3(-1, 0, 1),
		glm::vec3(0, 1, 0),
		glm::vec2(0, 1)
	}
};
const std::vector<uint32_t> planeIndices = {0, 2, 1, 0, 3, 2};


const float PIECES_BASE_Y = 1.0f;
const float PIECES_ELEVATED_Y = 3.0f;


// function to make a look in view matrix starting from the camera position and it's rotation (angles in radiants)
// cameraPos: coordinates of the camera
// alpha: angle of the camera with respect to the y axis (horizontal looking direction, yaw)
// beta: angle of the camera with respect to the x axis (vertical elevation, pitch)
// rho: angle of the camera with respect to the z axis (inclination, roll)
glm::mat4 lookIn(glm::vec3 cameraPos, float alpha, float beta, float rho) {
	return glm::rotate(glm::mat4(1.0), -rho, glm::vec3(0, 0, 1))
		* glm::rotate(glm::mat4(1.0), -beta, glm::vec3(1, 0, 0))
		* glm::rotate(glm::mat4(1.0), -alpha, glm::vec3(0, 1, 0))
		* glm::translate(glm::mat4(1.0), -cameraPos);
}

// function to make a world matrix starting from the object position, rotation (angles in radiants) and scale
// pos: coordinates of the object
// YPR: yaw(x), pitch(y) and roll(z) of the object
// size: scaling factors for the object
glm::mat4 MakeWorldMatrixEuler(glm::vec3 pos, glm::vec3 YPR, glm::vec3 size) {
	glm::mat4 out;
	glm::mat4 Pos = glm::translate(glm::mat4(1.0), glm::vec3(pos.x, pos.y, pos.z));
	glm::mat4 RotYaw = glm::rotate(glm::mat4(1.0), glm::radians(YPR.x), glm::vec3(0, 1, 0));
	glm::mat4 RotPitch = glm::rotate(glm::mat4(1.0), glm::radians(YPR.y), glm::vec3(1, 0, 0));
	glm::mat4 RotRoll = glm::rotate(glm::mat4(1.0), glm::radians(YPR.z), glm::vec3(0, 0, 1));
	glm::mat4 Sca = glm::scale(glm::mat4(1.0), glm::vec3(size.x, size.y, size.z));
	out = Pos * RotYaw * RotPitch * RotRoll * Sca;
	return out;
}



// The uniform buffer object that will be fed to the pipline
struct globalUniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 lightPos;
	alignas(16) glm::vec2 paramDecay;
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::vec4 color;
	alignas(4) float selected;
};



// class containing the informations for each model
class ModelInfo {
protected:
	std::string path;
	Model model;

public:
	DescriptorSet DS;
	glm::vec3 position;
	glm::vec3 eulerRotation;
	glm::vec3 scale;
	glm::vec4 color;

	ModelInfo() {
		position = glm::vec3();
		eulerRotation = glm::vec3();
		scale = glm::vec3();
		color = glm::vec4();
	}

	ModelInfo(BaseProject* pj, std::string path) {
		this->path = path;
		model.init(pj, path);

		position = glm::vec3(0.0f, 1.0f, -1.0f);
		eulerRotation = glm::vec3(0.0f, 0.0f, 0.0f);
		scale = glm::vec3(1.0f, 1.0f, 1.0f);
		color = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	}

	ModelInfo(BaseProject* pj, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
		model.BP = pj;
		model.vertices = vertices;
		model.indices = indices;
		model.createVertexBuffer();
		model.createIndexBuffer();

		position = glm::vec3(0.0f, 1.0f, -1.0f);
		eulerRotation = glm::vec3(0.0f, 0.0f, 0.0f);
		scale = glm::vec3(1.0f, 1.0f, 1.0f);
		color = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	}

	const Model& getModel() {
		return model;
	}

	const void drawModel(Pipeline P, VkCommandBuffer commandBuffer, int currentImage){
		VkBuffer vertexBuffers[] = { model.vertexBuffer };
		// property .vertexBuffer of models, contains the VkBuffer handle to its vertex buffer
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		// property .indexBuffer of models, contains the VkBuffer handle to its index buffer
		vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer, 0,
			VK_INDEX_TYPE_UINT32);

		// property .pipelineLayout of a pipeline contains its layout.
		// property .descriptorSets of a descriptor set contains its elements.
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P.pipelineLayout, 1, 1, &(DS.descriptorSets[currentImage]),
			0, nullptr);

		// property .indices.size() of models, contains the number of triangles * 3 of the mesh.
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
	}

	const void updateUBO(VkDevice device, uint32_t currentImage) {
		UniformBufferObject ubo;

		ubo.model = glm::scale(MakeWorldMatrixEuler(position, eulerRotation, scale), glm::vec3(1.0, 1.0, 1.0));
		ubo.color = color;
		ubo.selected = 0.0f;

		void* data;

		vkMapMemory(device, DS.uniformBuffersMemory[0][currentImage], 0,
			sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, DS.uniformBuffersMemory[0][currentImage]);
	}

	void cleanup() {
		DS.cleanup();
		model.cleanup();
	}

};

class PieceModelInfo : public ModelInfo {
protected:

public:
	bool selected;
	DescriptorSet previewDS;

	PieceModelInfo() : ModelInfo() {
		selected = false;
	}

	PieceModelInfo(BaseProject* pj, std::string path) : ModelInfo(pj, path) {
		selected = false;
	}

	PieceModelInfo(BaseProject* pj, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) :
		ModelInfo(pj, vertices, indices) {
		selected = false;
	}

	const void drawModel(Pipeline P, VkCommandBuffer commandBuffer, int currentImage) {
		ModelInfo::drawModel(P, commandBuffer, currentImage);

		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P.pipelineLayout, 1, 1, &(previewDS.descriptorSets[currentImage]),
			0, nullptr);

		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
	}

	const void updateUBO(VkDevice device, uint32_t currentImage) {
		UniformBufferObject ubo;

		ubo.model = glm::scale(MakeWorldMatrixEuler(position, eulerRotation, scale), glm::vec3(1.0, 1.0, 1.0));
		ubo.color = color;
		ubo.selected = selected ? 1.0f : 0.0f;

		void* data;

		vkMapMemory(device, DS.uniformBuffersMemory[0][currentImage], 0,
			sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, DS.uniformBuffersMemory[0][currentImage]);
	}

	const void updatePreviewUBO(VkDevice device, uint32_t currentImage, bool visible) {
		UniformBufferObject ubo;

		glm::vec3 pos = glm::vec3(position.x, PIECES_BASE_Y, position.z);

		ubo.model = glm::scale(MakeWorldMatrixEuler(pos, eulerRotation, scale), glm::vec3(1.0, 1.0, 1.0));
		ubo.color = color;
		ubo.color.a *= selected && visible ? 0.5f : 0.0f;
		ubo.selected = 0.0f;

		void* data;

		vkMapMemory(device, previewDS.uniformBuffersMemory[0][currentImage], 0,
			sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, previewDS.uniformBuffersMemory[0][currentImage]);
	}

	void cleanup() {
		ModelInfo::cleanup();
		previewDS.cleanup();
	}
};

enum class SelectionState {
	SELECTION_MODE,
	TRANSITION,
	TRANSLATION_MODE
};

// MAIN CLASS
class MyProject : public BaseProject {
	private:
	int	selectedPieceIndex = 0;
	float selectedModelTargetY = 1.0f;

	SelectionState selectionMode = SelectionState::SELECTION_MODE;
	SelectionState nextSelectionMode = SelectionState::SELECTION_MODE;

	
	protected:
	// Here you list all the Vulkan objects you need:
	
	// Descriptor Layouts [what will be passed to the shaders]
		DescriptorSetLayout DSLglobal;
		DescriptorSetLayout DSLobj;

	// Pipelines [Shader couples]
	Pipeline P1;

	// Models, textures and Descriptors (values assigned to the uniforms)
	ModelInfo trayModelInfo;
	std::vector <PieceModelInfo> piecesModelInfo = {};
	ModelInfo backgroundModelInfo;

	Texture trayTexture;
	Texture pieceTexture;
	DescriptorSet globalDS;



	//DEFAULT FUNCTIONS
	
	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Tangram";
		initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};
		
		// Descriptor pool sizes
		uniformBlocksInPool = 1 + 1 + 2 * PIECES_MODEL_PATHS.size() + 1; //global + 1 per model and 2 for pieces (tray, pieces, background)
		texturesInPool = 2;
		setsInPool = 1 + 1 + 2 * PIECES_MODEL_PATHS.size() + 1;
	}

	// Here you load and setup all your Vulkan objects
	void localInit() {
		// Descriptor Layouts [what will be passed to the shaders]
		DSLglobal.init(this, {
					// this array contains the binding:
					// first  element : the binding number
					// second element : the time of element (buffer or texture)
					// third  element : the pipeline stage where it will be used
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
					// {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
				  });

		DSLobj.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			});

		// Pipelines [Shader couples]
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
		P1.init(this, "shaders/vert.spv", "shaders/frag.spv", { &DSLglobal, &DSLobj });

		// Models, textures and Descriptors (values assigned to the uniforms)
		trayTexture.init(this, TRAY_TEXTURE_PATH);
		pieceTexture.init(this, PIECES_TEXTURE_PATH);

		trayModelInfo = ModelInfo(this, TRAY_MODEL_PATH);
		trayModelInfo.DS.init(this, &DSLobj, { {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
								{1, TEXTURE, 0, &trayTexture} });

		for (std::string path : PIECES_MODEL_PATHS)
		{
			PieceModelInfo mi = PieceModelInfo(this, path);
			mi.DS.init(this, &DSLobj, { {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
									{1, TEXTURE, 0, &pieceTexture} });
			mi.previewDS.init(this, &DSLobj, { {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
									{1, TEXTURE, 0, &pieceTexture} });
			piecesModelInfo.push_back(mi);
		}

		//background plane initialization
		backgroundModelInfo = ModelInfo(this, planeVertices, planeIndices);
		backgroundModelInfo.DS.init(this, &DSLobj, { {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
									{1, TEXTURE, 0, &pieceTexture} });
		backgroundModelInfo.position = glm::vec3(0.0f, -1.0f, 0.0f);
		backgroundModelInfo.color = glm::vec4(0.5f, 0.2f, 1.0f, 1.0f);
		backgroundModelInfo.scale = glm::vec3(15.0f, 15.0f, 15.0f);


		//container position and color initialization
		trayModelInfo.position = glm::vec3(0.0f, 0.0f, 0.0f);
		trayModelInfo.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		//pieces position and color initialization
		piecesModelInfo[0].position = glm::vec3(-3.0f, PIECES_BASE_Y, 0.0f);
		piecesModelInfo[1].position = glm::vec3(-2.0f, PIECES_BASE_Y, 0.0f);
		piecesModelInfo[2].position = glm::vec3(-1.0f, PIECES_BASE_Y, 0.0f);
		piecesModelInfo[3].position = glm::vec3(0.0f, PIECES_BASE_Y, 0.0f);
		piecesModelInfo[4].position = glm::vec3(1.0f, PIECES_BASE_Y, 0.0f);
		piecesModelInfo[5].position = glm::vec3(2.0f, PIECES_BASE_Y, 0.0f);
		piecesModelInfo[6].position = glm::vec3(3.0f, PIECES_BASE_Y, 0.0f);

		piecesModelInfo[0].color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		piecesModelInfo[1].color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		piecesModelInfo[2].color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
		piecesModelInfo[3].color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
		piecesModelInfo[4].color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
		piecesModelInfo[5].color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
		piecesModelInfo[6].color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		selectPiece(3);


		// T1.init(this, TEXTURE_PATH);
		globalDS.init(this, &DSLglobal, {
		// the second parameter, is a pointer to the Uniform Set Layout of this set
		// the last parameter is an array, with one element per binding of the set.
		// first  elmenet : the binding number
		// second element : UNIFORM or TEXTURE (an enum) depending on the type
		// third  element : only for UNIFORMs, the size of the corresponding C++ object
		// fourth element : only for TEXTUREs, the pointer to the corresponding texture object
					{0, UNIFORM, sizeof(globalUniformBufferObject), nullptr}
				});

		glfwSetWindowUserPointer(window, this);
		glfwSetKeyCallback(window, key_callback);
	}

	// Here you destroy all the objects you created!		
	void localCleanup() {
		trayTexture.cleanup();
		pieceTexture.cleanup();
		globalDS.cleanup();

		trayModelInfo.cleanup();
		for (PieceModelInfo mi : piecesModelInfo)
		{
			mi.cleanup(); //cleans both model and DS
		}
		backgroundModelInfo.cleanup();

		P1.cleanup();
		DSLglobal.cleanup();
		DSLobj.cleanup();
	}
	
	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
				
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				P1.graphicsPipeline);
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 0, 1, &globalDS.descriptorSets[currentImage],
			0, nullptr);
		
		trayModelInfo.drawModel(P1, commandBuffer, currentImage);
		for (PieceModelInfo mi : piecesModelInfo)
		{
			mi.drawModel(P1, commandBuffer, currentImage);
		}
		backgroundModelInfo.drawModel(P1, commandBuffer, currentImage);
	}

	// std::vector<glm::vec3> ObjsPos;
	// std::vector<glm::vec3> ObjsEuler;
	// std::vector<glm::vec3> ObjsSize;

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		static auto lastTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - lastTime).count();
		lastTime = currentTime;

		if (selectionMode == SelectionState::TRANSLATION_MODE) updateSelectedModelPosition(dt);
		if (selectionMode == SelectionState::TRANSITION) selectedModelTransition(dt);

		trayModelInfo.updateUBO(device, currentImage);
		for (PieceModelInfo mi : piecesModelInfo) {
			mi.updateUBO(device, currentImage);
			mi.updatePreviewUBO(device, currentImage, selectionMode == SelectionState::TRANSLATION_MODE || selectionMode == SelectionState::TRANSITION);
		}
		backgroundModelInfo.updateUBO(device, currentImage);

		void* data;
		globalUniformBufferObject gubo{};
		gubo.view = computeNewViewMatrix(dt);
		gubo.proj = glm::perspective(glm::radians(45.0f),
			swapChainExtent.width / (float)swapChainExtent.height,
			0.1f, 10.0f);
		gubo.proj[1][1] *= -1;
		gubo.lightPos = glm::vec3(0.0f, 8.0f, 0.0f);
		gubo.paramDecay = glm::vec2(2.0f, 0.2f);
		
		vkMapMemory(device, globalDS.uniformBuffersMemory[0][currentImage], 0,
			sizeof(gubo), 0, &data);
		memcpy(data, &gubo, sizeof(gubo));
		vkUnmapMemory(device, globalDS.uniformBuffersMemory[0][currentImage]);
	}



	//FUNCTION DEFINITION
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		static std::vector<int> modelToSkipIndexes = {};

		MyProject* that = static_cast<MyProject*>(glfwGetWindowUserPointer(window));

		if (that->selectionMode == SelectionState::TRANSITION) return;

		if (that->selectionMode == SelectionState::TRANSLATION_MODE) {
			if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
				that->setSelectionMode(true);
			}
			return;
		}

		float angleMin;
		float angleMax;
		if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
			angleMin = glm::radians(-45.0f);
			angleMax = glm::radians(45.0f);
		}
		else if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
			angleMin = glm::radians(45.0f);
			angleMax = glm::radians(135.0f);
		}
		else if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
			angleMin = glm::radians(135.0f);
			angleMax = glm::radians(-135.0f);
		}
		else if (key == GLFW_KEY_W && action == GLFW_RELEASE) {
			angleMin = glm::radians(-135.0f);
			angleMax = glm::radians(-45.0f);
		}
		else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
			that->setSelectionMode(false);
			return;
		}
		else {
			return;
		}

		int selectedPieceIndex = that->selectedPieceIndex;
		std::vector<PieceModelInfo> piecesModelInfo = that->piecesModelInfo;

		int newSelectedModelIndex = selectedPieceIndex;
		std::optional<float> minDistance = std::nullopt;

		glm::vec3 selectedModelPosition = piecesModelInfo[selectedPieceIndex].position;

		for (int i = 0; i < piecesModelInfo.size(); i++) {
			PieceModelInfo model = piecesModelInfo[i];

			float angle = atan2(model.position.z - selectedModelPosition.z, model.position.x - selectedModelPosition.x);

			bool isInCorrectDirection;
			if (angleMin <= angleMax)
				isInCorrectDirection = angleMin <= angle && angle <= angleMax;
			else
				isInCorrectDirection = (angleMin <= angle && angle <= glm::radians(180.0f)) || (glm::radians(-180.0f) <= angle && angle <= angleMax);

			if (i != selectedPieceIndex && isInCorrectDirection && std::count(modelToSkipIndexes.begin(), modelToSkipIndexes.end(), i)==0) {
				// std::cerr << "hit " << i << std::endl;
				float squaredDistance = (model.position.x - selectedModelPosition.x) * (model.position.x - selectedModelPosition.x) + (model.position.z - selectedModelPosition.z) * (model.position.z - selectedModelPosition.z);
				if (!minDistance.has_value() || squaredDistance < minDistance) {
					minDistance = squaredDistance;
					newSelectedModelIndex = i;
				}
			}
		}

		if (minDistance.value_or(-1) == 0) {
			modelToSkipIndexes.push_back(selectedPieceIndex);
		}
		else {
			modelToSkipIndexes.clear();
		}

		// std::cerr << modelToSkipIndexes.size() << std::endl;

		that->selectPiece(newSelectedModelIndex);
	}

	void selectPiece(int index) {
		piecesModelInfo[selectedPieceIndex].selected = false;
		piecesModelInfo[index].selected = true;
		selectedModelTargetY = piecesModelInfo[index].position.y;
		// std::cerr << index;
		selectedPieceIndex = index;
	}

	void setSelectionMode(bool isSelectionMode) {
		selectionMode = SelectionState::TRANSITION;
		if (isSelectionMode) {
			selectedModelTargetY = PIECES_BASE_Y;
			nextSelectionMode = SelectionState::SELECTION_MODE;
		}
		else {
			selectedModelTargetY = PIECES_ELEVATED_Y;
			nextSelectionMode = SelectionState::TRANSLATION_MODE;
		}
	}

	glm::mat4 computeNewViewMatrix(float deltaTime) {
		static glm::mat4 viewMatrix = lookIn(glm::vec3(0.0f, 8.0f, 0.0f), glm::radians(0.0f), glm::radians(-90.0f), glm::radians(0.0f)); // camera starts looking down

		static float linearSpeed = 1.0f;
		// static float angularSpeed = 20;

		glm::vec3 mov = glm::vec3(0, 0, 0);
		// glm::vec3 rot = glm::vec3(0, 0, 0);

		// camera movements controlls
		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			mov.x -= 1;
		}

		if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			mov.x += 1;
		}

		if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			mov.y -= 1;
		}

		if (glfwGetKey(window, GLFW_KEY_UP)) {
			mov.y += 1;
		}

		if (glfwGetKey(window, GLFW_KEY_Z)) {
			mov.z -= 1;
		}

		if (glfwGetKey(window, GLFW_KEY_X)) {
			mov.z += 1;
		}

		viewMatrix = glm::translate(glm::mat4(1.0), linearSpeed * deltaTime * (-mov))
			// * glm::rotate(glm::mat4(1.0), angularSpeed * deltaTime * rot.z, glm::vec3(0, 0, 1))
			// * glm::rotate(glm::mat4(1.0), angularSpeed * deltaTime * rot.y, glm::vec3(0, 1, 0))
			// * glm::rotate(glm::mat4(1.0), angularSpeed * deltaTime * rot.x, glm::vec3(1, 0, 0))
			* viewMatrix;

		return viewMatrix;
	}

	void updateSelectedModelPosition(float deltaTime) {
		static float linearSpeed = 1.0f;
		static float angularSpeed = 50;

		glm::vec3 mov = glm::vec3(0, 0, 0);
		int rot = 0;

		if (glfwGetKey(window, GLFW_KEY_A)) {
			mov.x += 1;
		}

		if (glfwGetKey(window, GLFW_KEY_D)) {
			mov.x -= 1;
		}

		if (glfwGetKey(window, GLFW_KEY_S)) {
			mov.z -= 1;
		}

		if (glfwGetKey(window, GLFW_KEY_W)) {
			mov.z += 1;
		}

		if (glfwGetKey(window, GLFW_KEY_E)) {
			rot -= 1;
		}

		if (glfwGetKey(window, GLFW_KEY_Q)) {
			rot += 1;
		}

		piecesModelInfo[selectedPieceIndex].position += linearSpeed * deltaTime * (-mov);
		piecesModelInfo[selectedPieceIndex].eulerRotation.x += angularSpeed * deltaTime * rot;
	}

	void selectedModelTransition(float deltaTime) {
		static float linearSpeed = 4.0f;

		if (selectedModelTargetY > piecesModelInfo[selectedPieceIndex].position.y) {
			piecesModelInfo[selectedPieceIndex].position.y += linearSpeed * deltaTime;
			if (piecesModelInfo[selectedPieceIndex].position.y >= selectedModelTargetY) {
				piecesModelInfo[selectedPieceIndex].position.y = selectedModelTargetY;
				selectionMode = nextSelectionMode;
			}
		}
		else {
			piecesModelInfo[selectedPieceIndex].position.y -= linearSpeed * deltaTime;
			if (piecesModelInfo[selectedPieceIndex].position.y <= selectedModelTargetY) {
				piecesModelInfo[selectedPieceIndex].position.y = selectedModelTargetY;
				selectionMode = nextSelectionMode;
			}
		}
	}
};


//THINGS MODIDIED ON MyProject.hpp
//1759: modified color blending to include alpha blending


// This is the main: probably you do not need to touch this!
int main() {
    MyProject app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}