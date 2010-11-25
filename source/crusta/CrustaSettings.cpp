#include <crusta/CrustaSettings.h>


#include <iostream>

#include <Misc/ConfigurationFile.h>
#include <Misc/File.h>
#include <Misc/StandardValueCoders.h>
#include <GL/GLValueCoders.h>


BEGIN_CRUSTA


CrustaSettings::CrustaSettings() :
    globeName("Sphere_Earth"), globeRadius(6371000.0),

    decorateVectorArt(false),

    terrainDefaultHeight(0.0f),
    terrainDefaultColor(0.5f, 0.5f, 0.5f, 1.0f),
    terrainDefaultLayerfData(0.0f),

    terrainAmbientColor(0.4f, 0.4f, 0.4f, 1.0f),
    terrainDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f),
    terrainEmissiveColor(0.0f, 0.0f, 0.0f, 1.0f),
    terrainSpecularColor(0.3f, 0.3f, 0.3f, 1.0f),
    terrainShininess(55.0f),

    cacheMainNodeSize(4096),
    cacheMainGeometrySize(4096),
    cacheMainLayerfSize(4096),
    cacheGpuGeometrySize(1024),
    cacheGpuLayerfSize(1024),
    cacheGpuCoverageSize(1024),
    cacheGpuLineDataSize(1024),

    dataManMaxDataLayers(32),
    dataManMaxFetchRequests(16),

    lineDataTexSize(8192),
    lineDataCoordStep(1.0f / lineDataTexSize),
    lineDataStartCoord(0.5f * lineDataCoordStep),
    lineCoverageTexSize(TILE_RESOLUTION>>1)
{
}

void CrustaSettings::
loadFromFiles(const Strings& cfgNames)
{
    Misc::ConfigurationFile cfgFile;

    //try to load the default system-wide configuration
    try
    {
        cfgFile.merge((std::string(CRUSTA_SHARE_PATH) +
                       std::string("crustaSettings.cfg")).c_str());
    }
    catch (Misc::File::OpenError e) {
        //ignore this exception
    }
    catch (std::runtime_error e) {
        std::cout << "Caught exception " << e.what() << " while reading " <<
                     "the system-wide crustaSettings.cfg configuration file." <<
                     std::endl;
    }

    //try to load the default local configuration file
    try
    {
        cfgFile.merge("crustaSettings.cfg");
    }
    catch (Misc::File::OpenError e) {
        //ignore this exception
    }
    catch (std::runtime_error e) {
        std::cout << "Caught exception " << e.what() << " while reading " <<
                     "the local crustaSettings.cfg configuration file." <<
                     std::endl;
    }

    //try to load the specified configuration files
    for (Strings::const_iterator it=cfgNames.begin(); it!=cfgNames.end(); ++it)
    {
        try {
            cfgFile.merge(it->c_str());
        }
        catch (std::runtime_error e) {
            std::cout << "Caught exception " << e.what() << " while reading " <<
                         "the following configuration file: " << *it <<
                         std::endl;
        }
    }


    //try to extract the globe specifications
    cfgFile.setCurrentSection("/Crusta/Globe");
    globeName   = cfgFile.retrieveString("./name", globeName);
    globeRadius = cfgFile.retrieveValue<double>("./radius", globeRadius);

    //try to extract the terrain properties
    cfgFile.setCurrentSection("/Crusta/Terrain");
    terrainDefaultHeight = cfgFile.retrieveValue<float>(
        "./defaultHeight", terrainDefaultHeight);
    terrainDefaultColor = cfgFile.retrieveValue<Color>(
        "./defaultColor", terrainDefaultColor);
    terrainDefaultLayerfData = cfgFile.retrieveValue<float>(
        "./defaultLayerfData", terrainDefaultLayerfData);

    terrainAmbientColor = cfgFile.retrieveValue<Color>(
        "./ambientColor", terrainAmbientColor);
    terrainDiffuseColor = cfgFile.retrieveValue<Color>(
        "./diffuseColor", terrainDiffuseColor);
    terrainEmissiveColor = cfgFile.retrieveValue<Color>(
        "./emissiveColor", terrainEmissiveColor);
    terrainSpecularColor = cfgFile.retrieveValue<Color>(
        "./specularColor", terrainSpecularColor);
    terrainShininess = cfgFile.retrieveValue<double>(
        "./shininess", terrainShininess);

    //try to extract the cache settings
    cfgFile.setCurrentSection("/Crusta/Cache");
    cacheMainNodeSize = cfgFile.retrieveValue<int>(
        "./mainNodeSize", cacheMainNodeSize);
    cacheMainGeometrySize = cfgFile.retrieveValue<int>(
        "./mainGeometrySize", cacheMainGeometrySize);
    cacheMainLayerfSize = cfgFile.retrieveValue<int>(
        "./mainLayerfSize", cacheMainLayerfSize);
    cacheGpuGeometrySize = cfgFile.retrieveValue<int>(
        "./gpuGeometrySize", cacheGpuGeometrySize);
    cacheGpuLayerfSize = cfgFile.retrieveValue<int>(
        "./gpuLayerfSize", cacheGpuLayerfSize);
    cacheGpuCoverageSize = cfgFile.retrieveValue<int>(
        "./gpuCoverageSize", cacheGpuCoverageSize);
    cacheGpuLineDataSize = cfgFile.retrieveValue<int>(
        "./gpuLineDataSize", cacheGpuLineDataSize);

    //try to extract the data manager settings
    cfgFile.setCurrentSection("/Crusta/DataManager");
    dataManMaxFetchRequests = cfgFile.retrieveValue<int>(
        "./maxFetchRequests", dataManMaxFetchRequests);

    //try to extract the line decoration settings
    cfgFile.setCurrentSection("/Crusta/DecoratedArt");
    decorateVectorArt = cfgFile.retrieveValue<bool>(
        "./decorateVectorArt", decorateVectorArt);
    lineDataTexSize = cfgFile.retrieveValue<int>(
        "./dataTexSize", lineDataTexSize);
    lineDataCoordStep  = 1.0f / lineDataTexSize;
    lineDataStartCoord = 0.5f * lineDataCoordStep;
}


CrustaSettings* SETTINGS;


END_CRUSTA
