#include <iostream>
#include <glad/include/glad/glad.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "patch.h"

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << "\n";
}

int main() {
    std::cout << "Starting Discord Patcher\n";
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "FAILED: glfwInit()\n";
        return -1;
    }

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(500, 300, "Discord Voice Patcher (Unofficial)", NULL, NULL);
    if (!window) {
        std::cerr << "FAILED: glfwCreateWindow()\n";
        glfwTerminate();
        return -1;
    }

    glfwShowWindow(window);
    glfwFocusWindow(window);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor) {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        int monitorX, monitorY;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);
        int posX = monitorX + (mode->width - 500) / 2;
        int posY = monitorY + (mode->height - 300) / 2;
        glfwSetWindowPos(window, posX, posY);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "FAILED: gladLoadGLLoader()\n";
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    float gain_value = 30.0f;
    bool patches_applied = false;
    char status[256] = "Click to apply";

    bool is_dragging = false;
    double drag_offset_x = 0.0;
    double drag_offset_y = 0.0;

    bool is_resizing = false;
    double resize_start_x = 0.0;
    double resize_start_y = 0.0;
    int resize_start_width = 0;
    int resize_start_height = 0;
    const int resize_border = 15;

    GLFWcursor* resize_cursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    GLFWcursor* arrow_cursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
        int win_width, win_height;
        glfwGetWindowSize(window, &win_width, &win_height);

        bool in_resize_zone = (mouse_x >= win_width - resize_border && mouse_y >= win_height - resize_border);

        if (in_resize_zone) {
            glfwSetCursor(window, resize_cursor);
        }
        else {
            glfwSetCursor(window, arrow_cursor);
        }

        if (in_resize_zone && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !is_resizing && !is_dragging) {
            is_resizing = true;
            resize_start_x = mouse_x;
            resize_start_y = mouse_y;
            resize_start_width = win_width;
            resize_start_height = win_height;
        }

        if (is_resizing) {
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                int new_width = resize_start_width + (int)(mouse_x - resize_start_x);
                int new_height = resize_start_height + (int)(mouse_y - resize_start_y);

                if (new_width < 400) new_width = 400;
                if (new_height < 250) new_height = 250;

                glfwSetWindowSize(window, new_width, new_height);
            }
            else {
                is_resizing = false;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGui::Begin("Discord Patcher (Unofficial)", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse);

        ImGui::BeginGroup();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Discord Audio Patcher (Unofficial)");
        ImGui::SameLine(ImGui::GetWindowWidth() - 30);
        if (ImGui::Button("X", ImVec2(20, 20))) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        ImGui::EndGroup();

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && !is_resizing) {
            is_dragging = true;
            glfwGetCursorPos(window, &mouse_x, &mouse_y);
            drag_offset_x = mouse_x;
            drag_offset_y = mouse_y;
        }

        if (is_dragging) {
            if (ImGui::IsMouseDown(0)) {
                glfwGetCursorPos(window, &mouse_x, &mouse_y);
                int win_x, win_y;
                glfwGetWindowPos(window, &win_x, &win_y);
                glfwSetWindowPos(window,
                    win_x + (int)(mouse_x - drag_offset_x),
                    win_y + (int)(mouse_y - drag_offset_y));
            }
            else {
                is_dragging = false;
            }
        }

        ImGui::Separator();

        ImGui::Text("Status: %s", status);
        ImGui::Spacing();

        if (!patches_applied) {
            if (ImGui::Button("Apply patches", ImVec2(300, 50))) {
                if (ApplyAllPatches(gain_value)) {
                    patches_applied = true;
                    strcpy_s(status, "Patches applied successfully");
                }
                else {
                    strcpy_s(status, "Patch failed (Try running as admin)");
                }
            }
            ImGui::SliderFloat("Gain", &gain_value, 1.0f, 255.0f, "%.1f");
        }
        else {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Patcher working");
            if (ImGui::SliderFloat("Gain", &gain_value, 1.0f, 255.0f, "%.1f")) {
                UpdateGainValue(gain_value);
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("Future updates: Spectrum Analyzer, Bitrate Config, EQ, ETC!!!!!");

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 window_pos = ImGui::GetWindowPos();
        ImVec2 window_size = ImGui::GetWindowSize();
        ImVec2 corner_pos = ImVec2(window_pos.x + window_size.x - 15, window_pos.y + window_size.y - 15);

        for (int i = 0; i < 3; i++) {
            draw_list->AddLine(
                ImVec2(corner_pos.x + i * 5, corner_pos.y + 15),
                ImVec2(corner_pos.x + 15, corner_pos.y + i * 5),
                IM_COL32(150, 150, 150, 255),
                1.5f
            );
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    glfwDestroyCursor(resize_cursor);
    glfwDestroyCursor(arrow_cursor);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    if (g_discord_process) CloseHandle(g_discord_process);
    return 0;
}