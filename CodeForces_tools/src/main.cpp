// main.cpp

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <chrono>

// Include GLFW (make sure to link with glfw)
#include "GLFW/glfw3.h"

// Include Dear ImGui headers and backends
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Include tinyfiledialogs for file selection
#include "third-party/tinyfiledialogs/tinyfiledialogs.h"

// Include your mesh processing headers
#include "ObjParser.h"
#include "Mesh.h"
#include "MeshValidator.h"

//------------------------------------------------------------------------------
// Helper function to compute which faces have errors based on edge counts.
// For each face, if any of its edges are not shared by exactly 2 faces,
// then mark that face as having an error.
std::vector<bool> getErrorFaces(const Mesh &mesh) {
    std::vector<bool> errorFaces(mesh.faces.size(), false);
    std::map<std::pair<int,int>, int> edgeCount;
    
    // Count occurrences of each edge (store edges with sorted vertex indices)
    for (const auto& face : mesh.faces) {
        size_t n = face.elements.size();
        for (size_t i = 0; i < n; i++) {
            int v1 = face.elements[i].vertexIndex;
            int v2 = face.elements[(i+1) % n].vertexIndex;
            if (v1 > v2) std::swap(v1, v2);
            edgeCount[{v1, v2}]++;
        }
    }
    
    // Mark any face that has at least one edge with an invalid count
    for (size_t i = 0; i < mesh.faces.size(); i++) {
        bool error = false;
        const Face &face = mesh.faces[i];
        size_t n = face.elements.size();
        for (size_t j = 0; j < n; j++) {
            int v1 = face.elements[j].vertexIndex;
            int v2 = face.elements[(j+1) % n].vertexIndex;
            if (v1 > v2) std::swap(v1, v2);
            auto it = edgeCount.find({v1, v2});
            if (it == edgeCount.end() || it->second != 2) {
                error = true;
                break;
            }
        }
        errorFaces[i] = error;
    }
    return errorFaces;
}

// Global camera and render mode control variables.
bool cameraMovementEnabled = false;
float camOffsetX = 0.0f;
float camOffsetY = 0.0f;
float camZoom = 1.0f;
int renderMode = 0; // 0 = Faces, 1 = Wireframe

// Global variable to store mesh processing time (in milliseconds)
double processingTime = 0.0;

//------------------------------------------------------------------------------
// Entry point
int main(int argc, char** argv) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Mesh Viewer", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends for ImGui
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Variables to store mesh and validation data
    Mesh mesh;
    bool meshLoaded = false;
    std::vector<std::string> validationErrors;
    std::vector<bool> errorFaces;

    int display_w, display_h;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Start a new ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Get the current framebuffer size (for positioning GUI windows)
        glfwGetFramebufferSize(window, &display_w, &display_h);
        
        // --- Update zoom based on mouse scroll (after NewFrame) ---
        if (io.MouseWheel != 0.0f) {
            camZoom *= (1.0f + io.MouseWheel * 0.1f);
            if (camZoom < 0.1f) camZoom = 0.1f;
            if (camZoom > 10.0f) camZoom = 10.0f;
        }
        
        // --- Camera Controls Window (Top Left) ---
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
        ImGui::Begin("Camera Controls");
        ImGui::Checkbox("Enable Camera Movement", &cameraMovementEnabled);
        ImGui::Text("Zoom: %.2f (use mouse scroll)", camZoom);
        ImGui::Text("Offset: (%.2f, %.2f)", camOffsetX, camOffsetY);
        const char* modes[] = { "Faces", "Wireframe" };
        ImGui::Combo("Render Mode", &renderMode, modes, IM_ARRAYSIZE(modes));
        ImGui::End();

        // --- Handle camera panning via mouse drag if enabled and not dragging GUI ---
        if (cameraMovementEnabled && !io.WantCaptureMouse && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            camOffsetX += io.MouseDelta.x * 0.01f;
            camOffsetY -= io.MouseDelta.y * 0.01f;
        }

        // --- Validation Results Window (Below Camera Controls) ---
        ImGui::SetNextWindowPos(ImVec2(10, 120), ImGuiCond_Once);
        ImGui::Begin("Validation Results");
        if (!meshLoaded) {
            ImGui::Text("No mesh loaded.");
        } else {
            if (validationErrors.empty()) {
                ImGui::Text("Mesh validation successful.");
            } else {
                ImGui::TextWrapped("Validation Errors:");
                for (const auto &err : validationErrors) {
                    ImGui::TextWrapped("%s", err.c_str());
                }
            }
        }
        ImGui::End();

        // --- File Selection Panel (Only if mesh not loaded) ---
        if (!meshLoaded) {
            ImGui::SetNextWindowPos(ImVec2(10, 250), ImGuiCond_Once);
            ImGui::Begin("Load OBJ File", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Select an OBJ file to process.");
            if (ImGui::Button("Select OBJ File")) {
                const char* filterPatterns[1] = { "*.obj" };
                const char* filePath = tinyfd_openFileDialog(
                    "Select OBJ File", 
                    "./",  // Default to project directory
                    1, 
                    filterPatterns, 
                    "OBJ Files", 
                    0
                );
                if (filePath) {
                    // Start timing the mesh processing.
                    auto startTime = std::chrono::high_resolution_clock::now();
                    try {
                        mesh = ObjParser::parse(filePath);
                        validationErrors = MeshValidator::validate(mesh);
                        errorFaces = getErrorFaces(mesh);
                        meshLoaded = true;
                    } catch (const std::exception &ex) {
                        std::cerr << "Error parsing OBJ file: " << ex.what() << "\n";
                    }
                    auto endTime = std::chrono::high_resolution_clock::now();
                    processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
                }
            }
            ImGui::End();
        }

        // --- Display Mesh Processing Runtime (Bottom Left) ---
        if (meshLoaded) {
            // Cast display_h - 60 to float to avoid conversion warnings.
            ImGui::SetNextWindowPos(ImVec2(10, static_cast<float>(display_h - 60)), ImGuiCond_Always);
            ImGui::Begin("Processing Time", nullptr,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Mesh Processing Time: %.2f ms", processingTime);
            ImGui::End();
        }
        
        // --- Render all ImGui windows ---
        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if (meshLoaded) {
            // Setup a simple orthographic projection
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            double aspect = static_cast<double>(display_w) / display_h;
            glOrtho(-aspect, aspect, -1, 1, -10, 10);
            
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            // Apply camera transformation: translation and zoom.
            glTranslatef(camOffsetX, camOffsetY, 0.0f);
            glScalef(camZoom, camZoom, 1.0f);
            
            glEnable(GL_DEPTH_TEST);
            
            // Set polygon mode based on render mode selection.
            if (renderMode == 1)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            
            // Render each face; error faces are drawn in red, valid faces in green.
            for (size_t i = 0; i < mesh.faces.size(); i++) {
                if (errorFaces[i])
                    glColor3f(1.0f, 0.0f, 0.0f);
                else
                    glColor3f(0.0f, 1.0f, 0.0f);
                
                glBegin(GL_POLYGON);
                const Face& face = mesh.faces[i];
                for (const auto &elem : face.elements) {
                    const Vertex &v = mesh.vertices[elem.vertexIndex];
                    glVertex3d(v.x, v.y, v.z);
                }
                glEnd();
            }
            // Reset polygon mode to fill for future rendering.
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        
        // Render ImGui on top of the OpenGL scene.
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup and exit
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
