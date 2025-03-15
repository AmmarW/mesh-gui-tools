// main_GUI.cpp

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <chrono>

// GLFW and OpenGL
#include "GLFW/glfw3.h"

// Dear ImGui and backends
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// tinyfiledialogs for file selection
#include "tinyfiledialogs.h"

// Mesh handling headers (for display and validation)
#include "ObjParser.h"
#include "Mesh.h"
#include "MeshValidator.h"

// Additional headers for transformation and export
#include "ObjExporter.h"
#include "MeshMetadata.h"
#include "MetadataExporter.h"
#include "MeshTransform.h"

//------------------------------------------------------------------------------
// Helper function to compute error faces.
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
    
    // Mark any face with an edge that does not occur exactly twice.
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

//------------------------------------------------------------------------------
// Global variables for viewer controls
bool cameraMovementEnabled = false;
float camOffsetX = 0.0f;
float camOffsetY = 0.0f;
float camZoom = 1.0f;
int renderMode = 0; // 0 = Faces, 1 = Wireframe

// Global mesh for display
Mesh mesh;
bool meshLoaded = false;
std::string loadedFilePath = "";
std::vector<std::string> validationErrors;
std::vector<bool> errorFaces;

// Processing time for mesh loading
double processingTime = 0.0;

//------------------------------------------------------------------------------
// Global variables for transformation and export controls
// Transformation parameters (defaults: no translation/rotation, unit scale)
double tx = 0.0, ty = 0.0, tz = 0.0;
double sx = 1.0, sy = 1.0, sz = 1.0;
double rx = 0.0, ry = 0.0, rz = 0.0;

// Mesh type selection: 0 = "surface", 1 = "volume", 2 = "both"
const char* meshTypeOptions[] = { "surface", "volume", "both" };
int currentMeshType = 0;

// Output file names for export
char outputFileName[128] = "output.obj";
char metadataFileName[128] = "metadata.txt";

// Message log for transformation & export results
std::string messageLog = "";
double exportProcessingTime = 0.0;

//------------------------------------------------------------------------------
// Main entry point
int main(int argc, char** argv) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    
    // Create window and OpenGL context
    // Get the primary monitor's video mode to determine the display resolution
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int width = mode->width;
    int height = mode->height;

    // Create a windowed full screen window with the display's resolution
    GLFWwindow* window = glfwCreateWindow(width, height, "Mesh Viewer", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context and style
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Initialize ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    int display_w, display_h;
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Start a new ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Get current framebuffer size for layout
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // --- Camera Controls Window (Top Left) ---
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
        ImGui::Begin("Camera Controls");
        ImGui::Checkbox("Enable Camera Movement", &cameraMovementEnabled);
        ImGui::Text("Zoom: %.2f (use mouse scroll)", camZoom);
        ImGui::Text("Offset: (%.2f, %.2f)", camOffsetX, camOffsetY);
        const char* modes[] = { "Faces", "Wireframe" };
        ImGui::Combo("Render Mode", &renderMode, modes, IM_ARRAYSIZE(modes));
        ImGui::End();

        // --- Handle camera panning via mouse drag if enabled ---
        if (cameraMovementEnabled && !io.WantCaptureMouse && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            // Reduced translation speed (multiplier reduced from 0.01 to 0.005)
            camOffsetX += io.MouseDelta.x * 0.005f;
            camOffsetY -= io.MouseDelta.y * 0.005f;
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
                    "./",  // Default directory
                    1, 
                    filterPatterns, 
                    "OBJ Files", 
                    0
                );
                if (filePath) {
                    loadedFilePath = filePath;
                    // Start timing mesh loading
                    auto startTime = std::chrono::high_resolution_clock::now();
                    try {
                        ObjParser parser; // Create an instance of ObjParser
                        mesh = parser.parse(filePath); // Call parse on the instance
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
        
        // --- Transform & Export Controls Panel ---
        ImGui::SetNextWindowPos(ImVec2(10, 400), ImGuiCond_Once);
        ImGui::Begin("Transform & Export Controls");
        ImGui::Text("Transformation Parameters:");
        // Translation inputs (for mesh transformation, not camera)
        ImGui::InputDouble("Translate X", &tx, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Translate Y", &ty, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Translate Z", &tz, 0.0, 0.0, "%.2f");
        // Scaling inputs
        ImGui::InputDouble("Scale X", &sx, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Scale Y", &sy, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Scale Z", &sz, 0.0, 0.0, "%.2f");
        // Rotation inputs
        ImGui::InputDouble("Rotate X (°)", &rx, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Rotate Y (°)", &ry, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Rotate Z (°)", &rz, 0.0, 0.0, "%.2f");
        
        // Mesh type selection drop-down
        ImGui::Combo("Mesh Type", &currentMeshType, meshTypeOptions, IM_ARRAYSIZE(meshTypeOptions));
        
        // Output file names for export
        ImGui::InputText("Output OBJ File", outputFileName, IM_ARRAYSIZE(outputFileName));
        ImGui::InputText("Metadata File", metadataFileName, IM_ARRAYSIZE(metadataFileName));
        
        // Button to reset transformation parameters to default values
        if (ImGui::Button("Reset Transformations")) {
            tx = 0.0; ty = 0.0; tz = 0.0;
            sx = 1.0; sy = 1.0; sz = 1.0;
            rx = 0.0; ry = 0.0; rz = 0.0;
            messageLog = "Transformation parameters reset to default.";
        }
        
        // Separate buttons for applying transformation and export
        if (ImGui::Button("Apply Transformations")) {
            if (loadedFilePath.empty()) {
                messageLog = "No file loaded. Please load an OBJ file first.";
            } else {
                try {
                    ObjParser parser;
                    // For display update, if "both" is selected, use surface mesh.
                    if (currentMeshType == 0 || currentMeshType == 2) {
                        mesh = parser.parseSurfaceMesh(loadedFilePath.c_str());
                    } else if (currentMeshType == 1) {
                        mesh = parser.parseVolumeMesh(loadedFilePath.c_str());
                    }
                    
                    // Apply transformation parameters to the mesh
                    if (tx != 0.0 || ty != 0.0 || tz != 0.0)
                        MeshTransformer::translate(mesh, tx, ty, tz);
                    if (sx != 1.0 || sy != 1.0 || sz != 1.0)
                        MeshTransformer::scale(mesh, sx, sy, sz);
                    if (rx != 0.0 || ry != 0.0 || rz != 0.0)
                        MeshTransformer::rotate(mesh, rx, ry, rz);
                    
                    // Update validation and error display after transformation
                    validationErrors = MeshValidator::validate(mesh);
                    errorFaces = getErrorFaces(mesh);
                    messageLog = "Transformations applied and view updated.";
                } catch (const std::exception &ex) {
                    messageLog = "Error applying transformations: " + std::string(ex.what());
                }
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Export Mesh")) {
            if (loadedFilePath.empty()) {
                messageLog = "No file loaded. Please load an OBJ file first.";
            } else {
                messageLog.clear();
                auto startExport = std::chrono::high_resolution_clock::now();
                ObjParser parser;
                // Export surface mesh if applicable
                if (currentMeshType == 0 || currentMeshType == 2) {
                    try {
                        Mesh surfaceMesh = parser.parseSurfaceMesh(loadedFilePath.c_str());
                        if (tx != 0.0 || ty != 0.0 || tz != 0.0)
                            MeshTransformer::translate(surfaceMesh, tx, ty, tz);
                        if (sx != 1.0 || sy != 1.0 || sz != 1.0)
                            MeshTransformer::scale(surfaceMesh, sx, sy, sz);
                        if (rx != 0.0 || ry != 0.0 || rz != 0.0)
                            MeshTransformer::rotate(surfaceMesh, rx, ry, rz);
                        std::string surfaceOutputFile = "surface_" + std::string(outputFileName);
                        if (ObjExporter::exportMesh(surfaceMesh, surfaceOutputFile))
                            messageLog += "Surface mesh exported to " + surfaceOutputFile + "\n";
                        else
                            messageLog += "Failed to export surface mesh to " + surfaceOutputFile + "\n";
                    } catch (const std::exception &ex) {
                        messageLog += "Error processing surface mesh: " + std::string(ex.what()) + "\n";
                    }
                }
                // Export volume mesh if applicable
                if (currentMeshType == 1 || currentMeshType == 2) {
                    try {
                        Mesh volumeMesh = parser.parseVolumeMesh(loadedFilePath.c_str());
                        if (tx != 0.0 || ty != 0.0 || tz != 0.0)
                            MeshTransformer::translate(volumeMesh, tx, ty, tz);
                        if (sx != 1.0 || sy != 1.0 || sz != 1.0)
                            MeshTransformer::scale(volumeMesh, sx, sy, sz);
                        if (rx != 0.0 || ry != 0.0 || rz != 0.0)
                            MeshTransformer::rotate(volumeMesh, rx, ry, rz);
                        std::string volumeOutputFile = "volume_" + std::string(outputFileName);
                        if (ObjExporter::exportMesh(volumeMesh, volumeOutputFile))
                            messageLog += "Volume mesh exported to " + volumeOutputFile + "\n";
                        else
                            messageLog += "Failed to export volume mesh to " + volumeOutputFile + "\n";
                    } catch (const std::exception &ex) {
                        messageLog += "Error processing volume mesh: " + std::string(ex.what()) + "\n";
                    }
                }
                auto endExport = std::chrono::high_resolution_clock::now();
                exportProcessingTime = std::chrono::duration<double, std::milli>(endExport - startExport).count();
                messageLog += "Export processing time: " + std::to_string(exportProcessingTime) + " ms\n";
            }
        }
        
        // Button to clear loaded mesh and start from scratch
        if (ImGui::Button("Clear Mesh")) {
            loadedFilePath = "";
            meshLoaded = false;
            messageLog = "Mesh cleared. Please load a new OBJ file.";
        }
        
        // Display messages from transformation and export operations.
        if (!messageLog.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("%s", messageLog.c_str());
        }
        ImGui::End();

        // --- Display Mesh Processing Time (Bottom Left) ---
        if (meshLoaded) {
            ImGui::SetNextWindowPos(ImVec2(10, static_cast<float>(display_h - 60)), ImGuiCond_Always);
            ImGui::Begin("Processing Time", nullptr,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("Mesh Load Time: %.2f ms", processingTime);
            ImGui::End();
        }
        
        // --- Update zoom based on mouse scroll ---
        if (io.MouseWheel != 0.0f) {
            // Reduced zoom speed (multiplier reduced from 0.1 to 0.05)
            camZoom *= (1.0f + io.MouseWheel * 0.05f);
            if (camZoom < 0.1f) camZoom = 0.1f;
            if (camZoom > 10.0f) camZoom = 10.0f;
        }
        
        // --- Render OpenGL scene ---
        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if (meshLoaded) {
            // Setup an orthographic projection
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            double aspect = static_cast<double>(display_w) / display_h;
            glOrtho(-aspect, aspect, -1, 1, -10, 10);
            
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            // Apply camera transformation for view only (mesh coordinates remain unchanged)
            glTranslatef(camOffsetX, camOffsetY, 0.0f);
            glScalef(camZoom, camZoom, 1.0f);
            
            glEnable(GL_DEPTH_TEST);
            
            // Set polygon mode based on render mode
            if (renderMode == 1)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            
            // Render each face; error faces in red, valid faces in green.
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
            
            // --- Draw global coordinate frame at the origin ---
            glLineWidth(2.0f);
            glBegin(GL_LINES);
                // X-axis in red
                glColor3f(1.0f, 0.0f, 0.0f);
                glVertex3d(0.0, 0.0, 0.0);
                glVertex3d(0.5, 0.0, 0.0); // Adjust length as needed
                
                // Y-axis in green
                glColor3f(0.0f, 1.0f, 0.0f);
                glVertex3d(0.0, 0.0, 0.0);
                glVertex3d(0.0, 0.5, 0.0);
                
                // Z-axis in blue
                glColor3f(0.0f, 0.0f, 1.0f);
                glVertex3d(0.0, 0.0, 0.0);
                glVertex3d(0.0, 0.0, 0.5);
            glEnd();
            glLineWidth(1.0f);
            
            // Reset polygon mode for future rendering.
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
