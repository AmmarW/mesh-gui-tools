#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <fstream>
#include <array>

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

#include "MeshConverter.h" // still included if needed elsewhere

// Include new class for Boolean operations
#include "MeshBooleanOperations.h"

// Include AdaptiveMeshGenerator
#include "AdaptiveMeshGenerator.h"

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
// Global variables for viewer controls
bool cameraMovementEnabled = false;
float camOffsetX = 0.0f;
float camOffsetY = 0.0f;
float camZoom = 1.0f;
int renderMode = 0; // 0 = Faces, 1 = Wireframe

// Transformation parameters (defaults: no translation/rotation, unit scale)
double tx = 0.0, ty = 0.0, tz = 0.0;
double sx = 1.0, sy = 1.0, sz = 1.0;
double rx = 0.0, ry = 0.0, rz = 0.0;

// Mesh type selection: 0 = "surface", 1 = "volume", 2 = "both"
const char* meshTypeOptions[] = { "surface", "volume", "both" };
int currentMeshType = 0;

// Output file names for export (for OBJ and metadata)
char outputFileName[128] = "output.obj";
char metadataFileName[128] = "metadata.txt";

// File name for the Boolean operations result (OFF file)
std::string booleanResultFile = "boolean_result.off";
bool booleanOperationPerformed = false;

// Global log text string
std::string logText = "";

// Append a message to the log (each message ends with a newline)
void appendLog(const std::string &msg) {
    logText += msg + "\n";
}

// New structure to hold each mesh and its metadata
struct SceneMesh {
    Mesh mesh;
    std::string filePath;
    std::vector<std::string> validationErrors;
    std::vector<bool> errorFaces;
    double loadTime; // in milliseconds
    bool enabled = true;
};

// A vector holding all loaded meshes in the scene.
std::vector<SceneMesh> sceneMeshes;
// Index of the currently active mesh (-1 means none selected)
int activeMeshIndex = -1;

// AMG parameters (for AdaptiveMeshGenerator Controls)
double amg_sizing_field = 0.7;
double amg_edge_distance = 0.01;
double amg_facet_angle = 25.0;
double amg_facet_distance = 0.01;
double amg_cell_radius_edge_ratio = 3.0;
float amg_cube_size = 10.0f; // Now a float

// Helper function: Convert our Mesh (from ObjParser) to a CGAL Polyhedron (OFF format) in memory.
bool convertMeshToPolyhedron(const Mesh &mesh, MeshBooleanOperations::Polyhedron &poly) {
    std::stringstream ss;
    ss << "OFF\n";
    ss << mesh.vertices.size() << " " << mesh.faces.size() << " 0\n";
    for (const auto &v : mesh.vertices) {
        ss << v.x << " " << v.y << " " << v.z << "\n";
    }
    for (const auto &face : mesh.faces) {
        ss << face.elements.size();
        for (const auto &elem : face.elements) {
            ss << " " << elem.vertexIndex;
        }
        ss << "\n";
    }
    poly.clear();
    if (!(ss >> poly)) {
        std::cerr << "Error converting mesh to Polyhedron" << std::endl;
        return false;
    }
    // Triangulate faces for robust boolean operations.
    CGAL::Polygon_mesh_processing::triangulate_faces(poly);
    return true;
}

// Helper function: Extract filename from full path.
std::string extractFilename(const std::string &fullPath) {
    size_t pos = fullPath.find_last_of("/\\");
    if (pos != std::string::npos)
        return fullPath.substr(pos + 1);
    return fullPath;
}

// Simple conversion function: Reads an OFF file and writes an OBJ file.
bool convertOffToObj(const std::string &offFile, const std::string &objFile) {
    std::ifstream in(offFile);
    if (!in) {
        std::cerr << "Failed to open OFF file: " << offFile << std::endl;
        return false;
    }
    std::string header;
    in >> header;
    if (header != "OFF") {
        std::cerr << "File " << offFile << " is not a valid OFF file." << std::endl;
        return false;
    }
    int nVertices, nFaces, nEdges;
    in >> nVertices >> nFaces >> nEdges;
    std::vector<std::array<double, 3>> vertices(nVertices);
    for (int i = 0; i < nVertices; i++) {
        in >> vertices[i][0] >> vertices[i][1] >> vertices[i][2];
    }
    std::vector<std::vector<int>> faces(nFaces);
    for (int i = 0; i < nFaces; i++) {
        int count;
        in >> count;
        faces[i].resize(count);
        for (int j = 0; j < count; j++) {
            in >> faces[i][j];
        }
    }
    in.close();

    std::ofstream out(objFile);
    if (!out) {
        std::cerr << "Failed to open OBJ file for writing: " << objFile << std::endl;
        return false;
    }
    // Write vertices.
    for (int i = 0; i < nVertices; i++) {
        out << "v " << vertices[i][0] << " " << vertices[i][1] << " " << vertices[i][2] << "\n";
    }
    // Write faces. OBJ indices start at 1.
    for (int i = 0; i < nFaces; i++) {
        out << "f";
        for (int idx : faces[i]) {
            out << " " << (idx + 1);
        }
        out << "\n";
    }
    out.close();
    return true;
}

// Function to convert the boolean result OFF file to OBJ and add it as a new mesh.
void addBooleanOperationMesh(const std::string &offFile) {
    // Use a fixed name for the new OBJ file.
    std::string newObjFile = "boolean_result.obj";
    if (convertOffToObj(offFile, newObjFile)) {
        appendLog("Converted boolean result OFF to OBJ: " + newObjFile);
        SceneMesh newMesh;
        newMesh.filePath = newObjFile;
        try {
            ObjParser parser;
            newMesh.mesh = parser.parse(newObjFile.c_str());
            newMesh.validationErrors = MeshValidator::validate(newMesh.mesh);
            newMesh.errorFaces = getErrorFaces(newMesh.mesh);
            newMesh.loadTime = 0.0;
            sceneMeshes.push_back(newMesh);
            appendLog("Added boolean operation mesh to scene: " + extractFilename(newObjFile));
        } catch (const std::exception &ex) {
            appendLog("Error reading OBJ file " + newObjFile + ": " + std::string(ex.what()));
        }
    } else {
        appendLog("Conversion from OFF to OBJ failed.");
    }
}

int main(int argc, char** argv) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    
    // Create window and OpenGL context
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int width = mode->width;
    int height = mode->height;
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
    
    // Log initial startup message.
    appendLog("Application started.");

    // Variable to track if the Log window is hovered.
    bool logHovered = false;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Start a new ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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

        if (cameraMovementEnabled && !io.WantCaptureMouse && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            camOffsetX += io.MouseDelta.x * 0.005f;
            camOffsetY -= io.MouseDelta.y * 0.005f;
        }
        
        // --- Add OBJ File Window (Below Camera Controls) ---
        ImGui::SetNextWindowPos(ImVec2(10, 70), ImGuiCond_Once);
        ImGui::Begin("Add OBJ File", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Select an OBJ file to add to the scene.");
        if (ImGui::Button("Select OBJ File")) {
            const char* filterPatterns[1] = { "*.obj" };
            const char* filePath = tinyfd_openFileDialog("Select OBJ File", "./", 1, filterPatterns, "OBJ Files", 0);
            if (filePath) {
                SceneMesh newMesh;
                newMesh.filePath = filePath;
                auto startTime = std::chrono::high_resolution_clock::now();
                try {
                    ObjParser parser;
                    newMesh.mesh = parser.parse(filePath);
                    newMesh.validationErrors = MeshValidator::validate(newMesh.mesh);
                    newMesh.errorFaces = getErrorFaces(newMesh.mesh);
                    appendLog("Imported mesh: " + extractFilename(newMesh.filePath));
                } catch (const std::exception &ex) {
                    appendLog("Error parsing OBJ file: " + std::string(ex.what()));
                }
                auto endTime = std::chrono::high_resolution_clock::now();
                newMesh.loadTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
                appendLog("Mesh " + extractFilename(newMesh.filePath) + " loaded in " + std::to_string(newMesh.loadTime) + " ms");
                sceneMeshes.push_back(newMesh);
                if (activeMeshIndex < 0)
                    activeMeshIndex = 0;
            }
        }
        if (ImGui::Button("Clear Meshes")) {
            sceneMeshes.clear();
            activeMeshIndex = -1;
            booleanOperationPerformed = false;
            appendLog("Cleared all meshes.");
        }
        ImGui::End();

        // --- Mesh List Window (Top Right) ---
        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(display_w - 220), 10.0f), ImGuiCond_Once);
        ImGui::Begin("Mesh List", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (sceneMeshes.empty()) {
            ImGui::Text("No meshes loaded.");
        } else {
            for (int i = 0; i < static_cast<int>(sceneMeshes.size()); i++) {
                ImGui::PushID(i);
                // Row 1: Checkbox and Mesh Name
                bool enabled = sceneMeshes[i].enabled;
                if (ImGui::Checkbox("", &enabled))
                    sceneMeshes[i].enabled = enabled;
                ImGui::SameLine();
                std::string meshName = extractFilename(sceneMeshes[i].filePath);
                if (i == activeMeshIndex)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1)); // Yellow for active mesh.
                if (ImGui::Selectable(meshName.c_str(), i == activeMeshIndex))
                    activeMeshIndex = i;
                if (i == activeMeshIndex)
                    ImGui::PopStyleColor();
                // Row 2: Remove button on next line.
                if (ImGui::Button("Remove")) {
                    appendLog("Removed mesh: " + meshName);
                    sceneMeshes.erase(sceneMeshes.begin() + i);
                    if (activeMeshIndex >= static_cast<int>(sceneMeshes.size()))
                        activeMeshIndex = sceneMeshes.empty() ? -1 : 0;
                    ImGui::PopID();
                    break; // Break out of loop to avoid invalid index iteration.
                }
                ImGui::Separator();
                ImGui::PopID();
            }
        }
        ImGui::End();

        // --- Transformation Controls Window ---
        ImGui::SetNextWindowPos(ImVec2(10, 200), ImGuiCond_Once);
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
        ImGui::Combo("Mesh Type", &currentMeshType, meshTypeOptions, IM_ARRAYSIZE(meshTypeOptions));
        
        if (ImGui::Button("Apply Transformations")) {
            if (activeMeshIndex < 0 || sceneMeshes.empty()) {
                appendLog("No active mesh selected for transformation.");
            } else {
                try {
                    SceneMesh &activeMesh = sceneMeshes[activeMeshIndex];
                    ObjParser parser;
                    if (currentMeshType == 0 || currentMeshType == 2)
                        activeMesh.mesh = parser.parseSurfaceMesh(activeMesh.filePath.c_str());
                    else if (currentMeshType == 1)
                        activeMesh.mesh = parser.parseVolumeMesh(activeMesh.filePath.c_str());
                    if (tx != 0.0 || ty != 0.0 || tz != 0.0)
                        MeshTransformer::translate(activeMesh.mesh, tx, ty, tz);
                    if (sx != 1.0 || sy != 1.0 || sz != 1.0)
                        MeshTransformer::scale(activeMesh.mesh, sx, sy, sz);
                    if (rx != 0.0 || ry != 0.0 || rz != 0.0)
                        MeshTransformer::rotate(activeMesh.mesh, rx, ry, rz);
        
                    activeMesh.validationErrors = MeshValidator::validate(activeMesh.mesh);
                    activeMesh.errorFaces = getErrorFaces(activeMesh.mesh);
                    appendLog("Transformations applied to mesh: " + extractFilename(activeMesh.filePath));
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

        // --- Boolean Operations Panel ---
        ImGui::SetNextWindowPos(ImVec2(250, 200), ImGuiCond_Once);
        ImGui::Begin("Boolean Operations", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Perform Boolean Operations on Transformed Meshes:");
        if (ImGui::Button("Union")) {
            std::vector<MeshBooleanOperations::Polyhedron> polyMeshes;
            bool conversionSuccess = true;
            for (const auto &sceneMesh : sceneMeshes) {
                MeshBooleanOperations::Polyhedron poly;
                if (!convertMeshToPolyhedron(sceneMesh.mesh, poly)) {
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
                        // Convert the OFF file to OBJ and add to scene
                        addBooleanOperationMesh(booleanResultFile);
                    } else
                        appendLog("Error writing union result to OFF file.");
                } else
                    appendLog("Union operation failed.");
            }
        }
        if (ImGui::Button("Intersection")) {
            std::vector<MeshBooleanOperations::Polyhedron> polyMeshes;
            bool conversionSuccess = true;
            for (const auto &sceneMesh : sceneMeshes) {
                MeshBooleanOperations::Polyhedron poly;
                if (!convertMeshToPolyhedron(sceneMesh.mesh, poly)) {
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
        }
        if (ImGui::Button("Difference")) {
            std::vector<MeshBooleanOperations::Polyhedron> polyMeshes;
            bool conversionSuccess = true;
            for (const auto &sceneMesh : sceneMeshes) {
                MeshBooleanOperations::Polyhedron poly;
                if (!convertMeshToPolyhedron(sceneMesh.mesh, poly)) {
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
        }
        ImGui::End();
        
        // --- Adaptive Mesh Generator Controls Panel ---
        ImGui::SetNextWindowPos(ImVec2(250, 400), ImGuiCond_Once);
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
                auto startExport = std::chrono::high_resolution_clock::now();
                try {
                    AdaptiveMeshGenerator adaptiveMeshGen;
                    if (adaptiveMeshGen.generateVolumeMesh(booleanResultFile, static_cast<int>(amg_cube_size), metadataFileName))
                        appendLog("Volume mesh generated and exported using AMG parameters.");
                    else
                        appendLog("Failed to generate volume mesh from boolean operation result.");
                } catch (const std::exception &ex) {
                    appendLog("Error exporting volume mesh: " + std::string(ex.what()));
                }
                auto endExport = std::chrono::high_resolution_clock::now();
                double exportProcessingTime = std::chrono::duration<double, std::milli>(endExport - startExport).count();
                appendLog("Volume mesh export processing time: " + std::to_string(exportProcessingTime) + " ms");
            }
        }
        ImGui::End();

        // --- Log Window (Bottom) ---
        ImGui::SetNextWindowPos(ImVec2(10, display_h - 150), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(display_w - 20, 140), ImGuiCond_Always);
        ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoResize);
        bool currentLogHovered = ImGui::IsWindowHovered();
        logHovered = currentLogHovered;
        if (ImGui::Button("Clear Log")) {
            logText.clear();
        }
        ImGui::Separator();
        ImGui::TextWrapped("%s", logText.c_str());
        ImGui::End();
        
        // --- Update Zoom only if Log window is not hovered ---
        if (io.MouseWheel != 0.0f && !logHovered) {
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
        double aspect = static_cast<double>(display_w) / display_h;
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
