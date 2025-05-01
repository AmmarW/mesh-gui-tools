// main_gui.cpp

#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>  // for std::min_element, std::max_element, and std::min
#include <array>
#include <sstream> // Added include
#include <future>  // Add for std::future and std::future_status

// Define M_PI if not defined (e.g. on MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// HeatStack includes
#include "MeshHandler.h"
#include "InitialTemperature.h"
#include "BoundaryConditions.h"
#include "HeatEquationSolver.h"
#include "TimeHandler.h"
#include "MaterialProperties.h"
#include "TemperatureComparator.h"

// GUI + OpenGL includes
#ifdef _WIN32
#define NOMINMAX // Prevent Windows headers from defining min/max macros
#include <windows.h>
#endif
#include <GL/gl.h>
// #include <GL/glu.h> // Using manual lookAt and perspective to avoid GLU dependency if needed
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "tinyfiledialogs.h"

// Forward declarations to fix reference errors
void drawCoordinateAxes();
void runSimulationLogic();
void renderSimulationControls();
void renderVisualizationControls();
void renderVisualization(int vx, int vy, int vw, int vh, bool isHovered);
void drawMeshWithTemperatures(const MeshHandler& mesh, const HeatEquationSolver& completedSolver);
void drawMeshWithThickness(const MeshHandler& mesh, const HeatEquationSolver& solver);
void drawMeshDefault(const MeshHandler& mesh);
void drawSliceLines(const MeshHandler& mesh);
void drawTemperaturePlot(int x, int y, int width, int height); // Added forward declaration
void updateMeshVisualization(); // Added forward declaration

// 🌟 Global GUI Variables 🌟
char meshPath[512] = "";
char initTempPath[512] = "";
float simDuration = 10.0f;
float timeStep = 0.1f;
bool triggerSimulation = false;
bool simulationCompleted = false;
float progress = 0.0f;
std::string appLog;
std::string currentProcessingStatus = "";

// Add missing inputs to the GUI
static int nSlices = 10;
static int pointsPerLayer = 100;
static bool useAdaptiveTimeStep = false;
static float theta = 0.5f;
static char outputFile[512] = "summary_output.csv";
static bool meshLoadedForVis = false; // Track if mesh is loaded for visualization
static int selectedSlice = 10; // Currently selected slice number for plots (1-based)

// 🌟 Mesh Handler and Solver Objects (keep them persistent)
MeshHandler meshHandler;
HeatEquationSolver solver; // Keep solver instance for potential result access

// Global variables for 3D visualization
bool cameraMovementEnabled = true; // Enable by default
float camDistance = 5.0f;       // Distance from origin (used for zoom)
float camAzimuth = 45.0f;       // Rotation around Y axis (degrees)
float camElevation = 30.0f;     // Rotation around X axis (degrees)
float camTargetX = 0.0f, camTargetY = 0.0f, camTargetZ = 0.0f; // Look-at point (for panning)
bool renderWireframe = false;
bool showMesh = true;           // NEW: Toggle for mesh visibility
const float mouseSensitivity = 0.4f;
const float zoomSensitivity = 0.5f;
const float panSensitivity = 0.01f; // Sensitivity for panning
enum VisualizationMode { TEMPERATURE_VIS, THICKNESS_VIS, LINE_PLOT_VIS };  // Added LINE_PLOT_VIS mode
VisualizationMode currentVisMode = THICKNESS_VIS; // Default to thickness visualization
bool showColorScale = true; // Whether to show the color scale
bool showSliceLines = true; // Add toggle for slice lines
bool autoAdjustCameraOnLoad = true; // Automatically center camera on mesh load

// Reset function to clear all inputs
void resetSimulation() {
    meshPath[0] = '\0';
    initTempPath[0] = '\0';
    simDuration = 10.0f;
    timeStep = 0.1f;
    nSlices = 10;
    pointsPerLayer = 100;
    useAdaptiveTimeStep = false;
    theta = 0.5f;
    strcpy_s(outputFile, sizeof(outputFile), "summary_output.csv"); // Use strcpy_s for safety
    progress = 0.0f;
    appLog.clear();
    currentProcessingStatus.clear();
    simulationCompleted = false;
    triggerSimulation = false;
    meshLoadedForVis = false; // Reset mesh loaded flag
    // Reset camera ? Optional, might be nice to keep view
    // camDistance = 5.0f;
    // camAzimuth = 45.0f;
    // camElevation = 30.0f;
}

// Helper function for perspective projection
void perspectiveGL(GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar) {
    const GLdouble pi = 3.1415926535897932384626433832795;
    GLdouble fH = tan(fovY / 360 * pi) * zNear;
    GLdouble fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

// Helper function for lookAt (manual implementation to avoid GLU dependency)
void lookAtGL(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ,
              GLdouble centerX, GLdouble centerY, GLdouble centerZ,
              GLdouble upX, GLdouble upY, GLdouble upZ)
{
    GLdouble f[3], up[3], s[3], u[3];
    GLdouble m[16];

    f[0] = centerX - eyeX;
    f[1] = centerY - eyeY;
    f[2] = centerZ - eyeZ;

    up[0] = upX;
    up[1] = upY;
    up[2] = upZ;

    // Normalize f
    GLdouble len = sqrt(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
    if (len != 0) { f[0] /= len; f[1] /= len; f[2] /= len; }

    // Calculate s = f x up
    s[0] = f[1] * up[2] - f[2] * up[1];
    s[1] = f[2] * up[0] - f[0] * up[2];
    s[2] = f[0] * up[1] - f[1] * up[0];

    // Normalize s
    len = sqrt(s[0]*s[0] + s[1]*s[1] + s[2]*s[2]);
    if (len != 0) { s[0] /= len; s[1] /= len; s[2] /= len; }

    // Calculate u = s x f
    u[0] = s[1] * f[2] - s[2] * f[1];
    u[1] = s[2] * f[0] - s[0] * f[2];
    u[2] = s[0] * f[1] - s[1] * f[0];

    // Set matrix
    m[0] = s[0]; m[4] = s[1]; m[8] = s[2];  m[12] = 0.0;
    m[1] = u[0]; m[5] = u[1]; m[9] = u[2];  m[13] = 0.0;
    m[2] = -f[0]; m[6] = -f[1]; m[10] = -f[2]; m[14] = 0.0;
    m[3] = 0.0;  m[7] = 0.0;  m[11] = 0.0;  m[15] = 1.0;

    glMultMatrixd(m);
    glTranslated(-eyeX, -eyeY, -eyeZ);
}

// Helper function to swap Y and Z axes when drawing vertices
void drawVertexWithYZSwap(const std::array<float, 3>& vertex) {
    // Swap Y and Z coordinates to make Z the vertical axis
    double x = static_cast<double>(vertex[0]);
    double y = static_cast<double>(vertex[2]); // Use Z as Y (vertical)
    double z = static_cast<double>(vertex[1]); // Use Y as Z (depth)
    glVertex3d(x, y, z);
}

// Helper function to calculate normal with Y-Z swap
std::array<double, 3> calculateNormalWithYZSwap(const std::array<float, 3>& v1, 
                                               const std::array<float, 3>& v2, 
                                               const std::array<float, 3>& v3) {
    // Convert coordinates with Y-Z swap for normal calculation
    double v1x = v1[0], v1y = v1[2], v1z = v1[1]; // Swap Y and Z
    double v2x = v2[0], v2y = v2[2], v2z = v2[1]; // Swap Y and Z
    double v3x = v3[0], v3y = v3[2], v3z = v3[1]; // Swap Y and Z
    
    // Calculate normal for the swapped coordinates
    double nx = (v2y - v1y) * (v3z - v1z) - (v2z - v1z) * (v3y - v1y);
    double ny = (v2z - v1z) * (v3x - v1x) - (v2x - v1x) * (v3z - v1z);
    double nz = (v2x - v1x) * (v3y - v1y) - (v2y - v1y) * (v3x - v1x);
    
    // Normalize
    double len = sqrt(nx*nx + ny*ny + nz*nz);
    if (len > 0) {
        nx /= len;
        ny /= len;
        nz /= len;
    }
    
    return {nx, ny, nz};
}

// Helper function to draw coordinate axes
void drawCoordinateAxes() {
    // Make axes smaller and relative to mesh size? For now, fixed size.
    float axisLength = 0.5f; // Adjust as needed
    glLineWidth(2.0f);
    glBegin(GL_LINES);
        // X axis (red) - horizontal
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(axisLength, 0.0f, 0.0f);
        
        // Y axis (green) - horizontal (changed from blue to green)
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, axisLength, 0.0f);
        
        // Z axis (blue) - vertical (changed from green to blue)
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, axisLength);
    glEnd();
    glLineWidth(1.0f);
}

// Helper function to update visualization based on the currently loaded mesh
void updateMeshVisualization() {
    if (meshLoadedForVis && autoAdjustCameraOnLoad) {
        // Calculate mesh bounds
        float minX = meshHandler.getMinX();
        float maxX = meshHandler.getMaxX();
        float minY = meshHandler.getMinY();
        float maxY = meshHandler.getMaxY();
        float minZ = meshHandler.getMinZ();
        float maxZ = meshHandler.getMaxZ();
        
        // Calculate center of the mesh
        float centerX = (minX + maxX) / 2.0f;
        float centerY = (minY + maxY) / 2.0f;
        float centerZ = (minZ + maxZ) / 2.0f;
        
        // Set camera target to the mesh center
        camTargetX = centerX;
        camTargetY = centerY;
        camTargetZ = centerZ;
        
        // Calculate appropriate distance based on mesh size
        float sizeX = maxX - minX;
        float sizeY = maxY - minY;
        float sizeZ = maxZ - minZ;
        float size = std::max(sizeX, std::max(sizeY, sizeZ));
        
        // Set camera distance based on mesh size
        camDistance = size * 1.5f;
        if (camDistance < 1.0f) camDistance = 1.0f;
        if (camDistance > 20.0f) camDistance = 20.0f;
        
        // Reset auto-adjust flag so it only happens once per load
        autoAdjustCameraOnLoad = false;
    }
}

// ---- GUI Simulation Controls ----
void renderSimulationControls() {
    // This window will be explicitly sized and positioned in the main loop
    ImGui::Begin("HeatStack Simulation");

    if (ImGui::InputText("Mesh Path", meshPath, sizeof(meshPath))) {
        meshLoadedForVis = false; // Reset vis flag if path changes
        simulationCompleted = false; // Results are invalid if mesh changes
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse Mesh")) {
        const char* filter[] = { "*.obj", "*.csv" };
        const char* file = tinyfd_openFileDialog("Select Mesh", "", 2, filter, "OBJ or CSV files", 0);
        if (file) {
            strncpy_s(meshPath, sizeof(meshPath), file, _TRUNCATE);
            meshLoadedForVis = false; // Reset vis flag
            simulationCompleted = false; // Results are invalid
        }
    }

    ImGui::InputText("Initial Temp (.csv)", initTempPath, sizeof(initTempPath));
    ImGui::SameLine();
    if (ImGui::Button("Browse Temp")) {
        const char* filter[] = { "*.csv" };
        const char* file = tinyfd_openFileDialog("Select Initial Temp CSV", "", 1, filter, "CSV files", 0);
        if (file) strncpy_s(initTempPath, sizeof(initTempPath), file, _TRUNCATE);
    }

    ImGui::InputFloat("Duration (s)", &simDuration, 0.1f, 1.0f, "%.1f");
    ImGui::InputFloat("Time Step (s)", &timeStep, 0.01f, 0.1f, "%.3f");
    ImGui::InputInt("Number of Slices", &nSlices);
    ImGui::InputInt("Points Per Layer", &pointsPerLayer);
    ImGui::Checkbox("Use Adaptive Time Step", &useAdaptiveTimeStep);
    ImGui::InputFloat("Theta Parameter", &theta, 0.05f, 0.1f, "%.2f");
    ImGui::InputText("Output File", outputFile, sizeof(outputFile));

    // Clamp inputs to reasonable values
    if (simDuration <= 0) simDuration = 0.1f;
    if (timeStep <= 0) timeStep = 0.001f;
    if (nSlices < 1) nSlices = 1;
    if (pointsPerLayer < 2) pointsPerLayer = 2;
    theta = std::clamp(theta, 0.0f, 1.0f); // Clamp theta [0, 1]

    if (triggerSimulation) {
        ImGui::BeginDisabled(true); // Disable button while running
        ImGui::Button("Running...");
        ImGui::EndDisabled();
    } else {
       if (ImGui::Button("Run Simulation")) {
            triggerSimulation = true; // This will trigger the logic in the main loop
            simulationCompleted = false;
            progress = 0.0f;
            appLog.clear();
            currentProcessingStatus = "Starting simulation...";
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset All")) {
        resetSimulation();
    }

    ImGui::Separator();
    ImGui::ProgressBar(progress, ImVec2(-1, 0));
    if (!currentProcessingStatus.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", currentProcessingStatus.c_str());
    }

    // Log window
    ImGui::Separator();
    ImGui::Text("Log:");
    ImGui::BeginChild("LogScrollingRegion", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 8), true, ImGuiWindowFlags_HorizontalScrollbar); // Limit height
    ImGui::TextUnformatted(appLog.c_str());
     if(ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) // Auto-scroll
           ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::End(); // End Simulation Controls Window
}

// Helper function to draw the mesh with temperature colors (only if simulation completed)
void drawMeshWithTemperatures(const MeshHandler& mesh, const HeatEquationSolver& completedSolver) {
    const auto& vertices = mesh.getVertices();
    const auto& faces = mesh.getFaces();
    const auto& Tdist = completedSolver.getTemperatureDistribution(); // Use the completed solver results

    if (vertices.empty() || faces.empty() || Tdist.empty()) return;

    // Calculate exact temperature range from temperature distribution
    double minTemp = *std::min_element(Tdist.begin(), Tdist.end());
    double maxTemp = *std::max_element(Tdist.begin(), Tdist.end());
    
    double tempRange = maxTemp - minTemp;
    if (tempRange <= 0) tempRange = 1.0; // Avoid division by zero

    // After Y-Z swap, we want to use Y coordinates (visualized as Z) for mapping
    // Calculate min and max Y for mapping, which becomes Z in visualization
    double yMin = mesh.getMinY(); // This is the min of what becomes Z in visualization
    double yMax = mesh.getMaxY(); // This is the max of what becomes Z in visualization
    double depthRange = yMax - yMin;
    
    if (depthRange <= 0) depthRange = 1.0; // Avoid division by zero

    glBegin(GL_TRIANGLES);
    for (const auto& face : faces) {
        // Get vertices for this face
        const auto& v1 = vertices[face[0]];
        const auto& v2 = vertices[face[1]];
        const auto& v3 = vertices[face[2]];

        // Calculate normal with Y-Z swapped coordinates
        auto normal = calculateNormalWithYZSwap(v1, v2, v3);
        glNormal3d(normal[0], normal[1], normal[2]); // Set normal per face

        // Calculate average Y position of this face (becomes Z in visualization)
        double avgY = (v1[1] + v2[1] + v3[1]) / 3.0;
        
        // Map Y position to normalized position in distribution array
        double normalizedPos = (avgY - yMin) / depthRange;
        normalizedPos = std::clamp(normalizedPos, 0.0, 1.0);
        
        // Calculate temperature at this position by interpolating from distribution
        int index = static_cast<int>(normalizedPos * (Tdist.size() - 1));
        int nextIndex = std::min(index + 1, static_cast<int>(Tdist.size() - 1));
        double fraction = normalizedPos * (Tdist.size() - 1) - index;
        
        double temp;
        if (index == nextIndex) {
            temp = Tdist[index];
        } else {
            temp = Tdist[index] * (1 - fraction) + Tdist[nextIndex] * fraction;
        }
        
        // Convert temperature to color (red=hot to blue=cold)
        float t = static_cast<float>((temp - minTemp) / tempRange);
        t = std::clamp(t, 0.0f, 1.0f); // Clamp [0, 1]
        
        // Apply color to all vertices of this face
        glColor3f(t, 0.0f, 1.0f - t);  // Red to Blue gradient

        // Draw all vertices with Y-Z swap
        for (int i = 0; i < 3; ++i) {
            drawVertexWithYZSwap(vertices[face[i]]);
        }
    }
    glEnd();
}

// Helper function to draw mesh with TPU thickness visualization
void drawMeshWithThickness(const MeshHandler& mesh, const HeatEquationSolver& solver) {
    const auto& vertices = mesh.getVertices();
    const auto& faces = mesh.getFaces();

    if (vertices.empty() || faces.empty()) return;

    // After Y-Z swap, we want to use Y coordinates (visualized as Z) for color mapping
    double yMin = mesh.getMinY(); // Min of new Z axis (former Y)
    double yMax = mesh.getMaxY(); // Max of new Z axis (former Y)
    double depthRange = yMax - yMin;
    
    if (depthRange <= 0) depthRange = 1.0; // Avoid division by zero

    // Load TPS thickness data from summary file if available
    struct SliceData {
        int sliceNumber;  // Add slice number for tracking
        double lL;
        double tpsThickness;
    };
    std::vector<SliceData> sliceData;
    
    // Try to load from summary_output.csv first (contains optimal TPS thickness)
    std::ifstream summaryFile("summary_output.csv");
    if (summaryFile.is_open()) {
        std::string line;
        // Skip header
        std::getline(summaryFile, line);
        
        while (std::getline(summaryFile, line)) {
            std::stringstream ss(line);
            std::string token;
            
            // Read slice number
            std::getline(ss, token, ',');
            int sliceNum = std::stoi(token);
            
            // Read l/L
            std::getline(ss, token, ',');
            double lL = std::stod(token);
            
            // Skip method
            std::getline(ss, token, ',');
            
            // Skip final steel temperature
            std::getline(ss, token, ',');
            
            // Read TPS thickness
            std::getline(ss, token, ',');
            double tpsThickness = std::stod(token);
            
            // Ensure we have a positive value (sometimes optimization returns negative values)
            if (tpsThickness > 0) {
                sliceData.push_back({sliceNum, lL, tpsThickness});
            }
        }
        summaryFile.close();
    }

    // If summary_output.csv didn't work, try stack_details.csv (contains initial TPS thickness)
    if (sliceData.empty()) {
        std::ifstream detailsFile("stack_details.csv");
        if (detailsFile.is_open()) {
            std::string line;
            // Skip header
            std::getline(detailsFile, line);
            
            while (std::getline(detailsFile, line)) {
                std::stringstream ss(line);
                std::string token;
                
                // Read slice number
                std::getline(ss, token, ',');
                int sliceNum = std::stoi(token);
                
                // Read l/L
                std::getline(ss, token, ',');
                double lL = std::stod(token);
                
                // Read TPS thickness
                std::getline(ss, token, ',');
                double tpsThickness = std::stod(token);
                
                sliceData.push_back({sliceNum, lL, tpsThickness});
            }
            detailsFile.close();
        }
    }

    // If no data was loaded from CSV files, use default values without logging errors
    if (sliceData.empty()) {
        // Default thickness values as a fallback - ensure we cover the ENTIRE mesh
        for (int i = 0; i < nSlices; i++) {
            // Reverse the order so slice 1 is at the bottom (l/L = 0.0)
            double lL = (nSlices > 1) ? static_cast<double>(i) / (nSlices - 1) : 0.5;
            sliceData.push_back({nSlices - i, lL, 0.001}); // Reverse slice number
        }
    }

    // Find min/max thickness for color mapping directly from sliceData
    double minThickness = DBL_MAX;
    double maxThickness = -DBL_MAX;
    
    for (const auto& slice : sliceData) {
        if (slice.tpsThickness < minThickness) minThickness = slice.tpsThickness;
        if (slice.tpsThickness > maxThickness) maxThickness = slice.tpsThickness;
    }
    
    if (minThickness == DBL_MAX) minThickness = 0.0;
    double thicknessRange = maxThickness - minThickness;
    if (thicknessRange <= 0) thicknessRange = 1.0; // Avoid division by zero
    
    // Sort slice data by l/L value (from start to end)
    std::sort(sliceData.begin(), sliceData.end(), 
             [](const SliceData& a, const SliceData& b) { return a.lL < b.lL; });
    
    // Calculate slice boundaries along the Y axis (which becomes Z in visualization)
    // NOTE: We reverse the slice order so that slice 1 is at the bottom (yMin)
    std::vector<double> sliceBoundaries;
    if (nSlices > 1) {
        // Use nSlices+1 boundaries to define nSlices regions
        for (int i = 0; i <= nSlices; i++) {
            double boundary = yMin + (static_cast<double>(i) / nSlices) * depthRange;
            sliceBoundaries.push_back(boundary);
        }
    } else {
        // If only one slice, use the entire Y range
        sliceBoundaries.push_back(yMin);
        sliceBoundaries.push_back(yMax);
    }

    // Draw mesh with color based on TPS thickness
    glBegin(GL_TRIANGLES);
    for (const auto& face : faces) {
        // Get vertices for this face
        const auto& v1 = vertices[face[0]];
        const auto& v2 = vertices[face[1]];
        const auto& v3 = vertices[face[2]];

        // Calculate normal with Y-Z swapped coordinates
        auto normal = calculateNormalWithYZSwap(v1, v2, v3);
        glNormal3d(normal[0], normal[1], normal[2]);

        // Determine the average Y position of this face (becomes Z in visualization)
        double avgY = (v1[1] + v2[1] + v3[1]) / 3.0;
        
        // Find which slice this face belongs to
        int sliceIndex = 0;
        for (size_t i = 0; i < sliceBoundaries.size() - 1; i++) {
            if (avgY >= sliceBoundaries[i] && avgY <= sliceBoundaries[i+1]) {
                sliceIndex = i;
                break;
            }
        }
        
        // Map slice index to l/L value - use evenly distributed l/L values
        // But reverse the mapping so that slice 1 (bottom) has l/L = 0
        double sliceLL = (nSlices > 1) ? 
                      static_cast<double>(sliceIndex) / (nSlices - 1) : 
                      0.5;
        
        // Find the corresponding slice data - need to match reversed slice numbering
        double thickness = 0.001; // Default
        int reversedSliceIndex = nSlices - 1 - sliceIndex; // Convert to the right slice number
        int actualSliceNumber = sliceIndex + 1; // Original slice number (1-based)
        
        for (const auto& slice : sliceData) {
            // Use approximate comparison for floating point
            if (fabs(slice.lL - sliceLL) < 0.01 || 
                (slice.sliceNumber == actualSliceNumber)) {
                thickness = slice.tpsThickness;
                break;
            }
        }
        
        // Map thickness to color (yellow to green)
        float t = static_cast<float>((thickness - minThickness) / thicknessRange);
        t = std::clamp(t, 0.0f, 1.0f);
        
        // Yellow (thick) to Green (thin) gradient
        glColor3f(1.0f - t, 1.0f, 0.0f);
        
        // Draw all vertices with Y-Z swap
        for (int i = 0; i < 3; ++i) {
            drawVertexWithYZSwap(vertices[face[i]]);
        }
    }
    glEnd();
}

// Helper function to draw horizontal slice planes at different Z positions
void drawSliceLines(const MeshHandler& mesh) {
    if (!meshLoadedForVis || nSlices <= 1) return;

    // Calculate min and max bounds of the mesh directly from MeshHandler
    // After Y-Z swap, Y becomes Z and Z becomes Y in the visualization
    double xMin = mesh.getMinX();
    double xMax = mesh.getMaxX();
    double yMin = mesh.getMinY(); // This becomes zMin in visualization
    double yMax = mesh.getMaxY(); // This becomes zMax in visualization (fixed typo)
    double zMin = mesh.getMinZ(); // This becomes yMin in visualization
    double zMax = mesh.getMaxZ(); // This becomes yMax in visualization
    
    // We want to slice along the new Z axis (which was Y before the swap)
    double depthMin = yMin; // Min of new Z axis (former Y)
    double depthMax = yMax; // Max of new Z axis (former Y)
    double depthRange = depthMax - depthMin;
    
    if (depthRange <= 0) depthRange = 1.0; // Avoid division by zero

    // Extend boundaries a bit for better visibility
    double xExtend = (xMax - xMin) * 0.1;
    double yExtend = (zMax - zMin) * 0.1; // Y in visualization is the former Z
    xMin -= xExtend;
    xMax += xExtend;
    double yMin_viz = zMin - yExtend; // Y in visualization is former Z
    double yMax_viz = zMax + yExtend;

    // Calculate slice positions along the NEW Z-axis (former Y) - depth axis after swap
    std::vector<double> sliceZPositions; // Z in visualization is the former Y
    for (int slice = 0; slice < nSlices; ++slice) {
        // Ensure we cover the ENTIRE range from depthMin to depthMax inclusively
        double depth = depthMin + ((nSlices > 1) ? (double(slice) / (nSlices - 1)) * depthRange : depthRange / 2.0);
        sliceZPositions.push_back(depth); // This Y value becomes Z in visualization
    }

    // Save the current OpenGL state
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST); // Disable depth testing so lines are visible
    glDisable(GL_LIGHTING);   // Disable lighting for lines

    // Set line properties
    glLineWidth(2.0f);        // Increase line width for visibility
    
    // Draw slice planes as vertical quads with slight transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    for (double z_vis : sliceZPositions) {
        // Draw semi-transparent quad for each slice plane at depth Z
        // After Y-Z swap, the Y axis in simulation becomes the Z axis in visualization
        glBegin(GL_QUADS);
        glColor4f(0.0f, 0.8f, 0.8f, 0.2f); // Cyan with 20% opacity
        glVertex3d(xMin, yMin_viz, z_vis); // Z is the depth in visualization (was Y)
        glVertex3d(xMax, yMin_viz, z_vis); 
        glVertex3d(xMax, yMax_viz, z_vis);
        glVertex3d(xMin, yMax_viz, z_vis);
        glEnd();
        
        // Draw the outline of the slice plane
        glBegin(GL_LINE_LOOP);
        glColor3f(0.0f, 1.0f, 1.0f); // Solid cyan for the outline
        glVertex3d(xMin, yMin_viz, z_vis);
        glVertex3d(xMax, yMin_viz, z_vis);
        glVertex3d(xMax, yMax_viz, z_vis);
        glVertex3d(xMin, yMax_viz, z_vis);
        glEnd();
    }
    
    glDisable(GL_BLEND);

    // Restore OpenGL state
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST); // Restore depth testing state
    }
}

// Helper function to draw the mesh with default material
void drawMeshDefault(const MeshHandler& mesh) {
    const auto& vertices = mesh.getVertices();
    const auto& faces = mesh.getFaces();

     if (vertices.empty() || faces.empty()) return;

    // Set a default material color (e.g., grey)
    glColor3f(0.7f, 0.7f, 0.7f);

    // Use basic lighting materials
    GLfloat material_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
    GLfloat material_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat material_specular[] = { 0.2f, 0.2f, 0.2f, 1.0f }; // Less shiny
    GLfloat material_shininess = 10.0f; // Lower shininess

    glMaterialfv(GL_FRONT, GL_AMBIENT, material_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, material_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, material_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, material_shininess);

    glBegin(GL_TRIANGLES);
    for (const auto& face : faces) {
         // Calculate face normal for proper lighting (flat shading)
         const auto& v1 = vertices[face[0]];
         const auto& v2 = vertices[face[1]];
         const auto& v3 = vertices[face[2]];

         auto normal = calculateNormalWithYZSwap(v1, v2, v3);
         glNormal3d(normal[0], normal[1], normal[2]); // Set normal once per face

        for (int i = 0; i < 3; ++i) {
            const auto& vertex = vertices[face[i]];
            drawVertexWithYZSwap(vertex);
        }
    }
    glEnd();
}

// Helper function to draw a color scale for temperature or thickness visualization
void drawColorScale(int x, int y, int width, int height, bool isTemperature) {
    // Draw background
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::Begin("##colorscale", nullptr, 
                ImGuiWindowFlags_NoTitleBar | 
                ImGuiWindowFlags_NoResize | 
                ImGuiWindowFlags_NoMove | 
                ImGuiWindowFlags_NoScrollbar);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Get window position for absolute coordinates
    ImVec2 pos = ImGui::GetCursorScreenPos();
    
    // Draw gradient
    const int barHeight = height - 60; // Leave room for text
    const int barWidth = 30;
    const int startX = pos.x + 10;
    const int startY = pos.y + 10;
    
    // Draw gradient bar
    for (int i = 0; i < barHeight; i++) {
        float t = 1.0f - (float)i / barHeight; // 1.0 at top, 0.0 at bottom
        ImVec4 color;
        if (isTemperature) {
            // Red to Blue gradient for temperature
            color = ImVec4(t, 0.0f, 1.0f - t, 1.0f);
        } else {
            // Yellow to Green gradient for thickness (matching the mesh visualization)
            color = ImVec4(1.0f - t, 1.0f, 0.0f, 1.0f);
        }
        ImU32 col32 = ImGui::ColorConvertFloat4ToU32(color);
        draw_list->AddRectFilled(
            ImVec2(startX, startY + i), 
            ImVec2(startX + barWidth, startY + i + 1), 
            col32);
    }
    
    // Draw labels
    draw_list->AddRect(
        ImVec2(startX, startY), 
        ImVec2(startX + barWidth, startY + barHeight), 
        IM_COL32_WHITE);
    
    if (isTemperature) {
        // Get min/max temperature from solver if available
        double minTemp = 273.0; // Default min (0°C)
        double maxTemp = solver.getTemperatureDistribution().empty() ? 
                        800.0 : solver.getTemperatureDistribution().back(); // Default max
        
        // Draw temperature labels
        std::string maxLabel = "Max: " + std::to_string((int)maxTemp) + "K";
        std::string minLabel = "Min: " + std::to_string((int)minTemp) + "K";
        draw_list->AddText(ImVec2(startX + barWidth + 5, startY), IM_COL32_WHITE, maxLabel.c_str());
        draw_list->AddText(ImVec2(startX + barWidth + 5, startY + barHeight - 15), IM_COL32_WHITE, minLabel.c_str());
        
        // Title
        ImGui::SetCursorPos(ImVec2(10, barHeight + 20));
        ImGui::Text("Temperature (K)");
    } else {
        // Get min/max TPU thickness from the simulation data
        double minThickness = DBL_MAX;
        double maxThickness = -DBL_MAX;

        std::ifstream summaryFile("summary_output.csv");
        if (summaryFile.is_open()) {
            std::string line;
            std::getline(summaryFile, line); // Skip header
            while (std::getline(summaryFile, line)) {
                std::stringstream ss(line);
                std::string token;
                std::getline(ss, token, ','); // slice
                std::getline(ss, token, ','); // l/L
                std::getline(ss, token, ','); // method
                std::getline(ss, token, ','); // finalSteelTemp
                std::getline(ss, token, ','); // TPS_thickness
                double thickness = std::stod(token);
                if (thickness > 0) {
                    if (thickness < minThickness) minThickness = thickness;
                    if (thickness > maxThickness) maxThickness = thickness;
                }
            }
            summaryFile.close();
        } else {
            std::ifstream detailsFile("stack_details.csv");
            if (detailsFile.is_open()) {
                std::string line;
                std::getline(detailsFile, line); // Skip header
                while (std::getline(detailsFile, line)) {
                    std::stringstream ss(line);
                    std::string token;
                    std::getline(ss, token, ','); // slice
                    std::getline(ss, token, ','); // l/L
                    std::getline(ss, token, ','); // TPS_thickness
                    double thickness = std::stod(token);
                    if (thickness < minThickness) minThickness = thickness;
                    if (thickness > maxThickness) maxThickness = thickness;
                }
                detailsFile.close();
            }
        }

        if (minThickness == DBL_MAX) minThickness = 0.0;
        if (maxThickness == -DBL_MAX) maxThickness = 0.001; // Default max if no data

        // Draw thickness labels
        std::string maxLabel = "Max: " + std::to_string(maxThickness).substr(0, 8) + "m";
        std::string minLabel = "Min: " + std::to_string(minThickness).substr(0, 8) + "m";
        draw_list->AddText(ImVec2(startX + barWidth + 5, startY), IM_COL32_WHITE, maxLabel.c_str());
        draw_list->AddText(ImVec2(startX + barWidth + 5, startY + barHeight - 15), IM_COL32_WHITE, minLabel.c_str());
        
        // Title
        ImGui::SetCursorPos(ImVec2(10, barHeight + 20));
        ImGui::Text("TPU Thickness (m)");
    }
    
    ImGui::End();
}

// Helper function to draw temperature line plot
void drawTemperaturePlot(int x, int y, int width, int height) {
    // Position and size the plot window
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::Begin("Temperature Plots", nullptr, 
                ImGuiWindowFlags_NoMove | 
                ImGuiWindowFlags_NoResize | 
                ImGuiWindowFlags_NoCollapse);

    // Create a 2x2 grid layout
    const float plotWidth = (width - 80) / 2;  // Two plots per row with spacing
    const float plotHeight = (height - 100) / 2; // Two rows with spacing
    const float horizontalSpacing = 40;
    const float verticalSpacing = 20;
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Common plot settings
    const float axisOffsetX = 50;  // Left margin for y-axis labels
    const float axisOffsetY = 30;  // Bottom margin for x-axis labels
    const float tickSize = 5.0f;
    const ImU32 axisColor = IM_COL32(255, 255, 255, 255); // White

    // ---- LOAD DATA ----
    // Time history data structure
    struct TimeHistoryData {
        std::vector<double> time;
        std::vector<double> carbonGlueTemp;
        std::vector<double> glueSteelTemp;
        std::vector<double> steelTemp;
    };

    TimeHistoryData origData, optData;

    // Load original time history data
    std::ifstream origHistFile("time_history_orig_slice_" + std::to_string(selectedSlice) + ".csv");
    if (origHistFile.is_open()) {
        std::string line;
        // Skip header
        std::getline(origHistFile, line);
        
        while (std::getline(origHistFile, line)) {
            std::stringstream ss(line);
            std::string token;
            
            // Read time
            std::getline(ss, token, ',');
            origData.time.push_back(std::stod(token));
            
            // Read carbon-glue temperature
            std::getline(ss, token, ',');
            origData.carbonGlueTemp.push_back(std::stod(token));
            
            // Read glue-steel temperature
            std::getline(ss, token, ',');
            origData.glueSteelTemp.push_back(std::stod(token));
            
            // Read steel temperature
            std::getline(ss, token, ',');
            origData.steelTemp.push_back(std::stod(token));
        }
        origHistFile.close();
    }

    // Load optimized time history data
    std::ifstream optHistFile("time_history_opt_slice_" + std::to_string(selectedSlice) + ".csv");
    if (optHistFile.is_open()) {
        std::string line;
        // Skip header
        std::getline(optHistFile, line);
        
        while (std::getline(optHistFile, line)) {
            std::stringstream ss(line);
            std::string token;
            
            // Read time
            std::getline(ss, token, ',');
            optData.time.push_back(std::stod(token));
            
            // Read carbon-glue temperature
            std::getline(ss, token, ',');
            optData.carbonGlueTemp.push_back(std::stod(token));
            
            // Read glue-steel temperature
            std::getline(ss, token, ',');
            optData.glueSteelTemp.push_back(std::stod(token));
            
            // Read steel temperature
            std::getline(ss, token, ',');
            optData.steelTemp.push_back(std::stod(token));
        }
        optHistFile.close();
    }

    // Final temperature data structure
    struct FinalTempData {
        std::vector<double> positions;
        std::vector<double> temperatures;
    };

    FinalTempData finalOrigData, finalOptData;

    // Load original final temperature data
    std::ifstream finalOrigFile("final_temperature_orig_slice_" + std::to_string(selectedSlice) + ".csv");
    if (finalOrigFile.is_open()) {
        std::string line;
        // Skip header
        std::getline(finalOrigFile, line);
        
        while (std::getline(finalOrigFile, line)) {
            std::stringstream ss(line);
            std::string token;
            
            // Read position
            std::getline(ss, token, ',');
            finalOrigData.positions.push_back(std::stod(token));
            
            // Read temperature
            std::getline(ss, token, ',');
            finalOrigData.temperatures.push_back(std::stod(token));
        }
        finalOrigFile.close();
    }

    // Load optimized final temperature data
    std::ifstream finalOptFile("final_temperature_opt_slice_" + std::to_string(selectedSlice) + ".csv");
    if (finalOptFile.is_open()) {
        std::string line;
        // Skip header
        std::getline(finalOptFile, line);
        
        while (std::getline(finalOptFile, line)) {
            std::stringstream ss(line);
            std::string token;
            
            // Read position
            std::getline(ss, token, ',');
            finalOptData.positions.push_back(std::stod(token));
            
            // Read temperature
            std::getline(ss, token, ',');
            finalOptData.temperatures.push_back(std::stod(token));
        }
        finalOptFile.close();
    }

    // Try to load material layer info from stack_details.csv for marking interfaces
    std::vector<std::pair<std::string, double>> layerBoundaries;
    std::ifstream detailsFile("stack_details.csv");
    if (detailsFile.is_open()) {
        // Read header to get column positions
        std::string header;
        std::getline(detailsFile, header);
        std::stringstream headerSS(header);
        std::string col;
        std::vector<std::string> columns;
        
        while (std::getline(headerSS, col, ',')) {
            columns.push_back(col);
        }
        
        // Find the row for selected slice
        std::string dataRow;
        while (std::getline(detailsFile, dataRow)) {
            std::stringstream ss(dataRow);
            std::string sliceValue;
            std::getline(ss, sliceValue, ',');
            
            if (sliceValue == std::to_string(selectedSlice)) { // Found selected slice
                std::stringstream dataSS(dataRow);
                std::vector<std::string> values;
                std::string val;
                
                while (std::getline(dataSS, val, ',')) {
                    values.push_back(val);
                }
                
                // Get layer thicknesses - Original TPS, CarbonFiber, Glue, Steel
                if (values.size() >= 6) {
                    double position = 0.0;
                    double tpsThick = std::stod(values[2]);
                    double cfThick = std::stod(values[3]);
                    double glueThick = std::stod(values[4]);
                    double steelThick = std::stod(values[5]);
                    
                    layerBoundaries.push_back({"TPS Start", position});
                    position += tpsThick;
                    layerBoundaries.push_back({"Carbon Fiber Start", position});
                    position += cfThick;
                    layerBoundaries.push_back({"Glue Start", position});
                    position += glueThick;
                    layerBoundaries.push_back({"Steel Start", position});
                    position += steelThick;
                    layerBoundaries.push_back({"End", position});
                }
                
                break; // Found what we needed
            }
        }
        detailsFile.close();
    }

    // ---- FIRST PLOT: TIME HISTORY (ORIGINAL) ----
    ImVec2 plot1Pos = ImGui::GetCursorScreenPos();
    
    // Title
    ImGui::Text("Time History (Original) - Slice %d", selectedSlice);
    ImGui::SameLine();
    ImGui::SetCursorPosX(width/2 + 20);
    ImGui::Text("Time History (Optimized) - Slice %d", selectedSlice);
    
    ImGui::SetCursorPos(ImVec2(10, ImGui::GetCursorPosY() + 5));

    // Draw plot area background
    drawList->AddRectFilled(
        ImVec2(plot1Pos.x + axisOffsetX, plot1Pos.y + 25),
        ImVec2(plot1Pos.x + plotWidth, plot1Pos.y + 25 + plotHeight - axisOffsetY),
        IM_COL32(30, 30, 30, 255)
    );
    
    // Plot 1: Original Time History
    bool haveOrigTimeData = !origData.time.empty();
    
    if (!haveOrigTimeData) {
        ImGui::SetCursorPos(ImVec2(plot1Pos.x - ImGui::GetWindowPos().x + 50, 
                                 plot1Pos.y - ImGui::GetWindowPos().y + plotHeight/2));
        ImGui::Text("No original time history data available for slice %d.", selectedSlice);
    } else {
        // Calculate plot ranges for time history (original)
        double timeMinOrig = *std::min_element(origData.time.begin(), origData.time.end());
        double timeMaxOrig = *std::max_element(origData.time.begin(), origData.time.end());
        
        double tempMinOrig = std::min({
            *std::min_element(origData.carbonGlueTemp.begin(), origData.carbonGlueTemp.end()),
            *std::min_element(origData.glueSteelTemp.begin(), origData.glueSteelTemp.end()),
            *std::min_element(origData.steelTemp.begin(), origData.steelTemp.end())
        });
        
        double tempMaxOrig = std::max({
            *std::max_element(origData.carbonGlueTemp.begin(), origData.carbonGlueTemp.end()),
            *std::max_element(origData.glueSteelTemp.begin(), origData.glueSteelTemp.end()),
            *std::max_element(origData.steelTemp.begin(), origData.steelTemp.end())
        });
        
        // Add some margin to the y-range
        double tempMarginOrig = (tempMaxOrig - tempMinOrig) * 0.05;
        if (tempMarginOrig < 1.0) tempMarginOrig = 1.0;
        tempMinOrig -= tempMarginOrig;
        tempMaxOrig += tempMarginOrig;
        
        // Draw axes for original time history plot
        // Y-axis
        drawList->AddLine(
            ImVec2(plot1Pos.x + axisOffsetX, plot1Pos.y + 25),
            ImVec2(plot1Pos.x + axisOffsetX, plot1Pos.y + 25 + plotHeight - axisOffsetY),
            axisColor, 1.5f
        );
        
        // X-axis
        drawList->AddLine(
            ImVec2(plot1Pos.x + axisOffsetX, plot1Pos.y + 25 + plotHeight - axisOffsetY),
            ImVec2(plot1Pos.x + plotWidth, plot1Pos.y + 25 + plotHeight - axisOffsetY),
            axisColor, 1.5f
        );
        
        // Draw axis labels
        ImGui::SetCursorPos(ImVec2(10, 60));
        ImGui::Text("Temp (K)");
        
        ImGui::SetCursorPos(ImVec2(plotWidth/2 - 30, plotHeight + 10));
        ImGui::Text("Time (s)");

        // Draw ticks and labels on axes
        // Y-axis ticks (5 ticks)
        for (int i = 0; i <= 5; i++) {
            float y = plot1Pos.y + 25 + (plotHeight - axisOffsetY) - (i * (plotHeight - axisOffsetY) / 5);
            double tempValue = tempMinOrig + (i * (tempMaxOrig - tempMinOrig) / 5);
            
            // Tick
            drawList->AddLine(
                ImVec2(plot1Pos.x + axisOffsetX - tickSize, y),
                ImVec2(plot1Pos.x + axisOffsetX, y),
                axisColor
            );
            
            // Label
            char label[32];
            snprintf(label, sizeof(label), "%.0f", tempValue);
            drawList->AddText(
                ImVec2(plot1Pos.x + axisOffsetX - 40, y - 8),
                axisColor, label
            );
        }
        
        // X-axis ticks (5 ticks)
        for (int i = 0; i <= 5; i++) {
            float x = plot1Pos.x + axisOffsetX + (i * (plotWidth - axisOffsetX) / 5);
            double timeValue = timeMinOrig + (i * (timeMaxOrig - timeMinOrig) / 5);
            
            // Tick
            drawList->AddLine(
                ImVec2(x, plot1Pos.y + 25 + plotHeight - axisOffsetY),
                ImVec2(x, plot1Pos.y + 25 + plotHeight - axisOffsetY + tickSize),
                axisColor
            );
            
            // Label
            char label[32];
            snprintf(label, sizeof(label), "%.1f", timeValue);
            drawList->AddText(
                ImVec2(x - 15, plot1Pos.y + 25 + plotHeight - axisOffsetY + 8),
                axisColor, label
            );
        }

        // Define colors for different temperature lines
        const ImU32 colorsOrig[] = {
            IM_COL32(255, 0, 0, 255),     // Red - carbon-glue
            IM_COL32(0, 255, 0, 255),     // Green - glue-steel
            IM_COL32(0, 0, 255, 255)      // Blue - steel
        };

        // Draw the legend for original time history plot
        const float legendX = plot1Pos.x + plotWidth - 150;
        const float legendY = plot1Pos.y + 35;
        const float legendLineLength = 20.0f;
        const float legendSpacing = 20.0f;
        
        drawList->AddLine(
            ImVec2(legendX, legendY), 
            ImVec2(legendX + legendLineLength, legendY), 
            colorsOrig[0], 2.0f);
        drawList->AddText(
            ImVec2(legendX + legendLineLength + 5, legendY - 8),
            axisColor, "Carbon-Glue");
            
        drawList->AddLine(
            ImVec2(legendX, legendY + legendSpacing), 
            ImVec2(legendX + legendLineLength, legendY + legendSpacing), 
            colorsOrig[1], 2.0f);
        drawList->AddText(
            ImVec2(legendX + legendLineLength + 5, legendY + legendSpacing - 8),
            axisColor, "Glue-Steel");
            
        drawList->AddLine(
            ImVec2(legendX, legendY + 2*legendSpacing), 
            ImVec2(legendX + legendLineLength, legendY + 2*legendSpacing), 
            colorsOrig[2], 2.0f);
        drawList->AddText(
            ImVec2(legendX + legendLineLength + 5, legendY + 2*legendSpacing - 8),
            axisColor, "Steel");
        
        // Draw original carbon-glue line
        std::vector<ImVec2> carbonGluePoints;
        for (size_t i = 0; i < origData.time.size(); i++) {
            float x = plot1Pos.x + axisOffsetX + ((origData.time[i] - timeMinOrig) / (timeMaxOrig - timeMinOrig)) * 
                    (plotWidth - axisOffsetX);
            float y = plot1Pos.y + 25 + (plotHeight - axisOffsetY) - 
                    ((origData.carbonGlueTemp[i] - tempMinOrig) / (tempMaxOrig - tempMinOrig)) * 
                    (plotHeight - axisOffsetY);
            carbonGluePoints.push_back(ImVec2(x, y));
        }
        
        for (size_t i = 0; i < carbonGluePoints.size() - 1; i++) {
            drawList->AddLine(carbonGluePoints[i], carbonGluePoints[i+1], colorsOrig[0], 2.0f);
        }
        
        // Draw original glue-steel line
        std::vector<ImVec2> glueSteelPoints;
        for (size_t i = 0; i < origData.time.size(); i++) {
            float x = plot1Pos.x + axisOffsetX + ((origData.time[i] - timeMinOrig) / (timeMaxOrig - timeMinOrig)) * 
                    (plotWidth - axisOffsetX);
            float y = plot1Pos.y + 25 + (plotHeight - axisOffsetY) - 
                    ((origData.glueSteelTemp[i] - tempMinOrig) / (tempMaxOrig - tempMinOrig)) * 
                    (plotHeight - axisOffsetY);
            glueSteelPoints.push_back(ImVec2(x, y));
        }
        
        for (size_t i = 0; i < glueSteelPoints.size() - 1; i++) {
            drawList->AddLine(glueSteelPoints[i], glueSteelPoints[i+1], colorsOrig[1], 2.0f);
        }
        
        // Draw original steel line
        std::vector<ImVec2> steelPoints;
        for (size_t i = 0; i < origData.time.size(); i++) {
            float x = plot1Pos.x + axisOffsetX + ((origData.time[i] - timeMinOrig) / (timeMaxOrig - timeMinOrig)) * 
                    (plotWidth - axisOffsetX);
            float y = plot1Pos.y + 25 + (plotHeight - axisOffsetY) - 
                    ((origData.steelTemp[i] - tempMinOrig) / (tempMaxOrig - tempMinOrig)) * 
                    (plotHeight - axisOffsetY);
            steelPoints.push_back(ImVec2(x, y));
        }
        
        for (size_t i = 0; i < steelPoints.size() - 1; i++) {
            drawList->AddLine(steelPoints[i], steelPoints[i+1], colorsOrig[2], 2.0f);
        }
    }

    // ---- SECOND PLOT: TIME HISTORY (OPTIMIZED) ----
    ImVec2 plot2Pos = ImVec2(plot1Pos.x + plotWidth + horizontalSpacing, plot1Pos.y);
    
    // Draw plot area background
    drawList->AddRectFilled(
        ImVec2(plot2Pos.x + axisOffsetX, plot2Pos.y + 25),
        ImVec2(plot2Pos.x + plotWidth, plot2Pos.y + 25 + plotHeight - axisOffsetY),
        IM_COL32(30, 30, 30, 255)
    );
    
    // Plot 2: Optimized Time History
    bool haveOptTimeData = !optData.time.empty();
    
    if (!haveOptTimeData) {
        ImGui::SetCursorPos(ImVec2(plot2Pos.x - ImGui::GetWindowPos().x + 50, 
                                 plot2Pos.y - ImGui::GetWindowPos().y + plotHeight/2));
        ImGui::Text("No optimized time history data available for slice %d.", selectedSlice);
    } else {
        // Calculate plot ranges for time history (optimized)
        double timeMinOpt = *std::min_element(optData.time.begin(), optData.time.end());
        double timeMaxOpt = *std::max_element(optData.time.begin(), optData.time.end());
        
        double tempMinOpt = std::min({
            *std::min_element(optData.carbonGlueTemp.begin(), optData.carbonGlueTemp.end()),
            *std::min_element(optData.glueSteelTemp.begin(), optData.glueSteelTemp.end()),
            *std::min_element(optData.steelTemp.begin(), optData.steelTemp.end())
        });
        
        double tempMaxOpt = std::max({
            *std::max_element(optData.carbonGlueTemp.begin(), optData.carbonGlueTemp.end()),
            *std::max_element(optData.glueSteelTemp.begin(), optData.glueSteelTemp.end()),
            *std::max_element(optData.steelTemp.begin(), optData.steelTemp.end())
        });
        
        // Add some margin to the y-range
        double tempMarginOpt = (tempMaxOpt - tempMinOpt) * 0.05;
        if (tempMarginOpt < 1.0) tempMarginOpt = 1.0;
        tempMinOpt -= tempMarginOpt;
        tempMaxOpt += tempMarginOpt;

        // Draw axes for optimized time history plot
        // Y-axis
        drawList->AddLine(
            ImVec2(plot2Pos.x + axisOffsetX, plot2Pos.y + 25),
            ImVec2(plot2Pos.x + axisOffsetX, plot2Pos.y + 25 + plotHeight - axisOffsetY),
            axisColor, 1.5f
        );
        
        // X-axis
        drawList->AddLine(
            ImVec2(plot2Pos.x + axisOffsetX, plot2Pos.y + 25 + plotHeight - axisOffsetY),
            ImVec2(plot2Pos.x + plotWidth, plot2Pos.y + 25 + plotHeight - axisOffsetY),
            axisColor, 1.5f
        );
        
        // Draw axis labels
        ImGui::SetCursorPos(ImVec2(width/2 + 10, 60));
        ImGui::Text("Temp (K)");
        
        ImGui::SetCursorPos(ImVec2(width/2 + plotWidth/2 - 30, plotHeight + 10));
        ImGui::Text("Time (s)");

        // Draw ticks and labels on axes
        // Y-axis ticks (5 ticks)
        for (int i = 0; i <= 5; i++) {
            float y = plot2Pos.y + 25 + (plotHeight - axisOffsetY) - (i * (plotHeight - axisOffsetY) / 5);
            double tempValue = tempMinOpt + (i * (tempMaxOpt - tempMinOpt) / 5);
            
            // Tick
            drawList->AddLine(
                ImVec2(plot2Pos.x + axisOffsetX - tickSize, y),
                ImVec2(plot2Pos.x + axisOffsetX, y),
                axisColor
            );
            
            // Label
            char label[32];
            snprintf(label, sizeof(label), "%.0f", tempValue);
            drawList->AddText(
                ImVec2(plot2Pos.x + axisOffsetX - 40, y - 8),
                axisColor, label
            );
        }
        
        // X-axis ticks (5 ticks)
        for (int i = 0; i <= 5; i++) {
            float x = plot2Pos.x + axisOffsetX + (i * (plotWidth - axisOffsetX) / 5);
            double timeValue = timeMinOpt + (i * (timeMaxOpt - timeMinOpt) / 5);
            
            // Tick
            drawList->AddLine(
                ImVec2(x, plot2Pos.y + 25 + plotHeight - axisOffsetY),
                ImVec2(x, plot2Pos.y + 25 + plotHeight - axisOffsetY + tickSize),
                axisColor
            );
            
            // Label
            char label[32];
            snprintf(label, sizeof(label), "%.1f", timeValue);
            drawList->AddText(
                ImVec2(x - 15, plot2Pos.y + 25 + plotHeight - axisOffsetY + 8),
                axisColor, label
            );
        }

        // Define colors for optimized temperature lines (same as original but brighter)
        const ImU32 colorsOpt[] = {
            IM_COL32(255, 80, 80, 255),   // Light red - carbon-glue
            IM_COL32(80, 255, 80, 255),   // Light green - glue-steel
            IM_COL32(80, 80, 255, 255)    // Light blue - steel
        };

        // Draw the legend for optimized time history plot
        const float legendX = plot2Pos.x + plotWidth - 150;
        const float legendY = plot2Pos.y + 35;
        const float legendLineLength = 20.0f;
        const float legendSpacing = 20.0f;
        
        drawList->AddLine(
            ImVec2(legendX, legendY), 
            ImVec2(legendX + legendLineLength, legendY), 
            colorsOpt[0], 2.0f);
        drawList->AddText(
            ImVec2(legendX + legendLineLength + 5, legendY - 8),
            axisColor, "Carbon-Glue");
            
        drawList->AddLine(
            ImVec2(legendX, legendY + legendSpacing), 
            ImVec2(legendX + legendLineLength, legendY + legendSpacing), 
            colorsOpt[1], 2.0f);
        drawList->AddText(
            ImVec2(legendX + legendLineLength + 5, legendY + legendSpacing - 8),
            axisColor, "Glue-Steel");
            
        drawList->AddLine(
            ImVec2(legendX, legendY + 2*legendSpacing), 
            ImVec2(legendX + legendLineLength, legendY + 2*legendSpacing), 
            colorsOpt[2], 2.0f);
        drawList->AddText(
            ImVec2(legendX + legendLineLength + 5, legendY + 2*legendSpacing - 8),
            axisColor, "Steel");

        // Draw optimized carbon-glue line
        std::vector<ImVec2> carbonGluePoints;
        for (size_t i = 0; i < optData.time.size(); i++) {
            float x = plot2Pos.x + axisOffsetX + ((optData.time[i] - timeMinOpt) / (timeMaxOpt - timeMinOpt)) * 
                    (plotWidth - axisOffsetX);
            float y = plot2Pos.y + 25 + (plotHeight - axisOffsetY) - 
                    ((optData.carbonGlueTemp[i] - tempMinOpt) / (tempMaxOpt - tempMinOpt)) * 
                    (plotHeight - axisOffsetY);
            carbonGluePoints.push_back(ImVec2(x, y));
        }
        
        for (size_t i = 0; i < carbonGluePoints.size() - 1; i++) {
            drawList->AddLine(carbonGluePoints[i], carbonGluePoints[i+1], colorsOpt[0], 2.0f);
        }
        
        // Draw optimized glue-steel line
        std::vector<ImVec2> glueSteelPoints;
        for (size_t i = 0; i < optData.time.size(); i++) {
            float x = plot2Pos.x + axisOffsetX + ((optData.time[i] - timeMinOpt) / (timeMaxOpt - timeMinOpt)) * 
                    (plotWidth - axisOffsetX);
            float y = plot2Pos.y + 25 + (plotHeight - axisOffsetY) - 
                    ((optData.glueSteelTemp[i] - tempMinOpt) / (tempMaxOpt - tempMinOpt)) * 
                    (plotHeight - axisOffsetY);
            glueSteelPoints.push_back(ImVec2(x, y));
        }
        
        for (size_t i = 0; i < glueSteelPoints.size() - 1; i++) {
            drawList->AddLine(glueSteelPoints[i], glueSteelPoints[i+1], colorsOpt[1], 2.0f);
        }
        
        // Draw optimized steel line
        std::vector<ImVec2> steelPoints;
        for (size_t i = 0; i < optData.time.size(); i++) {
            float x = plot2Pos.x + axisOffsetX + ((optData.time[i] - timeMinOpt) / (timeMaxOpt - timeMinOpt)) * 
                    (plotWidth - axisOffsetX);
            float y = plot2Pos.y + 25 + (plotHeight - axisOffsetY) - 
                    ((optData.steelTemp[i] - tempMinOpt) / (tempMaxOpt - tempMinOpt)) * 
                    (plotHeight - axisOffsetY);
            steelPoints.push_back(ImVec2(x, y));
        }
        
        for (size_t i = 0; i < steelPoints.size() - 1; i++) {
            drawList->AddLine(steelPoints[i], steelPoints[i+1], colorsOpt[2], 2.0f);
        }
    }

    // Move cursor down for next row of plots
    ImGui::SetCursorPosY(plotHeight + 50);

    // ---- THIRD PLOT: FINAL TEMPERATURE (ORIGINAL) ----
    ImVec2 plot3Pos = ImVec2(plot1Pos.x, plot1Pos.y + plotHeight + verticalSpacing);
    
    // Titles for bottom row
    ImGui::Text("Final Temperature (Original) - Slice %d", selectedSlice);
    ImGui::SameLine();
    ImGui::SetCursorPosX(width/2 + 20);
    ImGui::Text("Final Temperature (Optimized) - Slice %d", selectedSlice);
    
    ImGui::SetCursorPos(ImVec2(10, ImGui::GetCursorPosY() + 5));
    
    // Draw plot area background
    drawList->AddRectFilled(
        ImVec2(plot3Pos.x + axisOffsetX, plot3Pos.y + 25),
        ImVec2(plot3Pos.x + plotWidth, plot3Pos.y + 25 + plotHeight - axisOffsetY),
        IM_COL32(30, 30, 30, 255)
    );
    
    // Plot 3: Original Final Temperature
    bool haveOrigFinalData = !finalOrigData.positions.empty();
    
    if (!haveOrigFinalData) {
        ImGui::SetCursorPos(ImVec2(plot3Pos.x - ImGui::GetWindowPos().x + 50, 
                                 plot3Pos.y - ImGui::GetWindowPos().y + plotHeight/2));
        ImGui::Text("No original final temperature data available for slice %d.", selectedSlice);
    } else {
        // Calculate plot ranges for final temperature (original)
        double posMinOrig = *std::min_element(finalOrigData.positions.begin(), finalOrigData.positions.end());
        double posMaxOrig = *std::max_element(finalOrigData.positions.begin(), finalOrigData.positions.end());
        
        double finalTempMinOrig = *std::min_element(finalOrigData.temperatures.begin(), finalOrigData.temperatures.end());
        double finalTempMaxOrig = *std::max_element(finalOrigData.temperatures.begin(), finalOrigData.temperatures.end());
        
        // Add some margin to the y-range
        double tempMarginOrig = (finalTempMaxOrig - finalTempMinOrig) * 0.05;
        if (tempMarginOrig < 1.0) tempMarginOrig = 1.0;
        finalTempMinOrig -= tempMarginOrig;
        finalTempMaxOrig += tempMarginOrig;

        // Draw axes for original final temperature plot
        // Y-axis
        drawList->AddLine(
            ImVec2(plot3Pos.x + axisOffsetX, plot3Pos.y + 25),
            ImVec2(plot3Pos.x + axisOffsetX, plot3Pos.y + 25 + plotHeight - axisOffsetY),
            axisColor, 1.5f
        );
        
        // X-axis
        drawList->AddLine(
            ImVec2(plot3Pos.x + axisOffsetX, plot3Pos.y + 25 + plotHeight - axisOffsetY),
            ImVec2(plot3Pos.x + plotWidth, plot3Pos.y + 25 + plotHeight - axisOffsetY),
            axisColor, 1.5f
        );
        
        // Draw axis labels
        ImGui::SetCursorPos(ImVec2(10, plotHeight + 60));
        ImGui::Text("Temp (K)");
        
        ImGui::SetCursorPos(ImVec2(plotWidth/2 - 40, height - 30));
        ImGui::Text("Position (m)");

        // Draw ticks and labels on axes
        // Y-axis ticks (5 ticks)
        for (int i = 0; i <= 5; i++) {
            float y = plot3Pos.y + 25 + (plotHeight - axisOffsetY) - (i * (plotHeight - axisOffsetY) / 5);
            double tempValue = finalTempMinOrig + (i * (finalTempMaxOrig - finalTempMinOrig) / 5);
            
            // Tick
            drawList->AddLine(
                ImVec2(plot3Pos.x + axisOffsetX - tickSize, y),
                ImVec2(plot3Pos.x + axisOffsetX, y),
                axisColor
            );
            
            // Label
            char label[32];
            snprintf(label, sizeof(label), "%.0f", tempValue);
            drawList->AddText(
                ImVec2(plot3Pos.x + axisOffsetX - 40, y - 8),
                axisColor, label
            );
        }
        
        // X-axis ticks (5 ticks)
        for (int i = 0; i <= 5; i++) {
            float x = plot3Pos.x + axisOffsetX + (i * (plotWidth - axisOffsetX) / 5);
            double posValue = posMinOrig + (i * (posMaxOrig - posMinOrig) / 5);
            
            // Tick
            drawList->AddLine(
                ImVec2(x, plot3Pos.y + 25 + plotHeight - axisOffsetY),
                ImVec2(x, plot3Pos.y + 25 + plotHeight - axisOffsetY + tickSize),
                axisColor
            );
            
            // Label
            char label[32];
            snprintf(label, sizeof(label), "%.3f", posValue);
            drawList->AddText(
                ImVec2(x - 15, plot3Pos.y + 25 + plotHeight - axisOffsetY + 8),
                axisColor, label
            );
        }

        // Define color for original final temperature line
        const ImU32 finalTempColorOrig = IM_COL32(255, 100, 0, 255); // Orange

        // Draw original final temperature line
        std::vector<ImVec2> points;
        for (size_t i = 0; i < finalOrigData.positions.size(); i++) {
            float x = plot3Pos.x + axisOffsetX + ((finalOrigData.positions[i] - posMinOrig) / (posMaxOrig - posMinOrig)) * 
                    (plotWidth - axisOffsetX);
            float y = plot3Pos.y + 25 + (plotHeight - axisOffsetY) - 
                    ((finalOrigData.temperatures[i] - finalTempMinOrig) / (finalTempMaxOrig - finalTempMinOrig)) * 
                    (plotHeight - axisOffsetY);
            points.push_back(ImVec2(x, y));
        }
        
        for (size_t i = 0; i < points.size() - 1; i++) {
            drawList->AddLine(points[i], points[i+1], finalTempColorOrig, 2.0f);
        }
        
        // Draw material layer boundaries on original plot
        if (!layerBoundaries.empty()) {
            for (const auto& [name, position] : layerBoundaries) {
                // Only draw if within the viewable range
                if (position >= posMinOrig && position <= posMaxOrig) {
                    // Calculate screen x-coordinate for this position
                    float screenX = plot3Pos.x + axisOffsetX + 
                        ((position - posMinOrig) / (posMaxOrig - posMinOrig)) * (plotWidth - axisOffsetX);
                    
                    // Draw vertical line at layer boundary
                    ImU32 boundaryColor = IM_COL32(200, 200, 200, 128); // Light gray, semi-transparent
                    drawList->AddLine(
                        ImVec2(screenX, plot3Pos.y + 25),
                        ImVec2(screenX, plot3Pos.y + 25 + plotHeight - axisOffsetY),
                        boundaryColor
                    );
                    
                    // Add layer name (keep it compact)
                    drawList->AddText(
                        ImVec2(screenX - 20, plot3Pos.y + 35),
                        IM_COL32(200, 200, 200, 180),
                        name.c_str()
                    );
                }
            }
        }
    }

    // ---- FOURTH PLOT: FINAL TEMPERATURE (OPTIMIZED) ----
    ImVec2 plot4Pos = ImVec2(plot3Pos.x + plotWidth + horizontalSpacing, plot3Pos.y);
    
    // Draw plot area background
    drawList->AddRectFilled(
        ImVec2(plot4Pos.x + axisOffsetX, plot4Pos.y + 25),
        ImVec2(plot4Pos.x + plotWidth, plot4Pos.y + 25 + plotHeight - axisOffsetY),
        IM_COL32(30, 30, 30, 255)
    );
    
    // Plot 4: Optimized Final Temperature
    bool haveOptFinalData = !finalOptData.positions.empty();
    
    if (!haveOptFinalData) {
        ImGui::SetCursorPos(ImVec2(plot4Pos.x - ImGui::GetWindowPos().x + 50, 
                                 plot4Pos.y - ImGui::GetWindowPos().y + plotHeight/2));
        ImGui::Text("No optimized final temperature data available for slice %d.", selectedSlice);
    } else {
        // Calculate plot ranges for final temperature (optimized)
        double posMinOpt = *std::min_element(finalOptData.positions.begin(), finalOptData.positions.end());
        double posMaxOpt = *std::max_element(finalOptData.positions.begin(), finalOptData.positions.end());
        
        double finalTempMinOpt = *std::min_element(finalOptData.temperatures.begin(), finalOptData.temperatures.end());
        double finalTempMaxOpt = *std::max_element(finalOptData.temperatures.begin(), finalOptData.temperatures.end());
        
        // Add some margin to the y-range
        double tempMarginOpt = (finalTempMaxOpt - finalTempMinOpt) * 0.05;
        if (tempMarginOpt < 1.0) tempMarginOpt = 1.0;
        finalTempMinOpt -= tempMarginOpt;
        finalTempMaxOpt += tempMarginOpt;

        // Draw axes for optimized final temperature plot
        // Y-axis
        drawList->AddLine(
            ImVec2(plot4Pos.x + axisOffsetX, plot4Pos.y + 25),
            ImVec2(plot4Pos.x + axisOffsetX, plot4Pos.y + 25 + plotHeight - axisOffsetY),
            axisColor, 1.5f
        );
        
        // X-axis
        drawList->AddLine(
            ImVec2(plot4Pos.x + axisOffsetX, plot4Pos.y + 25 + plotHeight - axisOffsetY),
            ImVec2(plot4Pos.x + plotWidth, plot4Pos.y + 25 + plotHeight - axisOffsetY),
            axisColor, 1.5f
        );
        
        // Draw axis labels
        ImGui::SetCursorPos(ImVec2(width/2 + 10, plotHeight + 60));
        ImGui::Text("Temp (K)");
        
        ImGui::SetCursorPos(ImVec2(width/2 + plotWidth/2 - 40, height - 30));
        ImGui::Text("Position (m)");

        // Draw ticks and labels on axes
        // Y-axis ticks (5 ticks)
        for (int i = 0; i <= 5; i++) {
            float y = plot4Pos.y + 25 + (plotHeight - axisOffsetY) - (i * (plotHeight - axisOffsetY) / 5);
            double tempValue = finalTempMinOpt + (i * (finalTempMaxOpt - finalTempMinOpt) / 5);
            
            // Tick
            drawList->AddLine(
                ImVec2(plot4Pos.x + axisOffsetX - tickSize, y),
                ImVec2(plot4Pos.x + axisOffsetX, y),
                axisColor
            );
            
            // Label
            char label[32];
            snprintf(label, sizeof(label), "%.0f", tempValue);
            drawList->AddText(
                ImVec2(plot4Pos.x + axisOffsetX - 40, y - 8),
                axisColor, label
            );
        }
        
        // X-axis ticks (5 ticks)
        for (int i = 0; i <= 5; i++) {
            float x = plot4Pos.x + axisOffsetX + (i * (plotWidth - axisOffsetX) / 5);
            double posValue = posMinOpt + (i * (posMaxOpt - posMinOpt) / 5);
            
            // Tick
            drawList->AddLine(
                ImVec2(x, plot4Pos.y + 25 + plotHeight - axisOffsetY),
                ImVec2(x, plot4Pos.y + 25 + plotHeight - axisOffsetY + tickSize),
                axisColor
            );
            
            // Label
            char label[32];
            snprintf(label, sizeof(label), "%.3f", posValue);
            drawList->AddText(
                ImVec2(x - 15, plot4Pos.y + 25 + plotHeight - axisOffsetY + 8),
                axisColor, label
            );
        }

        // Define color for optimized final temperature line
        const ImU32 finalTempColorOpt = IM_COL32(0, 200, 255, 255); // Cyan

        // Draw optimized final temperature line
        std::vector<ImVec2> points;
        for (size_t i = 0; i < finalOptData.positions.size(); i++) {
            float x = plot4Pos.x + axisOffsetX + ((finalOptData.positions[i] - posMinOpt) / (posMaxOpt - posMinOpt)) * 
                    (plotWidth - axisOffsetX);
            float y = plot4Pos.y + 25 + (plotHeight - axisOffsetY) - 
                    ((finalOptData.temperatures[i] - finalTempMinOpt) / (finalTempMaxOpt - finalTempMinOpt)) * 
                    (plotHeight - axisOffsetY);
            points.push_back(ImVec2(x, y));
        }
        
        for (size_t i = 0; i < points.size() - 1; i++) {
            drawList->AddLine(points[i], points[i+1], finalTempColorOpt, 2.0f);
        }
        
        // Draw material layer boundaries on optimized plot
        if (!layerBoundaries.empty()) {
            for (const auto& [name, position] : layerBoundaries) {
                // Only draw if within the viewable range
                if (position >= posMinOpt && position <= posMaxOpt) {
                    // Calculate screen x-coordinate for this position
                    float screenX = plot4Pos.x + axisOffsetX + 
                        ((position - posMinOpt) / (posMaxOpt - posMinOpt)) * (plotWidth - axisOffsetX);
                    
                    // Draw vertical line at layer boundary
                    ImU32 boundaryColor = IM_COL32(200, 200, 200, 128); // Light gray, semi-transparent
                    drawList->AddLine(
                        ImVec2(screenX, plot4Pos.y + 25),
                        ImVec2(screenX, plot4Pos.y + 25 + plotHeight - axisOffsetY),
                        boundaryColor
                    );
                    
                    // Add layer name (keep it compact)
                    drawList->AddText(
                        ImVec2(screenX - 20, plot4Pos.y + 35),
                        IM_COL32(200, 200, 200, 180),
                        name.c_str()
                    );
                }
            }
        }
    }

    ImGui::End();
}

// ---- Simulation Logic ----
void runSimulationLogic() {
    try {
        if (strlen(meshPath) == 0) {
            appLog += "❌ Error: Please select a mesh file.\n";
            triggerSimulation = false; // Stop trigger if error
            return;
        }

        // === TIMERS ===
        double tMeshLoad = 0.0;
        double tInitTempLoad = 0.0;
        double tStackSetup = 0.0;
        double tOrigSolve = 0.0;
        double tHistOrigSave = 0.0;
        double tOptSolve = 0.0;
        double tOptSuggestion = 0.0;
        double tHistOptSave = 0.0;
        double tSummaryDetailsWrite = 0.0;

        // overall timer
        auto overall_start = std::chrono::high_resolution_clock::now();

        // Use the MeshHandler constructor with filename parameter
        auto start = std::chrono::high_resolution_clock::now();
        meshHandler = MeshHandler(meshPath); // Use the constructor that takes a filename
        tMeshLoad = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
        
        // Check if mesh is properly loaded
        if (meshHandler.getVertices().empty() || meshHandler.getFaces().empty()) {
            appLog += "❌ Error: Failed to load mesh file or mesh is empty: " + std::string(meshPath) + "\n";
            triggerSimulation = false; // Stop trigger if error
            meshLoadedForVis = false;
            return;
        }
        meshLoadedForVis = true; // Mark mesh as loaded for visualization
        appLog += "✅ Mesh loaded successfully for simulation.\n";

        // Get mesh bounds - use safe accessors with error checking
        double zmin = meshHandler.getMinZ();
        double zmax = meshHandler.getMaxZ();
        double height = zmax - zmin;
        if (height <= 0) height = 1.0; // Avoid division by zero if mesh is flat

        // ---- Initial temperature loading ----
        auto init_start = std::chrono::high_resolution_clock::now();
        std::vector<double> uniformInit;
        if (strlen(initTempPath) > 0) {
            InitialTemperature tempLoader;
            if (!std::filesystem::exists(initTempPath)) {
                appLog += "⚠️ Warning: Initial temperature file not found: " + std::string(initTempPath) + ". Using default 300K.\n";
            } else {
                uniformInit = tempLoader.loadInitialTemperature(initTempPath);
                if (uniformInit.empty()) {
                    appLog += "⚠️ Warning: Failed to load or empty initial temperature file. Using default 300K.\n";
                }
            }
        }
        tInitTempLoad = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - init_start).count();

        // Material properties
        MaterialProperties matProps;

        // Open output files with better error handling
        std::ofstream summaryOut(outputFile);
        if (!summaryOut) {
            appLog += "❌ Error: Could not open summary output file: " + std::string(outputFile) + "\n";
            triggerSimulation = false; return;
        }
        std::ofstream detailsOut("stack_details.csv");
        if (!detailsOut) {
            appLog += "❌ Error: Could not open details output file: stack_details.csv\n";
            triggerSimulation = false; return;
        }

        summaryOut << "slice,l/L,method,finalSteelTemp,TPS_thickness,OriginalSteelTemp\n";
        detailsOut << "slice,l/L,OriginalTPS,CarbonFiber_thickness,"
                  << "Glue_thickness,Steel_thickness,"
                  << "PreCarbonTemp,PreGlueTemp,PreSteelTemp,"
                  << "OptimizedTPS,PostCarbonTemp,PostGlueTemp,PostSteelTemp\n";

        double totalSolveMs = 0.0;
        HeatEquationSolver currentSolver; // Local solver for the simulation run

        // Loop over all slices
        for (int slice = 0; slice < nSlices; ++slice) {
            currentProcessingStatus = "Processing stack " + std::to_string(slice + 1) + " of " + std::to_string(nSlices);

            double z = zmin + (nSlices > 1 ? (double(slice)/(nSlices-1)) * height : height / 2.0); // Handle nSlices=1 case
            double lL = (nSlices > 1 ? (z - zmin) / height : 0.5);
            if (height <= 0) lL = 0.0; // Handle flat mesh case

            // ---- Stack setup (incl. grid gen) ----
            auto stack_start = std::chrono::high_resolution_clock::now();
            Stack stack;
            stack.id = slice + 1;

            // Define layers within the loop for clarity
            Layer tpsLayer = {{"TPS", 0.2, 160.0, 1200.0, 0.0, 1200.0}, matProps.getTPSThickness(lL), pointsPerLayer};
            Layer carbonFiberLayer = {{"CarbonFiber", 500.0, 1600.0, 700.0, 0.0, 350.0}, matProps.getCarbonFiberThickness(lL), pointsPerLayer};
            Layer glueLayer = {{"Glue", 200.0, 1300.0, 900.0, 0.0, 400.0}, matProps.getGlueThickness(lL), pointsPerLayer};
            Layer steelLayer = {{"Steel", 100.0, 7850.0, 500.0, 800.0, 0.0}, matProps.getSteelThickness(lL), pointsPerLayer};

            stack.layers = { tpsLayer, carbonFiberLayer, glueLayer, steelLayer };
            
            double tpsThick = matProps.getTPSThickness(lL);
            double cfThick = matProps.getCarbonFiberThickness(lL);
            double glueThick = matProps.getGlueThickness(lL);
            double steelThick = matProps.getSteelThickness(lL);

            matProps.generateGrid(stack, pointsPerLayer);
            tStackSetup = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - stack_start).count();

            if (stack.xGrid.empty()) {
                appLog += "❌ Error: Grid generation failed for slice " + std::to_string(slice + 1) + "\n";
                continue; // Skip this slice
            }

            // Compute interface indices
            double posCG = tpsThick + cfThick;
            double posGS = posCG + glueThick;
            auto idxCarbonGlue = std::lower_bound(stack.xGrid.begin(), stack.xGrid.end(), posCG) - stack.xGrid.begin();
            auto idxGlueSteel = std::lower_bound(stack.xGrid.begin(), stack.xGrid.end(), posGS) - stack.xGrid.begin();

            // Initialize solver for this slice
            TimeHandler timer(simDuration, timeStep, useAdaptiveTimeStep);
            currentSolver = HeatEquationSolver(theta); // Use the local solver
            currentSolver.initialize(stack, timer);

            // Set initial temperature
            if (!uniformInit.empty()) {
                // Check if size matches grid size
                if (uniformInit.size() != stack.xGrid.size()) {
                    appLog += "⚠️ Warning: Initial temperature data size mismatch (expected "
                            + std::to_string(stack.xGrid.size()) + ", got "
                            + std::to_string(uniformInit.size()) + "). Using default 300K.\n";
                    currentSolver.setInitialTemperature(std::vector<double>(stack.xGrid.size(), 300.0));
                } else {
                    currentSolver.setInitialTemperature(uniformInit);
                }
            } else {
                currentSolver.setInitialTemperature(std::vector<double>(stack.xGrid.size(), 300.0));
                if (slice == 0) { // Only log this once per run
                    appLog += "Using default initial temperature: 300K\n";
                }
            }

            // Set boundary conditions
            try {
                currentSolver.setBoundaryConditions(
                    new DirichletCondition(static_cast<float>(matProps.getExhaustTemp(lL))), 
                    new NeumannCondition(0.0f)
                );
            } catch (const std::exception& bc_err) {
                appLog += "❌ Error setting boundary conditions: " + std::string(bc_err.what()) + "\n";
                continue; // Skip slice if BCs fail
            }

            // Prepare time history buffer for original thickness
            std::ostringstream histOrigBuffer;
            histOrigBuffer << "time[s],T_carbon_glue[K],T_glue_steel[K],T_steel[K]\n";

            // Timer for solver per slice
            auto solve_start = std::chrono::high_resolution_clock::now();
            double maxSteps = (timeStep > 0) ? (simDuration / timeStep) : 1.0; // Avoid division by zero
            if (maxSteps <= 0) maxSteps = 1.0;

            // ---- Original solver run ----
            while (!timer.isFinished()) {
                try {
                    currentSolver.step();
                } catch (const std::exception& step_err) {
                    appLog += "❌ Error during solver step for slice " + std::to_string(slice+1) + ": " + std::string(step_err.what()) + "\n";
                    goto next_slice; // Break inner loop, go to next slice
                }
                
                double t = timer.getCurrentTime();
                const auto& Tdist = currentSolver.getTemperatureDistribution();
                
                // Record interface temperatures for history
                if (idxCarbonGlue < Tdist.size() && idxGlueSteel < Tdist.size()) {
                    histOrigBuffer 
                        << t << ","
                        << Tdist[idxCarbonGlue] << ","
                        << Tdist[idxGlueSteel] << ","
                        << Tdist.back() << "\n";
                }
                
                timer.advance();
                // Update progress (divide by nSlices to show overall progress)
                progress = (slice + (static_cast<float>(timer.getStepCount()) / maxSteps)) / (nSlices * 2); // First half of progress
            }

            auto solve_end = std::chrono::high_resolution_clock::now();
            tOrigSolve = std::chrono::duration<double, std::milli>(solve_end - solve_start).count();
            totalSolveMs += tOrigSolve;

            // ---- Original history CSV save ----
            auto saveOrig_start = std::chrono::high_resolution_clock::now();
            {
                std::ofstream histOrig(
                    "time_history_orig_slice_" + std::to_string(slice+1) + ".csv"
                );
                if (histOrig) {
                    histOrig << histOrigBuffer.str();
                    histOrig.close();
                } else {
                    appLog += "⚠️ Warning: Could not save original time history for slice " + std::to_string(slice+1) + "\n";
                }
            }
            tHistOrigSave = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - saveOrig_start).count();

            // Get results for original thickness
            const auto& Tdist = currentSolver.getTemperatureDistribution();
            if (Tdist.empty()) {
                appLog += "⚠️ Warning: No temperature distribution result for slice " + std::to_string(slice + 1) + "\n";
                continue; // Skip results processing for this slice
            }
            
            double origTempCarbon = (idxCarbonGlue < Tdist.size()) ? Tdist[idxCarbonGlue] : 0.0;
            double origTempGlue = (idxGlueSteel < Tdist.size()) ? Tdist[idxGlueSteel] : 0.0;
            double origTempSteel = Tdist.back(); // Assume last point is steel surface

            // Export final temperature distribution for each slice - Original version
            {
                std::ofstream outFile("final_temperature_orig_slice_" + std::to_string(slice+1) + ".csv");
                if (outFile) {
                    outFile << "x,Temperature\n"; // Add header
                    for (size_t i = 0; i < Tdist.size(); ++i) {
                        outFile << stack.xGrid[i] << "," << Tdist[i] << "\n";
                    }
                    outFile.close();
                } else {
                    appLog += "❌ Error: Could not open final_temperature_orig_slice_" + std::to_string(slice+1) + ".csv for writing.\n";
                }
                
                // Also save to final_temperature_orig.csv if it's the last slice
                if (slice == nSlices - 1) {
                    std::ofstream finalOutFile("final_temperature_orig.csv");
                    if (finalOutFile) {
                        finalOutFile << "x,Temperature\n"; // Add header
                        for (size_t i = 0; i < Tdist.size(); ++i) {
                            finalOutFile << stack.xGrid[i] << "," << Tdist[i] << "\n";
                        }
                        finalOutFile.close();
                    } else {
                        appLog += "❌ Error: Could not open final_temperature_orig.csv for writing.\n";
                    }
                }
            }

            // Suggest optimal TPS thickness
            currentProcessingStatus = "Optimizing TPS thickness for stack " + std::to_string(slice + 1);
            double tpsOpt = -1.0; // Default invalid value
            
            auto optSuggest_start = std::chrono::high_resolution_clock::now();
            try {
                TemperatureComparator comp;
                comp.setTimeStep(timeStep, useAdaptiveTimeStep);
                comp.setGridResolution(pointsPerLayer);
                
                // Ensure stack is valid before passing to comparator
                if (!stack.layers.empty() && !stack.xGrid.empty()) {
                    tpsOpt = comp.suggestTPSThickness(
                        stack,
                        800.0,   // max steel temp @ steel/glue
                        400.0,   // max glue  temp @ glue/carbon
                        350.0,   // max carbon temp @ carbon/external
                        simDuration,
                        lL,
                        matProps,
                        theta
                    );
                } else {
                    appLog += "⚠️ Warning: Cannot optimize TPS for slice " + std::to_string(slice + 1) + " due to invalid stack.\n";
                }
            } catch (const std::exception& opt_err) {
                appLog += "❌ Error during TPS optimization for slice " + std::to_string(slice+1) + ": " + std::string(opt_err.what()) + "\n";
            }
            tOptSuggestion = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - optSuggest_start).count();

            // --- Re-run solver with optimized thickness ---
            currentProcessingStatus = "Running with optimized thickness for stack " + std::to_string(slice + 1);
            
            // Only run optimized simulation if we got a valid thickness
            double postTempCarbon = 0.0;
            double postTempGlue = 0.0;
            double postTempSteel = 0.0;
            
            if (tpsOpt > 0) {
                stack.layers[0].thickness = tpsOpt;
                matProps.generateGrid(stack, pointsPerLayer);

                TimeHandler timerOpt(simDuration, timeStep, useAdaptiveTimeStep);
                HeatEquationSolver solverOpt(theta);
                solverOpt.initialize(stack, timerOpt);
                
                if (uniformInit.empty() || uniformInit.size() != stack.xGrid.size()) {
                    solverOpt.setInitialTemperature(std::vector<double>(stack.xGrid.size(), 300.0));
                } else {
                    solverOpt.setInitialTemperature(uniformInit);
                }
                
                try {
                    solverOpt.setBoundaryConditions(
                        new DirichletCondition(static_cast<float>(matProps.getExhaustTemp(lL))),
                        new NeumannCondition(0.0f)
                    );
                } catch (const std::exception& bc_err) {
                    appLog += "❌ Error setting boundary conditions for optimized run: " + std::string(bc_err.what()) + "\n";
                    goto skip_opt_run; // Skip optimized simulation if BCs fail
                }

                auto solveOpt_start = std::chrono::high_resolution_clock::now();
                std::ostringstream histOptBuffer;
                histOptBuffer << "time[s],T_carbon_glue[K],T_glue_steel[K],T_steel[K]\n";
                
                while (!timerOpt.isFinished()) {
                    try {
                        solverOpt.step();
                    } catch (const std::exception& step_err) {
                        appLog += "❌ Error during optimized solver step: " + std::string(step_err.what()) + "\n";
                        break;
                    }
                    
                    double t2 = timerOpt.getCurrentTime();
                    const auto& T2 = solverOpt.getTemperatureDistribution();
                    
                    if (idxCarbonGlue < T2.size() && idxGlueSteel < T2.size()) {
                        histOptBuffer
                            << t2 << ","
                            << T2[idxCarbonGlue] << ","
                            << T2[idxGlueSteel] << ","
                            << T2.back() << "\n";
                    }
                    
                    timerOpt.advance();
                    // Update progress for second half (optimized run)
                    progress = 0.5 + (slice + (static_cast<float>(timerOpt.getStepCount()) / maxSteps)) / (nSlices * 2);
                }
                
                tOptSolve = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - solveOpt_start).count();
                totalSolveMs += tOptSolve;
                
                // ---- Optimized history CSV save ----
                auto saveOpt_start = std::chrono::high_resolution_clock::now();
                {
                    std::ofstream histOpt(
                        "time_history_opt_slice_" + std::to_string(slice+1) + ".csv"
                    );
                    if (histOpt) {
                        histOpt << histOptBuffer.str();
                        histOpt.close();
                    } else {
                        appLog += "⚠️ Warning: Could not save optimized time history for slice " + std::to_string(slice+1) + "\n";
                    }
                }
                tHistOptSave = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - saveOpt_start).count();

                // Sample optimized steel temp
                const auto& Topt = solverOpt.getTemperatureDistribution();
                postTempCarbon = (idxCarbonGlue < Topt.size()) ? Topt[idxCarbonGlue] : 0.0;
                postTempGlue = (idxGlueSteel < Topt.size()) ? Topt[idxGlueSteel] : 0.0;
                postTempSteel = Topt.back();
                
                // Store the optimized solver result if it's the last slice
                if (slice == nSlices - 1) {
                    solver = solverOpt; // Store the solver state globally for visualization
                }

                // Export final temperature distribution for each slice - Optimized version
                if (tpsOpt > 0 && postTempSteel > 0) {
                    const auto& Topt = solverOpt.getTemperatureDistribution();
                    std::ofstream outFile("final_temperature_opt_slice_" + std::to_string(slice+1) + ".csv");
                    if (outFile) {
                        outFile << "x,Temperature\n"; // Add header
                        for (size_t i = 0; i < Topt.size(); ++i) {
                            outFile << stack.xGrid[i] << "," << Topt[i] << "\n";
                        }
                        outFile.close();
                    } else {
                        appLog += "❌ Error: Could not open final_temperature_opt_slice_" + std::to_string(slice+1) + ".csv for writing.\n";
                    }
                    
                    // Also save to final_temperature_opt.csv if it's the last slice
                    if (slice == nSlices - 1) {
                        std::ofstream finalOutFile("final_temperature_opt.csv");
                        if (finalOutFile) {
                            finalOutFile << "x,Temperature\n"; // Add header
                            for (size_t i = 0; i < Topt.size(); ++i) {
                                finalOutFile << stack.xGrid[i] << "," << Topt[i] << "\n";
                            }
                            finalOutFile.close();
                        } else {
                            appLog += "❌ Error: Could not open final_temperature_opt.csv for writing.\n";
                        }
                    }
                }

                // For compatibility, still save combined final temperature file (using optimized if available)
                {
                    std::ofstream outFile("final_temperature_slice_" + std::to_string(slice+1) + ".csv");
                    if (outFile) {
                        outFile << "x,Temperature\n"; // Add header
                        const auto& finalTdist = (postTempSteel > 0) ? solverOpt.getTemperatureDistribution() : Tdist;
                        for (size_t i = 0; i < finalTdist.size(); ++i) {
                            outFile << stack.xGrid[i] << "," << finalTdist[i] << "\n";
                        }
                        outFile.close();
                    } else {
                        appLog += "❌ Error: Could not open final_temperature_slice_" + std::to_string(slice+1) + ".csv for writing.\n";
                    }
                    
                    // Also save to final_temperature.csv if it's the last slice (for backward compatibility with visualization)
                    if (slice == nSlices - 1) {
                        std::ofstream finalOutFile("final_temperature.csv");
                        if (finalOutFile) {
                            finalOutFile << "x,Temperature\n"; // Add header
                            const auto& finalTdist = (postTempSteel > 0) ? solverOpt.getTemperatureDistribution() : Tdist;
                            for (size_t i = 0; i < finalTdist.size(); ++i) {
                                finalOutFile << stack.xGrid[i] << "," << finalTdist[i] << "\n";
                            }
                            finalOutFile.close();
                        } else {
                            appLog += "❌ Error: Could not open final_temperature.csv for writing.\n";
                        }
                    }
                }
            }
            
skip_opt_run:
            // ---- Summary & details CSV writes ----
            auto out_start = std::chrono::high_resolution_clock::now();

            // Write summary CSV with the optimized‐thickness temperature
            summaryOut 
                << (slice + 1) << ","
                << lL << ","
                << "BTCS" << ","
                << (postTempSteel > 0 ? postTempSteel : origTempSteel) << ","
                << (tpsOpt > 0 ? tpsOpt : tpsThick) << ","
                << origTempSteel << "\n";

            // write the detailed interface temps
            detailsOut
                << (slice+1) << ","
                << lL << ","
                << tpsThick << ","  // original TPS
                << cfThick << ","
                << glueThick << ","
                << steelThick << ","
                << origTempCarbon << ","
                << origTempGlue << ","
                << origTempSteel << ","
                << (tpsOpt > 0 ? tpsOpt : tpsThick) << ","
                << (postTempCarbon > 0 ? postTempCarbon : origTempCarbon) << ","
                << (postTempGlue > 0 ? postTempGlue : origTempGlue) << ","
                << (postTempSteel > 0 ? postTempSteel : origTempSteel) << "\n";
                
            tSummaryDetailsWrite = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - out_start).count();

next_slice:; // Label for jumping to next slice on error
        } // End slice loop

        // Clear processing status when done
        currentProcessingStatus.clear();

        // Close output files
        summaryOut.close();
        detailsOut.close();

        // overall end
        double overallMs = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - overall_start).count();

        appLog += "✅ Simulation completed!\n";
        appLog += "Processed " + std::to_string(nSlices) + " slices.\n";
        appLog += "\n=== Performance ===\n";
        appLog += "Mesh load time:             " + std::to_string(tMeshLoad) + " ms\n";
        appLog += "Init temp load time:        " + std::to_string(tInitTempLoad) + " ms\n";
        appLog += "Original solver time:       " + std::to_string(tOrigSolve) + " ms\n";
        appLog += "TPS optimization time:      " + std::to_string(tOptSuggestion) + " ms\n";
        appLog += "Optimized solver time:      " + std::to_string(tOptSolve) + " ms\n";
        appLog += "Total computation time:     " + std::to_string(overallMs) + " ms\n";
        appLog += "\n=== Output Files ===\n";
        appLog += "- final_temperature_slice_*.csv: Temperature distribution for each slice\n";
        appLog += "- " + std::string(outputFile) + ": Summary results\n";
        appLog += "- stack_details.csv: Detailed layer information\n";
        appLog += "- time_history_orig_slice_*.csv: Original time histories\n";
        appLog += "- time_history_opt_slice_*.csv: Optimized time histories\n";
        
        simulationCompleted = true; // Mark as completed
        triggerSimulation = false;  // Reset trigger

    } catch (const std::exception& e) {
        appLog += "❌ Exception in runSimulationLogic: ";
        appLog += e.what();
        appLog += "\n";
        currentProcessingStatus.clear();
        triggerSimulation = false; // Ensure trigger is reset on exception
        simulationCompleted = false;
    } catch (...) {
        appLog += "❌ Unknown exception occurred in runSimulationLogic.\n";
        currentProcessingStatus.clear();
        triggerSimulation = false;
        simulationCompleted = false;
    }
}

// ---- Visualization Controls Window ----
void renderVisualizationControls() {
    // This window will also be explicitly positioned and sized in the main loop
    ImGui::Begin("Visualization Controls");

    ImGui::Checkbox("Enable Camera Movement", &cameraMovementEnabled);
    ImGui::Checkbox("Wireframe Mode", &renderWireframe);
    ImGui::Checkbox("Show Mesh", &showMesh); // NEW: Toggle for mesh visibility
    ImGui::Checkbox("Show Color Scale", &showColorScale);
    ImGui::Checkbox("Show Slice Lines", &showSliceLines);
    
    if (ImGui::Button("Reset View")) {
        // Reset camera to default position
        camDistance = 5.0f;
        camAzimuth = 45.0f;
        camElevation = 30.0f;
        camTargetX = 0.0f;
        camTargetY = 0.0f;
        camTargetZ = 0.0f;
    }
    
    ImGui::Separator();
    ImGui::Text("Visualization Type:");
    if (ImGui::RadioButton("Temperature", currentVisMode == TEMPERATURE_VIS)) {
        currentVisMode = TEMPERATURE_VIS;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("TPU Thickness", currentVisMode == THICKNESS_VIS)) {
        currentVisMode = THICKNESS_VIS;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Line Plot", currentVisMode == LINE_PLOT_VIS)) { // NEW: Line plot mode
        currentVisMode = LINE_PLOT_VIS;
    }

    ImGui::Separator();
    ImGui::Text("Camera:");
    // Display Azimuth/Elevation/Distance instead of Position/Rotation
    ImGui::Text("Azimuth: %.1f, Elevation: %.1f", camAzimuth, camElevation);
    ImGui::Text("Distance: %.1f", camDistance);
    ImGui::Text("Target: (%.1f, %.1f, %.1f)", camTargetX, camTargetY, camTargetZ);

    ImGui::Separator();
    ImGui::Text("Slice Selection:");
    ImGui::InputInt("Selected Slice", &selectedSlice);
    if (ImGui::Button("Update Plot Data")) {
        // The selectedSlice is used directly in drawTemperaturePlot, 
        // so this button is just a visual cue for the user
        appLog += "Updated plot data for slice " + std::to_string(selectedSlice) + "\n";
    }
    ImGui::SameLine();
    ImGui::Text("(Range: 1-%d)", nSlices);
    
    // Ensure slice number is within valid range
    selectedSlice = std::clamp(selectedSlice, 1, nSlices);

    ImGui::End();
}

// ---- Updated Visualization Rendering Function ----
void renderVisualization(int vx, int vy, int vw, int vh, bool isHovered) {
    // Set OpenGL viewport and scissor test to constrain drawing
    glViewport(vx, vy, vw, vh);
    glScissor(vx, vy, vw, vh);
    glEnable(GL_SCISSOR_TEST);

    // --- Camera Control Logic ---
    if (cameraMovementEnabled && isHovered) {
        ImGuiIO& io = ImGui::GetIO();

        // Zoom: Mouse Wheel
        if (io.MouseWheel != 0.0f) {
            camDistance -= io.MouseWheel * zoomSensitivity * camDistance * 0.1f;
            camDistance = std::clamp(camDistance, 0.1f, 50.0f); // Clamp distance
        }

        // Orbit: Left Mouse Drag (rotates around the target point)
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            // Reset delta start pos for continuous dragging
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);

            camAzimuth += dragDelta.x * mouseSensitivity;
            camElevation -= dragDelta.y * mouseSensitivity; // Inverted to match natural movement

            // Clamp elevation to avoid flipping
            camElevation = std::clamp(camElevation, -89.9f, 89.9f);
        }
        
        // Pan: Right Mouse Drag - move in screen space without changing distance
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
            
            // Calculate camera right and up vectors for proper screen-space panning
            float azimuthRad = camAzimuth * M_PI / 180.0f;
            float elevationRad = camElevation * M_PI / 180.0f;
            
            // Right vector (perpendicular to view direction in horizontal plane)
            float rightX = -sin(azimuthRad);
            float rightY = cos(azimuthRad);
            float rightZ = 0.0f;
            
            // Up vector (aligned with world up, adjusted for elevation)
            float upX = -sin(elevationRad) * cos(azimuthRad);
            float upY = -sin(elevationRad) * sin(azimuthRad);
            float upZ = cos(elevationRad);
            
            // Calculate pan amount based on camera distance (farther = faster pan)
            float panSpeed = panSensitivity * camDistance;
            
            // Apply panning movement
            camTargetX += (rightX * dragDelta.x - upX * dragDelta.y) * panSpeed;
            camTargetY += (rightY * dragDelta.x - upY * dragDelta.y) * panSpeed;
            camTargetZ += (rightZ * dragDelta.x - upZ * dragDelta.y) * panSpeed;
        }
    }

    // --- OpenGL Setup for this Viewport ---
    // Set up projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (vh > 0) ? ((float)vw / (float)vh) : 1.0f;
    perspectiveGL(45.0f, aspect, 0.1f, 100.0f); // Use helper

    // Set up modelview matrix (camera)
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Calculate camera position using spherical coordinates (orbit)
    // Using Z as the up axis to ensure the mesh stands on its feet (Z is vertical)
    float camX = camTargetX + camDistance * cos(camElevation * M_PI / 180.0f) * sin(camAzimuth * M_PI / 180.0f);
    float camY = camTargetY + camDistance * cos(camElevation * M_PI / 180.0f) * cos(camAzimuth * M_PI / 180.0f);
    float camZ = camTargetZ + camDistance * sin(camElevation * M_PI / 180.0f);

    // Use lookAtGL with Z as up vector (vertical axis)
    lookAtGL(camX, camY, camZ,             // Eye position
             camTargetX, camTargetY, camTargetZ, // Target position
             0.0f, 0.0f, 1.0f);           // Up vector (Z is up)

    // Clear background specifically for this viewport
    glClearColor(0.15f, 0.15f, 0.17f, 1.0f); // Slightly different background for vis
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing, lighting, etc.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Setup basic lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat light_position[] = { 5.0f, 5.0f, 5.0f, 1.0f }; // Adjusted light position
    GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glEnable(GL_COLOR_MATERIAL); // Allow glColor to affect material diffuse
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH); // Use smooth shading

    // --- Drawing ---
    glPushMatrix();
    
    // Set initial camera orientation
    // No rotation needed as we're already using Z as the up (vertical) axis

    // Add scaling to handle size differences
    float meshScaleFactor = 1.0f;  // Adjust this if mesh is too large or small
    glScalef(meshScaleFactor, meshScaleFactor, meshScaleFactor);
    
    // Center the mesh automatically
    if (meshLoadedForVis && autoAdjustCameraOnLoad) {
        updateMeshVisualization(); // Use helper function to update visualization
    }

    // Draw coordinate axes
    glDisable(GL_LIGHTING); // Draw axes without lighting
    drawCoordinateAxes();
    glEnable(GL_LIGHTING); // Re-enable lighting

    // Set wireframe mode if enabled
    if (renderWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Load mesh for visualization if path is set and not yet loaded
    if (!meshLoadedForVis && strlen(meshPath) > 0) {
        try {
            if (meshHandler.loadMesh(meshPath)) {
                meshLoadedForVis = true;
                autoAdjustCameraOnLoad = true; // Force camera adjustment on new mesh load
                appLog += "✅ Mesh loaded successfully for visualization.\n";
            } else {
                appLog += "❌ Error loading mesh for visualization: " + std::string(meshPath) + "\n";
                meshPath[0] = '\0'; // Clear invalid path
            }
        } catch (const std::exception& e) {
            appLog += "❌ Exception loading mesh: " + std::string(e.what()) + "\n";
            meshPath[0] = '\0'; // Clear invalid path
        }
    }

    // Draw the mesh based on the selected visualization mode
    if (meshLoadedForVis && showMesh) {  // Respect the showMesh toggle
        // Only render in 3D mode if not in line plot mode
        if (currentVisMode != LINE_PLOT_VIS) {
            if (currentVisMode == TEMPERATURE_VIS && simulationCompleted && !solver.getTemperatureDistribution().empty()) {
                drawMeshWithTemperatures(meshHandler, solver); // Temperature visualization
            } else if (currentVisMode == THICKNESS_VIS) {
                drawMeshWithThickness(meshHandler, solver); // TPU thickness visualization
            } else {
                drawMeshDefault(meshHandler); // Default material
            }

            // Draw slice lines if enabled
            if (showSliceLines) {
                drawSliceLines(meshHandler);
            }
        }
    }
    
    glPopMatrix();

    // Draw color scale if enabled and not in line plot mode
    if (showColorScale && currentVisMode != LINE_PLOT_VIS) {
        int scaleWidth = 150;
        int scaleHeight = 300;
        int scaleX = vx + vw - scaleWidth - 10; // Position near the right edge
        int scaleY = vy + 10; // Position near the top
        drawColorScale(scaleX, scaleY, scaleWidth, scaleHeight, currentVisMode == TEMPERATURE_VIS);
    }
    
    // Draw temperature line plot if that mode is selected
    if (currentVisMode == LINE_PLOT_VIS) {
        drawTemperaturePlot(vx + 10, vy + 10, vw - 20, vh - 20);
    }

    // --- Cleanup OpenGL state for this viewport ---
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset polygon mode
    glDisable(GL_SCISSOR_TEST); // Disable scissor test IMPORTANT!
    glDisable(GL_LIGHTING); // Disable lighting
    glDisable(GL_DEPTH_TEST); // Disable depth test (ImGui prefers it off)
}

int main(int argc, char** argv) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Create Window
    GLFWwindow* window = glfwCreateWindow(1600, 900, "HeatStack Simulator", NULL, NULL);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create GLFW window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // --- Simulation Thread ---
    std::future<void> simulationFuture;
    bool isSimThreadRunning = false;

    // --- Main Loop ---
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Handle simulation triggering and thread management
        if (triggerSimulation && !isSimThreadRunning) {
            // Start simulation in a separate thread using std::async
            simulationFuture = std::async(std::launch::async, runSimulationLogic);
            isSimThreadRunning = true;
            triggerSimulation = false; // Consume the trigger
        }

        // Check if simulation thread finished
        if (isSimThreadRunning) {
            // Non-blocking check if thread is finished using future_status
            auto status = simulationFuture.wait_for(std::chrono::seconds(0));
            if (status == std::future_status::ready) {
                isSimThreadRunning = false;
                // SimulationCompleted flag should be set by runSimulationLogic
                if (!simulationCompleted) { // If logic finished but didn't set completed (e.g., due to error)
                    currentProcessingStatus = "Simulation finished with errors or was stopped.";
                }
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- Get Window Size ---
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // --- Define Layout ---
        // Simple split: Left ~40% for controls, Right ~60% for visualization
        float controlPanelWidth = display_w * 0.4f;
        int visX = static_cast<int>(controlPanelWidth);
        int visY = 0;
        int visW = display_w - visX;
        int visH = display_h;

        // --- Render Control Windows on the Left ---
        // Simulation Controls Window
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(controlPanelWidth, display_h * 0.65f), ImGuiCond_Always);
        renderSimulationControls();

        // Visualization Controls Window (below simulation controls)
        ImGui::SetNextWindowPos(ImVec2(0, display_h * 0.65f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(controlPanelWidth, display_h * 0.35f), ImGuiCond_Always);
        renderVisualizationControls();

        // --- Prepare for Visualization Rendering ---
        // Determine if the mouse is hovering over the visualization viewport *area*
        ImVec2 mousePos = ImGui::GetMousePos();
        bool isVisViewportHovered = (mousePos.x >= visX && mousePos.x < (visX + visW) &&
                                     mousePos.y >= visY && mousePos.y < (visY + visH));

        // Clear the entire window background (needed for ImGui)
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the visualization in its viewport
        renderVisualization(visX, visY, visW, visH, isVisViewportHovered);

        // --- Render ImGui ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // --- Cleanup ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}