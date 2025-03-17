/**
 * @file main.cpp
 * @brief Mesh viewer and editor application with ImGui-based GUI.
 * 
 * This application allows users to load, visualize, transform, and validate 3D meshes.
 * It includes support for OBJ file import, boolean operations, adaptive mesh generation,
 * and metadata assignment to mesh groups.
 */

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <fstream>
#include <array>
#include <cstring>
#include <cstdlib>
#include <iterator>
#include "Timer.h"
// GUI headers
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tinyfiledialogs.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// Mesh processing headers
#include "ObjParser.h"
#include "Mesh.h"
#include "MeshValidator.h"
#include "ObjExporter.h"
#include "MeshMetadata.h"        
#include "MetadataExporter.h"    
#include "MeshTransform.h"
#include "MeshConverter.h"
#include "MeshBooleanOperations.h"
#include "AdaptiveMeshGenerator.h"

//------------------------------------------------------------------------------

/** Global variables for viewer controls */
bool cameraMovementEnabled = false;
float camOffsetX = 0.0f;
float camOffsetY = 0.0f;
float camZoom = 1.0f;
int renderMode = 1; // 0 = Faces, 1 = Wireframe

/** Transformation parameters */ 
double tx = 0.0, ty = 0.0, tz = 0.0;
double sx = 1.0, sy = 1.0, sz = 1.0;
double rx = 0.0, ry = 0.0, rz = 0.0;

/** Adaptive Mesh Generator parameters */
double amg_sizing_field = 0.7;
double amg_edge_distance = 0.01;
double amg_facet_angle = 25.0;
double amg_facet_distance = 0.01;
double amg_cell_radius_edge_ratio = 3.0;
float amg_cube_size = 10.0f; // as float

// DEPRECATED: Mesh type selection: 0 = "surface", 1 = "volume", 2 = "both"
// const char* meshTypeOptions[] = { "surface", "volume", "both" };
int currentMeshType = 0;

/** Output file names */    
char outputFileName[128] = "output.obj";
char metadataFileName[128] = "metadata.json";
char outputVolMeshFileName[128] = "output_volume_mesh";

// File name for the Boolean operations result (OFF file)
std::string booleanResultFile = "boolean_result.off";
bool booleanOperationPerformed = false;

// Global merged metadata for the boolean-merged mesh.
MeshMetadata mergedMeshMetadata;
// Global active group name (for assigning picked faces)
std::string activeGroupName = "Inner";

// --- Drag Selection Globals ---
bool pickMode = false;      // toggled by a checkbox in the Metadata Assignment panel
bool dragSelectionActive = false;
ImVec2 dragStart, dragEnd;
bool exportCentroidInfo = false;  // If checked, export centroid info
bool exportFaceInfo = false;      // If checked, export face info

// Global log text string
std::string logText = "";
/**
 * @brief Appends a message to the global log.
 * @param msg The message to log.
 */
void appendLog(const std::string &msg) {
    logText += msg + "\n";
}


/** Structure representing a scene mesh */
struct SceneMesh {
    Mesh mesh; ///< The mesh object
    std::string filePath; ///< File path of the mesh
    std::vector<std::string> validationErrors; ///< Validation error messages
    std::vector<bool> errorFaces; ///< Boolean array for faces with errors
    double loadTime; ///< Time taken to load the mesh (ms)
    bool enabled = true; ///< Visibility status of the mesh
};

/** Global list of scene meshes */
std::vector<SceneMesh> sceneMeshes;

/** Index of the currently active mesh */
int activeMeshIndex = -1;

/**
 * @brief Extracts the filename from a full file path.
 * @param fullPath The complete file path.
 * @return Extracted filename.
 */
std::string extractFilename(const std::string &fullPath) {
    size_t pos = fullPath.find_last_of("/\\");
    if (pos != std::string::npos)
        return fullPath.substr(pos + 1);
    return fullPath;
}

// Helper function to compute error faces.
std::vector<bool> getErrorFaces(const Mesh &mesh) {
    std::vector<bool> errorFaces(mesh.faces.size(), false);
    // std::map<std::pair<int,int>, int> edgeCount; // Old code
    std::unordered_map<std::pair<int,int>, int, boost::hash<std::pair<int,int>>> edgeCount;
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

// Global validation log text string
std::string validationLogText = "";

// Function to append messages to the validation log
void appendValidationLog(const std::string &msg) {
    validationLogText += msg + "\n";
}

/**
 * @brief Validates the active mesh and updates the validation log.
 */
void validateActiveMesh() {
    if (activeMeshIndex < 0 || sceneMeshes.empty()) {
        appendValidationLog("No active mesh selected for validation.");
        return;
    }
    SceneMesh &activeMesh = sceneMeshes[activeMeshIndex];
    activeMesh.validationErrors = MeshValidator::validate(activeMesh.mesh);
    activeMesh.errorFaces = getErrorFaces(activeMesh.mesh);
    if (activeMesh.validationErrors.empty()) {
        appendValidationLog("Mesh " + extractFilename(activeMesh.filePath) + " passed validation.");
    } else {
        appendValidationLog("Validation errors found in mesh " + extractFilename(activeMesh.filePath) + ":");
        for (const auto &error : activeMesh.validationErrors) {
            appendValidationLog("- " + error);
        }
    }
}

// Helper: Simple projection of a 3D point into screen coordinates (orthographic).
ImVec2 projectPoint(const std::array<double, 3>& point, double aspect, float zoom,
                    float offsetX, float offsetY, int display_w, int display_h) {
    // Apply zoom and offset to the point coordinates
    double x = (point[0] + offsetX) * zoom;
    double y = (point[1] + offsetY) * zoom;
    
    // Convert to normalized device coordinates (NDC)
    double ndc_x = (x + aspect) / (2 * aspect);
    double ndc_y = (y + 1.0) / 2.0;
    
    // Convert NDC to screen coordinates
    ImVec2 screenPos;
    screenPos.x = ndc_x * display_w;
    screenPos.y = (1.0 - ndc_y) * display_h; // invert y-axis for screen coordinates
    return screenPos;
}

// Process drag selection: add faces to the active group's metadata.
void processDragSelection(int dispw, int disph, double aspect) {
    // Find the merged mesh
    int mergedIdx = -1;
    for (int i = 0; i < (int)sceneMeshes.size(); i++) {
        if (sceneMeshes[i].filePath == "boolean_result.obj") {
            mergedIdx = i;
            break;
        }
    }
    if (mergedIdx < 0) {
        appendLog("No merged mesh found for drag selection.");
        return;
    }
    auto &mergedMesh = sceneMeshes[mergedIdx].mesh;

    // Get the active group
    GroupMetadata* group = mergedMeshMetadata.getGroupMetadata(activeGroupName);
    if (!group) {
        appendLog("Could not find active group metadata for " + activeGroupName);
        return;
    }

    // Clear existing face assignments in that group.
    group->faceIndices.clear();
    group->spatialData.clear();

    // Determine the selection rectangle boundaries
    float x0 = std::min(dragStart.x, dragEnd.x);
    float x1 = std::max(dragStart.x, dragEnd.x);
    float y0 = std::min(dragStart.y, dragEnd.y);
    float y1 = std::max(dragStart.y, dragEnd.y);

    // For each face, compute centroid, project it, check if inside selection rect.
    for (int i = 0; i < (int)mergedMesh.faces.size(); i++) {
        const Face &face = mergedMesh.faces[i];
        double cx = 0, cy = 0, cz = 0;
        for (auto &elem : face.elements) {
            const Vertex &v = mergedMesh.vertices[elem.vertexIndex];
            cx += v.x;
            cy += v.y;
            cz += v.z;
        }
        int n = face.elements.size();
        if (n > 0) { 
            cx /= n; cy /= n; cz /= n; 
        }
        std::array<double,3> centroid = {cx, cy, cz};
        ImVec2 screenPt = projectPoint(centroid, aspect, camZoom, camOffsetX, camOffsetY, dispw, disph);

        // Check if the projected point is within the selection rectangle
        if (screenPt.x >= x0 && screenPt.x <= x1 && screenPt.y >= y0 && screenPt.y <= y1) {
            // Depending on export flags:
            if (exportCentroidInfo && !exportFaceInfo) {
                FaceSpatialData fsd;
                fsd.faceIndex = i;
                fsd.centroid = centroid;
                group->spatialData.push_back(fsd);
            }
            else if (!exportCentroidInfo && exportFaceInfo) {
                group->faceIndices.push_back(i);
            }
            else if (exportCentroidInfo && exportFaceInfo) {
                FaceSpatialData fsd;
                fsd.faceIndex = i;
                fsd.centroid = centroid;
                for (auto &elem : face.elements) {
                    const Vertex &v = mergedMesh.vertices[elem.vertexIndex];
                    fsd.vertices.push_back({v.x, v.y, v.z});
                }
                group->spatialData.push_back(fsd);
            }
            // If neither is selected, do nothing with that face.
        }
    }
    appendLog("Drag selection processed. Faces added to group: " + activeGroupName);
}

// After boolean operation, disable individual meshes (all except merged one).
void disableIndividualMeshes() {
    for (auto &mesh : sceneMeshes) {
        if (mesh.filePath != "boolean_result.obj")
            mesh.enabled = false;
    }
}

// Convert the boolean result OFF to OBJ, add to the scene, create default group, disable others.
void addBooleanOperationMesh(const std::string &offFile) {
    std::string newObjFile = "boolean_result.obj";
    MeshConverter converter;
    // Convert the OFF file to OBJ format
    if (converter.convertOffToObj(offFile, newObjFile)) {
        appendLog("Converted boolean result OFF to OBJ: " + newObjFile);
        SceneMesh newMesh;
        newMesh.filePath = newObjFile;
        try {
            // Parse the newly created OBJ file
            ObjParser parser;
            newMesh.mesh = parser.parse(newObjFile.c_str());
            // Validate the mesh and get error faces
            newMesh.validationErrors = MeshValidator::validate(newMesh.mesh);
            newMesh.errorFaces = getErrorFaces(newMesh.mesh);
            appendValidationLog("Boolean operation mesh added: " + extractFilename(newMesh.filePath));
            if (newMesh.validationErrors.empty()) {
                appendValidationLog("Mesh is valid after boolean operation.");
            } else {
                appendValidationLog("Validation errors in boolean operation mesh:");
                for (const auto& error : newMesh.validationErrors) {
                    appendValidationLog("- " + error);
                }
            }
            newMesh.loadTime = 0.0;
            // Add the new mesh to the scene
            sceneMeshes.push_back(newMesh);
            appendLog("Added boolean operation mesh to scene: " + extractFilename(newObjFile));
            booleanOperationPerformed = true;
            // Disable individual meshes after boolean operation
            disableIndividualMeshes();
            // Initialize merged metadata with a default group
            mergedMeshMetadata = MeshMetadata();
            GroupMetadata defaultGroup;
            defaultGroup.groupName = "Inner";
            defaultGroup.boundaryCondition.type = "fixed";
            defaultGroup.boundaryCondition.parameters = {0.0};
            defaultGroup.materialProperties.density = 7850.0;
            defaultGroup.materialProperties.elasticModulus = 210e9;
            defaultGroup.materialProperties.poissonRatio = 0.3;
            mergedMeshMetadata.addGroupMetadata(defaultGroup);
            activeGroupName = "Inner";
            appendLog("Initialized default metadata group 'Inner'.");
        } catch (const std::exception &ex) {
            appendLog("Error reading OBJ file " + newObjFile + ": " + std::string(ex.what()));
        }
    } else {
        appendLog("Conversion from OFF to OBJ failed.");
    }
}

// Function to load an image as an OpenGL texture
GLuint LoadTextureFromFile(const char* filename, int& width, int& height) {
    int channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
    if (data) {
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
        return texture;
    }
    return 0;
}

/**
 * @brief Entry point of the application.
 * 
 * Initializes GLFW and creates a window for rendering. Sets up ImGui context and initializes
 * ImGui for OpenGL and GLFW. The main loop handles rendering, user input, and GUI interactions.
 * 
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return int Exit status of the application.
 * 
 * The main loop includes:
 * - Camera controls for zoom and offset.
 * - Adding and clearing OBJ files.
 * - Displaying a list of loaded meshes.
 * - Applying transformations to the active mesh.
 * - Performing boolean operations on meshes.
 * - Generating and exporting adaptive volume meshes.
 * - Assigning metadata to mesh groups.
 * - Logging application events and errors.
 * 
 * The rendering section uses OpenGL to draw the loaded meshes and handles camera transformations.
 * ImGui windows are used for various controls and settings.
 */
int main(int argc, char** argv) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int width = mode->width, height = mode->height;
    GLFWwindow* window = glfwCreateWindow(width, height, "MeshX by CodeForces", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    // Load the icon
    int iconWidth, iconHeight, channels;
    unsigned char* pixels = stbi_load("icon.png", &iconWidth, &iconHeight, &channels, 4);
    if (pixels) {
        GLFWimage icon;
        icon.width = iconWidth;
        icon.height = iconHeight;
        icon.pixels = pixels;
        glfwSetWindowIcon(window, 1, &icon);
        stbi_image_free(pixels);
    } else {
        std::cerr << "Failed to load icon" << std::endl;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

   // Load the splash screen texture (the app icon)
   GLuint iconTexture = LoadTextureFromFile("icon.png", iconWidth, iconHeight);
   // Start timer for splash screen
   auto splashStartTime = std::chrono::steady_clock::now();
   const int splashDuration = 3000; // Duration in milliseconds

   bool showSplash = true;        

    int display_w, display_h;
    appendLog("Application started.");

    static int newGroupCounter = 1; // for naming new groups

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Check splash screen duration
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - splashStartTime).count();
        bool showSplash = (elapsed < splashDuration);
       if (showSplash) {
           // Draw the splash screen
           ImGui::SetNextWindowPos(ImVec2(0, 0));
           ImGui::SetNextWindowSize(io.DisplaySize);
           ImGui::Begin("Splash", nullptr,
                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoInputs);
           // Center content by using dummy widgets or calculate offsets
           ImGui::Dummy(ImVec2(0.0f, 450.0f)); // Vertical offset
           // Draw the app icon (centered)
           ImVec2 windowSize = io.DisplaySize;
           ImGui::SetCursorPosX((windowSize.x - iconWidth) * 0.5f);
           ImGui::Image((ImTextureID)(intptr_t)iconTexture, ImVec2(iconWidth, iconHeight));
           // App name (centered)
           ImGui::SetCursorPosX((windowSize.x - iconWidth) * 0.5f); // Adjust width as needed
           ImGui::Text("MeshX by CodeForces");
           ImGui::SetCursorPosX((windowSize.x - iconWidth) * 0.5f); // Adjust width as needed
           ImGui::Text("--------------------");
           // Developer names
           const char* developers[5] = { "Ammar Waheed", "Shivam Vashi", "Jasdeep Bajaj", "Harsh Mittal", "Mohini Priya Kolluri" };
           for (int i = 0; i < 5; i++) {
               ImGui::SetCursorPosX((windowSize.x - iconWidth) * 0.5f); // Adjust width as needed
               ImGui::Text("%s", developers[i]);
           }
           ImGui::End();
       }


        glfwGetFramebufferSize(window, &display_w, &display_h);
        
        // --- Camera Controls ---
        ImGui::SetNextWindowPos(ImVec2(10, 10),  ImGuiCond_FirstUseEver);
        ImGui::Begin("Camera Controls");
        ImGui::Checkbox("Enable Camera Movement", &cameraMovementEnabled);
        ImGui::Text("Zoom: %.2f (use mouse scroll)", camZoom);
        ImGui::Text("Offset: (%.2f, %.2f)", camOffsetX, camOffsetY);
        const char* modesArr[] = { "Faces", "Wireframe" };
        ImGui::Combo("Render Mode", &renderMode, modesArr, IM_ARRAYSIZE(modesArr));
        ImGui::End();

        // Drag camera if enabled
        if (cameraMovementEnabled && !io.WantCaptureMouse && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            camOffsetX += io.MouseDelta.x * 0.005f;
            camOffsetY -= io.MouseDelta.y * 0.005f;
        }

        // Set initial position on the left side and allow resizing
        ImGui::SetNextWindowPos(ImVec2(10, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Mesh Validation Log", nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        if (ImGui::Button("Clear Validation Log")) {
            validationLogText.clear();
        }
        ImGui::Separator();
        ImGui::BeginGroup();
        ImGui::BeginChildFrame(ImGui::GetID("ValidationLogFrame"), ImVec2(0, ImGui::GetContentRegionAvail().y));
        if (!validationLogText.empty()) {
            ImGui::TextWrapped("%s", validationLogText.c_str());
        } else {
            ImGui::Text("No validation errors.");
        }
        ImGui::EndChildFrame();
        ImGui::EndGroup();
        ImGui::End();
    

        // --- Add OBJ File ---
        ImGui::SetNextWindowPos(ImVec2(10, 70),  ImGuiCond_FirstUseEver);
        ImGui::Begin("Add OBJ File", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Select an OBJ file to add.");
        if (ImGui::Button("Select OBJ File")) {
            const char* filterPatterns[1] = { "*.obj" };
            const char* filePath = tinyfd_openFileDialog("Select OBJ File", "./", 1, filterPatterns, "OBJ Files", 0);
            if (filePath) {
                SceneMesh newMesh;
                newMesh.filePath = filePath;
                Timer timer;
                try {
                    ObjParser parser;
                    newMesh.mesh = parser.parse(filePath);
                    newMesh.validationErrors = MeshValidator::validate(newMesh.mesh);
                    newMesh.errorFaces = getErrorFaces(newMesh.mesh);
                    // Append validation messages
                    appendValidationLog("Imported mesh: " + extractFilename(newMesh.filePath));
                    if (newMesh.validationErrors.empty()) {
                        appendValidationLog("Mesh is valid.");
                    } else {
                        appendValidationLog("Validation errors in " + extractFilename(newMesh.filePath) + ":");
                        for (const auto& error : newMesh.validationErrors) {
                            appendValidationLog("- " + error);
                        }
                    }
                    appendLog("Imported mesh: " + extractFilename(newMesh.filePath));
                } catch (const std::exception &ex) {
                    appendLog("Error parsing OBJ file: " + std::string(ex.what()));
                }
                newMesh.loadTime = timer.elapsed();
                appendLog("Mesh " + extractFilename(newMesh.filePath) + " loaded and validated in " + std::to_string(newMesh.loadTime) + " ms");
                sceneMeshes.push_back(newMesh);
                if (activeMeshIndex < 0) {
                    activeMeshIndex = 0;
                }
            }
        }
        
        if (ImGui::Button("Clear Meshes")) {
            sceneMeshes.clear();
            activeMeshIndex = -1;
            booleanOperationPerformed = false;
            appendLog("Cleared all meshes.");
        }
        ImGui::End();
        
        // --- Mesh List ---
        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(display_w - 220), 10.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Mesh List", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (sceneMeshes.empty()) {
            ImGui::Text("No meshes loaded.");
        } else {
            for (int i = 0; i < (int)sceneMeshes.size(); i++) {
                ImGui::PushID(i);
                bool enabled = sceneMeshes[i].enabled;
                if (ImGui::Checkbox("", &enabled))
                    sceneMeshes[i].enabled = enabled;
                ImGui::SameLine();
                std::string meshName = extractFilename(sceneMeshes[i].filePath);
                if (i == activeMeshIndex)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
                if (ImGui::Selectable(meshName.c_str(), i == activeMeshIndex))
                    activeMeshIndex = i;
                if (i == activeMeshIndex)
                    ImGui::PopStyleColor();
                if (ImGui::Button("Remove")) {
                    appendLog("Removed mesh: " + meshName);
                    sceneMeshes.erase(sceneMeshes.begin() + i);
                    if (activeMeshIndex >= (int)sceneMeshes.size())
                        activeMeshIndex = sceneMeshes.empty() ? -1 : 0;
                    ImGui::PopID();
                    break;
                }
                ImGui::Separator();
                ImGui::PopID();
            }
        }
        ImGui::End();
        
        // --- Transformation Controls ---
        ImGui::SetNextWindowPos(ImVec2(10, 200),  ImGuiCond_FirstUseEver);
        ImGui::Begin("Transformation Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Transformation Parameters:");
        ImGui::InputDouble("Translate X", &tx, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Translate Y", &ty, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Translate Z", &tz, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Scale X", &sx, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Scale Y", &sy, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Scale Z", &sz, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Rotate X (°)", &rx, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Rotate Y (°)", &ry, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Rotate Z (°)", &rz, 0.0, 0.0, "%.2f");
        // ImGui::Combo("Mesh Type", &currentMeshType, meshTypeOptions, IM_ARRAYSIZE(meshTypeOptions));
        if (ImGui::Button("Apply Transformations")) {
            if (activeMeshIndex < 0 || sceneMeshes.empty()) {
                appendLog("No active mesh selected for transformation.");
            } else {
                try {
                    SceneMesh &activeMesh = sceneMeshes[activeMeshIndex];
                    Timer timer;
                    ObjParser parser;
                    if (currentMeshType == 0 || currentMeshType == 2)
                        activeMesh.mesh = parser.parseSurfaceMesh(activeMesh.filePath.c_str());
                    if (tx != 0.0 || ty != 0.0 || tz != 0.0)
                        MeshTransform::translate(activeMesh.mesh, tx, ty, tz);
                    if (sx != 1.0 || sy != 1.0 || sz != 1.0)
                        MeshTransform::scale(activeMesh.mesh, sx, sy, sz);
                    if (rx != 0.0 || ry != 0.0 || rz != 0.0)
                        MeshTransform::rotate(activeMesh.mesh, rx, ry, rz);
                    double transformTime = timer.elapsed();
                    activeMesh.validationErrors = MeshValidator::validate(activeMesh.mesh);
                    activeMesh.errorFaces = getErrorFaces(activeMesh.mesh);
                    appendValidationLog("Transformations applied to " + extractFilename(activeMesh.filePath));
                    if (activeMesh.validationErrors.empty()) {
                        appendValidationLog("Mesh is valid after transformation.");
                    } else {
                        appendValidationLog("Validation errors after transformation:");
                        for (const auto& error : activeMesh.validationErrors) {
                            appendValidationLog("- " + error);
                        }
                    }
                    appendLog("Transformations applied to mesh: " + extractFilename(activeMesh.filePath));
                    appendLog("Transformation time: " + std::to_string(transformTime) + " ms");
                } catch (const std::exception &ex) {
                    appendLog("Error applying transformations: " + std::string(ex.what()));
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Export Transformed Mesh")) {
            if (activeMeshIndex < 0 || sceneMeshes.empty()) {
                appendLog("No active mesh selected for export.");
            } else {
                try {
                    SceneMesh &activeMesh = sceneMeshes[activeMeshIndex];
                    if (ObjExporter::exportMesh(activeMesh.mesh, outputFileName))
                        appendLog("Transformed mesh exported to " + std::string(outputFileName));
                    else
                        appendLog("Failed to export transformed mesh.");
                } catch (const std::exception &ex) {
                    appendLog("Error exporting transformed mesh: " + std::string(ex.what()));
                }
            }
        }
        ImGui::End();
        
        // --- Boolean Operations ---
        ImGui::SetNextWindowPos(ImVec2(250, 200),  ImGuiCond_FirstUseEver);
        ImGui::Begin("Boolean Operations", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Perform Boolean Operations on Transformed Meshes:");
        if (ImGui::Button("Union")) {
            Timer timer;
            std::vector<MeshBooleanOperations::Polyhedron> polyMeshes;
            bool conversionSuccess = true;
            MeshConverter converter; // Create an instance of MeshConverter
            for (const auto &sceneMesh : sceneMeshes) {
                MeshBooleanOperations::Polyhedron poly;
                if (!converter.convertMeshToPolyhedron(sceneMesh.mesh, poly)) {
                    conversionSuccess = false;
                    break;
                }
                polyMeshes.push_back(poly);
            }
            if (!conversionSuccess || polyMeshes.empty())
                appendLog("Error: Failed to convert meshes for union operation.");
            else {
                MeshBooleanOperations::Polyhedron resultPoly;
                if (MeshBooleanOperations::computeUnion(polyMeshes, resultPoly)) {
                    if (MeshBooleanOperations::writeOFF(booleanResultFile, resultPoly)) {
                        appendLog("Union operation successful. Result saved to " + booleanResultFile);
                        booleanOperationPerformed = true;
                        addBooleanOperationMesh(booleanResultFile);
                    } else
                        appendLog("Error writing union result to OFF file.");
                } else
                    appendLog("Union operation failed.");
            }
            double unionTime = timer.elapsed();
            appendLog("Union operation time: " + std::to_string(unionTime) + " ms");
        }
        if (ImGui::Button("Intersection")) {
            Timer timer;
            std::vector<MeshBooleanOperations::Polyhedron> polyMeshes;
            bool conversionSuccess = true;
            MeshConverter converter;
            for (const auto &sceneMesh : sceneMeshes) {
                MeshBooleanOperations::Polyhedron poly;
                if (!converter.convertMeshToPolyhedron(sceneMesh.mesh, poly)) {
                    conversionSuccess = false;
                    break;
                }
                polyMeshes.push_back(poly);
            }
            if (!conversionSuccess || polyMeshes.empty())
                appendLog("Error: Failed to convert meshes for intersection operation.");
            else {
                MeshBooleanOperations::Polyhedron resultPoly;
                if (MeshBooleanOperations::computeIntersection(polyMeshes, resultPoly)) {
                    if (MeshBooleanOperations::writeOFF(booleanResultFile, resultPoly)) {
                        appendLog("Intersection operation successful. Result saved to " + booleanResultFile);
                        booleanOperationPerformed = true;
                        addBooleanOperationMesh(booleanResultFile);
                    } else
                        appendLog("Error writing intersection result to OFF file.");
                } else
                    appendLog("Intersection operation failed.");
            }
            double intersectionTime = timer.elapsed();
            appendLog("Intersection operation time: " + std::to_string(intersectionTime) + " ms");
        }
        if (ImGui::Button("Difference")) {
            Timer timer;
            std::vector<MeshBooleanOperations::Polyhedron> polyMeshes;
            bool conversionSuccess = true;
            MeshConverter converter;
            for (const auto &sceneMesh : sceneMeshes) {
                MeshBooleanOperations::Polyhedron poly;
                if (!converter.convertMeshToPolyhedron(sceneMesh.mesh, poly)) {
                    conversionSuccess = false;
                    break;
                }
                polyMeshes.push_back(poly);
            }
            if (!conversionSuccess || polyMeshes.empty())
                appendLog("Error: Failed to convert meshes for difference operation.");
            else {
                MeshBooleanOperations::Polyhedron resultPoly;
                if (MeshBooleanOperations::computeDifference(polyMeshes, resultPoly)) {
                    if (MeshBooleanOperations::writeOFF(booleanResultFile, resultPoly)) {
                        appendLog("Difference operation successful. Result saved to " + booleanResultFile);
                        booleanOperationPerformed = true;
                        addBooleanOperationMesh(booleanResultFile);
                    } else
                        appendLog("Error writing difference result to OFF file.");
                } else
                    appendLog("Difference operation failed.");
            }
            double differenceTime = timer.elapsed();
            appendLog("Difference operation time: " + std::to_string(differenceTime) + " ms");
        }
        ImGui::End();
        
        // --- Adaptive Mesh Generator Controls ---
        ImGui::SetNextWindowPos(ImVec2(250, 400),  ImGuiCond_FirstUseEver);
        ImGui::Begin("Adaptive Mesh Generator Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("AMG Parameters:");
        ImGui::InputDouble("Sizing Field", &amg_sizing_field, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Edge Distance", &amg_edge_distance, 0.0, 0.0, "%.3f");
        ImGui::InputDouble("Facet Angle", &amg_facet_angle, 0.0, 0.0, "%.2f");
        ImGui::InputDouble("Facet Distance", &amg_facet_distance, 0.0, 0.0, "%.3f");
        ImGui::InputDouble("Cell Radius Edge Ratio", &amg_cell_radius_edge_ratio, 0.0, 0.0, "%.2f");
        ImGui::InputFloat("Cube Size", &amg_cube_size, 0.0f, 0.0f, "%.2f");
        if (ImGui::Button("Export Volume Mesh")) {
            if (!booleanOperationPerformed)
                appendLog("No boolean operation result available for volume mesh generation.");
            else {
                Timer timer;
                try {
                    AdaptiveMeshGenerator adaptiveMeshGen;
                    if (adaptiveMeshGen.generateVolumeMesh(booleanResultFile, static_cast<int>(amg_cube_size), outputVolMeshFileName))
                        appendLog("Volume mesh generated and exported using AMG parameters.");
                    else
                        appendLog("Failed to generate volume mesh from boolean operation result.");
                } catch (const std::exception &ex) {
                    appendLog("Error exporting volume mesh: " + std::string(ex.what()));
                }
                double exportProcessingTime = timer.elapsed();
                appendLog("Volume mesh export processing time: " + std::to_string(exportProcessingTime) + " ms");
                if (MetadataExporter::exportMetadata(metadataFileName, mergedMeshMetadata))
                    appendLog("Metadata exported successfully to " + std::string(metadataFileName));
                else
                    appendLog("Failed to export metadata to " + std::string(metadataFileName));
            }
        }
        ImGui::End();
        
        // --- Metadata Assignment and Group Management ---
        if (booleanOperationPerformed) {
            ImGui::Begin("Metadata Assignment");
            if (ImGui::IsWindowHovered())
                io.MouseWheel = 0.0f;
            ImGui::Text("Pick/Assign faces to the merged mesh (boolean_result.obj)");

            // Checkbox for Pick Faces mode
            ImGui::Checkbox("Pick Faces", &pickMode);
            // Export options
            ImGui::Checkbox("Export Centroid Info", &exportCentroidInfo);
            ImGui::Checkbox("Export Face Info", &exportFaceInfo);

            ImGui::Separator();
            ImGui::Text("Groups:");
            // Retrieve group names from merged metadata
            std::vector<std::string> groupNames;
            for (auto const &pair : mergedMeshMetadata.getAllMetadata()) {
                groupNames.push_back(pair.first);
            }

            // For each group, display a collapsible sub-box with unique ID
            int i = 0;
            for (auto &gName : groupNames) {
                GroupMetadata* group = mergedMeshMetadata.getGroupMetadata(gName);
                if (!group) continue;

                // Push a unique ID to avoid collisions
                ImGui::PushID(i);

                // Use a unique label for the collapsing header
                bool isActive = (gName == activeGroupName);
                ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_DefaultOpen;
                if (isActive) node_flags |= ImGuiTreeNodeFlags_Selected;

                // Show the group name visually, but keep ID unique with ## suffix
                std::string headerLabel = group->groupName + "##collapsing" + std::to_string(i);
                bool nodeOpen = ImGui::CollapsingHeader(headerLabel.c_str(), node_flags);

                // If the user clicks on this header (and it's not the active group), switch
                if (ImGui::IsItemClicked() && !isActive) {
                    activeGroupName = group->groupName;
                    pickMode = false;
                    appendLog("Active group changed to " + activeGroupName + ". Please pick faces again.");
                }

                // If the header is open, show the editable fields
                if (nodeOpen) {
                    // Group Name
                    ImGui::Text("Group Name:");
                    ImGui::SameLine();
                    char buffer[128];
                    strncpy_s(buffer, group->groupName.c_str(), sizeof(buffer));
                    // Make sure ID is unique with "##groupName"
                    if (ImGui::InputText(std::string("##groupName").c_str(), buffer, sizeof(buffer))) {
                        std::string newName = buffer;
                        if (newName != group->groupName && !newName.empty()) {
                            // If name changes, remove the old group and add the new one
                            GroupMetadata temp = *group;
                            temp.groupName = newName;
                            mergedMeshMetadata.removeGroupMetadata(group->groupName);
                            mergedMeshMetadata.addGroupMetadata(temp);
                            if (activeGroupName == group->groupName) {
                                activeGroupName = newName;
                            }
                        }
                    }
                    
                    // Boundary Condition Type
                    ImGui::Text("Boundary Condition:");
                    ImGui::SameLine();
                    const char* bcOptions[] = { "fixed", "sliding", "free", "periodic" };
                    static int currentBcIndex = 0;
                    for (int i = 0; i < IM_ARRAYSIZE(bcOptions); ++i) {
                        if (group->boundaryCondition.type == bcOptions[i]) {
                            currentBcIndex = i;
                            break;
                        }
                    }
                    if (ImGui::Combo("##bcType", &currentBcIndex, bcOptions, IM_ARRAYSIZE(bcOptions))) {
                        group->boundaryCondition.type = bcOptions[currentBcIndex];
                    }

                    // Boundary Condition Parameters (comma-separated)
                    ImGui::Text("BC Parameters:");
                    ImGui::SameLine();
                    std::string bcParamsStr;
                    for (double d : group->boundaryCondition.parameters) {
                        bcParamsStr += std::to_string(d) + ",";
                    }
                    if (!bcParamsStr.empty()) bcParamsStr.pop_back();
                    char bcParamsBuffer[128];
                    strncpy_s(bcParamsBuffer, bcParamsStr.c_str(), sizeof(bcParamsBuffer));
                    if (ImGui::InputText(std::string("##bcParams").c_str(), bcParamsBuffer, sizeof(bcParamsBuffer))) {
                        std::vector<double> params;
                        std::istringstream ss(bcParamsBuffer);
                        std::string token;
                        while (std::getline(ss, token, ',')) {
                            try {
                                double val = std::stod(token);
                                params.push_back(val);
                            } catch (...) {}
                        }
                        group->boundaryCondition.parameters = params;
                    }

                    // Material Properties
                    ImGui::Text("Density:");
                    ImGui::SameLine();
                    ImGui::InputDouble(std::string("##density").c_str(), &group->materialProperties.density, 0.0, 0.0, "%.2f");

                    ImGui::Text("Elastic Modulus:");
                    ImGui::SameLine();
                    ImGui::InputDouble(std::string("##elastic").c_str(), &group->materialProperties.elasticModulus, 0.0, 0.0, "%.2f");

                    ImGui::Text("Poisson Ratio:");
                    ImGui::SameLine();
                    ImGui::InputDouble(std::string("##poisson").c_str(), &group->materialProperties.poissonRatio, 0.0, 0.0, "%.2f");

                    // Element Tags (comma-separated)
                    ImGui::Text("Element Tags:");
                    ImGui::SameLine();
                    std::string tagsStr;
                    for (const auto &tag : group->elementTags) {
                        tagsStr += tag + ",";
                    }
                    if (!tagsStr.empty()) tagsStr.pop_back();
                    char tagsBuffer[128];
                    strncpy_s(tagsBuffer, tagsStr.c_str(), sizeof(tagsBuffer));
                    if (ImGui::InputText(std::string("##tags").c_str(), tagsBuffer, sizeof(tagsBuffer))) {
                        std::vector<std::string> tags;
                        std::istringstream iss(tagsBuffer);
                        std::string token;
                        while (std::getline(iss, token, ',')) {
                            if (!token.empty()) {
                                tags.push_back(token);
                            }
                        }
                        group->elementTags = tags;
                    }

                    // Delete Group (only if more than one group)
                    if (groupNames.size() > 1) {
                        if (ImGui::Button("Delete Group")) {
                            std::string toDelete = group->groupName;
                            mergedMeshMetadata.removeGroupMetadata(toDelete);
                            appendLog("Deleted group: " + toDelete);
                            if (activeGroupName == toDelete) {
                                // Switch active group to any other
                                for (auto &gn : groupNames) {
                                    if (gn != toDelete) {
                                        activeGroupName = gn;
                                        break;
                                    }
                                }
                                pickMode = false;
                                appendLog("Active group changed to " + activeGroupName + ". Please pick faces again.");
                            }
                            // Because we removed the group from the map, we break out of the loop
                            ImGui::PopID();
                            goto endGroupLoop; 
                        }
                    }
                }
                ImGui::PopID();
                i++;
            }
endGroupLoop:;

            // Add Group button
            if (ImGui::Button("Add Group")) {
                std::string newGroupName = "Group " + std::to_string(newGroupCounter++);
                GroupMetadata newGroup;
                newGroup.groupName = newGroupName;
                newGroup.boundaryCondition.type = "fixed";
                newGroup.boundaryCondition.parameters = {0.0};
                newGroup.materialProperties.density = 7850.0;
                newGroup.materialProperties.elasticModulus = 210e9;
                newGroup.materialProperties.poissonRatio = 0.3;
                mergedMeshMetadata.addGroupMetadata(newGroup);
                activeGroupName = newGroupName;
                pickMode = false;
                appendLog("Added new group: " + newGroupName + ". Please pick faces for the new active group.");
            }

            // Export Metadata
            if (ImGui::Button("Export Metadata")) {
                if (!exportCentroidInfo && !exportFaceInfo) {
                    appendLog("No export option selected. Nothing done.");
                } else {
                    Timer timer;
                    bool success = MetadataExporter::exportMetadata(metadataFileName, mergedMeshMetadata);
                    double exportTime = timer.elapsed();
                    if (success)
                        appendLog("Metadata exported to " + std::string(metadataFileName) + " in " + std::to_string(exportTime) + " ms");
                    else
                        appendLog("Failed exporting metadata to " + std::string(metadataFileName));
                }
            }
            ImGui::End();
        }
        
        // --- Handle pick/drag outside of ImGui windows ---
        if (pickMode && !cameraMovementEnabled && !ImGui::GetIO().WantCaptureMouse) {
            if (!dragSelectionActive && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                dragSelectionActive = true;
                dragStart = ImGui::GetMousePos();
                dragEnd = dragStart;
                appendLog("Drag selection started.");
            }
            if (dragSelectionActive && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                dragEnd = ImGui::GetMousePos();
            }
            if (dragSelectionActive && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                dragSelectionActive = false;
                double aspect = (double)display_w / (double)display_h;
                processDragSelection(display_w, display_h, aspect);
                appendLog("Drag selection processed.");
            }
            if (dragSelectionActive) {
                ImDrawList* dl = ImGui::GetForegroundDrawList();
                dl->AddRect(dragStart, dragEnd, IM_COL32(255,0,0,255), 0.0f, 0, 2.0f);
            }
        }
        
        // --- Log Window ---
        ImGui::SetNextWindowPos(ImVec2(10, display_h - 150), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(display_w - 20, 140), ImGuiCond_FirstUseEver);
        ImGui::Begin("Log", nullptr);
        if (ImGui::Button("Clear Log")) {
            logText.clear();
        }
        ImGui::Separator();
        ImGui::TextWrapped("%s", logText.c_str());
        ImGui::End();
        
        // Only apply camera zoom if ImGui is NOT capturing the mouse wheel.
        if (io.MouseWheel != 0.0f && !io.WantCaptureMouse)
        {
            camZoom *= (1.0f + io.MouseWheel * 0.05f);
            if (camZoom < 0.1f) camZoom = 0.1f;
            if (camZoom > 10.0f) camZoom = 10.0f;
        }

        
        // --- Render OpenGL Scene ---
        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        double aspect = (double)display_w / (double)display_h;
        glOrtho(-aspect, aspect, -1, 1, -10, 10);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(camOffsetX, camOffsetY, 0.0f);
        glScalef(camZoom, camZoom, 1.0f);
        glEnable(GL_DEPTH_TEST);

        if (renderMode == 1)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


        // Set grid color and line width
        glColor3f(0.3f, 0.3f, 0.3f); // A subtle grey
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        float gridSpacing = 0.2f;  // Adjust spacing as desired

        // Compute the visible region in world coordinates (inverse of the modelview transformation)
        // The projection is set with: glOrtho(-aspect, aspect, -1, 1, -10, 10)
        // and the modelview applies: glTranslatef(camOffsetX, camOffsetY, 0); glScalef(camZoom, camZoom, 1);
        float xMin = (-aspect) / camZoom - camOffsetX;
        float xMax = (aspect) / camZoom - camOffsetX;
        float yMin = (-1.0f) / camZoom - camOffsetY;
        float yMax = (1.0f) / camZoom - camOffsetY;

        // Align boundaries to grid spacing so the lines "snap" to a grid
        float startX = gridSpacing * floor(xMin / gridSpacing);
        float endX   = gridSpacing * ceil(xMax / gridSpacing);
        float startY = gridSpacing * floor(yMin / gridSpacing);
        float endY   = gridSpacing * ceil(yMax / gridSpacing);

        glColor3f(0.3f, 0.3f, 0.3f); // Subtle grey for grid lines
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        // Draw vertical grid lines
        for (float x = startX; x <= endX; x += gridSpacing) {
            glVertex3f(x, yMin, 0.0f);
            glVertex3f(x, yMax, 0.0f);
        }

        // Draw horizontal grid lines
        for (float y = startY; y <= endY; y += gridSpacing) {
            glVertex3f(xMin, y, 0.0f);
            glVertex3f(xMax, y, 0.0f);
        }
        glEnd();


        for (const auto &sceneMesh : sceneMeshes) {
            if (!sceneMesh.enabled)
                continue;
            for (size_t i = 0; i < sceneMesh.mesh.faces.size(); i++) {
                if (sceneMesh.errorFaces[i])
                    glColor3f(1.0f, 0.0f, 0.0f);
                else
                    glColor3f(0.0f, 1.0f, 0.0f);
                glBegin(GL_POLYGON);
                const Face& face = sceneMesh.mesh.faces[i];
                for (const auto &elem : face.elements) {
                    const Vertex &v = sceneMesh.mesh.vertices[elem.vertexIndex];
                    glVertex3d(v.x, v.y, v.z);
                }
                glEnd();
            }
        }
        glLineWidth(2.0f);
        glBegin(GL_LINES);
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex3d(0.0, 0.0, 0.0);
            glVertex3d(0.5, 0.0, 0.0);
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex3d(0.0, 0.0, 0.0);
            glVertex3d(0.0, 0.5, 0.0);
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex3d(0.0, 0.0, 0.0);
            glVertex3d(0.0, 0.0, 0.5);
        glEnd();
        glLineWidth(1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
