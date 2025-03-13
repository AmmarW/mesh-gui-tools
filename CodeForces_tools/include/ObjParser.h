/**
 * @file ObjParser.h
 * @brief Header file for the ObjParser class.
 *
 * This file contains the declaration of the ObjParser class, which provides
 * functionality to parse OBJ files and convert them into Mesh objects.
 */

#ifndef OBJPARSER_H
#define OBJPARSER_H

#include "Mesh.h"
#include <string>

/**
 * @class ObjParser
 * @brief A class for parsing OBJ files.
 *
 * The ObjParser class provides a static method to parse an OBJ file and
 * return a Mesh object.
 */
class ObjParser {
public:
    /**
     * @brief Parses an OBJ file and returns a Mesh.
     *
     * This static method takes the file path of an OBJ file as input,
     * parses the file, and returns a Mesh object representing the 3D model.
     *
     * @param filePath The path to the OBJ file to be parsed.
     * @return A Mesh object representing the parsed 3D model.
     */
    static Mesh parse(const std::string& filePath);
};

#endif // OBJPARSER_H
