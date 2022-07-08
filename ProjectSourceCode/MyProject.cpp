// This has been adapted from the Vulkan tutorial

#include "MyProject.hpp"

const std::vector<std::string> MODEL_PATHS = { 
	"models/tray.obj",
	"models/piece1.obj", 
	"models/piece2.obj",
	"models/piece3.obj",
	"models/piece4.obj",
	"models/piece5.obj",
	"models/piece6.obj",
	"models/piece7.obj"
};
const std::string TEXTURE_PATH = "textures/wood_texture.jpg";



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
private:
	std::string path;
	Model model;

public:
	DescriptorSet DS;
	bool selected;
	glm::vec3 position;
	glm::vec3 eulerRotation;
	glm::vec3 scale;
	glm::vec4 color;

	ModelInfo(BaseProject* pj, std::string path) {
		this->path = path;
		model.init(pj, path);

		position = glm::vec3(0.0f, 1.0f, -1.0f);
		eulerRotation = glm::vec3(0.0f, 0.0f, 0.0f);
		scale = glm::vec3(1.0f, 1.0f, 1.0f);
		color = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		selected = false;
	}

	const Model& getModel() {
		return model;
	}

	void cleanup() {
		DS.cleanup();
		model.cleanup();
	}
};

// MAIN CLASS
class MyProject : public BaseProject {
	private:
	int	selectedModelIndex = 0;
	bool selectionMode = true;

	protected:
	// Here you list all the Vulkan objects you need:
	
	// Descriptor Layouts [what will be passed to the shaders]
		DescriptorSetLayout DSLglobal;
		DescriptorSetLayout DSLobj;

	// Pipelines [Shader couples]
	Pipeline P1;

	// Models, textures and Descriptors (values assigned to the uniforms)
	std::vector <ModelInfo> modelInfos = {};
	Texture T1;
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
		uniformBlocksInPool = 1 + MODEL_PATHS.size();
		texturesInPool = 1;
		setsInPool = 1 + MODEL_PATHS.size();
	}

	// Here you load and setup all your Vulkan objects
	void localInit() {
		// Descriptor Layouts [what will be passed to the shaders]
		DSLglobal.init(this, {
					// this array contains the binding:
					// first  element : the binding number
					// second element : the time of element (buffer or texture)
					// third  element : the pipeline stage where it will be used
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
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
		for(std::string path : MODEL_PATHS)
		{
			ModelInfo mi = ModelInfo(this, path);
			T1.init(this, TEXTURE_PATH);
			mi.DS.init(this, &DSLobj, { {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
										{1, TEXTURE, 0, &T1} });
			modelInfos.push_back(mi);
		}

		modelInfos[0].position = glm::vec3(0.0f, 0.0f, 0.0f);
		modelInfos[1].position = glm::vec3(-3.0f, 1.0f, 0.0f);
		modelInfos[2].position = glm::vec3(-2.0f, 1.0f, 0.0f);
		modelInfos[3].position = glm::vec3(-1.0f, 1.0f, 0.0f);
		modelInfos[4].position = glm::vec3(0.0f, 1.0f, 0.0f);
		modelInfos[5].position = glm::vec3(1.0f, 1.0f, 0.0f);
		modelInfos[6].position = glm::vec3(2.0f, 1.0f, 0.0f);
		modelInfos[7].position = glm::vec3(3.0f, 1.0f, 0.0f);

		selectModel(4);


		// T1.init(this, TEXTURE_PATH);
		globalDS.init(this, &DSLglobal, {
		// the second parameter, is a pointer to the Uniform Set Layout of this set
		// the last parameter is an array, with one element per binding of the set.
		// first  elmenet : the binding number
		// second element : UNIFORM or TEXTURE (an enum) depending on the type
		// third  element : only for UNIFORMs, the size of the corresponding C++ object
		// fourth element : only for TEXTUREs, the pointer to the corresponding texture object
					{0, UNIFORM, sizeof(globalUniformBufferObject), nullptr},
					{1, TEXTURE, 0, &T1}
				});

		glfwSetWindowUserPointer(window, this);
		glfwSetKeyCallback(window, key_callback);
	}

	// Here you destroy all the objects you created!		
	void localCleanup() {
		T1.cleanup();
		globalDS.cleanup();
		// T1.cleanup();
		for (ModelInfo mi : modelInfos)
		{
			mi.cleanup(); //cleans both model and DS
		}
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
		
		for (ModelInfo mi : modelInfos)
		{
			Model M = mi.getModel();
			VkBuffer vertexBuffers[] = { M.vertexBuffer };
			// property .vertexBuffer of models, contains the VkBuffer handle to its vertex buffer
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			// property .indexBuffer of models, contains the VkBuffer handle to its index buffer
			vkCmdBindIndexBuffer(commandBuffer, M.indexBuffer, 0,
				VK_INDEX_TYPE_UINT32);

			// property .pipelineLayout of a pipeline contains its layout.
			// property .descriptorSets of a descriptor set contains its elements.
			vkCmdBindDescriptorSets(commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				P1.pipelineLayout, 1, 1, &mi.DS.descriptorSets[currentImage],
				0, nullptr);

			// property .indices.size() of models, contains the number of triangles * 3 of the mesh.
			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(M.indices.size()), 1, 0, 0, 0);
		}
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

		void* data;

		UniformBufferObject ubo{};

		if (!selectionMode) updateSelectedModelPosition(dt);

		for (ModelInfo mi : modelInfos) {
			
			ubo.model = glm::scale(MakeWorldMatrixEuler(mi.position, mi.eulerRotation, mi.scale), glm::vec3(1.0, 1.0, 1.0));
			ubo.color = mi.color;
			ubo.selected = mi.selected ? 1.0f : 0.0f;

			// Here is where you actually update your uniforms

			//for (ModelInfo mi : modelInfos)
			//{

			vkMapMemory(device, mi.DS.uniformBuffersMemory[0][currentImage], 0,
				sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(device, mi.DS.uniformBuffersMemory[0][currentImage]);
			//}

		}

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

		if (!that->selectionMode) {
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

		int selectedModelIndex = that->selectedModelIndex;
		std::vector<ModelInfo> modelInfos = that->modelInfos;

		int newSelectedModelIndex = selectedModelIndex;
		std::optional<float> minDistance = std::nullopt;

		glm::vec3 selectedModelPosition = modelInfos[selectedModelIndex].position;

		for (int i = 1; i < modelInfos.size(); i++) {
			ModelInfo model = modelInfos[i];

			float angle = atan2(model.position.z - selectedModelPosition.z, model.position.x - selectedModelPosition.x);

			bool isInCorrectDirection;
			if (angleMin <= angleMax)
				isInCorrectDirection = angleMin <= angle && angle <= angleMax;
			else
				isInCorrectDirection = (angleMin <= angle && angle <= glm::radians(180.0f)) || (glm::radians(-180.0f) <= angle && angle <= angleMax);

			if (i != selectedModelIndex && isInCorrectDirection && std::count(modelToSkipIndexes.begin(), modelToSkipIndexes.end(), i)==0) {
				// std::cerr << "hit " << i << std::endl;
				float squaredDistance = (model.position.x - selectedModelPosition.x) * (model.position.x - selectedModelPosition.x) + (model.position.z - selectedModelPosition.z) * (model.position.z - selectedModelPosition.z);
				if (!minDistance.has_value() || squaredDistance < minDistance) {
					minDistance = squaredDistance;
					newSelectedModelIndex = i;
				}
			}
		}

		if (minDistance.value_or(-1) == 0) {
			modelToSkipIndexes.push_back(selectedModelIndex);
		}
		else {
			modelToSkipIndexes.clear();
		}

		std::cerr << modelToSkipIndexes.size() << std::endl;

		that->selectModel(newSelectedModelIndex);
	}

	void selectModel(int index) {
		modelInfos[selectedModelIndex].selected = false;
		modelInfos[index].selected = true;
		// std::cerr << index;
		selectedModelIndex = index;
	}

	void setSelectionMode(bool isSelectionMode) {
		selectionMode = isSelectionMode;
		if (selectionMode) {
			modelInfos[selectedModelIndex].position.y -= 2;
		}
		else {
			modelInfos[selectedModelIndex].position.y += 2;
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

		modelInfos[selectedModelIndex].position += linearSpeed * deltaTime * (-mov);
		modelInfos[selectedModelIndex].eulerRotation.x += angularSpeed * deltaTime * rot;
	}
};

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