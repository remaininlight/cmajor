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

static constexpr auto patchManifest = R"(
{
    "CmajorVersion":    1,
    "ID":               "com.your_name.your_patch_ID",
    "version":          "1.0",
    "name":             "NAME",
    "description":      "NAME",
    "category":         "generator",
    "manufacturer":     "Your Company Goes Here",
    "isInstrument":     true,

    "source":           "NAME.cmajor"
}
)";

static constexpr auto patchCode = R"(
/*
    This file was auto-generated by the cmaj tool!
    cmajor.dev
*/

// Here's a very simple graph that plays a sine-wave to get you started..
graph NAME  [[main]]
{
    // Our processor has a single floating point output stream:
    output stream float out;

    // we'll create a node which is an oscillator from the standard library:
    node sine = std::oscillators::Sine (float, 440);

    // and we'll connect our oscillator to the output stream via a fixed
    // gain processor, to make it quieter:
    connection sine -> std::levels::ConstantGain (float, 0.15f) -> out;
}

)";

void createPatch (choc::ArgumentList& args)
{
    std::string name;

    if (auto n = args.removeValueFor ("--name"))
        name = choc::file::makeSafeFilename (choc::text::removeDoubleQuotes (*n));

    auto files = args.getAllAsFiles();

    if (files.size() != 1)
        throw std::runtime_error ("Expected the name of a folder to create");

    auto folder = files[0];

    if (exists (folder))
        throw std::runtime_error ("This folder already exists");

    std::filesystem::create_directories (folder);

    if (name.empty())
        name = folder.stem().string();

    choc::file::replaceFileWithContent ((folder / name).replace_extension (".cmajorpatch"),
                                        choc::text::trimStart (choc::text::replace (patchManifest, "NAME", name)));

    choc::file::replaceFileWithContent ((folder / name).replace_extension (".cmajor"),
                                        choc::text::trimStart (choc::text::replace (patchCode, "NAME", name)));

    std::cout << "Created Cmajor patch in: " << folder.string() << std::endl;
}
