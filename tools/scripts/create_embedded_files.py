#!/usr/bin/env python3

#
#     ,ad888ba,                              88
#    d8"'    "8b
#   d8            88,dba,,adba,   ,aPP8A.A8  88     The Cmajor Toolkit
#   Y8,           88    88    88  88     88  88
#    Y8a.   .a8P  88    88    88  88,   ,88  88     (C)2024 Cmajor Software Ltd
#     '"Y888Y"'   88    88    88  '"8bbP"Y8  88     https://cmajor.dev
#                                           ,88
#                                        888P"
#
#
#  This script is used to regenerate the C++ files containing embedded assets
#  that are needed by the compiler and other parts of the Cmajor toolchain.
#
#  If you modify any of the javascript files, or the public API headers,
#  you'll need to run this and then rebuild any projects that might embed them.
#
#  We commit the latest versions of the embedded data to the repo so
#  there's no need to run this if you just pull the latest changes, but
#  if you're modifying things yourself, you will need to use it.
#

import os
import sys
import zipfile
sys.dont_write_bytecode = True
from string_literal_helpers import *


#####################################################################################################
def createStringLiteralFile(variableName, targetFile, source):
    content = "\n// This file was generated by tools/scripts/create_embedded_files.py\n\n"

    with open (source, 'r') as f:
        content += "//==============================================================================\n"
        content += "static constexpr std::string_view " + variableName + " = " + createCppStringLiteral (f.read(), "    ") + ";\n"

    replaceFileIfDifferent (targetFile, content)

#####################################################################################################
def convertHTMLToCppCode (filename, code):
    print ("Adding " + filename)
    return "static constexpr const char* " + filename.replace (".", "_") + " = " + createCppStringLiteral (code, "    ") + ";\n"

#####################################################################################################
def createHTMLFile(targetFile, folder):
    content = "\n// This file was generated by the script create_embedded_files\n\n"

    print ("Searching: " + folder)

    for root, dirs, files in os.walk (folder):
        for file in files:
            if file.endswith(".html"):
                with open (os.path.join(root, file), 'r') as f:
                    content += "//==============================================================================\n"
                    content += "//==============================================================================\n"
                    content += convertHTMLToCppCode (file, f.read())

    if not content:
        exit ("No HTML found!")

    replaceFileIfDifferent (targetFile, content)

#####################################################################################################
def createZipImageLiteral(folder):
    tempZipFile = "temp.zip"
    filesToZip = []

    with zipfile.ZipFile (tempZipFile, 'w', zipfile.ZIP_DEFLATED, True, 9) as zip:
        for root, dirs, files in os.walk (folder):
            for file in files:
                path = os.path.normpath (os.path.join (root, file))

                if (path.find ("/.") < 0):
                    filesToZip.append (path)

        filesToZip.sort()

        for file in filesToZip:
            print ("zipping " + file)
            zip.write (file, os.path.relpath (file, folder))

        zip.close()

    literal = createCppDataLiteralFromFile (tempZipFile, "  ")
    os.unlink (tempZipFile)
    return literal

#####################################################################################################
def createEmbeddedZipData(desc, variable, targetFile, folder):
    content = "// Auto-generated: this is a zip file image of " + desc + "\n\n"
    content += "#pragma once\n\n"
    content += "static constexpr const uint8_t " + variable + "[] = {\n  " + createZipImageLiteral (folder) + "\n};\n"
    replaceFileIfDifferent (targetFile, content)


#####################################################################################################
def createWebAPIAssetsFile (assetFolder, targetHeader):
    output = '''\
//
//     ,ad888ba,                              88
//    d8"'    "8b
//   d8            88,dba,,adba,   ,aPP8A.A8  88     The Cmajor Toolkit
//   Y8,           88    88    88  88     88  88
//    Y8a.   .a8P  88    88    88  88,   ,88  88     (C)2024 Cmajor Software Ltd
//     '"Y888Y"'   88    88    88  '"8bbP"Y8  88     https://cmajor.dev
//                                           ,88
//                                        888P"
//
//  Cmajor may be used under the terms of the ISC license:
//
//  Permission to use, copy, modify, and/or distribute this software for any purpose with or
//  without fee is hereby granted, provided that the above copyright notice and this permission
//  notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//  WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//  AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//  CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#pragma once

#include <array>
#include <string_view>

namespace cmaj
{

/// This contains the javascript and other asset files availble to patch views running in
/// a browser envirtonment.
///
/// It contains modules that provide the generic GUI view, and other helper classes that
/// can be used by custom views.
///
struct EmbeddedWebAssets
{
    static std::string_view findResource (std::string_view path)
    {
        for (auto& file : files)
            if (path == file.name)
                return file.content;

        return {};
    }

    struct File { std::string_view name, content; };

FILE_DATA
};

} // namespace cmaj
'''
    filesToAdd = []

    for root, dirs, files in os.walk (assetFolder):
        for file in files:
            if file.find (".DS_Store") < 0:
                with open (os.path.join (root, file), 'rb') as f:
                    filesToAdd.append ([ os.path.relpath (os.path.join (root, file), assetFolder), f.read() ])

    output = output.replace ("FILE_DATA", createCppFileData (filesToAdd))
    replaceFileIfDifferent (targetHeader, output)


#####################################################################################################
def createServerWebAssetsFile (assetFolder, targetHeader):
    output = '''\
//
//     ,ad888ba,                              88
//    d8"'    "8b
//   d8            88,dba,,adba,   ,aPP8A.A8  88     The Cmajor Toolkit
//   Y8,           88    88    88  88     88  88
//    Y8a.   .a8P  88    88    88  88,   ,88  88     (C)2024 Cmajor Software Ltd
//     '"Y888Y"'   88    88    88  '"8bbP"Y8  88     https://cmajor.dev
//                                           ,88
//                                        888P"
//
//  The Cmajor project is subject to commercial or open-source licensing.
//  You may use it under the terms of the GPLv3 (see www.gnu.org/licenses), or
//  visit https://cmajor.dev to learn about our commercial licence options.
//
//  CMAJOR IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
//  EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
//  DISCLAIMED.

// This file contains auto-generated embedded file data

struct Files
{
    struct File { std::string_view name, content; };

FILE_DATA
};
'''
    filesToAdd = []

    for root, dirs, files in os.walk (assetFolder):
        for file in files:
            path = os.path.relpath (os.path.join (root, file), assetFolder)

            if path.find (".DS_Store") < 0:
                with open (os.path.join (root, file), 'rb') as f:
                    filesToAdd.append ([ path, f.read() ])

    output = output.replace ("FILE_DATA", createCppFileData (filesToAdd))
    replaceFileIfDifferent (targetHeader, output)


#####################################################################################################

scriptsFolder = os.path.dirname (os.path.realpath(__file__))
repoFolder = os.path.join (scriptsFolder, "../..")

sys.setrecursionlimit (2000)

print ("-------------------------------------------------------------")

createStringLiteralFile ("testFunctionLibrary",
                         os.path.join (repoFolder, "modules/scripting/src/cmaj_TestRunnerLibrary.h"),
                         os.path.join (repoFolder, "tests/cmaj_test_functions.js"))

print ("-------------------------------------------------------------")

createWebAPIAssetsFile (os.path.join (repoFolder, "javascript/cmaj_api"),
                        os.path.join (repoFolder, "include/cmajor/helpers/cmaj_EmbeddedWebAssets.h"))

print ("-------------------------------------------------------------")

createServerWebAssetsFile (os.path.join (repoFolder, "modules/embedded_assets/files"),
                           os.path.join (repoFolder, "modules/embedded_assets/cmaj_EmbeddedAssets_data.h"))

print ("-------------------------------------------------------------")

createEmbeddedZipData ("cmajor/include", "cmajorIncludeFolderZip",
                       os.path.join (repoFolder, "tools/command/Source/cmaj_command_EmbeddedIncludeFolder.h"),
                       os.path.join (repoFolder, "include"))

print ("-------------------------------------------------------------")

createEmbeddedZipData ("cmajor/modules/plugin/include", "cmajorPluginHelpersFolderZip",
                       os.path.join (repoFolder, "tools/command/Source/cmaj_command_EmbeddedPluginHelpersFolder.h"),
                       os.path.join (repoFolder, "modules/plugin/include"))

