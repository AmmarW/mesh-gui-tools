// main_gui.cpp - GUI frontend for HeatStack simulation

#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <fstream>

// HeatStack includes
#include "HeatEquationSolver.h"
#include "BoundaryConditions.h"
#include "InitialTemperature.h"
#include "MeshHandler.h"
#include "TimeHandler.h"
#include "TemperatureComparator.h"

// GUI + OpenGL includes
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// GUI state
char meshPath[256] = "mesh.obj";
char initTempPath[256] = "initial_temperature.csv";
char outputPath[256] = "output_temperature.csv";
float simDuration = 10.0f;
float timeStep = 0.1f;
bool runSimulation = false;

void launch_gui() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "HeatStack GUI", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Simulation Config");

        ImGui::InputText("Mesh File", meshPath, IM_ARRAYSIZE(meshPath));
        ImGui::InputText("Initial Temp CSV", initTempPath, IM_ARRAYSIZE(initTempPath));
        ImGui::InputText("Output CSV", outputPath, IM_ARRAYSIZE(outputPath));
        ImGui::InputFloat("Simulation Duration", &simDuration);
        ImGui::InputFloat("Time Step", &timeStep);

        if (ImGui::Button("Run Simulation")) {
            runSimulation = true;
        }

        ImGui::End();
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        if (runSimulation) {
            // Call HeatStack modules here
            MeshHandler meshHandler(meshPath);
            InitialTemperature initTemp(initTempPath);
            BoundaryConditions boundary;
            TimeHandler time(simDuration, timeStep);
            HeatEquationSolver solver(meshHandler, boundary, initTemp, time);
            solver.solve();
            solver.saveResults(outputPath);
            runSimulation = false;
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main() {
    launch_gui();
    return 0;
}
