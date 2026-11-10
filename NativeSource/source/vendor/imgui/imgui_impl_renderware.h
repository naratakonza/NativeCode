#pragma once

struct ImDrawData;

bool ImGui_ImplRenderWare_Init();
bool ImGui_ImplRenderWare_NewFrame();
void ImGui_ImplRenderWare_RenderDrawData(const ImDrawData* drawData);
void ImGui_ImplRenderWare_ShutDown();
