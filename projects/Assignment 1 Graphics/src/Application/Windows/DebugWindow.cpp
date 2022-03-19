#include "DebugWindow.h"
#include "Application/Application.h"
#include "Application/ApplicationLayer.h"
#include "Application/Layers/RenderLayer.h"

DebugWindow::DebugWindow() :
	IEditorWindow()
{
	Name = "Debug";
	SplitDirection = ImGuiDir_::ImGuiDir_None;
	SplitDepth = 0.5f;
	Requirements = EditorWindowRequirements::Menubar;
}

DebugWindow::~DebugWindow() = default;

void DebugWindow::RenderMenuBar() 
{
	Application& app = Application::Get();
	RenderLayer::Sptr renderLayer = app.GetLayer<RenderLayer>();

	BulletDebugMode physicsDrawMode = app.CurrentScene()->GetPhysicsDebugDrawMode();
	if (BulletDebugDraw::DrawModeGui("Physics Debug Mode:", physicsDrawMode)) {
		app.CurrentScene()->SetPhysicsDebugDrawMode(physicsDrawMode);
	}

	ImGui::Separator();

	RenderFlags flags = renderLayer->GetRenderFlags();
	bool changed = false;
	bool temp = *(flags & RenderFlags::EnableColorCorrection);
	if (ImGui::Checkbox("Enable Warm Correction", &temp)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableColorCorrection) | (temp ? RenderFlags::EnableColorCorrection : RenderFlags::None);
	}
	
	temp = *(flags & RenderFlags::EnableColdCorrection);
	if (ImGui::Checkbox("Enable Cold Correction", &temp)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableColdCorrection) | (temp ? RenderFlags::EnableColdCorrection : RenderFlags::None);
	}
	
	temp = *(flags & RenderFlags::EnableBWCorrection);
	if (ImGui::Checkbox("Enable Black and White Correction", &temp)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableBWCorrection) | (temp ? RenderFlags::EnableBWCorrection : RenderFlags::None);
	}
	
	if (changed) {
		renderLayer->SetRenderFlags(flags);
	}
}
