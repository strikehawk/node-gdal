#include "../gdal_common.hpp"
#include "../gdal_dataset.hpp"
#include "gdal_translate.hpp"

#include <sstream>
#include <vector>
#include <string>
#include <gdal_translate_lib.cpp>

namespace node_gdal {
    namespace {
        // --- Custom error handling ---
        static CPLErrorHandler last_err_handler;
        static void CPL_STDCALL errorHandler(CPLErr eErrClass, int err_no, const char *msg) {
            if (last_err_handler) {
                last_err_handler(eErrClass, err_no, msg);
            }
            Nan::ThrowError(msg);
        }

        static void pushErrorHandler() {
            last_err_handler = CPLSetErrorHandler(errorHandler);
        }

        static void popErrorHandler() {
            if(!last_err_handler) return;
            CPLSetErrorHandler(last_err_handler);
        }

        // Utility methods
        char** toCharArray(std::vector<std::string> vector)
        {
            char** list = NULL;
            for(size_t i = 0; i < vector.size(); i++)
            {
                list = CSLAddString(list, vector[i].c_str());
            }
            
            return list;
        }

        std::vector<std::string> buildParametersVector(Local<Array> bbox, int width, int height)
        {
            Local<Value> val;
            double minx;
            double miny;
            double maxx;
            double maxy;

            val = bbox->Get(0);
            if (!val->IsNumber()) {
                Nan::ThrowError("Bbox array must only contain numbers");
                return {};
            }
            minx = val->NumberValue();

            val = bbox->Get(1);
            if (!val->IsNumber()) {
                Nan::ThrowError("Bbox array must only contain numbers");
                return {};
            }
            miny = val->NumberValue();

            val = bbox->Get(2);
            if (!val->IsNumber()) {
                Nan::ThrowError("Bbox array must only contain numbers");
                return {};
            }
            maxx = val->NumberValue();

            val = bbox->Get(3);
            if (!val->IsNumber()) {
                Nan::ThrowError("Bbox array must only contain numbers");
                return {};
            }
            maxy = val->NumberValue();

            std::ostringstream ssMinx;
            ssMinx << minx;

            std::ostringstream ssMiny;
            ssMiny << miny;

            std::ostringstream ssMaxx;
            ssMaxx << maxx;

            std::ostringstream ssMaxy;
            ssMaxy << maxy;

            std::vector<std::string> vector;
            vector.push_back("-of");
            vector.push_back("MEM");
            vector.push_back("-projwin");
            vector.push_back(ssMinx.str().c_str());
            vector.push_back(ssMaxy.str().c_str());
            vector.push_back(ssMaxx.str().c_str());
            vector.push_back(ssMiny.str().c_str());
            // vector.push_back("-tr");
            // vector.push_back(ssResolution.str().c_str());
            // vector.push_back(ssResolution.str().c_str());

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

        GDALDataset* translateDataset(std::string srcFilePath, char** argsArray, Local<Array> array) 
        {
            // const char *resultName = "C:\\Projects\\GitHub\\result.tif";
            const char *resultName = "result";
            
            pushErrorHandler();

            // create source dataset from source file path
            GDALAccess access = GA_ReadOnly;
            GDALDatasetH hSrcDs = GDALOpenEx(srcFilePath.c_str(), GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL, NULL, NULL);

            if (hSrcDs == NULL)
            {
                LOG("Source dataset is null");
                return NULL;
            }

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

            return (GDALDataset*) hOutDs;
        }
    }

    void Translate::Initialize(Local<Object> target)
    {
        Nan::SetMethod(target, "translate", translate);
    }

    NAN_METHOD(Translate::translate)
    {
        Nan::HandleScope scope;

        std::string srcFilePath("");
        Local<Array> bbox;
        int width;
        int height;

        NODE_ARG_STR(0, "srcFilePath", srcFilePath);
        NODE_ARG_ARRAY(1, "bbox", bbox);
        NODE_ARG_INT(2, "width", width);
        NODE_ARG_INT(3, "height", height);

        if (srcFilePath == "") {
            Nan::ThrowError("Source file path cannot be empty");
			return;
        }

        if (bbox->Length() != 4) {
            Nan::ThrowError("Bounding box should contain 4 elements");
			return;
        }

        if (width < 0) {
            Nan::ThrowError("Width must be positive");
			return; 
        }
        
        if (height < 0) {
            Nan::ThrowError("Height must be positive");
			return; 
        }

        std::vector<std::string> vector = buildParametersVector(bbox, width, height);
        if (vector.empty()) return;

        char** argsArray = toCharArray(vector);

        Local<Array> array = Nan::New<Array>(0);
        GDALDataset* ds = translateDataset(srcFilePath, argsArray, array);

        LOG("Ready to return");
        info.GetReturnValue().Set(Dataset::New(ds));
    }
} // namespace node_gdal