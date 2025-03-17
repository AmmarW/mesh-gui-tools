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

#include "MeshConverter.h"

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
int renderMode = 1; // 0 = Faces, 1 = Wireframe

// Transformation parameters (defaults: no translation/rotation, unit scale)
double tx = 0.0, ty = 0.0, tz = 0.0;
double sx = 1.0, sy = 1.0, sz = 1.0;
double rx = 0.0, ry = 0.0, rz = 0.0;

// Mesh type selection: 0 = "surface", 1 = "volume", 2 = "both"
const char* meshTypeOptions[] = { "surface", "volume", "both" };
int currentMeshType = 0;

// Output file names for export (for OBJ and metadata)
char outputFileName[128] = "output.obj";
char metadataFileName[128] = "metadata.json";

// File name for the Boolean operations result (OFF file)
std::string booleanResultFile = "boolean_result.off";
bool booleanOperationPerformed = false;

// Global log text string
std::string logText = "";
void appendLog(const std::string &msg) {
    logText += msg + "\n";
}

// Structure for each mesh in the scene.
struct SceneMesh {
    Mesh mesh;
    std::string filePath;
    std::vector<std::string> validationErrors;
    std::vector<bool> errorFaces;
    double loadTime; // in milliseconds
    bool enabled = true;
};
std::vector<SceneMesh> sceneMeshes;
int activeMeshIndex = -1;

// Global merged metadata for the boolean-merged mesh.
MeshMetadata mergedMeshMetadata;
// Global active group name (for assigning picked faces)
std::string activeGroupName = "Inner";

// AMG parameters (for AdaptiveMeshGenerator Controls)
double amg_sizing_field = 0.7;
double amg_edge_distance = 0.01;
double amg_facet_angle = 25.0;
double amg_facet_distance = 0.01;
double amg_cell_radius_edge_ratio = 3.0;
float amg_cube_size = 10.0f; // as float

// --- Drag Selection Globals ---
bool pickMode = false;      // toggled by a checkbox in the Metadata Assignment panel
bool dragSelectionActive = false;
ImVec2 dragStart, dragEnd;
bool exportCentroidInfo = false;  // If checked, export centroid info
bool exportFaceInfo = false;      // If checked, export face info

// Helper: convert a Mesh to a CGAL Polyhedron (OFF format) in memory.
bool convertMeshToPolyhedron(const Mesh &mesh, MeshBooleanOperations::Polyhedron &poly) {
    std::stringstream ss;
    ss << "OFF\n";
    ss << mesh.vertices.size() << " " << mesh.faces.size() << " 0\n";
    for (const auto &v : mesh.vertices)
        ss << v.x << " " << v.y << " " << v.z << "\n";
    for (const auto &face : mesh.faces) {
        ss << face.elements.size();
        for (const auto &elem : face.elements)
            ss << " " << elem.vertexIndex;
        ss << "\n";
    }
    poly.clear();
    if (!(ss >> poly)) {
        std::cerr << "Error converting mesh to Polyhedron" << std::endl;
        return false;
    }
    CGAL::Polygon_mesh_processing::triangulate_faces(poly);
    return true;
}

// Helper: Extract filename from a full path.
std::string extractFilename(const std::string &fullPath) {
    size_t pos = fullPath.find_last_of("/\\");
    if (pos != std::string::npos)
        return fullPath.substr(pos + 1);
    return fullPath;
}

// Simple function to convert an OFF file to an OBJ file.
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
    for (int i = 0; i < nVertices; i++)
        out << "v " << vertices[i][0] << " " << vertices[i][1] << " " << vertices[i][2] << "\n";
    for (int i = 0; i < nFaces; i++) {
        out << "f";
        for (int idx : faces[i])
            out << " " << (idx + 1);
        out << "\n";
    }
    out.close();
    return true;
}

// Helper: Simple projection of a 3D point into screen coordinates (orthographic).
ImVec2 projectPoint(const std::array<double, 3>& point, double aspect, float zoom,
                    float offsetX, float offsetY, int display_w, int display_h) {
    double x = (point[0] + offsetX) * zoom;
    double y = (point[1] + offsetY) * zoom;
    double ndc_x = (x + aspect) / (2 * aspect);
    double ndc_y = (y + 1.0) / 2.0;
    ImVec2 screenPos;
    screenPos.x = ndc_x * display_w;
    screenPos.y = (1.0 - ndc_y) * display_h; // invert y
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
            booleanOperationPerformed = true;
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

int main(int argc, char** argv) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int width = mode->width, height = mode->height;
    GLFWwindow* window = glfwCreateWindow(width, height, "Mesh Viewer", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    int display_w, display_h;
    appendLog("Application started.");

    static int newGroupCounter = 1; // for naming new groups

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
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
        
        // --- Boolean Operations ---
        ImGui::SetNextWindowPos(ImVec2(250, 200),  ImGuiCond_FirstUseEver);
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
                    if (MetadataExporter::exportMetadata(metadataFileName, mergedMeshMetadata))
                        appendLog("Metadata exported to " + std::string(metadataFileName));
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
        ImGui::SetNextWindowPos(ImVec2(10, display_h - 150), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(display_w - 20, 140), ImGuiCond_Always);
        ImGui::Begin("Log", nullptr, ImGuiWindowFlags_NoResize);
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
