// This file is part of the AliceVision project.
// Copyright (c) 2017 AliceVision contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <aliceVision/keyframe/KeyframeSelector.hpp>
#include <aliceVision/system/Logger.hpp>
#include <aliceVision/system/cmdline.hpp>
#include <aliceVision/system/main.hpp>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <string>
#include <vector>

// These constants define the current software version.
// They must be updated when the command line is changed.
#define ALICEVISION_SOFTWARE_VERSION_MAJOR 2
#define ALICEVISION_SOFTWARE_VERSION_MINOR 0

using namespace aliceVision::keyframe;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

int aliceVision_main(int argc, char** argv)
{
    // command-line parameters
    std::string verboseLevel = aliceVision::system::EVerboseLevel_enumToString(aliceVision::system::Logger::getDefaultVerboseLevel());
    std::string outputFolder;
    std::string sensorDbPath;
    bool useSimpleMethod = true;

    // Info for each "rig" view
    std::vector<std::string> mediaPaths; // media file path list
    std::vector<std::string> brands;     // media brand list
    std::vector<std::string> models;     // media model list
    std::vector<float> mmFocals;         // media focal (mm) list

    unsigned int minFrameStep = 12;
    unsigned int maxNbOutFrame = 0;

    po::options_description allParams("This program is used to extract keyframes from single camera or a camera rig");

    po::options_description inputParams("Required parameters");
    inputParams.add_options()
        ("mediaPaths", po::value<std::vector<std::string>>(&mediaPaths)->required()->multitoken(), "Input video files or image sequence directories.")
        ("sensorDbPath", po::value<std::string>(&sensorDbPath)->required(), "Camera sensor width database path.")
        ("outputFolder", po::value<std::string>(&outputFolder)->required(), "Output keyframes folder for .jpg")
        ("useSimpleMethod", po::value<bool>(&useSimpleMethod)->required(), "Use simple algorithm instead of smart");

    po::options_description algorithmParams("Algorithm parameters");
    algorithmParams.add_options()
        ("minFrameStep", po::value<unsigned int>(&minFrameStep)->default_value(minFrameStep),"minimum number of frames between two keyframes")
        ("maxNbOutFrame", po::value<unsigned int>(&maxNbOutFrame)->default_value(maxNbOutFrame), "maximum number of output frames (0 = no limit)");

    po::options_description metadataParams("Metadata parameters");
    metadataParams.add_options()
        ("brands", po::value<std::vector<std::string>>(&brands)->default_value(brands)->multitoken(), "Camera brands.")
        ("models", po::value<std::vector<std::string>>(&models)->default_value(models)->multitoken(), "Camera models.")
        ("mmFocals", po::value<std::vector<float>>(&mmFocals)->default_value(mmFocals)->multitoken(), "Focals in mm (will be use if not 0).");

    po::options_description logParams("Log parameters");
    logParams.add_options()("verboseLevel,v", po::value<std::string>(&verboseLevel)->default_value(verboseLevel), "verbosity level (fatal, error, warning, info, debug, trace).");

    allParams.add(inputParams).add(algorithmParams).add(metadataParams).add(logParams);

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, allParams), vm);

        if(vm.count("help") || (argc == 1))
        {
            ALICEVISION_COUT(allParams);
            return EXIT_SUCCESS;
        }
        po::notify(vm);
    }
    catch(boost::program_options::required_option& e)
    {
        ALICEVISION_CERR("ERROR: " << e.what());
        ALICEVISION_COUT("Usage:\n\n" << allParams);
        return EXIT_FAILURE;
    }
    catch(boost::program_options::error& e)
    {
        ALICEVISION_CERR("ERROR: " << e.what());
        ALICEVISION_COUT("Usage:\n\n" << allParams);
        return EXIT_FAILURE;
    }

    ALICEVISION_COUT("Program called with the following parameters:");
    ALICEVISION_COUT(vm);

    // set verbose level
    aliceVision::system::Logger::get()->setLogLevel(verboseLevel);

    const std::size_t nbCameras = mediaPaths.size();

    // check output folder and update to its absolute path
    {
        const fs::path outDir = fs::absolute(outputFolder);
        outputFolder = outDir.string();
        if(!fs::is_directory(outDir))
        {
            ALICEVISION_LOG_ERROR("Cannot find folder: " << outputFolder);
            return EXIT_FAILURE;
        }
    }

    if (minFrameStep <= 0)
    {
        ALICEVISION_LOG_ERROR("Min frame step must be at least 1");
        return EXIT_FAILURE;
    }

    if(nbCameras < 1)
    {
        ALICEVISION_LOG_ERROR("Program need at least one media path.");
        return EXIT_FAILURE;
    }

    //Put default value is not filled
    brands.resize(nbCameras);
    models.resize(nbCameras);
    mmFocals.resize(nbCameras);

    // initialize KeyframeSelector
    KeyframeSelector selector;

    // set algorithm parameters
    selector.setMinFrameStep(minFrameStep);
    selector.setMaxOutFrame(maxNbOutFrame);

    // process
    if(useSimpleMethod)
    {
        selector.processSimple(mediaPaths);
    }
    else
    {
        selector.processSmart(mediaPaths);
    }

    if (!selector.writeSelection(outputFolder, mediaPaths, brands, models, mmFocals))
    {
        ALICEVISION_LOG_ERROR("Impossible to write selection");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
