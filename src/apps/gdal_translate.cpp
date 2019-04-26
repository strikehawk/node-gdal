#include "../gdal_common.hpp"
#include "../gdal_dataset.hpp"
#include "gdal_translate.hpp"

#include <sstream>
#include <vector>
#include <string>
#include <gdal_translate_lib.cpp>
#include <gdalwarper.h>

namespace node_gdal
{
namespace
{
    // --- Custom error handling ---
    std::string generic_error = "";
    static CPLErrorHandler last_err_handler;
    static void CPL_STDCALL errorHandler(CPLErr eErrClass, int err_no, const char *msg)
    {
        generic_error = msg;

        if (last_err_handler)
        {
            last_err_handler(eErrClass, err_no, msg);
        }
        Nan::ThrowError(msg);
    }

    static void pushErrorHandler()
    {
        generic_error = "";
        last_err_handler = CPLSetErrorHandler(errorHandler);
    }

    static void popErrorHandler()
    {
        if (!last_err_handler)
            return;
        CPLSetErrorHandler(last_err_handler);
    }

    // Utility methods
    char **toCharArray(std::vector<std::string> vector)
    {
        char **list = NULL;
        for (size_t i = 0; i < vector.size(); i++)
        {
            list = CSLAddString(list, vector[i].c_str());
        }

        return list;
    }

    int parseBoundingBox(Local<Array> bbox, double (&dstArray)[4])
    {
        Local<Value> val;

        val = bbox->Get(0);
        if (!val->IsNumber())
        {
            Nan::ThrowError("Bbox array must only contain numbers");
            return 1;
        }
        dstArray[0] = val->NumberValue();

        val = bbox->Get(1);
        if (!val->IsNumber())
        {
            Nan::ThrowError("Bbox array must only contain numbers");
            return 1;
        }
        dstArray[1] = val->NumberValue();

        val = bbox->Get(2);
        if (!val->IsNumber())
        {
            Nan::ThrowError("Bbox array must only contain numbers");
            return 1;
        }
        dstArray[2] = val->NumberValue();

        val = bbox->Get(3);
        if (!val->IsNumber())
        {
            Nan::ThrowError("Bbox array must only contain numbers");
            return 1;
        }
        dstArray[3] = val->NumberValue();

        return 0;
    }

    int convertBoundingBox(double bbox[4], OGRSpatialReference *srcSrs, OGRSpatialReference *dstSrs, double (&dstArray)[4])
    {
        double minx = bbox[0];
        double miny = bbox[3];
        double maxx = bbox[2];
        double maxy = bbox[1];

        OGRCoordinateTransformation *poCT;
        poCT = OGRCreateCoordinateTransformation(dstSrs, srcSrs);
        if (poCT == NULL)
        {
            Nan::ThrowError("Could not create transformation from destination to source projection.");
            return 1;
        }

        /** Convert bbox corners into src SRS **/
        double x, y;

        // Upper left corner
        double ul[2];
        x = minx;
        y = maxy;
        if (!poCT->Transform(1, &x, &y))
        {
            Nan::ThrowError("Could not create convert UL corner of the bounding box.");
            return 1;
        }
        ul[0] = x;
        ul[1] = y;

        // Lower left corner
        double ll[2];
        x = minx;
        y = miny;
        if (!poCT->Transform(1, &x, &y))
        {
            Nan::ThrowError("Could not create convert LL corner of the bounding box.");
            return 1;
        }
        ll[0] = x;
        ll[1] = y;

        // Upper right corner
        double ur[2];
        x = maxx;
        y = maxy;
        if (!poCT->Transform(1, &x, &y))
        {
            Nan::ThrowError("Could not create convert UR corner of the bounding box.");
            return 1;
        }
        ur[0] = x;
        ur[1] = y;

        // Lower right corner
        double lr[2];
        x = maxx;
        y = miny;
        if (!poCT->Transform(1, &x, &y))
        {
            Nan::ThrowError("Could not create convert LR corner of the bounding box.");
            return 1;
        }
        lr[0] = x;
        lr[1] = y;

        /** Compute src bbox from converted corners **/
        auto resultX = std::minmax({ul[0], ll[0], ur[0], lr[0]});
        auto resultY = std::minmax({ul[1], ll[1], ur[1], lr[1]});

        dstArray[0] = resultX.first;
        dstArray[1] = resultY.second;
        dstArray[2] = resultX.second;
        dstArray[3] = resultY.first;

        return 0;
    }

    std::vector<std::string> buildParametersVector(double bbox[4], int width, int height)
    {
        std::ostringstream ssMinx;
        ssMinx << bbox[0];

        std::ostringstream ssMiny;
        ssMiny << bbox[3];

        std::ostringstream ssMaxx;
        ssMaxx << bbox[2];

        std::ostringstream ssMaxy;
        ssMaxy << bbox[1];

        std::vector<std::string> vector;
        vector.push_back("-of");
        vector.push_back("MEM");
        vector.push_back("-projwin");
        vector.push_back(ssMinx.str().c_str());
        vector.push_back(ssMaxy.str().c_str());
        vector.push_back(ssMaxx.str().c_str());
        vector.push_back(ssMiny.str().c_str());

        if (width != 0 || height != 0)
        {
            std::ostringstream ssWidth;
            ssWidth << width;

            std::ostringstream ssHeight;
            ssHeight << height;

            vector.push_back("-outsize");
            vector.push_back(ssWidth.str().c_str());
            vector.push_back(ssHeight.str().c_str());
        }

        return vector;
    }

    GDALDatasetH extractWindowFromDataset(GDALDatasetH hSrcDs, char **argsArray)
    {
        if (hSrcDs == NULL)
        {
            LOG("Source dataset is null");
            return NULL;
        }

        const char *resultName = "result";

        pushErrorHandler();

        // create translate options
        GDALTranslateOptions *options = GDALTranslateOptionsNew(argsArray, NULL);
        if (options == NULL)
        {
            LOG("Options are null");
            return NULL;
        }

        GDALDatasetH hOutDs = GDALTranslate(resultName, hSrcDs, options, NULL);
        if (hOutDs == NULL)
        {
            LOG("Output dataset is null");
            return NULL;
        }

        // close dataset
        GDALClose(hSrcDs);

        // free options
        GDALTranslateOptionsFree(options);

        popErrorHandler();

        return hOutDs;
    }

    GDALDatasetH createWarpOutputDataset(GDALDatasetH hSrcDs, const char *srcWkt, const char *dstWkt)
    {
        LOG("Creating output");

        // Create output with same datatype as first input band.
        GDALDataType eDT = GDALGetRasterDataType(GDALGetRasterBand(hSrcDs, 1));
        LOG("Got data type");

        LOG("Source SRS: %s", GDALGetProjectionRef(hSrcDs));

        // Create a transformer that maps from source pixel/line coordinates to destination georeferenced coordinates (not destination
        // pixel line).  We do that by omitting the destination dataset handle (setting it to NULL).
        pushErrorHandler();
        void *hTransformArg = GDALCreateGenImgProjTransformer(hSrcDs, srcWkt, NULL, dstWkt, TRUE, 1000.0, 0);
        popErrorHandler();

        if (!generic_error.empty()) 
        {
            LOG("Could not create a transformer from source to destination projection: %s", generic_error.c_str());
            Nan::ThrowError("Could not create a transformer from source to destination projection");
            return NULL;
        }

        if(hTransformArg == NULL) {
            LOG("Could not create a transformer from source to destination projection");
            Nan::ThrowError("Could not create a transformer from source to destination projection");
            return NULL;
        }
        LOG("Created transformer");

        // Get approximate output georeferenced bounds and resolution for dataset.
        double adfDstGeoTransform[6];
        int nPixels = 0, nLines = 0;
        CPLErr eErr = GDALSuggestedWarpOutput(hSrcDs, GDALGenImgProjTransform, hTransformArg, adfDstGeoTransform, &nPixels, &nLines);
        if (eErr != CPLE_None) {
            LOG("Could not approximate bounds of destination dataset");
            Nan::ThrowError("Could not approximate bounds of destination dataset");
            return NULL;
        }
        LOG("Ran GDALSuggestedWarpOutput");

        GDALDestroyGenImgProjTransformer(hTransformArg);

        // Create the output dataset.
        GDALDatasetH hDstDs = GDALCreate(GDALGetDatasetDriver(hSrcDs), "warped", nPixels, nLines, GDALGetRasterCount(hSrcDs), eDT, NULL);
        if(hDstDs == NULL) {
            LOG("An error occurred when trying to create output dataset");
            Nan::ThrowError("An error occurred when trying to create output dataset");
            return NULL;
        }
        LOG("Output Dataset OK");

        // Write out the projection definition.
        GDALSetProjection(hDstDs, dstWkt);
        GDALSetGeoTransform(hDstDs, adfDstGeoTransform);

        return hDstDs;
    }
} // namespace

void Translate::Initialize(Local<Object> target)
{
    Nan::SetMethod(target, "translate", translate);
}

NAN_METHOD(Translate::translate)
{
    Nan::HandleScope scope;

    std::string srcFilePath("");
    std::string dstProj("");
    Local<Array> bbox;
    int width;
    int height;

    NODE_ARG_STR(0, "srcFilePath", srcFilePath);
    NODE_ARG_STR(1, "destinationProj", dstProj);
    NODE_ARG_ARRAY(2, "bbox", bbox);
    NODE_ARG_INT(3, "width", width);
    NODE_ARG_INT(4, "height", height);

    if (srcFilePath == "")
    {
        Nan::ThrowError("Source file path cannot be empty");
        return;
    }

    if (dstProj == "")
    {
        Nan::ThrowError("Destination projection cannot be empty");
        return;
    }

    if (bbox->Length() != 4)
    {
        Nan::ThrowError("Bounding box should contain 4 elements");
        return;
    }

    if (width < 0)
    {
        Nan::ThrowError("Width must be positive");
        return;
    }

    if (height < 0)
    {
        Nan::ThrowError("Height must be positive");
        return;
    }

    // Create source dataset from source file path
    GDALAccess access = GA_ReadOnly;

    pushErrorHandler();
    GDALDatasetH hSrcDs = GDALOpenEx(srcFilePath.c_str(), GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL, NULL, NULL);
    popErrorHandler();

    if (!generic_error.empty())
    {
        Nan::ThrowError(generic_error.c_str());
        return;
    }

    if (hSrcDs == NULL)
    {
        Nan::ThrowError("Source dataset is null.");
        return;
    }

    // Get projection of source dataset
    const char *srcWkt = GDALGetProjectionRef(hSrcDs);

    if (EQUAL(srcWkt, ""))
    {
        Nan::ThrowError("Source dataset projection is empty.");
        return;
    }

    OGRSpatialReference *srcSrs = new OGRSpatialReference(srcWkt);

    // Parse target projection
    OGRSpatialReference *dstSrs = new OGRSpatialReference();
    OGRErr ogrErr = dstSrs->SetFromUserInput(dstProj.c_str());
    if (ogrErr != OGRERR_NONE)
    {
        Nan::ThrowError("Could not parse destination projection.");
        return;
    }

    char *dstWkt;
    ogrErr = dstSrs->exportToWkt(&dstWkt);
    if (ogrErr != OGRERR_NONE)
    {
        Nan::ThrowError("Error when exporting destination projection as WKT.");
        return;
    }

    // Parse bbox
    double bboxArray[4];
    if (parseBoundingBox(bbox, bboxArray) != 0)
    {
        Nan::ThrowError("Could not parse bounding box.");
        return;
    }

    std::vector<std::string> vector;
    char **argsArray;
    GDALDatasetH ds;

    if (!srcSrs->IsSame(dstSrs))
    {
        LOG("Source and Target SRS are different");

        // Source and Target SRS are different
        double convertedBbox[4];
        if (convertBoundingBox(bboxArray, srcSrs, dstSrs, convertedBbox) != 0)
        {
            Nan::ThrowError("Could not convert bounding box.");
            return;
        }
        LOG("Converted bbox: %f %f %f %f", convertedBbox[0], convertedBbox[1], convertedBbox[2], convertedBbox[3]);

        /** Extract window from source corresponding to converted bbox **/
        vector = buildParametersVector(convertedBbox, width, height);
        argsArray = toCharArray(vector);
        ds = extractWindowFromDataset(hSrcDs, argsArray);
        if (ds == NULL) 
        {
            LOG("First extract is NULL.");
            Nan::ThrowError("First extract is NULL.");
            return;
        }
        LOG("First extract OK");

        /** Reproject window **/
        GDALDatasetH hWarpedDs = createWarpOutputDataset(ds, srcWkt, dstWkt);
        if (hWarpedDs == NULL)
        {
            LOG("Could not create output dataset from Warp operation.");
            Nan::ThrowError("Could not create output dataset from Warp operation.");
            return;
        }
        LOG("Create Warp output OK");

        CPLErr err = GDALReprojectImage(ds, srcWkt, hWarpedDs, dstWkt, GRA_Bilinear, 0.0, 0.0, NULL, NULL, NULL);
        if(err) {
            NODE_THROW_CPLERR(err);
            return;
        }
        LOG("Warping OK");

        /** Read bbox from reprojected window **/
        vector = buildParametersVector(bboxArray, width, height);
        argsArray = toCharArray(vector);
        ds = extractWindowFromDataset(hWarpedDs, argsArray);
        LOG("Extract from warped OK");
    }
    else
    {
        vector = buildParametersVector(bboxArray, width, height);
        argsArray = toCharArray(vector);
        ds = extractWindowFromDataset(hSrcDs, argsArray);
    }

    info.GetReturnValue().Set(Dataset::New((GDALDataset *)ds));
}
} // namespace node_gdal