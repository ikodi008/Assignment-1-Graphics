#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"



DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json & config) {
	_CreateScene();
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	}
	else {
		// This time we'll have 2 different shaders, and share data between both of them using the UBO
		// This shader will handle reflective materials 
		ShaderProgram::Sptr reflectiveShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_environment_reflective.glsl" }
		});
		reflectiveShader->SetDebugName("Reflective");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr basicShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_blinn_phong_textured.glsl" }
		});
		basicShader->SetDebugName("Blinn-phong");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr specShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/textured_specular.glsl" }
		});
		specShader->SetDebugName("Textured-Specular");

		// This shader handles our foliage vertex shader example
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/screendoor_transparency.glsl" }
		});
		foliageShader->SetDebugName("Foliage");

		// This shader handles our cel shading example
		ShaderProgram::Sptr toonShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/toon_shading.glsl" }
		});
		toonShader->SetDebugName("Toon Shader");

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping");

		// This shader handles our tangent space normal mapping
		ShaderProgram::Sptr tangentSpaceMapping = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		tangentSpaceMapping->SetDebugName("Tangent Space Mapping");

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing");

		// Load in the meshes
		MeshResource::Sptr carMesh = ResourceManager::CreateAsset<MeshResource>("car.obj");
		MeshResource::Sptr carMesh2 = ResourceManager::CreateAsset<MeshResource>("car2.obj");
		MeshResource::Sptr buildingMesh = ResourceManager::CreateAsset<MeshResource>("building1.obj");
		MeshResource::Sptr buildingMesh1 = ResourceManager::CreateAsset<MeshResource>("building1.obj");
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("TrafficL.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/asphalt.png");
		Texture2D::Sptr    boxSpec = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    LightTex = ResourceManager::CreateAsset<Texture2D>("textures/light.png");
		Texture2D::Sptr    carTex = ResourceManager::CreateAsset<Texture2D>("textures/car.png");
		Texture2D::Sptr    car2Tex = ResourceManager::CreateAsset<Texture2D>("textures/car2.png");
		Texture2D::Sptr    buildTex = ResourceManager::CreateAsset<Texture2D>("textures/2build texture.png");
		Texture2D::Sptr    buildTex2 = ResourceManager::CreateAsset<Texture2D>("textures/2build.png");
		Texture2D::Sptr    buildTex3 = ResourceManager::CreateAsset<Texture2D>("textures/build.png");
		Texture2D::Sptr    leafTex = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);


		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png");
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" }
		});

		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();


		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap);
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/WARM.CUBE");

		Texture3D::Sptr lut1 = ResourceManager::CreateAsset<Texture3D>("luts/COOL.CUBE");

		Texture3D::Sptr lut2 = ResourceManager::CreateAsset<Texture3D>("luts/BANDW.CUBE");

		// Configure the color correction LUT

		scene->SetColorLUT(lut, 0);
		
		scene->SetColorLUT(lut1, 1); 

		scene->SetColorLUT(lut2, 2);

		
		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.Diffuse", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
		}
		Material::Sptr carMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			carMaterial->Name = "Car";
			carMaterial->Set("u_Material.Diffuse", carTex);
			carMaterial->Set("u_Material.Shininess", 0.1f);

		}
		Material::Sptr carMaterial2 = ResourceManager::CreateAsset<Material>(specShader);
		{
			carMaterial2->Name = "Car2";
			carMaterial2->Set("u_Material.Diffuse", car2Tex);
			carMaterial2->Set("u_Material.Specular", car2Tex);

		}
		Material::Sptr buildMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			buildMaterial->Name = "Building";
			buildMaterial->Set("u_Material.Diffuse", buildTex);
			buildMaterial->Set("u_Material.Shininess", 0.1f);
		}
		Material::Sptr buildMaterial2 = ResourceManager::CreateAsset<Material>(basicShader);
		{
			buildMaterial2->Name = "Building2";
			buildMaterial2->Set("u_Material.Diffuse", buildTex2);
			buildMaterial2->Set("u_Material.Shininess", 0.1f);
		}
		Material::Sptr buildMaterial3 = ResourceManager::CreateAsset<Material>(basicShader);
		{
			buildMaterial3->Name = "Building3";
			buildMaterial3->Set("u_Material.Diffuse", buildTex3);
			buildMaterial3->Set("u_Material.Shininess", 0.1f);
		}
		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			monkeyMaterial->Name = "Light";
			monkeyMaterial->Set("u_Material.Diffuse", LightTex);
			monkeyMaterial->Set("u_Material.Shininess", 0.5f);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(specShader);
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.Diffuse", boxTexture);
			testMaterial->Set("u_Material.Specular", boxSpec);
		}

		// Our foliage vertex shader material
		Material::Sptr foliageMaterial = ResourceManager::CreateAsset<Material>(foliageShader);
		{
			foliageMaterial->Name = "Foliage Shader";
			foliageMaterial->Set("u_Material.Diffuse", leafTex);
			foliageMaterial->Set("u_Material.Shininess", 0.1f);
			foliageMaterial->Set("u_Material.Threshold", 0.1f);

			foliageMaterial->Set("u_WindDirection", glm::vec3(1.0f, 1.0f, 0.0f));
			foliageMaterial->Set("u_WindStrength", 0.5f);
			foliageMaterial->Set("u_VerticalScale", 1.0f);
			foliageMaterial->Set("u_WindSpeed", 1.0f);
		}

		// Our toon shader material
		Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(toonShader);
		{
			toonMaterial->Name = "Toon";
			toonMaterial->Set("u_Material.Diffuse", boxTexture);
			toonMaterial->Set("s_ToonTerm", toonLut);
			toonMaterial->Set("u_Material.Shininess", 0.1f);
			toonMaterial->Set("u_Material.Steps", 8);
		}


		Material::Sptr displacementTest = ResourceManager::CreateAsset<Material>(displacementShader);
		{
			Texture2D::Sptr displacementMap = ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png");
			Texture2D::Sptr normalMap = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			displacementTest->Name = "Displacement Map";
			displacementTest->Set("u_Material.Diffuse", diffuseMap);
			displacementTest->Set("s_Heightmap", displacementMap);
			displacementTest->Set("s_NormalMap", normalMap);
			displacementTest->Set("u_Material.Shininess", 0.5f);
			displacementTest->Set("u_Scale", 0.1f);
		}

		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(tangentSpaceMapping);
		{
			Texture2D::Sptr normalMap = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.Diffuse", diffuseMap);
			normalmapMat->Set("s_NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr multiTextureMat = ResourceManager::CreateAsset<Material>(multiTextureShader);
		{
			Texture2D::Sptr sand = ResourceManager::CreateAsset<Texture2D>("textures/terrain/sand.png");
			Texture2D::Sptr grass = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");

			multiTextureMat->Name = "Multitexturing";
			multiTextureMat->Set("u_Material.DiffuseA", sand);
			multiTextureMat->Set("u_Material.DiffuseB", grass);
			multiTextureMat->Set("u_Material.Shininess", 0.5f);
			multiTextureMat->Set("u_Scale", 0.1f);
		}

		// Create some lights for our scene
		scene->Lights.resize(3);
		scene->Lights[0].Position = glm::vec3(0.0f, 1.0f, 12.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 170.0f;

		//scene->Lights[1].Position = glm::vec3(1.0f, 0.0f, 5.0f);
		//scene->Lights[1].Color = glm::vec3(0.2f, 0.8f, 5.1f);

		//scene->Lights[2].Position = glm::vec3(0.0f, 1.0f, 3.0f);
		//scene->Lights[2].Color = glm::vec3(1.0f, 0.2f, 0.1f);

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion({ 7.640,23.160, 4.630 });
			camera->SetRotation({ 81.696, 0, -143.680 });
			camera->LookAt(glm::vec3(0.0f));

			camera->Add<SimpleCameraControl>();

			// This is now handled by scene itself!
			//Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			//scene->MainCamera = cam;
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(20.0f)));
			tiledMesh->GenerateMesh();

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(boxMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(BoxCollider::Create(glm::vec3(50.0f, 50.0f, 1.0f)))->SetPosition({ 0,0,-1 });
		}
		GameObject::Sptr car = scene->CreateGameObject("DRiftcar1");
		{
			// Set position in the scene
			car->SetPostion(glm::vec3(-6.5f, -3.0f, -1.1f));
			car->SetRotation(glm::vec3(89.0f, 0.0f, 0.0f));

			// Add some behaviour that relies on the physics body
			car->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer1 = car->Add<RenderComponent>();
			renderer1->SetMesh(carMesh);
			renderer1->SetMaterial(carMaterial);

			car->Add<TriggerVolumeEnterBehaviour>();
			RotatingBehaviour::Sptr behaviour1 = car->Add<RotatingBehaviour>();
			behaviour1->RotationSpeed = glm::vec3(0.0f, 0.0f, 32.0f);

		}
		GameObject::Sptr car2 = scene->CreateGameObject("DRiftcar2");
		{
			// Set position in the scene
			car2->SetPostion(glm::vec3(-6.5f, -3.0f, -1.1f));
			car2->SetRotation(glm::vec3(89.0f, 0.0f, 38.0f));

			// Add some behaviour that relies on the physics body
			car2->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer14 = car2->Add<RenderComponent>();
			renderer14->SetMesh(carMesh2);
			renderer14->SetMaterial(carMaterial2);

			car2->Add<TriggerVolumeEnterBehaviour>();
			RotatingBehaviour::Sptr behaviour1t = car2->Add<RotatingBehaviour>();
			behaviour1t->RotationSpeed = glm::vec3(0.0f, 0.0f, 32.0f);

		}
		GameObject::Sptr Building = scene->CreateGameObject("Building");
		{
			// Set position in the scene
			Building->SetPostion(glm::vec3(-2.5f, -26.490f, -1.090f));
			Building->SetRotation(glm::vec3(89.0f, 0.0f, -94.00f));

			// Add some behaviour that relies on the physics body
			Building->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer1 = Building->Add<RenderComponent>();
			renderer1->SetMesh(buildingMesh);
			renderer1->SetMaterial(buildMaterial);

		}
		GameObject::Sptr Building1 = scene->CreateGameObject("Building");
		{
			// Set position in the scene
			Building1->SetPostion(glm::vec3(10.180f, -26.490f, -1.090f));
			Building1->SetRotation(glm::vec3(89.0f, 0.0f, -94.00f));

			// Add some behaviour that relies on the physics body
			Building1->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer11 = Building1->Add<RenderComponent>();
			renderer11->SetMesh(buildingMesh);
			renderer11->SetMaterial(buildMaterial2);

		}
		GameObject::Sptr Building2 = scene->CreateGameObject("Building3");
		{
			// Set position in the scene
			Building2->SetPostion(glm::vec3(17.850f, 16.980f, -1.090f));
			Building2->SetRotation(glm::vec3(89.0f, 0.0f, 2.00f));

			// Add some behaviour that relies on the physics body
			Building2->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer114 = Building2->Add<RenderComponent>();
			renderer114->SetMesh(buildingMesh);
			renderer114->SetMaterial(buildMaterial);

		}
		GameObject::Sptr Building4 = scene->CreateGameObject("Building4");
		{
			// Set position in the scene
			Building4->SetPostion(glm::vec3(13.410f, -22.340f, -1.090f));
			Building4->SetRotation(glm::vec3(89.0f, 0.0f, 2.00f));

			// Add some behaviour that relies on the physics body
			Building4->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer1144 = Building4->Add<RenderComponent>();
			renderer1144->SetMesh(buildingMesh);
			renderer1144->SetMaterial(buildMaterial);

		}
		GameObject::Sptr Building5 = scene->CreateGameObject("Building5");
		{
			// Set position in the scene
			Building5->SetPostion(glm::vec3(-16.340f, -26.490f, -1.090f));
			Building5->SetRotation(glm::vec3(89.0f, 0.0f, -94.00f));

			// Add some behaviour that relies on the physics body
			Building5->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer118 = Building5->Add<RenderComponent>();
			renderer118->SetMesh(buildingMesh);
			renderer118->SetMaterial(buildMaterial3);

		}
		GameObject::Sptr Building6 = scene->CreateGameObject("Building6");
		{
			// Set position in the scene
			Building6->SetPostion(glm::vec3(-24.850f, 12.450f, -1.090f));
			Building6->SetRotation(glm::vec3(89.0f, 0.0f, 173.00f));

			// Add some behaviour that relies on the physics body
			Building6->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer1181 = Building6->Add<RenderComponent>();
			renderer1181->SetMesh(buildingMesh);
			renderer1181->SetMaterial(buildMaterial3);

		}
		GameObject::Sptr Building7 = scene->CreateGameObject("Building7");
		{
			// Set position in the scene
			Building7->SetPostion(glm::vec3(-31.380f, -26.490f, -1.090f));
			Building7->SetRotation(glm::vec3(89.0f, 0.0f, -94.00f));

			// Add some behaviour that relies on the physics body
			Building7->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer1182 = Building7->Add<RenderComponent>();
			renderer1182->SetMesh(buildingMesh);
			renderer1182->SetMaterial(buildMaterial2);

		}
		
		GameObject::Sptr lights = scene->CreateGameObject("lights");
		{
			// Set position in the scene
			lights->SetPostion(glm::vec3(16.910f, -15.590f, 0.700f));
			lights->SetRotation(glm::vec3(89.0f, -1.00f, -94.00f));


			// Add some behaviour that relies on the physics body
			lights->Add<JumpBehaviour>();

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer112 = lights->Add<RenderComponent>();
			renderer112->SetMesh(monkeyMesh);
			renderer112->SetMaterial(monkeyMaterial);

		}


		GameObject::Sptr demoBase = scene->CreateGameObject("Demo Parent");




		/*// Create a trigger volume for testing how we can detect collisions with objects!
		GameObject::Sptr trigger = scene->CreateGameObject("Trigger");
		{
			TriggerVolume::Sptr volume = trigger->Add<TriggerVolume>();
			CylinderCollider::Sptr collider = CylinderCollider::Create(glm::vec3(3.0f, 3.0f, 1.0f));
			collider->SetPosition(glm::vec3(0.0f, 0.0f, 0.5f));
			volume->AddCollider(collider);

			trigger->Add<TriggerVolumeEnterBehaviour>();
		}

		/////////////////////////// UI //////////////////////////////
		/*
		GameObject::Sptr canvas = scene->CreateGameObject("UI Canvas");
		{
			RectTransform::Sptr transform = canvas->Add<RectTransform>();
			transform->SetMin({ 16, 16 });
			transform->SetMax({ 256, 256 });

			GuiPanel::Sptr canPanel = canvas->Add<GuiPanel>();


			GameObject::Sptr subPanel = scene->CreateGameObject("Sub Item");
			{
				RectTransform::Sptr transform = subPanel->Add<RectTransform>();
				transform->SetMin({ 10, 10 });
				transform->SetMax({ 128, 128 });

				GuiPanel::Sptr panel = subPanel->Add<GuiPanel>();
				panel->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/upArrow.png"));

				Font::Sptr font = ResourceManager::CreateAsset<Font>("fonts/Roboto-Medium.ttf", 16.0f);
				font->Bake();

				GuiText::Sptr text = subPanel->Add<GuiText>();
				text->SetText("Hello world!");
				text->SetFont(font);

				monkey1->Get<JumpBehaviour>()->Panel = text;
			}

			canvas->AddChild(subPanel);
		}
		*/

		GameObject::Sptr particles = scene->CreateGameObject("Particles");
		{
			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();
			particleManager->AddEmitter(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 10.0f), 10.0f, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		}

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}
