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
// const std::string TEXTURE_PATH = "textures/viking_room.png";

// The uniform buffer object used in this example
struct globalUniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
};

// class containing the informations for each model
class ModelInfo {
private:
	std::string path;
	Model model;

public:
	DescriptorSet DS;

	ModelInfo(BaseProject* pj, std::string path) {
		this->path = path;
		model.init(pj, path);
	}

	const Model& getModel() {
		return model;
	}

	void cleanup() {
		DS.cleanup();
		model.cleanup();
	}
};

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

// MAIN ! 
class MyProject : public BaseProject {
	protected:
	// Here you list all the Vulkan objects you need:
	
	// Descriptor Layouts [what will be passed to the shaders]
		DescriptorSetLayout DSLglobal;
		DescriptorSetLayout DSLobj;

	// Pipelines [Shader couples]
	Pipeline P1;

	// Models, textures and Descriptors (values assigned to the uniforms)
	std::vector <ModelInfo> modelInfos = {};
	// Texture T1;
	DescriptorSet globalDS;
	
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
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
					// {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
				  });

		DSLobj.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			});

		// Pipelines [Shader couples]
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
		P1.init(this, "shaders/vert.spv", "shaders/frag.spv", { &DSLglobal, &DSLobj });

		// Models, textures and Descriptors (values assigned to the uniforms)
		for(std::string path : MODEL_PATHS)
		{
			ModelInfo mi = ModelInfo(this, path);
			mi.DS.init(this, &DSLobj, { {0, UNIFORM, sizeof(UniformBufferObject), nullptr} });
			modelInfos.push_back(mi);
		}
		// T1.init(this, TEXTURE_PATH);
		globalDS.init(this, &DSLglobal, {
		// the second parameter, is a pointer to the Uniform Set Layout of this set
		// the last parameter is an array, with one element per binding of the set.
		// first  elmenet : the binding number
		// second element : UNIFORM or TEXTURE (an enum) depending on the type
		// third  element : only for UNIFORMs, the size of the corresponding C++ object
		// fourth element : only for TEXTUREs, the pointer to the corresponding texture object
					{0, UNIFORM, sizeof(globalUniformBufferObject), nullptr},
					// {1, TEXTURE, 0, &T1}
				});
	}

	// Here you destroy all the objects you created!		
	void localCleanup() {
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

	glm::mat4 computeNewViewMatrix(float deltaTime) {
		static glm::mat4 viewMatrix = lookIn(glm::vec3(0.0f, 8.0f, 0.0f), glm::radians(0.0f), glm::radians(-90.0f), glm::radians(0.0f));

		static float linearSpeed = 1.0f;
		static float angularSpeed = glm::radians(30.0f);

		glm::vec3 mov = glm::vec3(0, 0, 0);
		glm::vec3 rot = glm::vec3(0, 0, 0);

		if (glfwGetKey(window, GLFW_KEY_A)) {
			mov.x -= 1;
		}

		if (glfwGetKey(window, GLFW_KEY_D)) {
			mov.x += 1;
		}

		if (glfwGetKey(window, GLFW_KEY_S)) {
			mov.y -= 1;
		}

		if (glfwGetKey(window, GLFW_KEY_W)) {
			mov.y += 1;
		}

		if (glfwGetKey(window, GLFW_KEY_E)) {
			mov.z -= 1;
		}

		if (glfwGetKey(window, GLFW_KEY_Q)) {
			mov.z += 1;
		}

		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			rot.y -= 1;
		}

		if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			rot.y += 1;
		}

		if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			rot.x += 1;
		}

		if (glfwGetKey(window, GLFW_KEY_UP)) {
			rot.x -= 1;
		}

		viewMatrix = glm::translate(glm::mat4(1.0), linearSpeed * deltaTime * (-mov))
			* glm::rotate(glm::mat4(1.0), angularSpeed * deltaTime * rot.z, glm::vec3(0, 0, 1))
			* glm::rotate(glm::mat4(1.0), angularSpeed * deltaTime * rot.y, glm::vec3(0, 1, 0))
			* glm::rotate(glm::mat4(1.0), angularSpeed * deltaTime * rot.x, glm::vec3(1, 0, 0))
			* viewMatrix;

		return viewMatrix;
	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		static auto lastTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - lastTime).count();
		lastTime = currentTime;


		globalUniformBufferObject gubo{};
		UniformBufferObject ubo{};

		ubo.model = glm::mat4(1.0f); /* glm::rotate(glm::mat4(1.0f),
								time * glm::radians(90.0f),
								glm::vec3(0.0f, 0.0f, 1.0f)); */
		gubo.view = computeNewViewMatrix(dt); //camera looking down at the start
		gubo.proj = glm::perspective(glm::radians(45.0f),
			swapChainExtent.width / (float)swapChainExtent.height,
			0.1f, 10.0f);
		gubo.proj[1][1] *= -1;

		void* data;

		// Here is where you actually update your uniforms
		vkMapMemory(device, globalDS.uniformBuffersMemory[0][currentImage], 0,
			sizeof(gubo), 0, &data);
		memcpy(data, &gubo, sizeof(gubo));
		vkUnmapMemory(device, globalDS.uniformBuffersMemory[0][currentImage]);

		for (ModelInfo mi : modelInfos)
		{
			vkMapMemory(device, mi.DS.uniformBuffersMemory[0][currentImage], 0,
				sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(device, mi.DS.uniformBuffersMemory[0][currentImage]);
		}
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