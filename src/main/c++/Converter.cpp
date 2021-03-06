#include "Converter.h"
#include "OsmWorld.h"
#include "My3DWorld.h"
#include "OsmBuildingManager.h"
#include "OsmGround.h"
#include "osm/OsmUtil.h"
#include <cmath>
#include "Log.h"
#include <math.h>       /* sqrt */

#include <boost/algorithm/string/replace.hpp>

void Converter::osmWorld23DWorld() {
}

/**
 *                                    
 *              +--------->          
 *              |       X            
 *              |                    
 *              |                    
 * lat  ^       | Y                  
 *      |       v                    
 *      |                            
 *      |    A       B               
 *      |     +-----+    lat=lat_A   
 *      |     |     |                
 *      |     |     |    lon=lon_A   
 *      |     +-----+                
 *      |    D       C               
 *      |                            
 *      +----------------->          
 *                      long         
 *                                    
 */

void Converter::osmWorld23DGround() {
    if (
            OsmGround::getInstance()->zoom_ &&
            OsmGround::getInstance()->xTile_ &&
            OsmGround::getInstance()->yTile_) {
        double x0 = RADIUS * (OsmGround::getInstance()->minGround.second - OsmWorld::getInstance()->origin_.second) * (M_PI / 180);
        double x1 = RADIUS * (OsmGround::getInstance()->maxGround.second - OsmWorld::getInstance()->origin_.second) * (M_PI / 180);
        double z0 = RADIUS * (OsmWorld::getInstance()->origin_.first - OsmGround::getInstance()->minGround.first) * (M_PI / 180);
        double z1 = RADIUS * (OsmWorld::getInstance()->origin_.first - OsmGround::getInstance()->maxGround.first) * (M_PI / 180);
        shared_ptr<My3DGroundTile> my3DGroundTile(new My3DGroundTile(
                make_pair(x0, z0), make_pair(x1, z1),
                *(OsmGround::getInstance()->xTile_),
                *(OsmGround::getInstance()->yTile_),
                *(OsmGround::getInstance()->zoom_)));
        My3DGround::getInstance()->tiles_.push_back(my3DGroundTile);
    } else {

        // Square from selected area
        double latMin = OsmGround::getInstance()->minGround.first;
        double latMax = OsmGround::getInstance()->maxGround.first;
        double lonMin = OsmGround::getInstance()->minGround.second;
        double lonMax = OsmGround::getInstance()->maxGround.second;
        My3DGround::getInstance()->minGround_ = make_pair(
                RADIUS * (lonMin - OsmWorld::getInstance()->origin_.second) * (M_PI / 180),
                RADIUS * (OsmWorld::getInstance()->origin_.first - latMin) * (M_PI / 180));
        My3DGround::getInstance()->maxGround_ = make_pair(
                RADIUS * (lonMax - OsmWorld::getInstance()->origin_.second) * (M_PI / 180),
                RADIUS * (OsmWorld::getInstance()->origin_.first - latMax) * (M_PI / 180));



        double phi = max(lonMax - lonMin, latMax - latMin);
        // Take the integer part of decimal zoom
        int minZoom = (int) (log2(360 / phi) - 0.5);
        int zoom = minZoom + 2;
        int xTileStart = OsmUtil::long2tilex(lonMin, zoom);
        int xTile = xTileStart;
        int yTile = OsmUtil::lat2tiley(latMax, zoom);
        double lonStart = OsmUtil::tilex2long(xTile, zoom);
        double lon = lonStart;
        double lat = OsmUtil::tiley2lat(yTile, zoom);
        //    FILE_LOG(logINFO) << "Converter::osmWorld23DGround - zoom: " << zoom;
        //    FILE_LOG(logINFO) << "Converter::osmWorld23DGround - xTile: " << xTile;
        //    FILE_LOG(logINFO) << "Converter::osmWorld23DGround - yTile: " << yTile;
        //    FILE_LOG(logINFO) << "Converter::osmWorld23DGround - latMin: " << latMin;
        //    FILE_LOG(logINFO) << "Converter::osmWorld23DGround - lonMax: " << lonMax;

        //    FILE_LOG(logINFO) << "Converter::osmWorld23DGround - lat: " << lat;
        //    FILE_LOG(logINFO) << "Converter::osmWorld23DGround - lon: " << lon;
        while (lat > latMin) {
            //        FILE_LOG(logINFO) << "Converter::osmWorld23DGround - lat: " << lat;
            double nextLat = OsmUtil::tiley2lat(yTile + 1, zoom);
            while (lon < lonMax) {
                //            FILE_LOG(logINFO) << "Converter::osmWorld23DGround - lon: " << lon;
                double nextLon = OsmUtil::tilex2long(xTile + 1, zoom);



                double x0 = RADIUS * (lon - OsmWorld::getInstance()->origin_.second) * (M_PI / 180);
                double x1 = RADIUS * (nextLon - OsmWorld::getInstance()->origin_.second) * (M_PI / 180);
                double z0 = RADIUS * (OsmWorld::getInstance()->origin_.first - lat) * (M_PI / 180);
                double z1 = RADIUS * (OsmWorld::getInstance()->origin_.first - nextLat) * (M_PI / 180);


                shared_ptr<My3DGroundTile> my3DGroundTile(new My3DGroundTile(
                        make_pair(x0, z0), make_pair(x1, z1), xTile, yTile, zoom));

                My3DGround::getInstance()->tiles_.push_back(my3DGroundTile);

                xTile++;
                lon = nextLon;
            }
            lon = lonStart;
            xTile = xTileStart;
            yTile++;
            lat = nextLat;
        }
    }
}

void Converter::osmWorld23DBuildings() {
    // For each building
    for (
            unordered_map<int, shared_ptr < OsmBuilding >> ::iterator osmBuildingIt = OsmBuildingManager::osmBuildings_.begin();
            osmBuildingIt != OsmBuildingManager::osmBuildings_.end();
            ++osmBuildingIt) {
        shared_ptr < OsmBuilding > osmBuilding(osmBuildingIt->second);
        shared_ptr < My3DBuilding > my3DBuilding(new My3DBuilding(osmBuilding->getId()));

        FILE_LOG(logINFO) << "Converter::osmWorld23DBuildings - osmBuilding->name_: "
                << "\"" << osmBuilding->name_ << "\"";
        if (!osmBuilding->name_.empty()) {
            string name = "_" + osmBuilding->name_;
            boost::replace_all(name, " ", "_");
            boost::replace_all(name, "&", "_");
            boost::replace_all(name, ":", "-");
            my3DBuilding->name_ = name;
        } else {
            stringstream ss;
            ss << osmBuilding->getId();
            my3DBuilding->name_ = ss.str();
        }
        FILE_LOG(logINFO) << "Converter::osmWorld23DBuildings - my3DBuilding->name_: "
                << my3DBuilding->name_;

        // For each building part
        for (
                vector < shared_ptr < OsmBuildingPart >> ::iterator osmBuildingPartIt = osmBuilding->osmBuildingParts_.begin();
                osmBuildingPartIt != osmBuilding->osmBuildingParts_.end();
                ++osmBuildingPartIt) {
            string buildingPartColour = OsmUtil::osmColor2x3DColor(
                    (*osmBuildingPartIt)->colour_);

            shared_ptr < vector < Node> > osmNodes = (*osmBuildingPartIt)->nodes_;
            shared_ptr < vector < pair<double, double >> > buildingPartPoints(new vector < pair<double, double >>);
            double perimeter = 0.0;
            // For each node
            for (
                    vector < Node> ::iterator nodeIt = osmNodes->begin();
                    nodeIt != osmNodes->end();
                    ++nodeIt) {
                double z = RADIUS * (OsmWorld::getInstance()->origin_.first - nodeIt->lat_) * (M_PI / 180);
                double x = RADIUS * (nodeIt->lon_ - OsmWorld::getInstance()->origin_.second) * (M_PI / 180);
                buildingPartPoints->push_back(make_pair(x, z));
                perimeter += sqrt(pow(x, 2.0) + pow(z, 2.0));
            }

            // Building part roof
            boost::optional<double> osmRoofHeight;
            if ((*osmBuildingPartIt)->osmRoof_->optHeight_) {
                osmRoofHeight = *((*osmBuildingPartIt)->osmRoof_->optHeight_);
            }
            boost::optional<double> osmBuildingPartHeight;
            if ((*osmBuildingPartIt)->optHeight_) {
                osmBuildingPartHeight = *((*osmBuildingPartIt)->optHeight_);
            }
            double roofElevation = 0;
            if (osmBuildingPartHeight && osmRoofHeight) {
                roofElevation = *osmBuildingPartHeight - *osmRoofHeight;
                FILE_LOG(logINFO) << "Converter::osmWorld23DBuildings - optBuildingPartHeight: " << *osmBuildingPartHeight;
                FILE_LOG(logINFO) << "Converter::osmWorld23DBuildings - optRoofHeight: " << *osmRoofHeight;
            }
            shared_ptr<My3DRoof> my3DBuildingPartRoof(new My3DRoof(
                    buildingPartPoints,
                    roofElevation,
                    Shape::getEnumFromString((*osmBuildingPartIt)->osmRoof_->shape_),
                    "0 0 0"));
            //                    OsmUtil::osmColor2x3DColor((*osmBuildingPartIt)->osmRoof_->colour_)

            if (osmRoofHeight) {
                my3DBuildingPartRoof->optHeight_ = *osmRoofHeight;
            }
            my3DBuilding->perimeter_ = perimeter;
            //            double buildingPartHeight;
            boost::optional<double> my3DBuildingPartHeight;
            if (osmBuildingPartHeight) {
                my3DBuildingPartHeight = *osmBuildingPartHeight
                        - (*osmBuildingPartIt)->minHeight_;
                if (osmRoofHeight) {
                    my3DBuildingPartHeight = *my3DBuildingPartHeight - *osmRoofHeight;
                    FILE_LOG(logINFO) << "Converter::osmWorld23DBuildings - *osmRoofHeight: " << *osmRoofHeight;
                }
                FILE_LOG(logINFO) << "Converter::osmWorld23DBuildings - *osmBuildingPartHeight: " << *osmBuildingPartHeight;
                FILE_LOG(logINFO) << "Converter::osmWorld23DBuildings - (*osmBuildingPartIt)->minHeight_): " << (*osmBuildingPartIt)->minHeight_;
                FILE_LOG(logINFO) << "Converter::osmWorld23DBuildings - *my3DBuildingPartHeight: " << *my3DBuildingPartHeight;
            }
            FILE_LOG(logDEBUG)
                    << "Converter::osmWorld23DBuildings - (*osmBuildingPartIt)->minLevel_:"
                    << (*osmBuildingPartIt)->minLevel_;

            // Add my3DBuildingPart
            double buildingPartElevation = (*osmBuildingPartIt)->minHeight_;
            shared_ptr<My3DBuildingPart> my3DBuildingPart(new My3DBuildingPart(
                    buildingPartPoints,
                    //                    buildingPartHeight,
                    buildingPartElevation,
                    buildingPartColour,
                    my3DBuildingPartRoof));
            if (my3DBuildingPartHeight) {
                my3DBuildingPart->optHeight_ = *my3DBuildingPartHeight;
            }
            (*my3DBuilding).addBuildingPart(my3DBuildingPart);

            if ((*osmBuildingPartIt)->levels_ && my3DBuildingPartHeight) {

                my3DBuildingPart->transparency_ = 0.6;


                double floorHeight = *my3DBuildingPartHeight
                        / (double) (*((*osmBuildingPartIt)->levels_) - (*osmBuildingPartIt)->minLevel_);
                FILE_LOG(logDEBUG)
                        << "Converter::osmWorld23DBuildings - (*osmBuildingPartIt)->levels_:"
                        << (*osmBuildingPartIt)->levels_;
                for (
                        int level = (*osmBuildingPartIt)->minLevel_;
                        level < (*osmBuildingPartIt)->levels_;
                        level++) {
                    double floorElevation = buildingPartElevation + (double) ((level - (*osmBuildingPartIt)->minLevel_) * floorHeight);
                    shared_ptr<My3DBuildingFloorPart> my3DBuildingFloorPart(
                            new My3DBuildingFloorPart(
                            buildingPartPoints,
                            floorElevation,
                            buildingPartColour));
                    my3DBuildingFloorPart->optHeight_ = floorHeight;
                    FILE_LOG(logDEBUG)
                            << "Converter::osmWorld23DBuildings - Level " << level
                            << ": elevation=" << floorElevation
                            << " / " << "minHeight=" << (*osmBuildingPartIt)->minHeight_
                            << " / " << "minLevel=" << (*osmBuildingPartIt)->minLevel_
                            << " / " << "height=" << floorHeight;
                    //                }
                    (*my3DBuilding).addLevel(level, my3DBuildingFloorPart);
                }
            }
        }
        My3DWorld::getInstance()->addBuilding(my3DBuilding);
    }
}
